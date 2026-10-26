#ifndef TERA_LIFECYCLE_H
#define TERA_LIFECYCLE_H
#include "tera/exploration.h"
typedef enum { TERA_LIFE_ACK,TERA_LIFE_CREATE,TERA_LIFE_READY,TERA_LIFE_COMPLETE,TERA_LIFE_ERROR } TeraLifePhase;
typedef struct {
    TeraExplorationState state;
    TeraCreationSession creation;
    TeraLifePhase phase;
} TeraLifeSession;
/** TERAMAIN + OVL00CB branche defaite : musique originale, pause puis
 * OVL0022 sans regeneration du monde. Refuse un groupe encore disponible.
 * Si les quatre objectifs sont deja vrais, la boucle externe se termine
 * apres les condoleances, meme en cas de mort ; ne pas creer un successeur.
 * La fin victorieuse avec score reste un bloc distinct. */
int tera_life_start(TeraLifeSession *s,const TeraExplorationState *state);
/** Toute touche acquitte la pause BIOS, puis touches originales de creation.
 * READY fournit l'etat avant l'arrivee automatique de TERAMAIN ; sa graine
 * est poursuivie, son monde/cartes/objectifs/butin sont conserves. */
int tera_life_key(TeraLifeSession *s,unsigned key);
#endif
