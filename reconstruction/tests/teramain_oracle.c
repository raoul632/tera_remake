/* Oracle d'integration TERAMAIN : code original reloge + vrais overlays,
 * vrais RNG/calendrier/combats. Seules les I/O DOS/BIOS et primitives de
 * presentation sans effet metier sont abstraites, selon les bancs BIN
 * precedents. Les differences sont fatales et localisees par adresse DS. */
#include "teramain_oracle.h"
#include <unicorn/unicorn.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

enum { DATA=0x10000, STACK=0xf800, STOP=0xf100 };
typedef struct {
    uc_engine *cpu;
    unsigned char overlays[0x17000];size_t overlay_size;
    unsigned overlay,pending,shot_key,keys,commands,draws,loads;
    unsigned seen[0x170];
    char trace[16][160];
    uint32_t last_seed;
    unsigned bounds[64],callers[64];
    unsigned char score[6];int score_seen;
    const TeraGameSession *expected;
} Oracle;
static Oracle machine;
static void fail(const char *reason)
{
    uint16_t ip=0,sp=0;
    if(machine.cpu) {uc_reg_read(machine.cpu,UC_X86_REG_IP,&ip);uc_reg_read(machine.cpu,UC_X86_REG_SP,&sp);}
    fprintf(stderr,"TERAMAIN oracle: %s key=%u overlay=%04x IP=%04x SP=%04x core_phase=%u\n",
        reason,machine.keys,machine.overlay,ip,sp,machine.expected ? (unsigned)machine.expected->phase : 99u);
    if(machine.cpu) {uint16_t ax=0;unsigned i;uc_reg_read(machine.cpu,UC_X86_REG_AX,&ax);fprintf(stderr,"AX=%04x stack:",ax);for(i=0;i<12;++i) {uint16_t n=0;uc_mem_read(machine.cpu,sp+2*i,&n,2);fprintf(stderr," %04x",n);}fputc('\n',stderr);}
    {unsigned i;for(i=0;i<16;++i) {unsigned at=(machine.keys+i+1)%16;if(machine.trace[at][0]) fprintf(stderr,"%s\n",machine.trace[at]);}}
    exit(1);
}
static void checked(uc_err result)
{
    if(result!=UC_ERR_OK) fail(uc_strerror(result));
}
static uint16_t reg16(int reg)
{
    uint16_t value=0;checked(uc_reg_read(machine.cpu,reg,&value));return value;
}
static void set16(int reg,uint16_t value) {checked(uc_reg_write(machine.cpu,reg,&value));}
static void write_bytes(uint32_t address,const void *bytes,size_t size)
{
    checked(uc_mem_write(machine.cpu,address,bytes,size));
}
static void word(uint32_t address,int value)
{
    unsigned char bytes[2]={(unsigned char)value,(unsigned char)((unsigned)value>>8)};
    write_bytes(address,bytes,2);
}
static void byte(uint32_t address,int value)
{
    unsigned char n=(unsigned char)value;write_bytes(address,&n,1);
}
static uint16_t read_word(uint32_t address)
{
    unsigned char bytes[2];checked(uc_mem_read(machine.cpu,address,bytes,2));
    return (uint16_t)(bytes[0]+256u*bytes[1]);
}
static void return_io(unsigned cleanup)
{
    uint16_t sp=reg16(UC_X86_REG_SP),ip=read_word(sp);
    set16(UC_X86_REG_SP,(uint16_t)(sp+2+cleanup));set16(UC_X86_REG_IP,ip);
}
static void pascal_string(uint32_t address,const char *text,size_t capacity)
{
    unsigned char bytes[80]={0};size_t length=strlen(text);
    if(length>=capacity || capacity>sizeof bytes) fail("initial string size");
    bytes[0]=(unsigned char)length;memcpy(bytes+1,text,length);
    write_bytes(DATA+address,bytes,capacity);
}
static size_t resource(const char *dir,const char *name,unsigned char *bytes,size_t capacity)
{
    char path[2048];FILE *file;size_t count;
    if(snprintf(path,sizeof path,"%s/%s",dir,name)<0) fail("resource path");
    file=fopen(path,"rb");if(!file) fail(name);
    count=fread(bytes,1,capacity,file);
    if(ferror(file) || fgetc(file)!=EOF || fclose(file)!=0) fail("resource read/size");
    return count;
}
static void data_resource(const char *dir,const char *name,unsigned address)
{
    unsigned char bytes[8192];size_t size=resource(dir,name,bytes,sizeof bytes);
    write_bytes(DATA+address,bytes,size);
}
static void initialize_data(const TeraExplorationState *s)
{
    const TeraWorldState *w=&s->world;const TeraCharacterState *ch=&w->characters;
    const TeraCombatGame *g=&ch->combat;unsigned i,j;unsigned char seed[4];
    for(i=0;i<4;++i) seed[i]=(unsigned char)(g->random.seed>>(8*i));
    write_bytes(DATA+0x1fc,seed,4);
    for(i=0;i<10;++i) {
        const TeraRoundActor *a=&g->encounter.actors[i];
        byte(DATA+0x9402+i,a->active);pascal_string(0x94c8+10*i,ch->names[i],10);
        pascal_string(0x952c+3*i,a->condition,3);
        write_bytes(DATA+0x7414+10*i,g->encounter.attributes[i].stats,10);
        write_bytes(DATA+0x7478+10*i,g->encounter.attributes[i].skills,10);
        write_bytes(DATA+0x73b0+10*i,g->equipment[i],10);
        word(DATA+0x74dc+2*i,a->hp);word(DATA+0x74f0+2*i,a->magic);
        word(DATA+0x7504+2*i,a->hp_limit);word(DATA+0x7518+2*i,a->magic_limit);
        word(DATA+0x752c+2*i,ch->keys[i]);
        if(i) {word(DATA+0x7550+2*i,w->locations[i]);word(DATA+0x753e + 2*i,w->extra_locations[i]);}
    }
    for(i=0;i<22;++i) for(j=0;j<6;++j) word(DATA+0xa751+12*i+2*j,g->monster_values[i][j]);
    for(i=0;i<9;++i) word(DATA+0x7564+2*i,w->horizontal_mountains[i]);
    for(i=0;i<25;++i) word(DATA+0x7576+2*i,w->vertical_mountains[i]);
    for(i=0;i<20;++i) word(DATA+0x75a8+2*i,w->towns[i]);
    for(i=0;i<38;++i) pascal_string(0x954a+40*i,w->hints[i],40);
    for(i=0;i<4;++i) pascal_string(0x9b3a+10*i,w->secret_words[i],10);
    write_bytes(DATA+0x75ea,g->helpers,4);write_bytes(DATA+0x75ee,w->maps,sizeof w->maps);
    write_bytes(DATA+0x940c,w->has_map,11);write_bytes(DATA+0x9419,w->tasks_done,4);
    word(DATA+0x75d2,g->position);word(DATA+0x9b72,g->previous_position);
    word(DATA+0x75d4,g->money);word(DATA+0x75d6,g->experience);
    word(DATA+0x75d8,ch->level);word(DATA+0x75da,ch->dummy_level);word(DATA+0x75dc,w->tasks_counter);
    word(DATA+0x75de,w->hidden_positions[0]);word(DATA+0x75e0,w->hidden_positions[1]);
    word(DATA+0x75e2,ch->days);word(DATA+0x75e4,w->arioch_position);word(DATA+0x75e6,ch->energy);
    word(DATA+0x75d0,w->alfol_position);word(DATA+0x75e8,g->difficulty);word(DATA+0x9b76,g->world_id);
    byte(DATA+0x9417,w->initial_flag);byte(DATA+0x9418,w->mental_extension);
    byte(DATA+0xa745,g->encounter.party_available);byte(DATA+0xa746,g->encounter.victory);
    byte(DATA+0xafcf,g->encounter.fled);byte(DATA+0xa748,g->on_alfol);
    byte(DATA+0xa74a,g->sound);byte(DATA+0xa74d,w->fast_moves);byte(DATA+0xa74f,ch->arrival_pending);
    byte(DATA+0xa749,s->moved);byte(DATA+0xafd4,s->town_found);byte(DATA+0xafd3,s->town_id);
    byte(DATA+0xafd0,s->force_day);byte(DATA+0xafd2,s->encounter_suppressed);
    byte(DATA+0xafce,g->recruit_ready);byte(DATA+0xafd1,g->location_changed);
    byte(DATA+0xa750,s->information_context);word(DATA+0x9b66,9);word(DATA+0x9b6c,4);
    byte(DATA+0xa74e,1);byte(DATA+0xa679,'G');
}

static void equal_number(unsigned address,unsigned width,uint32_t value)
{
    unsigned char bytes[4];unsigned i;uint32_t actual=0,mask=width==1 ? 255u : width==2 ? 65535u : UINT32_MAX;
    checked(uc_mem_read(machine.cpu,DATA+address,bytes,width));
    for(i=0;i<width;++i) actual|=(uint32_t)bytes[i]<<(8*i);
    if(actual!=(value&mask)) {
        if(address==0x1fc) {uint32_t n=machine.last_seed;unsigned k;for(k=0;k<100000;++k) {if(n==actual || n==value) fprintf(stderr,"seed distance %s=%u\n",n==actual ? "DOS" : "C",k);n=n*UINT32_C(129)+UINT32_C(0x361962e9);}for(k=0;k<32;++k) {unsigned at=(machine.draws-32+k)%64;fprintf(stderr,"R%u@%04x ",machine.bounds[at],machine.callers[at]);}fputc('\n',stderr);}
        fprintf(stderr,"DS:%04x DOS=%lu C=%lu\n",address,(unsigned long)actual,(unsigned long)(value&mask));fail("persistent state mismatch");
    }
}
static void equal_bytes(unsigned address,const void *expected,size_t size)
{
    const unsigned char *want=expected;unsigned char actual[7700];size_t i;
    if(size>sizeof actual) fail("comparison buffer");
    checked(uc_mem_read(machine.cpu,DATA+address,actual,size));
    for(i=0;i<size;++i) if(actual[i]!=want[i]) {equal_number(address+(unsigned)i,1,want[i]);}
}
static void equal_string(unsigned address,const char *text,unsigned capacity)
{
    unsigned char actual[40];size_t length=strlen(text);
    checked(uc_mem_read(machine.cpu,DATA+address,actual,capacity));
    if(actual[0]!=length || memcmp(actual+1,text,length)) {fprintf(stderr,"string DS:%04x\n",address);fail("string mismatch");}
}
/* Frontieres durables uniquement : les cadres locaux et l'affichage ne
 * font pas partie de la comparaison. Cartes et roster sont compares en
 * entier ; ne jamais recaler une graine ou masquer un ecart d'objet. */
static void compare_ready(const TeraGameSession *s)
{
    const TeraWorldState *w=&s->state.world;const TeraCharacterState *ch=&w->characters;
    const TeraCombatGame *g=&ch->combat;unsigned i;
    equal_number(0x1fc,4,g->random.seed);equal_number(0x75d2,2,(uint16_t)g->position);
    equal_number(0x9b76,2,g->world_id);equal_number(0x9b72,2,(uint16_t)g->previous_position);
    equal_number(0x75d4,2,(uint16_t)g->money);equal_number(0x75d6,2,(uint16_t)g->experience);
    equal_number(0x75d8,2,(uint16_t)ch->level);equal_number(0x75da,2,(uint16_t)ch->dummy_level);
    equal_number(0x75e2,2,(uint16_t)ch->days);equal_number(0x75e6,2,(uint16_t)ch->energy);
    equal_number(0x75dc,2,(uint16_t)w->tasks_counter);
    equal_bytes(0x75ee,w->maps,sizeof w->maps);equal_bytes(0x940c,w->has_map,11);
    equal_bytes(0x9419,w->tasks_done,4);equal_bytes(0x75ea,g->helpers,4);
    equal_number(0xa748,1,(unsigned)g->on_alfol);equal_number(0xa74a,1,(unsigned)g->sound);
    equal_number(0xa74d,1,(unsigned)w->fast_moves);equal_number(0x9418,1,(unsigned)w->mental_extension);
    equal_number(0xa745,1,(unsigned)g->encounter.party_available);
    equal_number(0xa74f,1,(unsigned)ch->arrival_pending);equal_number(0xafcf,1,(unsigned)g->encounter.fled);
    equal_number(0xafd0,1,(unsigned)s->state.force_day);equal_number(0xafd2,1,(unsigned)s->state.encounter_suppressed);
    equal_number(0xafce,1,(unsigned)g->recruit_ready);equal_number(0xafd1,1,(unsigned)g->location_changed);
    equal_number(0xa750,1,(unsigned)s->state.information_context);
    if(g->on_alfol) equal_number(0xafd5,2,(uint16_t)s->control.space_backup_position);
    else if(g->world_id==10) equal_number(0xafd5,2,(uint16_t)s->control.meduz_backup_position);
    else if(g->world_id<9) equal_number(0xafd5,2,(uint16_t)s->state.city_backup_position);
    for(i=0;i<10;++i) {
        const TeraRoundActor *a=&g->encounter.actors[i];
        equal_number(0x9402+i,1,a->active);equal_string(0x94c8+10*i,ch->names[i],10);
        equal_string(0x952c+3*i,a->condition,3);
        equal_bytes(0x7414+10*i,g->encounter.attributes[i].stats,10);
        equal_bytes(0x7478+10*i,g->encounter.attributes[i].skills,10);
        equal_bytes(0x73b0+10*i,g->equipment[i],10);
        equal_number(0x74dc+2*i,2,(uint16_t)a->hp);equal_number(0x74f0+2*i,2,(uint16_t)a->magic);
        equal_number(0x7504+2*i,2,(uint16_t)a->hp_limit);equal_number(0x7518+2*i,2,(uint16_t)a->magic_limit);
        equal_number(0x752c+2*i,2,(uint16_t)ch->keys[i]);
        if(i) {equal_number(0x7550+2*i,2,(uint16_t)w->locations[i]);equal_number(0x753e + 2*i,2,(uint16_t)w->extra_locations[i]);}
    }
    for(i=0;i<20;++i) equal_number(0x75a8+2*i,2,(uint16_t)w->towns[i]);
    for(i=0;i<38;++i) equal_string(0x954a+40*i,w->hints[i],40);
    for(i=0;i<4;++i) equal_string(0x9b3a+10*i,w->secret_words[i],10);
    ++machine.commands;
    machine.last_seed=g->random.seed;
}

static void instruction(uc_engine *cpu,uint64_t address,uint32_t size,void *context)
{
    (void)cpu;(void)size;(void)context;
    if(address==0x10da) {machine.bounds[machine.draws%64]=reg16(UC_X86_REG_AX);machine.callers[machine.draws%64]=read_word(reg16(UC_X86_REG_SP));++machine.draws;return;}
    if(address==0xbfcd && machine.overlay==0xcb) {
        checked(uc_mem_read(machine.cpu,reg16(UC_X86_REG_BP)-8,machine.score,6));machine.score_seen=1;
    }
    if(address==0x2904) {set16(UC_X86_REG_SP,(uint16_t)(reg16(UC_X86_REG_SP)+2));return_io(0);return;}
    if(address==0xba0f) {
        unsigned module=reg16(UC_X86_REG_DX),length=reg16(UC_X86_REG_AX);
        size_t first=(size_t)module*256;
        if(module==0xd2) {return_io(0);return;} /* Fichier DOS de sauvegarde, sans regle/RNG. */
        if(module>=0x170 || first+length>machine.overlay_size || length>0x2000) fail("overlay bounds");
        machine.overlay=module;++machine.loads;++machine.seen[module];
        write_bytes(0xba21,machine.overlays+first,length);
        checked(uc_ctl_remove_cache(machine.cpu,0xba21,0xda20));
        set16(UC_X86_REG_IP,0xba28);return;
    }
    if(address==0x4277) {machine.pending=1;checked(uc_emu_stop(machine.cpu));return;}
    /* OVL0085 utilise ici sa lecture brute C420C et non C4277. Conserver
     * la frontiere de commande sans executer une attente BIOS active. */
    if(address==0xcd09 && machine.overlay==0x85) {machine.pending=5;checked(uc_emu_stop(machine.cpu));return;}
    if(address==0x4210) {
        unsigned caller=read_word(reg16(UC_X86_REG_SP));
        /* Identifier les vraies lectures par le DOS, independamment de
         * la phase du core : condoleances/fin, H/S et panneau L. */
        if(machine.overlay==0xcb || caller==0xa969 || caller==0xe6db) {
            machine.pending=2;checked(uc_emu_stop(machine.cpu));return;
        }
        return_io(0);return;
    }
    if(address==0x0da4) {
        uint16_t caller=read_word(reg16(UC_X86_REG_SP));uint32_t flags=0;
        if(machine.overlay==0xae && caller==0xbafd) {machine.pending=3;checked(uc_emu_stop(machine.cpu));return;}
        set16(UC_X86_REG_AX,0);checked(uc_reg_read(machine.cpu,UC_X86_REG_EFLAGS,&flags));flags|=64;
        checked(uc_reg_write(machine.cpu,UC_X86_REG_EFLAGS,&flags));return_io(0);return;
    }
    if(machine.overlay==0xae && address==0xbb05) {
        uint16_t parent=read_word(reg16(UC_X86_REG_BP)-2);byte(parent-18,(int)machine.shot_key);
        set16(UC_X86_REG_IP,0xbb1a);return;
    }
    if(address==0xe7c0 || address==0x0c89 || address==0x103c) {
        machine.pending=4;checked(uc_emu_stop(machine.cpu));return;
    }
    /* Sorties qui ne modifient ni RNG ni regles. Leurs tailles sont les
     * arguments Pascal reellement empiles, pas la taille d'une API C. */
    switch(address) {
    case 0x2558:case 0x1022:case 0x4503:case 0x0213:case 0x0226:case 0x02d9:
    case 0x03bb:case 0x03cf:case 0x2975:case 0x6013:case 0x6031:case 0x6052:
    case 0x609a:case 0x60bb:case 0x666b:case 0x6721:return_io(0);return;
    case 0x6073:byte(DATA+0xa750,1);return_io(0);return;
    case 0x0371:case 0x037f:set16(UC_X86_REG_AX,1);return_io(0);return;
    case 0x038d:return_io(6);return;
    case 0x2926:return_io(read_word(reg16(UC_X86_REG_SP)+2)%256+1);return;
    case 0x4779:return_io(40);return;
    case 0x47b3:return_io(80);return;
    case 0x289d:case 0x28b9:case 0x0daa:case 0x4873:case 0x49fd:case 0x4520:case 0x4efe:return_io(2);return;
    case 0x29c9:case 0x2a20:case 0x2a3d:case 0x2a0e:case 0x2dab:case 0x2dae:return_io(2);return;
    case 0x0f4d:if(reg16(UC_X86_REG_AX)==0x4000) {return_io(8);return;}break;
    case 0x6179:return_io(8);return;
    case 0x4544:case 0x46bc:case 0x4601:case 0x2db7:return_io(10);return;
    case 0x5051:case 0x5e04:case 0x2db4:case 0x6221:return_io(6);return;
    case 0x2dc0:case 0x2dc6:case 0x2dcf:return_io(8);return;
    case 0x2dc3:return_io(12);return;
    case 0x295d: {
        uint16_t sp=reg16(UC_X86_REG_SP),ip=read_word(sp);unsigned char length;
        checked(uc_mem_read(machine.cpu,ip,&length,1));word(sp,ip+length+1);return_io(0);return;
    }
    case 0x4807:set16(UC_X86_REG_IP,0x4820);return;
    case 0x5495:word(DATA+0x9b66,9);return_io(0);return;
    case 0x5274:set16(UC_X86_REG_IP,0x548c);return;
    case 0x7d36:set16(UC_X86_REG_IP,0x7d8d);return;
    case 0x4d6b:set16(UC_X86_REG_IP,0x4d9c);return;
    case 0x4dc2:set16(UC_X86_REG_IP,0x4dfc);return;
    case 0x4e03:set16(UC_X86_REG_IP,0x4ec3);return;
    case 0x8765:set16(UC_X86_REG_IP,0x8783);return;
    case 0x9ac3:set16(UC_X86_REG_IP,0x9ae6);return;
    case 0x9af2:set16(UC_X86_REG_IP,0x9b0f);return;
    case 0x9364:set16(UC_X86_REG_IP,0x938d);return;
    case 0x8b6a:set16(UC_X86_REG_IP,0x8b70);return;
    case 0x8bac:set16(UC_X86_REG_IP,0x8bf7);return;
    case 0x8c1e:set16(UC_X86_REG_IP,0x8c33);return;
    case 0x8cef:set16(UC_X86_REG_IP,0x8d20);return;
    case 0x9298:set16(UC_X86_REG_IP,0x92ef);return;
    }
    if(address==0xba2f && machine.overlay==0x22) {return_io(48);return;}
    if(address==0xba2f && machine.overlay==0xbc) {return_io(0);return;}
    if(address==0xc3a8 && machine.overlay==0xae) {set16(UC_X86_REG_IP,0xc406);return;}
}
static void advance(void)
{
    machine.pending=0;
    checked(uc_emu_start(machine.cpu,reg16(UC_X86_REG_IP),STOP,3000000,5000000));
    if(!machine.pending) fail("no input/terminal boundary before budget");
}
void teramain_oracle_start(const char *dir,const TeraExplorationState *world,const TeraGameSession *session)
{
    unsigned char image[65536];size_t length;uc_hook hook;
    memset(&machine,0,sizeof machine);machine.expected=session;
    checked(uc_open(UC_ARCH_X86,UC_MODE_16,&machine.cpu));checked(uc_mem_map(machine.cpu,0,0x20000,UC_PROT_ALL));
    length=resource(dir,"TERA.COM",image,sizeof image);
    write_bytes(0x100,image,length);write_bytes(0xda21,image+0xda21-0x20fc,length-(0xda21-0x20fc));
    machine.overlay_size=resource(dir,"TERA.000",machine.overlays,sizeof machine.overlays);
    set16(UC_X86_REG_DS,DATA>>4);set16(UC_X86_REG_BP,0xf900);set16(UC_X86_REG_SP,STACK);
    word(STACK,STOP);word(0xf8fe,0xf900);
    data_resource(dir,"VERBES",0x5f98);data_resource(dir,"MESSAGE",0x63c0);
    data_resource(dir,"MESSAGE2",0x9c24);data_resource(dir,"MESSAGE3",0xa14c);
    data_resource(dir,"OBJETS",0x5e08);initialize_data(world);
    checked(uc_hook_add(machine.cpu,&hook,UC_HOOK_CODE,(void *)instruction,NULL,1,0));
    set16(UC_X86_REG_IP,0xdbac);advance();
}
void teramain_oracle_key(unsigned key,const TeraGameSession *session)
{
    unsigned pending=machine.pending;machine.expected=session;++machine.keys;
    if(pending==1) {set16(UC_X86_REG_AX,(uint16_t)(key>='a' && key<='z' ? key-32 : key));return_io(1);}
    else if(pending==2) return_io(0);
    else if(pending==3) {machine.shot_key=key;set16(UC_X86_REG_AX,(uint16_t)(key!=0));return_io(0);}
    else if(pending==5) {byte(DATA+0xa679,(int)(key>='a' && key<='z' ? key-32 : key));set16(UC_X86_REG_IP,0xcd1a);}
    else fail("key after DOS termination");
    advance();
    snprintf(machine.trace[machine.keys%16],sizeof machine.trace[0],"key=%u char=%u phase=%u caller=%04x DOSworld=%u pos=%u prev=%u Cworld=%u pos=%d prev=%d",machine.keys,key,(unsigned)session->phase,read_word(reg16(UC_X86_REG_SP)),
        read_word(DATA+0x9b76),read_word(DATA+0x75d2),read_word(DATA+0x9b72),session->state.world.characters.combat.world_id,session->state.world.characters.combat.position,session->state.world.characters.combat.previous_position);
    if(session->phase==TERA_GAME_READY) {
        unsigned caller=read_word(reg16(UC_X86_REG_SP));
        int command=machine.pending==5 || (machine.pending==1 && (caller==0xdc19 ||
            caller==0xd751 || caller==0xbf7f || ((machine.overlay==0x2e || machine.overlay==0x49 || machine.overlay==0x69) && caller>=0xba21)));
        if(!command) fail("core READY does not match a DOS command boundary");
        compare_ready(session);
    }
}
void teramain_oracle_finish(const TeraGameSession *session)
{
    unsigned module;
    if(session->phase!=TERA_GAME_COMPLETE || machine.pending!=4) fail("endings do not agree");
    compare_ready(session);
    if(!machine.score_seen || memcmp(machine.score,session->active.ending.score_real48,6)) fail("final Real48 score mismatch");
    printf("TERAMAIN BIN/C keys=%u stable_boundaries=%u original_rng_calls=%u overlay_loads=%u\n",
        machine.keys,machine.commands,machine.draws,machine.loads);
    printf("TERAMAIN overlays:");for(module=0;module<0x170;++module) if(machine.seen[module]) printf(" %04x:%u",module,machine.seen[module]);puts("");
    checked(uc_close(machine.cpu));machine.cpu=NULL;
}
