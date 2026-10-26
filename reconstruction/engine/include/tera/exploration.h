#ifndef TERA_EXPLORATION_H
#define TERA_EXPLORATION_H
#include "tera/world.h"

typedef struct {
    TeraWorldState world;
    int sound, moved, town_found;
    unsigned town_id;
    int force_day, encounter_suppressed;
    int16_t city_backup_position; /* AFD5 de TERAMAIN, preserve hors entree confirmee. */
    int navigation_overlay; /* 0=002E, 1=0049, 2=0069/Seth ; distinct de WorldId. */
    int chest_open; /* BP_03 des overlays, efface au prochain rendu. */
    int information_context; /* A750, pose par C6073 ; distinct de 9418. */
} TeraExplorationState;

typedef struct {
    unsigned valid,count,horizon_kind,horizon_height;
    int position;unsigned command;
    struct {unsigned x,y;} stars[16];
    struct {unsigned x,y,sprite;} ground[65];
    unsigned mountain_count;unsigned short mountains[35];
} TeraLandscape;

typedef struct {
    int moved, near_mountain,npc_present;
    unsigned scene, random_calls,npc_variant;
    TeraLandscape landscape; /* Resultats des tirages graphiques existants. */
} TeraMoveReport;

/** OVL00FB : deplacement Amarande 2/4/6/8, montagnes, bouclage 1000..1899,
 * detection des vingt lieux et calendrier RNG du decor original.
 * Ne traite pas les entrees de lieux. Retour 0 sans mutation pour mauvais
 * contexte/touche. Un obstacle est un succes d'appel avec moved=0.
 * Le passage du temps est une operation separee, appelee seulement si moved.
 * La fuite peut produire 0..762 : ces positions restent admissibles.
 * -1 est admis avant le pas nord automatique de TERAMAIN. Seuls sud/ouest
 * appliquent le bouclage inferieur, comme dans OVL00FB. */
int tera_exploration_move(TeraExplorationState *state, unsigned key, TeraMoveReport *report);

/** Tirages de presentation C9B4E, dont les 120 points d'Arioch (795E).
 * Aucun combat ni changement de drapeaux. type=1..22, compteur fourni.
 * L'animation 15 teste game->input apres chacune des six images,
 * avec purge initiale. Sans service : chemin original sans touche. */
int tera_monster_presentation_random(TeraCombatGame *game,unsigned type,unsigned *calls);

typedef enum { TERA_DAY_RUNNING, TERA_DAY_COMBAT, TERA_DAY_DONE, TERA_DAY_ERROR } TeraDayPhase;
typedef struct {
    TeraExplorationState state;
    TeraCombatSession combat;
    TeraDayPhase phase;
    unsigned stage, encounters, random_calls;
    int day_advanced, level_advanced;
    int fixed_encounter_pending;
} TeraDaySession;

/** C_A0DD : rencontres, jour, regeneration/poison, niveau/caracteristiques
 * et competences du groupe. Suspend au menu d'un vrai combat C ; pas de
 * combat resolu arbitrairement par l'hote. Aucun tirage en attente de touche.
 * Donnees VALMON/H/mots/cartes requises pour les combats eventuels.
 * L'animation du monstre 15 execute six images (poll clavier sans touche).
 * Retour 0 si contexte invalide, sans mutation ; voir phase pour erreurs
 * techniques pendant l'execution (division DOS impossible, groupe sans skill). */
int tera_day_start(TeraDaySession *session, const TeraExplorationState *state,
                   TeraCombatObserver observer, void *context);
/** OVL002E/0049 : monstre de la case AVANT C_A0DD. Une victoire efface
 * Maps[Position courante][6] et marque Arioch selon l'overlay appelant.
 * Puis reprendre le calendrier quotidien, avec AFD2 pose par C9B4E. */
int tera_day_start_location(TeraDaySession *session,const TeraExplorationState *state,
                            TeraCombatObserver observer,void *context);
/** Fournir une touche au combat en attente ; son resultat est reintegre
 * AVANT les operations suivantes du jour. Retour 0 hors phase COMBAT. */
int tera_day_key(TeraDaySession *session, unsigned key);
#endif
