#include "tera/chest.h"
#include "tera/sound.h"
#include <string.h>

static unsigned draw(TeraChestSession *s,unsigned bound)
{
    unsigned value=0;
    tera_random_draw(&s->state.world.characters.combat.random,bound,&value);return value;
}

static int16_t word16(int32_t n)
{
    uint32_t bits=(uint32_t)n&65535u;
    return (int16_t)(bits<32768u ? (int32_t)bits : (int32_t)bits-65536);
}

/* Explosion(TRUE)/C6D32 : garder huit couleurs, plus les seize sons C61D1
 * de chaque image lorsque SoundFlag=1. Ces jets precedent les blessures. */
static void explosion_random(TeraChestSession *s)
{
    unsigned i;
    for(i=0;i<8;++i) {
        draw(s,4);draw(s,16);
        if(s->state.sound) tera_sound_random_tones(&s->state.world.characters.combat,100,2,0,16);
    }
}

static void trap_damage(TeraChestSession *s,unsigned actor,unsigned base)
{
    TeraCombatGame *g=&s->state.world.characters.combat;
    TeraRoundActor *a=&g->encounter.actors[actor];
    a->hp=word16(a->hp-(int32_t)draw(s,base+g->difficulty));
    tera_combat_update_actor_condition(a);
    tera_presentation_actor(g,TERA_SOURCE_CHEST,TERA_EFFECT_TRAP,actor);
}

static void report_trap(TeraChestSession *s)
{
    const TeraCombatGame *g=&s->state.world.characters.combat;
    TeraPresentationEvent e=tera_presentation_make(g,TERA_PRESENT_TRAP,TERA_SOURCE_CHEST);
    e.data.trap.actor=s->actor;e.data.trap.kind=s->trap;tera_presentation_emit(g,&e);
}

static void resolve_trap(TeraChestSession *s)
{
    TeraCombatGame *g=&s->state.world.characters.combat;
    const TeraCombatActor *attributes=&g->encounter.attributes[s->actor];
    unsigned id;
    int16_t power;
    if(draw(s,s->state.navigation_overlay==2 ? 2 : 3)!=0) return;
    power=word16((attributes->stats[7]+attributes->stats[9]+
        (attributes->skills[7] ? attributes->skills[7] : -2))*(int32_t)(1+draw(s,4)));
    if(power>11+(int)g->difficulty) {s->trap=4;report_trap(s);return;}
    s->trap=(int)draw(s,4);
    report_trap(s);
    if(s->trap==0 || s->trap==3) {
        if(s->trap==3 && s->state.navigation_overlay!=2) trap_damage(s,s->actor,2);
        /* SoundFx1(50,1500,10,20) : 150-5+1 iterations. */
        if(s->state.sound) tera_sound_sweep(g,50,1500,10,20);
        if(s->trap==3 && s->state.navigation_overlay==2) trap_damage(s,s->actor,2);
        if(s->trap==0)
            for(id=0;id<10;++id) if(g->encounter.actors[id].active &&
                strcmp(g->encounter.actors[id].condition,"HS")!=0)
            {
                memcpy(g->encounter.actors[id].condition,"EM",3);
                tera_presentation_actor(g,TERA_SOURCE_CHEST,TERA_EFFECT_TRAP,id);
            }
    } else if(s->trap==1) {
        explosion_random(s);
        for(id=0;id<10;++id) if(g->encounter.actors[id].active &&
            strcmp(g->encounter.actors[id].condition,"HS")!=0) trap_damage(s,id,s->state.navigation_overlay==2 ? 3 : 2);
    } else {
        memcpy(g->encounter.actors[s->actor].condition,"EM",3);
        tera_presentation_actor(g,TERA_SOURCE_CHEST,TERA_EFFECT_TRAP,s->actor);
    }
    /* Affichage apres le piege ; aucune normalisation du groupe inventee. */
    tera_inventory_refresh_limits(&s->state.world.characters);
}

int tera_chest_start(TeraChestSession *s,const TeraExplorationState *state,unsigned command)
{
    TeraExplorationState copy;
    unsigned content;
    if(!s || !state || (command!='O' && command!='P') || state->world.characters.combat.world_id>8 ||
       state->world.characters.combat.position<0 || state->world.characters.combat.position>=900) return 0;
    /* BP_03 ne peut etre vrai pour un marqueur >=100 dans un parcours normal.
     * Une sauvegarde manuelle incoherente ferait deborder Keys dans le DOS.
     * Rejeter ce contexte sans ecrire hors de la structure C. */
    if(command=='P' && state->chest_open && state->world.maps[state->world.characters.combat.position][4]>=100) return 0;
    copy=*state;memset(s,0,sizeof *s);s->state=copy;s->command=command;s->trap=-1;
    content=copy.world.maps[copy.world.characters.combat.position][4];
    s->phase=TERA_CHEST_DONE;
    if(command=='O' && content<(copy.navigation_overlay==2 ? 90u : 100u)) {
        if(!copy.chest_open) s->phase=TERA_CHEST_CONFIRM;
    } else if(command=='P' && copy.chest_open && content) s->phase=TERA_CHEST_CONFIRM;
    else tera_presentation_message(&s->state.world.characters.combat,TERA_SOURCE_CHEST,
        TERA_TEXT_COMMAND_RESPONSE,command,draw(s,3));
    return 1;
}

static int chest_key(TeraChestSession *s,unsigned key)
{
    TeraCharacterState *characters;
    TeraCombatGame *g;
    uint8_t *content;
    int actor;
    if(!s || key>255 || s->phase>=TERA_CHEST_DONE) return 0;
    if(key>='a' && key<='z') key-=32;
    characters=&s->state.world.characters;g=&characters->combat;
    content=&s->state.world.maps[g->position][4];
    if(s->phase==TERA_CHEST_CONFIRM) {
        s->phase=TERA_CHEST_DONE;
        if(key!='O' && key!=13) {s->outcome=TERA_ITEM_CANCELLED;return 1;}
        if(s->command=='O') {
            if(!*content) {s->outcome=TERA_ITEM_EMPTY;return 1;}
            tera_inventory_refresh_limits(characters);s->phase=TERA_CHEST_ACTOR;
        } else if(*content>89) {
            if(*content==99) s->state.world.has_map[g->world_id]=1;
            else characters->keys[*content-90]=(int16_t)(*content-90);
            *content=0;s->outcome=TERA_ITEM_ADDED;
        } else {tera_inventory_refresh_limits(characters);s->phase=TERA_CHEST_ACTOR;}
        return 1;
    }
    if(s->command=='O' && key==27) {s->phase=TERA_CHEST_DONE;s->outcome=TERA_ITEM_CANCELLED;return 1;}
    actor=tera_inventory_pick_actor(characters,key);
    if(actor<0) return 1;
    s->actor=(unsigned)actor;s->phase=TERA_CHEST_DONE;
    if(s->command=='P') {
        s->pending_item=*content;
        s->outcome=tera_inventory_acquire(characters,s->actor,&s->pending_item);
        if(!s->pending_item) *content=0;
    } else if(strcmp(g->encounter.actors[actor].condition,"HS")==0) s->outcome=TERA_ITEM_HS;
    else {
        resolve_trap(s);
        if(*content>25 && *content<90) {
            s->reward=*content-20;
            g->money=word16(g->money+(int32_t)s->reward);*content=0;
        } else s->state.chest_open=1;
    }
    return 1;
}

int tera_chest_key(TeraChestSession *s,unsigned key)
{
    unsigned content;
    TeraChestPhase previous_phase;
    if(!s || key>255 || s->phase>=TERA_CHEST_DONE) return 0;
    content=s->state.world.maps[s->state.world.characters.combat.position][4];
    previous_phase=s->phase;
    if(!chest_key(s,key)) return 0;
    if(s->phase==TERA_CHEST_DONE) {
        TeraCombatGame *g=&s->state.world.characters.combat;
        TeraPresentationEvent e;
        e=tera_presentation_make(g,TERA_PRESENT_CHEST,TERA_SOURCE_CHEST);
        e.data.chest.command=s->command;
        e.data.chest.actor=previous_phase==TERA_CHEST_ACTOR && s->outcome!=TERA_ITEM_CANCELLED ? s->actor : ~0u;
        e.data.chest.content=content;
        e.data.chest.outcome=(int)s->outcome;e.data.chest.open=s->state.chest_open;
        e.data.chest.reward=s->reward;
        tera_presentation_emit(g,&e);
    }
    return 1;
}
