#ifndef TERA_SDL_RESOURCES_H
#define TERA_SDL_RESOURCES_H
#include "tera/game.h"
#include <stddef.h>
#include <stdint.h>

#define TERA_SPRITE_COUNT 82u
#define TERA_BACKGROUND_COUNT 6u
#define TERA_ATLAS_BYTES 23206u
#define TERA_ATLAS_PIXELS (TERA_ATLAS_BYTES*4u)
typedef enum {TERA_RES_VERBS,TERA_RES_ITEMS,TERA_RES_MONSTERS,TERA_RES_NAMES,
    TERA_RES_MESSAGE,TERA_RES_MESSAGE2,TERA_RES_MESSAGE3,TERA_RES_TEXT_COUNT} TeraTextTable;
typedef struct {
    unsigned raw_width,raw_height,hscale,vscale,width,height;
    size_t file_offset,pixel_offset,plane_offset;
} TeraSprite;
typedef struct {
    TeraSprite sprites[TERA_SPRITE_COUNT];
    uint8_t pixels[TERA_ATLAS_PIXELS];
    /* Original split byte planes, including reads across sprite boundaries. */
    uint8_t planes[2][TERA_ATLAS_BYTES/2];
    size_t used;
} TeraSpriteAtlas;
typedef struct {
    char text[TERA_RES_TEXT_COUNT][102][40]; /* CP437 termine, IDs publics 1-based. */
    TeraSpriteAtlas atlas;
    uint8_t backgrounds[TERA_BACKGROUND_COUNT][320*200];
} TeraResources;

/* Decodeurs sans SDL : validation complete avant mutation, pas d'allocation
 * ni de dependance au packing natif. Le loader impose les tailles originales. */
int tera_resource_strings(const uint8_t *bytes,size_t size,unsigned count,unsigned stride,char out[102][40]);
int tera_resource_atlas(const uint8_t *bytes,size_t size,TeraSpriteAtlas *out);
int tera_resource_cga(const uint8_t *bytes,size_t size,uint8_t out[320*200]);
/* Charger un paquet complet en lecture seule ; alloue une valeur uniquement
 * au succes. Liberer par free(). error recoit l'element fautif. */
TeraResources *tera_resources_load(const char *directory,char *error,size_t capacity);
const char *tera_resource_text(const TeraResources *resources,TeraTextTable table,unsigned id);
const char *tera_resource_background_name(unsigned index);
/* Conversion CP437 -> UTF-8 exacte, taille NUL incluse dans capacity.
 * Retour 0 sans modifier out si trop petit. Contrat aussi valable pour les
 * glyphes que la police de remplacement ne sait pas encore dessiner. */
uint32_t tera_cp437_codepoint(unsigned char byte);
int tera_cp437_utf8(const char *source,char *out,size_t capacity);
/* Retourne le texte selectionne par le core, sans RNG. Hints viennent du
 * monde courant. Les inscriptions residentes de Seth sont transcrites du PAS. NULL signale
 * une reference non resolue, pas une phrase inventee. */
const char *tera_resource_message(const TeraResources *resources,const TeraPresentationEvent *event,
    const TeraWorldState *world);
#endif
