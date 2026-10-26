#ifndef TERA_CHEST_H
#define TERA_CHEST_H
#include "tera/exploration.h"
#include "tera/inventory.h"
typedef enum { TERA_CHEST_CONFIRM,TERA_CHEST_ACTOR,TERA_CHEST_DONE,TERA_CHEST_ERROR } TeraChestPhase;
typedef struct {
    TeraExplorationState state;
    TeraChestPhase phase;
    unsigned command,actor,pending_item;
    unsigned reward; /* Montant du coffre, avant arithmetique signee 16 bits. */
    int trap; /* -1=aucun, 0=gaz,1=explosion,2=cyanure,3=lame,4=dejoue. */
    TeraItemOutcome outcome;
} TeraChestSession;
/** O/P des overlays 002E/0049/0069 : ouverture distincte du ramassage, drapeau
 * local conserve jusqu'au prochain rendu. Aucun jour ni rencontre ajoute.
 * Les jets du piege et de l'explosion/son restent dans le RNG du monde.
 * overlay=2 conserve les bornes et l'ordre des degats propres a Seth. */
int tera_chest_start(TeraChestSession *session,const TeraExplorationState *state,unsigned command);
int tera_chest_key(TeraChestSession *session,unsigned key);
#endif
