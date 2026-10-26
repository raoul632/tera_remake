/* Oracle facultatif C : executer 7868..795D dans le COM original avec
 * Unicorn natif. Aucun interpreteur Python, aucune interface. Les hooks
 * remplacent uniquement clavier, dessin, son et attente ; RANDOM execute
 * ses instructions originales et sa vraie mise a jour de DS:01FC. */
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unicorn/unicorn.h>
#include "tera/exploration.h"
#include "tera/space.h"

enum { DATA=0x10000, STACK=0xe000, STOP=0xf100 };
typedef struct {
    unsigned stop,frames,draws,purged;
    int pending,failed;
} Script;
static void checked(uc_err error)
{
    if(error!=UC_ERR_OK) {fprintf(stderr,"Unicorn: %s\n",uc_strerror(error));assert(0);}
}
static uint16_t reg16(uc_engine *uc,int reg)
{
    uint16_t value=0;checked(uc_reg_read(uc,reg,&value));return value;
}
static void set16(uc_engine *uc,int reg,uint16_t value)
{
    checked(uc_reg_write(uc,reg,&value));
}
static uint16_t word_at(uc_engine *uc,uint32_t address)
{
    unsigned char bytes[2];checked(uc_mem_read(uc,address,bytes,2));
    return (uint16_t)(bytes[0]+256u*bytes[1]);
}
static void return_io(uc_engine *uc,unsigned cleanup)
{
    uint16_t sp=reg16(uc,UC_X86_REG_SP),ip=word_at(uc,sp);
    set16(uc,UC_X86_REG_SP,(uint16_t)(sp+2+cleanup));set16(uc,UC_X86_REG_IP,ip);
}
static void instruction(uc_engine *uc,uint64_t address,uint32_t size,void *context)
{
    Script *s=context;(void)size;
    if(address==0x10da) {
        static const unsigned bounds[]={60,24,255};
        if(reg16(uc,UC_X86_REG_AX)!=bounds[s->draws%3]) s->failed=1;
        ++s->draws;return; /* Ne pas substituer le resultat du RNG. */
    }
    if(address==0x0da4) {
        uint16_t caller=word_at(uc,reg16(uc,UC_X86_REG_SP));
        uint32_t flags=0;int pressed;
        if(caller==0x7931) {
            ++s->frames;
            if(s->stop && s->frames==s->stop) s->pending=1;
        } else if(caller!=0x7875) {s->failed=1;checked(uc_emu_stop(uc));return;}
        pressed=s->pending;set16(uc,UC_X86_REG_AX,(uint16_t)pressed);
        checked(uc_reg_read(uc,UC_X86_REG_EFLAGS,&flags));
        flags=pressed ? flags&~UINT32_C(64) : flags|UINT32_C(64);
        checked(uc_reg_write(uc,UC_X86_REG_EFLAGS,&flags));return_io(uc,0);return;
    }
    if(address==0x253b) {s->pending=0;++s->purged;return_io(uc,2);return;}
    if(address==0x2762) {return_io(uc,2);return;}
    if(address==0x2dc3) {return_io(uc,12);return;}
    if(address==0x2dc6) {return_io(uc,8);return;}
    if(address==0x5051) {return_io(uc,6);return;}
    if(address==0x1022 || address==0x0213 || address==0x059c || address==0x05c3)
        return_io(uc,0);
}
static int probe(void *context,TeraInputBoundary boundary,unsigned frame)
{
    Script *s=context;
    if(boundary==TERA_INPUT_PURGE) {s->pending=0;++s->purged;return 0;}
    assert(frame==++s->frames);
    if(s->stop && frame==s->stop) s->pending=1;
    return s->pending;
}
static void compare(const unsigned char *image,size_t length,uint32_t seed,unsigned stop,int sound)
{
    uc_engine *uc;uc_hook hook;Script binary={stop,0,0,0,1,0},port=binary;
    TeraCombatGame game={0};TeraInputSource input={probe,&port};unsigned calls=0,i;
    unsigned char bytes[4],ret[2]={(unsigned char)STOP,(unsigned char)(STOP>>8)};
    uint32_t actual=0;uint16_t parent=0xd000;
    checked(uc_open(UC_ARCH_X86,UC_MODE_16,&uc));
    checked(uc_mem_map(uc,0,0x20000,UC_PROT_ALL));
    checked(uc_mem_write(uc,0x100,image,length));
    set16(uc,UC_X86_REG_DS,DATA>>4);set16(uc,UC_X86_REG_BP,parent);
    set16(uc,UC_X86_REG_SP,STACK);checked(uc_mem_write(uc,STACK,ret,2));
    for(i=0;i<4;++i) bytes[i]=(unsigned char)(seed>>(8*i));
    checked(uc_mem_write(uc,DATA+0x1fc,bytes,4));bytes[0]=(unsigned char)sound;
    checked(uc_mem_write(uc,DATA+0xa74a,bytes,1));
    /* L'API C externe prend un void* pour ses callbacks. Cette conversion
     * est l'ABI Unicorn de la cible native, pas une donnee du moteur. */
    checked(uc_hook_add(uc,&hook,UC_HOOK_CODE,(void *)instruction,&binary,1,0));
    checked(uc_emu_start(uc,0x7868,STOP,1000000,100000));
    assert(reg16(uc,UC_X86_REG_IP)==STOP && reg16(uc,UC_X86_REG_SP)==STACK+2);
    assert(reg16(uc,UC_X86_REG_BP)==parent && !binary.failed);
    checked(uc_mem_read(uc,DATA+0x1fc,bytes,4));
    for(i=0;i<4;++i) actual|=(uint32_t)bytes[i]<<(8*i);
    game.random.seed=seed;game.sound=sound;game.input=&input;
    assert(tera_monster_presentation_random(&game,15,&calls));
    assert(binary.frames==(stop ? stop : 6) && binary.draws==3*binary.frames);
    assert(binary.frames==port.frames && binary.pending==port.pending);
    assert(binary.purged==1 && port.purged==1 && calls==binary.draws);
    assert(game.random.seed==actual);checked(uc_close(uc));
}
typedef struct {
    unsigned now,event_time,key,pending,draws,delays,purges;
    int delivered,finished;
} SpaceClock;
static void space_instruction(uc_engine *uc,uint64_t address,uint32_t size,void *context)
{
    SpaceClock *s=context;(void)size;
    if(address==0x10da) {
        static const unsigned bounds[]={10,3,3};
        assert(s->draws<3 && reg16(uc,UC_X86_REG_AX)==bounds[s->draws]);
        ++s->draws;return;
    }
    if(address==0x0213) {
        unsigned duration=reg16(uc,UC_X86_REG_AX);
        static const unsigned durations[]={300,230,200,170,90};
        if(s->delays) assert(s->delays<=5 && duration==durations[s->delays-1]);
        else assert(duration<=900 && duration%100==0);
        ++s->delays;s->now+=duration;
        if(!s->delivered && s->event_time<=s->now) {s->pending=s->key;s->delivered=1;}
        return_io(uc,0);return;
    }
    if(address==0x0da4) {
        uint16_t caller=word_at(uc,reg16(uc,UC_X86_REG_SP));uint32_t flags=0;
        if(caller==0xbafd) {s->finished=1;checked(uc_emu_stop(uc));return;}
        assert(caller==0xbfb5 || caller==0xc021);
        set16(uc,UC_X86_REG_AX,(uint16_t)(s->pending!=0));
        checked(uc_reg_read(uc,UC_X86_REG_EFLAGS,&flags));
        flags=s->pending ? flags&~UINT32_C(64) : flags|UINT32_C(64);
        checked(uc_reg_write(uc,UC_X86_REG_EFLAGS,&flags));return_io(uc,0);return;
    }
    if(address==0x253b) {s->pending=0;++s->purges;return_io(uc,2);return;}
    if(address==0x2762) {return_io(uc,2);return;}
    if(address==0x1022) {return_io(uc,0);return;}
    if(address==0x2db4 || address==0x5051) {return_io(uc,6);return;}
    if(address==0x2dc3) {return_io(uc,12);return;}
    if(address==0x2dc6) return_io(uc,8);
}
/* Isoler BF72..premier poll BAFD : les veritables branches de purge,
 * les six delais, les coordonnees et RANDOM sont executes. La resolution
 * des degats/pirates et le reste du vol restent des campagnes distinctes. */
static int compare_space(const unsigned char *image,size_t length,const unsigned char *overlay,
    uint32_t seed,int sound,int offset)
{
    static const unsigned pad[3][3]={{'7','8','9'},{'4','5','6'},{'1','2','3'}};
    TeraCharacterState state={0};TeraSpaceSession port;uc_engine *uc;uc_hook hook;
    SpaceClock clock={0};unsigned i,count,end;uint32_t binary_seed,actual=0;
    unsigned char bytes[4],parent[2]={0,0xd0};
    state.energy=100;state.combat.random.seed=seed;state.combat.sound=sound;
    assert(tera_space_start(&port,&state,8) && tera_space_key(&port,'N'));
    if(!port.delay_ms && offset<0) return 0;
    end=port.delay_ms+990;clock.event_time=(unsigned)((int)port.delay_ms+offset);
    clock.key=pad[port.alien_y+1][port.alien_x+1];
    /* La mise en scene du vol precede le premier R10 ; reproduire son
     * nombre de tirages etabli separement, puis executer le vrai RNG DOS. */
    count=96+32*(2+(sound ? 16 : 0));binary_seed=seed;
    for(i=0;i<count;++i) binary_seed=binary_seed*UINT32_C(129)+UINT32_C(0x361962e9);
    checked(uc_open(UC_ARCH_X86,UC_MODE_16,&uc));checked(uc_mem_map(uc,0,0x20000,UC_PROT_ALL));
    checked(uc_mem_write(uc,0x100,image,length));checked(uc_mem_write(uc,0xba21,overlay,0xe00));
    set16(uc,UC_X86_REG_DS,DATA>>4);set16(uc,UC_X86_REG_BP,0xd100);set16(uc,UC_X86_REG_SP,STACK);
    checked(uc_mem_write(uc,0xd0fe,parent,2));
    for(i=0;i<4;++i) bytes[i]=(unsigned char)(binary_seed>>(8*i));
    checked(uc_mem_write(uc,DATA+0x1fc,bytes,4));
    checked(uc_hook_add(uc,&hook,UC_HOOK_CODE,(void *)space_instruction,&clock,1,0));
    checked(uc_emu_start(uc,0xbf72,STOP,1000000,100000));
    assert(clock.finished && clock.draws==3 && clock.delays==6 && clock.now==end);
    assert((int16_t)word_at(uc,0xd000-12)==port.alien_x);
    assert((int16_t)word_at(uc,0xd000-14)==port.alien_y);
    checked(uc_mem_read(uc,DATA+0x1fc,bytes,4));
    for(i=0;i<4;++i) actual|=(uint32_t)bytes[i]<<(8*i);
    assert(actual==port.state.combat.random.seed);
    assert(tera_space_input_at(&port,clock.event_time,(int)clock.key));
    if(clock.event_time<end) assert(tera_space_input_at(&port,end,-1));
    assert(port.trials==1 && port.hit==(clock.pending!=0));
    assert(clock.purges==(offset<=300 ? 1u : 0u));
    checked(uc_close(uc));return 1;
}
int main(int argc,char **argv)
{
    static const uint32_t seeds[]={0,1,1986,UINT32_C(0x7fffffff),UINT32_MAX};
    static const int offsets[]={-1,0,1,299,300,301,529,530,531,729,730,731,899,900,901,989,990};
    unsigned char image[0xff00],overlay[0xe00];FILE *file;size_t length;unsigned i,stop,sound,cases=0;
    assert(argc==3);file=fopen(argv[1],"rb");
    if(!file) {puts("SKIP: TERA.COM absent");return 77;}
    length=fread(image,1,sizeof image,file);
    assert(length>0x795d-0x100 && !ferror(file) && fgetc(file)==EOF);
    assert(fclose(file)==0);
    /* Garde locale du point d'entree ; l'empreinte SHA-256 complete est
     * verifiee a la configuration CMake de ce banc facultatif. */
    assert(memcmp(image+0x7868-0x100,"\x55\x8b\xec\x55\xe9\0\0",7)==0);
    for(i=0;i<sizeof seeds/sizeof seeds[0];++i)
        for(stop=0;stop<=6;++stop) for(sound=0;sound<2;++sound) {
            compare(image,length,seeds[i],stop,(int)sound);++cases;
        }
    printf("BIN/C monster15: %u cases, zero mismatch; original RNG executed\n",cases);
    file=fopen(argv[2],"rb");assert(file);assert(fseek(file,0xae00,SEEK_SET)==0);
    assert(fread(overlay,1,sizeof overlay,file)==sizeof overlay);assert(fclose(file)==0);
    cases=0;
    for(i=0;i<sizeof seeds/sizeof seeds[0];++i)
        for(stop=0;stop<sizeof offsets/sizeof offsets[0];++stop) for(sound=0;sound<2;++sound) {
            cases+=(unsigned)compare_space(image,length,overlay,seeds[i],(int)sound,offsets[stop]);
        }
    printf("BIN/C space timing: %u cases, zero mismatch\n",cases);
    return 0;
}
