#include "tera/round.h"
#include <stddef.h>

static int16_t wrap_signed16(int32_t value)
{
    uint32_t bits = (uint32_t)value & UINT32_C(65535);
    return (int16_t)(bits < 32768 ? (int32_t)bits : (int32_t)bits - 65536);
}

/* Comparaison de la chaine complete : "HSx" n'est pas "HS".
 * Les chaines de cette API ont exactement trois octets de stockage. */
static int is_out_of_action(const TeraRoundActor *actor)
{
    return actor->condition[0] == 'H' && actor->condition[1] == 'S' &&
           actor->condition[2] == '\0';
}

int tera_combat_actor_limits(const TeraCombatActor *actor, TeraCombatLimits *result)
{
    if (actor == NULL || result == NULL)
        return 0;
    /* Les trois octets totalisent au plus 765 : pas de debordement 16 bits.
     * Les competences n'interviennent pas dans les maxima de C_4C6C. */
    result->hp = (int16_t)(actor->stats[1] + actor->stats[4] + actor->stats[9]);
    result->magic = (int16_t)(actor->stats[2] + actor->stats[5]);
    return 1;
}

int tera_combat_update_actor_condition(TeraRoundActor *actor)
{
    if (actor == NULL)
        return 0;
    /* C_7CC8 : le marquage est conditionnel, l'etat HS reste persistant.
     * Garder le cas HP negatifs + HS : le binaire ne les ramene pas a zero. */
    if (actor->hp < 1 && !is_out_of_action(actor)) {
        actor->hp = 0;
        actor->condition[0] = 'H';
        actor->condition[1] = 'S';
        actor->condition[2] = '\0';
    }
    return !is_out_of_action(actor);
}

int tera_combat_prepare_round(TeraRoundActor actors[10],
                              int16_t monster_initiative,
                              unsigned initiative_draw,
                              TeraRoundPreparation *result)
{
    TeraRoundPreparation prepared = {0, {0}, 0, 0};
    unsigned id, slot;
    int32_t sum = 0;
    int16_t threshold;
    if (actors == NULL || result == NULL || initiative_draw > 4)
        return 0;
    for (id = 0; id < 10; ++id)
        if (actors[id].active && !is_out_of_action(&actors[id])) {
            /* Le CASE DOS memorise quatre IDs, mais compte tous les aptes.
             * Les suivants divisent la moyenne sans contribuer a la somme. */
            if (prepared.count < 4) {
                prepared.actor_ids[prepared.count] = (uint8_t)id;
                sum += actors[id].agility;
            }
            ++prepared.count;
        }
    if (prepared.count == 0)
        return 0;
    /* La preparation regenere chaque membre, meme si le monstre va jouer
     * en premier. Ce n'est pas une regeneration apres l'action individuelle. */
    for (slot = 0; slot < prepared.count && slot < 4; ++slot) {
        TeraRoundActor *actor = &actors[prepared.actor_ids[slot]];
        if (actor->magic < actor->magic_limit)
            actor->magic = wrap_signed16((int32_t)actor->magic + 1);
    }
    prepared.mean_agility = (int16_t)(sum / prepared.count);
    threshold = wrap_signed16((int32_t)monster_initiative + (int32_t)initiative_draw - 2);
    prepared.party_first = prepared.mean_agility > threshold;
    *result = prepared;
    return 1;
}

int tera_combat_select_round_target(const TeraRoundPreparation *round,
                                    unsigned target_draw, uint8_t *actor_id)
{
    if (round == NULL || actor_id == NULL || round->count < 1 ||
        round->count > 10 || target_draw >= round->count || target_draw >= 4)
        return 0;
    *actor_id = round->actor_ids[target_draw];
    return 1;
}
