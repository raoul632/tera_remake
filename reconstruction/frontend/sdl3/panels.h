#ifndef TERA_SDL_PANELS_H
#define TERA_SDL_PANELS_H
#include "scene.h"
#include "cga.h"
bool tera_help_screen(SDL_Renderer *r);
bool tera_map_overlay(SDL_Renderer *r,const TeraGameView *view,Uint64 ticks);
bool tera_end_screen(SDL_Renderer *r,const TeraGameView *view);
bool tera_halt_screen(SDL_Renderer *r,const TeraResources *res,const TeraTextures *textures);
void tera_map_raster(const TeraGameView *view,Uint64 ticks,uint8_t *pixels);
#endif
