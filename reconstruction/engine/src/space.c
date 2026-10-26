#include "tera/space.h"
#include "tera/sound.h"
#include <string.h>
/* BIN OVL00AE (AE00:BC00), branche jeu NotDemo=1 ; entree TERAMAIN E222.
 * Le terminal gere le temps/clavier ; ce module seul decide les effets
 * et garde les tirages des etoiles/explosions dans leur ordre original. */

static int16_t word16(int32_t n)
{
    uint32_t bits=(uint32_t)n&65535u;
    return (int16_t)(bits<32768u ? (int32_t)bits : (int32_t)bits-65536);
}
static unsigned draw(TeraSpaceSession *s,unsigned bound)
{
    unsigned value=0;tera_random_draw(&s->state.combat.random,bound,&value);return value;
}
static void explosions(TeraSpaceSession *s,unsigned count)
{
    unsigned i,j,k;
    for(i=0;i<count;++i) for(j=0;j<8;++j) {
        TeraCombatGame *g=&s->state.combat;
        TeraPresentationEvent e=tera_presentation_make(g,TERA_PRESENT_FLASH,TERA_SOURCE_SPACE);
        e.data.flash.palette=draw(s,4);e.data.flash.border=draw(s,16);
        e.data.flash.audio=g->sound!=0;e.data.flash.ms=g->sound ? 32 : 60;
        tera_presentation_emit(g,&e);
        if(g->sound) for(k=0;k<16;++k) tera_media_tone(g,10*draw(s,100),2);
    }
}
static void stars(TeraSpaceSession *s)
{
    unsigned i;TeraPresentationEvent e=tera_presentation_make(&s->state.combat,TERA_PRESENT_SCENERY,TERA_SOURCE_SPACE);
    e.data.scenery.kind=TERA_PICTURE_SPACE;
    for(i=0;i<24;++i) {
        e.data.scenery.values[e.data.scenery.count++]=(unsigned short)draw(s,200);
        e.data.scenery.values[e.data.scenery.count++]=(unsigned short)draw(s,90);
    }
    tera_presentation_emit(&s->state.combat,&e);
}
static void next_trial(TeraSpaceSession *s)
{
    s->delay_ms=100*draw(s,10);
    s->alien_x=(int)draw(s,3)-1;s->alien_y=(int)draw(s,3)-1;
    s->input_ms=0;s->input_stage=0;s->input_key=-1;
    s->phase=TERA_SPACE_SHOT;
}
static void begin_flight(TeraSpaceSession *s)
{
    TeraCombatGame *g=&s->state.combat;
    /* OVL00AE : depart, explosions, annonce des pirates, premier essai.
     * Les sons deterministes n'ajoutent aucun RANDOM. */
    stars(s);
    if(g->sound) tera_sound_hyperspace(g,0);else tera_media_wait(g,2000);
    explosions(s,4);stars(s);
    if(g->sound) tera_sound_wave(g,200,2,200);else tera_media_wait(g,3000);
    s->counter=s->pirates;next_trial(s);
}
int tera_space_start(TeraSpaceSession *s,const TeraCharacterState *state,int16_t pirates)
{
    TeraCharacterState copy;
    if(!s || !state) return 0;
    copy=*state;memset(s,0,sizeof *s);s->state=copy;s->pirates=pirates;s->pilot=-1;s->phase=TERA_SPACE_FUEL;return 1;
}
int tera_space_travel_start(TeraSpaceSession *s,const TeraCharacterState *state,
                            int returning,int16_t pirates,int16_t backup)
{
    if((returning!=0 && returning!=1) || !tera_space_start(s,state,pirates)) return 0;
    s->returning=returning;s->backup_position=backup;
    if(returning) {
        s->state.combat.position=backup;if(s->pirates<4) s->pirates=4;s->departed=1;
    } else s->phase=TERA_SPACE_CONFIRM;
    return 1;
}

/* Seule la cle la plus ancienne est lue dans ce tir ; les suivantes
 * seront purgees au debut du suivant (BFB2/C01E). */
int tera_space_input_at(TeraSpaceSession *s,unsigned elapsed_ms,int key)
{
    unsigned boundary,stage;
    if(!s || s->phase!=TERA_SPACE_SHOT || key< -1 || key>255 ||
       elapsed_ms<s->input_ms || elapsed_ms>s->delay_ms+990) return 0;
    stage=s->input_stage;
    /* Purger avant l'evenement seulement si sa date est APRES la purge :
     * une touche tardive ne doit pas etre effacee retroactivement. */
    while(stage<2) {
        boundary=s->delay_ms+(stage ? 300u : 0u);
        if(boundary>=elapsed_ms) break;
        s->input_key=-1;++stage;
    }
    if(key>=0 && s->input_key<0) s->input_key=key;
    while(stage<2) {
        boundary=s->delay_ms+(stage ? 300u : 0u);
        if(boundary>elapsed_ms) break;
        s->input_key=-1;++stage;
    }
    s->input_stage=stage;s->input_ms=elapsed_ms;
    if(elapsed_ms==s->delay_ms+990)
        return tera_space_key(s,s->input_key<0 ? 0u : (unsigned)s->input_key);
    return 1;
}
int tera_space_key(TeraSpaceSession *s,unsigned key)
{
    unsigned id;int16_t roll,last=0;
    TeraCombatGame *g;
    if(!s || key>255 || s->phase>=TERA_SPACE_DONE) return 0;
    if(key>='a' && key<='z') key-=32;
    g=&s->state.combat;
    if(s->phase==TERA_SPACE_CONFIRM) {
        s->phase=TERA_SPACE_DONE;
        if(key!='O' && key!=13) {
            tera_presentation_message(g,TERA_SOURCE_SPACE,TERA_TEXT_COMMAND_RESPONSE,'E',draw(s,3));return 1;
        }
        for(id=0;id<10;++id) {
            tera_combat_base_roll(&g->encounter.attributes[id],4,4,draw(s,15)+3,&roll);
            if(g->encounter.actors[id].active && strcmp(g->encounter.actors[id].condition,"HS")!=0 && roll>0) {
                s->pilot=(int)id;last=roll;
            }
        }
        if(s->pilot>=0) {
            s->pirates=word16(word16(150-last)/10+g->difficulty);
            if(s->pirates<4) s->pirates=4;
            int old_alfol=g->on_alfol;
            s->backup_position=g->position;g->on_alfol=1;s->departed=1;s->phase=TERA_SPACE_FUEL;
            tera_presentation_zone(g,TERA_SOURCE_SPACE,g->world_id,g->position,old_alfol);
        } else tera_presentation_message(g,TERA_SOURCE_SPACE,TERA_TEXT_COMMAND_RESPONSE,'E',draw(s,3));
    } else if(s->phase==TERA_SPACE_FUEL) {
        if(key=='O') s->phase=TERA_SPACE_DIGIT;else begin_flight(s);
    } else if(s->phase==TERA_SPACE_DIGIT) {
        if(key<'0' || key>'9') return 1;
        roll=(int16_t)(50*(key-'0'));
        if(roll<word16(g->money+1)) {g->money=word16(g->money-roll);s->state.energy=word16(s->state.energy+roll);}
        begin_flight(s);
    } else {
        /* Le compteur baisse deux fois sur un tir juste, une fois sinon.
         * Un essai reste obligatoire avec zero pirate (REPEAT, pas WHILE). */
        s->hit=key>='1' && key<='9' && (int)(key-'1')==3*(1-s->alien_y)+s->alien_x+1;
        if(key>='1' && key<='9') {
            TeraPresentationEvent e=tera_presentation_make(g,TERA_PRESENT_RAY,TERA_SOURCE_SPACE);
            e.data.ray.x=s->hit ? 155+64*s->alien_x : 155+64*((int)((key-'1')%3)-1);
            e.data.ray.y=s->hit ? 90+24*s->alien_y : key<='3' ? 118 : key<='6' ? 90 : 66;
            e.data.ray.ms=g->sound ? 36 : 30;tera_presentation_emit(g,&e);
            if(g->sound) tera_sound_wave(g,6000,6,6);else tera_media_wait(g,30);
        }
        ++s->trials;
        if(s->hit) {s->pirates=word16(s->pirates-1);s->counter=word16(s->counter-1);g->experience=word16(g->experience+3);}
        else {s->state.energy=word16(s->state.energy-5);explosions(s,1);}
        s->counter=word16(s->counter-1);
        if(s->state.energy<5) {
            if(g->sound) tera_sound_burst(g,10000);
            explosions(s,4);s->phase=TERA_SPACE_HALTED;
        } else if(s->counter<1) {
            tera_media_wait(g,1000);
            if(g->sound) tera_sound_hyperspace(g,0);else tera_media_wait(g,2000);
            explosions(s,4);stars(s);
            s->state.energy=s->state.energy>25 ? word16(s->state.energy-25) : 5;
            if(g->sound) tera_sound_hyperspace(g,1);
            s->state.days=word16(s->state.days+7);
            if(s->returning) {
                int old_alfol=g->on_alfol;g->on_alfol=0;
                tera_presentation_zone(g,TERA_SOURCE_SPACE,g->world_id,g->position,old_alfol);
            }
            s->phase=TERA_SPACE_DONE;
        } else next_trial(s);
    }
    return 1;
}
