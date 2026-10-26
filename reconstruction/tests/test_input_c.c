#include "module_test_support.h"
#include "tera/game.h"

typedef struct { unsigned stop,frames,purges;int pending; } Script;
/* File de test : une touche ancienne est deja la avant l'animation ; une
 * nouvelle apparait au poll choisi. PURGE seule retire la touche. */
static int probe(void *context,TeraInputBoundary boundary,unsigned frame)
{
    Script *s=context;
    if(boundary==TERA_INPUT_PURGE) {++s->purges;s->pending=0;return 0;}
    assert(frame==++s->frames);
    if(s->stop && frame==s->stop) s->pending=1;
    return s->pending;
}
static void test_monster15_interruptions(void)
{
    unsigned stop,sound;
    for(sound=0;sound<2;++sound) for(stop=0;stop<=6;++stop) {
        TeraExplorationState w=module_fixture();Script script={stop,0,0,1};
        TeraInputSource source={probe,&script};unsigned calls=7,n=stop ? stop : 6;
        w.world.characters.combat.sound=(int)sound;w.world.characters.combat.input=&source;
        assert(tera_monster_presentation_random(&w.world.characters.combat,15,&calls));
        assert(calls==7+3*n && script.frames==n && script.purges==1);
        assert(w.world.characters.combat.random.seed==seed_after(1986,3*n));
        assert(script.pending==(stop!=0)); /* KEYPRESSED ne lit pas la touche. */
    }
}
static void test_game_discards_animation_key(void)
{
    TeraExplorationState w=module_fixture();TeraGameSession s;
    Script script={2,0,0,1};TeraInputSource source={probe,&script};
    w.world.characters.combat.world_id=1;w.world.characters.combat.position=100;
    w.world.characters.combat.previous_position=99;w.navigation_overlay=1;
    w.world.maps[100][0]=1;w.world.maps[101][5]=15;
    w.world.characters.combat.input=&source;
    assert(tera_game_start(&s,&w,NULL,NULL,NULL));assert(!script.pending);
    assert(tera_game_key(&s,'8') && s.phase==TERA_GAME_NAVIGATION);
    assert(script.frames==2 && !script.pending && script.purges==3);
    assert(s.active.navigation.day.combat.phase==TERA_COMBAT_ACTION);
    assert(tera_game_key(&s,'T'));
    assert(s.active.navigation.day.combat.phase==TERA_COMBAT_CONFIRM);
}
static unsigned target(const TeraSpaceSession *s)
{
    static const unsigned pad[3][3]={{'7','8','9'},{'4','5','6'},{'1','2','3'}};
    return pad[s->alien_y+1][s->alien_x+1];
}
static void test_space_boundaries(void)
{
    static const int offsets[]={-1,0,1,299,300,301,529,530,531,729,730,731,899,900,901,989,990};
    unsigned i,sound;
    for(sound=0;sound<2;++sound) for(i=0;i<sizeof offsets/sizeof offsets[0];++i) {
        TeraExplorationState w=module_fixture();TeraSpaceSession s,expected,before;
        unsigned time,end,key;
        w.world.characters.combat.sound=(int)sound;
        assert(tera_space_start(&s,&w.world.characters,4));assert(tera_space_key(&s,'N'));
        expected=s;end=s.delay_ms+990;
        if(!s.delay_ms && offsets[i]<0) continue; /* Aucun temps negatif. */
        time=(unsigned)((int)s.delay_ms+offsets[i]);key=target(&s);
        assert(tera_space_input_at(&s,time,(int)key));
        if(time<end) assert(tera_space_input_at(&s,end,-1));
        assert(tera_space_key(&expected,offsets[i]>300 ? key : 0));
        assert(s.hit==(offsets[i]>300) && s.trials==1);
        assert(s.phase==expected.phase && s.pirates==expected.pirates && s.counter==expected.counter);
        assert(memcmp(&s.state,&expected.state,sizeof s.state)==0);
        before=s;assert(!tera_space_input_at(&s,s.delay_ms+991,-1));
        assert(memcmp(&s,&before,sizeof s)==0);
    }
}
static void test_first_key_partition_and_game_return(void)
{
    TeraExplorationState w=module_fixture();TeraSpaceSession s,expected,before;
    TeraGameSession game;unsigned end,key;
    assert(tera_space_start(&s,&w.world.characters,4));assert(tera_space_key(&s,'N'));
    expected=s;end=s.delay_ms+990;key=target(&s);
    assert(tera_space_input_at(&s,s.delay_ms+301,'X'));
    assert(tera_space_input_at(&s,s.delay_ms+500,(int)key));
    before=s;assert(!tera_space_input_at(&s,0,-1));assert(!tera_space_input_at(&s,end,256));
    assert(memcmp(&s,&before,sizeof s)==0);
    assert(tera_space_input_at(&s,end,-1));assert(tera_space_key(&expected,'X'));
    assert(!s.hit && memcmp(&s.state,&expected.state,sizeof s.state)==0);
    /* Par l'API globale, le dernier tir retourne bien a l'atterrissage,
     * au lieu de laisser le frontend reintegrer manuellement le vol. */
    w.town_found=1;w.town_id=1;
    assert(tera_game_start(&game,&w,NULL,NULL,NULL));
    assert(tera_game_key(&game,'E'));assert(tera_game_key(&game,'O'));assert(tera_game_key(&game,'N'));
    while(game.phase==TERA_GAME_SPACE) {
        unsigned delay=game.active.space.delay_ms;
        key=target(&game.active.space);
        assert(tera_game_space_input_at(&game,delay+301,(int)key));
        assert(tera_game_space_input_at(&game,delay+530,-1));
        assert(tera_game_space_input_at(&game,delay+730,-1));
        assert(tera_game_space_input_at(&game,delay+900,-1));
        assert(tera_game_space_input_at(&game,delay+990,-1));
    }
    assert(game.phase==TERA_GAME_READY && game.state.world.characters.combat.on_alfol);
    assert(game.state.world.characters.combat.position==0);
    assert(!tera_game_space_input_at(&game,0,-1));
}
/* Une nouvelle cible ouvre une nouvelle origine temporelle et ne conserve
 * aucune touche du tir precedent, meme apres un echec. Comparer au chemin
 * semantique controle aussi les effets de son et le calendrier du RNG. */
static void test_next_shot_resets_clock_and_queue(void)
{
    unsigned sound,hit;
    for(sound=0;sound<2;++sound) for(hit=0;hit<2;++hit) {
        TeraExplorationState w=module_fixture();TeraSpaceSession s,expected;
        unsigned key,end;
        w.world.characters.combat.sound=(int)sound;
        assert(tera_space_start(&s,&w.world.characters,8));
        assert(tera_space_key(&s,'N'));expected=s;
        key=hit ? target(&s) : 'X';end=s.delay_ms+990;
        assert(tera_space_input_at(&s,s.delay_ms+301,(int)key));
        assert(tera_space_input_at(&s,end,-1));
        assert(tera_space_key(&expected,key));
        assert(s.phase==TERA_SPACE_SHOT && s.trials==1);
        assert(s.input_ms==0 && s.input_stage==0 && s.input_key==-1);
        assert(memcmp(&s,&expected,sizeof s)==0);
        /* Sans evenement clavier au second tir, aucune ancienne touche
         * ne peut produire un succes sur une cible identique par hasard. */
        assert(tera_space_input_at(&s,0,-1));
        assert(tera_space_input_at(&s,s.delay_ms+990,-1));
        assert(tera_space_key(&expected,0));
        assert(!s.hit && s.trials==2);
        assert(memcmp(&s,&expected,sizeof s)==0);
    }
}
/* L'API brute ne doit jamais avancer une confirmation ou consommer un
 * tirage lorsqu'un evenement ne respecte pas son domaine d'entree. */
static void test_raw_input_refusals_and_optional_source(void)
{
    TeraExplorationState w=module_fixture();TeraSpaceSession s,before;
    TeraInputSource empty={NULL,NULL};unsigned calls=0;
    assert(!tera_input_boundary(NULL,TERA_INPUT_PURGE,0));
    assert(!tera_input_boundary(&empty,TERA_INPUT_MONSTER15_FRAME,1));
    w.world.characters.combat.input=&empty;
    assert(tera_monster_presentation_random(&w.world.characters.combat,15,&calls));
    assert(calls==18 && w.world.characters.combat.random.seed==seed_after(1986,18));
    assert(!tera_space_input_at(NULL,0,-1));
    assert(tera_space_start(&s,&w.world.characters,8));before=s;
    assert(!tera_space_input_at(&s,0,-1));
    assert(memcmp(&s,&before,sizeof s)==0);
    assert(tera_space_key(&s,'N'));before=s;
    assert(!tera_space_input_at(&s,0,-2));
    assert(!tera_space_input_at(&s,0,256));
    assert(!tera_space_input_at(&s,s.delay_ms+991,-1));
    assert(memcmp(&s,&before,sizeof s)==0);
    assert(tera_space_input_at(&s,s.delay_ms+301,'X'));before=s;
    assert(!tera_space_input_at(&s,s.delay_ms+300,-1));
    assert(memcmp(&s,&before,sizeof s)==0);
}
int main(void)
{
    test_monster15_interruptions();test_game_discards_animation_key();test_space_boundaries();
    test_first_key_partition_and_game_return();
    test_next_shot_resets_clock_and_queue();
    test_raw_input_refusals_and_optional_source();return 0;
}
