#ifndef TERA_SDL_BATTLE_H
#define TERA_SDL_BATTLE_H
#include "scene.h"
/* Dessin pur du combat ou d'un resultat intermediaire conserve par la
 * chronologie. Aucun jet, calcul de degats ou recherche d'action permise. */
bool tera_battle_screen(SDL_Renderer *r,const TeraGameView *view,const TeraResources *res,
    const TeraTextures *textures,const TeraPresentationEvent *events,unsigned count,
    const TeraBattleFrame *frame);
/* C9B4E : dessiner uniquement la composition de rencontre capturee dans
 * textures, sur le decor deja present. Aucun changement du core. */
bool tera_battle_monster(SDL_Renderer *r,const TeraResources *res,
    const TeraTextures *textures,unsigned type,unsigned world);
/* C7A96/C7B05/C7B74 : silhouettes residentes et portrait superpose.
 * variant 0/1/2 ; le dernier conserve la couleur 1 de sa capuche. */
bool tera_scene_portrait(SDL_Renderer *r,const TeraResources *res,const TeraTextures *textures,
    int x,unsigned color,unsigned variant);
/* Restitution des quatre palettes CGA et bordure EGA tirees par C6D32. */
bool tera_battle_flash(SDL_Renderer *r,unsigned palette,unsigned border);
/* Palette CGA d'une scene complete ; conserve la zone de letterboxing. */
bool tera_cga_present_palette(SDL_Renderer *r,unsigned palette);
#endif
