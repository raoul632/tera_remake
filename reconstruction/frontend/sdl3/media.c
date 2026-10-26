#include "text.h"
#include "cga.h"
#include "xt.h"
#include "battle.h"
#include "media.h"
#include "tera/interactive.h"
#include <limits.h>
#include <string.h>

bool tera_media_open(TeraMedia *m)
{
    SDL_AudioSpec spec={SDL_AUDIO_F32,1,48000};
    if(!SDL_InitSubSystem(SDL_INIT_AUDIO)) return false;
    m->stream=SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&spec,NULL,NULL);
    if(!m->stream) return false;
    if(!SDL_ResumeAudioStreamDevice(m->stream)) {SDL_DestroyAudioStream(m->stream);m->stream=NULL;return false;}
    return true;
}
void tera_media_close(TeraMedia *m)
{SDL_DestroyAudioStream(m->stream);SDL_free(m->pcm);SDL_free(m->cues);SDL_zero(*m);}
void tera_media_reset(TeraMedia *m)
{
    if(m->stream) SDL_ClearAudioStream(m->stream);
    m->samples=0;m->count=0;m->active=0;m->failed=0;m->phase=0;m->xt_remainder=0;m->xt_resets=0;
}
static void tone_samples(TeraMedia *m,unsigned hz,size_t n)
{
    size_t i,needed=m->samples+n;unsigned divisor;
    double increment=0;
    if(needed>m->capacity) {
        size_t cap=needed+48000;float *next=SDL_realloc(m->pcm,cap*sizeof *next);
        if(!next) {m->failed=1;return;}m->pcm=next;m->capacity=cap;
    }
    /* PIT 8253 : diviseur entier 1193180 / Hz. Pas de sinusoide ajoutee. */
    if(hz) {divisor=1193180u/hz;if(!divisor) divisor=1;if(divisor>65535) divisor=65535;increment=1193180.0/divisor/48000;}
    for(i=0;i<n;++i) {
        m->pcm[m->samples++]=hz ? (m->phase<0.5 ? 0.12f : -0.12f) : 0;
        m->phase+=increment;while(m->phase>=1) m->phase-=1;
    }
}
static void tone(TeraMedia *m,unsigned hz,unsigned ms)
{tone_samples(m,hz,ms ? (size_t)ms*48 : 1);}
static void cue(TeraMedia *m,const TeraPresentationEvent *e)
{
    if(m->count==m->cue_capacity) {
        size_t cap=m->cue_capacity ? m->cue_capacity*2 : 32;
        TeraMediaCue *next=SDL_realloc(m->cues,cap*sizeof *next);
        if(!next) {m->failed=1;return;}m->cues=next;m->cue_capacity=cap;
    }
    m->cues[m->count].at=m->samples;m->cues[m->count].until=m->samples;m->cues[m->count++].event=*e;
}
void tera_media_collect(TeraMedia *m,const TeraPresentationEvent *e)
{
    unsigned i;
    if(m->failed) return;
    if(e->kind==TERA_PRESENT_TONES) {
        if(e->data.tones.count>64) {m->failed=1;return;}
        for(i=0;i<e->data.tones.count;++i) tone(m,e->data.tones.hz[i],e->data.tones.ms[i]);
    } else if(e->kind==TERA_PRESENT_WAIT) tone(m,0,e->data.wait.ms);
    else if(e->kind==TERA_PRESENT_FLASH) {cue(m,e);if(!e->data.flash.audio) tone(m,0,e->data.flash.ms);}
    else if(e->kind==TERA_PRESENT_COMBAT && e->data.combat.kind!=TERA_EVENT_ROUND) cue(m,e);
    else if(e->kind==TERA_PRESENT_RAY || e->kind==TERA_PRESENT_ENDING) cue(m,e);
    else if(e->kind==TERA_PRESENT_TUNNEL) {
        uint64_t cycles=tera_xt_tunnel_cycles(&e->data.tunnel),numerator;
        if(e->data.tunnel.reset && m->xt_resets++%7==0) cycles+=TERA_XT_SEQUENCE_CLEAR_CYCLES;
        numerator=cycles*48000+m->xt_remainder;
        cue(m,e);if(m->failed) return;
        m->xt_remainder=numerator%TERA_XT_CLOCK_HZ;
        tone_samples(m,e->data.tunnel.hz,(size_t)(numerator/TERA_XT_CLOCK_HZ));
        m->cues[m->count-1].until=m->samples;
    }
}
const TeraPresentationEvent *tera_media_ray(const TeraMedia *m,Uint64 now)
{
    size_t i,at=(size_t)(now>=m->started ? now-m->started : 0)*48;
    if(!tera_media_busy(m,now)) return NULL;
    for(i=0;i<m->count;++i) if(m->cues[i].event.kind==TERA_PRESENT_RAY &&
        m->cues[i].at<=at && at<m->cues[i].at+(size_t)m->cues[i].event.data.ray.ms*48) return &m->cues[i].event;
    return NULL;
}
bool tera_media_start(TeraMedia *m,Uint64 now)
{
    m->started=now;m->active=m->samples!=0;
    if(m->failed || m->samples>INT_MAX/sizeof(float)) return false;
    if(m->stream && m->samples && !SDL_PutAudioStreamData(m->stream,m->pcm,(int)(m->samples*sizeof(float)))) return false;
    return true;
}
bool tera_media_busy(const TeraMedia *m,Uint64 now)
{return m->active && now>=m->started && (now-m->started)*48<m->samples;}
const TeraPresentationEvent *tera_media_current(const TeraMedia *m,Uint64 now)
{
    size_t i,at=(size_t)(now>=m->started ? now-m->started : 0)*48;
    const TeraPresentationEvent *current=NULL;
    if(!tera_media_busy(m,now)) return NULL;
    for(i=0;i<m->count && m->cues[i].at<=at;++i)
        if(m->cues[i].event.kind==TERA_PRESENT_COMBAT) current=&m->cues[i].event;
        else if(m->cues[i].event.kind==TERA_PRESENT_TUNNEL || m->cues[i].event.kind==TERA_PRESENT_ENDING) current=NULL;
    return current;
}
const TeraPresentationEvent *tera_media_flash(const TeraMedia *m,Uint64 now)
{
    size_t i,at=(size_t)(now>=m->started ? now-m->started : 0)*48;
    if(!tera_media_busy(m,now)) return NULL;
    for(i=0;i<m->count;++i) if(m->cues[i].event.kind==TERA_PRESENT_FLASH &&
        m->cues[i].at<=at && at<m->cues[i].at+(size_t)m->cues[i].event.data.flash.ms*48)
        return &m->cues[i].event;
    return NULL;
}

const TeraPresentationEvent *tera_media_tunnel(const TeraMedia *m,Uint64 now)
{
    size_t i,at=(size_t)(now>=m->started ? now-m->started : 0)*48;
    if(!tera_media_busy(m,now)) return NULL;
    for(i=0;i<m->count;++i) if(m->cues[i].event.kind==TERA_PRESENT_TUNNEL &&
        m->cues[i].at<=at && at<m->cues[i].until) return &m->cues[i].event;
    return NULL;
}
void tera_media_tunnel_raster(const TeraMedia *m,const TeraPresentationEvent *active,uint8_t *pixels)
{
    size_t i,first=0;unsigned j;const TeraTunnelFrame *f;
    for(i=0;i<m->count && &m->cues[i].event!=active;++i)
        if(m->cues[i].event.kind==TERA_PRESENT_TUNNEL && m->cues[i].event.data.tunnel.reset) first=i;
    if(active->data.tunnel.reset) first=i;
    memset(pixels,0,TERA_CGA_PIXELS);
    for(i=first;i<m->count;++i) if(m->cues[i].event.kind==TERA_PRESENT_TUNNEL) {
        f=&m->cues[i].event.data.tunnel;
        if(f->reset) for(j=0;j<24;++j) tera_cga_plot(pixels,f->stars[2*j],f->stars[2*j+1],3);
        tera_cga_circle(pixels,f->x,f->y,f->radius,f->color);
        if(f->erase_radius) tera_cga_circle(pixels,f->erase_x,f->erase_y,f->erase_radius,0);
        if(&m->cues[i].event==active) break;
    }
}
bool tera_media_tunnel_draw(SDL_Renderer *r,const TeraMedia *m,Uint64 now)
{
    static const uint8_t colors[4][3][3]={
        {{0,170,0},{170,0,0},{170,85,0}},{{0,170,170},{170,0,170},{170,170,170}},
        {{85,255,85},{255,85,85},{255,255,85}},{{85,255,255},{255,85,255},{255,255,255}}};
    const TeraPresentationEvent *active=tera_media_tunnel(m,now);uint8_t pixels[TERA_CGA_PIXELS];
    SDL_Surface *surface;SDL_Texture *texture;bool ok;unsigned i;
    if(!active || !tera_video_mode(r,0)) return false;
    tera_media_tunnel_raster(m,active,pixels);
    surface=SDL_CreateSurface(320,200,SDL_PIXELFORMAT_RGBA32);if(!surface) return false;
    for(i=0;i<TERA_CGA_PIXELS;++i) {
        uint8_t *dst=(uint8_t *)surface->pixels+(i/320)*surface->pitch+(i%320)*4;
        if(pixels[i]) memcpy(dst,colors[active->data.tunnel.palette&3][pixels[i]-1],3);
        else {dst[0]=dst[1]=0;dst[2]=active->data.tunnel.background ? 170 : 0;}
        dst[3]=255;
    }
    texture=SDL_CreateTextureFromSurface(r,surface);SDL_DestroySurface(surface);if(!texture) return false;
    ok=SDL_SetTextureScaleMode(texture,SDL_SCALEMODE_NEAREST) && SDL_RenderTexture(r,texture,NULL,NULL);
    SDL_DestroyTexture(texture);return ok;
}

const TeraPresentationEvent *tera_media_ending(const TeraMedia *m,Uint64 now)
{
    size_t i,at=(size_t)(now>=m->started ? now-m->started : 0)*48;
    if(!tera_media_busy(m,now)) return NULL;
    for(i=0;i<m->count && m->cues[i].at<=at;++i)
        if(m->cues[i].event.kind==TERA_PRESENT_ENDING) return &m->cues[i].event;
    return NULL;
}
