#ifndef TERA_SDL_TEXT_H
#define TERA_SDL_TEXT_H
#include <SDL3/SDL.h>
/* Texte CP437 8x8, retour ligne et limite de colonnes/lignes. Retourne false
 * seulement sur erreur SDL. Aucune transformation des donnees du core. */
bool tera_text_draw(SDL_Renderer *renderer,int x,int y,const char *cp437,unsigned columns,unsigned lines);
/* ROM bitmap, bit 7 a gauche. Mode graphique BIOS ou mode texte CGA. */
const unsigned char *tera_text_glyph(unsigned char code,int text_mode);
bool tera_text_box(SDL_Renderer *r,int x,int y,unsigned columns,unsigned rows);
/* Meme viewport CGA en 40 et 80 colonnes : pixels texte deux fois plus hauts. */
bool tera_video_mode(SDL_Renderer *r,int text_mode);
#endif
