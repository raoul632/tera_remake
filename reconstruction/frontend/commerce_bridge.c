#include "exploration_transport.h"
#include "character_transport.h"
#include "tera/commerce.h"
#include <stdio.h>
#include <string.h>

static void show(const TeraShopSession *s)
{
    TeraCreationSession packet;unsigned i;
    memset(&packet,0,sizeof packet);packet.phase=TERA_CREATE_DONE;packet.state=s->state.world.characters;
    tera_frontend_show_creation(&packet);
    printf("K %d %u %u %u %d %d %d",s->phase,s->command,s->actor,s->item,s->price,s->weeks,s->outcome);
    for(i=0;i<11;++i) printf(" %u",s->state.world.has_map[i]);
    putchar('\n');fflush(stdout);
}

int main(void)
{
    TeraExplorationState state;TeraShopSession session;unsigned mode,key,i;int value;
    if(!tera_frontend_read_exploration(&state,&mode,&key) || (mode!=0 && mode!=2)) return 2;
    for(i=0;i<11;++i) {
        if(!tera_frontend_read_int(&value,0,1)) return 2;
        state.world.has_map[i]=(uint8_t)value;
    }
    if(!tera_shop_start(&session,&state)) return 3;
    if(mode==2) {session.phase=TERA_SHOP_CONFIRM;session.command='E';}
    show(&session);
    while(session.phase<TERA_SHOP_DONE) {
        if(!tera_frontend_read_int(&value,0,255)) return 4;
        if(!tera_shop_key(&session,(unsigned)value)) return 3;
        show(&session);
    }
    return session.phase==TERA_SHOP_ERROR ? 3 : 0;
}
