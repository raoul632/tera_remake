#ifndef TERA_DIALOGUE_H
#define TERA_DIALOGUE_H
#include "tera/exploration.h"
#include "tera/inventory.h"
#include "tera/recruitment.h"

typedef enum { TERA_DIALOGUE_MENU,TERA_DIALOGUE_CONFIRM,TERA_DIALOGUE_TRANSFER,
    TERA_DIALOGUE_DONE,TERA_DIALOGUE_ERROR } TeraDialoguePhase;
typedef struct {
    TeraExplorationState state;
    TeraInventorySession transfer;
    TeraRecruitSession recruitment;
    TeraDialoguePhase phase;
    unsigned command,choice,hint_count,hints[4],available_mask,response_variant;
    int response_id; /* -1=aucune phrase ; ID 22/23 des fichiers MESSAGE*. */
    int message; /* 0=aucun,1=ellipsis,2=MESSAGE46 ; IDs d'indices 1..18. */
    int extra_actor;
} TeraDialogueSession;
/** CAFE8/N -> C_AF EF : menu I informations, A acheter, V vendre,
 * D donner, S solliciter. Acheter/vendre sont des phrases dans ce resident,
 * aucune transaction inventee. S direct est la seduction CAFE8/S.
 * Suspend aux confirmations/transferts, aucun jour ajoute ; conserve les
 * tirages de la melodie de seduction. Copie state, sans modifier l'entree. */
int tera_dialogue_start(TeraDialogueSession *s,const TeraExplorationState *state,unsigned command);
int tera_dialogue_key(TeraDialogueSession *s,unsigned key);
#endif
