#ifndef TERA_SDL_SCENE_H
#define TERA_SDL_SCENE_H
#include "sprites.h"
/* Compositions PAS en coordonnees logiques. Aucun appel mutable au core. */
bool tera_scene_draw(SDL_Renderer *r,const TeraGameView *view,
    const TeraResources *resources,const TeraTextures *textures);
/* Primitives residentes pures, aussi utilisables pour le comparatif BIN. */
bool tera_scene_catalyst(SDL_Renderer *renderer);
bool tera_scene_monolith(SDL_Renderer *renderer);
bool tera_scene_mountains(SDL_Renderer *renderer,const TeraResources *resources,
    const TeraTextures *textures,const unsigned short *values,unsigned count);
bool tera_scene_ui(SDL_Renderer *r,const TeraGameView *view,const TeraResources *resources,
    const TeraPresentationEvent *events,unsigned count);
/* Retourne une phrase de resultat d'un evenement, jamais une deduction HP. */
int tera_scene_result(const TeraPresentationEvent *event,const TeraResources *resources,
    char *text,size_t capacity);
#endif
