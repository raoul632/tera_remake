#include "exploration_transport.h"
#include "character_transport.h"
#include "tera/ending.h"
#include <stdio.h>
#include <string.h>
static void show(const TeraEndingSession *s)
{
    TeraCreationSession packet;unsigned i;
    memset(&packet,0,sizeof packet);packet.phase=TERA_CREATE_DONE;packet.state=s->state.world.characters;
    tera_frontend_show_creation(&packet);
    printf("K %u %u %u",s->complete,s->victory,s->runtime_error);
    for(i=0;i<6;++i) printf(" %u",s->score_real48[i]);
    printf("\nR %.17g %s\n",s->score,s->score_text[0] ? s->score_text : "none");fflush(stdout);
}
int main(void)
{
    TeraExplorationState state;TeraEndingSession session;unsigned mode,key,i;int v;
    if(!tera_frontend_read_exploration(&state,&mode,&key) || mode) return 2;
    for(i=0;i<4;++i) {
        if(!tera_frontend_read_int(&v,0,1)) return 2;
        state.world.tasks_done[i]=(uint8_t)v;
    }
    if(!tera_ending_start(&session,&state)) return 3;
    show(&session);
    if(!session.complete) {
        if(!tera_frontend_read_int(&v,0,255)) return 4;
        if(!tera_ending_key(&session,(unsigned)v)) return 3;
        show(&session);
    }
    return 0;
}
