#include "module_test_support.h"
#include "tera/space.h"
static TeraPresentationEvent last_ray;static unsigned rays;
static void ray_observer(void *context,const TeraPresentationEvent *e)
{(void)context;if(e->kind==TERA_PRESENT_RAY) {last_ray=*e;++rays;}}

static unsigned target_key(const TeraSpaceSession *s)
{
    /* Table du pave de la specification, independante de la formule C
     * de reconnaissance du tir : elle detecte une inversion haut/bas. */
    static const unsigned keypad[3][3]={{'7','8','9'},{'4','5','6'},{'1','2','3'}};
    assert(s->alien_x>=-1 && s->alien_x<=1 && s->alien_y>=-1 && s->alien_y<=1);
    return keypad[s->alien_y+1][s->alien_x+1];
}
static void test_all_nine_directions(void)
{
    TeraExplorationState w=module_fixture();TeraSpaceSession s;unsigned seed,seen=0;
    for(seed=0;seed<128;++seed) {
        unsigned key;
        w.world.characters.combat.random.seed=seed;
        assert(tera_space_start(&s,&w.world.characters,4));assert(tera_space_key(&s,13));
        key=target_key(&s);seen|=1u<<(key-'1');
        assert(tera_space_key(&s,key) && s.hit && s.counter==2 && s.pirates==3);
        assert(s.state.combat.experience==3 && s.state.energy==100);
    }
    assert(seen==511); /* Les neuf directions ont effectivement ete exercees. */
}
static void test_pilot_and_fuel(void)
{
    TeraExplorationState w=module_fixture();TeraSpaceSession s,before;unsigned i;
    TeraCharacterState *c=&w.world.characters;
    assert(tera_space_travel_start(&s,c,0,4,1200));assert(tera_space_key(&s,'N'));
    assert(s.phase==TERA_SPACE_DONE && !s.departed && s.state.combat.random.seed==seed_after(1986,1));
    for(i=0;i<10;++i) c->combat.encounter.actors[i].active=0;
    assert(tera_space_travel_start(&s,c,0,4,1200));assert(tera_space_key(&s,13));
    assert(s.phase==TERA_SPACE_DONE && s.pilot==-1 && s.state.combat.random.seed==seed_after(1986,11));
    c->combat.encounter.actors[2].active=c->combat.encounter.actors[8].active=1;
    assert(tera_space_travel_start(&s,c,0,4,1200));assert(tera_space_key(&s,'o'));
    assert(s.pilot==8 && s.departed && s.state.combat.on_alfol);
    assert(s.state.combat.random.seed==seed_after(1986,10));
    /* Argent exact autorise ; +1 signe deborde a 32767. Une mauvaise
     * touche du chiffre ne tire rien et ne lance pas le vol. */
    for(i=0;i<3;++i) {
        c->combat.money=i==0 ? 50 : i==1 ? 49 : 32767;
        assert(tera_space_start(&s,c,4));assert(tera_space_key(&s,'O'));
        before=s;assert(tera_space_key(&s,27));assert(memcmp(&s,&before,sizeof s)==0);
        assert(tera_space_key(&s,'1') && s.phase==TERA_SPACE_SHOT);
        assert(s.state.energy==(i==0 ? 150 : 100));
        assert(s.state.combat.money==(i==0 ? 0 : c->combat.money));
        assert(s.state.combat.random.seed==seed_after(1986,163));
    }
}
static void test_shots_days_and_halt(void)
{
    TeraExplorationState w=module_fixture();TeraSpaceSession s,before;unsigned sound;
    for(sound=0;sound<2;++sound) {
        w.world.characters.combat.sound=(int)sound;
        assert(tera_space_start(&s,&w.world.characters,4));assert(tera_space_key(&s,13));
        assert(s.phase==TERA_SPACE_SHOT && s.counter==4);
        assert(s.state.combat.random.seed==seed_after(1986,163+512*sound));
        assert(tera_space_key(&s,target_key(&s)) && s.counter==2 && s.pirates==3);
        assert(s.state.combat.experience==3 && s.state.energy==100);
        assert(tera_space_key(&s,0) && s.counter==1 && s.state.energy==95);
        assert(tera_space_key(&s,target_key(&s)) && s.phase==TERA_SPACE_DONE);
        assert(s.state.days==7307 && s.state.energy==70 && s.state.combat.experience==6);
        /* 163 debut +3 cible suivante +16 echec +3 cible +112 fin. */
        assert(s.state.combat.random.seed==seed_after(1986,297+1152*sound));
        before=s;assert(!tera_space_key(&s,0));assert(memcmp(&s,&before,sizeof s)==0);
        w.world.characters.energy=9;
        assert(tera_space_start(&s,&w.world.characters,4));assert(tera_space_key(&s,13));
        assert(tera_space_key(&s,0) && s.phase==TERA_SPACE_HALTED && s.state.energy==4);
        assert(s.state.days==7300);
        assert(s.state.combat.random.seed==seed_after(1986,243+11152*sound));
        w.world.characters.energy=100;
    }
    w.world.characters.combat.on_alfol=1;w.world.characters.days=32766;w.world.characters.energy=25;
    assert(tera_space_travel_start(&s,&w.world.characters,1,0,1202));assert(s.pirates==4);
    assert(tera_space_key(&s,13));assert(tera_space_key(&s,target_key(&s)));assert(tera_space_key(&s,target_key(&s)));
    assert(s.phase==TERA_SPACE_DONE && s.state.days==-32763 && s.state.energy==5);
    assert(s.state.combat.position==1202 && !s.state.combat.on_alfol);
    assert(tera_space_start(&s,&w.world.characters,0));assert(tera_space_key(&s,13));
    assert(s.phase==TERA_SPACE_SHOT && !s.trials);assert(tera_space_key(&s,target_key(&s)));
    assert(s.phase==TERA_SPACE_DONE && s.trials==1); /* REPEAT, meme zero pirate. */
}
static void test_rays(void)
{
    TeraPresentationSink sink={ray_observer,NULL};unsigned key,sound;
    static const int xs[]={91,155,219,91,155,219,91,155,219};
    static const int ys[]={118,118,118,90,90,90,66,66,66};
    for(sound=0;sound<2;++sound) for(key='1';key<='9';++key) {
        TeraExplorationState e=module_fixture();TeraSpaceSession s,plain;
        e.world.characters.combat.sound=(int)sound;
        assert(tera_space_start(&s,&e.world.characters,4) && tera_space_key(&s,'N'));
        s.alien_x=s.alien_y=0;plain=s;s.state.combat.presentation=&sink;rays=0;
        assert(tera_space_key(&s,key) && tera_space_key(&plain,key));
        assert(rays==1 && last_ray.data.ray.x==xs[key-'1'] && last_ray.data.ray.y==ys[key-'1']);
        assert(last_ray.data.ray.ms==(sound ? 36u : 30u));s.state.combat.presentation=NULL;
        assert(!memcmp(&s,&plain,sizeof plain));
    }
}
int main(void) {test_all_nine_directions();test_pilot_and_fuel();test_shots_days_and_halt();test_rays();return 0;}
