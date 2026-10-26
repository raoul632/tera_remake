#ifndef TERA_WORLD_H
#define TERA_WORLD_H
#include "tera/creation.h"

/* Etat commun aux generateurs et interactions. Les sept colonnes restent
 * celles du record DOS : quatre murs, contenu, monstre et type de case.
 * Ne pas deduire une traversabilite universelle de la colonne 7. */
typedef struct {
    TeraCharacterState characters;
    uint8_t maps[1100][7];
    int16_t towns[20], locations[10], extra_locations[10];
    int16_t horizontal_mountains[9], vertical_mountains[25];
    int16_t hidden_positions[2], arioch_position, alfol_position;
    char hints[38][40], secret_words[4][10]; /* Octets CP437, termines. */
    uint8_t has_map[11], tasks_done[4];
    int fast_moves, initial_flag, mental_extension;
    int16_t tasks_counter;
    int16_t placement_result_residue; /* Mot non initialise du resultat BA2F.
      * L'hote fournit le contexte DOS de reference ; -211 pour SP=FFFE
      * et appels standards du programme. Ne pas inventer un reroll. */
} TeraWorldState;

typedef struct {
    uint32_t random_calls, map_attempts, placement_collisions;
    int exhausted;
} TeraWorldReport;

/** OVL0000 : cartes, rencontres, contenus et marqueurs obligatoires.
 * difficulty=0..9 ; graine deja initialisee par l'hote, comme apres
 * RANDOMIZE. max_draws est une limite technique pour interrompre une
 * execution, jamais un changement de probabilites ou un reroll alternatif.
 * Retour 1 si generation complete ; 0 sans mutation de world si NULL,
 * difficulte invalide, budget epuise. Rapport publie meme si budget epuise.
 * Les cartes refusees consomment TOUS leurs tirages avant la suivante. */
int tera_world_generate_maps(TeraWorldState *world, unsigned difficulty,
                              uint32_t max_draws, TeraWorldReport *report);
/** OVL0010 entier apres les cartes : profils, montagnes, villes, compagnons,
 * indices, mots secrets, H et placements finaux. Meme calendrier RNG.
 * Ne pas lancer sur des cartes de test sans cases admissibles.
 * Retour 0 sans mutation si budget epuise/donnees invalides ; rapport detaille.
 * Les collisions originales de placement seront conservees selon les
 * residus de resultat du binaire documentes dans la specification. */
int tera_world_initialize_places(TeraWorldState *world, const TeraNameVariants *variants,
                                  uint32_t max_draws, TeraWorldReport *report);
/** Reintegrer la creation du heros APRES generation, sans perdre cartes,
 * placements, aides, mots, flags et contexte. Exige session terminee. */
int tera_world_apply_creation(TeraWorldState *world, const TeraCreationSession *session);
#endif
