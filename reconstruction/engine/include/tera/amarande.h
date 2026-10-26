#ifndef TERA_AMARANDE_H
#define TERA_AMARANDE_H
#include "tera/exploration.h"
typedef enum {TERA_AM_CONFIRM,TERA_AM_WORD,TERA_AM_COMBAT,TERA_AM_HALT,
    TERA_AM_PAUSE,TERA_AM_DAY,TERA_AM_DONE,TERA_AM_ERROR} TeraAmarandePhase;
typedef struct {
    TeraExplorationState state;TeraCombatSession combat;TeraDaySession day;
    TeraAmarandePhase phase;TeraCombatObserver observe;void *context;
    unsigned command,choice,kind,npc,shown_actor,outcome,word_length,extra_variant;
    unsigned response_id,response_variant,response_count;
    int16_t east,north,status_actor;
    int quit_requested;char word[10];
} TeraAmarandeSession;
/** Commandes residentes C/I/H/#, reponses generiques et panneaux L de
 * TERAMAIN. Copie l'etat ; aucun deplacement/jour hors H/D ou H/M.
 * Le contexte D9B66 est transporte pour les alias d'Alfol deja documentes. */
int tera_amarande_start(TeraAmarandeSession *s,const TeraExplorationState *state,
    unsigned command,int16_t status_actor,TeraCombatObserver observer,void *context);
/** Consommer une seule lecture originale. Les sous-sessions de combat/jour
 * restent suspendues dans s ; DONE seul autorise l'export du frontend.
 * Retourne 0 pour une touche hors octet ou une session deja terminee. */
int tera_amarande_key(TeraAmarandeSession *s,unsigned key);
#endif
