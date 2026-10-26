#include "tera/meduz.h"
#include "tera/sound.h"
#include <string.h>
/* BIN OVL00A4, TERAMAIN/pyramide, OVL0145 et OVL0142. Le dessin reste
 * abstrait, son calendrier RANDOM reste dans ce module, jamais dans l'hote. */
static int16_t word16(int32_t v)
{
    uint32_t n=(uint32_t)v&65535u;
    return (int16_t)(n<32768 ? (int32_t)n : (int32_t)n-65536);
}
static unsigned draw(TeraMeduzSession *s,unsigned bound)
{
    unsigned v=0;tera_random_draw(&s->state.world.characters.combat.random,bound,&v);return v;
}
/* OVL0145 : sept tunnels. RANDOM3(color), RANDOM5(step), RANDOM7(trail)
 * sont evalues avant l'appel ; deux directions puis vingt-quatre etoiles. */
static void transition(TeraMeduzSession *s)
{
    TeraCombatGame *g=&s->state.world.characters.combat;
    unsigned i,j,step,height,trail,color;int dx,dy,x,y;
    for(i=0;i<7;++i) {
        TeraPresentationEvent e=tera_presentation_make(g,TERA_PRESENT_TUNNEL,TERA_SOURCE_MEDUZ);
        TeraTunnelFrame *f=&e.data.tunnel;
        color=1+draw(s,3);step=2+draw(s,5);trail=4+draw(s,7);
        dx=(int)draw(s,3)-1;dy=(int)draw(s,3)-1;
        x=160+(100/(int)step)*dx*(int)step*4/3;
        y=100+(100/(int)step)*dy*(int)step*5/4;
        for(j=0;j<24;++j) {f->stars[2*j]=(unsigned short)(30+draw(s,255));f->stars[2*j+1]=(unsigned short)(10+draw(s,180));}
        height=100;f->reset=1;f->color=color;
        while(height>step) {
            f->palette=draw(s,4)==1 ? draw(s,3) : 3;
            f->background=draw(s,16)==1;
            height-=step;x-=dx*(int)step*4/3;y-=dy*(int)step*5/4;
            f->x=x;f->y=y;f->radius=height;
            f->erase_x=x+(int)trail*dx*(int)step*4/3;
            f->erase_y=y+(int)trail*dy*(int)step*5/4;
            f->erase_radius=height<100-trail*step ? height+trail*step : 0;
            f->hz=s->state.sound ? height*80 : 0;
            tera_presentation_emit(g,&e);f->reset=0;
        }
    }
}
static void return_to_pyramid(TeraMeduzSession *s)
{
    unsigned i;TeraCombatGame *g=&s->state.world.characters.combat;
    int old_position=g->position;
    s->state.world.characters.days=word16(s->state.world.characters.days+7);
    g->position=s->backup_position;g->world_id=11;
    tera_presentation_zone(g,TERA_SOURCE_MEDUZ,10,old_position,g->on_alfol);
    /* Les animations de TERAMAIN ne font pas partie du banc overlay seul. */
    if(s->entered) {
        TeraPresentationEvent e=tera_presentation_make(g,TERA_PRESENT_SCENERY,TERA_SOURCE_MEDUZ);
        e.data.scenery.kind=TERA_PICTURE_PLACE;e.data.scenery.variant=3;
        transition(s);
        for(i=0;i<26;++i) {
            e.data.scenery.values[e.data.scenery.count++]=(unsigned short)draw(s,76);
            e.data.scenery.values[e.data.scenery.count++]=(unsigned short)draw(s,8);
        }
        for(i=0;i<12;++i) {
            e.data.scenery.values[e.data.scenery.count++]=(unsigned short)draw(s,300);
            e.data.scenery.values[e.data.scenery.count++]=(unsigned short)draw(s,48);
        }
        tera_presentation_emit(g,&e);
    }
}
static void render(TeraMeduzSession *s,int changed);
static void finish(TeraMeduzSession *s)
{
    TeraCombatGame *g;int invocation=0;
    if(s->fixed_combat) {
        if(s->combat.phase==TERA_COMBAT_ERROR) {s->phase=TERA_MEDUZ_ERROR;return;}
        if(s->combat.phase!=TERA_COMBAT_DONE) return;
        s->state.world.characters.combat=s->combat.game;invocation=s->fixed_combat==2;s->fixed_combat=0;
        if(invocation) {
            s->phase=TERA_MEDUZ_DONE;g=&s->state.world.characters.combat;
            if(g->encounter.fled) {g->encounter.fled=0;render(s,1);}
            else if(!g->encounter.party_available) return_to_pyramid(s);
            return;
        }
        if(!tera_day_start(&s->day,&s->state,s->observe,s->context)) {s->phase=TERA_MEDUZ_ERROR;return;}
    }
    if(s->day.phase==TERA_DAY_ERROR) {s->phase=TERA_MEDUZ_ERROR;return;}
    if(s->day.phase!=TERA_DAY_DONE) return;
    s->state=s->day.state;s->phase=TERA_MEDUZ_DONE;g=&s->state.world.characters.combat;
    /* La fuite est traitee AVANT UNTIL, meme apres un combat/jour mortel. */
    if(g->encounter.fled) {g->encounter.fled=0;render(s,1);return;}
    if(s->initial_render) {s->mandatory_command=!g->encounter.party_available;return;}
    if(g->position==999 || !g->encounter.party_available) return_to_pyramid(s);
}
/* BCDD : chaque tentative de marche, meme bloquee, appelle DailyRefresh.
 * Les cinq sprites R66 et le son deterministe C6221 precedent FightSequence(22,0),
 * qui n'est pas SpawnMonster : ne pas ajouter ses tirages de presentation. */
static void render(TeraMeduzSession *s,int changed)
{
    TeraCombatGame *g=&s->state.world.characters.combat;
    unsigned i,content=s->state.world.maps[g->position][4];int fight=0;
    if(changed) {
        TeraPresentationEvent e=tera_presentation_make(g,TERA_PRESENT_SCENERY,TERA_SOURCE_MEDUZ);
        e.data.scenery.kind=TERA_PICTURE_MEDUZ;
        for(i=0;i<16;++i) {
            e.data.scenery.values[e.data.scenery.count++]=(unsigned short)draw(s,300);
            e.data.scenery.values[e.data.scenery.count++]=(unsigned short)draw(s,70);
        }
        fight=content==2 || content==3;
        if(!fight) fight=draw(s,9)==4;
        if(fight) {
            for(i=0;i<5;++i) e.data.scenery.values[e.data.scenery.count++]=(unsigned short)draw(s,66);
            tera_presentation_emit(g,&e);
            s->state.information_context=1; /* C6073 ; C6221 n'a aucun RANDOM. */
            if(s->state.sound) tera_sound_wave(g,3000,30,100);
            g->sound=s->state.sound;g->encounter.monster_type=22;
            if(!tera_combat_start(&s->combat,g,0,s->observe,s->context)) {s->phase=TERA_MEDUZ_ERROR;return;}
            s->fixed_combat=1;s->phase=TERA_MEDUZ_DAY;finish(s);return;
        }
        tera_presentation_emit(g,&e);
    }
    if(!tera_day_start(&s->day,&s->state,s->observe,s->context)) {s->phase=TERA_MEDUZ_ERROR;return;}
    s->phase=TERA_MEDUZ_DAY;finish(s);
}
static void arrive(TeraMeduzSession *s)
{
    TeraCombatGame *g=&s->state.world.characters.combat;
    unsigned old_world=g->world_id;int old_position=g->position;
    g->world_id=10;g->position=1000;g->previous_position=999;s->initial_render=1;
    tera_presentation_zone(g,TERA_SOURCE_MEDUZ,old_world,old_position,g->on_alfol);render(s,1);
}
int tera_meduz_start(TeraMeduzSession *s,const TeraExplorationState *state,unsigned command,
    int enter,int initial,int travelling,int16_t backup,TeraCombatObserver observer,void *context)
{
    TeraExplorationState copy;TeraCombatGame *g;int heading,delta,wall;
    if(!s || !state || command>255 || (enter!=0 && enter!=1) || (initial!=0 && initial!=1) ||
       (travelling!=0 && travelling!=1) || (enter && initial)) return 0;
    copy=*state;memset(s,0,sizeof *s);s->state=copy;s->command=command;s->backup_position=backup;
    s->observe=observer;s->context=context;s->entered=enter ? 0 : travelling;s->phase=TERA_MEDUZ_DONE;g=&s->state.world.characters.combat;
    if(enter) {s->phase=TERA_MEDUZ_ENTRY;return 1;}
    if(initial) {arrive(s);return 1;}
    if(g->world_id!=10 || g->position<1000 || g->position>1099) return 0;
    if(command=='M' && s->state.world.maps[g->position][4]==2) {s->phase=TERA_MEDUZ_MEDITATE;return 1;}
    if(command=='I' && s->state.world.maps[g->position][4]==3) {s->phase=TERA_MEDUZ_INVOKE;return 1;}
    if(command=='I') {s->generic_invocation=1;s->phase=TERA_MEDUZ_WORD;return 1;}
    if(command=='2' || command=='4' || command=='6' || command=='8') {
        int old_position=g->position;
        heading=word16(g->position-g->previous_position);delta=heading;
        if(command=='2') delta=-heading;
        if(command=='6') delta=heading==1 ? 10 : heading==-1 ? -10 : heading==10 ? -1 : 1;
        if(command=='4') delta=heading==1 ? -10 : heading==-1 ? 10 : heading==10 ? 1 : -1;
        wall=delta==1 ? 0 : delta==-1 ? 2 : delta==10 ? 3 : 1;
        if(heading!=1 && heading!=-1 && heading!=10 && heading!=-10) return 0;
        if(s->state.world.maps[g->position][wall]) {
            g->previous_position=g->position;g->position=word16(g->position+delta);s->moved=1;
            if(g->position<0 || g->position>1099) return 0;
        }
        tera_presentation_move(g,TERA_SOURCE_MEDUZ,command,old_position,s->moved,(unsigned)wall);
        render(s,s->moved);
    } else if(command=='D' || command=='M' || command=='L' || command=='O' || command=='P' || command=='E')
        tera_presentation_message(g,TERA_SOURCE_MEDUZ,TERA_TEXT_COMMAND_RESPONSE,command,draw(s,3));
    return 1;
}
int tera_meduz_resume(TeraMeduzSession *s,const TeraExplorationState *state,int16_t backup)
{
    if(!tera_meduz_start(s,state,0,0,0,1,backup,NULL,NULL)) return 0;
    s->entered=1;
    if(!s->state.world.characters.combat.encounter.party_available || s->state.world.characters.combat.position==999)
        return_to_pyramid(s);
    return 1;
}
/* Les lecteurs de mots suivent C42AC : dix lettres acceptees, neuf
 * stockees, retour arriere ; comparaison exacte des mots generes. */
int tera_meduz_key(TeraMeduzSession *s,unsigned key)
{
    unsigned i;TeraCombatGame *g;
    if(!s || key>255 || s->phase>=TERA_MEDUZ_DONE) return 0;
    if(key>='a' && key<='z') key-=32;
    g=&s->state.world.characters.combat;
    if(s->phase==TERA_MEDUZ_DAY) {
        if(s->fixed_combat) {if(!tera_combat_key(&s->combat,key)) return 0;}
        else if(!tera_day_key(&s->day,key)) return 0;
        finish(s);return 1;
    }
    if(s->phase==TERA_MEDUZ_ENTRY_WORD || s->phase==TERA_MEDUZ_WORD) {
        if(key==8 && s->word_length) s->word[--s->word_length]=0;
        else if(key>='A' && key<='Z') {if(s->word_length<9) s->word[s->word_length]=(char)key;++s->word_length;}
        if(key!=13 && s->word_length<10) return 1;
        s->word[9]=0;
        if(s->phase==TERA_MEDUZ_ENTRY_WORD) {
            s->phase=TERA_MEDUZ_DONE;
            if(strcmp(s->word,s->state.world.secret_words[1])==0) {
                s->backup_position=g->position;s->entered=1;
                if(s->state.sound) tera_sound_entry_meduz(g);else tera_media_wait(g,1000);
                transition(s);arrive(s);
            }
        } else {
            s->phase=TERA_MEDUZ_DONE;
            if(s->generic_invocation) {
                int allowed=!g->on_alfol;unsigned calls=0;
                for(i=0;i<10;++i) if(s->state.world.towns[i]==g->position) allowed=0;
                if(allowed && strcmp(s->word,"ARIOCH")==0) {
                    s->state.encounter_suppressed=1;s->state.information_context=1;
                    g->sound=s->state.sound;g->encounter.monster_type=21;
                    tera_monster_presentation_random(g,21,&calls);
                    if(!tera_combat_start(&s->combat,g,0,s->observe,s->context)) {s->phase=TERA_MEDUZ_ERROR;return 1;}
                    s->fixed_combat=2;s->phase=TERA_MEDUZ_DAY;finish(s);
                }
            } else if(strcmp(s->word,s->state.world.secret_words[2])==0 && s->state.world.mental_extension && !s->state.world.tasks_done[0]) {
                s->state.world.tasks_done[0]=1;s->state.world.tasks_counter=word16(s->state.world.tasks_counter+1);
                tera_presentation_objective(g,TERA_SOURCE_MEDUZ,0);
                if(s->state.sound) tera_sound_objective(g);
            }
        }
        return 1;
    }
    if(s->phase==TERA_MEDUZ_ENTRY) {
        s->phase=key=='O' || key==13 ? TERA_MEDUZ_ENTRY_WORD : TERA_MEDUZ_DONE;
    } else if(s->phase==TERA_MEDUZ_INVOKE) {
        s->phase=key=='O' || key==13 ? TERA_MEDUZ_WORD : TERA_MEDUZ_DONE;
    } else {
        s->phase=TERA_MEDUZ_DONE;
        if((key=='O' || key==13) && !s->state.world.mental_extension) {
            s->state.world.mental_extension=1;
            for(i=0;i<10;++i) if(g->encounter.actors[i].active) {
                ++g->encounter.attributes[i].stats[5];++g->encounter.attributes[i].stats[2];
            }
            tera_presentation_interaction(g,TERA_SOURCE_MEDUZ,TERA_INTERACTION_MENTAL_EXTENSION,0);
        }
    }
    return 1;
}
