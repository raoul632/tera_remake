#include "tera/encounter.h"
#include <stddef.h>

static int16_t wrap_signed16(int32_t value)
{
    uint32_t bits = (uint32_t)value & UINT32_C(65535);
    return (int16_t)(bits < 32768 ? (int32_t)bits : (int32_t)bits - 65536);
}

/* C_9419 : premiere action sans garde victoire, suivantes avec les deux
 * drapeaux ; test HP apres les trois premiers emplacements seulement.
 * Le parent effectue le dernier test. Ne pas raccourcir en boucle qui
 * normaliserait immediatement les personnages blesses. */
static void execute_party_actions(TeraEncounterState *state,
                                  const TeraRoundPreparation *round,
                                  const TeraEncounterRoundInputs *inputs,
                                  TeraEncounterRoundResult *result)
{
    unsigned slot;
    for (slot = 0; slot < 4; ++slot) {
        if (slot < round->count && !state->fled && (slot == 0 || !state->victory)) {
            uint8_t id = round->actor_ids[slot];
            const TeraEncounterChoice *choice = &inputs->choices[id];
            result->events[result->event_count++] = id + 1;
            if (choice->letter == 'F')
                state->fled = 1;
            else {
                TeraHeroAttackResult attack;
                tera_combat_resolve_hero_attack(&state->reaction, choice->letter,
                    choice->raw_action, state->actors[id].magic, state->monster_hp,
                    choice->secret_word_matches, &attack);
                state->actors[id].magic = attack.magic_after;
                state->monster_hp = attack.monster_hp_after;
            }
        }
        if (slot < 3 && state->monster_hp < 3)
            state->victory = 1;
    }
}

static int execute_retaliation(TeraEncounterState *state,
                               const TeraRoundPreparation *round,
                               const TeraEncounterRoundInputs *inputs,
                               TeraEncounterRoundResult *result)
{
    uint8_t id;
    TeraMonsterAttackResult attack;
    if (inputs->target_draw >= round->count)
        return 0;
    result->target_draw_required = 1;
    /* Les cibles 5..10 sont un CASE absent dans le binaire. */
    if (!tera_combat_select_round_target(round, inputs->target_draw, &id))
        return 1;
    tera_combat_resolve_monster_attack(state->monster_type, state->monster_hp,
        inputs->raw_defense[id], inputs->poison_draw_is_zero, &attack);
    result->target_draw_required = 1;
    result->poison_draw_required = attack.poison_draw_required;
    result->events[result->event_count++] = -(int)(id + 1);
    state->actors[id].hp = wrap_signed16((int32_t)state->actors[id].hp - attack.damage);
    if (attack.poisoned) {
        state->actors[id].condition[0] = 'E';
        state->actors[id].condition[1] = 'M';
        state->actors[id].condition[2] = '\0';
    }
    return 1;
}

int tera_combat_execute_round(TeraEncounterState *state,
                              const TeraEncounterRoundInputs *inputs,
                              TeraEncounterRoundResult *result)
{
    TeraEncounterState next;
    TeraEncounterRoundResult report = {0, {0}, 0, 0, 0};
    TeraRoundPreparation round;
    unsigned slot, id, shown = 0;
    if (state == NULL || inputs == NULL || result == NULL || state->victory ||
        state->fled || state->monster_type < 1 || state->monster_type > 22)
        return 0;
    next = *state;
    if (!tera_combat_prepare_round(next.actors, inputs->monster_initiative,
                                   inputs->initiative_draw, &round))
        return 0;
    if (round.party_first) {
        execute_party_actions(&next, &round, inputs, &report);
        if (next.monster_hp < 3)
            next.victory = 1;
        else if (!execute_retaliation(&next, &round, inputs, &report))
            return 0;
    } else {
        if (!execute_retaliation(&next, &round, inputs, &report))
            return 0;
        /* A745 est le drapeau anterieur : la mort de la cible n'a pas
         * encore appele C_7CC8. Garder sa possibilite d'action originale. */
        if (next.party_available)
            execute_party_actions(&next, &round, inputs, &report);
        if (next.monster_hp < 3)
            next.victory = 1;
    }
    next.party_available = 0;
    for (slot = 0; slot < round.count && slot < 4; ++slot)
        if (tera_combat_update_actor_condition(&next.actors[round.actor_ids[slot]]))
            next.party_available = 1;
    /* C_4D46 affiche/recalcule les quatre premiers ACTIFS, HS inclus.
     * Cela peut differer de la liste des quatre aptes du tour. */
    for (id = 0; id < 10 && shown < 4; ++id)
        if (next.actors[id].active) {
            TeraCombatLimits limits;
            tera_combat_actor_limits(&next.attributes[id], &limits);
            next.actors[id].hp_limit = limits.hp;
            next.actors[id].magic_limit = limits.magic;
            ++shown;
        }
    report.continue_combat = next.party_available && !next.victory && !next.fled;
    *state = next;
    *result = report;
    return 1;
}

int tera_combat_victory_rewards(int victory, int party_available,
                                int16_t money, int16_t experience,
                                int16_t initial_monster_hp, unsigned money_draw,
                                TeraCombatRewards *result)
{
    TeraCombatRewards rewards = {0, money, experience};
    if (result == NULL)
        return 0;
    if (victory && party_available) {
        if (money_draw >= 50)
            return 0;
        rewards.granted = 1;
        rewards.money = wrap_signed16((int32_t)money + (int32_t)money_draw + 2);
        rewards.experience = wrap_signed16((int32_t)experience + initial_monster_hp);
    }
    *result = rewards;
    return 1;
}

int tera_combat_escape(unsigned world_id, int on_alfol,
                        int16_t old_previous_position,
                        const uint8_t maps_column7[1100],
                        const unsigned *draws, unsigned draw_count,
                        TeraCombatEscape *result)
{
    unsigned index;
    TeraCombatEscape escaped = {0, old_previous_position, 0};
    if (world_id > 11 || result == NULL || draws == NULL || draw_count == 0 ||
        (world_id < 11 && maps_column7 == NULL))
        return 0;
    for (index = 0; index < draw_count; ++index) {
        unsigned bound = world_id < 11 ? 90 : on_alfol ? 99 : 255;
        if (draws[index] >= bound)
            return 0;
        ++escaped.draws_used;
        if (world_id == 11) {
            escaped.position = (int16_t)(on_alfol ? draws[index] + 1 : 3 * draws[index]);
            *result = escaped;
            return 1;
        }
        /* Colonne 7 testee apres chaque candidat ; un refus consomme bien
         * son tirage. Ne pas remplacer par un tirage parmi des cases filtrees. */
        escaped.position = (int16_t)(world_id * 100 + draws[index] + 5);
        if (maps_column7[escaped.position]) {
            escaped.previous_position = (int16_t)(escaped.position - 1);
            *result = escaped;
            return 1;
        }
    }
    return 0;
}
