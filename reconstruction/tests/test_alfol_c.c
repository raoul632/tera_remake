#include "module_test_support.h"
#include "tera/alfol.h"

static TeraExplorationState building_fixture(void)
{
    TeraExplorationState w=module_fixture();
    w.world.characters.combat.on_alfol=1;w.world.characters.combat.world_id=9;
    w.world.characters.combat.position=900;w.world.characters.combat.previous_position=899;
    w.world.maps[900][0]=1;return w;
}
static void test_surface_gates_and_return(void)
{
    TeraExplorationState w=module_fixture();TeraAlfolSession s,before;
    w.world.characters.combat.on_alfol=1;w.world.characters.combat.position=0;
    w.world.extra_locations[1]=99; /* Pas de rencontre de surface dans cette fixture. */
    assert(tera_alfol_start(&s,&w,'2',0,9,NULL,NULL));
    assert(s.phase==TERA_ALFOL_DONE && s.state.world.characters.combat.position==99 && s.status_actor==16);
    assert(s.state.world.characters.days==7301);
    assert(tera_alfol_start(&s,&w,'R',0,9,NULL,NULL));assert(tera_alfol_key(&s,'N'));
    assert(s.phase==TERA_ALFOL_RETURN); /* Le refus au point zero sort aussi. */
    w.world.characters.combat.position=1;
    assert(tera_alfol_start(&s,&w,'R',0,9,NULL,NULL));assert(tera_alfol_key(&s,'O'));
    assert(s.phase==TERA_ALFOL_DONE && s.state.world.characters.combat.position==1);
    w=building_fixture();w.world.maps[900][0]=3;w.world.maps[900][1]=3;
    w.world.characters.keys[9]=9;
    assert(tera_alfol_start(&s,&w,'8',0,9,NULL,NULL) && s.phase==TERA_ALFOL_GATE);
    before=s;assert(!tera_alfol_key(&s,256));assert(memcmp(&s,&before,sizeof s)==0);
    assert(tera_alfol_key(&s,13) && !s.moved && s.state.world.maps[900][0]==3);
    assert(tera_alfol_start(&s,&w,'8',0,9,NULL,NULL));assert(tera_alfol_key(&s,'u'));
    assert(s.state.world.maps[900][0]==2 && s.state.world.maps[900][1]==2);
    assert(s.state.world.characters.keys[9]==9 && s.state.world.characters.combat.position==901);
    w.world.characters.keys[9]=10;
    assert(tera_alfol_start(&s,&w,'8',0,9,NULL,NULL) && s.phase==TERA_ALFOL_DONE && !s.moved);
    assert(tera_alfol_leave(&s,&w,9));
    assert(s.state.world.characters.combat.world_id==11 && s.state.world.characters.combat.on_alfol);
    assert(s.state.world.characters.combat.position==45 && s.state.world.characters.days==7300);
    assert(s.status_actor==16 && s.state.world.characters.combat.random.seed==seed_after(1986,173));
}
static void test_media_idempotence_and_confirmation(void)
{
    TeraExplorationState w=building_fixture();TeraAlfolSession s;unsigned pass;
    w.world.maps[901][4]=1;w.world.characters.combat.money=999;
    assert(tera_alfol_start(&s,&w,'8',0,9,NULL,NULL) && s.phase==TERA_ALFOL_MEDIA);
    assert(tera_alfol_key(&s,'O'));
    assert(!s.state.world.tasks_done[1] && s.state.world.characters.combat.money==999);
    w.world.characters.combat.money=2000;
    assert(tera_alfol_start(&s,&w,'8',0,9,NULL,NULL));assert(tera_alfol_key(&s,13));
    assert(!s.state.world.tasks_done[1] && s.state.world.characters.combat.money==2000);
    for(pass=0;pass<2;++pass) {
        assert(tera_alfol_start(&s,&w,'8',0,9,NULL,NULL));assert(tera_alfol_key(&s,'o'));
        assert(s.phase==TERA_ALFOL_DONE && s.state.world.tasks_done[1]);
        assert(s.state.world.tasks_counter==1);
        assert(s.state.world.characters.combat.money==(pass ? 0 : 1000));
        w=s.state;w.world.characters.combat.position=900;w.world.characters.combat.previous_position=899;
    }
}
static void test_radiation_alias_and_context_error(void)
{
    TeraExplorationState w=building_fixture();TeraAlfolSession s;unsigned i;
    w.world.maps[901][4]=2;w.world.characters.combat.encounter.actors[6].magic=-1;
    memset(w.world.hints[0],'X',sizeof w.world.hints[0]);w.world.hints[0][39]=0;
    assert(tera_alfol_start(&s,&w,'8',0,16,NULL,NULL));
    assert(s.phase==TERA_ALFOL_DONE && s.status_actor==16);
    assert(s.state.world.characters.combat.encounter.actors[6].magic==0);
    assert(s.state.world.hints[0][17]==2 && s.state.world.hints[0][18]=='H' && s.state.world.hints[0][19]=='S');
    for(i=0;i<39;++i) if(i<17 || i>19) assert(s.state.world.hints[0][i]=='X');
    assert(s.state.world.characters.combat.encounter.actors[0].hp<=30);
    assert(tera_alfol_start(&s,&w,'8',0,17,NULL,NULL) && s.phase==TERA_ALFOL_ERROR);
    assert(!tera_alfol_key(&s,'O'));
}
int main(void)
{
    test_surface_gates_and_return();test_media_idempotence_and_confirmation();
    test_radiation_alias_and_context_error();return 0;
}
