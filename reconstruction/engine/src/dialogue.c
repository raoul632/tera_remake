#include "tera/dialogue.h"
#include "tera/sound.h"
#include <string.h>

static unsigned draw(TeraDialogueSession *s,unsigned bound)
{
    unsigned value=0;tera_random_draw(&s->state.world.characters.combat.random,bound,&value);return value;
}

/* B047..B19F : le dernier personnage supplementaire correspondant gagne.
 * A750 est un drapeau du decor C6073, distinct des facultes mentales 9418.
 * Les indices et les phrases ont des tirages qui influencent le combat. */
static void information(TeraDialogueSession *s)
{
    unsigned id,first;
    TeraWorldState *w=&s->state.world;
    s->extra_actor=-1;s->message=1;
    if(s->state.information_context)
        for(id=1;id<10;++id) if(w->extra_locations[id]==w->characters.combat.position) s->extra_actor=(int)id;
    if(s->extra_actor>=0) {
        s->hints[s->hint_count++]=2*(unsigned)s->extra_actor-1;
        s->hints[s->hint_count++]=2*(unsigned)s->extra_actor;s->message=0;
    }
    s->available_mask=tera_recruit_available(w);
    w->characters.combat.recruit_ready=s->available_mask!=0;
    if(s->available_mask) {
        if(draw(s,2)==0) {
            first=2*draw(s,9)+1;
            s->hints[s->hint_count++]=first;s->hints[s->hint_count++]=first+1;
        } else s->message=2;
    }
}

int tera_dialogue_start(TeraDialogueSession *s,const TeraExplorationState *state,unsigned command)
{
    TeraExplorationState copy;
    if(!s || !state || (command!='N' && command!='S')) return 0;
    copy=*state;memset(s,0,sizeof *s);s->state=copy;s->command=command;s->extra_actor=-1;s->response_id=-1;
    s->phase=command=='N' ? TERA_DIALOGUE_MENU : TERA_DIALOGUE_CONFIRM;
    return 1;
}

static int dialogue_key(TeraDialogueSession *s,unsigned key)
{
    TeraWorldState *w;
    if(!s || key>255 || s->phase>=TERA_DIALOGUE_DONE) return 0;
    if(key>='a' && key<='z') key-=32;
    w=&s->state.world;
    if(s->phase==TERA_DIALOGUE_MENU) {
        s->choice=key;
        if(key=='I' || key=='A' || key=='V' || key=='D' || key=='S') s->phase=TERA_DIALOGUE_CONFIRM;
        else s->phase=TERA_DIALOGUE_DONE;
    } else if(s->phase==TERA_DIALOGUE_TRANSFER) {
        if(!tera_inventory_key(&s->transfer,key)) return 0;
        if(s->transfer.phase==TERA_INV_DONE) {w->characters=s->transfer.state;s->phase=TERA_DIALOGUE_DONE;}
    } else {
        s->phase=TERA_DIALOGUE_DONE;
        if(key!='O' && key!=13) return 1;
        if(s->command=='S' || s->choice=='S') {
            TeraRecruitRequest request=s->command=='S' ? TERA_RECRUIT_SEDUCE : TERA_RECRUIT_SOLICIT;
            tera_recruit_start(&s->recruitment,w,request,w->characters.combat.recruit_ready);
            tera_recruit_key(&s->recruitment,key);*w=s->recruitment.world;
            /* B866..B881 : le son du recrutement negocie partage RANDOM
             * avec le jeu. Le module resident doit conserver ce calendrier. */
            if(s->recruitment.outcome==TERA_RECRUIT_NEGOTIATED && s->state.sound)
                tera_sound_melody(&w->characters.combat,0,1,9,8);
            s->available_mask=s->recruitment.available_mask;
        } else if(s->choice=='I') information(s);
        else if(s->choice=='A' || s->choice=='V') {
            s->response_id=s->choice=='A' ? 22 : 23;s->response_variant=draw(s,3);
        }
        else if(s->choice=='D') {
            /* Deleguer au transfert deja compare a CAD08, sans lire une
             * seconde confirmation : elle est celle qui vient d'arriver. */
            tera_inventory_start(&s->transfer,&w->characters,'N');
            tera_inventory_key(&s->transfer,'D');tera_inventory_key(&s->transfer,key);
            s->phase=TERA_DIALOGUE_TRANSFER;
        }
    }
    return 1;
}

int tera_dialogue_key(TeraDialogueSession *s,unsigned key)
{
    unsigned i;
    if(!dialogue_key(s,key)) return 0;
    if(s->phase==TERA_DIALOGUE_DONE) {
        const TeraCombatGame *g=&s->state.world.characters.combat;
        for(i=0;i<s->hint_count;++i)
            tera_presentation_message(g,TERA_SOURCE_DIALOGUE,TERA_TEXT_HINT,s->hints[i],TERA_TEXT_NO_VARIANT);
        if(s->response_id>=0)
            tera_presentation_message(g,TERA_SOURCE_DIALOGUE,TERA_TEXT_MESSAGE,(unsigned)s->response_id,s->response_variant);
        if(s->message)
            tera_presentation_message(g,TERA_SOURCE_DIALOGUE,s->message==1 ? TERA_TEXT_ELLIPSIS : TERA_TEXT_MESSAGE,s->message==1 ? 0 : 46,TERA_TEXT_NO_VARIANT);
    }
    return 1;
}
