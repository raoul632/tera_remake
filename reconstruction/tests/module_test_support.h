#ifndef TERA_MODULE_TEST_SUPPORT_H
#define TERA_MODULE_TEST_SUPPORT_H
/* Les contrats doivent rester executes aussi dans une compilation Release. */
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <string.h>
#include "tera/exploration.h"

/* Donnees SYNTHETIQUES, sans fichier original : valeurs explicites pour
 * isoler les regles, pas une nouvelle partie ni un equilibrage propose. */
static inline TeraExplorationState module_fixture(void)
{
    TeraExplorationState s={0};unsigned i,j;
    TeraCombatGame *g=&s.world.characters.combat;
    g->world_id=11;g->position=1200;g->previous_position=1199;
    g->difficulty=1;g->random.seed=1986;g->money=2000;
    g->encounter.party_available=1;g->encounter.actors[0].active=1;
    s.world.characters.days=7300;s.world.characters.energy=100;
    s.world.alfol_position=45;s.world.initial_flag=1;s.city_backup_position=1200;
    for(i=0;i<10;++i) {
        strcpy(s.world.characters.names[i],"TEST");strcpy(g->encounter.actors[i].condition,"OK");
        g->encounter.actors[i].hp=g->encounter.actors[i].hp_limit=30;
        g->encounter.actors[i].magic=g->encounter.actors[i].magic_limit=20;
        for(j=1;j<10;++j) {g->encounter.attributes[i].stats[j]=10;g->encounter.attributes[i].skills[j]=3;}
        s.world.locations[i]=-3000;s.world.extra_locations[i]=-3000;
    }
    for(i=0;i<22;++i) {
        g->monster_values[i][0]=10;g->monster_values[i][1]=1;g->monster_values[i][2]=1;
        g->monster_values[i][3]=1;g->monster_values[i][4]=1;g->monster_values[i][5]=1;
    }
    for(i=0;i<4;++i) g->helpers[i]=(uint8_t)(3*(i+1));
    for(i=0;i<1100;++i) {s.world.maps[i][6]=1;g->maps_column7[i]=1;}
    for(i=0;i<9;++i) s.world.horizontal_mountains[i]=-3000;
    for(i=0;i<25;++i) s.world.vertical_mountains[i]=-3000;
    for(i=0;i<20;++i) s.world.towns[i]=(int16_t)(1201+i);
    strcpy(g->secret_word,"SECRET");strcpy(s.world.secret_words[1],"VOYAGE");
    strcpy(s.world.secret_words[2],"MENTAL");
    return s;
}
/* Compteur independant des modules : la recurrence BIN documentee suffit
 * a verifier un nombre fixe de tirages, sans appeler leur routine de son. */
static inline uint32_t seed_after(uint32_t seed,unsigned count)
{
    while(count--) seed=seed*UINT32_C(129)+UINT32_C(0x361962e9);
    return seed;
}
#endif
