#include "tera/combat.h"
#include <stddef.h>

/* Conserver AX modulo 65536 puis l'interpreter comme mot signe.
 * Meme convention que combat.c : pas de conversion C hors plage. */
static int16_t wrap_signed16(int32_t value)
{
    uint32_t bits = (uint32_t)value & UINT32_C(65535);
    return (int16_t)(bits < 32768 ? (int32_t)bits : (int32_t)bits - 65536);
}

/* BIN C_8B5E et C_8C45 : test signe AVANT soustraction/addition 16 bits.
 * Ne pas borner les degats : un debordement peut donner un mot negatif.
 * C99 tronque la division signee vers zero, comme IDIV dans le binaire. */
static int16_t damage_from_attack(int16_t attack, int16_t resistance)
{
    return attack > resistance
        ? (int16_t)(wrap_signed16((int32_t)attack - resistance + 3) / 3)
        : 0;
}

int tera_combat_resolve_hero_attack(const TeraMonsterReaction *monster,
                                   char action_letter, int16_t raw_action,
                                   int16_t magic, int16_t monster_hp,
                                   int secret_word_matches,
                                   TeraHeroAttackResult *result)
{
    unsigned divisor = 0, cost = 0;
    int16_t resistance;
    TeraHeroAttackResult resolved = {0, 0, magic, monster_hp, 0};
    if (monster == NULL || result == NULL)
        return 0;
    resistance = monster->resistance_b;
    /* C_8CE3, 8D45..928D : lettres du menu, pas types de C_842A.
     * Un diviseur nul signifie aucune branche. Les seuils MP sont stricts
     * et controles avant paiement, exactement comme CMP/JG originaux. */
    switch (monster->reaction_category) {
    case 0:
        if (action_letter == 'L' || action_letter == 'T') {
            divisor = 6; resistance = monster->resistance_a;
        } else if ((action_letter == 'J' || action_letter == 'A') && magic > 2) {
            divisor = 6; cost = 3;
        }
        break;
    case 1:
        if (action_letter == 'T') {
            divisor = 5; resistance = monster->resistance_a;
        } else if ((action_letter == 'J' || action_letter == 'A') && magic > 2) {
            divisor = 7; cost = 3;
        }
        break;
    case 3:
        if (action_letter == 'D')
            divisor = 5;
        else if (action_letter == 'J' && magic > 2) {
            divisor = 7; cost = 3;
        }
        break;
    case 4:
        if (action_letter == 'P')
            divisor = 5;
        else if (action_letter == 'L') {
            divisor = 9; resistance = monster->resistance_a;
        }
        break;
    case 5:
        if (action_letter == 'C' && secret_word_matches)
            divisor = 3;
        else if (action_letter == 'A' && magic > 5) {
            divisor = 9; cost = 6;
        }
        break;
    case 6:
        if (action_letter == 'A' && magic > 2) {
            divisor = 8; cost = 3;
        } else if (action_letter == 'T') {
            divisor = 5; resistance = monster->resistance_a;
        }
        break;
    default:
        break;
    }
    if (divisor != 0) {
        resolved.eligible = 1;
        resolved.damage = damage_from_attack((int16_t)(raw_action / (int)divisor),
                                             resistance);
        resolved.magic_after = wrap_signed16((int32_t)magic - (int32_t)cost);
        resolved.monster_hp_after = wrap_signed16((int32_t)monster_hp - resolved.damage);
    }
    /* C_9419 teste le seuil apres chaque compagnon, meme sans degats.
     * Ceci indique la defaite locale du monstre, pas la victoire du jeu. */
    resolved.defeated = resolved.monster_hp_after < 3;
    *result = resolved;
    return 1;
}

int tera_combat_resolve_monster_attack(unsigned monster_type,
                                      int16_t monster_hp, int16_t raw_defense,
                                      int poison_draw_is_zero,
                                      TeraMonsterAttackResult *result)
{
    int16_t attack, defense;
    TeraMonsterAttackResult resolved;
    if (result == NULL || monster_type < 1 || monster_type > 22)
        return 0;
    /* C_92FE : force liee aux HP ACTUELS, defense physique avant ID 15,
     * mentale ensuite. Aucun tirage supplementaire pour la force. */
    attack = wrap_signed16((int32_t)monster_hp + 4);
    defense = (int16_t)(raw_defense / (monster_type < 15 ? 11 : 9));
    resolved.damage = damage_from_attack(attack, defense);
    /* C_8C45 + routines d'ensemble 14FC/150E/15E1 : le tirage est fait
     * apres le test attaque > defense, avant le AND avec l'appartenance.
     * Le conditionner aux seules especes toxiques decalerait tout le RNG. */
    resolved.poison_draw_required = attack > defense;
    resolved.poisoned = resolved.poison_draw_required && poison_draw_is_zero &&
        (monster_type == 1 || monster_type == 2 || monster_type == 3 ||
         monster_type == 6 || monster_type == 7);
    *result = resolved;
    return 1;
}
