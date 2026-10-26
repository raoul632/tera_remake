#ifndef TERA_SDL_MEDIA_H
#define TERA_SDL_MEDIA_H
#include <SDL3/SDL.h>
#include "tera/presentation.h"
/* Bornes en echantillons ; until est renseigne pour les images XT. */
typedef struct {size_t at,until;TeraPresentationEvent event;} TeraMediaCue;
typedef struct {
    SDL_AudioStream *stream;
    float *pcm;size_t samples,capacity;
    TeraMediaCue *cues;size_t count,cue_capacity;
    Uint64 started;double phase;uint64_t xt_remainder;
    int active,failed;unsigned xt_resets;
} TeraMedia;
/* Horloge fournie par l'hote : tests deterministes sans attendre le mur.
 * Le device est facultatif ; absence de son ne supprime pas les durees. */
bool tera_media_open(TeraMedia *m);
void tera_media_close(TeraMedia *m);
void tera_media_reset(TeraMedia *m);
void tera_media_collect(TeraMedia *m,const TeraPresentationEvent *e);
bool tera_media_start(TeraMedia *m,Uint64 now);
bool tera_media_busy(const TeraMedia *m,Uint64 now);
const TeraPresentationEvent *tera_media_current(const TeraMedia *m,Uint64 now);
const TeraPresentationEvent *tera_media_flash(const TeraMedia *m,Uint64 now);
/* OVL00AE : rayon temporaire pendant les notes/delai deja emis. */
const TeraPresentationEvent *tera_media_ray(const TeraMedia *media,Uint64 now);
/* Rejoue les primitives jusqu'a l'image active, sans mutation du core. */
const TeraPresentationEvent *tera_media_ending(const TeraMedia *media,Uint64 now);
const TeraPresentationEvent *tera_media_tunnel(const TeraMedia *media,Uint64 now);
bool tera_media_tunnel_draw(SDL_Renderer *renderer,const TeraMedia *media,Uint64 now);
void tera_media_tunnel_raster(const TeraMedia *media,const TeraPresentationEvent *active,uint8_t *pixels);
#endif
