#include "tera/creation.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    TeraCharacterState state;
    TeraNameVariants variants;
    TeraCreationSession session, before;
    TeraCombatGame destination, untouched;
    unsigned id, stat, group;
    uint32_t seed;
    const char *bonus = "FFFFFF";
    memset(&state, 0, sizeof state);
    memset(&variants, 0, sizeof variants);
    state.combat.random.seed = 1986;
    state.combat.money = -1;
    for (id = 1; id < 10; ++id) {
        snprintf(state.names[id], 10, "Name%u", id);
        for (group = 0; group < 2; ++group)
            snprintf(variants.names[group][id], 10, "Alt%u%u", group, id);
    }
    assert(tera_characters_initialize(&state, &variants));
    assert(state.combat.encounter.actors[0].active);
    for (id = 1; id < 10; ++id) {
        assert(!state.combat.encounter.actors[id].active);
        assert(state.combat.encounter.attributes[id].stats[id] == 9);
        assert(state.combat.encounter.attributes[id].skills[id] == 3);
        for (stat = 1; stat < 10; ++stat)
            assert(state.combat.encounter.attributes[id].stats[stat] >= 2 &&
                   state.combat.encounter.attributes[id].stats[stat] <= 9);
    }
    assert(state.combat.encounter.attributes[3].stats[5] == 7);
    assert(state.combat.encounter.attributes[6].skills[2] == 1);
    assert(state.combat.equipment[1][0] == 13 && state.combat.equipment[1][1] == 20);
    assert(state.combat.equipment[9][0] == 21 && state.combat.equipment[9][1] == 18);
    /* OVL0022 peut servir a une renaissance : ses objets restent presents. */
    state.combat.equipment[0][0] = 18;
    state.combat.encounter.attributes[0].skills[0] = 255;
    assert(tera_creation_start(&session, &state));
    seed = session.state.combat.random.seed;
    assert(tera_creation_key(&session, 'a'));
    assert(tera_creation_key(&session, '7'));
    assert(tera_creation_key(&session, 'b'));
    assert(tera_creation_key(&session, 8));
    assert(tera_creation_key(&session, 13));
    assert(strcmp(session.state.names[0], "A") == 0);
    assert(session.state.combat.random.seed == seed);
    assert(tera_creation_key(&session, 'h'));
    assert(session.phase == TERA_CREATE_SPLIT);
    assert(session.state.combat.encounter.attributes[0].stats[1] +
           session.state.combat.encounter.attributes[0].stats[2] +
           session.state.combat.encounter.attributes[0].stats[3] == 18);
    seed = session.state.combat.random.seed;
    assert(tera_creation_key(&session, '?'));
    assert(session.split == 1 && session.state.combat.random.seed == seed);
    assert(tera_creation_key(&session, '0'));
    assert(session.state.combat.encounter.attributes[0].stats[4] == 0);
    assert(tera_creation_key(&session, '9'));
    assert(session.state.combat.random.seed != seed);
    assert(tera_creation_key(&session, '9'));
    assert(session.phase == TERA_CREATE_BONUS && session.bonuses_left == 6);
    assert(tera_creation_key(&session, '?'));
    assert(session.bonuses_left == 6);
    for (id = 0; id < 6; ++id) assert(tera_creation_key(&session, (unsigned)bonus[id]));
    assert(session.phase == TERA_CREATE_KEEP);
    assert(session.state.combat.encounter.attributes[0].stats[1] >= 13);
    before = session;
    assert(tera_creation_key(&session, 'n'));
    assert(session.phase == TERA_CREATE_NAME && session.attempts == 2);
    assert(session.state.combat.random.seed == before.state.combat.random.seed);
    assert(session.state.combat.equipment[0][0] == 18);
    /* Accepter n'importe quelle touche sauf N, meme espace. */
    session = before;
    assert(tera_creation_key(&session, ' '));
    assert(session.phase == TERA_CREATE_DONE);
    assert(session.state.combat.money == 500);
    assert(session.state.days == 7300 && session.state.level == 0 && session.state.dummy_level == 1);
    assert(session.state.combat.world_id == 11 && session.state.combat.position == 1000);
    assert(session.state.arrival_pending && session.state.keys[0] == 0);
    assert(session.state.combat.equipment[0][0] == 18);
    for (id = 0; id < 10; ++id) {
        TeraRoundActor *a = &session.state.combat.encounter.actors[id];
        assert(a->hp == a->hp_limit && a->magic == a->magic_limit);
        assert(strcmp(a->condition, "OK") == 0);
        assert(session.state.combat.encounter.attributes[0].skills[id] == 0);
    }
    memset(&destination, 0, sizeof destination);
    memset(destination.maps_column7, 201, sizeof destination.maps_column7);
    strcpy(destination.secret_word, "SECRET");
    destination.helpers[0] = 12;
    destination.monster_values[3][5] = 37;
    destination.on_alfol = 1;
    untouched = destination;
    assert(tera_creation_apply_combat(&session, &destination));
    assert(memcmp(destination.maps_column7, untouched.maps_column7, sizeof destination.maps_column7) == 0);
    assert(strcmp(destination.secret_word, "SECRET") == 0);
    assert(destination.helpers[0] == 12 && destination.monster_values[3][5] == 37 && destination.on_alfol == 1);
    assert(destination.world_id == 11 && destination.position == 1000);
    before = session;
    assert(!tera_creation_key(&session, 'N'));
    assert(memcmp(&session, &before, sizeof session) == 0);
    state.names[9][9] = 'X';
    memset(state.names[9], 'X', 10);
    assert(!tera_creation_start(&session, &state));
    assert(memcmp(&session, &before, sizeof session) == 0);
    state = session.state;
    assert(tera_creation_start(&session, &session.state));
    assert(session.phase == TERA_CREATE_NAME);
    assert(memcmp(&session.state, &state, sizeof state) == 0);
    puts("Creation contracts passed");
    return 0;
}
