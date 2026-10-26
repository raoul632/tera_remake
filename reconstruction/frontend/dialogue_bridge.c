#include "exploration_transport.h"
#include "character_transport.h"
#include "tera/dialogue.h"
#include <stdio.h>
#include <string.h>

static void show(const TeraDialogueSession *s)
{
    TeraCreationSession packet;
    unsigned i;
    memset(&packet,0,sizeof packet);packet.phase=TERA_CREATE_DONE;
    packet.state=s->phase==TERA_DIALOGUE_TRANSFER ? s->transfer.state : s->state.world.characters;
    tera_frontend_show_creation(&packet);
    printf("T %d %u %u %d %u %d %d %u",(int)s->phase,s->command,s->choice,
        packet.state.combat.recruit_ready,s->available_mask,s->extra_actor,s->message,s->hint_count);
    for(i=0;i<4;++i) printf(" %u",s->hints[i]);
    printf(" %d %d %u %u %u %u %d %d %u %d %u %u %d %u\n",(int)s->recruitment.outcome,(int)s->transfer.phase,
        s->transfer.subject,s->transfer.slot,s->transfer.target,s->transfer.pending_item,
        (int)s->transfer.outcome,s->state.information_context,s->recruitment.rolls,
        s->recruitment.negotiator,s->recruitment.initial_party_count,s->recruitment.joined_mask,
        s->response_id,s->response_variant);
    fflush(stdout);
    if(s->phase==TERA_DIALOGUE_DONE && s->recruitment.rolls==10)
        printf("J 9\n"); /* B757 : dernier indice du scan de negociation. */
}

int main(void)
{
    TeraExplorationState state;
    TeraDialogueSession session;
    unsigned mode,command;
    int key;
    if(!tera_frontend_read_exploration(&state,&mode,&command) || mode ||
       !tera_dialogue_start(&session,&state,command)) return 2;
    show(&session);
    while(session.phase<TERA_DIALOGUE_DONE) {
        if(!tera_frontend_read_int(&key,0,255)) return 4;
        if(!tera_dialogue_key(&session,(unsigned)key)) return 3;
        show(&session);
    }
    return session.phase==TERA_DIALOGUE_ERROR ? 3 : 0;
}
