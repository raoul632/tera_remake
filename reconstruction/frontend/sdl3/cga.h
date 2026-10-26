#ifndef TERA_CGA_RASTER_H
#define TERA_CGA_RASTER_H
#include <stdint.h>
#define TERA_CGA_PIXELS (320u*200u)
/* GRAPH : origine visible en haut, clipping 320x200, indices 0..3.
 * Les buffers peuvent contenir 255 pour les pixels de fond non touches. */
void tera_cga_plot(uint8_t *pixels,int x,int y,unsigned color);
/* GRAPH 3BCD : ties depend on the original start point. Callback receives
 * visible points only; no geometric endpoint clipping before rasterizing. */
void tera_cga_walk_line(int x,int y,int xx,int yy,void (*plot)(void *,int,int),void *context);
void tera_cga_line(uint8_t *pixels,int x,int y,int xx,int yy,unsigned color);
void tera_cga_fill(uint8_t *pixels,int x,int y,int xx,int yy,unsigned color);
void tera_cga_circle(uint8_t *pixels,int x,int y,unsigned radius,unsigned color);
/* GRAPH FillShape : stops at either border or already-filled pixels. */
void tera_cga_flood(uint8_t *pixels,int x,int y,unsigned color,unsigned border);
/* C4544 : coordonnees memoire en octets, x2 exclusif, deux banques CGA. */
void tera_cga_bytes(uint8_t *pixels,int x,int y,int xx,int yy,unsigned color);
#endif
