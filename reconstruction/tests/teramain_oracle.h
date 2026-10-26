#ifndef TERA_TERAMAIN_ORACLE_H
#define TERA_TERAMAIN_ORACLE_H
#include "tera/game.h"
/* Banc natif facultatif : une machine DOS conserve son etat pendant tout
 * le parcours. Les touches du conducteur sont donnees aux deux moteurs.
 * Aucun etat du core n'est reinjecte dans le DOS apres l'initialisation. */
void teramain_oracle_start(const char *resources,const TeraExplorationState *world,
    const TeraGameSession *session);
void teramain_oracle_key(unsigned key,const TeraGameSession *session);
void teramain_oracle_finish(const TeraGameSession *session);
#endif
