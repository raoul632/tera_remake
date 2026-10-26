#include "tera/encounter.h"
#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "check failed at line %d\n", __LINE__); return 1; \
} } while (0)

static void setup(TeraEncounterState *state, TeraEncounterRoundInputs *inputs,
                   unsigned count)
{
    unsigned id;
    memset(state, 0, sizeof *state);
    memset(inputs, 0, sizeof *inputs);
    state->monster_type = 1; state->monster_hp = 20;
    state->reaction.resistance_a = 1; state->reaction.resistance_b = 20;
    state->party_available = 1;
    inputs->initiative_draw = 2;
    for (id = 0; id < count; ++id) {
        state->actors[id].active = 1; state->actors[id].agility = 10;
        state->actors[id].hp = 30; state->actors[id].magic = 9;
        state->actors[id].magic_limit = 12;
        strcpy(state->actors[id].condition, "OK");
        state->attributes[id].stats[2] = 5; state->attributes[id].stats[5] = 7;
        inputs->choices[id].letter = 'T'; inputs->choices[id].raw_action = 180;
    }
}

int main(void)
{
    TeraEncounterState state, before;
    TeraEncounterRoundInputs inputs;
    TeraEncounterRoundResult result;
    TeraCombatRewards rewards;
    TeraCombatEscape escaped;
    uint8_t column7[1100] = {0};
    const unsigned escape_draws[2] = {0, 89};
    /* Fuite apres une action de groupe : C_997E ne teste pas AFCF et
     * laisse la riposte si le monstre a encore >=3 HP. */
    setup(&state, &inputs, 2);
    inputs.choices[0].letter = 'F';
    CHECK(tera_combat_execute_round(&state, &inputs, &result));
    CHECK(state.fled && !state.victory && !result.continue_combat);
    CHECK(result.event_count == 2 && result.events[0] == 1 && result.events[1] == -1);
    CHECK(state.actors[0].hp == 21 && state.actors[1].magic == 10);
    /* Cible mortellement touchee avant le groupe : pas encore HS au
     * moment de son action. C_9567 normalise a la fin, comme le BIN. */
    setup(&state, &inputs, 1);
    inputs.monster_initiative = 255; state.actors[0].hp = 1;
    CHECK(tera_combat_execute_round(&state, &inputs, &result));
    CHECK(result.event_count == 2 && result.events[0] == -1 && result.events[1] == 1);
    CHECK(state.monster_hp == 10 && state.actors[0].hp == 0);
    CHECK(strcmp(state.actors[0].condition, "HS") == 0 && !state.party_available);
    /* Quatrieme coup : seuil du parent, pas une riposte apres victoire.
     * Le jet de cible invalide est ignore parce qu'aucune cible n'est tiree. */
    setup(&state, &inputs, 4); state.monster_hp = 33;
    inputs.target_draw = 999;
    CHECK(tera_combat_execute_round(&state, &inputs, &result));
    CHECK(state.victory && state.monster_hp == -7 && result.event_count == 4);
    CHECK(!result.target_draw_required && !result.poison_draw_required);
    /* Une borne de cible invalide quand elle est requise ne produit pas
     * un demi-tour : transaction locale, aucun effet dans state/result. */
    setup(&state, &inputs, 1); inputs.choices[0].raw_action = 0;
    inputs.target_draw = 1; before = state; result.event_count = 99;
    CHECK(!tera_combat_execute_round(&state, &inputs, &result));
    CHECK(memcmp(&state, &before, sizeof state) == 0 && result.event_count == 99);
    /* Maxima recalcules meme pour un actif HS : C_4D46 ne filtre pas HS. */
    setup(&state, &inputs, 1);
    state.actors[5].active = 1; strcpy(state.actors[5].condition, "HS");
    state.actors[5].magic_limit = 77;
    state.attributes[5].stats[2] = 3; state.attributes[5].stats[5] = 4;
    CHECK(tera_combat_execute_round(&state, &inputs, &result));
    CHECK(state.actors[5].magic_limit == 7);
    /* Recompenses seulement si victoire et survivant, XP sur HP initiaux.
     * Le jet maximal donne 51 pieces et conserve le debordement original. */
    CHECK(tera_combat_victory_rewards(1, 1, 32767, 32767, 20, 49, &rewards));
    CHECK(rewards.granted && rewards.money == -32718 && rewards.experience == -32749);
    CHECK(tera_combat_victory_rewards(1, 0, 10, 20, 30, 999, &rewards));
    CHECK(!rewards.granted && rewards.money == 10 && rewards.experience == 20);
    rewards.money = 123;
    CHECK(!tera_combat_victory_rewards(1, 1, 10, 20, 30, 50, &rewards));
    CHECK(rewards.money == 123);
    /* C_86E1 : premier candidat refuse, second accepte dans monde 10. */
    column7[1094] = 1;
    CHECK(tera_combat_escape(10, 0, -123, column7, escape_draws, 2, &escaped));
    CHECK(escaped.position == 1094 && escaped.previous_position == 1093 && escaped.draws_used == 2);
    escaped.position = 123;
    CHECK(!tera_combat_escape(10, 0, -123, column7, escape_draws, 1, &escaped));
    CHECK(escaped.position == 123);
    CHECK(tera_combat_escape(11, 1, -123, NULL, escape_draws, 1, &escaped));
    CHECK(escaped.position == 1 && escaped.previous_position == -123);
    CHECK(tera_combat_escape(11, 0, -123, NULL, escape_draws+1, 1, &escaped));
    CHECK(escaped.position == 267 && escaped.previous_position == -123);
    puts("C integrated encounter source vectors passed");
    return 0;
}
