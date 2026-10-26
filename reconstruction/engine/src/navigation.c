#include "tera/navigation.h"
#include "tera/sound.h"
#include "tera/recruitment.h"
#include <stddef.h>
#include <string.h>

static int16_t word16(int32_t value)
{
    uint32_t bits=(uint32_t)value&65535u;
    return (int16_t)(bits<32768 ? (int32_t)bits : (int32_t)bits-65536);
}

static unsigned draw(TeraCombatGame *game,unsigned bound)
{
    unsigned result=0;
    tera_random_draw(&game->random,bound,&result);
    return result;
}

/* OVL002E : les deux cotes du decor consomment des jets differents.
 * Aucun jet graphique pour le mur du fond. L'ordre droite/gauche suit
 * strictement le CASE Position-Previous du binaire. */
static void side_random(TeraCombatGame *game,unsigned wall,int right,TeraScenePicture *p)
{
    unsigned i,limit,bound;
    if(wall>3) return;
    limit=wall==1 ? 18 : right ? 18 : 16;
    bound=2+2*draw(game,limit);
    for(i=0;i<3;++i) {
        unsigned x,y,id;static const unsigned sprites[]={0x31,0x37,0x38};
        if(wall==1) {x=(right ? 40 : 0)+draw(game,right ? 37 : 38);y=148+2*draw(game,20);}
        else {unsigned offset=draw(game,bound);x=right ? 40+offset : 40-offset;y=160+bound;}
        /* BC77 remplace D9B68 pour le prochain sprite : conserver ce flux. */
        bound=2+2*draw(game,16);id=sprites[draw(game,3)];
        p->values[p->count++]=(unsigned short)x;p->values[p->count++]=(unsigned short)y;p->values[p->count++]=(unsigned short)id;
    }
}

static void scriptures_random(TeraCombatGame *g,unsigned variant)
{
    unsigned i;TeraPresentationEvent e=tera_presentation_make(g,TERA_PRESENT_SCENERY,TERA_SOURCE_NAVIGATION);
    e.data.scenery.kind=TERA_PICTURE_FACADE;e.data.scenery.variant=variant;
    for(i=0;i<48;++i) {
        e.data.scenery.values[e.data.scenery.count++]=(unsigned short)draw(g,2);
        e.data.scenery.values[e.data.scenery.count++]=(unsigned short)draw(g,3);
    }
    tera_presentation_emit(g,&e);
}
static void consume_scene(TeraNavigationSession *s)
{
    TeraWorldState *w=&s->state.world;
    TeraCombatGame *g=&w->characters.combat;
    int heading=word16(g->position-g->previous_position);
    unsigned left,right;
    TeraPresentationEvent picture=tera_presentation_make(g,TERA_PRESENT_SCENERY,TERA_SOURCE_NAVIGATION);
    picture.data.scenery.kind=TERA_PICTURE_RUINS;
    if(s->state.navigation_overlay==2) {
        /* BA2F -> C724A : l'autel porte 48 petites inscriptions. */
        if(w->maps[g->position][4]==110)
            scriptures_random(g,1);
        return;
    }
    if(!s->state.navigation_overlay) {
        if(heading==1 || heading==-1) {
            right=heading==1 ? 3 : 1;left=heading==1 ? 1 : 3;
            side_random(g,w->maps[g->position][right],1,&picture.data.scenery);
            side_random(g,w->maps[g->position][left],0,&picture.data.scenery);
        } else if(heading==10 || heading==-10) {
            left=heading==10 ? 0 : 2;right=heading==10 ? 2 : 0;
            if(heading==10) {side_random(g,w->maps[g->position][left],0,&picture.data.scenery);side_random(g,w->maps[g->position][right],1,&picture.data.scenery);}
            else {side_random(g,w->maps[g->position][right],1,&picture.data.scenery);side_random(g,w->maps[g->position][left],0,&picture.data.scenery);}
        }
    } else if(w->maps[g->position][4]>103 && w->maps[g->position][4]!=255) {
        /* BIN OVL0049 BA2F..BAB9 : contenus 104..113 -> C724A ; au-dela,
         * C7309 rappelle aussi C724A. Les DEUX dessins font 48 paires
         * R2/R3 avant le monstre fixe, meme sans son. 255 reste vide. */
        scriptures_random(g,w->maps[g->position][4]>113 ? 2 : 1);
    }
    tera_presentation_emit(g,&picture);
}

static void render(TeraNavigationSession *s);

static void finish_day(TeraNavigationSession *s)
{
    TeraCombatGame *g;
    int rendering=!s->invocation_pending;
    if(s->invocation_pending) {
        if(s->day.combat.phase==TERA_COMBAT_ERROR) {s->phase=TERA_NAV_ERROR;return;}
        if(s->day.combat.phase!=TERA_COMBAT_DONE) return;
        s->state.world.characters.combat=s->day.combat.game;
        if(s->invocation_pending==2 && s->day.combat.game.encounter.victory && !s->state.world.tasks_done[3]) {
            s->state.world.tasks_done[3]=1;
            s->state.world.tasks_counter=word16(s->state.world.tasks_counter+1);
            tera_presentation_objective(&s->state.world.characters.combat,TERA_SOURCE_NAVIGATION,3);
            if(s->state.sound) tera_sound_objective(&s->state.world.characters.combat);
        }
        s->invocation_pending=0;
    } else {
    if(s->day.phase==TERA_DAY_ERROR) {s->phase=TERA_NAV_ERROR;return;}
    if(s->day.phase!=TERA_DAY_DONE) return;
    s->state=s->day.state;g=&s->state.world.characters.combat;
    /* Le AND Pascal consomme RANDOM18 meme si le son est coupe. */
    if(s->state.navigation_overlay==2) {
        /* CB3F : RANDOM12 meme sans son et meme a l'entree 800. */
        if(draw(g,12)==3 && s->state.sound && g->position!=800) tera_sound_alert(g);
    } else if(draw(g,18)==4 && s->state.sound) tera_sound_alert(g);
    }
    g=&s->state.world.characters.combat;
    s->phase=TERA_NAV_DONE;
    if(s->state.navigation_overlay==2) {
        TeraRoundActor *actor=&g->encounter.actors[5];
        if(rendering && g->position==800) {
            unsigned i;
            for(i=0;i<48;++i) {draw(g,2);draw(g,3);} /* CBAC : apres le calendrier. */
            if(s->state.world.locations[5]!=800 || (!actor->active && strcmp(actor->condition,"HS")!=0))
                s->state.information_context=1;
        }
        if(!g->encounter.party_available || g->position==799) {
            /* CC23 precede REPEAT : meme une entree mortelle attend
             * encore la premiere commande avant le test UNTIL. */
            if(s->temple_entry_render && g->position!=799) {s->mandatory_temple_command=1;return;}
            unsigned old_world=g->world_id;int old_position=g->position;
            g->position=s->backup_position;g->world_id=11;
            tera_presentation_zone(g,TERA_SOURCE_NAVIGATION,old_world,old_position,g->on_alfol);return;
        }
    }
    /* La boucle de l'overlay traite FleeingFlag avant de demander une
     * nouvelle commande. Redessiner la case de fuite sans avancer encore. */
    if(g->encounter.fled && g->encounter.party_available) {
        g->encounter.fled=0;
        if(!s->state.navigation_overlay && s->state.world.maps[g->position][4]==101 &&
            word16(g->position-g->previous_position)==1) {
            if(s->state.sound) tera_sound_alert(g);
            unsigned old_world=g->world_id;int old_position=g->position;
            s->state.navigation_overlay=1;g->world_id=1;g->position=100;g->previous_position=99;
            tera_presentation_zone(g,TERA_SOURCE_NAVIGATION,old_world,old_position,g->on_alfol);
            s->changed_level=1;
        }
        render(s);
    }
}

static void render(TeraNavigationSession *s)
{
    TeraCombatGame *g=&s->state.world.characters.combat;
    if(g->position<0 || g->position>=900) {s->phase=TERA_NAV_ERROR;return;}
    s->render_world=g->world_id;
    s->state.chest_open=0; /* BP_03 n'est pas une propriete permanente de la case. */
    /* A74D vient de la generation du monde : meme un pas silencieux du
     * terminal conserve les tirages des pas sonores de C67D7. */
    if(s->state.sound) tera_sound_steps(g,s->state.world.fast_moves);
    consume_scene(s);
    /* C6C6/C6E5/C700 et C7E2/C801/C81C : les sprites des
     * compagnons disponibles activent A750, lu ensuite par N/I. */
    if(s->state.navigation_overlay!=2 && (tera_recruit_available(&s->state.world)&((1u<<1)|(1u<<2)|(1u<<6))))
        s->state.information_context=1;
    if(!tera_day_start_location(&s->day,&s->state,s->observe,s->context)) {s->phase=TERA_NAV_ERROR;return;}
    s->phase=TERA_NAV_DAY;
    finish_day(s);
}

/* OVL002E a l'entree Position=1 : SoundFx6(400), puis (600), tous deux
 * tirent RANDOM255 pour chaque son. Son inaudible ne signifie pas RNG nul. */
static void entrance_random(TeraNavigationSession *s)
{
    if(!s->state.navigation_overlay && s->state.sound && s->state.world.characters.combat.position==1)
    {
        TeraCombatGame *g=&s->state.world.characters.combat;
        /* CBAD/C6496 -> C62B7 : melodie avant les deux rafales C65B5.
         * Seules les frequences different de la victoire, pas les tirages. */
        tera_sound_melody(g,1,1,1,1);
        tera_sound_burst(g,400);tera_media_wait(g,120);
        tera_sound_burst(g,600);tera_media_wait(g,180);
        tera_sound_melody(g,1,1,9,8); /* CBD2, apres les deux rafales. */
    }
}

static void take_step(TeraNavigationSession *s)
{
    TeraCombatGame *g=&s->state.world.characters.combat;
    int16_t old=g->position;
    g->position=word16(g->position+s->delta);g->previous_position=old;
    s->moved=1; /* A742, sans changer A749 de la grande carte. */
    tera_presentation_move(g,TERA_SOURCE_NAVIGATION,s->key,old,1,s->wall);
    if(s->state.navigation_overlay==2 && g->position==799) {
        unsigned old_world=g->world_id;int old_position=g->position;
        g->position=s->backup_position;g->world_id=11;s->phase=TERA_NAV_DONE;
        tera_presentation_zone(g,TERA_SOURCE_NAVIGATION,old_world,old_position,g->on_alfol);return;
    }
    /* Les sentinelles terminent l'overlay avant son rendu. */
    if(!s->state.navigation_overlay && g->position==0) {s->phase=TERA_NAV_DONE;return;}
    if(s->state.navigation_overlay && g->position==99) {
        unsigned i,old_world=g->world_id;int old_position=g->position;
        g->world_id=0;
        for(i=0;i<100;++i) if(s->state.world.maps[i][4]==101) g->position=(int16_t)i;
        g->previous_position=word16(g->position+1);s->state.navigation_overlay=0;s->changed_level=1;
        tera_presentation_zone(g,TERA_SOURCE_NAVIGATION,old_world,old_position,g->on_alfol);
        entrance_random(s);
    }
    render(s);
}

static int initialize(TeraNavigationSession *s,const TeraExplorationState *state,
                       int16_t backup,TeraCombatObserver observer,void *context)
{
    TeraExplorationState copy;
    if(!s || !state || state->world.characters.combat.position<0 ||
        state->world.characters.combat.position>=900 || state->world.characters.combat.world_id>8) return 0;
    copy=*state;memset(s,0,sizeof *s);s->state=copy;
    s->phase=TERA_NAV_DONE;s->backup_position=backup;s->state.city_backup_position=backup;s->observe=observer;s->context=context;
    return 1;
}

int tera_navigation_start(TeraNavigationSession *s,const TeraExplorationState *state,
                           unsigned key,int initial_move,int16_t backup,
                           TeraCombatObserver observer,void *context)
{
    TeraCombatGame *g;
    int heading,direction;
    unsigned content;
    if(key!='2' && key!='4' && key!='6' && key!='8' && key!='D' && key!='M' && key!='I' && key!='L' && key!='O' && key!='P' && key!=0) return 0;
    if(initial_move!=0 && initial_move!=1) return 0;
    if(!initialize(s,state,backup,observer,context)) return 0;
    if(key=='O' || key=='P') {
        if(!tera_chest_start(&s->chest,&s->state,key)) return 0;
        if(s->chest.phase==TERA_CHEST_DONE) s->state=s->chest.state;
        else s->phase=TERA_NAV_CHEST;
        return 1;
    }
    g=&s->state.world.characters.combat;s->key=key;content=s->state.world.maps[g->position][4];
    heading=word16(g->position-g->previous_position);
    if(key==0 || initial_move) {
        s->temple_entry_render=initial_move && s->state.navigation_overlay==2 && g->position==800;
        s->moved=1;if(key==0) entrance_random(s);render(s);return 1;
    }
    if(key=='D' || key=='M' || key=='L' || key=='I') {
        if(s->state.navigation_overlay==2) {
            if((key=='L' && (g->position==800 || content==110)) || (key=='I' && content==110)) s->phase=TERA_NAV_CONFIRM;
        } else if((key=='D' && s->state.navigation_overlay && g->world_id!=0 && content==101) || (key=='M' && s->state.navigation_overlay && content==100 && g->world_id>0) ||
           (key=='L' && s->state.navigation_overlay && content>103 && content!=255) ||
           (key=='I' && s->state.navigation_overlay && content==103)) s->phase=TERA_NAV_CONFIRM;
        if(s->phase==TERA_NAV_DONE) {
            if(key=='I') s->phase=TERA_NAV_WORD;
            else tera_presentation_message(g,TERA_SOURCE_NAVIGATION,TERA_TEXT_COMMAND_RESPONSE,key,draw(g,3));
        }
        return 1;
    }
    /* OVL002E : le portail est teste AVANT C7EF6, a partir du sens courant. */
    if(!s->state.navigation_overlay && content==101 &&
       ((key=='8' && heading==1) || (key=='6' && heading==-10) ||
        (key=='4' && heading==10) || (key=='2' && heading==-1))) {
        if(s->state.sound) tera_sound_alert(g); /* D371, avant le changement d'overlay. */
        unsigned old_world=g->world_id;int old_position=g->position;
        s->state.navigation_overlay=1;g->world_id=1;g->position=100;g->previous_position=99;s->changed_level=1;s->moved=1;
        tera_presentation_zone(g,TERA_SOURCE_NAVIGATION,old_world,old_position,g->on_alfol);
        render(s);return 1;
    }
    if(heading!=1 && heading!=-1 && heading!=10 && heading!=-10) {
        tera_presentation_move(g,TERA_SOURCE_NAVIGATION,key,g->position,0,0);return 1;
    }
    direction=heading;
    if(key=='2') direction=-heading;
    if(key=='6') direction=heading==1 ? 10 : heading==-1 ? -10 : heading==10 ? -1 : 1;
    if(key=='4') direction=heading==1 ? -10 : heading==-1 ? 10 : heading==10 ? 1 : -1;
    s->delta=direction;s->wall=direction==1 ? 0 : direction==-1 ? 2 : direction==10 ? 3 : 1;
    content=s->state.world.maps[g->position][s->wall];
    if(content==1) take_step(s);
    else if(content==2 || (content==3 && s->state.world.characters.keys[g->world_id]!=10)) s->phase=TERA_NAV_GATE;
    else tera_presentation_move(g,TERA_SOURCE_NAVIGATION,key,g->position,0,s->wall);
    return 1;
}

int tera_navigation_enter(TeraNavigationSession *s,const TeraExplorationState *state,
                           TeraCombatObserver observer,void *context)
{
    TeraExplorationState copy;
    if(!s || !state || state->world.characters.combat.world_id!=11 || state->world.characters.combat.on_alfol ||
       !state->town_found || (state->town_id!=2 && state->town_id!=4)) return 0;
    copy=*state;memset(s,0,sizeof *s);s->state=copy;
    s->backup_position=copy.city_backup_position;s->observe=observer;s->context=context;
    s->enter_city=state->town_id==4 ? 2 : 1;s->key='E';s->phase=TERA_NAV_CONFIRM;
    return 1;
}

int tera_navigation_leave(TeraExplorationState *state,int16_t backup)
{
    TeraCombatGame *g;
    if(!state || backup<1000 || backup>1899) return 0;
    g=&state->world.characters.combat;
    if(g->world_id>8 || (g->encounter.party_available &&
        (g->position!=0 || g->world_id!=0 || state->navigation_overlay))) return 0;
    {
        unsigned old_world=g->world_id;int old_position=g->position;
        g->position=backup;g->world_id=11;
        tera_presentation_zone(g,TERA_SOURCE_NAVIGATION,old_world,old_position,g->on_alfol);
    }
    return 1;
}

/* C4D46 recalcule les quatre premiers ACTIFS, meme HS. Cet affichage
 * a un effet durable : ne pas l'effacer avec la presentation terminal. */
static void recalculate_visible_limits(TeraCombatGame *game)
{
    unsigned id,shown=0;
    for(id=0;id<10 && shown<4;++id) if(game->encounter.actors[id].active) {
        TeraCombatLimits limits;
        tera_combat_actor_limits(&game->encounter.attributes[id],&limits);
        game->encounter.actors[id].hp_limit=limits.hp;
        game->encounter.actors[id].magic_limit=limits.magic;++shown;
    }
}

static void finish_invocation_word(TeraNavigationSession *s)
{
    TeraCombatGame *g=&s->state.world.characters.combat;
    unsigned i;
    int allowed=!g->on_alfol;
    s->word[9]=0; /* C42AC stocke neuf caracteres apres dix touches. */
    for(i=0;i<10;++i) if(s->state.world.towns[i]==g->position) allowed=0;
    s->phase=TERA_NAV_DONE;
    if((s->state.navigation_overlay==2 && s->key=='I' && s->state.world.maps[g->position][4]==110) ?
       strcmp(s->word,s->state.world.secret_words[3])==0 : (strcmp(s->word,"ARIOCH")==0 && allowed)) {
        s->state.encounter_suppressed=1;g->encounter.monster_type=21;
        s->state.information_context=1; /* B604 -> C9B55 -> C6073. */
        g->sound=s->state.sound;
        memset(&s->day,0,sizeof s->day);s->day.state=s->state;s->day.phase=TERA_DAY_COMBAT;
        tera_monster_presentation_random(g,21,&s->day.random_calls);
        if(!tera_combat_start(&s->day.combat,g,0,s->observe,s->context)) {s->phase=TERA_NAV_ERROR;return;}
        s->invocation_pending=s->state.navigation_overlay==2 && s->state.world.maps[g->position][4]==110 ? 2 : 1;s->phase=TERA_NAV_DAY;
        finish_day(s); /* Aucun jour ni tirage RANDOM18 pour cette commande. */
    }
}

int tera_navigation_key(TeraNavigationSession *s,unsigned key)
{
    TeraCombatGame *g;
    unsigned i,id,content;
    if(!s || key>255 || s->phase==TERA_NAV_DONE || s->phase==TERA_NAV_ERROR) return 0;
    if(key>='a' && key<='z') key-=32;
    if(s->phase==TERA_NAV_CHEST) {
        if(!tera_chest_key(&s->chest,key)) return 0;
        if(s->chest.phase==TERA_CHEST_DONE) {s->state=s->chest.state;s->phase=TERA_NAV_DONE;}
        return 1;
    }
    g=&s->state.world.characters.combat;
    if(s->phase==TERA_NAV_WORD) {
        if(key==13) finish_invocation_word(s);
        else if(key==8 && s->word_length) s->word[--s->word_length]=0;
        else if(key>='A' && key<='Z' && s->word_length<10) {
            s->word[s->word_length++]=(char)key;s->word[s->word_length]=0;
            if(s->word_length==10) finish_invocation_word(s);
        }
        return 1;
    }
    if(s->phase==TERA_NAV_DAY) {
        if(s->invocation_pending) {if(!tera_combat_key(&s->day.combat,key)) return 0;}
        else if(!tera_day_key(&s->day,key)) return 0;
        finish_day(s);return 1;
    }
    if(s->phase==TERA_NAV_GATE) {
        content=s->state.world.maps[g->position][s->wall];
        if((content==2 && key=='O') || (content==3 && key=='U')) {
            if(content==3)
                for(i=0;i<4;++i) if(s->state.world.maps[g->position][i]==3) s->state.world.maps[g->position][i]=2;
            take_step(s);
        } else {
            s->phase=TERA_NAV_DONE;
            tera_presentation_move(g,TERA_SOURCE_NAVIGATION,s->key,g->position,0,s->wall);
        }
        return 1;
    }
    s->phase=TERA_NAV_DONE;
    if(key!='O' && key!=13) {
        if(s->key=='D' || s->key=='M')
            tera_presentation_message(g,TERA_SOURCE_NAVIGATION,TERA_TEXT_COMMAND_RESPONSE,s->key,draw(g,3));
        if(s->key=='I' && s->state.navigation_overlay!=2) s->phase=TERA_NAV_WORD;
        return 1;
    }
    if(s->enter_city) {
        unsigned old_world=g->world_id;int old_position=g->position;
        s->backup_position=g->position;s->state.city_backup_position=g->position;
        s->state.navigation_overlay=s->enter_city==2 ? 2 : 0;
        s->temple_entry_render=s->enter_city==2;
        g->world_id=s->enter_city==2 ? 8 : 0;g->position=s->enter_city==2 ? 800 : 1;
        g->previous_position=(int16_t)(g->position-1);s->moved=1;
        tera_presentation_zone(g,TERA_SOURCE_NAVIGATION,old_world,old_position,g->on_alfol);
        entrance_random(s);render(s);return 1;
    }
    if(s->key=='D') {
        unsigned old_world=g->world_id;int old_position=g->position;
        if(g->world_id>=8) {s->phase=TERA_NAV_ERROR;return 1;}
        ++g->world_id;g->position=(int16_t)(100*g->world_id);g->previous_position=word16(g->position-1);
        tera_presentation_zone(g,TERA_SOURCE_NAVIGATION,old_world,old_position,g->on_alfol);
        s->changed_level=1;s->moved=1;render(s);
    } else if(s->key=='M') {
        unsigned old_world=g->world_id;int old_position=g->position;
        --g->world_id;
        for(i=g->world_id*100;i<g->world_id*100+100;++i) if(s->state.world.maps[i][4]==101) g->position=(int16_t)i;
        g->previous_position=word16(g->position+1);s->changed_level=1;s->moved=1;
        tera_presentation_zone(g,TERA_SOURCE_NAVIGATION,old_world,old_position,g->on_alfol);
        render(s);
    } else if(s->key=='L') {
        content=s->state.world.maps[g->position][4];
        if(s->state.navigation_overlay==2) {s->hint_count=1;s->hints[0]=content==110 ? 101 : 100;}
        else if(content<114) {s->hint_count=2;s->hints[0]=2*(17-g->world_id)-1;s->hints[1]=s->hints[0]+1;}
        else {s->hint_count=1;s->hints[0]=g->world_id==1 ? 0 : g->world_id<5 ? 31+g->world_id : 28+g->world_id;}
        for(i=0;i<s->hint_count;++i) {
            unsigned hint=s->hints[i];
            tera_presentation_message(g,TERA_SOURCE_NAVIGATION,hint>=100 ? TERA_TEXT_SETH : hint ? TERA_TEXT_HINT : TERA_TEXT_MESSAGE,
                hint ? hint : 70,TERA_TEXT_NO_VARIANT);
        }
    } else if(s->key=='I') {
        if(s->state.navigation_overlay==2) {s->phase=TERA_NAV_WORD;return 1;}
        recalculate_visible_limits(g);
        /* Le AND compile tire pour les dix acteurs, meme les inactifs/HS. */
        for(id=0;id<10;++id) {
            int16_t result;
            TeraRoundActor *a=&g->encounter.actors[id];
            tera_combat_base_roll(&g->encounter.attributes[id],6,6,draw(g,15)+3,&result);
            if(a->active && result>0 && strcmp(a->condition,"HS")!=0) {
                TeraCombatLimits limits;
                tera_combat_actor_limits(&g->encounter.attributes[id],&limits);
                a->hp=a->hp_limit=limits.hp;a->magic=a->magic_limit=limits.magic;
                s->healed_mask |= 1u<<id; /* EM est conserve par C4CF9. */
                tera_presentation_actor(g,TERA_SOURCE_NAVIGATION,TERA_EFFECT_HEAL,id);
            }
        }
        recalculate_visible_limits(g);
    }
    return 1;
}
