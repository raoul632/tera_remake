#include "tera/combat.h"
#include <stdio.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "check failed at line %d\n", __LINE__); return 1; \
} } while (0)

int main(void)
{
    /* Oracle relu dans C_8CE3 : raw=180, resistance A=7, B=11, MP=9.
     * Valeurs calculees sur papier depuis les divisions et C_8B5E,
     * independamment du Python historique et du code C teste. */
    static const struct {
        int16_t category;
        char letter;
        int16_t damage, magic_after;
    } vectors[] = {
        {0, 'L', 8, 9}, {0, 'T', 8, 9}, {0, 'J', 7, 6}, {0, 'A', 7, 6},
        {1, 'T', 10, 9}, {1, 'J', 5, 6}, {1, 'A', 5, 6},
        {3, 'D', 9, 9}, {3, 'J', 5, 6},
        {4, 'P', 9, 9}, {4, 'L', 5, 9},
        {5, 'C', 17, 9}, {5, 'A', 4, 3},
        {6, 'A', 4, 6}, {6, 'T', 10, 9}
    };
    TeraMonsterReaction monster = {7, 11, 0};
    TeraHeroAttackResult hero;
    TeraMonsterAttackResult retaliation;
    unsigned i, category, letter;
    const char letters[] = "LTJADPC";
    for (i = 0; i < sizeof vectors / sizeof vectors[0]; ++i) {
        monster.reaction_category = vectors[i].category;
        CHECK(tera_combat_resolve_hero_attack(&monster, vectors[i].letter,
                                             180, 9, 30, 1, &hero));
        CHECK(hero.eligible && hero.damage == vectors[i].damage);
        CHECK(hero.magic_after == vectors[i].magic_after);
        CHECK(hero.monster_hp_after == 30 - vectors[i].damage && !hero.defeated);
    }
    /* Toutes les autres paires categorie/lettre restent sans effet.
     * Ceci detecte notamment une action valable dans toutes les categories. */
    for (category = 0; category <= 6; ++category)
        for (letter = 0; letter < sizeof letters - 1; ++letter) {
            int expected_eligible = 0;
            for (i = 0; i < sizeof vectors / sizeof vectors[0]; ++i)
                if (vectors[i].category == (int16_t)category &&
                    vectors[i].letter == letters[letter])
                    expected_eligible = 1;
            monster.reaction_category = (int16_t)category;
            CHECK(tera_combat_resolve_hero_attack(&monster, letters[letter],
                                                 180, 9, 30, 1, &hero));
            CHECK(hero.eligible == expected_eligible);
            if (!expected_eligible)
                CHECK(hero.damage == 0 && hero.magic_after == 9 && hero.monster_hp_after == 30);
        }
    /* Seuil magique : 2 refuse, 3 accepte et paie, meme sur un rate.
     * Le cout depend de la branche, pas de la reussite du jet. */
    monster.reaction_category = 0;
    CHECK(tera_combat_resolve_hero_attack(&monster, 'J', 0, 2, 3, 0, &hero));
    CHECK(!hero.eligible && hero.magic_after == 2 && !hero.defeated);
    CHECK(tera_combat_resolve_hero_attack(&monster, 'J', 0, 3, 2, 0, &hero));
    CHECK(hero.eligible && hero.magic_after == 0 && hero.damage == 0 && hero.defeated);
    monster.reaction_category = 5;
    CHECK(tera_combat_resolve_hero_attack(&monster, 'A', 180, 5, 30, 1, &hero));
    CHECK(!hero.eligible && hero.magic_after == 5);
    CHECK(tera_combat_resolve_hero_attack(&monster, 'A', 180, 6, 30, 1, &hero));
    CHECK(hero.eligible && hero.magic_after == 0);
    CHECK(tera_combat_resolve_hero_attack(&monster, 'C', 180, 9, 30, 0, &hero));
    CHECK(!hero.eligible && hero.damage == 0);
    /* CMP signe et AX 16 bits : subtraction qui deborde, jamais un clamp. */
    monster.reaction_category = 1; monster.resistance_a = -32768;
    CHECK(tera_combat_resolve_hero_attack(&monster, 'T', 32767, 9, 30, 0, &hero));
    CHECK(hero.damage == -8737 && hero.monster_hp_after == 8767);
    /* Defense physique jusqu'a 14, mentale des 15. 198/11=18, /9=22.
     * Avec HP=20, attaque=24 : degats respectifs 3 et 1. */
    CHECK(tera_combat_resolve_monster_attack(14, 20, 198, 1, &retaliation));
    CHECK(retaliation.damage == 3 && retaliation.poison_draw_required && !retaliation.poisoned);
    CHECK(tera_combat_resolve_monster_attack(15, 20, 198, 1, &retaliation));
    CHECK(retaliation.damage == 1 && retaliation.poison_draw_required && !retaliation.poisoned);
    CHECK(tera_combat_resolve_monster_attack(1, 20, 264, 1, &retaliation));
    CHECK(retaliation.damage == 0 && !retaliation.poison_draw_required && !retaliation.poisoned);
    for (i = 1; i <= 22; ++i) {
        int toxic = i == 1 || i == 2 || i == 3 || i == 6 || i == 7;
        CHECK(tera_combat_resolve_monster_attack(i, 20, 0, 1, &retaliation));
        CHECK(retaliation.damage == 9 && retaliation.poison_draw_required);
        CHECK(retaliation.poisoned == toxic);
        CHECK(tera_combat_resolve_monster_attack(i, 20, 0, 0, &retaliation));
        CHECK(!retaliation.poisoned);
    }
    CHECK(tera_combat_resolve_monster_attack(1, 32767, 0, 1, &retaliation));
    CHECK(retaliation.damage == 0 && !retaliation.poison_draw_required);
    hero.damage = 123;
    CHECK(!tera_combat_resolve_hero_attack(NULL, 'T', 180, 9, 30, 1, &hero));
    CHECK(hero.damage == 123);
    retaliation.damage = 123;
    CHECK(!tera_combat_resolve_monster_attack(0, 20, 0, 1, &retaliation));
    CHECK(retaliation.damage == 123);
    CHECK(!tera_combat_resolve_monster_attack(23, 20, 0, 1, &retaliation));
    puts("C combat reaction source vectors passed");
    return 0;
}
