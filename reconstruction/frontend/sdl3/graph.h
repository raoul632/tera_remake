#ifndef TERA_SDL_GRAPH_H
#define TERA_SDL_GRAPH_H
#include <SDL3/SDL.h>
/* Native GRAPH pixel raster, drawn with the current SDL color/blend mode. */
bool tera_graph_line(SDL_Renderer *r,int x,int y,int xx,int yy);
bool tera_graph_bytes(SDL_Renderer *r,int x,int y,int xx,int yy);
bool tera_graph_disc(SDL_Renderer *r,int x,int y,unsigned radius);
bool tera_graph_flood(SDL_Renderer *r,int x,int y,unsigned color,unsigned border);
/* C7A55: upright sides, native 180 degree arc and closing baseline. */
bool tera_graph_arch(SDL_Renderer *r,int x,int y,int height,int width);
#endif
