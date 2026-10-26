#include "tera/amarande.h"
#include "tera/recruitment.h"
#include "tera/inventory.h"
#include <string.h>
/* Integer Pascal : conserver les debordements des directions signees. */
static int16_t word16(int32_t v)
{
    uint32_t bits=(uint32_t)v&65535u;
    return (int16_t)(bits<32768 ? (int32_t)bits : (int32_t)bits-65536);
}
/* Chaque presentation et action partagent l'unique graine du combat. */
static unsigned draw(TeraAmarandeSession *s,unsigned bound)
{
    unsigned v=0;tera_random_draw(&s->state.world.characters.combat.random,bound,&v);return v;
}
/* C4473 : choisir la ressource originale sans ajouter de tirage au frontend.
 * count distingue deux reponses successives de meme ID et de meme variante. */
static void response(TeraAmarandeSession *s,unsigned id)
{
    s->response_id=id;
    s->response_variant=draw(s,3);
    ++s->response_count;
    tera_presentation_message(&s->state.world.characters.combat,TERA_SOURCE_AMARANDE,
        TERA_TEXT_RESPONSE,id,s->response_variant);
}
/* CAFE8/C : l'ordre ID1..9 est conserve, meme apres fuite ou mort.
 * La seconde rencontre depend de LocExtra et A750 APRES la premiere. */
static void next_npc(TeraAmarandeSession *s)
{
    static const unsigned types[10]={0,10,17,17,12,17,17,12,8,14};
    TeraWorldState *w=&s->state.world;TeraCombatGame *g=&w->characters.combat;
    unsigned type;
    while(s->npc<10) {
        type=0;
        if(s->kind==0) {
            s->kind=1;
            if(tera_recruit_available(w)&(1u<<s->npc)) type=types[s->npc];
        } else {
            s->kind=0;
            if(w->extra_locations[s->npc]==g->position && s->state.information_context) {
                type=(s->npc==3 || s->npc==5 || s->npc==8) ? 17 : s->extra_variant==0 ? 14 : 10;
            }
            ++s->npc;
        }
        if(type) {
            g->sound=s->state.sound;g->encounter.monster_type=type;
            if(!tera_combat_start(&s->combat,g,s->kind ? s->npc : 0,s->observe,s->context)) {s->phase=TERA_AM_ERROR;return;}
            s->outcome=1;s->phase=TERA_AM_COMBAT;return;
        }
    }
    if(!s->outcome) response(s,3);
    s->phase=TERA_AM_DONE;
}
/* Reintegrer le combat termine avant de chercher le prochain PNJ. */
static void finish_combat(TeraAmarandeSession *s)
{
    if(s->combat.phase==TERA_COMBAT_ERROR) {s->phase=TERA_AM_ERROR;return;}
    if(s->combat.phase!=TERA_COMBAT_DONE) return;
    s->state.world.characters.combat=s->combat.game;
    if(s->command=='C') {
        if(s->kind && s->combat.game.encounter.victory)
            memcpy(s->state.world.characters.combat.encounter.actors[s->npc].condition,"HS",3);
        if(!s->kind) s->state.information_context=0;
        next_npc(s);
        /* Un groupe mort peut terminer immediatement une autre rencontre. */
        if(s->phase==TERA_AM_COMBAT && s->combat.phase==TERA_COMBAT_DONE) finish_combat(s);
    } else s->phase=TERA_AM_DONE;
}
/* H/S : une fiche et une vraie lecture 4210 par compagnon actif. */
static void show_character(TeraAmarandeSession *s,unsigned actor)
{
    TeraCombatGame *g=&s->state.world.characters.combat;
    TeraPresentationEvent e=tera_presentation_make(g,TERA_PRESENT_CHARACTER,TERA_SOURCE_AMARANDE);
    tera_character_card(&s->state.world.characters,NULL,actor,&e.data.character);
    tera_presentation_emit(g,&e);
}
static void inspect_next(TeraAmarandeSession *s)
{
    while(++s->shown_actor<10) if(s->state.world.characters.combat.encounter.actors[s->shown_actor].active) {
        tera_inventory_refresh_limits(&s->state.world.characters);s->status_actor=(int16_t)s->shown_actor;
        show_character(s,s->shown_actor);s->phase=TERA_AM_PAUSE;return;
    }
    s->status_actor=9;s->phase=TERA_AM_DONE;
}
int tera_amarande_start(TeraAmarandeSession *s,const TeraExplorationState *state,unsigned command,
    int16_t status_actor,TeraCombatObserver observer,void *context)
{
    TeraExplorationState copy;
    if(!s || !state || command>255) return 0;
    copy=*state;memset(s,0,sizeof *s);s->state=copy;s->command=command;s->status_actor=status_actor;
    s->observe=observer;s->context=context;s->phase=TERA_AM_DONE;
    if(command=='?' || command=='+') s->phase=TERA_AM_PAUSE;
    else if(command=='C' || command=='#' || command=='U') {s->phase=TERA_AM_CONFIRM;s->kind=command;}
    else if(command=='L' && state->town_found && state->town_id>9) {s->phase=TERA_AM_CONFIRM;s->kind='L';}
    else if(command=='I') s->phase=TERA_AM_WORD;
    else if(command=='H') {response(s,7);s->phase=TERA_AM_HALT;}
    else if(command=='D' || command=='E' || command=='F' || command=='L' || command=='M' || command=='O' || command=='P') {
        /* Les verbes L/M/O/P occupent les entrees 8/9/11/12, pas leur
         * rang alphabetique ; D/E/F utilisent directement 4/5/6. */
        response(s,command>='L' ? command-'D' : command-'@');s->outcome=2;
    }
    return 1;
}
static int amarande_key(TeraAmarandeSession *s,unsigned key)
{
    TeraCombatGame *g;unsigned i,calls=0;int16_t roll,delta;
    if(!s || key>255 || s->phase>=TERA_AM_DONE) return 0;
    if(key>='a' && key<='z') key-=32;
    g=&s->state.world.characters.combat;
    if(s->phase==TERA_AM_COMBAT) {
        if(!tera_combat_key(&s->combat,key)) return 0;
        finish_combat(s);return 1;
    }
    if(s->phase==TERA_AM_DAY) {
        if(!tera_day_key(&s->day,key)) return 0;
        if(s->day.phase==TERA_DAY_DONE) {s->state=s->day.state;response(s,s->kind=='D' ? 16 : 17);s->phase=TERA_AM_DONE;}
        else if(s->day.phase==TERA_DAY_ERROR) s->phase=TERA_AM_ERROR;
        return 1;
    }
    if(s->phase==TERA_AM_WORD) {
        if(key==8 && s->word_length) s->word[--s->word_length]=0;
        else if(key>='A' && key<='Z') {if(s->word_length<9) s->word[s->word_length]=(char)key;++s->word_length;}
        if(key!=13 && s->word_length<10) return 1;
        s->word[9]=0;s->status_actor=9;s->phase=TERA_AM_DONE;
        for(i=0;i<10;++i) if(s->state.world.towns[i]==g->position) return 1;
        if(!g->on_alfol && strcmp(s->word,"ARIOCH")==0) {
            s->state.encounter_suppressed=1;s->state.information_context=1;
            g->sound=s->state.sound;g->encounter.monster_type=21;
            tera_monster_presentation_random(g,21,&calls);
            s->status_actor=120; /* C795E : dernier point de la presentation. */
            if(!tera_combat_start(&s->combat,g,0,s->observe,s->context)) {s->phase=TERA_AM_ERROR;return 1;}
            s->phase=TERA_AM_COMBAT;finish_combat(s);
        }
        return 1;
    }
    if(s->phase==TERA_AM_PAUSE) {
        if(s->command=='H' && s->choice=='S') inspect_next(s);
        else if(s->command=='U') s->phase=TERA_AM_HALT;
        else s->phase=TERA_AM_DONE;
        return 1;
    }
    if(s->phase==TERA_AM_HALT) {
        /* U confirme lit d'abord 4210, puis une seconde touche via 4277. */
        if(s->command=='U') {s->phase=TERA_AM_DONE;return 1;}
        s->choice=key;s->phase=TERA_AM_DONE;
        if(key=='A' || key=='D' || key=='M' || key=='S' || key==27) {s->kind=key;s->phase=TERA_AM_CONFIRM;}
        else if(key=='C') {
            /* WorldId=11 lit le flag adjacent 9417, initialise a vrai. */
            if(g->world_id==11 ? s->state.world.initial_flag : s->state.world.has_map[g->world_id]) {
                s->outcome=3;s->status_actor=9;
                if(g->world_id!=11 && g->difficulty<5) s->phase=TERA_AM_PAUSE;
            }else {response(s,20);s->outcome=4;}
        } else if(key=='H') {
            TeraPresentationEvent e=tera_presentation_make(g,TERA_PRESENT_SCENERY,TERA_SOURCE_AMARANDE);
            e.data.scenery.kind=TERA_PICTURE_HALT;
            for(i=0;i<9;++i) {
                e.data.scenery.values[e.data.scenery.count++]=(unsigned short)draw(s,2);
                e.data.scenery.values[e.data.scenery.count++]=(unsigned short)draw(s,60);
                e.data.scenery.values[e.data.scenery.count++]=(unsigned short)draw(s,3);
            }
            tera_presentation_emit(g,&e);
            s->outcome=5;s->status_actor=8;
        }
        else if(key=='I') {s->outcome=6;tera_inventory_refresh_limits(&s->state.world.characters);show_character(s,0);}
        return 1;
    }
    s->phase=TERA_AM_DONE;
    if(s->kind=='A') {s->state.world.fast_moves=key=='O' || key==13;return 1;}
    if(key!='O' && key!=13) return 1;
    if(s->kind=='#') s->state.sound=!s->state.sound;
    else if(s->kind=='U') {response(s,15);s->phase=TERA_AM_PAUSE;s->outcome=2;}
    else if(s->kind=='L') {
        delta=word16((int32_t)s->state.world.towns[0]-g->position);
        s->east=(int16_t)(delta/30);s->north=(int16_t)(delta%30);s->status_actor=s->north;
        s->phase=TERA_AM_PAUSE;s->outcome=7;
    } else if(s->kind=='C') {s->kind=0;s->npc=1;next_npc(s);if(s->phase==TERA_AM_COMBAT) finish_combat(s);}
    else if(s->kind=='D' || s->kind=='M') {
        s->state.force_day=1;
        if(!tera_day_start(&s->day,&s->state,s->observe,s->context)) {s->phase=TERA_AM_ERROR;return 1;}
        s->phase=TERA_AM_DAY;
        if(s->day.phase==TERA_DAY_DONE) {s->state=s->day.state;response(s,s->kind=='D' ? 16 : 17);s->phase=TERA_AM_DONE;}
        else if(s->day.phase==TERA_DAY_ERROR) s->phase=TERA_AM_ERROR;
    } else if(s->kind=='S') {
        tera_combat_base_roll(&g->encounter.attributes[0],5,5,draw(s,15)+3,&roll);
        if(roll>0) {
            if(strcmp(g->encounter.actors[0].condition,"HS")!=0) {s->outcome=8;s->shown_actor=0;inspect_next(s);}
            else s->outcome=9;
        } else {response(s,19);s->outcome=10;}
    } else if(s->kind==27) s->quit_requested=1;
    return 1;
}

int tera_amarande_key(TeraAmarandeSession *s,unsigned key)
{
    unsigned previous_outcome;
    if(!s || key>255 || s->phase>=TERA_AM_DONE) return 0;
    previous_outcome=s->outcome;
    if(!amarande_key(s,key)) return 0;
    if(s->outcome && s->outcome!=previous_outcome) {
        TeraPresentationEvent e=tera_presentation_make(&s->state.world.characters.combat,TERA_PRESENT_RESIDENT,TERA_SOURCE_AMARANDE);
        e.data.resident.command=s->command;e.data.resident.choice=s->choice;
        e.data.resident.actor=s->shown_actor;e.data.resident.outcome=(int)s->outcome;
        e.data.resident.east=s->east;e.data.resident.north=s->north;
        tera_presentation_emit(&s->state.world.characters.combat,&e);
    }
    return 1;
}
