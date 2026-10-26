#include "character_transport.h"
#include "exploration_transport.h"
#include "tera/exploration.h"
#include <stdio.h>
#include <string.h>

static void event(void *context,const TeraCombatEvent *e)
{
    (void)context;
    printf("E %d %u %d %d\n",(int)e->kind,e->actor,(int)e->letter,e->value);
}

static void show(TeraDaySession *s)
{
    TeraCreationSession packet;
    TeraCombatGame *g=s->phase==TERA_DAY_COMBAT ? &s->combat.game : &s->state.world.characters.combat;
    memset(&packet,0,sizeof packet);
    packet.state=s->state.world.characters;packet.state.combat=*g;packet.phase=TERA_CREATE_DONE;
    tera_frontend_show_creation(&packet);
    printf("J %d\n",s->state.information_context);
    printf("D %d %u %u %d %d %d %d %u %d %d %d %d %d %u %u %u %u %d %d %d\n",
        (int)s->phase,s->stage,s->encounters,s->day_advanced,s->level_advanced,
        s->state.moved,s->state.town_found,s->state.town_id,s->state.force_day,
        s->state.encounter_suppressed,g->recruit_ready,g->on_alfol,g->location_changed,
        g->encounter.monster_type,(unsigned)s->combat.phase,s->combat.actor,s->combat.attempt,
        g->encounter.fled,g->encounter.victory,g->previous_position);
    fflush(stdout);
}

int main(void)
{
    TeraExplorationState state;
    TeraDaySession session;
    TeraMoveReport move={0};
    unsigned mode,key;
    int input;
    if(!tera_frontend_read_exploration(&state,&mode,&key)) return 2;
    if(mode!=1 && !tera_exploration_move(&state,key,&move)) return 3;
    if(mode==0 || (mode==2 && !move.moved)) {
        memset(&session,0,sizeof session);session.state=state;session.phase=TERA_DAY_DONE;
    } else if(!tera_day_start(&session,&state,event,NULL)) return 3;
    printf("M %d %d %u %u\n",move.moved,move.near_mountain,move.scene,move.random_calls);
    if(mode!=1 && move.npc_present) printf("Y %u\n",move.npc_variant);
    show(&session);
    while(session.phase==TERA_DAY_COMBAT) {
        if(scanf("%d",&input)!=1) return 4; /* EOF : jamais une victoire fabriquee. */
        if(input<0 || input>255 || !tera_day_key(&session,(unsigned)input)) return 3;
        show(&session);
    }
    return session.phase==TERA_DAY_ERROR ? 5 : 0;
}
