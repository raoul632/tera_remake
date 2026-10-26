#include "exploration_transport.h"
#include "character_transport.h"
#include "tera/amarande.h"
#include <stdio.h>
#include <string.h>
static void event(void *context,const TeraCombatEvent *e)
{
    (void)context;printf("E %d %u %d %d\n",e->kind,e->actor,e->letter,e->value);
}
static void show(const TeraAmarandeSession *s)
{
    TeraCreationSession p;const TeraExplorationState *state=&s->state;
    const TeraCombatSession *combat=NULL;
    if(s->phase==TERA_AM_COMBAT) combat=&s->combat;
    else if(s->phase==TERA_AM_DAY) {state=&s->day.state;if(s->day.phase==TERA_DAY_COMBAT) combat=&s->day.combat;}
    memset(&p,0,sizeof p);p.phase=TERA_CREATE_DONE;p.state=state->world.characters;
    if(combat) p.state.combat=combat->game;
    tera_frontend_show_creation(&p);
    printf("Y %d %d %d %d %d %d %d %d %u %u %d %u %u %u %d %d %d\n",s->phase,state->sound,
        state->world.fast_moves,state->information_context,state->encounter_suppressed,
        p.state.combat.recruit_ready,p.state.combat.location_changed,p.state.combat.previous_position,
        s->outcome,s->shown_actor,combat ? (int)combat->phase : -1,combat ? combat->actor : 0,
        combat ? combat->game.encounter.monster_type : 0,s->choice,s->east,s->north,s->status_actor);
    printf("F %d\nQ %d\nW %u",p.state.combat.encounter.fled,s->quit_requested,s->word_length);
    {unsigned i;for(i=0;i<10;++i) printf(" %u",(unsigned char)s->word[i]);}
    printf("\nR %u %u %u\nZ\n",s->response_id,s->response_variant,s->response_count);fflush(stdout);
}
int main(void)
{
    TeraExplorationState state;TeraAmarandeSession s;unsigned mode,command;int scratch,variant,key,v;
    if(!tera_frontend_read_exploration(&state,&mode,&command) || mode ||
       !tera_frontend_read_int(&scratch,-32768,32767) || !tera_frontend_read_int(&variant,0,2) ||
       !tera_frontend_read_int(&v,0,1)) return 2;
    state.world.initial_flag=v;
    if(!tera_amarande_start(&s,&state,command,(int16_t)scratch,event,NULL)) return 3;
    s.extra_variant=(unsigned)variant;show(&s);
    while(s.phase<TERA_AM_DONE) {
        if(!tera_frontend_read_int(&key,0,255)) return 4;
        if(!tera_amarande_key(&s,(unsigned)key)) return 3;
        show(&s);
    }
    return s.phase==TERA_AM_ERROR ? 5 : 0;
}
