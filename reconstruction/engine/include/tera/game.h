#ifndef TERA_GAME_H
#define TERA_GAME_H
#include "tera/amarande.h"
#include "tera/navigation.h"
#include "tera/alfol.h"
#include "tera/meduz.h"
#include "tera/commerce.h"
#include "tera/space.h"
#include "tera/dialogue.h"
#include "tera/lifecycle.h"
#include "tera/ending.h"

/* READY seul accepte une commande de jeu ; les autres phases attendent une
 * lecture du module actif. COMPLETE/QUIT/HALTED/ERROR sont terminaux. */
typedef enum {
    TERA_GAME_READY, TERA_GAME_CREATE, TERA_GAME_DAY, TERA_GAME_AMARANDE,
    TERA_GAME_NAVIGATION, TERA_GAME_ALFOL, TERA_GAME_MEDUZ, TERA_GAME_SHOP,
    TERA_GAME_SPACE, TERA_GAME_DIALOGUE, TERA_GAME_INVENTORY,
    TERA_GAME_LIFE, TERA_GAME_ENDING, TERA_GAME_COMPLETE, TERA_GAME_QUIT,
    TERA_GAME_HALTED, TERA_GAME_ERROR
} TeraGamePhase;

/* Globals TERAMAIN hors record du monde, necessaires aussi a une reprise.
 * mandatory_command : 0=aucun, 1=Seth, 2=batiments Alfol, 3=Meduz.
 * Une arrivee mortelle attend encore UNE commande avant le test UNTIL. */
typedef struct {
    int16_t status_actor, space_backup_position, meduz_backup_position, pirates;
    unsigned npc_variant, mandatory_command;
} TeraGameContext;

typedef struct {
    TeraExplorationState state; /* Derniere frontiere d'operation terminee. */
    TeraGameContext control;
    TeraGamePhase phase;
    TeraGamePhase completed_from; /* Valide seulement en COMPLETE : LIFE/ENDING. */
    TeraCombatObserver observe;
    void *observer_context;
    TeraMoveReport move;
    unsigned after_mandatory;
    union {
        TeraCreationSession creation;
        TeraDaySession day;
        TeraAmarandeSession amarande;
        TeraNavigationSession navigation;
        TeraAlfolSession alfol;
        TeraMeduzSession meduz;
        TeraShopSession shop;
        TeraSpaceSession space;
        TeraDialogueSession dialogue;
        TeraInventorySession inventory;
        TeraLifeSession life;
        TeraEndingSession ending;
    } active;
} TeraGameSession;

/** Reprendre un monde dont le heros est deja cree. Copie les donnees et
 * execute les transitions automatiques jusqu'a une lecture ou une fin.
 * control=NULL initialise les globals pour Amarande ; un contexte explicite
 * est requis pour reprendre Meduz/Alfol (positions de retour/pirates).
 * Les tables de monstres, H, noms/mots/cartes sont fournies par l'appelant.
 * Aucun fichier, horloge, Python, SDL ou attente bloquante dans cette API.
 * Retour 0 pour contexte invalide sans modifier session ; erreur technique
 * d'un module apres demarrage => phase ERROR. Ne pas rappeler sur soi-meme. */
int tera_game_start(TeraGameSession *session,const TeraExplorationState *state,
    const TeraGameContext *control,TeraCombatObserver observer,void *context);
/** Restaurer une frontiere READY deja stabilisee, sans rejouer settle,
 * le decor, les arrivees, le RNG ou les callbacks. Exige control explicite.
 * Les services runtime sont rebranches par l'appelant. Retour 0 sans
 * mutation pour contexte incoherent ou transitions automatiques en attente.
 * API de reprise technique, pas une commande supplementaire du jeu. */
int tera_game_restore_ready(TeraGameSession *session,const TeraExplorationState *state,
    const TeraGameContext *control,const TeraInputSource *input,
    TeraCombatObserver observer,void *context);
/** Commencer la creation sur un monde deja genere par world.h ; puis
 * appliquer le heros et l'arrivee TERAMAIN, sans regenerer ni resemer. */
int tera_game_create(TeraGameSession *session,const TeraExplorationState *world,
    TeraCombatObserver observer,void *context);
/** Une commande READY ou une lecture du module actif (octet 0..255).
 * Minuscules normalisees par le routage, sans alterer la saisie des modules.
 * Retour 0 pour commande non prise en charge, touche invalide ou fin :
 * aucune mutation. Une touche acceptee peut mener a plusieurs transitions,
 * mais jamais consommer la prochaine lecture sans appel explicite.
 * Pendant l'attente, les details courants sont dans active.<module> ; state
 * n'est publie qu'au retour du module. Observer combat transmis tel quel. */
int tera_game_key(TeraGameSession *session,unsigned key);
/** Evenement horodate du tir spatial actif ; contrat de space.h.
 * Reintegre ensuite vol, arrivee/retour et transitions generales.
 * Ne pas melanger avec tera_game_key pour la resolution du meme tir. */
int tera_game_space_input_at(TeraGameSession *session,unsigned elapsed_ms,int key);

typedef enum {
    TERA_PROMPT_COMMAND, TERA_PROMPT_MENU, TERA_PROMPT_CONFIRM,
    TERA_PROMPT_WORD, TERA_PROMPT_ACTOR, TERA_PROMPT_SLOT, TERA_PROMPT_PAUSE,
    TERA_PROMPT_NAME, TERA_PROMPT_GENDER, TERA_PROMPT_SPLIT,
    TERA_PROMPT_BONUS, TERA_PROMPT_KEEP, TERA_PROMPT_GATE,
    TERA_PROMPT_DIGIT, TERA_PROMPT_SHOT, TERA_PROMPT_TERMINAL
} TeraPromptKind;
typedef struct {
    TeraPromptKind kind;
    TeraPresentationSource source;
    unsigned command,choice,actor,slot,remaining,pool,confirm_key;
    /* Sous-commande residente H ; information de presentation uniquement. */
    unsigned accepts_enter; /* Seulement CONFIRM/GATE ; certaines lectures exigent O/U. */
    unsigned selecting_target; /* Inventaire : lecture du destinataire. */
    char input[11]; /* Saisie courante CP437/ASCII, jamais un mot secret. */
} TeraGamePrompt;
/** Rattacher/detacher le service emprunte a toutes les sous-sessions actives.
 * Aucun evenement rejoue, aucun appel au service, aucun effet RNG. Peut etre
 * utilise apres chargement ou pendant un combat. session doit etre initialisee. */
int tera_game_set_presentation(TeraGameSession *session,const TeraPresentationSink *sink);

/* Vue empruntee de la lecture courante, sans regle de presentation ni SDL.
 * subphase=-1 hors module. Les pointeurs sont valides seulement jusqu'a la
 * prochaine mutation de session ; ne pas les conserver dans un renderer.
 * exploration reste le dernier monde du module ; combat/creation exposent
 * leurs donnees en cours lorsque ce monde n'est pas encore reintegre. */
typedef enum {TERA_SCENE_AMARANDE,TERA_SCENE_PLACE,TERA_SCENE_RUINS,
    TERA_SCENE_INTERIOR,TERA_SCENE_ALFOL_SURFACE,TERA_SCENE_ALFOL_INTERIOR,
    TERA_SCENE_MEDUZ,TERA_SCENE_UNKNOWN} TeraSceneKind;
typedef enum {TERA_FEATURE_NONE,TERA_FEATURE_CHEST,TERA_FEATURE_OPEN_CHEST,
    TERA_FEATURE_UP,TERA_FEATURE_DOWN,TERA_FEATURE_ALTAR,TERA_FEATURE_WRITING,
    TERA_FEATURE_MEDITATION,TERA_FEATURE_INVOCATION,TERA_FEATURE_OFFICE,
    TERA_FEATURE_RADIATION} TeraSceneFeature;
typedef struct {
    TeraSceneKind kind;TeraSceneFeature feature;
    unsigned world,place,variant,left,front,right,npc_mask,visible_item,palette;
    unsigned ambient_npc,npc_variant,place_npc,writing_kind;
    int position,heading_valid;
    const TeraLandscape *landscape; /* Emprunte, facultatif apres chargement. */
} TeraSceneView;

typedef struct {
    TeraGamePhase phase;
    int subphase;
    TeraGamePrompt prompt;
    const TeraExplorationState *exploration;
    const TeraCharacterState *characters;
    const TeraCombatSession *combat;
    const TeraCreationSession *creation;
    const TeraSpaceSession *space;
    const TeraEndingSession *ending;
    TeraSceneView scene;
} TeraGameView;
/** Retour 0 pour NULL, sinon publie la vue sans effet sur etat/RNG/callbacks. */
int tera_game_view(const TeraGameSession *session,TeraGameView *view);
#endif
