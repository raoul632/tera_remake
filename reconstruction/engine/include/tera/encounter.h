#ifndef TERA_ENCOUNTER_H
#define TERA_ENCOUNTER_H

#include "tera/round.h"

/* Entrees a la frontiere du menu : raw_action doit provenir de C_842A
 * deja porte. F signifie fuite CONFIRMEE, dont le changement de lieu
 * reste a effectuer par l'appelant. Aucun menu ou deplacement invente. */
typedef struct {
    char letter;
    int16_t raw_action;
    int secret_word_matches;
} TeraEncounterChoice;

typedef struct {
    TeraRoundActor actors[10];
    TeraCombatActor attributes[10];
    TeraMonsterReaction reaction;
    unsigned monster_type;
    int16_t monster_hp;
    int victory;
    int fled;
    int party_available;
} TeraEncounterState;

typedef struct {
    TeraEncounterChoice choices[10];
    int16_t raw_defense[10]; /* C_842A(cible,7/8), a fournir pour la cible. */
    int16_t monster_initiative; /* VALMON[type,5]. */
    unsigned initiative_draw;
    unsigned target_draw;
    int poison_draw_is_zero;
} TeraEncounterRoundInputs;

typedef struct {
    unsigned event_count;
    int events[5];          /* ID+1 pour une action, -(ID+1) pour la riposte. */
    int target_draw_required;
    int poison_draw_required;
    int continue_combat;
} TeraEncounterRoundResult;

/** Executer un tour numerique de C_9757..9AAB, en conservant le calendrier.
 * Preparation, attaques, riposte, HS puis recalcul des maxima sont integres.
 * choices/defenses sont des sorties fournies aux frontieres du menu/jet,
 * jamais des tirages generes avant leur tour par cette fonction.
 * target_draw n'est lu que si une riposte a lieu ; le rapport dit quels
 * tirages externes ont ete requis (hors tirages de C_842A et du menu).
 * Ne pas mettre HS juste apres un coup : C_9567 le fait en fin de tour.
 * F arrete les choix suivants, mais peut laisser la riposte du tour.
 * Retour 1 si valide ; 0 sans modifier state/result si NULL, drapeaux de
 * fin deja poses, type invalide, groupe vide ou cible hors borne.
 * Pas de relocation de fuite, initialisation de combat ni recompense. */
int tera_combat_execute_round(TeraEncounterState *state,
                              const TeraEncounterRoundInputs *inputs,
                              TeraEncounterRoundResult *result);

typedef struct {
    int granted;
    int16_t money;
    int16_t experience;
} TeraCombatRewards;

/** Sortie 9AAE..9B42 : si victoire ET groupe encore apte, ajouter
 * RANDOM(50)+2 a l'argent et HP INITIAUX du monstre a l'experience.
 * money_draw vaut 0..49, ignore si aucune recompense ; le rapport granted
 * indique si ce tirage etait requis. Arithmetique signee 16 bits conservee.
 * Retour 0 si NULL ou jet requis invalide, sans modifier result. */
int tera_combat_victory_rewards(int victory, int party_available,
                                int16_t money, int16_t experience,
                                int16_t initial_monster_hp, unsigned money_draw,
                                TeraCombatRewards *result);

typedef struct {
    int16_t position;
    int16_t previous_position;
    unsigned draws_used;
} TeraCombatEscape;

/** C_86E1, relocation apres confirmation de fuite.
 * world_id 0..10 : candidat world*100 + RANDOM(90)+5, repeter jusqu'a
 * maps_column7[candidat] !=0 ; previous_position = candidat-1.
 * world_id 11 : on_alfol => RANDOM(99)+1, sinon 3*RANDOM(255) ; dans ces
 * deux cas previous_position reste old_previous_position, carte ignoree.
 * draws est la suite de valeurs RANDOM fournies, non un algorithme RNG.
 * maps_column7 contient les 1100 septiemes colonnes originales, sans leur
 * attribuer une signification plus generale de "case marchable".
 * Retour 0 si entree invalide, borne de jet incorrecte ou suite epuisee
 * avant candidat accepte ; aucun resultat partiel. Retour 1 sinon.
 * Les sons, affichages et drapeaux du menu restent externes. */
int tera_combat_escape(unsigned world_id, int on_alfol,
                        int16_t old_previous_position,
                        const uint8_t maps_column7[1100],
                        const unsigned *draws, unsigned draw_count,
                        TeraCombatEscape *result);

#endif
