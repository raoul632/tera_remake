#include "tera/sound.h"
#include "tera/lifecycle.h"
#include <string.h>

static int objectives_complete(const TeraWorldState *world)
{
    return world->tasks_done[0] && world->tasks_done[1] && world->tasks_done[2] && world->tasks_done[3];
}

int tera_life_start(TeraLifeSession *s,const TeraExplorationState *state)
{
    TeraExplorationState copy;
    if(!s || !state || state->world.characters.combat.encounter.party_available) return 0;
    copy=*state;memset(s,0,sizeof *s);s->state=copy;
    s->phase=TERA_LIFE_ACK;
    {
        TeraPresentationEvent e=tera_presentation_make(&s->state.world.characters.combat,TERA_PRESENT_ENDING,TERA_SOURCE_LIFE);
        tera_presentation_emit(&s->state.world.characters.combat,&e);
    }
    if(copy.sound) tera_sound_ending(&s->state.world.characters.combat,0);
    return 1;
}

int tera_life_key(TeraLifeSession *s,unsigned key)
{
    if(!s || key>255 || s->phase>=TERA_LIFE_READY) return 0;
    if(s->phase==TERA_LIFE_ACK) {
        if(objectives_complete(&s->state.world)) {s->phase=TERA_LIFE_COMPLETE;return 1;}
        if(!tera_creation_start(&s->creation,&s->state.world.characters)) {s->phase=TERA_LIFE_ERROR;return 1;}
        s->phase=TERA_LIFE_CREATE;
    } else {
        if(!tera_creation_key(&s->creation,key)) return 0;
        if(s->creation.phase==TERA_CREATE_DONE) {
            tera_world_apply_creation(&s->state.world,&s->creation);
            /* Affectation de TERAMAIN APRES OVL0022, pas un effacement
             * des cartes/H/mots/cles/PNJ/flags de la partie. */
            s->state.world.characters.combat.on_alfol=0;
            s->phase=TERA_LIFE_READY;
        }
    }
    return 1;
}
