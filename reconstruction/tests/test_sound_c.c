#include "module_test_support.h"
#include "tera/sound.h"
#include "tera/game.h"
#include "tera/commerce.h"
#include "tera/space.h"
#include "tera/meduz.h"
#include "original_progression_sound_vectors.h"
#include <stdio.h>
static unsigned notes,ms;static unsigned short first_hz,first_ms;
static uint32_t score_hash;
static TeraBattleFrame last_frame;
static void collect(void *ctx,const TeraPresentationEvent *e)
{
    unsigned i;(void)ctx;
    if(e->kind==TERA_PRESENT_COMBAT) last_frame=e->data.combat;
    if(e->kind==TERA_PRESENT_WAIT) {
        ms+=e->data.wait.ms;
        score_hash=(score_hash^0u)*UINT32_C(16777619);
        score_hash=(score_hash^e->data.wait.ms)*UINT32_C(16777619);
    }
    if(e->kind!=TERA_PRESENT_TONES) return;
    assert(e->data.tones.count && e->data.tones.count<=64);
    for(i=0;i<e->data.tones.count;++i) {
        if(!notes) {first_hz=e->data.tones.hz[i];first_ms=e->data.tones.ms[i];}
        ++notes;ms+=e->data.tones.ms[i];
        score_hash=(score_hash^e->data.tones.hz[i])*UINT32_C(16777619);
        score_hash=(score_hash^e->data.tones.ms[i])*UINT32_C(16777619);
    }
}
int main(void)
{
    TeraExplorationState e=module_fixture();TeraCombatGame *g=&e.world.characters.combat;
    TeraPresentationSink sink={collect,NULL};TeraRandom old;unsigned i,value,calls;
    static const char actions[]="TLPC";static const unsigned counts[]={600,2062,31,465};
    for(i=0;i<4;++i) {
        g->random.seed=1986;g->presentation=&sink;notes=ms=0;
        tera_sound_action(g,actions[i]);assert(g->random.seed==seed_after(1986,counts[i]));
        assert(notes>=counts[i]);
        if(i==0) {old.seed=1986;assert(tera_random_draw(&old,255,&value));assert(first_hz==30*value+10 && !first_ms);}
        if(i==2) assert(first_hz==40 && first_ms==1500);
    }
    g->random.seed=1986;notes=ms=0;tera_sound_hit(g);assert(notes==58 && g->random.seed==seed_after(1986,58));
    g->random.seed=1986;old=g->random;notes=ms=0;calls=tera_random_sound_melody(&old);
    tera_sound_victory(g);assert(g->random.seed==old.seed && calls>207 && notes==207 && ms==3105);
    for(i=0;i<2;++i) {
        g->random.seed=1986;old=g->random;notes=ms=0;calls=tera_random_sound_steps(&old,(int)i);
        assert(tera_sound_steps(g,(int)i)==calls && g->random.seed==old.seed);
        assert(notes==calls && ms==(i ? 0u : 2850u));
    }
    g->random.seed=1986;notes=ms=0;
    assert(tera_sound_alert(g)==180 && notes==180 && ms==5400 && g->random.seed==seed_after(1986,180));
    /* Empreintes de SOUND/DELAY executes dans TERA.COM, graine 1986.
     * Outil optionnel : tools/audit_media_binary.py. Aucun oracle Python
     * requis ici. Toutes les notes/durees, pas seulement leur nombre. */
    for(i=0;i<11;++i) {
        static const uint32_t hashes[]={0x6b2d2103,0xadeaa136,0x7f8435e7,0x83cb3b46,0x2225cd26,
            0xdbda13b0,0xda106054,0x70bf7f5c,0x05b7417c,0xe92fa20f,0x70db0b09};
        static const unsigned times[]={36,400,8400,4604,4604,2645,2645,3105,3105,5940,7920};
        g->random.seed=1986;old=g->random;notes=ms=0;score_hash=UINT32_C(2166136261);
        switch(i) {
        case 0:tera_sound_wave(g,6000,6,6);break;
        case 1:tera_sound_wave(g,200,2,200);break;
        case 2:tera_sound_wave(g,30,1400,6);break;
        case 3:tera_sound_hyperspace(g,0);break;
        case 4:tera_sound_hyperspace(g,1);break;
        case 5:case 6:
            calls=tera_random_sound_country(&old);
            assert(tera_sound_country(g,i==5 ? 1 : 4,i==5 ? 1 : 3)==calls);break;
        case 7:case 8:
            calls=tera_random_sound_melody(&old);
            assert(tera_sound_melody(g,i==7,2,9,8)==calls);break;
        case 9:case 10:
            calls=tera_random_sound_noise(&old,100,i==9 ? 12 : 6);
            assert(tera_sound_noise(g,i==9 ? 60 : 130,100,i==9 ? 60 : 80,
                i==9 ? 12 : 6,14,i==9 ? 8000 : 1,i==9 ? 10000 : 5000)==calls);break;
        }
        if(score_hash!=hashes[i]) fprintf(stderr,"BIN score %u: %08lx != %08lx, %u ms\n",i,
            (unsigned long)score_hash,(unsigned long)hashes[i],ms);
        assert(score_hash==hashes[i] && ms==times[i] && g->random.seed==old.seed);
    }
    /* Partitions de progression executees par le binaire, frequences,
     * delais et graine finale independants du port C. */
    for(i=0;i<7;++i) {
        const OriginalProgressionSound *v=&original_progression_sounds[i];
        g->random.seed=1986;notes=ms=0;score_hash=UINT32_C(2166136261);
        switch(i) {
        case 0:tera_sound_objective(g);break;
        case 1:tera_sound_melody(g,0,1,9,8);break;
        case 2:tera_sound_entry_meduz(g);break;
        case 3:tera_sound_refusal(g);break;
        case 4:tera_sound_sweep(g,50,1500,10,20);break;
        case 5:tera_sound_random_tones(g,100,2,0,16);break;
        case 6:tera_sound_wave(g,3000,30,100);break;
        }
        assert(score_hash==v->hash && notes==v->notes && ms==v->ms && g->random.seed==v->seed);
    }
    /* Objectif Meduz : seul le premier succes joue, le mode muet garde
     * sa graine. Brancher un lecteur ne modifie jamais le resultat. */
    for(i=0;i<2;++i) {
        TeraExplorationState state=module_fixture();TeraMeduzSession observed,plain;
        const char *key="OMENTAL\r";
        state.sound=(int)i;state.world.mental_extension=1;
        state.world.characters.combat.world_id=10;state.world.characters.combat.position=1000;
        state.world.characters.combat.previous_position=999;state.world.maps[1000][4]=3;
        assert(tera_meduz_start(&plain,&state,'I',0,0,0,1200,NULL,NULL));
        state.world.characters.combat.presentation=&sink;
        assert(tera_meduz_start(&observed,&state,'I',0,0,0,1200,NULL,NULL));
        notes=ms=0;score_hash=UINT32_C(2166136261);
        while(*key) {
            assert(tera_meduz_key(&observed,(unsigned char)*key));
            assert(tera_meduz_key(&plain,(unsigned char)*key++));
        }
        assert(notes==(i ? 621u : 0u) && ms==(i ? 9315u : 0u));
        if(i) assert(score_hash==original_progression_sounds[0].hash);
        observed.state.world.characters.combat.presentation=NULL;
        assert(!memcmp(&observed.state,&plain.state,sizeof plain.state));
        assert(plain.state.world.tasks_done[0] && plain.state.world.tasks_counter==1);
        assert(plain.state.world.characters.combat.random.seed==(i ? original_progression_sounds[0].seed : 1986u));
        state=plain.state;state.world.characters.combat.presentation=&sink;
        assert(tera_meduz_start(&observed,&state,'I',0,0,0,1200,NULL,NULL));
        notes=ms=0;key="OMENTAL\r";
        while(*key) assert(tera_meduz_key(&observed,(unsigned char)*key++));
        assert(!notes && !ms && observed.state.world.tasks_counter==1);
    }
    /* Raccordement des sept lieux : son actif, sans modifier leurs etats
     * quand aucun frontend n'est branche. Quantites/durees TERAMAIN. */
    for(i=0;i<7;++i) {
        static const unsigned towns[]={0,1,3,4,6,8,9};
        static const unsigned counts[]={166,9208,6,6,621,99,414};
        static const unsigned times[]={5290,9208,8400,8400,9315,7920,6210};
        TeraExplorationState observed=module_fixture(),plain;
        observed.town_found=observed.moved=observed.sound=1;observed.town_id=towns[i];plain=observed;
        observed.world.characters.combat.presentation=&sink;notes=ms=0;
        assert(tera_places_present(&observed,0) && tera_places_present(&plain,0));
        assert(notes==counts[i] && ms==times[i]);observed.world.characters.combat.presentation=NULL;
        assert(!memcmp(&observed,&plain,sizeof plain));
    }
    {
        TeraSpaceSession observed,plain;TeraCharacterState state=module_fixture().world.characters;
        state.combat.sound=1;assert(tera_space_start(&plain,&state,4));
        state.combat.presentation=&sink;assert(tera_space_start(&observed,&state,4));notes=ms=0;
        assert(tera_space_key(&observed,'N') && tera_space_key(&plain,'N'));
        assert(notes==5316 && ms==6028);observed.state.combat.presentation=NULL;
        assert(!memcmp(&observed,&plain,sizeof plain));
    }
    /* Le service absent ne change aucune frequence/branche ni graine. */
    old=g->random;g->presentation=NULL;tera_sound_hit(g);assert(g->random.seed==seed_after(old.seed,58));
    {
        TeraCombatSession s;unsigned key,turn=0;int money;
        e=module_fixture();g=&e.world.characters.combat;g->presentation=&sink;g->encounter.monster_type=1;
        g->monster_values[0][0]=3;g->monster_values[0][3]=0;
        g->encounter.attributes[0].stats[1]=100;
        money=g->money;assert(tera_combat_start(&s,g,0,NULL,NULL));
        while(s.phase<TERA_COMBAT_DONE && turn++<100) {
            key=s.phase==TERA_COMBAT_ACTION ? 'L' : 'O';assert(tera_combat_key(&s,key));
        }
        assert(s.phase==TERA_COMBAT_DONE && s.game.encounter.victory);
        assert(last_frame.kind==TERA_EVENT_END && last_frame.value && last_frame.reward>=2 && last_frame.reward<=51);
        assert(last_frame.reward==s.game.money-money);
    }
    return 0;
}
