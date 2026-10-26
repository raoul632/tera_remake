#ifndef TERA_INVENTORY_H
#define TERA_INVENTORY_H
#include "tera/creation.h"

typedef enum { TERA_ITEM_NONE,TERA_ITEM_ADDED,TERA_ITEM_FULL,TERA_ITEM_HS,
    TERA_ITEM_EMPTY,TERA_ITEM_NOT_POTION,TERA_ITEM_CONSUMED,TERA_ITEM_DROPPED,
    TERA_ITEM_GIVEN,TERA_ITEM_CANCELLED,TERA_ITEM_INACTIVE } TeraItemOutcome;
typedef enum { TERA_INV_MENU,TERA_INV_CONFIRM,TERA_INV_SUBJECT,TERA_INV_SLOT,
    TERA_INV_TARGET,TERA_INV_SUBMENU,TERA_INV_DONE,TERA_INV_ERROR } TeraInventoryPhase;
typedef struct {
    TeraCharacterState state;
    TeraInventoryPhase phase;
    unsigned command,subject,slot,target,pending_item;
    TeraItemOutcome outcome;
} TeraInventorySession;

/** C4B55 : A designe toujours le heros ; B..D sont les compagnons actifs
 * dans l'ordre du roster (HS inclus). Retour -1 : reprendre la lecture,
 * aucune selection fictive ni tirage. */
int tera_inventory_pick_actor(const TeraCharacterState *state,unsigned key);
/** C4D46 : l'affichage recalcule les maxima des quatre premiers actifs,
 * meme HS. Aucun soin ni changement de disponibilite du groupe. */
void tera_inventory_refresh_limits(TeraCharacterState *state);
/** CAA13 : deposer pending dans le PREMIER emplacement nul, sans compactage.
 * HS/inventaire plein conserve pending ; succes le met a zero. */
TeraItemOutcome tera_inventory_acquire(TeraCharacterState *state,unsigned actor,unsigned *pending);
/** Commandes originales A (abandonner), B (boire), N puis D (donner).
 * Suspendre a chaque lecture, y compris reprises A-D/chiffre. Pas de jour.
 * Les autres dialogues de N restent hors de cette API. */
int tera_inventory_start(TeraInventorySession *session,const TeraCharacterState *state,unsigned command);
int tera_inventory_key(TeraInventorySession *session,unsigned key);
#endif
