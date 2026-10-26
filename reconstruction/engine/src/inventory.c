#include "tera/inventory.h"
#include <string.h>

int tera_inventory_pick_actor(const TeraCharacterState *state,unsigned key)
{
    unsigned id,count=0;
    if(!state) return -1;
    if(key=='A') return 0;
    if(key<'B' || key>'D') return -1;
    for(id=1;id<10;++id) if(state->combat.encounter.actors[id].active)
        if(++count==key-'A') return (int)id;
    return -1;
}

void tera_inventory_refresh_limits(TeraCharacterState *state)
{
    unsigned id,shown=0;
    if(!state) return;
    for(id=0;id<10 && shown<4;++id) if(state->combat.encounter.actors[id].active) {
        TeraCombatLimits limits;
        tera_combat_actor_limits(&state->combat.encounter.attributes[id],&limits);
        state->combat.encounter.actors[id].hp_limit=limits.hp;
        state->combat.encounter.actors[id].magic_limit=limits.magic;++shown;
    }
}

TeraItemOutcome tera_inventory_acquire(TeraCharacterState *state,unsigned actor,unsigned *pending)
{
    unsigned slot;
    if(!state || !pending || actor>9 || *pending>255) return TERA_ITEM_NONE;
    /* CAA13 ne teste pas IsInParty : seule la condition HS refuse. */
    if(strcmp(state->combat.encounter.actors[actor].condition,"HS")==0) return TERA_ITEM_HS;
    for(slot=0;slot<10;++slot) if(!state->combat.equipment[actor][slot]) {
        state->combat.equipment[actor][slot]=(uint8_t)*pending;*pending=0;
        return TERA_ITEM_ADDED;
    }
    return TERA_ITEM_FULL;
}

static void finish(TeraInventorySession *s,TeraItemOutcome outcome)
{
    s->outcome=outcome;s->phase=TERA_INV_DONE;
}

int tera_inventory_start(TeraInventorySession *s,const TeraCharacterState *state,unsigned command)
{
    TeraCharacterState copy;
    unsigned value;
    if(!s || !state || (command!='A' && command!='B' && command!='N')) return 0;
    copy=*state;memset(s,0,sizeof *s);s->state=copy;s->command=command;
    if(command=='A' || command=='B') {
        /* C4473 : phrase d'abandon/boisson avant la premiere lecture. */
        tera_random_draw(&s->state.combat.random,3,&value);
        tera_presentation_message(&s->state.combat,TERA_SOURCE_INVENTORY,
            TERA_TEXT_COMMAND_RESPONSE,command,value);
    }
    if(command=='A') s->phase=TERA_INV_SUBMENU;
    else if(command=='N') s->phase=TERA_INV_MENU;
    else {tera_inventory_refresh_limits(&s->state);s->phase=TERA_INV_SUBJECT;}
    return 1;
}

static int inventory_key(TeraInventorySession *s,unsigned key)
{
    int actor;
    TeraRoundActor *a;
    unsigned item;
    if(!s || key>255 || s->phase>=TERA_INV_DONE) return 0;
    if(key>='a' && key<='z') key-=32;
    if(s->phase==TERA_INV_MENU) {
        if(key=='D') s->phase=TERA_INV_CONFIRM;
        else if(key=='I' || key=='A' || key=='V' || key=='S') s->phase=TERA_INV_ERROR;
        else finish(s,TERA_ITEM_CANCELLED); /* Autres dialogues : non portes ici. */
    } else if(s->phase==TERA_INV_CONFIRM) {
        if(key=='O' || key==13) {tera_inventory_refresh_limits(&s->state);s->phase=TERA_INV_SUBJECT;}
        else finish(s,TERA_ITEM_CANCELLED);
    } else if(s->phase==TERA_INV_SUBMENU) {
        if(key!='C' && key!='O') {finish(s,TERA_ITEM_CANCELLED);return 1;}
        s->target=key;tera_inventory_refresh_limits(&s->state);s->phase=TERA_INV_SUBJECT;
    } else if(s->phase==TERA_INV_SUBJECT || s->phase==TERA_INV_TARGET) {
        if(s->command=='B' && key==27) {finish(s,TERA_ITEM_CANCELLED);return 1;}
        if(s->command=='A' && (key<'A' || key>'D' || (s->target=='C' && key=='A'))) {
            finish(s,TERA_ITEM_CANCELLED);return 1;
        }
        actor=tera_inventory_pick_actor(&s->state,key);
        if(actor<0) return 1; /* C4B5C redemande, meme sur ESC. */
        if(s->phase==TERA_INV_TARGET) {
            s->target=(unsigned)actor;
            if(!s->state.combat.encounter.actors[actor].active) {finish(s,TERA_ITEM_INACTIVE);return 1;}
            s->pending_item=s->state.combat.equipment[s->subject][s->slot];
            s->outcome=tera_inventory_acquire(&s->state,s->target,&s->pending_item);
            if(!s->pending_item) {
                /* Copie AVANT retrait, meme si source et cible sont identiques. */
                s->state.combat.equipment[s->subject][s->slot]=0;s->outcome=TERA_ITEM_GIVEN;
            }
            s->phase=TERA_INV_DONE;return 1;
        }
        s->subject=(unsigned)actor;
        if(s->command=='A' && s->target=='C') {
            s->state.combat.encounter.actors[actor].active=0;finish(s,TERA_ITEM_DROPPED);
        } else if(s->command!='A' && !s->state.combat.encounter.actors[actor].active)
            finish(s,TERA_ITEM_INACTIVE);
        else s->phase=TERA_INV_SLOT;
    } else if(s->phase==TERA_INV_SLOT) {
        if(key<'0' || key>'9') return 1; /* C4418 n'admet pas ESC comme annulation. */
        s->slot=key-'0';item=s->state.combat.equipment[s->subject][s->slot];
        a=&s->state.combat.encounter.actors[s->subject];
        if(s->command=='A') {
            if(item) {s->state.combat.equipment[s->subject][s->slot]=0;finish(s,TERA_ITEM_DROPPED);}
            else finish(s,TERA_ITEM_EMPTY);
        } else if(s->command=='N') {
            if(item) s->phase=TERA_INV_TARGET;else finish(s,TERA_ITEM_EMPTY);
        } else if(strcmp(a->condition,"HS")==0) finish(s,TERA_ITEM_HS);
        else if(item==23 || item==24) {
            if(item==23) {
                TeraCombatLimits limits;
                tera_combat_actor_limits(&s->state.combat.encounter.attributes[s->subject],&limits);
                a->hp=a->hp_limit=limits.hp;a->magic=a->magic_limit=limits.magic;
                /* C4CF9 ne guerit pas EM ; boire 24 le fait sans restaurer HP. */
            } else memcpy(a->condition,"OK",3);
            s->state.combat.equipment[s->subject][s->slot]=0;
            tera_inventory_refresh_limits(&s->state);finish(s,TERA_ITEM_CONSUMED);
            tera_presentation_actor(&s->state.combat,TERA_SOURCE_INVENTORY,TERA_EFFECT_HEAL,s->subject);
        } else finish(s,TERA_ITEM_NOT_POTION);
    }
    return 1;
}

int tera_inventory_key(TeraInventorySession *s,unsigned key)
{
    TeraCharacterState before;
    TeraInventoryPhase previous_phase;
    if(!s || key>255 || s->phase>=TERA_INV_DONE) return 0;
    before=s->state;previous_phase=s->phase;
    if(!inventory_key(s,key)) return 0;
    if(s->phase==TERA_INV_DONE) {
        TeraPresentationEvent e=tera_presentation_make(&s->state.combat,TERA_PRESENT_INVENTORY,TERA_SOURCE_INVENTORY);
        e.data.inventory.command=s->command;e.data.inventory.subject=s->subject;
        e.data.inventory.target=s->target;e.data.inventory.slot=s->slot;
        if(previous_phase==TERA_INV_SLOT || previous_phase==TERA_INV_TARGET)
            e.data.inventory.item=before.combat.equipment[s->subject][s->slot];
        e.data.inventory.outcome=(int)s->outcome;tera_presentation_emit(&s->state.combat,&e);
    }
    return 1;
}
