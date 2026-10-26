#ifndef TERA_ALFOL_H
#define TERA_ALFOL_H
#include "tera/exploration.h"
#include "tera/inventory.h"
#include "tera/recruitment.h"
typedef enum { TERA_ALFOL_ENTER,TERA_ALFOL_REBOARD,TERA_ALFOL_MEDIA,TERA_ALFOL_GATE,
    TERA_ALFOL_DAY,TERA_ALFOL_WORD,TERA_ALFOL_DONE,TERA_ALFOL_RETURN,TERA_ALFOL_ERROR } TeraAlfolPhase;
typedef struct {
    TeraExplorationState state;
    TeraDaySession day;
    TeraCombatSession combat;
    TeraAlfolPhase phase;
    TeraCombatObserver observe;void *context;
    int fixed_combat,initial_render,moved,status_actor;
    unsigned word_length;char word[11]; /* Saisie d'affichage, mot ignore par OnAlfol. */
    int16_t delta;unsigned wall,command;
} TeraAlfolSession;
/** OVL0085 : surface 0..99, batiments 900..999, medias et radiations.
 * initial=1 reproduit l'arrivee en position zero, avec un premier rendu/jour.
 * status_actor est D9B66 a l'entree : le depot utilise ce global plutot que
 * son indice local. Les valeurs legales observees sont 0..9 et 16 ;
 * 16 emule les alias DOS MP[6]/premier indice sans debordement C.
 * Copie state, suspend aux lectures et combats ; aucun prix/jet dans l'hote. */
int tera_alfol_start(TeraAlfolSession *session,const TeraExplorationState *state,unsigned command,
                    int initial,int status_actor,TeraCombatObserver observer,void *context);
int tera_alfol_key(TeraAlfolSession *session,unsigned key);
/** Sortie CCBE apres le dernier ordre obligatoire : dessin exterieur sans jour.
 * Ne remplace pas la fuite, qui a sa propre reprise de rendu. */
int tera_alfol_leave(TeraAlfolSession *session,const TeraExplorationState *state,int status_actor);
#endif
