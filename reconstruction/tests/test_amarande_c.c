#include "module_test_support.h"
#include "tera/amarande.h"
#include "tera/commerce.h"

static void keys(TeraAmarandeSession *s,const char *input)
{
    while(*input) assert(tera_amarande_key(s,(unsigned char)*input++));
}
static void test_facades_and_information(void)
{
    const unsigned counts[10]={0,89,23,76,18,18,0,0,0,0};
    TeraExplorationState w=module_fixture(),s,before;unsigned town,mode;
    for(town=0;town<10;++town) {
        s=w;s.town_found=1;s.town_id=town;s.moved=1;
        assert(tera_places_present(&s,0) && !s.moved);
        assert(s.world.characters.combat.random.seed==seed_after(1986,counts[town]));
        assert(s.world.characters.days==7300);before=s;
        assert(tera_places_present(&s,0));assert(memcmp(&s,&before,sizeof s)==0);
    }
    for(mode=0;mode<4;++mode) {
        s=w;s.town_id=5;s.town_found=1;s.moved=1;
        s.world.locations[3]=mode==3 ? -1 : s.world.towns[5];
        s.world.characters.combat.encounter.actors[3].active=mode==0;
        if(mode==1) strcpy(s.world.characters.combat.encounter.actors[3].condition,"HS");
        assert(tera_places_present(&s,0));assert(s.information_context==(mode>=2));
    }
    s=w;s.town_id=7;s.town_found=1;s.moved=1;s.information_context=1;
    s.world.locations[5]=s.world.towns[7];s.world.characters.combat.encounter.actors[5].active=1;
    assert(tera_places_present(&s,0) && s.information_context); /* Pas de remise a zero. */
    s=w;s.town_found=1;s.town_id=2;assert(tera_places_present(&s,1));
    assert(s.world.characters.combat.random.seed==seed_after(1986,23));
    before=s;assert(!tera_places_present(&s,2));assert(memcmp(&s,&before,sizeof s)==0);
}
static void test_signs_pause_and_halt(void)
{
    TeraExplorationState w=module_fixture();TeraAmarandeSession s,before;
    w.town_found=1;w.town_id=10;w.world.towns[0]=1139; /* delta=-61, DIV vers zero. */
    assert(tera_amarande_start(&s,&w,'L',9,NULL,NULL));keys(&s,"o");
    assert(s.phase==TERA_AM_PAUSE && s.east==-2 && s.north==-1 && s.status_actor==-1);
    assert(s.state.world.characters.combat.random.seed==1986);
    keys(&s,"x");assert(s.phase==TERA_AM_DONE);
    assert(tera_amarande_start(&s,&w,'U',9,NULL,NULL));keys(&s,"o");
    assert(s.phase==TERA_AM_PAUSE && s.response_id==15);
    keys(&s,"x");assert(s.phase==TERA_AM_HALT);keys(&s,"y");assert(s.phase==TERA_AM_DONE);
    assert(s.state.world.characters.combat.random.seed==seed_after(1986,1));
    before=s;assert(!tera_amarande_key(&s,13));assert(memcmp(&s,&before,sizeof s)==0);
    w.world.fast_moves=1;
    assert(tera_amarande_start(&s,&w,'H',9,NULL,NULL));keys(&s,"an");
    assert(!s.state.world.fast_moves); /* Refuser desactive, ne preserve pas vrai. */
    assert(tera_amarande_start(&s,&w,'H',9,NULL,NULL));keys(&s,"h");
    assert(s.status_actor==8 && s.state.world.characters.combat.random.seed==seed_after(1986,28));
    w.world.initial_flag=0;
    assert(tera_amarande_start(&s,&w,'H',9,NULL,NULL));keys(&s,"c");
    assert(s.outcome==4 && s.response_id==20);
    w.world.initial_flag=1;
    assert(tera_amarande_start(&s,&w,'H',9,NULL,NULL));keys(&s,"c");
    assert(s.outcome==3 && s.state.world.characters.combat.random.seed==seed_after(1986,1));
    assert(tera_amarande_start(&s,&w,'#',9,NULL,NULL));before=s;
    assert(!tera_amarande_key(&s,256));assert(memcmp(&s,&before,sizeof s)==0);
    keys(&s,"o");assert(s.state.sound && s.state.world.characters.combat.random.seed==1986);
}
static void test_invocation_filters_and_npc_victory(void)
{
    TeraExplorationState w=module_fixture();TeraAmarandeSession s;unsigned turns=0;
    w.world.towns[0]=1200;
    assert(tera_amarande_start(&s,&w,'I',9,NULL,NULL));keys(&s,"ARIOCH\r");
    assert(s.phase==TERA_AM_DONE && s.status_actor==9 && s.state.world.characters.combat.random.seed==1986);
    w.world.towns[0]=1201;w.world.characters.combat.on_alfol=1;
    assert(tera_amarande_start(&s,&w,'I',9,NULL,NULL));keys(&s,"ARIOCH\r");assert(s.phase==TERA_AM_DONE);
    w.world.characters.combat.on_alfol=0;
    assert(tera_amarande_start(&s,&w,'I',9,NULL,NULL));keys(&s,"ARIOCH\r");
    assert(s.phase==TERA_AM_COMBAT && s.combat.game.encounter.monster_type==21 && s.status_actor==120);
    assert(s.state.encounter_suppressed && s.state.information_context);
    w.world.locations[1]=1200;w.world.characters.combat.equipment[0][0]=18;
    assert(tera_amarande_start(&s,&w,'C',9,NULL,NULL));keys(&s,"o");
    assert(s.phase==TERA_AM_COMBAT && s.combat.game.encounter.monster_type==10);
    while(s.phase==TERA_AM_COMBAT) {
        unsigned key=s.combat.phase==TERA_COMBAT_ACTION ? 'T' : 'O';
        assert(++turns<100);assert(tera_amarande_key(&s,key));
    }
    assert(s.phase==TERA_AM_DONE && !strcmp(s.state.world.characters.combat.encounter.actors[1].condition,"HS"));
    assert(s.state.world.characters.combat.encounter.actors[1].hp==30); /* Statut seul. */
    assert(!s.state.world.characters.combat.encounter.actors[1].active);
    assert(s.state.world.characters.days==7300);
}
int main(void)
{
    test_facades_and_information();test_signs_pause_and_halt();test_invocation_filters_and_npc_victory();return 0;
}
