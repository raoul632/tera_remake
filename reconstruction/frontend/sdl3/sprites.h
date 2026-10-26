#ifndef TERA_SDL_SPRITES_H
#define TERA_SDL_SPRITES_H
#include <SDL3/SDL.h>
#include "resources.h"
typedef struct {
    SDL_Texture *sprites[TERA_SPRITE_COUNT],*backgrounds[TERA_BACKGROUND_COUNT];
    SDL_Texture *byte_sprites[TERA_SPRITE_COUNT],*scene_layer;
    TeraMonsterPicture monster; /* Derniers points deja tires par le core. */
    struct {unsigned world;int position,on_alfol;TeraScenePicture picture;} scenery[TERA_PICTURE_COUNT];
} TeraTextures;
/* Palette CGA cyan/magenta/blanc de base (hypothese de restitution pour
 * les scenes dont les couleurs ne sont pas encore confirmees). Les textures
 * brutes restent opaques pour l'inspection ; byte_sprites suit le saut
 * d'octets nuls confirme BIN C5051/C5E04, pas une cle de couleur par pixel. */
void tera_cga_rgba(unsigned index,uint8_t color[4]);
bool tera_textures_create(SDL_Renderer *renderer,const TeraResources *resources,TeraTextures *textures);
void tera_textures_destroy(TeraTextures *textures);
bool tera_background_draw(SDL_Renderer *renderer,const TeraTextures *textures,unsigned index);
/* C5051/C5E04 : sauter les octets CGA nuls, conserver le noir des octets
 * mixtes. x en octets, y rabattu sur une paire de lignes ; limite y=192. */
bool tera_sprite_draw(SDL_Renderer *renderer,const TeraTextures *textures,
    const TeraResources *resources,unsigned id,int x,int y);
/* Memoriser un evenement graphique sans effet sur le moteur. Les details
 * sont invalides sur une autre case/zone ou apres chargement. */
void tera_scenery_collect(TeraTextures *textures,const TeraPresentationEvent *event);
#endif
