#include "tera/round.h"
#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "check failed at line %d\n", __LINE__); return 1; \
} } while (0)

int main(void)
{
    TeraRoundActor actors[10] = {{0}}, before[10];
    TeraRoundPreparation round;
    TeraCombatActor stats = {{0}, {0}};
    TeraCombatLimits limits;
    uint8_t target = 99;
    /* Maxima : les seules colonnes concernees, pas les competences. */
    stats.stats[1] = 4; stats.stats[4] = 6; stats.stats[9] = 8;
    stats.stats[2] = 3; stats.stats[5] = 7; stats.stats[6] = 255;
    CHECK(tera_combat_actor_limits(&stats, &limits));
    CHECK(limits.hp == 18 && limits.magic == 10);
    /* C_7CC8 ne traite pas EM comme HS et ne guerit jamais un HS. */
    strcpy(actors[0].condition, "EM"); actors[0].hp = 1;
    CHECK(tera_combat_update_actor_condition(&actors[0]));
    CHECK(strcmp(actors[0].condition, "EM") == 0);
    actors[0].hp = -3;
    CHECK(!tera_combat_update_actor_condition(&actors[0]));
    CHECK(actors[0].hp == 0 && strcmp(actors[0].condition, "HS") == 0);
    actors[0].hp = -3;
    CHECK(!tera_combat_update_actor_condition(&actors[0]) && actors[0].hp == -3);
    actors[0].hp = 12;
    CHECK(!tera_combat_update_actor_condition(&actors[0]) && actors[0].hp == 12);
    /* Roster 1,4,9 ; un actif HS et un inactif rapide ne contribuent pas.
     * Moyenne (4+8+11)/3=7 ; seuil 5+4-2=7, egalite au monstre. */
    actors[0].active = 1; actors[0].agility = 255;
    actors[1].active = actors[4].active = actors[9].active = 1;
    actors[1].agility = 4; actors[4].agility = 8; actors[9].agility = 11;
    actors[2].agility = 255;
    actors[1].magic = 2; actors[1].magic_limit = 3;
    actors[4].magic = 5; actors[4].magic_limit = 5;
    actors[9].magic = 6; actors[9].magic_limit = 5;
    CHECK(tera_combat_prepare_round(actors, 5, 4, &round));
    CHECK(round.count == 3 && round.actor_ids[0] == 1 && round.actor_ids[1] == 4 && round.actor_ids[2] == 9);
    CHECK(round.mean_agility == 7 && !round.party_first);
    CHECK(actors[1].magic == 3 && actors[4].magic == 5 && actors[9].magic == 6);
    CHECK(tera_combat_prepare_round(actors, 5, 3, &round) && round.party_first);
    CHECK(tera_combat_select_round_target(&round, 0, &target) && target == 1);
    CHECK(tera_combat_select_round_target(&round, 2, &target) && target == 9);
    CHECK(!tera_combat_select_round_target(&round, 3, &target) && target == 9);
    /* La liste depend de l'etat, pas d'un test HP ajoute prematurement. */
    actors[1].hp = -5;
    CHECK(tera_combat_prepare_round(actors, 32767, 4, &round) && round.party_first);
    CHECK(round.count == 3);
    /* Recrutement multiple : cinq aptes, mais seulement quatre emplacements. */
    actors[2].active = actors[3].active = 1;
    CHECK(tera_combat_prepare_round(actors, 5, 0, &round));
    CHECK(round.count == 5 && round.actor_ids[3] == 4);
    CHECK(round.mean_agility == (4+255+0+8)/5);
    CHECK(!tera_combat_select_round_target(&round, 4, &target) && target == 9);
    memcpy(before, actors, sizeof actors);
    round.count = 99;
    CHECK(!tera_combat_prepare_round(actors, 5, 5, &round));
    CHECK(memcmp(before, actors, sizeof actors) == 0 && round.count == 99);
    puts("C combat round source vectors passed");
    return 0;
}
