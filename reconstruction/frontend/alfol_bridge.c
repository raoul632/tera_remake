#include "exploration_transport.h"
#include "character_transport.h"
#include "tera/alfol.h"
#include <stdio.h>
#include <string.h>
static uint8_t previous_maps[1100][7];
static char previous_hint[40];
static void event(void *context,const TeraCombatEvent *e)
{
    (void)context;printf("E %d %u %d %d\n",e->kind,e->actor,e->letter,e->value);
}
static void show(const TeraAlfolSession *s)
{
    TeraCreationSession packet;const TeraExplorationState *state=&s->state;
    const TeraCombatSession *combat=NULL;unsigned i,j;
    if(s->phase==TERA_ALFOL_DAY) {
        if(s->fixed_combat) combat=&s->combat;
        else {state=&s->day.state;if(s->day.phase==TERA_DAY_COMBAT) combat=&s->day.combat;}
    }
    memset(&packet,0,sizeof packet);packet.phase=TERA_CREATE_DONE;packet.state=state->world.characters;
    if(combat) packet.state.combat=combat->game;
    tera_frontend_show_creation(&packet);
    printf("Y %d %d %d %d %d %d %d %d %d %d %d %d %u %u %d\n",s->phase,s->moved,
        packet.state.combat.on_alfol,s->status_actor,state->information_context,state->encounter_suppressed,
        packet.state.combat.recruit_ready,packet.state.combat.location_changed,packet.state.combat.previous_position,
        state->world.tasks_counter,state->world.tasks_done[1],combat ? (int)combat->phase : -1,
        combat ? combat->actor : 0,combat ? combat->game.encounter.monster_type : 0,
        s->phase==TERA_ALFOL_DONE && s->initial_render && !packet.state.combat.encounter.party_available);
    for(i=0;i<1100;++i) if(memcmp(previous_maps[i],state->world.maps[i],7)!=0) {
        printf("B %u",i);for(j=0;j<7;++j) printf(" %u",state->world.maps[i][j]);putchar('\n');
        memcpy(previous_maps[i],state->world.maps[i],7);
    }
    if(memcmp(previous_hint,state->world.hints[0],40)!=0) {
        printf("H");for(i=0;i<40;++i) printf(" %u",(unsigned char)state->world.hints[0][i]);putchar('\n');
        memcpy(previous_hint,state->world.hints[0],40);
    }
    printf("Z\n");fflush(stdout);
}
int main(void)
{
    TeraExplorationState state;TeraAlfolSession session;unsigned mode,key,i;int v,actor;
    if(!tera_frontend_read_exploration(&state,&mode,&key) || mode>2 ||
       !tera_frontend_read_int(&actor,-32768,32767) || !tera_frontend_read_int(&v,-32768,32767)) return 2;
    state.world.tasks_counter=(int16_t)v;
    for(i=0;i<4;++i) {
        if(!tera_frontend_read_int(&v,0,1)) return 2;
        state.world.tasks_done[i]=(uint8_t)v;
    }
    for(i=0;i<40;++i) {
        if(!tera_frontend_read_int(&v,0,255)) return 2;
        state.world.hints[0][i]=(char)v;
    }
    memcpy(previous_maps,state.world.maps,sizeof previous_maps);
    memcpy(previous_hint,state.world.hints[0],40);
    if(mode==2) {if(!tera_alfol_leave(&session,&state,actor)) return 3;}
    else if(!tera_alfol_start(&session,&state,key,mode,actor,event,NULL)) return 3;
    show(&session);
    while(session.phase<TERA_ALFOL_DONE) {
        if(!tera_frontend_read_int(&v,0,255)) return 4;
        if(!tera_alfol_key(&session,(unsigned)v)) return 3;
        show(&session);
    }
    return session.phase==TERA_ALFOL_ERROR ? 5 : 0;
}
