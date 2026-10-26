#include "tera/game.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Fixtures SYNTHETIQUES de raccordement : les modules gardent leurs
 * propres vecteurs BIN. Ici on compare une chaine de modules independante
 * a l'API globale, et on verifie les frontieres qui etaient dans l'hote. */
static TeraExplorationState fixture(void)
{
    TeraExplorationState s={0};unsigned i,j;
    TeraCombatGame *g=&s.world.characters.combat;
    g->world_id=11;g->position=1200;g->previous_position=1199;
    g->difficulty=1;g->random.seed=1986;g->money=2000;
    g->encounter.party_available=1;g->encounter.actors[0].active=1;
    s.world.characters.days=7300;s.world.characters.energy=1000;
    for(i=0;i<10;++i) {
        strcpy(s.world.characters.names[i],"HERO");
        strcpy(g->encounter.actors[i].condition,"OK");
        g->encounter.actors[i].hp=g->encounter.actors[i].hp_limit=30;
        g->encounter.actors[i].magic=g->encounter.actors[i].magic_limit=20;
        for(j=1;j<10;++j) {g->encounter.attributes[i].stats[j]=10;g->encounter.attributes[i].skills[j]=3;}
        s.world.locations[i]=-3000;s.world.extra_locations[i]=-3000;
    }
    for(i=0;i<22;++i) {
        g->monster_values[i][0]=10;g->monster_values[i][1]=1;
        g->monster_values[i][2]=1;g->monster_values[i][3]=1;
        g->monster_values[i][4]=1;g->monster_values[i][5]=1;
    }
    for(i=0;i<4;++i) g->helpers[i]=(uint8_t)(3*(i+1));
    for(i=0;i<1100;++i) {s.world.maps[i][6]=1;g->maps_column7[i]=1;}
    for(i=0;i<9;++i) s.world.horizontal_mountains[i]=-3000;
    for(i=0;i<25;++i) s.world.vertical_mountains[i]=-3000;
    for(i=0;i<20;++i) s.world.towns[i]=(int16_t)(1201+i);
    s.world.alfol_position=45;s.world.initial_flag=1;
    strcpy(g->secret_word,"SECRET");strcpy(s.world.secret_words[1],"VOYAGE");
    s.city_backup_position=1203;
    return s;
}
static void same_state(const TeraExplorationState *a,const TeraExplorationState *b)
{
    assert(memcmp(a,b,sizeof *a)==0);
}
static void type_keys(TeraGameSession *s,const char *keys)
{
    while(*keys) assert(tera_game_key(s,(unsigned char)*keys++));
}
static void test_move_and_arrival(void)
{
    TeraExplorationState state=fixture(),expected;
    TeraDaySession day;TeraMoveReport move;
    TeraGameSession s,before;
    state.sound=1;expected=state;
    assert(tera_exploration_move(&expected,'8',&move));
    assert(tera_day_start(&day,&expected,NULL,NULL) && day.phase==TERA_DAY_DONE);
    expected=day.state;assert(tera_places_present(&expected,0));
    assert(tera_game_start(&s,&state,NULL,NULL,NULL));
    assert(tera_game_key(&s,'8') && s.phase==TERA_GAME_READY);
    same_state(&s.state,&expected); /* Comprend graine, facade et jours. */
    before=s;assert(!tera_game_key(&s,256));assert(!tera_game_key(&s,0));
    assert(!tera_game_key(&s,'Z'));assert(memcmp(&s,&before,sizeof s)==0);
    /* Le replay d'arrivee ne rajoute pas de case ni deuxieme facade. */
    state.world.characters.arrival_pending=1;
    expected=state;expected.world.characters.arrival_pending=0;
    --expected.world.characters.combat.position;
    assert(tera_exploration_move(&expected,'8',&move));
    assert(tera_day_start(&day,&expected,NULL,NULL) && day.phase==TERA_DAY_DONE);
    expected=day.state;assert(tera_places_present(&expected,0));
    assert(tera_game_start(&s,&state,NULL,NULL,NULL) && s.phase==TERA_GAME_READY);
    same_state(&s.state,&expected);
    assert(s.state.world.characters.combat.position==1200);
    /* Meme reprise pour le drapeau de fuite, pas un nouveau jet de fuite. */
    state.world.characters.arrival_pending=0;state.world.characters.combat.encounter.fled=1;
    assert(tera_game_start(&s,&state,NULL,NULL,NULL));same_state(&s.state,&expected);
}
static void test_resident_and_shop(void)
{
    TeraExplorationState state=fixture();TeraGameSession s;
    TeraDialogueSession dialogue;TeraShopSession shop;
    assert(tera_game_start(&s,&state,NULL,NULL,NULL));
    assert(tera_dialogue_start(&dialogue,&state,'N'));
    assert(tera_dialogue_key(&dialogue,'A'));assert(tera_dialogue_key(&dialogue,'O'));
    type_keys(&s,"nao");assert(s.phase==TERA_GAME_READY);same_state(&s.state,&dialogue.state);
    /* L'entree Kronopolis se confirme avant le menu ; ESC premier relit. */
    state.town_found=1;state.town_id=0;
    assert(tera_game_start(&s,&state,NULL,NULL,NULL));
    type_keys(&s,"en");assert(s.phase==TERA_GAME_READY);same_state(&s.state,&state);
    assert(tera_shop_start(&shop,&state));shop.phase=TERA_SHOP_CONFIRM;shop.command='E';
    assert(tera_shop_key(&shop,'O'));
    assert(tera_shop_key(&shop,'A'));assert(tera_shop_key(&shop,'O'));
    assert(tera_shop_key(&shop,'A'));assert(tera_shop_key(&shop,'A'));
    assert(tera_shop_key(&shop,27));assert(tera_shop_key(&shop,27));
    type_keys(&s,"eoaoaa\033\033");
    assert(s.phase==TERA_GAME_READY && s.state.world.characters.combat.equipment[0][0]==17);
    same_state(&s.state,&shop.state);
    /* Une annulation de confirmation ne quitte pas le jeu. */
    type_keys(&s,"#n");assert(s.phase==TERA_GAME_READY && !s.state.sound);
    type_keys(&s,"h\033n");assert(s.phase==TERA_GAME_READY);
    type_keys(&s,"h\033o");assert(s.phase==TERA_GAME_QUIT);
    assert(!tera_game_key(&s,'8'));
}
static void test_navigation_roundtrip(void)
{
    TeraExplorationState state=fixture(),expected;
    TeraNavigationSession nav;TeraGameSession s;
    state.town_found=1;state.town_id=2;state.world.characters.combat.position=1203;
    state.world.maps[1][2]=1;
    assert(tera_navigation_enter(&nav,&state,NULL,NULL));
    assert(tera_navigation_key(&nav,'O') && nav.phase==TERA_NAV_DONE);
    expected=nav.state;
    assert(tera_game_start(&s,&state,NULL,NULL,NULL));type_keys(&s,"eo");
    assert(s.phase==TERA_GAME_READY);same_state(&s.state,&expected);
    assert(tera_navigation_start(&nav,&expected,'2',0,1203,NULL,NULL));
    assert(nav.phase==TERA_NAV_DONE);expected=nav.state;
    assert(tera_navigation_leave(&expected,1203));assert(tera_places_present(&expected,1));
    assert(tera_game_key(&s,'2') && s.phase==TERA_GAME_READY);
    same_state(&s.state,&expected);
    assert(s.state.world.characters.combat.world_id==11);
}
static void test_meduz_return_and_delayed_ending(void)
{
    TeraExplorationState state=fixture(),expected;TeraGameSession s;
    TeraGameContext control={9,1202,1204,4,0,0};
    TeraMeduzSession meduz;TeraEndingSession ending;unsigned i;
    state.world.characters.combat.world_id=10;state.world.characters.combat.position=999+1;
    state.world.characters.combat.previous_position=999;
    for(i=0;i<4;++i) state.world.tasks_done[i]=1;
    assert(tera_game_start(&s,&state,&control,NULL,NULL) && s.phase==TERA_GAME_READY);
    /* La victoire attend la sortie de l'overlay ; aucune musique en avance. */
    assert(s.state.world.characters.combat.random.seed==1986);
    state.world.maps[1000][2]=1;s.state.world.maps[1000][2]=1;
    assert(tera_meduz_start(&meduz,&state,'2',0,0,1,1204,NULL,NULL));
    assert(meduz.phase==TERA_MEDUZ_DONE);expected=meduz.state;
    assert(expected.world.characters.combat.world_id==11);
    assert(tera_ending_start(&ending,&expected));
    assert(tera_game_key(&s,'2') && s.phase==TERA_GAME_ENDING);
    same_state(&s.state,&expected);
    assert(strcmp(s.active.ending.score_text,ending.score_text)==0);
    assert(tera_game_key(&s,13) && s.phase==TERA_GAME_COMPLETE);
    assert(!tera_game_key(&s,13));
}
static void test_mandatory_and_succession(void)
{
    TeraExplorationState state=fixture(),expected;TeraGameSession s;
    TeraGameContext control={9,1202,1204,4,0,1};
    TeraLifeSession life;unsigned i;
    state.world.characters.combat.world_id=8;state.world.characters.combat.position=800;
    state.navigation_overlay=2;state.world.characters.combat.encounter.party_available=0;
    strcpy(state.world.characters.combat.encounter.actors[0].condition,"HS");
    state.world.characters.combat.encounter.actors[0].hp=0;
    state.world.maps[1099][5]=19;state.world.tasks_done[1]=1;
    assert(tera_game_start(&s,&state,&control,NULL,NULL) && s.phase==TERA_GAME_READY);
    /* Une commande refusee ne consomme pas l'obligation. */
    assert(!tera_game_key(&s,'Z'));assert(s.control.mandatory_command==1);
    type_keys(&s,"nn");assert(s.phase==TERA_GAME_LIFE);
    assert(s.state.world.characters.combat.world_id==11);
    expected=state;
    assert(tera_navigation_leave(&expected,state.city_backup_position));
    assert(tera_places_present(&expected,1));
    assert(tera_life_start(&life,&expected));
    same_state(&s.active.life.state,&life.state);
    type_keys(&s,"\rNEXT\rH333FIVEHAO");
    assert(s.phase==TERA_GAME_READY);
    assert(s.state.world.maps[1099][5]==19 && s.state.world.tasks_done[1]);
    assert(!s.state.world.characters.arrival_pending);
    assert(s.state.world.characters.combat.position==1000);
    assert(s.state.world.characters.days==7301);
    /* Defaite avec les quatre objectifs => condoleances, aucun successeur. */
    control.mandatory_command=0;
    for(i=0;i<4;++i) state.world.tasks_done[i]=1;
    assert(tera_game_start(&s,&state,&control,NULL,NULL) && s.phase==TERA_GAME_ENDING);
    assert(!s.active.ending.victory);
    assert(tera_game_key(&s,'O') && s.phase==TERA_GAME_COMPLETE);
}
static void test_space_roundtrip_and_halt(void)
{
    TeraExplorationState state=fixture();TeraGameSession s;
    unsigned shots=0;int16_t days;
    state.town_found=1;state.town_id=1;state.world.characters.combat.position=1202;
    assert(tera_game_start(&s,&state,NULL,NULL,NULL));type_keys(&s,"eon");
    assert(s.phase==TERA_GAME_SPACE && s.active.space.phase==TERA_SPACE_SHOT);
    while(s.phase==TERA_GAME_SPACE) {
        unsigned key=(unsigned)('1'+3*(1-s.active.space.alien_y)+s.active.space.alien_x+1);
        assert(++shots<200);assert(tera_game_key(&s,key));
    }
    assert(s.phase==TERA_GAME_READY && s.state.world.characters.combat.on_alfol);
    assert(s.state.world.characters.combat.position==0);
    assert(s.control.space_backup_position==1202 && s.control.status_actor==16);
    days=s.state.world.characters.days;
    type_keys(&s,"ron"); /* R lit UNE touche ; la suivante alimente le fuel. */
    shots=0;
    while(s.phase==TERA_GAME_SPACE) {
        unsigned key=(unsigned)('1'+3*(1-s.active.space.alien_y)+s.active.space.alien_x+1);
        assert(++shots<200);assert(tera_game_key(&s,key));
    }
    assert(s.phase==TERA_GAME_READY && !s.state.world.characters.combat.on_alfol);
    assert(s.state.world.characters.combat.position==1202);
    assert(s.state.world.characters.days==days+7);
    state.world.characters.energy=4;
    assert(tera_game_start(&s,&state,NULL,NULL,NULL));type_keys(&s,"eon");
    assert(tera_game_key(&s,0) && s.phase==TERA_GAME_HALTED);
    assert(!tera_game_key(&s,'O'));
}
static void test_create_and_invalid_context(void)
{
    TeraExplorationState state=fixture();TeraGameSession s,before;
    assert(tera_game_create(&s,&state,NULL,NULL));
    type_keys(&s,"NEW\rH333FIVEHAO");assert(s.phase==TERA_GAME_READY);
    assert(s.state.world.characters.combat.position==1000);
    assert(s.state.world.characters.days==7301);
    before=s;state.world.characters.combat.world_id=10;
    assert(!tera_game_start(&s,&state,NULL,NULL,NULL));assert(memcmp(&s,&before,sizeof s)==0);
}

typedef struct { TeraCombatEvent events[256];unsigned count; } EventLog;
static void record_event(void *context,const TeraCombatEvent *event)
{
    EventLog *log=context;
    assert(log->count<256);log->events[log->count++]=*event;
}
static void test_nested_combat(void)
{
    TeraExplorationState state=fixture();TeraGameSession s;
    TeraNavigationSession nav;EventLog direct={0},global={0};unsigned turns=0,i;
    state.world.characters.combat.world_id=1;state.navigation_overlay=1;
    state.world.characters.combat.position=100;state.world.characters.combat.previous_position=99;
    state.world.maps[100][0]=1;state.world.maps[101][5]=7;
    state.world.characters.combat.equipment[0][0]=18;
    assert(tera_navigation_start(&nav,&state,'8',0,state.city_backup_position,record_event,&direct));
    assert(tera_game_start(&s,&state,NULL,record_event,&global));
    assert(tera_game_key(&s,'8') && s.phase==TERA_GAME_NAVIGATION);
    assert(nav.phase==TERA_NAV_DAY && nav.day.phase==TERA_DAY_COMBAT);
    while(nav.phase==TERA_NAV_DAY) {
        unsigned key=nav.day.combat.phase==TERA_COMBAT_ACTION ? 'T' : 'O';
        assert(++turns<100);
        assert(s.active.navigation.day.combat.phase==nav.day.combat.phase);
        assert(s.active.navigation.day.combat.game.random.seed==nav.day.combat.game.random.seed);
        assert(tera_navigation_key(&nav,key));assert(tera_game_key(&s,key));
    }
    assert(nav.phase==TERA_NAV_DONE && s.phase==TERA_GAME_READY);
    same_state(&s.state,&nav.state);
    assert(!s.state.world.maps[101][5]);
    assert(direct.count && direct.count==global.count);
    for(i=0;i<direct.count;++i) {
        assert(direct.events[i].kind==global.events[i].kind);
        assert(direct.events[i].actor==global.events[i].actor);
        assert(direct.events[i].letter==global.events[i].letter);
        assert(direct.events[i].value==global.events[i].value);
    }
}
static void test_other_mandatory_arrivals(void)
{
    TeraExplorationState state=fixture(),expected;TeraGameSession s;
    TeraGameContext control={16,1202,1204,4,0,2};TeraAlfolSession alfol;
    TeraMeduzSession meduz;TeraLifeSession life;
    state.world.characters.combat.world_id=9;state.world.characters.combat.position=900;
    state.world.characters.combat.on_alfol=1;
    state.world.characters.combat.encounter.party_available=0;
    state.world.characters.combat.encounter.actors[0].hp=0;
    strcpy(state.world.characters.combat.encounter.actors[0].condition,"HS");
    assert(tera_game_start(&s,&state,&control,NULL,NULL) && s.phase==TERA_GAME_READY);
    type_keys(&s,"nn");assert(s.phase==TERA_GAME_LIFE);
    assert(tera_alfol_leave(&alfol,&state,16));expected=alfol.state;
    assert(tera_life_start(&life,&expected));same_state(&s.active.life.state,&life.state);
    assert(s.control.status_actor==16);
    state.world.characters.combat.on_alfol=0;
    state.world.characters.combat.world_id=10;state.world.characters.combat.position=1000;
    control.mandatory_command=3;
    assert(tera_game_start(&s,&state,&control,NULL,NULL) && s.phase==TERA_GAME_READY);
    type_keys(&s,"nn");assert(s.phase==TERA_GAME_LIFE);
    assert(tera_meduz_resume(&meduz,&state,1204));expected=meduz.state;
    assert(tera_life_start(&life,&expected));same_state(&s.active.life.state,&life.state);
    assert(s.state.world.characters.days==state.world.characters.days+7);
}
static void test_meduz_entry_and_errors(void)
{
    TeraExplorationState state=fixture();TeraGameSession s,before;
    TeraMeduzSession direct;
    state.town_found=1;state.town_id=3;
    assert(tera_game_start(&s,&state,NULL,NULL,NULL));
    assert(tera_meduz_start(&direct,&state,'M',1,0,1,1200,NULL,NULL));
    type_keys(&s,"moFAUX\r");
    assert(s.phase==TERA_GAME_READY);same_state(&s.state,&state);
    assert(tera_meduz_key(&direct,'O'));
    {const char *word="VOYAGE\r";while(*word) assert(tera_meduz_key(&direct,(unsigned char)*word++));}
    type_keys(&s,"moVOYAGE\r");
    assert(s.phase==TERA_GAME_READY && direct.phase==TERA_MEDUZ_DONE);
    same_state(&s.state,&direct.state);assert(s.control.meduz_backup_position==1200);
    before=s;assert(!tera_game_key(&s,0));assert(memcmp(&s,&before,sizeof s)==0);
    /* Une table de monstre invalide est une erreur, jamais une victoire
     * ni une permission de continuer a commander le personnage. */
    state.world.characters.combat.world_id=1;state.navigation_overlay=1;
    state.world.characters.combat.position=100;state.world.characters.combat.previous_position=99;
    state.world.maps[100][0]=1;state.world.maps[101][5]=23;
    assert(tera_game_start(&s,&state,NULL,NULL,NULL));
    assert(tera_game_key(&s,'8') && s.phase==TERA_GAME_ERROR);
    assert(!tera_game_key(&s,'O'));
}
static void test_actual_mortal_entries(void)
{
    TeraExplorationState state=fixture();TeraGameSession s;
    TeraGameContext control={9,1202,1204,4,0,0};
    /* Le rendu/jour, pas le routage, normalise le drapeau de disponibilite.
     * On place donc un heros HS devant chaque entree encore non rafraichie. */
    state.world.characters.combat.encounter.actors[0].hp=0;
    strcpy(state.world.characters.combat.encounter.actors[0].condition,"HS");
    state.town_found=1;state.town_id=4;
    assert(tera_game_start(&s,&state,NULL,NULL,NULL));type_keys(&s,"eo");
    assert(s.phase==TERA_GAME_READY && s.control.mandatory_command==1);
    assert(s.state.world.characters.combat.world_id==8);
    /* L est immobile : son retour doit quand meme redessiner Seth. */
    {TeraExplorationState expected=s.state;TeraNavigationSession nav;TeraLifeSession life;
     assert(tera_navigation_start(&nav,&expected,'L',0,1200,NULL,NULL));
     assert(tera_navigation_key(&nav,'O'));expected=nav.state;
     assert(tera_navigation_leave(&expected,1200));assert(tera_places_present(&expected,1));
     assert(tera_life_start(&life,&expected));
     type_keys(&s,"lo");assert(s.phase==TERA_GAME_LIFE);
     same_state(&s.active.life.state,&life.state);}
    state.world.characters.combat.on_alfol=1;
    state.world.characters.combat.position=state.world.alfol_position;
    assert(tera_game_start(&s,&state,&control,NULL,NULL));type_keys(&s,"eo");
    assert(s.phase==TERA_GAME_READY && s.control.mandatory_command==2);
    type_keys(&s,"nn");assert(s.phase==TERA_GAME_LIFE);
    state.world.characters.combat.on_alfol=0;state.world.characters.combat.position=1200;
    state.town_id=3;
    assert(tera_game_start(&s,&state,NULL,NULL,NULL));type_keys(&s,"moVOYAGE\r");
    assert(s.phase==TERA_GAME_READY && s.control.mandatory_command==3);
    type_keys(&s,"nn");assert(s.phase==TERA_GAME_LIFE);
}
int main(void)
{
    test_move_and_arrival();test_resident_and_shop();test_navigation_roundtrip();
    test_meduz_return_and_delayed_ending();test_mandatory_and_succession();
    test_space_roundtrip_and_halt();test_create_and_invalid_context();
    test_nested_combat();test_other_mandatory_arrivals();test_meduz_entry_and_errors();
    test_actual_mortal_entries();
    puts("Orchestration C : contrats et raccordements valides.");return 0;
}
