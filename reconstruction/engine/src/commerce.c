#include "tera/commerce.h"
#include "tera/recruitment.h"
#include "tera/sound.h"
#include <string.h>

static int16_t word16(int32_t value)
{
    uint32_t bits=(uint32_t)value&65535u;
    return (int16_t)(bits<32768 ? (int32_t)bits : (int32_t)bits-65536);
}

int tera_shop_catalog(unsigned key,unsigned *item,int *price)
{
    static const unsigned items[19]={17,18,21,26,27,28,13,14,16,23,24,29,19,20,1,4,7,10,22};
    static const int prices[19]={100,500,1000,50,100,100,25,50,75,90,40,25,90,300,200,250,300,350,750};
    if(key<'A' || key>'S' || !item || !price) return 0;
    *item=items[key-'A'];*price=prices[key-'A'];return 1;
}

static void picture_draw(TeraRandom *rng,unsigned bound,unsigned *value,TeraScenePicture *p)
{tera_random_draw(rng,bound,value);p->values[p->count++]=(unsigned short)*value;}
int tera_places_present(TeraExplorationState *s,int force)
{
    TeraRandom *rng;unsigned i,value,id=0;TeraPresentationEvent picture;
    if(!s || (force!=0 && force!=1)) return 0;
    if(s->world.characters.combat.world_id!=11 || s->world.characters.combat.on_alfol ||
       !s->town_found || (!force && !s->moved) || s->town_id>9 || (force && s->town_id!=4 && s->town_id!=1 && s->town_id!=2)) return 1;
    rng=&s->world.characters.combat.random;
    picture=tera_presentation_make(&s->world.characters.combat,TERA_PRESENT_SCENERY,TERA_SOURCE_SHOP);
    picture.data.scenery.kind=TERA_PICTURE_PLACE;picture.data.scenery.variant=s->town_id;
#define D(bound) picture_draw(rng,bound,&value,&picture.data.scenery)
    if(s->town_id==4) {
        /* OVL00F0 : quatre sprites puis cinq, deux RANDOM par sprite. */
        for(i=0;i<9;++i) {D(76);D(10);}
        if(s->sound) tera_sound_wave(&s->world.characters.combat,30,1400,6);
        s->moved=0;
    } else if(s->town_id==0) {
        if(s->sound) {
            tera_sound_country(&s->world.characters.combat,1,1);
            tera_sound_country(&s->world.characters.combat,4,3);
        }
        s->moved=0;
    } else {
        /* TERAMAIN : les autres lieux sont des facades suivies de CAFE8,
         * sans menu d'entree ajoute. Les sprites/musiques gardent leur RNG. */
        if(s->town_id==1) {
            for(i=0;i<16;++i) {D(300);D(70);}
            for(i=0;i<9;++i) {D(2);D(60);D(3);}
            for(i=0;i<12;++i) {D(3);D(2);}
            for(i=0;i<3;++i) {D(4);D(2);}
            if(tera_recruit_available(&s->world)&(1u<<4)) s->information_context=1;
        } else if(s->town_id==2) {
            unsigned bound;
            D(18);
            for(i=0;i<3;++i) {
                D(38);D(20);D(16);D(3);
            }
            D(18);bound=2+2*value;
            for(i=0;i<3;++i) {
                D(bound);D(16);bound=2+2*value;D(3);
            }
        } else if(s->town_id==3) {
            for(i=0;i<26;++i) {D(76);D(8);}
            for(i=0;i<12;++i) {D(300);D(48);}
        } else if(s->town_id==5) {
            for(i=0;i<9;++i) {D(76);D(10);}
        }
        if(s->town_id==5 || s->town_id==9) id=3;
        else if(s->town_id==7) id=5;
        else if(s->town_id==6 || s->town_id==8) id=8;
        if(id && (s->world.locations[id]!=s->world.towns[s->town_id] ||
            (!s->world.characters.combat.encounter.actors[id].active &&
             strcmp(s->world.characters.combat.encounter.actors[id].condition,"HS")!=0)))
            s->information_context=1;
        if(s->sound) {
            TeraCombatGame *g=&s->world.characters.combat;
            if(s->town_id==1) {tera_sound_hyperspace(g,0);tera_sound_hyperspace(g,1);}
            else if(s->town_id==3) tera_sound_wave(g,30,1400,6);
            else if(s->town_id==6) {
                tera_sound_melody(g,1,1,9,8);tera_sound_melody(g,0,1,9,8);tera_sound_melody(g,0,2,9,8);
            } else if(s->town_id==8) tera_sound_noise(g,130,100,80,6,14,1,5000);
            else if(s->town_id==9) {tera_sound_melody(g,1,1,1,1);tera_sound_melody(g,1,1,9,8);}
        }
        s->moved=0;
    }
    tera_presentation_emit(&s->world.characters.combat,&picture);
#undef D
    return 1;
}

/* C004 n'assigne rien hors 1..25 : garder le prix precedent, meme pour
 * une sauvegarde contenant un objet hors catalogue. La liste de vente
 * affiche 1..24 et laisse donc initialement _price a 20. */
static int sale_base(unsigned item,int previous)
{
    static const int prices[25]={100,200,300,125,250,375,150,300,450,175,350,500,15,25,500,40,50,250,45,150,500,375,45,20,10};
    return item>0 && item<=25 ? prices[item-1] : previous;
}

int tera_shop_sale_quote(unsigned item,int *price)
{
    if(!price || !item || item>24) return 0;
    *price=sale_base(item,0);return 1;
}

static int negotiation(TeraShopSession *s)
{
    unsigned roll=0;int16_t value=0;
    TeraCombatGame *g=&s->state.world.characters.combat;
    tera_random_draw(&g->random,15,&roll);
    tera_combat_base_roll(&g->encounter.attributes[s->actor],8,8,roll+3,&value);
    return value;
}

static void hospital(TeraShopSession *s)
{
    unsigned id;int16_t missing,max_missing=0;
    TeraCharacterState *c=&s->state.world.characters;
    for(id=0;id<10;++id) if(c->combat.encounter.actors[id].active) {
        TeraRoundActor *a=&c->combat.encounter.actors[id];
        missing=word16((int32_t)a->hp_limit-a->hp);
        if(missing>max_missing) max_missing=missing;
        memcpy(a->condition,"OK",3);
    }
    s->weeks=word16(max_missing+1);c->days=word16(c->days+7*(int32_t)s->weeks);
    /* C4CF9 restaure les DIX personnages, sans guerir l'etat des inactifs. */
    for(id=0;id<10;++id) {
        TeraCombatLimits limits;TeraRoundActor *a=&c->combat.encounter.actors[id];
        tera_combat_actor_limits(&c->combat.encounter.attributes[id],&limits);
        a->hp=a->hp_limit=limits.hp;a->magic=a->magic_limit=limits.magic;
        tera_presentation_actor(&c->combat,TERA_SOURCE_SHOP,TERA_EFFECT_HEAL,id);
    }
    s->outcome=3;
}

static void buy(TeraShopSession *s,unsigned key)
{
    int base,denominator;unsigned pending;
    TeraCharacterState *c=&s->state.world.characters;
    TeraCombatGame *g=&c->combat;
    if(!tera_shop_catalog(key,&s->item,&base)) return;
    denominator=word16(30+negotiation(s)/5);
    if(!denominator) {s->phase=TERA_SHOP_ERROR;return;} /* IDIV DOS invalide. */
    s->price=word16(word16(base*30)/denominator);
    if(s->price>=word16(g->money+1)) {s->outcome=4;return;}
    if(s->item<26) {
        TeraItemOutcome result;pending=s->item;
        result=tera_inventory_acquire(c,s->actor,&pending);
        s->outcome=result==TERA_ITEM_ADDED ? 1 : result==TERA_ITEM_FULL ? 5 : 6;
        if(!pending) g->money=word16(g->money-s->price);
    } else {
        if(s->item==26) c->energy=word16(c->energy+50);
        else if(s->item==27) s->state.world.has_map[10]=1;
        else if(s->item==28) s->state.world.has_map[9]=1;
        else c->keys[9]=9;
        g->money=word16(g->money-s->price);s->outcome=1;
    }
}

int tera_shop_start(TeraShopSession *s,const TeraExplorationState *state)
{
    TeraExplorationState copy;
    if(!s || !state) return 0;
    copy=*state;memset(s,0,sizeof *s);s->state=copy;s->phase=TERA_SHOP_MENU;s->first_menu=1;return 1;
}

static int shop_key(TeraShopSession *s,unsigned key)
{
    int actor,base;
    TeraCharacterState *c;
    if(!s || key>255 || s->phase>=TERA_SHOP_DONE) return 0;
    if(key>='a' && key<='z') key-=32;
    c=&s->state.world.characters;s->outcome=0;
    if(s->phase==TERA_SHOP_MENU) {
        /* Le premier ESC entre quand meme dans REPEAT, qui relit une touche. */
        if(key==27 && !s->first_menu) s->phase=TERA_SHOP_DONE;
        else if(key=='H' || key=='A' || key=='V') {s->command=key;s->phase=TERA_SHOP_CONFIRM;}
        s->first_menu=0;
    } else if(s->phase==TERA_SHOP_CONFIRM) {
        s->phase=TERA_SHOP_MENU;
        if(s->command=='E') {
            if(key!='O' && key!=13) s->phase=TERA_SHOP_DONE;
            return 1;
        }
        if(key=='O' || key==13) {
            if(s->command=='H') hospital(s);
            else {tera_inventory_refresh_limits(c);s->phase=TERA_SHOP_ACTOR;}
        }
    } else if(s->phase==TERA_SHOP_ACTOR) {
        actor=tera_inventory_pick_actor(c,key);if(actor<0) return 1;
        s->actor=(unsigned)actor;
        if(!c->combat.encounter.actors[actor].active) s->phase=TERA_SHOP_MENU;
        else if(s->command=='A') s->phase=TERA_SHOP_BUY;
        else {s->phase=TERA_SHOP_SELL;s->price=20;}
    } else if(key==27) s->phase=TERA_SHOP_MENU;
    else if(s->phase==TERA_SHOP_BUY) buy(s,key);
    else if(key>='0' && key<='9' && c->combat.equipment[s->actor][key-'0']) {
        s->item=c->combat.equipment[s->actor][key-'0'];
        base=sale_base(s->item,s->price);
        s->price=word16(base+(base/100)*negotiation(s));
        c->combat.equipment[s->actor][key-'0']=0;
        c->combat.money=word16(c->combat.money+s->price);s->outcome=2;
    }
    return 1;
}

int tera_shop_key(TeraShopSession *s,unsigned key)
{
    if(!shop_key(s,key)) return 0;
    if(s->outcome) {
        TeraPresentationEvent e=tera_presentation_make(&s->state.world.characters.combat,TERA_PRESENT_SHOP,TERA_SOURCE_SHOP);
        e.data.shop.outcome=s->outcome;
        if(s->outcome==3) e.data.shop.weeks=s->weeks;
        else {e.data.shop.actor=s->actor;e.data.shop.item=s->item;e.data.shop.price=s->price;}
        tera_presentation_emit(&s->state.world.characters.combat,&e);
    }
    return 1;
}
