#ifndef TERA_SDL_RENDERER_H
#define TERA_SDL_RENDERER_H
#include <SDL3/SDL.h>
#include "tera/game.h"
#include "resources.h"
#include "sprites.h"
/* Creation, fiches, scenes et combat ; fin/erreur encore diagnostiques.
 * Vue sans mutation. Les effets intermediaires utilisent battle.h/media.h. */
bool tera_sdl_render(SDL_Renderer *renderer,const TeraGameSession *game,
    const TeraResources *resources,const TeraTextures *textures,const char *message,const char *event,
    const TeraPresentationEvent *text_events,unsigned text_count,const TeraCharacterCard *held);
#endif

