#ifndef TERA_MEDUZ_H
#define TERA_MEDUZ_H
#include "tera/exploration.h"
typedef enum {TERA_MEDUZ_ENTRY,TERA_MEDUZ_ENTRY_WORD,TERA_MEDUZ_MEDITATE,
    TERA_MEDUZ_INVOKE,TERA_MEDUZ_WORD,TERA_MEDUZ_DAY,TERA_MEDUZ_DONE,TERA_MEDUZ_ERROR} TeraMeduzPhase;
typedef struct {
    TeraExplorationState state;
    TeraDaySession day;
    TeraCombatSession combat;
    TeraMeduzPhase phase;
    TeraCombatObserver observe;void *context;
    int16_t backup_position;
    int fixed_combat,generic_invocation,initial_render,mandatory_command,moved,entered;
    char word[10];unsigned word_length,command;
} TeraMeduzSession;
/** TERAMAIN/pyramide et OVL00A4. enter=1 demande confirmation/mot de voyage ;
 * initial=1 rend l'arrivee de l'overlay seul (banc). Sinon une commande.
 * travelling=1 ajoute les animations TERAMAIN au retour ; 0 isole l'overlay.
 * Copie state ; suspend aux lectures/combats, conserve tous les RANDOM.
 * backup est la position d'Amarande, restauree a la sortie avec sept jours. */
int tera_meduz_start(TeraMeduzSession *session,const TeraExplorationState *state,
    unsigned command,int enter,int initial,int travelling,int16_t backup,TeraCombatObserver observer,void *context);
int tera_meduz_key(TeraMeduzSession *session,unsigned key);
/** Terminer l'overlay apres une commande residente externe (N/S/A/B ou
 * commande obligatoire apres arrivee mortelle). Aucun nouveau jour. */
int tera_meduz_resume(TeraMeduzSession *session,const TeraExplorationState *state,int16_t backup);
#endif
