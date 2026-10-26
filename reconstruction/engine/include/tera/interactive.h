#ifndef TERA_INTERACTIVE_H
#define TERA_INTERACTIVE_H
#include "tera/encounter.h"
#include "tera/random.h"
#include "tera/input.h"
#include "tera/presentation.h"

typedef enum {
    TERA_COMBAT_ACTION, TERA_COMBAT_CONFIRM, TERA_COMBAT_WORD,
    TERA_COMBAT_DONE, TERA_COMBAT_ERROR
} TeraCombatPhase;
typedef enum {
    TERA_EVENT_INTRO, TERA_EVENT_ROUND, TERA_EVENT_ACTION,
    TERA_EVENT_HERO_HIT, TERA_EVENT_MONSTER_HIT, TERA_EVENT_ESCAPE,
    TERA_EVENT_CANCEL, TERA_EVENT_PASS, TERA_EVENT_END, TERA_EVENT_EQUIPMENT
} TeraCombatEventKind;
typedef struct {
    TeraCombatEventKind kind;
    unsigned actor;
    char letter;
    int value; /* Intro/phrase=0..2, hit=degats, escape=position, end=recompense. */
} TeraCombatEvent;
/* Le frontend peut lire la session, mais ne doit pas la modifier ni appeler
 * le moteur depuis ce callback. Aucun tirage dans le frontend. */
typedef void (*TeraCombatObserver)(void *context, const TeraCombatEvent *event);

/* Donnees de jeu fournies par l'appelant : VALMON brut, equipement personnel,
 * permutation H[1..4], SecretWords[1], monde et VRAIE colonne 7 des cartes.
 * Aucun profil, carte ou monstre n'est fabrique par le moteur. */
typedef struct TeraCombatGame {
    TeraEncounterState encounter;
    int16_t monster_values[22][6];
    uint8_t equipment[10][10];
    uint8_t helpers[4];
    char secret_word[10];
    uint8_t maps_column7[1100];
    unsigned world_id, difficulty;
    int on_alfol;
    int sound; /* A74A : les effets sonores consomment aussi le RNG du jeu. */
    int16_t position, previous_position, money, experience;
    int location_changed; /* AFD1 pose par F, independant de l'issue du tour. */
    int recruit_ready; /* AFCE partage avec le menu de combat ; ne pas
       * inventer deux drapeaux independants pour la future boucle du jeu. */
    TeraRandom random;
    const TeraInputSource *input; /* Service runtime emprunte, jamais du savedata. */
    const TeraPresentationSink *presentation; /* Service runtime, jamais serialise. */
} TeraCombatGame;

/* Une session peut rester a une frontiere clavier aussi longtemps que voulu.
 * L'absence d'entree n'annule rien et ne consomme aucun tirage.
 * Champs de progression publics pour inspection, non modifiables par l'hote. */
typedef struct {
    TeraCombatGame game;
    TeraCombatPhase phase;
    TeraRoundPreparation round;
    unsigned slot, actor, attempt, word_length, round_number;
    char letter, word[11];
    int16_t initial_monster_hp;
    int retaliation_done, action_context;
    uint8_t bonus_item; /* AFCC : objet souligne, aucun objet consomme. */
    unsigned last_attack_eligible,last_bonus_item; /* Resultat visuel, aucune regle. */
    TeraCombatObserver observe;
    void *context;
} TeraCombatSession;

/** C_838A : valider, copier game, normaliser les actifs, annoncer (RANDOM3
 * si aggressor=0), initialiser HP=VALMON1+RANDOM(VALMON6), puis avancer
 * jusqu'a une saisie ou la fin. aggressor est l'ID de presentation original
 * (0..9), sans effet sur les formules. Aucun tirage si personne n'est apte.
 * Retour 0 sans ecriture si donnees invalides (type, profils ou contexte).
 * Apres succes, conserver session.game pour reintegrer tout l'etat du jeu. */
int tera_combat_start(TeraCombatSession *session, const TeraCombatGame *game,
                      unsigned aggressor, TeraCombatObserver observer, void *context);
/** Fournir UNE touche de la lecture originale C_4277 (ASCII).
 * Minuscules converties ; confirmation O/o/Entree seulement, toute autre
 * touche annule. Mot : lettres, retour arriere, Entree ; arret automatique
 * a dix lettres, puis stockage des neuf premieres comme dans le COM.
 * Avance les effets synchrones jusqu'a la prochaine entree. Retour 0 pour
 * NULL, touche >255 ou session deja finie/en erreur, sans mutation.
 * ERROR signale une carte impossible (aucune case admissible), plutot
 * qu'une boucle infinie originale ; ne pas traiter ERROR comme une fuite. */
int tera_combat_key(TeraCombatSession *session, unsigned key);
#endif
