#ifndef TERA_ROUND_H
#define TERA_ROUND_H

#include <stdint.h>
#include "tera/combat.h"

typedef struct {
    int16_t hp;
    int16_t magic;
} TeraCombatLimits;

/** C_4C6C, appele par l'affichage de fin de tour C_4D46 : recalculer
 * HP max = force + endurance + habilete ; MP max = intelligence + volonte.
 * Ne pas traiter cet affichage comme sans effet sur les donnees du jeu.
 * Retour 1 si valide, 0 si NULL sans ecriture ; actor n'est pas modifie. */
int tera_combat_actor_limits(const TeraCombatActor *actor, TeraCombatLimits *result);

/* Personnages originaux 0..9, heros inclus a l'indice 0. Le statut est
 * conserve comme chaine de deux caracteres, sans enum qui effacerait
 * un etat original encore non interprete. "HS" est le seul etat exclu. */
typedef struct {
    uint8_t active;      /* Booleen original : 0 ou 1. */
    uint8_t agility;
    int16_t hp;
    int16_t magic;
    int16_t magic_limit;
    int16_t hp_limit;
    char condition[3];
} TeraRoundActor;

typedef struct {
    unsigned count;      /* Tous les aptes, 1..10, meme au-dela des quatre IDs. */
    uint8_t actor_ids[4]; /* Ordre croissant des IDs, pas tri par agilite. */
    int16_t mean_agility;
    int party_first;     /* Moyenne strictement > initiative monstre + jet -2. */
} TeraRoundPreparation;

/** C_7CC8 : si HP <1 et etat different de HS, mettre HP=0 et etat=HS.
 * Un HS deja marque n'est ni ressuscite ni renormalise, meme avec HP !=0.
 * Retour 1 si l'etat final n'est pas HS, 0 si HS ou pointeur NULL.
 * EM avec HP positifs reste EM ; aucun degat de poison ici. */
int tera_combat_update_actor_condition(TeraRoundActor *actor);

/** C_9757..9977 : choisir active && etat !=HS, dans l'ordre 0..9,
 * regenerer les quatre premiers (1 MP si MP < limite), puis calculer
 * moyenne et initiative.
 * initiative_draw est RANDOM(5), donc 0..4 ; RNG externe et non remplace.
 * Les etats doivent avoir ete mis a jour AVANT le premier tour du combat
 * et a la FIN du tour precedent. Ne pas exclure quelqu'un sur ses HP seuls.
 * Retour 1 si valide. Retour 0 si NULL, jet invalide ou groupe vide ;
 * dans ce cas ni actors ni result ne sont modifies. Seuls les quatre premiers
 * regenerent et contribuent a la somme, divisee par le nombre TOTAL.
 * Cette fonction ne choisit pas encore de cible ni n'execute les attaques. */
int tera_combat_prepare_round(TeraRoundActor actors[10],
                              int16_t monster_initiative,
                              unsigned initiative_draw,
                              TeraRoundPreparation *result);

/** Selection de cible a 9992/99F9 : RANDOM(count) indexe la liste du debut
 * du tour. La liste n'est pas reconstruite au milieu des attaques.
 * Retour 1 et ID si cible 0..3 presente ; 0 sans ecriture pour un jet
 * invalide ou une cible 4..9 : le CASE original ne lance aucune attaque. */
int tera_combat_select_round_target(const TeraRoundPreparation *round,
                                    unsigned target_draw, uint8_t *actor_id);

#endif
