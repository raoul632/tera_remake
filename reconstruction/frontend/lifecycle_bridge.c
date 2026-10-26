#include "exploration_transport.h"
#include "character_transport.h"
#include "tera/lifecycle.h"
#include <stdio.h>
#include <string.h>

static void show(const TeraLifeSession *s)
{
    TeraCreationSession packet;
    const TeraCombatGame *g;
    memset(&packet,0,sizeof packet);
    if(s->phase==TERA_LIFE_CREATE) packet=s->creation;
    else {packet.state=s->state.world.characters;packet.phase=TERA_CREATE_DONE;}
    tera_frontend_show_creation(&packet);g=&packet.state.combat;
    printf("L %d %d %u %u %u %u %d %d %d %d\n",(int)s->phase,(int)packet.phase,
        packet.split,packet.bonuses_left,packet.name_length,packet.attempts,g->on_alfol,
        g->encounter.fled,g->recruit_ready,g->encounter.victory);fflush(stdout);
}

int main(void)
{
    TeraExplorationState state;
    TeraLifeSession session;
    unsigned mode,command,i;
    int v,key;
    if(!tera_frontend_read_exploration(&state,&mode,&command) || mode) return 2;
    for(i=0;i<4;++i) {
        if(!tera_frontend_read_int(&v,0,1)) return 2;
        state.world.tasks_done[i]=(uint8_t)v;
    }
    if(!tera_frontend_read_int(&v,0,1)) return 2;
    state.world.characters.combat.encounter.fled=v;
    if(!tera_frontend_read_int(&v,0,1)) return 2;
    state.world.characters.combat.encounter.victory=v;
    if(!tera_life_start(&session,&state)) return 3;
    show(&session);
    while(session.phase<TERA_LIFE_READY) {
        if(!tera_frontend_read_int(&key,0,255)) return 4;
        if(!tera_life_key(&session,(unsigned)key)) return 3;
        show(&session);
    }
    return session.phase==TERA_LIFE_ERROR ? 3 : 0;
}
