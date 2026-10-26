#include "module_test_support.h"
#include "tera/commerce.h"

/* Negociation nulle => prix de base connu, tout en conservant le jet RNG. */
static TeraExplorationState shop_fixture(void)
{
    TeraExplorationState s=module_fixture();
    TeraCombatActor *a=&s.world.characters.combat.encounter.attributes[0];
    a->stats[8]=a->skills[8]=a->skills[7]=0;return s;
}
static void shop_keys(TeraShopSession *s,const char *keys)
{
    while(*keys) assert(tera_shop_key(s,(unsigned char)*keys++));
}
static void test_catalog_and_purchase_guards(void)
{
    const unsigned items[19]={17,18,21,26,27,28,13,14,16,23,24,29,19,20,1,4,7,10,22};
    const int prices[19]={100,500,1000,50,100,100,25,50,75,90,40,25,90,300,200,250,300,350,750};
    TeraExplorationState state=shop_fixture();TeraShopSession s,before;unsigned i,item=99;int price=-1;
    for(i=0;i<19;++i) {assert(tera_shop_catalog('A'+i,&item,&price));assert(item==items[i] && price==prices[i]);}
    assert(!tera_shop_catalog('T',&item,&price));assert(!tera_shop_catalog('A',NULL,&price));
    for(i=0;i<3;++i) {
        state.world.characters.combat.money=i==0 ? 99 : i==1 ? 100 : 32767;
        assert(tera_shop_start(&s,&state));shop_keys(&s,"aoaa");
        assert(s.phase==TERA_SHOP_BUY && s.price==100);
        assert(s.outcome==(i==1 ? 1 : 4));
        assert(s.state.world.characters.combat.equipment[0][0]==(i==1 ? 17 : 0));
        assert(s.state.world.characters.combat.money==(i==1 ? 0 : state.world.characters.combat.money));
        assert(s.state.world.characters.combat.random.seed==seed_after(1986,1));
    }
    before=s;assert(!tera_shop_key(&s,256));assert(memcmp(&s,&before,sizeof s)==0);
    assert(tera_shop_start(&s,&state));assert(tera_shop_key(&s,27));assert(s.phase==TERA_SHOP_MENU);
    assert(tera_shop_key(&s,27) && s.phase==TERA_SHOP_DONE);
    before=s;assert(!tera_shop_key(&s,'A'));assert(memcmp(&s,&before,sizeof s)==0);
}
static void test_full_hs_services_and_sales(void)
{
    TeraExplorationState state=shop_fixture();TeraShopSession s;unsigned i;
    for(i=0;i<10;++i) state.world.characters.combat.equipment[0][i]=13;
    assert(tera_shop_start(&s,&state));shop_keys(&s,"aoaa");
    assert(s.outcome==5 && s.state.world.characters.combat.money==2000);
    memset(state.world.characters.combat.equipment,0,sizeof state.world.characters.combat.equipment);
    strcpy(state.world.characters.combat.encounter.actors[0].condition,"HS");
    assert(tera_shop_start(&s,&state));shop_keys(&s,"aoaa");assert(s.outcome==6);
    assert(s.state.world.characters.combat.money==2000);
    /* Services sans slot : meme HS, energie/cartes/cle sont achetables. */
    assert(tera_shop_start(&s,&state));shop_keys(&s,"aoadefl");
    assert(s.state.world.characters.energy==150 && s.state.world.has_map[9] && s.state.world.has_map[10]);
    assert(s.state.world.characters.keys[9]==9 && s.state.world.characters.combat.money==1725);
    assert(s.state.world.characters.combat.random.seed==seed_after(1986,4));
    state=shop_fixture();state.world.characters.combat.equipment[0][0]=25;
    state.world.characters.combat.equipment[0][1]=255;
    state.world.characters.combat.money=32765;
    assert(tera_shop_start(&s,&state));shop_keys(&s,"voa9");
    assert(s.state.world.characters.combat.random.seed==1986); /* Slot vide. */
    shop_keys(&s,"01");
    assert(s.state.world.characters.combat.money==-32751); /* +10 puis prix precedent 10. */
    assert(!s.state.world.characters.combat.equipment[0][0] && !s.state.world.characters.combat.equipment[0][1]);
    assert(s.state.world.characters.combat.random.seed==seed_after(1986,2));
}
static void test_hospital(void)
{
    TeraExplorationState state=shop_fixture();TeraShopSession s;unsigned i;
    TeraRoundActor *a=state.world.characters.combat.encounter.actors;
    a[0].hp_limit=20;a[0].hp=15;strcpy(a[0].condition,"EM");
    a[3].active=1;a[3].hp_limit=30;a[3].hp=20;strcpy(a[3].condition,"HS");
    a[4].hp=0;strcpy(a[4].condition,"HS");state.world.characters.days=32760;
    assert(tera_shop_start(&s,&state));shop_keys(&s,"ho");
    assert(s.weeks==11 && s.state.world.characters.days==-32699);
    assert(s.state.world.characters.combat.money==2000 && s.state.world.characters.combat.random.seed==1986);
    for(i=0;i<10;++i) {
        const TeraRoundActor *b=&s.state.world.characters.combat.encounter.actors[i];
        assert(b->hp==b->hp_limit && b->magic==b->magic_limit);
    }
    assert(strcmp(s.state.world.characters.combat.encounter.actors[0].condition,"OK")==0);
    assert(strcmp(s.state.world.characters.combat.encounter.actors[3].condition,"OK")==0);
    assert(strcmp(s.state.world.characters.combat.encounter.actors[4].condition,"HS")==0);
}
int main(void)
{
    int quote=999;
    assert(tera_shop_sale_quote(1,&quote) && quote==100);
    assert(tera_shop_sale_quote(24,&quote) && quote==20);
    assert(!tera_shop_sale_quote(25,&quote) && quote==20);
    assert(!tera_shop_sale_quote(0,&quote) && !tera_shop_sale_quote(1,NULL));
    test_catalog_and_purchase_guards();test_full_hs_services_and_sales();test_hospital();return 0;
}
