#include "exploration_transport.h"
#include "character_transport.h"
#include "tera/navigation.h"
#include <stdio.h>
#include <string.h>

static uint8_t previous_maps[1100][7];

static void event(void *context,const TeraCombatEvent *e)
{
    (void)context;
    printf("E %d %u %d %d\n",(int)e->kind,e->actor,(int)e->letter,e->value);
}

static void show(TeraNavigationSession *s)
{
    TeraCreationSession packet;
    TeraExplorationState *state=s->phase==TERA_NAV_DAY ? &s->day.state : &s->state;
    if(s->phase==TERA_NAV_CHEST) state=&s->chest.state;
    TeraCombatGame *g=s->phase==TERA_NAV_DAY && s->day.phase==TERA_DAY_COMBAT ?
        &s->day.combat.game : &state->world.characters.combat;
    unsigned i,j;
    memset(&packet,0,sizeof packet);packet.phase=TERA_CREATE_DONE;
    packet.state=state->world.characters;packet.state.combat=*g;
    tera_frontend_show_creation(&packet);
    printf("J %d\n",state->information_context);
    printf("F %u\n",state->world.tasks_done[3]);
    printf("R %d\n",s->mandatory_temple_command);
    printf("N %d %d %d %d %d %d %d %d %d %u %u %u %u %u %u %u %d %d %d %d %d %d %d %d\n",
        (int)s->phase,s->moved,s->changed_level,s->backup_position,state->navigation_overlay,
        state->encounter_suppressed,g->recruit_ready,g->location_changed,g->previous_position,
        s->hint_count,s->hints[0],s->hints[1],s->healed_mask,g->encounter.monster_type,
        s->day.combat.actor,s->day.combat.attempt,(int)s->day.combat.phase,
        g->encounter.fled,g->encounter.victory,state->world.tasks_done[2],state->world.tasks_counter,
        state->force_day,s->day.day_advanced,s->day.level_advanced);
    printf("W %u",s->word_length);
    for(i=0;i<11;++i) printf(" %u",(unsigned char)s->word[i]);
    putchar('\n');
    printf("G %d %d %d %d %u %u",state->chest_open,(int)s->chest.phase,
        s->chest.trap,(int)s->chest.outcome,s->chest.actor,s->chest.pending_item);
    for(i=0;i<11;++i) printf(" %u",state->world.has_map[i]);
    putchar('\n');
    for(i=0;i<1100;++i) if(memcmp(previous_maps[i],state->world.maps[i],7)!=0) {
        printf("B %u",i);
        for(j=0;j<7;++j) printf(" %u",state->world.maps[i][j]);
        putchar('\n');memcpy(previous_maps[i],state->world.maps[i],7);
    }
    printf("Z\n");fflush(stdout); /* Frontiere : B arrive avant la touche suivante. */
}

int main(void)
{
    TeraExplorationState state;
    TeraNavigationSession session;
    unsigned mode,key;
    int input,initial_move,backup,overlay,v;
    if(!tera_frontend_read_exploration(&state,&mode,&key) ||
       !tera_frontend_read_int(&initial_move,0,1) || !tera_frontend_read_int(&backup,-32768,32767) ||
       !tera_frontend_read_int(&overlay,0,2) || !tera_frontend_read_int(&v,-32768,32767)) return 2;
    state.city_backup_position=(int16_t)backup;state.navigation_overlay=overlay;state.world.tasks_counter=(int16_t)v;
    if(!tera_frontend_read_int(&v,0,1)) return 2;
    state.world.tasks_done[2]=(uint8_t)v;
    if(!tera_frontend_read_int(&v,-32768,32767)) return 2;
    state.world.arioch_position=(int16_t)v;
    if(!tera_frontend_read_int(&v,0,1)) return 2;
    state.chest_open=v;
    for(input=0;input<11;++input) {
        if(!tera_frontend_read_int(&v,0,1)) return 2;
        state.world.has_map[input]=(uint8_t)v;
    }
    if(!tera_frontend_read_int(&v,0,1)) return 2;
    state.world.tasks_done[3]=(uint8_t)v;
    for(input=0;input<10;++input) {
        if(!tera_frontend_read_int(&v,0,255)) return 2;
        state.world.secret_words[3][input]=(char)v;
    }
    state.world.secret_words[3][9]=0;
    memcpy(previous_maps,state.world.maps,sizeof previous_maps);
    if(mode==1) {
        if(!tera_navigation_leave(&state,(int16_t)backup)) return 3;
        memset(&session,0,sizeof session);session.state=state;
        session.backup_position=(int16_t)backup;session.phase=TERA_NAV_DONE;
    } else if(mode==2) {
        if(!tera_navigation_enter(&session,&state,event,NULL)) return 3;
    } else if(!tera_navigation_start(&session,&state,key,initial_move,(int16_t)backup,event,NULL)) return 3;
    show(&session);
    while(session.phase!=TERA_NAV_DONE && session.phase!=TERA_NAV_ERROR) {
        if(scanf("%d",&input)!=1) return 4;
        if(input<0 || input>255 || !tera_navigation_key(&session,(unsigned)input)) return 3;
        show(&session);
    }
    return session.phase==TERA_NAV_ERROR ? 5 : 0;
}
