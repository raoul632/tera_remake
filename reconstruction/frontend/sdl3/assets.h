#ifndef TERA_SDL_ASSETS_H
#define TERA_SDL_ASSETS_H
#include "tera/game.h"
#include <stddef.h>
/* Lecture autonome en C : ne modifie world qu'apres validation/generation.
 * dir contient NOMPERS/VALMON. error recoit le diagnostic d'echec. */
int tera_assets_world(TeraExplorationState *world,const char *dir,uint32_t seed,
    unsigned difficulty,char *error,size_t capacity);
#endif
