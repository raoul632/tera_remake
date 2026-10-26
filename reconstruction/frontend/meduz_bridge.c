#include "exploration_transport.h"
#include "character_transport.h"
#include "tera/meduz.h"
#include <stdio.h>
#include <string.h>
static void event(void *context,const TeraCombatEvent *e)
{
    (void)context;printf("E %d %u %d %d\n",e->kind,e->actor,e->letter,e->value);
}
static void show(const TeraMeduzSession *s)
{
    TeraCreationSession packet;const TeraExplorationState *state=&s->state;
    const TeraCombatSession *combat=NULL;
    if(s->phase==TERA_MEDUZ_DAY) {
        if(s->fixed_combat) combat=&s->combat;
        else {state=&s->day.state;if(s->day.phase==TERA_DAY_COMBAT) combat=&s->day.combat;}
    }
    memset(&packet,0,sizeof packet);packet.phase=TERA_CREATE_DONE;packet.state=state->world.characters;
    if(combat) packet.state.combat=combat->game;
    tera_frontend_show_creation(&packet);
    printf("V %d %d %d %d %d %d %d %d %d %d %d %d %d %u %u %u\n",s->phase,s->moved,s->backup_position,
        state->world.mental_extension,state->world.tasks_counter,state->world.tasks_done[0],s->mandatory_command,
        state->information_context,state->encounter_suppressed,packet.state.combat.recruit_ready,
        packet.state.combat.location_changed,packet.state.combat.previous_position,
        combat ? (int)combat->phase : -1,combat ? combat->actor : 0,
        combat ? combat->game.encounter.monster_type : 0,s->entered);
    printf("W %u",s->word_length);
    {unsigned i;for(i=0;i<10;++i) printf(" %u",(unsigned char)s->word[i]);}
    printf("\nZ\n");fflush(stdout);
}
int main(void)
{
    TeraExplorationState state;TeraMeduzSession session;unsigned mode,key,i,j;int v,backup,travel,enter;
    if(!tera_frontend_read_exploration(&state,&mode,&key) ||
       !tera_frontend_read_int(&backup,-32768,32767) || !tera_frontend_read_int(&travel,0,1) ||
       !tera_frontend_read_int(&enter,0,1) || !tera_frontend_read_int(&v,0,1)) return 2;
    state.world.mental_extension=v;
    if(!tera_frontend_read_int(&v,-32768,32767)) return 2;
    state.world.tasks_counter=(int16_t)v;
    for(i=0;i<4;++i) {
        if(!tera_frontend_read_int(&v,0,1)) return 2;
        state.world.tasks_done[i]=(uint8_t)v;
    }
    for(i=0;i<4;++i) for(j=0;j<10;++j) {
        if(!tera_frontend_read_int(&v,0,255)) return 2;
        state.world.secret_words[i][j]=(char)v;
    }
    for(i=0;i<4;++i) if(!memchr(state.world.secret_words[i],0,10)) return 2;
    if(mode==2) {if(!tera_meduz_resume(&session,&state,(int16_t)backup)) return 3;}
    else if(!tera_meduz_start(&session,&state,key,enter,mode,travel,(int16_t)backup,event,NULL)) return 3;
    show(&session);
    while(session.phase<TERA_MEDUZ_DONE) {
        if(!tera_frontend_read_int(&v,0,255)) return 4;
        if(!tera_meduz_key(&session,(unsigned)v)) return 3;
        show(&session);
    }
    return session.phase==TERA_MEDUZ_ERROR ? 5 : 0;
}
