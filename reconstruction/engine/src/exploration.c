#include "tera/exploration.h"
#include "tera/sound.h"
#include "tera/recruitment.h"
#include <stddef.h>
#include <string.h>

static int16_t word16(int32_t value)
{
    uint32_t bits=(uint32_t)value&65535u;
    return (int16_t)(bits<32768u ? (int32_t)bits : (int32_t)bits-65536);
}

static unsigned draw(TeraCombatGame *game, unsigned bound, unsigned *calls)
{
    unsigned value=0;
    tera_random_draw(&game->random,bound,&value);
    ++*calls;
    return value;
}

/* BD0C, C0EC et 6C4F : les coordonnees sont du rendu, mais leurs tirages
 * changent la prochaine rencontre. Conserver les boucles et leurs bornes. */
static void horizon(TeraCombatGame *game,unsigned *calls,TeraLandscape *landscape)
{
    unsigned i;
    for(i=0;i<16;++i) {
        landscape->stars[i].x=10+draw(game,300,calls);
        landscape->stars[i].y=4+draw(game,70,calls);
    }
    landscape->horizon_kind=draw(game,2,calls);
    landscape->horizon_height=draw(game,20,calls);
}

int tera_exploration_move(TeraExplorationState *state,unsigned key,TeraMoveReport *report)
{
    static const unsigned ground_blocks[23]={5,5,5,5,5,3,4,5,5,5,5,5,5,4,5,5,5,5,3,5,5,5,5};
    TeraExplorationState next;
    TeraMoveReport r={0};
    TeraWorldState *world;
    TeraCombatGame *game;
    int position,origin,delta,blocked=0,near=0;
    unsigned i,j;
    if(!state || !report || state->world.characters.combat.world_id!=11 ||
        state->world.characters.combat.on_alfol ||
        state->world.characters.combat.position<-1 || state->world.characters.combat.position>1899 ||
        (key!='2' && key!='4' && key!='6' && key!='8')) return 0;
    next=*state;world=&next.world;game=&world->characters.combat;
    game->location_changed=0; /* AFD1, aussi efface quand le pas est bloque. */
    position=game->position;
    origin=position;
    delta=key=='8' ? 1 : key=='2' ? -1 : key=='6' ? 30 : -30;
    for(i=0;i<(key=='8' || key=='2' ? 9u : 25u);++i) {
        int mountain=key=='8' || key=='2' ? world->horizontal_mountains[i] : world->vertical_mountains[i];
        blocked |= mountain == position+(delta<0 ? delta : 0);
    }
    next.moved=!blocked;r.moved=!blocked;
    if(!blocked) {
        position+=delta;
        if(position>1899) position-=900;
        if(delta<0 && position<1000) position+=900;
        game->position=(int16_t)position;game->recruit_ready=0;
        if(next.sound) {
            unsigned sound_choice;
            r.random_calls+=tera_sound_steps(game,world->fast_moves);
            sound_choice=draw(game,24,&r.random_calls);
            if(sound_choice>=1 && sound_choice<=3)
                r.random_calls+=tera_sound_country(game,sound_choice==2 ? 4 : sound_choice==3 ? 3 : 1,
                    sound_choice==2 ? 3 : sound_choice==3 ? 2 : 1);
        }
        next.town_found=0;
        for(i=0;i<20;++i) if(world->towns[i]==position) {next.town_found=1;next.town_id=i;}
        for(i=0;i<(key=='8' || key=='2' ? 9u : 25u);++i) {
            int mountain=key=='8' || key=='2' ? world->horizontal_mountains[i] : world->vertical_mountains[i];
            near |= mountain == position+(delta<0 ? delta : 0);
        }
        r.near_mountain=near;r.scene=(unsigned)position%23;
        if(!next.town_found || next.town_id>9) {
            r.landscape.valid=1;r.landscape.position=position;r.landscape.command=key;
            if(near) {
                horizon(game,&r.random_calls,&r.landscape);
                for(i=0;i<7;++i) {
                    r.landscape.mountains[r.landscape.mountain_count++]=(unsigned short)draw(game,3,&r.random_calls);
                    /* Deux extremites (x,y), chacune avec deux RANDOM4. */
                    for(j=0;j<4;++j) r.landscape.mountains[r.landscape.mountain_count++]=(unsigned short)draw(game,4,&r.random_calls);
                }
            }
            for(i=0;i<ground_blocks[r.scene];++i)
                for(j=0;j<13;++j) {
                    unsigned n=r.landscape.count++;
                    r.landscape.ground[n].x=draw(game,76,&r.random_calls);
                    r.landscape.ground[n].y=148+2*draw(game,20,&r.random_calls);
                    r.landscape.ground[n].sprite=j<4 ? 0x14 : j<8 ? 0x35 : 0x36;
                }
            if(!near) horizon(game,&r.random_calls,&r.landscape);
        }
        /* 7309 appelle 724A : 12x4 fenetres, RANDOM2 puis RANDOM3,
         * meme quand la facade est uniquement annoncee au terminal. */
        if(next.town_found && next.town_id>9) {
            TeraPresentationEvent e=tera_presentation_make(game,TERA_PRESENT_SCENERY,TERA_SOURCE_EXPLORATION);
            e.data.scenery.kind=TERA_PICTURE_FACADE;
            for(i=0;i<48;++i) {
                e.data.scenery.values[e.data.scenery.count++]=(unsigned short)draw(game,2,&r.random_calls);
                e.data.scenery.values[e.data.scenery.count++]=(unsigned short)draw(game,3,&r.random_calls);
            }
            tera_presentation_emit(game,&e);
        }
        for(i=1;i<10;++i)
            if(i!=3 && i!=5 && i!=8 && world->extra_locations[i]==position) {
                next.information_context=1; /* C6073 precede la presentation du PNJ. */
                r.npc_variant=draw(game,3,&r.random_calls);
                r.npc_present=1;
            }
    }
    *state=next;*report=r;
    tera_presentation_move(&state->world.characters.combat,TERA_SOURCE_EXPLORATION,
        key,origin,r.moved,~0u);
    if(r.moved && next.town_found) {
        TeraPresentationEvent e=tera_presentation_make(&state->world.characters.combat,TERA_PRESENT_PLACE,TERA_SOURCE_EXPLORATION);
        e.data.place.id=next.town_id;tera_presentation_emit(&state->world.characters.combat,&e);
    }
    return 1;
}

static int apt(const TeraRoundActor *actor)
{
    return actor->active && strcmp(actor->condition,"HS")!=0;
}

/* 9B4E : presentation avant C838A, avec ses vrais tirages. Le type 15
 * anime six images quand aucun clavier n'interrompt la presentation. */
int tera_monster_presentation_random(TeraCombatGame *game,unsigned type,unsigned *calls)
{
    unsigned i;TeraPresentationEvent picture;
    if(!game || !calls || type<1 || type>22) return 0;
    picture=tera_presentation_make(game,TERA_PRESENT_MONSTER,TERA_SOURCE_COMBAT);
    picture.data.monster.type=type;
#define PICTURE_DRAW(bound) picture.data.monster.values[picture.data.monster.count++]=(unsigned short)draw(game,bound,calls)
    if(type==1) {PICTURE_DRAW(38);}
    if(type==2) {PICTURE_DRAW(16);}
    if(type==3 || type==9)
        for(i=0;i<11;++i) {PICTURE_DRAW(40);PICTURE_DRAW(4);}
    if(type==6)
        for(i=0;i<3;++i) {PICTURE_DRAW(40);PICTURE_DRAW(6);}
    if(game->sound && (type==3 || type==6 || type==9))
        *calls+=tera_sound_noise(game,60,100,60,12,14,8000,10000);
    if(type==15) {
        /* BIN 7872 : vider le clavier avant la premiere image. 792E
         * teste KEYPRESSED apres CHAQUE image, sixieme comprise. La touche
         * reste tamponnee ; C4210 la purgera avant la lecture de combat. */
        tera_input_boundary(game->input,TERA_INPUT_PURGE,0);
        for(i=0;i<6;++i) {
            PICTURE_DRAW(60);PICTURE_DRAW(24);PICTURE_DRAW(255);
            /* C78EA..790C : afficher puis DELAY(700+RANDOM255),
             * meme frequence pendant 20 ms ; tester le clavier APRES. */
            tera_presentation_emit(game,&picture);
            {unsigned duration=700+picture.data.monster.values[picture.data.monster.count-1];
             tera_media_wait(game,duration);tera_media_tone(game,duration,20);}
            if(tera_input_boundary(game->input,TERA_INPUT_MONSTER15_FRAME,i+1)) break;
        }
    }
    if(type==21)
        for(i=0;i<120;++i) {PICTURE_DRAW(42);PICTURE_DRAW(110);}
    if(game->sound && (type==17 || type==21))
        *calls+=tera_sound_alert(game);
    if(game->sound && (type==16 || type==18 || type==19 || type==20)) {
        /* C693B : deux rafales C65B5, puis la descente C6179. */
        tera_sound_burst(game,100);tera_media_tone(game,165,250);
        tera_sound_burst(game,100);tera_sound_sweep(game,250,20,5,15);
        *calls+=247;
    }
    tera_presentation_emit(game,&picture);
#undef PICTURE_DRAW
    return 1;
}

static void refresh_group(TeraDaySession *s)
{
    TeraCombatGame *game=&s->state.world.characters.combat;
    unsigned id,slot;
    game->encounter.party_available=0; /* A222, avant la regeneration. */
    for(id=0;id<10;++id) if(apt(&game->encounter.actors[id])) {
        TeraRoundActor *actor=&game->encounter.actors[id];
        TeraCombatLimits limits;
        int16_t roll;
        int healing_item=0;
        for(slot=0;slot<10;++slot) healing_item |= game->equipment[id][slot]==22;
        tera_combat_actor_limits(&game->encounter.attributes[id],&limits);
        actor->hp_limit=limits.hp;actor->magic_limit=limits.magic;
        /* Pascal AND/OR ne court-circuite pas : deux jets pour chaque apte,
         * meme sans blessure, sans poison, ou avec objet de regeneration. */
        tera_combat_base_roll(&game->encounter.attributes[id],6,6,
            draw(game,15,&s->random_calls)+3,&roll);
        if(actor->hp<actor->hp_limit && (healing_item || roll>0)) actor->hp=word16(actor->hp+1);
        if(actor->magic<actor->magic_limit) actor->magic=word16(actor->magic+1);
        tera_combat_base_roll(&game->encounter.attributes[id],6,6,
            draw(game,15,&s->random_calls)+3,&roll);
        if(strcmp(actor->condition,"EM")==0 && roll<1) actor->hp=word16(actor->hp-1);
        if(tera_combat_update_actor_condition(actor)) game->encounter.party_available=1;
    }
}

static int level_group(TeraDaySession *s)
{
    TeraCharacterState *characters=&s->state.world.characters;
    TeraCombatGame *game=&characters->combat;
    int16_t denominator=word16(30+10*(int32_t)characters->level+5*(int32_t)game->difficulty);
    unsigned id,skill,stat,i,learned[9]={0},any_skill=0,companion=0;
    int32_t quotient;
    if(!denominator) return 0; /* IDIV serait une erreur DOS, pas un nouveau niveau. */
    quotient=game->experience/denominator;
    if(quotient>32767 || quotient<-32768) return 0;
    if(quotient<=characters->level) return 1;
    characters->level=word16(characters->level+1);s->level_advanced=1;
    for(id=0;id<10;++id) if(apt(&game->encounter.actors[id])) {
        if(id) companion=1;else characters->dummy_level=word16(characters->dummy_level+1);
        stat=draw(game,9,&s->random_calls)+1;
        if(game->encounter.attributes[id].stats[stat]<11) ++game->encounter.attributes[id].stats[stat];
    }
    if(!companion) return 1;
    for(i=1;i<10;++i) {
        for(id=0;id<10;++id) if(game->encounter.actors[id].active && game->encounter.attributes[id].skills[i]>0) learned[i-1]=1;
        any_skill |= learned[i-1];
    }
    if(!any_skill) return 0; /* Original boucle sans fin : erreur d'integration explicite. */
    for(id=0;id<10;++id) if(apt(&game->encounter.actors[id])) {
        do {skill=draw(game,9,&s->random_calls)+1;} while(!learned[skill-1]);
        if(game->encounter.attributes[id].skills[skill]<5) ++game->encounter.attributes[id].skills[skill];
        /* Chaque membre voit aussi les competences acquises par les precedents. */
        learned[skill-1]=1;
    }
    return 1;
}

static void normalize_group(TeraDaySession *s)
{
    unsigned id;
    TeraEncounterState *e=&s->state.world.characters.combat.encounter;
    e->party_available=0;
    for(id=0;id<10;++id) if(e->actors[id].active && tera_combat_update_actor_condition(&e->actors[id])) e->party_available=1;
}

static int begin_encounter(TeraDaySession *s,unsigned type)
{
    s->state.information_context=1; /* C9B55 appelle C6073 pour le monstre. */
    TeraCombatGame *game=&s->state.world.characters.combat;
    game->sound=s->state.sound;
    tera_monster_presentation_random(game,type,&s->random_calls);
    s->state.encounter_suppressed=1;game->encounter.monster_type=type;++s->encounters;
    if(!tera_combat_start(&s->combat,game,0,s->combat.observe,s->combat.context)) {s->phase=TERA_DAY_ERROR;return 0;}
    s->phase=TERA_DAY_COMBAT;
    return 1;
}

/* Le calendrier reste suspendu au combat : les jours/soins/niveaux utilisent
 * sa graine et ses mutations finales, jamais une copie calculee d'avance. */
static void advance_day(TeraDaySession *s)
{
    TeraWorldState *world=&s->state.world;
    TeraCharacterState *characters=&world->characters;
    TeraCombatGame *game=&characters->combat;
    unsigned jet,type,i,available;
    int allowed,position,bound;
    for(;;) {
        if(s->phase==TERA_DAY_COMBAT) {
            if(s->combat.phase==TERA_COMBAT_ERROR) {s->phase=TERA_DAY_ERROR;return;}
            if(s->combat.phase!=TERA_COMBAT_DONE) return;
            *game=s->combat.game;s->phase=TERA_DAY_RUNNING;
            if(s->fixed_encounter_pending) {
                s->fixed_encounter_pending=0;
                if(game->encounter.victory && game->position>=0 && game->position<1100) {
                    world->maps[game->position][5]=0;
                    if(game->position==world->arioch_position && s->state.navigation_overlay!=2) {
                        int newly_completed=!world->tasks_done[2];
                        if(s->state.navigation_overlay && !world->tasks_done[2]) {
                            world->tasks_counter=word16(world->tasks_counter+1);
                            if(s->state.sound) {
                                s->random_calls+=tera_sound_objective(game);
                                s->random_calls+=tera_sound_alert(game);
                            }
                        }
                        world->tasks_done[2]=1;
                        if(newly_completed) tera_presentation_objective(game,TERA_SOURCE_EXPLORATION,2);
                    }
                }
            }
        }
        if(s->stage==0) {
            s->stage=1;position=game->position;
            available=tera_recruit_available(world)&0x7eu; /* IDs1..6 seulement. */
            jet=draw(game,20-game->difficulty,&s->random_calls);
            if(position>0 && position<900 && position!=800 && !available &&
                world->maps[position][4]==255 && game->world_id!=11 && !s->state.encounter_suppressed && jet==0) {
                type=draw(game,17,&s->random_calls)+1;
                /* BIN A1C3..A1D6 : ensemble [12..14,16], pas [12,14,16].
                 * C1515 construit un INTERVALLE : le type 13 est exclu
                 * lui aussi, apres consommation du R17. */
                if((type<12 || type>14) && type!=16) {if(!begin_encounter(s,type)) return;continue;}
            }
        } else if(s->stage==1) {
            s->stage=2;s->state.encounter_suppressed=0;
            jet=draw(game,7,&s->random_calls);
            if(game->world_id==11 || jet==1 || s->state.force_day) {
                characters->days=word16(characters->days+1);s->day_advanced=1;s->state.force_day=0;
                refresh_group(s);
                bound=word16(255-game->experience);if(bound<20) bound=20;
                allowed=game->position!=0 && game->position!=world->alfol_position;
                for(i=0;i<20;++i) if(world->towns[i]==game->position) allowed=0;
                for(i=1;i<10;++i) if(world->extra_locations[i]==game->position) allowed=0;
                jet=draw(game,(unsigned)bound,&s->random_calls);
                if(jet==3 && game->world_id==11 && allowed) {
                    if(draw(game,4,&s->random_calls)==1) type=draw(game,4,&s->random_calls)+12;
                    else type=draw(game,7,&s->random_calls)+1;
                    if(!begin_encounter(s,type)) return;
                    continue;
                }
            } else s->stage=3;
        } else if(s->stage==2) {
            s->stage=3;
            if(!level_group(s)) {s->phase=TERA_DAY_ERROR;return;}
        } else {
            normalize_group(s);s->phase=TERA_DAY_DONE;return;
        }
    }
}

int tera_day_start(TeraDaySession *session,const TeraExplorationState *state,
                   TeraCombatObserver observer,void *context)
{
    TeraExplorationState copy;
    if(!session || !state || state->world.characters.combat.difficulty>10 || state->world.characters.combat.world_id>11) return 0;
    copy=*state;memset(session,0,sizeof *session);session->state=copy;
    session->combat.observe=observer;session->combat.context=context;
    advance_day(session);return 1;
}

int tera_day_key(TeraDaySession *session,unsigned key)
{
    if(!session || session->phase!=TERA_DAY_COMBAT || !tera_combat_key(&session->combat,key)) return 0;
    advance_day(session);return 1;
}


int tera_day_start_location(TeraDaySession *session,const TeraExplorationState *state,
                            TeraCombatObserver observer,void *context)
{
    TeraExplorationState copy;
    unsigned type;
    if(!session || !state || state->world.characters.combat.position<0 ||
        state->world.characters.combat.position>=900 || state->world.characters.combat.world_id>8) return 0;
    copy=*state;memset(session,0,sizeof *session);session->state=copy;
    session->combat.observe=observer;session->combat.context=context;
    type=copy.world.maps[copy.world.characters.combat.position][5];
    if(type) {
        if(type>22) {session->phase=TERA_DAY_ERROR;return 1;}
        session->fixed_encounter_pending=1;
        if(!begin_encounter(session,type)) return 1;
    }
    advance_day(session);
    return 1;
}
