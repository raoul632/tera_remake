#include "exploration_transport.h"
#include "character_transport.h"
#include "tera/commerce.h"
#include <stdio.h>
#include <string.h>
int main(void)
{
    TeraExplorationState state;TeraCreationSession packet;unsigned mode,force;
    if(!tera_frontend_read_exploration(&state,&mode,&force) || mode || force>1) return 2;
    if(!tera_places_present(&state,(int)force)) return 3;
    memset(&packet,0,sizeof packet);packet.phase=TERA_CREATE_DONE;packet.state=state.world.characters;
    tera_frontend_show_creation(&packet);printf("V %d\nJ %d\n",state.moved,state.information_context);return 0;
}
