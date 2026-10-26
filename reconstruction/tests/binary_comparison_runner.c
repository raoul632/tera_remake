#include "tera/combat.h"
#include "tera/round.h"
#include "tera/encounter.h"
#include "tera/random.h"
#include <stdio.h>
#include <string.h>

/* Petit pont de test en processus separe : Python 64 bits peut ainsi
 * comparer la bibliotheque compilee avec le GCC 32 bits de ce poste.
 * Protocole entier par ligne, reserve au test, sans dependance JSON. */
int main(void)
{
    char mode;
    int v[256], n, i;
    while (scanf(" %c %d", &mode, &n) == 2) {
        int16_t value = 0;
        TeraCombatActor actor = {{0}, {0}};
        if (n < 0 || n > 256) return 2;
        for (i = 0; i < n; ++i)
            if (scanf("%d", &v[i]) != 1) return 2;
        if (mode == 'Z' && n == 4) {
            TeraRandom rng = {0};
            if (!tera_random_clock_seed(&rng, (unsigned)v[0], (unsigned)v[1],
                                        (unsigned)v[2], (unsigned)v[3])) return 3;
            printf("%lu\n", (unsigned long)rng.seed);
        } else if (mode == 'G' && n == 3) {
            TeraRandom rng = {(uint32_t)v[0] | ((uint32_t)v[1] << 16)};
            unsigned draw;
            if (!tera_random_draw(&rng, (unsigned)v[2], &draw)) return 3;
            printf("%lu %u\n", (unsigned long)rng.seed, draw);
        } else if ((mode == 'A' || mode == 'B') && n == 24) {
            for (i = 0; i < 10; ++i) {
                actor.stats[i] = (uint8_t)v[4+i];
                actor.skills[i] = (uint8_t)v[14+i];
            }
            if (mode == 'A') {
                if (!tera_combat_action(&actor, (unsigned)v[0], (unsigned)v[1],
                                         (unsigned)v[2], (unsigned)v[3], &value)) return 3;
            } else if (!tera_combat_base_roll(&actor, (unsigned)v[0], (unsigned)v[1],
                                              (unsigned)v[2], &value)) return 3;
            printf("%d\n", value);
        } else if (mode == 'E' && n == 11) {
            uint8_t equipment[10];
            for (i = 0; i < 10; ++i) equipment[i] = (uint8_t)v[i+1];
            printf("%u\n", tera_combat_equipment_bonus(equipment, (unsigned)v[0]));
        } else if (mode == 'H' && n == 8) {
            TeraMonsterReaction monster = {(int16_t)v[1], (int16_t)v[2], (int16_t)v[0]};
            TeraHeroAttackResult result;
            if (!tera_combat_resolve_hero_attack(&monster, (char)v[3], (int16_t)v[4],
                                                  (int16_t)v[5], (int16_t)v[6], v[7], &result)) return 3;
            printf("%d %d %d %d\n", result.eligible, result.damage,
                   result.magic_after, result.monster_hp_after);
        } else if (mode == 'M' && n == 4) {
            TeraMonsterAttackResult result;
            if (!tera_combat_resolve_monster_attack((unsigned)v[0], (int16_t)v[1],
                                                     (int16_t)v[2], v[3], &result)) return 3;
            printf("%d %d %d\n", result.damage, result.poison_draw_required, result.poisoned);
        } else if (mode == 'V' && n == 10) {
            TeraCombatLimits limits;
            for (i = 0; i < 10; ++i) actor.stats[i] = (uint8_t)v[i];
            if (!tera_combat_actor_limits(&actor, &limits)) return 3;
            printf("%d %d\n", limits.hp, limits.magic);
        } else if (mode == 'S' && n == 3) {
            TeraRoundActor state = {0};
            int available;
            state.hp = (int16_t)v[0];
            strcpy(state.condition, v[1] == 2 ? "HS" : v[1] == 1 ? "EM" : "OK");
            available = tera_combat_update_actor_condition(&state);
            printf("%d %d %d\n", state.hp,
                   strcmp(state.condition, "HS") == 0 ? 2 : v[1], v[2] || available);
        } else if (mode == 'R' && n == 53) {
            TeraRoundActor actors[10] = {{0}};
            TeraRoundPreparation prepared;
            uint8_t target;
            for (i = 0; i < 10; ++i) {
                actors[i].active = (uint8_t)v[3+5*i];
                strcpy(actors[i].condition, v[4+5*i] == 2 ? "HS" : v[4+5*i] == 1 ? "EM" : "OK");
                actors[i].agility = (uint8_t)v[5+5*i];
                actors[i].magic = (int16_t)v[6+5*i];
                actors[i].magic_limit = (int16_t)v[7+5*i];
            }
            if (!tera_combat_prepare_round(actors, (int16_t)v[0], (unsigned)v[1], &prepared) ||
                !tera_combat_select_round_target(&prepared, (unsigned)v[2], &target)) return 3;
            printf("%u", prepared.count);
            for (i = 0; i < (int)prepared.count; ++i) printf(" %u", prepared.actor_ids[i]);
            printf(" %d %d %u", prepared.mean_agility, prepared.party_first, target);
            for (i = 0; i < 10; ++i) printf(" %d", actors[i].magic);
            putchar('\n');
        } else if (mode == 'I' && n == 160) {
            TeraEncounterState state = {0};
            TeraEncounterRoundInputs inputs = {0};
            TeraEncounterRoundResult report;
            state.monster_type = (unsigned)v[0]; state.monster_hp = (int16_t)v[1];
            state.reaction.reaction_category = (int16_t)v[2];
            state.reaction.resistance_a = (int16_t)v[3]; state.reaction.resistance_b = (int16_t)v[4];
            inputs.monster_initiative = (int16_t)v[5]; inputs.initiative_draw = (unsigned)v[6];
            inputs.target_draw = (unsigned)v[7]; inputs.poison_draw_is_zero = v[8];
            state.party_available = v[9];
            for (i = 0; i < 10; ++i) {
                int offset = 10 + 15*i;
                state.actors[i].active = (uint8_t)v[offset];
                strcpy(state.actors[i].condition, v[offset+1] == 2 ? "HS" : v[offset+1] == 1 ? "EM" : "OK");
                state.actors[i].hp = (int16_t)v[offset+2]; state.actors[i].magic = (int16_t)v[offset+3];
                state.actors[i].magic_limit = (int16_t)v[offset+4]; state.actors[i].agility = (uint8_t)v[offset+5];
                inputs.choices[i].letter = (char)v[offset+6]; inputs.choices[i].raw_action = (int16_t)v[offset+7];
                inputs.choices[i].secret_word_matches = v[offset+8]; inputs.raw_defense[i] = (int16_t)v[offset+9];
                state.attributes[i].stats[1] = (uint8_t)v[offset+10]; state.attributes[i].stats[4] = (uint8_t)v[offset+11];
                state.attributes[i].stats[9] = (uint8_t)v[offset+12]; state.attributes[i].stats[2] = (uint8_t)v[offset+13];
                state.attributes[i].stats[5] = (uint8_t)v[offset+14];
            }
            if (!tera_combat_execute_round(&state, &inputs, &report)) return 3;
            printf("%d %d %d %d %d %d %u", state.monster_hp, state.victory, state.fled,
                   state.party_available, report.continue_combat, report.target_draw_required, report.event_count);
            for (i = 0; i < (int)report.event_count; ++i) printf(" %d", report.events[i]);
            printf(" %d", report.poison_draw_required);
            for (i = 0; i < 10; ++i)
                printf(" %d %d %d %d %d", state.actors[i].hp, state.actors[i].magic, state.actors[i].magic_limit,
                       state.actors[i].hp_limit,
                       strcmp(state.actors[i].condition, "HS") == 0 ? 2 : strcmp(state.actors[i].condition, "EM") == 0 ? 1 : 0);
            putchar('\n');
        } else if (mode == 'W' && n == 6) {
            TeraCombatRewards rewards;
            if (!tera_combat_victory_rewards(v[0], v[1], (int16_t)v[2], (int16_t)v[3],
                                              (int16_t)v[4], (unsigned)v[5], &rewards)) return 3;
            printf("%d %d %d\n", rewards.granted, rewards.money, rewards.experience);
        } else if (mode == 'X' && n == 6) {
            uint8_t column7[1100] = {0};
            unsigned draws[2] = {(unsigned)v[3], (unsigned)v[4]};
            TeraCombatEscape escaped;
            if (v[0] < 11) column7[v[0]*100 + v[4] + 5] = 1;
            if (!tera_combat_escape((unsigned)v[0], v[1], (int16_t)v[2], column7,
                                    draws, (unsigned)v[5], &escaped)) return 3;
            printf("%d %d %u\n", escaped.position, escaped.previous_position, escaped.draws_used);
        } else return 2;
    }
    return 0;
}
