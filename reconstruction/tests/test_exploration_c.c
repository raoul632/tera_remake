#include "tera/exploration.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Contrats d'integration distincts de l'oracle BIN : bloquage atomique,
 * regeneration sans inventaire consomme et erreurs DOS non masquées. */
int main(void)
{
    TeraExplorationState state={0},before;
    TeraMoveReport report;
    TeraDaySession day;
    TeraRoundActor *hero=&state.world.characters.combat.encounter.actors[0];
    unsigned i;
    state.world.characters.combat.world_id=11;
    state.world.characters.combat.position=1000;
    state.world.characters.combat.random.seed=1986;
    state.world.characters.combat.difficulty=1;
    state.world.characters.combat.recruit_ready=1;
    state.world.characters.combat.location_changed=1;
    for(i=0;i<9;++i) state.world.horizontal_mountains[i]=-1;
    for(i=0;i<25;++i) state.world.vertical_mountains[i]=-1;
    for(i=0;i<20;++i) state.world.towns[i]=-1;
    before=state;
    assert(!tera_exploration_move(&state,'E',&report));
    assert(memcmp(&state,&before,sizeof state)==0);
    state.world.horizontal_mountains[0]=1000;
    assert(tera_exploration_move(&state,'8',&report));
    assert(!report.moved && !report.random_calls);
    assert(state.world.characters.combat.position==1000);
    assert(state.world.characters.combat.recruit_ready==1);
    assert(state.world.characters.combat.location_changed==0);
    state.world.horizontal_mountains[0]=-1;
    assert(tera_exploration_move(&state,'2',&report));
    assert(state.world.characters.combat.position==1899 && report.moved);
    assert(!state.world.characters.combat.recruit_ready);
    assert(tera_exploration_move(&state,'8',&report));
    assert(state.world.characters.combat.position==1000);
    /* Le nord apres une fuite ne ramene pas artificiellement a >=1000. */
    /* -1 est maintenant une vraie position d'arrivee : ne pas utiliser
     * cette valeur comme montagne fictive pour le test sans obstacle. */
    for(i=0;i<9;++i) state.world.horizontal_mountains[i]=-3000;
    for(i=0;i<25;++i) state.world.vertical_mountains[i]=-3000;
    state.world.characters.combat.position=-1;
    assert(tera_exploration_move(&state,'8',&report));
    assert(state.world.characters.combat.position==0);
    assert(tera_exploration_move(&state,'6',&report));
    assert(state.world.characters.combat.position==30);
    assert(tera_exploration_move(&state,'2',&report));
    assert(state.world.characters.combat.position==929);
    state.world.characters.combat.position=1000;
    /* Arriver juste devant une montagne est permis ; la traverser ne l'est pas. */
    state.world.horizontal_mountains[0]=1001;
    assert(tera_exploration_move(&state,'8',&report));
    assert(report.moved && report.near_mountain);
    before=state;
    assert(tera_exploration_move(&state,'8',&report) && !report.moved);
    assert(state.world.characters.combat.random.seed==before.world.characters.combat.random.seed);

    state.world.towns[0]=state.world.characters.combat.position;
    hero->active=1;hero->hp=1;hero->magic=0;
    strcpy(hero->condition,"OK");
    state.world.characters.combat.encounter.attributes[0].stats[1]=3;
    state.world.characters.combat.encounter.attributes[0].stats[2]=2;
    state.world.characters.combat.encounter.attributes[0].stats[4]=3;
    state.world.characters.combat.encounter.attributes[0].stats[9]=3;
    state.world.characters.combat.equipment[0][0]=22;
    state.world.characters.days=32767;
    state.world.characters.combat.random.seed=1986;
    assert(tera_day_start(&day,&state,NULL,NULL));
    assert(day.phase==TERA_DAY_DONE && day.day_advanced && !day.encounters);
    assert(day.state.world.characters.days==-32768);
    assert(day.state.world.characters.combat.encounter.actors[0].hp==2);
    assert(day.state.world.characters.combat.encounter.actors[0].magic==1);
    assert(day.state.world.characters.combat.equipment[0][0]==22);
    assert(day.random_calls==5); /* Deux gardes + deux jets vitalite + rencontre. */
    assert(!tera_day_key(&day,'O'));

    /* Le niveau ne grimpe qu'une fois par appel, meme avec beaucoup d'XP. */
    state.world.characters.combat.experience=30000;
    assert(tera_day_start(&day,&state,NULL,NULL));
    assert(day.phase==TERA_DAY_DONE && day.state.world.characters.level==1);
    assert(day.state.world.characters.dummy_level==1);
    /* Un groupe dont toutes les competences sont nulles ferait boucler
     * le rejet original : publier ERROR, sans competences fabriquees. */
    state.world.characters.combat.encounter.actors[1].active=1;
    state.world.characters.combat.encounter.actors[1].hp=1;
    assert(tera_day_start(&day,&state,NULL,NULL));
    assert(day.phase==TERA_DAY_ERROR);
    puts("Exploration/time integration contracts passed");
    return 0;
}
