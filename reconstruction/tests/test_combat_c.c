#include "tera/combat.h"
#include <stdio.h>

/* Garder les verifications actives meme en Release : assert pourrait etre
 * desactive par NDEBUG et faire annoncer un succes sans rien verifier. */
#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "check failed at line %d\n", __LINE__); return 1; \
} } while (0)

int main(void)
{
    /* Verifier le port C avec des resultats derives des instructions BIN,
     * sans utiliser la sortie du Python comme reference originale.
     * Stats=10, competences=3, systeme D=3, tirages 7 puis 5, bonus=3. */
    const int16_t expected[9] = {180, 202, 180, 180, 180, 224, 87, 126, 45};
    TeraCombatActor actor = {{0}, {0}};
    int16_t result = -1;
    unsigned i;
    uint8_t equipment[10] = {0};
    /* La qualite d'objet prime sur sa position dans l'inventaire. */
    CHECK(tera_combat_equipment_bonus(equipment, 15) == 0);
    equipment[0] = 13;
    CHECK(tera_combat_equipment_bonus(equipment, 15) == 1);
    equipment[9] = 14;
    CHECK(tera_combat_equipment_bonus(equipment, 15) == 2);
    equipment[5] = 15;
    CHECK(tera_combat_equipment_bonus(equipment, 15) == 3);
    for (i = 1; i <= 9; ++i) {
        actor.stats[i] = 10;
        actor.skills[i] = 3;
    }
    for (i = 1; i <= 9; ++i) {
        CHECK(tera_combat_action(&actor, i, 3, 7, 5, &result));
        CHECK(result == expected[i - 1]);
    }
    /* Sans arme, tirer echoue ; le type compose 6 ignore le second facteur. */
    CHECK(tera_combat_action(&actor, 2, 0, 7, 5, &result) && result == 0);
    CHECK(tera_combat_action(&actor, 6, 3, 3, 0, &result) && result == 0);
    /* Frontiere exacte : 11*4=44 echoue ; 15*3=45 produit 10. */
    actor.stats[6] = 8; actor.skills[6] = 0; actor.skills[7] = 5;
    CHECK(tera_combat_base_roll(&actor, 6, 6, 4, &result) && result == 0);
    actor.stats[6] = 11; actor.skills[6] = 1; actor.skills[7] = 3;
    CHECK(tera_combat_base_roll(&actor, 6, 6, 3, &result) && result == 10);
    /* IMUL conserve AX avant IDIV : 12970*3*3 revient a -14342. */
    actor.stats[9] = 255; actor.skills[9] = 255; actor.skills[7] = 255;
    CHECK(tera_combat_action(&actor, 2, 3, 7, 17, &result) && result == -7171);
    /* Les valeurs hors contrat sont rejetees par l'API moderne. */
    CHECK(!tera_combat_action(&actor, 0, 0, 7, 5, &result));
    CHECK(!tera_combat_action(&actor, 1, 0, 7, 0, &result));
    puts("C combat source vectors passed");
    return 0;
}
