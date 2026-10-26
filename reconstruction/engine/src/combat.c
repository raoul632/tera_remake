#include "tera/combat.h"
#include <stddef.h>

/* Reproduire le mot signe conserve dans AX apres une operation 8086.
 * Intention : preserver les debordements originaux, tout en evitant une
 * conversion C hors plage dont le resultat dependrait du compilateur.
 * Le calcul en 32 bits est technique ; le resultat de jeu reste en 16 bits. */
static int16_t wrap_signed16(int32_t value)
{
    uint32_t bits = (uint32_t)value & UINT32_C(65535);
    return (int16_t)(bits < 32768 ? (int32_t)bits : (int32_t)bits - 65536);
}

/* C_8215 : une competence non apprise (niveau 0) impose -2.
 * Un niveau appris est ajoute tel quel ; il n'y a pas de plancher a zero. */
static int32_t skill_modifier(uint8_t level)
{
    return level == 0 ? -2 : level;
}

/* C_82E5 : transformer le jet signe en resultat numerique d'action.
 * Le test est strict : 44 echoue, 45 donne 10. Ne pas remplacer cette
 * regle par max(0, roll-35), qui accepterait des jets de 36 a 44. */
static int16_t apply_success_threshold(int16_t roll)
{
    return roll > 44 ? (int16_t)(roll - 35) : 0;
}

unsigned tera_combat_equipment_bonus(const uint8_t equipment[10],
                                     unsigned required_item)
{
    unsigned offset, slot;
    /* Les appels audites donnent des IDs >=3. Rejeter les autres valeurs
     * protege l'API moderne sans inventer un comportement original. */
    if (equipment == NULL || required_item < 3 || required_item > 25)
        return 0;
    /* L'ordre des candidats prime sur l'ordre des emplacements : meme si
     * l'epee simple est au debut, l'epee runique doit gagner si presente.
     * Aucun compagnon n'est consulte par C_8391. */
    for (offset = 0; offset < 3; ++offset)
        for (slot = 0; slot < 10; ++slot)
            if (equipment[slot] == required_item - offset)
                return 3 - offset;
    return 0;
}

int tera_combat_base_roll(const TeraCombatActor *actor, unsigned stat,
                         unsigned skill, unsigned factor, int16_t *result)
{
    int32_t power;
    if (actor == NULL || result == NULL || stat < 1 || stat > 9 ||
        skill < 1 || skill > 9 || factor < 3 || factor > 17)
        return 0;
    /* BIN 8359 : la colonne 7 est le niveau brut de systeme D, pas un
     * bonus distinct ni skill_modifier(7). Il peut donc compter deux fois
     * si la competence principale est elle-meme la colonne 7. */
    power = actor->stats[stat] + skill_modifier(actor->skills[skill]) + actor->skills[7];
    *result = apply_success_threshold(wrap_signed16(power * (int32_t)factor));
    return 1;
}

int tera_combat_action(const TeraCombatActor *actor, unsigned action,
                       unsigned equipment_bonus, unsigned entry_factor,
                       unsigned base_factor, int16_t *result)
{
    /* Indices Pascal des actions simples. Les zeros des types 6..8
     * indiquent une formule composee, jamais une lecture de Stats[0]. */
    static const unsigned stat_by_action[10] = {0, 1, 9, 5, 3, 6, 0, 0, 0, 2};
    int16_t base;
    int32_t power;
    if (actor == NULL || result == NULL || action < 1 || action > 9 ||
        equipment_bonus > 3 || entry_factor < 3 || entry_factor > 17)
        return 0;
    if (action <= 5 || action == 9) {
        /* C_842A tire toujours entry_factor, mais ces branches l'ignorent
         * et utilisent un second tirage dans C_8315. L'appelant conserve
         * cette consommation meme si le premier tirage semble inutile. */
        unsigned stat = stat_by_action[action];
        if (!tera_combat_base_roll(actor, stat, stat, base_factor, &base))
            return 0;
    } else if (action == 6) {
        /* Chance + vitalite et leurs DEUX competences. Division apres
         * multiplication et reduction 16 bits, puis test du seuil. */
        power = actor->stats[3] + actor->stats[6] +
                skill_modifier(actor->skills[6]) + skill_modifier(actor->skills[3]);
        base = apply_success_threshold(
            (int16_t)(wrap_signed16(power * (int32_t)entry_factor) / 2));
    } else if (action == 7) {
        /* Endurance + agilite : division AVANT le facteur aleatoire,
         * sans competence. Deplacer cette division changerait l'arrondi. */
        power = (actor->stats[4] + actor->stats[7]) / 2;
        base = apply_success_threshold(wrap_signed16(power * (int32_t)entry_factor));
    } else {
        /* Type 8 : intelligence + volonte, seule competence volonte.
         * Les appelants ont ete bornes a 1..9 ci-dessus. */
        power = actor->stats[2] + actor->stats[5] + skill_modifier(actor->skills[5]);
        base = apply_success_threshold(wrap_signed16(power * (int32_t)entry_factor));
    }
    /* Appliquer l'equipement APRES le jet. Garder ces branches distinctes :
     * type 2 exige une arme, type 7 a un plancher de 2, types 8/9 ignorent
     * l'equipement, et les autres gardent un resultat meme sans objet.
     * Les divisions C99 signees tronquent vers zero, comme IDIV. */
    if (action == 2) {
        /* Deux IMUL distincts : reduire apres chacun, puis diviser.
         * Un unique calcul 32 bits suivi de /2 ne serait pas equivalent. */
        int16_t scaled_result = wrap_signed16((int32_t)base * (int32_t)equipment_bonus);
        scaled_result = wrap_signed16((int32_t)scaled_result * 3);
        *result = (int16_t)(scaled_result / 2);
    } else if (action == 7) {
        int16_t scaled_result = wrap_signed16((int32_t)base * (int32_t)(2 + equipment_bonus));
        *result = (int16_t)(scaled_result / 2);
    } else if (action == 8 || action == 9)
        *result = base;
    else
        *result = wrap_signed16((int32_t)base * (int32_t)(1 + equipment_bonus));
    return 1;
}
