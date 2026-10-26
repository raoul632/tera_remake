#include "exploration_transport.h"
#include "character_transport.h"
#include "tera/space.h"
#include <stdio.h>
#include <string.h>
static void show(const TeraSpaceSession *s)
{
    TeraCreationSession packet;
    memset(&packet,0,sizeof packet);packet.state=s->state;packet.phase=TERA_CREATE_DONE;
    tera_frontend_show_creation(&packet);
    printf("X %d %d %d %d %d %d %d %d %d %u %u %d\n",s->phase,s->pirates,s->counter,s->backup_position,
        s->pilot,s->departed,s->alien_x,s->alien_y,s->hit,s->delay_ms,s->trials,s->state.combat.on_alfol);fflush(stdout);
}
int main(void)
{
    TeraExplorationState state;TeraSpaceSession session;unsigned mode,command;int pirates,backup,key;
    if(!tera_frontend_read_exploration(&state,&mode,&command) || mode || command>2 ||
       !tera_frontend_read_int(&pirates,-32768,32767) || !tera_frontend_read_int(&backup,-32768,32767)) return 2;
    if(command==0) {if(!tera_space_start(&session,&state.world.characters,(int16_t)pirates)) return 3;}
    else if(!tera_space_travel_start(&session,&state.world.characters,command==2,(int16_t)pirates,(int16_t)backup)) return 3;
    show(&session);
    while(session.phase<TERA_SPACE_DONE) {
        if(!tera_frontend_read_int(&key,0,255)) return 4;
        if(!tera_space_key(&session,(unsigned)key)) return 3;
        show(&session);
    }
    return 0;
}
