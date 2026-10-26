#include "tera/interactive.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Fixtures artificielles : tester le contrat de l'hote et les frontieres,
 * en complement des campagnes COM sur les vraies lignes VALMON. */
static TeraCombatGame game_fixture(void)
{
    TeraCombatGame g;
    unsigned i;
    memset(&g, 0, sizeof g);
    g.encounter.monster_type = 1;
    g.monster_values[0][0] = 40;
    g.monster_values[0][3] = 1;
    g.monster_values[0][5] = 1;
    g.encounter.actors[0].active = 1;
    g.encounter.actors[0].hp = 100;
    g.encounter.actors[0].magic = 0;
    g.encounter.actors[0].magic_limit = 20;
    for (i = 0; i < 10; ++i)
        strcpy(g.encounter.actors[i].condition, "OK");
    for (i = 1; i < 10; ++i) {
        g.encounter.attributes[0].stats[i] = 10;
        g.encounter.attributes[0].skills[i] = 3;
    }
    g.equipment[0][0] = 18;
    g.equipment[0][1] = 21;
    g.helpers[0] = 3; g.helpers[1] = 6; g.helpers[2] = 9; g.helpers[3] = 12;
    strcpy(g.secret_word, "TEST");
    memset(g.maps_column7, 1, sizeof g.maps_column7);
    g.random.seed = 1986;
    g.difficulty = 1;
    return g;
}

int main(void)
{
    TeraCombatGame g = game_fixture();
    TeraCombatSession s, before;
    TeraRandom rng = {0};
    unsigned value = 99;
    uint32_t seed;
    assert(tera_random_clock_seed(&rng, 23, 59, 59, 99));
    assert(rng.seed == UINT32_C(0x173b3b63));
    seed = rng.seed;
    assert(!tera_random_clock_seed(&rng, 24, 0, 0, 0));
    assert(rng.seed == seed);
    assert(!tera_random_draw(&rng, 0, &value));
    assert(!tera_random_draw(&rng, 65536, &value));
    assert(rng.seed == seed && value == 99);
    rng.seed = 0;
    assert(tera_random_draw(&rng, 15, &value));
    assert(rng.seed == UINT32_C(0x361962e9) && value == 9);

    assert(tera_combat_start(&s, &g, 0, NULL, NULL));
    assert(s.phase == TERA_COMBAT_ACTION && s.round_number == 1);
    assert(s.game.recruit_ready == 0);
    assert(s.initial_monster_hp == 40);
    assert(s.game.encounter.actors[0].magic == 1);
    seed = s.game.random.seed;
    assert(tera_combat_key(&s, 't'));
    assert(s.phase == TERA_COMBAT_CONFIRM && s.letter == 'T');
    assert(s.game.random.seed == seed);
    assert(tera_combat_key(&s, 'n'));
    assert(s.phase == TERA_COMBAT_ACTION && s.attempt == 1);
    assert(s.game.recruit_ready == 1);
    assert(s.game.random.seed == seed); /* Annuler ne tire rien. */
    assert(tera_combat_key(&s, 'T'));
    assert(tera_combat_key(&s, 27)); /* Esc annule aussi, pas une confirmation. */
    assert(s.game.recruit_ready == 0);
    assert(s.round_number == 2); /* Deuxieme annulation consomme le tour. */

    assert(tera_combat_start(&s, &g, 0, NULL, NULL));
    assert(tera_combat_key(&s, 'C'));
    assert(tera_combat_key(&s, 13));
    assert(s.phase == TERA_COMBAT_WORD);
    seed = s.game.random.seed;
    assert(tera_combat_key(&s, 'a'));
    assert(tera_combat_key(&s, '7')); /* Seules les lettres sont conservees. */
    assert(tera_combat_key(&s, 'b'));
    assert(tera_combat_key(&s, 8));
    assert(strcmp(s.word, "A") == 0 && s.game.random.seed == seed);
    assert(tera_combat_key(&s, 13));
    assert(s.phase != TERA_COMBAT_WORD && s.game.random.seed != seed);

    assert(tera_combat_start(&s, &g, 0, NULL, NULL));
    assert(tera_combat_key(&s, 'F'));
    assert(tera_combat_key(&s, 'O'));
    assert(s.phase == TERA_COMBAT_DONE && s.game.encounter.fled);
    assert(s.game.location_changed);
    assert(s.game.position >= 5 && s.game.position <= 94);
    assert(s.game.previous_position == s.game.position - 1);
    before = s;
    assert(!tera_combat_key(&s, 'T'));
    assert(memcmp(&s, &before, sizeof s) == 0);

    memset(g.maps_column7, 0, sizeof g.maps_column7);
    assert(tera_combat_start(&s, &g, 0, NULL, NULL));
    assert(tera_combat_key(&s, 'F'));
    seed = s.game.random.seed;
    assert(tera_combat_key(&s, 'O'));
    assert(s.phase == TERA_COMBAT_ERROR && !s.game.encounter.fled);
    assert(s.game.random.seed == seed); /* Carte impossible : pas de faux tirage. */

    g = game_fixture();
    g.encounter.actors[0].hp = 0;
    assert(tera_combat_start(&s, &g, 0, NULL, NULL));
    assert(s.phase == TERA_COMBAT_DONE && s.game.random.seed == 1986);
    assert(strcmp(s.game.encounter.actors[0].condition, "HS") == 0);
    before = s;
    g = game_fixture();
    g.monster_values[0][5] = 0;
    assert(!tera_combat_start(&s, &g, 0, NULL, NULL));
    assert(memcmp(&s, &before, sizeof s) == 0);
    g = game_fixture();
    g.world_id = 12;
    assert(!tera_combat_start(&s, &g, 0, NULL, NULL));
    assert(memcmp(&s, &before, sizeof s) == 0);
    puts("Interactive input/state/RNG contracts passed");
    return 0;
}
