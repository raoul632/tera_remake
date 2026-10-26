#ifndef TERA_NAVIGATION_H
#define TERA_NAVIGATION_H
#include "tera/exploration.h"
#include "tera/chest.h"

typedef enum {
    TERA_NAV_GATE, TERA_NAV_CONFIRM, TERA_NAV_DAY, TERA_NAV_DONE, TERA_NAV_ERROR, TERA_NAV_WORD, TERA_NAV_CHEST
} TeraNavigationPhase;
typedef struct {
    TeraExplorationState state;
    TeraDaySession day;
    TeraChestSession chest;
    TeraNavigationPhase phase;
    unsigned key, wall, render_world;
    int delta, moved, changed_level, enter_city, invocation_pending;
    int temple_entry_render,mandatory_temple_command;
    char word[11];
    unsigned word_length;
    int16_t backup_position;
    unsigned hints[2], hint_count, healed_mask;
    TeraCombatObserver observe;
    void *context;
} TeraNavigationSession;

/** C7EF6/C7DBD et OVL002E/0049 : mouvement RELATIF au sens Position-Previous.
 * key=0 redessine l'arrivee, 2/4/6/8 deplacent ; D/M passent les niveaux,
 * L lit les indices, I invoque a l'autel 103. initial_move reproduit A742
 * lors d'une fuite/transition : pas de deuxieme mouvement ajoute.
 * Une porte 2 attend O uniquement ; une porte 3 attend U uniquement si
 * Keys[world]!=10. Le deverrouillage transforme toutes les portes 3 de la
 * case courante en 2, sans modifier la case voisine ni consommer la cle.
 * O/P suspendent a l'ouverture/ramassage via le module chest. Retour 0
 * sans mutation si contexte/commande invalide. */
int tera_navigation_start(TeraNavigationSession *session,const TeraExplorationState *state,
                           unsigned key,int initial_move,int16_t backup_position,
                           TeraCombatObserver observer,void *context);
/** TERAMAIN : E confirme la Cite (lieu 2 : Position=1, World=0) ou Seth
 * (lieu 4 : Position=800, World=8, overlay=2). Previous=Position-1.
 * Conserve BackupPosition ; l'entree mortelle de Seth laisse encore une
 * commande mandatory_temple_command avant UNTIL, comme CC23/CC48. */
int tera_navigation_enter(TeraNavigationSession *session,const TeraExplorationState *state,
                           TeraCombatObserver observer,void *context);
/** TERAMAIN apres retour de l'overlay : restaurer BackupPosition et World=11.
 * Exige sortie par Position=0 ou defaite ; pas de teleportation volontaire.
 * Conserve Previous et les donnees des lieux comme dans le binaire. */
int tera_navigation_leave(TeraExplorationState *state,int16_t backup_position);
/** Confirmer porte/interaction ou fournir une touche au combat imbrique.
 * Les indices sont des IDs 1..38 de world.hints, 0 pour MESSAGE original 70,
 * 100/101 pour les inscriptions propres a Seth.
 * Retour 0 en dehors d'une phase d'attente, sans action speculative. */
int tera_navigation_key(TeraNavigationSession *session,unsigned key);
#endif
