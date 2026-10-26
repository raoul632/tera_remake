#ifndef TERA_EXPLORATION_TRANSPORT_H
#define TERA_EXPLORATION_TRANSPORT_H
#include "tera/exploration.h"
/* Transport prive partage Amarande/Cite : decimal, bornes controlees,
 * ressources locales fournies par l'hote ; aucune regle de jeu ici. */
int tera_frontend_read_exploration(TeraExplorationState *state,unsigned *mode,unsigned *key);
#endif
