#ifndef TERA_SDL_INPUT_H
#define TERA_SDL_INPUT_H
#include <SDL3/SDL.h>
/* Mapper une pression physique en octet de lecture core ; -1 = hors jeu.
 * Aucun test de validite de commande ni decision de gameplay ici. */
int tera_sdl_key(const SDL_KeyboardEvent *event);
#endif
