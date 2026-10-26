#ifndef TERA_SDL_CHARACTERS_H
#define TERA_SDL_CHARACTERS_H
#include "resources.h"
#include <SDL3/SDL.h>
/* Presentation seule ; dimensions logiques choisies selon le mode original. */
bool tera_character_screen(SDL_Renderer *r,const TeraGameView *v,
    const TeraResources *resources,const TeraCharacterCard *held);
bool tera_creation_screen(SDL_Renderer *r,const TeraCreationSession *c,
    const TeraResources *resources);
bool tera_party_draw(SDL_Renderer *r,const TeraGameView *v);
#endif
