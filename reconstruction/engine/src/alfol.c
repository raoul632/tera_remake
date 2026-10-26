#include "tera/alfol.h"
#include "tera/sound.h"
#include <string.h>
/* BIN OVL0085 (8500:A400), resident CA0DD/C7EF6 et rendu C7136.
 * Les routines de dessin ci-dessous conservent seulement leurs RANDOM et
 * le global D9B66 qui a un effet ulterieur sur les depots radioactifs. */
static int16_t word16(int32_t v)
{
    uint32_t n=(uint32_t)v&65535u;
    return (int16_t)(n<32768 ? (int32_t)n : (int32_t)n-65536);
}
static unsigned draw(TeraAlfolSession *s,unsigned bound)
{
    unsigned v=0;tera_random_draw(&s->state.world.characters.combat.random,bound,&v);return v;
}
static unsigned scene_draw(TeraAlfolSession *s,unsigned bound,TeraPresentationEvent *e)
{unsigned v=draw(s,bound);e->data.scenery.values[e->data.scenery.count++]=(unsigned short)v;return v;}
static TeraPresentationEvent picture(TeraAlfolSession *s,unsigned kind,unsigned variant)
{
    TeraPresentationEvent e=tera_presentation_make(&s->state.world.characters.combat,TERA_PRESENT_SCENERY,TERA_SOURCE_ALFOL);
    e.data.scenery.kind=kind;e.data.scenery.variant=variant;return e;
}
static void horizon(TeraAlfolSession *s,TeraPresentationEvent *e)
{
    unsigned i;scene_draw(s,2,e);for(i=0;i<16;++i) {scene_draw(s,300,e);scene_draw(s,70,e);}s->status_actor=16;
}
/* C30D : vue de la ville. Aucun jour ici : CCBE la redessine a la sortie,
 * alors que CD8F ajoute son jour lorsqu'on s'en approche par la surface. */
static void city_scene(TeraAlfolSession *s)
{
    unsigned i,j;
    TeraPresentationEvent e=picture(s,TERA_PICTURE_ALFOL,1);
    /* CCBE dessine avant de retablir WorldId/Position : rattacher le
     * decor a sa destination, sans avancer cette mutation de jeu. */
    e.world=11;e.position=s->state.world.alfol_position;e.on_alfol=1;
    horizon(s,&e);
    for(i=0;i<7;++i) {scene_draw(s,3,&e);for(j=0;j<4;++j) scene_draw(s,4,&e);}
    for(i=0;i<9;++i) {scene_draw(s,2,&e);scene_draw(s,60,&e);scene_draw(s,3,&e);}
    for(i=0;i<39;++i) {scene_draw(s,76,&e);scene_draw(s,20,&e);}
    tera_presentation_emit(&s->state.world.characters.combat,&e);
}
static void landing_scene(TeraAlfolSession *s)
{
    unsigned i;
    TeraPresentationEvent e=picture(s,TERA_PICTURE_ALFOL,0);
    /* C7136 : ciel, neuf gratte-ciels, trois groupes de sprites. */
    for(i=0;i<16;++i) {scene_draw(s,300,&e);scene_draw(s,70,&e);}
    for(i=0;i<9;++i) {scene_draw(s,2,&e);scene_draw(s,60,&e);scene_draw(s,3,&e);}
    for(i=0;i<12;++i) {scene_draw(s,3,&e);scene_draw(s,2,&e);}
    for(i=0;i<3;++i) {scene_draw(s,4,&e);scene_draw(s,2,&e);}
    tera_presentation_emit(&s->state.world.characters.combat,&e);
    s->status_actor=16; /* C6F85 : le ciel laisse le global a seize. */
}
static void finish_day(TeraAlfolSession *s);
static void begin_day(TeraAlfolSession *s)
{
    if(!tera_day_start(&s->day,&s->state,s->observe,s->context)) {s->phase=TERA_ALFOL_ERROR;return;}
    s->phase=TERA_ALFOL_DAY;finish_day(s);
}
/* CD8F : rendu suivi de CA0DD, y compris a l'arrivee sans deplacement.
 * Les scenes modulo 23 ont des densites de flore differentes. */
static void surface(TeraAlfolSession *s)
{
    static const unsigned blocks[23]={5,5,5,5,5,3,4,5,5,5,5,5,5,4,5,5,5,5,3,5,5,5,5};
    unsigned i;int position=s->state.world.characters.combat.position;
    if(s->state.sound) tera_sound_steps(&s->state.world.characters.combat,s->state.world.fast_moves);
    if(position==0) landing_scene(s);
    else if(position==s->state.world.alfol_position) city_scene(s);
    else {
        TeraPresentationEvent e=picture(s,TERA_PICTURE_ALFOL,2);
        for(i=0;i<13*blocks[position%23];++i) {scene_draw(s,76,&e);scene_draw(s,20,&e);}
        horizon(s,&e);tera_presentation_emit(&s->state.world.characters.combat,&e);
    }
    begin_day(s);
}
static void office(TeraAlfolSession *s)
{
    unsigned i;TeraPresentationEvent e=picture(s,TERA_PICTURE_ROOM,1);
    for(i=0;i<4;++i) {scene_draw(s,4,&e);scene_draw(s,5,&e);}
    tera_presentation_emit(&s->state.world.characters.combat,&e);
}
static void after_media(TeraAlfolSession *s)
{
    TeraWorldState *w=&s->state.world;TeraCombatGame *g=&w->characters.combat;
    unsigned mask,id,last=0,type,calls=0;
    if(g->position==w->locations[7]) office(s);
    mask=tera_recruit_available(w);
    for(id=1;id<10;++id) if(mask&(1u<<id)) last=id;
    if(last==7 || last==9) s->state.information_context=1;
    if(w->maps[g->position][5]) {
        type=draw(s,3)+12;s->state.encounter_suppressed=1;s->state.information_context=1;
        g->sound=s->state.sound;g->encounter.monster_type=type;
        tera_monster_presentation_random(g,type,&calls);
        if(!tera_combat_start(&s->combat,g,0,s->observe,s->context)) {s->phase=TERA_ALFOL_ERROR;return;}
        s->fixed_combat=1;s->phase=TERA_ALFOL_DAY;finish_day(s);
    } else begin_day(s);
}
/* CAFC/CADx : contenu de la salle, avant PNJ, monstre puis jour.
 * Garder la faute d'indice de l'original : la perte de HP porte sur
 * chaque actif, mais UpdateHSState utilise le global D9B66. */
static void building(TeraAlfolSession *s)
{
    TeraCharacterState *c=&s->state.world.characters;TeraCombatGame *g=&c->combat;
    unsigned i,content=s->state.world.maps[g->position][4];
    if(s->state.sound) tera_sound_steps(g,s->state.world.fast_moves);
    if(content==2) {
        TeraPresentationEvent e=picture(s,TERA_PICTURE_ROOM,2);
        for(i=0;i<18;++i) scene_draw(s,3,&e);
        tera_presentation_emit(g,&e);
        if(s->state.sound) tera_sound_noise(g,30,110,50,10,10,8500,11000);
        else tera_media_wait(g,1000);
        if((s->status_actor<0 || s->status_actor>9) && s->status_actor!=16) {s->phase=TERA_ALFOL_ERROR;return;}
        for(i=0;i<10;++i) if(g->encounter.actors[i].active && g->encounter.actors[i].hp>0) {
            g->encounter.actors[i].hp=word16(g->encounter.actors[i].hp-1);
            if(s->status_actor==16) {
                /* C7CC8 lit alors HP[16] = MP[6], et son statut tombe dans
                 * le premier indice Pascal, a l'offset 18 de son record.
                 * Reproduire ces deux alias sans acces C hors tableau. */
                if(g->encounter.actors[6].magic<1 && !(s->state.world.hints[0][17]==2 &&
                    s->state.world.hints[0][18]=='H' && s->state.world.hints[0][19]=='S')) {
                    g->encounter.actors[6].magic=0;
                    s->state.world.hints[0][17]=2;s->state.world.hints[0][18]='H';s->state.world.hints[0][19]='S';
                }
            } else tera_combat_update_actor_condition(&g->encounter.actors[s->status_actor]);
        }
        tera_inventory_refresh_limits(c);
        for(i=0;i<10;++i) tera_presentation_actor(g,TERA_SOURCE_ALFOL,TERA_EFFECT_RADIATION,i);
    }
    if(content==1) {office(s);s->phase=TERA_ALFOL_MEDIA;}
    else after_media(s);
}
static void finish_day(TeraAlfolSession *s)
{
    TeraCombatGame *g;
    if(s->fixed_combat) {
        if(s->combat.phase==TERA_COMBAT_ERROR) {s->phase=TERA_ALFOL_ERROR;return;}
        if(s->combat.phase!=TERA_COMBAT_DONE) return;
        s->state.world.characters.combat=s->combat.game;s->fixed_combat=0;
        g=&s->state.world.characters.combat;
        if(g->encounter.victory) s->state.world.maps[g->position][5]=0;
        begin_day(s);return;
    }
    if(s->day.phase==TERA_DAY_ERROR) {s->phase=TERA_ALFOL_ERROR;return;}
    if(s->day.phase!=TERA_DAY_DONE) return;
    s->state=s->day.state;s->phase=TERA_ALFOL_DONE;
    g=&s->state.world.characters.combat;
    if(g->world_id==9 && (g->position==899 || !g->encounter.party_available) && !s->initial_render) {
        int old_position=g->position;
        city_scene(s);g->position=s->state.world.alfol_position;g->world_id=11;
        tera_presentation_zone(g,TERA_SOURCE_ALFOL,9,old_position,g->on_alfol);
    } else if(g->encounter.fled && g->encounter.party_available) {
        g->encounter.fled=0;
        if(g->world_id==11) {g->location_changed=0;g->position=word16(g->position-1);g->position=word16(g->position+1);surface(s);}
        else building(s);
    }
}
static void grid_step(TeraAlfolSession *s)
{
    TeraCombatGame *g=&s->state.world.characters.combat;
    int old_position=g->position;
    g->previous_position=g->position;g->position=word16(g->position+s->delta);s->moved=1;
    tera_presentation_move(g,TERA_SOURCE_ALFOL,s->command,old_position,1,s->wall);
    /* Meme le passage 899 est redessine et rafraichi AVANT UNTIL. */
    building(s);
}
int tera_alfol_leave(TeraAlfolSession *s,const TeraExplorationState *state,int status_actor)
{
    int old_position;
    if(!tera_alfol_start(s,state,0,0,status_actor,NULL,NULL)) return 0;
    if(s->state.world.characters.combat.world_id!=9) return 0;
    old_position=s->state.world.characters.combat.position;
    city_scene(s);s->state.world.characters.combat.position=s->state.world.alfol_position;
    s->state.world.characters.combat.world_id=11;
    tera_presentation_zone(&s->state.world.characters.combat,TERA_SOURCE_ALFOL,9,old_position,s->state.world.characters.combat.on_alfol);return 1;
}
int tera_alfol_start(TeraAlfolSession *s,const TeraExplorationState *state,unsigned command,
                    int initial,int status_actor,TeraCombatObserver observer,void *context)
{
    TeraExplorationState copy;TeraCombatGame *g;int heading,direction;
    if(!s || !state || command>255 || (initial!=0 && initial!=1)) return 0;
    if(state->world.characters.combat.world_id!=9 && state->world.characters.combat.world_id!=11) return 0;
    copy=*state;memset(s,0,sizeof *s);s->state=copy;s->command=command;s->status_actor=status_actor;
    s->observe=observer;s->context=context;s->phase=TERA_ALFOL_DONE;g=&s->state.world.characters.combat;
    if(initial) {
        unsigned old_world=g->world_id;int old_position=g->position,old_alfol=g->on_alfol;
        g->position=0;g->world_id=11;g->on_alfol=1;s->initial_render=1;
        tera_presentation_zone(g,TERA_SOURCE_ALFOL,old_world,old_position,old_alfol);surface(s);return 1;
    }
    if(command=='E' && g->world_id==11 && g->position==s->state.world.alfol_position) {s->phase=TERA_ALFOL_ENTER;return 1;}
    if(command=='R' && g->world_id==11) {s->phase=TERA_ALFOL_REBOARD;return 1;}
    if(command=='I') {s->phase=TERA_ALFOL_WORD;return 1;} /* C_B4FC lit le mot, puis refuse OnAlfol. */
    if(command!='2' && command!='4' && command!='6' && command!='8') {
        if(command=='D' || command=='M' || command=='L' || command=='O' || command=='P' || command=='E')
            tera_presentation_message(g,TERA_SOURCE_ALFOL,TERA_TEXT_COMMAND_RESPONSE,command,draw(s,3));
        return 1;
    }
    if(g->world_id==11) {
        int old_position=g->position;
        if(g->position<0 || g->position>99) return 0;
        direction=command=='8' ? 1 : command=='2' ? -1 : command=='6' ? 10 : -10;
        g->location_changed=0;g->position=word16(g->position+direction);
        if(g->position>99) g->position=word16(g->position-100);
        if(g->position<0) g->position=word16(g->position+100);
        s->moved=1;tera_presentation_move(g,TERA_SOURCE_ALFOL,command,old_position,1,~0u);surface(s);
    } else {
        if(g->position<900 || g->position>999) return 0;
        heading=word16(g->position-g->previous_position);direction=heading;
        if(command=='2') direction=-heading;
        if(command=='6') direction=heading==1 ? 10 : heading==-1 ? -10 : heading==10 ? -1 : 1;
        if(command=='4') direction=heading==1 ? -10 : heading==-1 ? 10 : heading==10 ? 1 : -1;
        if(heading!=1 && heading!=-1 && heading!=10 && heading!=-10) return 1;
        s->delta=(int16_t)direction;s->wall=direction==1 ? 0 : direction==-1 ? 2 : direction==10 ? 3 : 1;
        direction=s->state.world.maps[g->position][s->wall];
        if(direction==1) grid_step(s);
        else if(direction==2 || (direction==3 && s->state.world.characters.keys[9]!=10)) s->phase=TERA_ALFOL_GATE;
        else tera_presentation_move(g,TERA_SOURCE_ALFOL,command,g->position,0,s->wall);
    }
    return 1;
}
int tera_alfol_key(TeraAlfolSession *s,unsigned key)
{
    TeraCombatGame *g;unsigned i;
    if(!s || key>255 || s->phase>=TERA_ALFOL_DONE) return 0;
    if(key>='a' && key<='z') key-=32;
    g=&s->state.world.characters.combat;
    if(s->phase==TERA_ALFOL_WORD) {
        if(key==8 && s->word_length) s->word[--s->word_length]=0;
        else if(key>='A' && key<='Z') {
            if(s->word_length<9) s->word[s->word_length]=(char)key;
            ++s->word_length;s->word[s->word_length<10 ? s->word_length : 9]=0;
        }
        if(key==13 || s->word_length==10) {
            s->status_actor=9; /* B590..B5C7 scanne les dix villes avant le refus OnAlfol. */
            s->phase=TERA_ALFOL_DONE;
        }
        return 1;
    }
    if(s->phase==TERA_ALFOL_DAY) {
        if(s->fixed_combat) {if(!tera_combat_key(&s->combat,key)) return 0;}
        else if(!tera_day_key(&s->day,key)) return 0;
        finish_day(s);return 1;
    }
    if(s->phase==TERA_ALFOL_MEDIA) {
        if(key=='O') {
            if(g->money>999) {
                g->money=word16(g->money-1000);
                if(!s->state.world.tasks_done[1]) {
                    s->state.world.tasks_done[1]=1;s->state.world.tasks_counter=word16(s->state.world.tasks_counter+1);
                    tera_presentation_objective(g,TERA_SOURCE_ALFOL,1);
                    if(s->state.sound) tera_sound_objective(g);
                    else tera_media_wait(g,2000);
                }
                tera_presentation_interaction(g,TERA_SOURCE_ALFOL,TERA_INTERACTION_MEDIA_PAID,1000);
            } else {
                tera_presentation_interaction(g,TERA_SOURCE_ALFOL,TERA_INTERACTION_TOO_POOR,0);
                if(s->state.sound) {
                    tera_sound_refusal(g);
                }
            }
        }
        after_media(s);
    } else if(s->phase==TERA_ALFOL_REBOARD) {
        s->phase=g->position==0 ? TERA_ALFOL_RETURN : TERA_ALFOL_DONE;
    } else if(s->phase==TERA_ALFOL_ENTER) {
        s->phase=TERA_ALFOL_DONE;
        if(key=='O' || key==13) {
            int old_position=g->position;
            g->position=900;g->previous_position=899;g->world_id=9;s->initial_render=1;
            tera_presentation_zone(g,TERA_SOURCE_ALFOL,11,old_position,g->on_alfol);building(s);
        } else tera_presentation_message(g,TERA_SOURCE_ALFOL,TERA_TEXT_COMMAND_RESPONSE,'E',draw(s,3));
    } else {
        i=s->state.world.maps[g->position][s->wall];s->phase=TERA_ALFOL_DONE;
        if((i==2 && key=='O') || (i==3 && key=='U')) {
            if(i==3) for(i=0;i<4;++i) if(s->state.world.maps[g->position][i]==3) s->state.world.maps[g->position][i]=2;
            grid_step(s);
        } else tera_presentation_move(g,TERA_SOURCE_ALFOL,s->command,g->position,0,s->wall);
    }
    return 1;
}
