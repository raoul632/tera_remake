#include "module_test_support.h"
#include "tera/game.h"
#include "tera/save.h"
#include <stdio.h>

typedef struct {TeraPresentationEvent events[512];unsigned count;} Capture;
static void collect(void *context,const TeraPresentationEvent *e)
{
    Capture *c=context;assert(c->count<512);c->events[c->count++]=*e;
}
static unsigned count_kind(const Capture *c,TeraPresentationKind kind)
{
    unsigned i,n=0;for(i=0;i<c->count;++i) n+=c->events[i].kind==kind;return n;
}
static const TeraPresentationEvent *last(const Capture *c,TeraPresentationKind kind)
{
    unsigned i=c->count;while(i--) if(c->events[i].kind==kind) return &c->events[i];
    assert(0);return NULL;
}
static void type(TeraGameSession *s,const char *keys)
{
    while(*keys) assert(tera_game_key(s,(unsigned char)*keys++));
}
static void canonical_equal(const TeraGameSession *a,const TeraGameSession *b)
{
    unsigned char left[TERA_SAVE_CAPACITY],right[TERA_SAVE_CAPACITY];size_t n,m;
    assert(tera_save_encode(a,left,sizeof left,&n)==TERA_SAVE_OK);
    assert(tera_save_encode(b,right,sizeof right,&m)==TERA_SAVE_OK);
    assert(n==m && !memcmp(left,right,n));
}
static void test_lifetime_and_rejection(void)
{
    Capture c={0};TeraPresentationSink sink={collect,&c};
    TeraExplorationState w=module_fixture();TeraGameSession s,before;TeraGameView v;
    TeraPresentationEvent retained;
    assert(tera_game_start(&s,&w,NULL,NULL,NULL));
    assert(tera_game_set_presentation(&s,&sink));before=s;
    assert(!tera_game_key(&s,256) && !tera_game_key(&s,'Z') && !tera_game_key(&s,0));
    assert(!c.count && !memcmp(&s,&before,sizeof s));
    assert(tera_game_view(&s,&v));assert(v.prompt.kind==TERA_PROMPT_COMMAND);
    assert(!c.count && !memcmp(&s,&before,sizeof s));
    assert(tera_game_key(&s,'L'));assert(s.phase==TERA_GAME_READY);
    retained=*last(&c,TERA_PRESENT_MESSAGE);
    assert(retained.data.message.id==8 && retained.data.message.variant<3);
    assert(tera_game_key(&s,'H'));assert(s.phase==TERA_GAME_AMARANDE);
    assert(!memcmp(&retained,&c.events[0],sizeof retained));
    assert(last(&c,TERA_PRESENT_MESSAGE)->data.message.id==7);
    assert(tera_game_view(&s,&v));assert(v.prompt.kind==TERA_PROMPT_MENU);
    before=s;assert(tera_game_set_presentation(&s,NULL));
    assert(tera_game_key(&s,'I'));assert(c.count==2);
    (void)before;
}
static void test_movement_and_doors(void)
{
    Capture c={0};TeraPresentationSink sink={collect,&c};TeraMoveReport move;
    TeraExplorationState w=module_fixture();TeraGameSession s;TeraGameView v;
    w.world.characters.combat.presentation=&sink;
    w.world.characters.combat.position=1899;
    assert(tera_exploration_move(&w,'8',&move));
    assert(last(&c,TERA_PRESENT_MOVE)->data.move.from==1899);
    assert(last(&c,TERA_PRESENT_MOVE)->data.move.to==1000);
    w.world.horizontal_mountains[0]=1000;
    assert(tera_exploration_move(&w,'8',&move));
    assert(last(&c,TERA_PRESENT_BLOCKED)->position==1000);
    w=module_fixture();w.world.characters.combat.world_id=0;
    w.world.characters.combat.position=1;w.world.characters.combat.previous_position=0;
    w.world.characters.combat.presentation=&sink;
    assert(tera_game_start(&s,&w,NULL,NULL,NULL));
    assert(tera_game_key(&s,'8'));assert(s.phase==TERA_GAME_READY);
    assert(last(&c,TERA_PRESENT_BLOCKED)->source==TERA_SOURCE_NAVIGATION);
    w.world.maps[1][0]=3;
    assert(tera_game_start(&s,&w,NULL,NULL,NULL));
    assert(tera_game_key(&s,'8'));assert(tera_game_view(&s,&v));
    assert(v.prompt.kind==TERA_PROMPT_GATE && v.prompt.confirm_key=='U' && !v.prompt.accepts_enter);
    assert(tera_game_key(&s,'X'));assert(s.phase==TERA_GAME_READY);
    assert(last(&c,TERA_PRESENT_BLOCKED)->data.move.wall==0);
    assert(tera_game_key(&s,'8'));assert(tera_game_key(&s,'U'));
    assert(last(&c,TERA_PRESENT_MOVE)->data.move.from==1);
    assert(last(&c,TERA_PRESENT_MOVE)->data.move.to==2);
    w=module_fixture();w.town_found=1;w.town_id=2;
    w.world.characters.combat.presentation=&sink;
    assert(tera_game_start(&s,&w,NULL,NULL,NULL));
    type(&s,"en");assert(!count_kind(&c,TERA_PRESENT_ZONE));
    type(&s,"eo");
    assert(last(&c,TERA_PRESENT_ZONE)->data.zone.from_world==11);
    assert(last(&c,TERA_PRESENT_ZONE)->data.zone.from_position==1200);
    assert(last(&c,TERA_PRESENT_ZONE)->world==0 && last(&c,TERA_PRESENT_ZONE)->position==1);
}
static void test_shop_inventory_and_save(void)
{
    Capture c={0};TeraPresentationSink sink={collect,&c};
    TeraExplorationState w=module_fixture();TeraGameSession s,plain,loaded;TeraGameView v;
    unsigned char bytes[TERA_SAVE_CAPACITY];size_t size;unsigned before;
    w.town_found=1;w.town_id=0;
    assert(tera_game_start(&s,&w,NULL,NULL,NULL));plain=s;
    tera_game_set_presentation(&s,&sink);
    type(&s,"eoaoaa");type(&plain,"eoaoaa");
    assert(count_kind(&c,TERA_PRESENT_SHOP)==1);
    assert(last(&c,TERA_PRESENT_SHOP)->data.shop.outcome==1);
    assert(last(&c,TERA_PRESENT_SHOP)->data.shop.item==17);
    assert(last(&c,TERA_PRESENT_SHOP)->data.shop.price==s.active.shop.price);
    assert(tera_game_view(&s,&v));assert(v.prompt.source==TERA_SOURCE_SHOP);
    type(&s,"\033ho\033");type(&plain,"\033ho\033");canonical_equal(&s,&plain);
    assert(last(&c,TERA_PRESENT_SHOP)->data.shop.outcome==3);
    assert(last(&c,TERA_PRESENT_SHOP)->data.shop.weeks>0);
    assert(count_kind(&c,TERA_PRESENT_ACTOR)==10);
    /* Objet retire : le rapport conserve son ID apres effacement du slot. */
    type(&s,"aoa0");type(&plain,"aoa0");canonical_equal(&s,&plain);
    assert(last(&c,TERA_PRESENT_INVENTORY)->data.inventory.item==17);
    assert(last(&c,TERA_PRESENT_INVENTORY)->data.inventory.outcome==TERA_ITEM_DROPPED);
    before=c.count;assert(tera_save_encode(&s,bytes,sizeof bytes,&size)==TERA_SAVE_OK);
    assert(size==11521);assert(tera_save_decode(&loaded,bytes,size,NULL,NULL,NULL)==TERA_SAVE_OK);
    assert(!loaded.state.world.characters.combat.presentation && c.count==before);
    assert(tera_game_set_presentation(&loaded,&sink));assert(c.count==before);
    canonical_equal(&s,&loaded);
    assert(tera_game_key(&loaded,'L'));assert(c.count>before);
}
static void test_chest_and_transient_results(void)
{
    Capture c={0};TeraPresentationSink sink={collect,&c};
    TeraExplorationState w=module_fixture();TeraGameSession s;
    const TeraPresentationEvent *e;unsigned seed;
    w.world.characters.combat.world_id=1;w.world.characters.combat.position=105;
    w.world.characters.combat.previous_position=104;w.navigation_overlay=1;
    w.world.maps[105][4]=30;w.world.characters.combat.presentation=&sink;
    assert(tera_game_start(&s,&w,NULL,NULL,NULL));type(&s,"ooa");
    assert(s.phase==TERA_GAME_READY);e=last(&c,TERA_PRESENT_CHEST);
    assert(e->data.chest.content==30 && e->data.chest.reward==10);
    assert(!s.state.world.maps[105][4]);
    /* Rechercher une graine du piege sans forcer son resultat dans le jeu. */
    for(seed=0;seed<100;++seed) {
        TeraChestSession chest;
        w.world.characters.combat.random.seed=seed;
        w.world.characters.combat.encounter.attributes[0].stats[7]=0;
        w.world.characters.combat.encounter.attributes[0].stats[9]=0;
        w.world.characters.combat.encounter.attributes[0].skills[7]=0;
        w.world.characters.combat.presentation=NULL;
        assert(tera_chest_start(&chest,&w,'O'));assert(tera_chest_key(&chest,'O'));assert(tera_chest_key(&chest,'A'));
        if(chest.trap>=0) {
            TeraChestSession observed;
            w.world.characters.combat.presentation=&sink;
            assert(tera_chest_start(&observed,&w,'O'));assert(tera_chest_key(&observed,'O'));assert(tera_chest_key(&observed,'A'));
            assert(last(&c,TERA_PRESENT_TRAP)->data.trap.kind==chest.trap);
            assert(count_kind(&c,TERA_PRESENT_ACTOR)>0);
            assert(c.events[c.count-1].kind==TERA_PRESENT_CHEST);
            observed.state.world.characters.combat.presentation=NULL;
            assert(!memcmp(&observed.state,&chest.state,sizeof chest.state));break;
        }
    }
    assert(seed<100);
    {
        TeraChestSession chest;
        w.world.maps[105][4]=99;w.chest_open=1;w.world.characters.combat.presentation=&sink;
        assert(tera_chest_start(&chest,&w,'P'));assert(tera_chest_key(&chest,'O'));
        assert(last(&c,TERA_PRESENT_CHEST)->data.chest.actor==~0u);
    }
}
static void test_messages_progress_and_ending(void)
{
    Capture c={0};TeraPresentationSink sink={collect,&c};TeraGameView v;
    TeraExplorationState w=module_fixture();TeraGameSession s;TeraGameContext control={0};unsigned before;
    w.world.characters.combat.presentation=&sink;
    assert(tera_game_start(&s,&w,NULL,NULL,NULL));type(&s,"nao");
    assert(last(&c,TERA_PRESENT_MESSAGE)->data.message.id==22);
    /* Indices lus puis detruits par l'operation suivante : valeurs conservees. */
    w.world.extra_locations[1]=1200;w.information_context=1;
    assert(tera_game_start(&s,&w,NULL,NULL,NULL));type(&s,"nio");
    assert(last(&c,TERA_PRESENT_MESSAGE)->data.message.resource==TERA_TEXT_HINT);
    assert(last(&c,TERA_PRESENT_MESSAGE)->data.message.id==2);
    type(&s,"nso");assert(last(&c,TERA_PRESENT_RECRUIT)->data.recruit.outcome==TERA_RECRUIT_NOBODY);
    {
        TeraRecruitSession recruit;
        assert(tera_recruit_start(&recruit,&w.world,TERA_RECRUIT_SEDUCE,0));
        assert(tera_recruit_key(&recruit,'O'));
        assert(last(&c,TERA_PRESENT_MESSAGE)->data.message.resource==TERA_TEXT_COMMAND_RESPONSE);
        assert(last(&c,TERA_PRESENT_MESSAGE)->data.message.id=='S');
        assert(last(&c,TERA_PRESENT_MESSAGE)->data.message.variant<3);
    }
    w.world.characters.combat.world_id=10;w.world.characters.combat.position=1001;
    w.world.characters.combat.previous_position=1000;w.world.maps[1001][4]=3;w.world.mental_extension=1;
    control.status_actor=9;control.meduz_backup_position=1200;
    assert(tera_game_start(&s,&w,&control,NULL,NULL));type(&s,"ioMENTAL\r");
    assert(last(&c,TERA_PRESENT_OBJECTIVE)->data.objective.id==0);
    before=count_kind(&c,TERA_PRESENT_OBJECTIVE);type(&s,"ioMENTAL\r");
    assert(count_kind(&c,TERA_PRESENT_OBJECTIVE)==before);
    w=module_fixture();w.world.characters.combat.presentation=&sink;
    memset(w.world.tasks_done,1,sizeof w.world.tasks_done);w.world.tasks_counter=4;
    assert(tera_game_start(&s,&w,NULL,NULL,NULL));assert(s.phase==TERA_GAME_ENDING);
    assert(last(&c,TERA_PRESENT_ENDING)->data.ending.victory);
    assert(!strcmp(last(&c,TERA_PRESENT_ENDING)->data.ending.score,s.active.ending.score_text));
    assert(tera_game_view(&s,&v));assert(v.prompt.kind==TERA_PROMPT_PAUSE);
    assert(tera_game_key(&s,13));assert(tera_game_view(&s,&v));
    assert(v.prompt.kind==TERA_PROMPT_TERMINAL && v.ending && v.ending->victory);
}
static void test_creation_and_combat_projection(void)
{
    Capture c={0};TeraPresentationSink sink={collect,&c};
    TeraExplorationState w=module_fixture();TeraGameSession s,before;TeraGameView v;
    assert(tera_game_create(&s,&w,NULL,NULL));tera_game_set_presentation(&s,&sink);
    assert(tera_game_view(&s,&v));assert(v.prompt.kind==TERA_PROMPT_NAME);
    type(&s,"TERA\rH");before=s;
    assert(tera_game_view(&s,&v));assert(v.prompt.kind==TERA_PROMPT_SPLIT);
    assert(v.prompt.pool==s.active.creation.state.combat.encounter.attributes[0].stats[1]);
    assert(!memcmp(&s,&before,sizeof s));
    s.phase=TERA_GAME_DAY;s.active.day.phase=TERA_DAY_COMBAT;
    w.world.characters.combat.encounter.monster_type=7;
    assert(tera_combat_start(&s.active.day.combat,&w.world.characters.combat,0,NULL,NULL));
    assert(tera_game_set_presentation(&s,&sink));
    assert(tera_game_view(&s,&v));assert(v.prompt.source==TERA_SOURCE_COMBAT);
    assert(tera_game_key(&s,'F'));assert(tera_game_view(&s,&v));
    assert(v.prompt.kind==TERA_PROMPT_CONFIRM);
    assert(tera_game_key(&s,'N'));
    assert(count_kind(&c,TERA_PRESENT_COMBAT)>0);
}
static void test_character_snapshots(void)
{
    Capture c={0};TeraPresentationSink sink={collect,&c};
    TeraExplorationState w=module_fixture();TeraGameSession s,plain;
    TeraCharacterCard card;
    w.world.characters.combat.encounter.actors[3].active=1;
    w.world.characters.combat.encounter.actors[8].active=1;
    w.world.characters.combat.encounter.attributes[0].stats[5]=100;
    strcpy(w.world.characters.combat.encounter.actors[3].condition,"HS");
    assert(tera_game_start(&s,&w,NULL,NULL,NULL));plain=s;
    assert(tera_game_set_presentation(&s,&sink));
    type(&s,"hi");type(&plain,"hi");
    assert(s.phase==TERA_GAME_READY && count_kind(&c,TERA_PRESENT_CHARACTER)==1);
    card=last(&c,TERA_PRESENT_CHARACTER)->data.character;assert(card.actor==0);
    canonical_equal(&s,&plain);
    assert(tera_game_set_presentation(&s,&sink));
    type(&s,"hso");assert(last(&c,TERA_PRESENT_CHARACTER)->data.character.actor==3);
    assert(!strcmp(last(&c,TERA_PRESENT_CHARACTER)->data.character.condition,"HS"));
    type(&s,"x");assert(last(&c,TERA_PRESENT_CHARACTER)->data.character.actor==8);
    type(&s,"x");assert(s.phase==TERA_GAME_READY);
    assert(count_kind(&c,TERA_PRESENT_CHARACTER)==3);
    assert(card.actor==0); /* Photographie precedente toujours autonome. */
}
int main(void)
{
    test_lifetime_and_rejection();test_movement_and_doors();test_shop_inventory_and_save();
    test_chest_and_transient_results();test_messages_progress_and_ending();test_creation_and_combat_projection();
    test_character_snapshots();
    puts("presentation contracts OK");return 0;
}

