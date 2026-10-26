#include "module_test_support.h"
#include "tera/save.h"
#include <stdlib.h>

static unsigned char original[TERA_SAVE_CAPACITY],altered[TERA_SAVE_CAPACITY];
static unsigned callbacks;
static int input_probe(void *context,TeraInputBoundary boundary,unsigned frame)
{
    (void)context;(void)boundary;(void)frame;++callbacks;return 0;
}
static void observe(void *context,const TeraCombatEvent *event)
{
    (void)context;(void)event;++callbacks;
}
static TeraGameSession fixture(void)
{
    TeraGameSession s;TeraExplorationState w=module_fixture();
    assert(tera_game_start(&s,&w,NULL,NULL,NULL));return s;
}
/* Comparer les octets canoniques, jamais le padding d'une structure C.
 * Les services runtime volontairement differents doivent etre ignores. */
static void same_checkpoint(const TeraGameSession *a,const TeraGameSession *b)
{
    unsigned char left[TERA_SAVE_CAPACITY],right[TERA_SAVE_CAPACITY];size_t n=0,m=0;
    assert(tera_save_encode(a,left,sizeof left,&n)==TERA_SAVE_OK);
    assert(tera_save_encode(b,right,sizeof right,&m)==TERA_SAVE_OK);
    assert(n==m && memcmp(left,right,n)==0);
}
static void test_roundtrip_and_resume(void)
{
    TeraGameSession source=fixture(),loaded;size_t size=0;
    TeraInputSource input={input_probe,&source};uint32_t seed;
    source.observe=observe;source.observer_context=&source;
    source.state.world.characters.combat.input=&input;
    source.state.world.characters.combat.money=INT16_MIN;
    source.state.world.characters.days=INT16_MAX;
    source.state.world.characters.combat.random.seed=UINT32_MAX;
    source.state.world.hints[37][0]=(char)0x82; /* Octet CP437, pas ASCII. */
    seed=source.state.world.characters.combat.random.seed;callbacks=0;
    assert(tera_save_encode(&source,original,sizeof original,&size)==TERA_SAVE_OK);
    assert(size==11521); /* Contrat du format v1, independant du packing. */
    assert(memcmp(original,"TERASAV\0\1\0\0\0",12)==0);
    memset(&loaded,0xa5,sizeof loaded);
    assert(tera_save_decode(&loaded,original,size,&input,observe,&loaded)==TERA_SAVE_OK);
    assert(!callbacks && source.state.world.characters.combat.random.seed==seed);
    assert(loaded.state.world.characters.combat.input==&input);
    assert(loaded.observe==observe && loaded.observer_context==&loaded);
    assert(loaded.phase==TERA_GAME_READY && !loaded.after_mandatory);
    same_checkpoint(&source,&loaded);
    /* Reprise sans aucun callback implicite ; ensuite memes commandes et
     * memes graines, meme apres un deplacement susceptible de rencontrer. */
    source.state.world.characters.combat.input=NULL;source.observe=NULL;
    source.observer_context=NULL;
    assert(tera_save_decode(&loaded,original,size,NULL,NULL,NULL)==TERA_SAVE_OK);
    assert(!loaded.state.world.characters.combat.input && !loaded.observe && !loaded.observer_context);
    assert(tera_game_key(&source,'L')==tera_game_key(&loaded,'L'));
    assert(source.phase==loaded.phase);
    assert(tera_game_key(&source,'8')==tera_game_key(&loaded,'8'));
    assert(source.phase==loaded.phase);
    assert(source.state.world.characters.combat.random.seed==loaded.state.world.characters.combat.random.seed);
    assert(memcmp(&source.state,&loaded.state,sizeof source.state)==0);
}
static void assert_rejected(const unsigned char *bytes,size_t size,TeraSaveResult expected)
{
    TeraGameSession result=fixture(),before=result;
    assert(tera_save_decode(&result,bytes,size,NULL,NULL,NULL)==expected);
    assert(memcmp(&result,&before,sizeof result)==0);
}
/* Recalculer le CRC permet de verifier aussi les validations semantiques,
 * au-dela du simple rejet des fichiers corrompus. Algorithme de reference
 * polynomial bit a bit, propre au banc et non expose par le moteur. */
static void update_crc(unsigned char *bytes,size_t size)
{
    uint32_t crc=UINT32_MAX;size_t i;unsigned j;
    for(i=20;i<size;++i) {
        crc^=bytes[i];for(j=0;j<8;++j) {if(crc&1) crc=(crc>>1)^UINT32_C(0xedb88320);else crc>>=1;}
    }
    crc=~crc;for(j=0;j<4;++j) bytes[16+j]=(unsigned char)(crc>>(8*j));
}
static void test_malformed_and_atomic_refusal(void)
{
    TeraGameSession source=fixture();size_t size=0,i,untouched=123;
    assert(tera_save_encode(&source,original,sizeof original,&size)==TERA_SAVE_OK);
    for(i=0;i<size;++i) assert_rejected(original,i,TERA_SAVE_FORMAT);
    memcpy(altered,original,size);altered[size]=0;
    assert_rejected(altered,size+1,TERA_SAVE_FORMAT);
    for(i=0;i<16;++i) {
        memcpy(altered,original,size);altered[i]^=0x80;
        assert_rejected(altered,size,TERA_SAVE_FORMAT);
    }
    for(i=16;i<size;i+=317) {
        memcpy(altered,original,size);altered[i]^=1;
        assert_rejected(altered,size,TERA_SAVE_CHECKSUM);
    }
    memcpy(altered,original,size);altered[20]=2;update_crc(altered,size);
    assert_rejected(altered,size,TERA_SAVE_STATE); /* Active n'est pas un booleen. */
    memcpy(altered,original,size);memset(altered+30,'X',3);update_crc(altered,size);
    assert_rejected(altered,size,TERA_SAVE_STATE); /* Statut non termine. */
    memset(altered,0x5a,sizeof altered);
    assert(tera_save_encode(&source,altered,1,&untouched)==TERA_SAVE_BUFFER);
    assert(untouched==123 && altered[0]==0x5a && altered[1]==0x5a);
    source.phase=TERA_GAME_DAY;
    assert(tera_save_encode(&source,altered,sizeof altered,&untouched)==TERA_SAVE_STATE);
    assert(untouched==123 && altered[0]==0x5a);
    assert(tera_save_encode(NULL,altered,sizeof altered,&untouched)==TERA_SAVE_ARGUMENT);
    assert(tera_save_decode(&source,NULL,0,NULL,NULL,NULL)==TERA_SAVE_ARGUMENT);
    assert(tera_save_decode(NULL,original,size,NULL,NULL,NULL)==TERA_SAVE_ARGUMENT);
}
static void test_contexts_and_no_automatic_transition(void)
{
    TeraGameSession source=fixture(),loaded,before;size_t size;unsigned world;
    for(world=0;world<=11;++world) {
        TeraCombatGame *g;source=fixture();g=&source.state.world.characters.combat;
        g->world_id=world;g->on_alfol=world==9;
        g->position=(int16_t)(world==11 ? 1200 : 100*world+15);
        source.control.space_backup_position=1200;source.control.meduz_backup_position=1300;
        source.state.navigation_overlay=world==8 ? 2 : 1;
        /* Les commandes obligatoires apres arrivee mortelle doivent etre
         * conservees : reprendre ne cree pas prematurement un heros. */
        if(world==8 || world==9 || world==10) {
            source.control.mandatory_command=world-7;g->encounter.party_available=0;
        }
        assert(tera_save_encode(&source,original,sizeof original,&size)==TERA_SAVE_OK);
        assert(tera_save_decode(&loaded,original,size,NULL,NULL,NULL)==TERA_SAVE_OK);
        same_checkpoint(&source,&loaded);
    }
    source=fixture();before=source;
    source.state.world.characters.arrival_pending=1;
    assert(!tera_game_restore_ready(&loaded,&source.state,&source.control,NULL,NULL,NULL));
    assert(tera_save_encode(&source,original,sizeof original,&size)==TERA_SAVE_STATE);
    source=before;source.state.world.characters.combat.encounter.party_available=0;
    assert(tera_save_encode(&source,original,sizeof original,&size)==TERA_SAVE_STATE);
    source=before;memset(source.state.world.tasks_done,1,4);
    assert(tera_save_encode(&source,original,sizeof original,&size)==TERA_SAVE_STATE);
    source=before;source.control.npc_variant=3;
    assert(tera_save_encode(&source,original,sizeof original,&size)==TERA_SAVE_STATE);
    loaded=before;
    assert(!tera_game_restore_ready(&loaded,&source.state,&source.control,NULL,NULL,NULL));
    assert(memcmp(&loaded,&before,sizeof loaded)==0);
    assert(tera_game_restore_ready(&before,&before.state,&before.control,NULL,NULL,NULL));
}
/* Fichier de test dedie, fourni par CTest : fermer puis rouvrir prouve
 * que la reprise ne depend pas d'un buffer ou d'une session encore vive. */
static void test_disk_file(const char *path)
{
    TeraGameSession source=fixture(),loaded,before;FILE *file;
    file=fopen(path,"wb");assert(file);
    assert(tera_save_write(file,&source)==TERA_SAVE_OK);assert(fclose(file)==0);
    file=fopen(path,"rb");assert(file);
    assert(tera_save_read(file,&loaded,NULL,NULL,NULL)==TERA_SAVE_OK);
    assert(tera_save_write(file,&source)==TERA_SAVE_IO); /* Flux en lecture seule. */
    clearerr(file);
    assert(fclose(file)==0);same_checkpoint(&source,&loaded);
    file=fopen(path,"ab");assert(file);assert(fputc('X',file)!=EOF);assert(fclose(file)==0);
    before=loaded;file=fopen(path,"rb");assert(file);
    assert(tera_save_read(file,&loaded,NULL,NULL,NULL)==TERA_SAVE_FORMAT);
    assert(fclose(file)==0 && memcmp(&before,&loaded,sizeof loaded)==0);
    assert(remove(path)==0);
    assert(tera_save_read(NULL,&loaded,NULL,NULL,NULL)==TERA_SAVE_ARGUMENT);
    assert(tera_save_write(NULL,&source)==TERA_SAVE_ARGUMENT);
}
int main(int argc,char **argv)
{
    assert(argc==2);
    test_roundtrip_and_resume();test_malformed_and_atomic_refusal();
    test_contexts_and_no_automatic_transition();test_disk_file(argv[1]);return 0;
}
