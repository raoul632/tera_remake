#include "module_test_support.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "battle.h"
#include "graph.h"
#include "media.h"
#include "renderer.h"
#include "assets.h"
#include "tera/sound.h"
#include "original_tunnel_vectors.h"
#include "original_tunnel_raster_vectors.h"
#include "original_xt_vectors.h"
#include "xt.h"
#include "cga.h"
#include <stdio.h>
#include <stdlib.h>
#include "original_figure_vectors.h"
static TeraTextures textures;static TeraMedia media;
static void collect(void *ctx,const TeraPresentationEvent *e)
{
    (void)ctx;tera_media_collect(&media,e);
    tera_scenery_collect(&textures,e);
    if(e->kind==TERA_PRESENT_MONSTER) textures.monster=e->data.monster;
}
static void equal(const TeraCombatSession *a,const TeraCombatSession *b)
{
    TeraCombatSession x=*a,y=*b;x.game.presentation=y.game.presentation=NULL;
    assert(!memcmp(&x,&y,sizeof x));
}
static void capture(SDL_Renderer *r,TeraGameSession *s,const TeraResources *res,const char *dir,const char *name,const TeraBattleFrame *frame)
{
    TeraGameSession before=*s;TeraGameView v;SDL_Surface *surface;char path[4096];
    assert(tera_game_view(s,&v));
    if(frame) assert(tera_battle_screen(r,&v,res,&textures,NULL,0,frame));
    else assert(tera_sdl_render(r,s,res,&textures,"","",NULL,0,NULL));
    assert(!memcmp(&before,s,sizeof before));
    surface=SDL_RenderReadPixels(r,NULL);assert(surface);
    snprintf(path,sizeof path,"%s/%s.bmp",dir,name);assert(SDL_SaveBMP(surface,path));SDL_DestroySurface(surface);
    assert(SDL_RenderPresent(r));
}
static void u16(FILE *f,unsigned v) {fputc((int)(v&255),f);fputc((int)((v>>8)&255),f);}
static void u32(FILE *f,unsigned v) {u16(f,v&65535);u16(f,v>>16);}
static void wave(const char *dir,const char *name)
{
    char path[4096];FILE *f;size_t i;
    snprintf(path,sizeof path,"%s/%s.wav",dir,name);f=fopen(path,"wb");assert(f);
    fwrite("RIFF",1,4,f);u32(f,36+(unsigned)media.samples*2);fwrite("WAVEfmt ",1,8,f);
    u32(f,16);u16(f,1);u16(f,1);u32(f,48000);u32(f,96000);u16(f,2);u16(f,16);
    fwrite("data",1,4,f);u32(f,(unsigned)media.samples*2);
    for(i=0;i<media.samples;++i) u16(f,(unsigned)(uint16_t)(int16_t)(media.pcm[i]*32767));
    assert(!fclose(f));
}
/* Rejouer les frontieres DRAW/CLEAR/SPRITE relevees par execution du BIN.
 * Seule la primitive de ligne SDL est commune : ce test certifie la
 * composition, pas l'identite de rasterisation avec la carte CGA. */
static void original_primitives(SDL_Renderer *r,const TeraResources *res,
    const OriginalPrimitive *p,size_t count)
{
    size_t i;
    for(i=0;i<count;++i,++p) {
        Uint8 c[4];tera_cga_rgba((unsigned)p->color,c);
        assert(SDL_SetRenderDrawColor(r,c[0],c[1],c[2],255));
        if(p->kind==1) assert(tera_graph_line(r,p->a,p->b,p->c,p->d));
        else if(p->kind==2) {
            SDL_FRect area={(float)(4*p->a),(float)(p->b-p->b%2),
                (float)(4*(p->c-p->a)),(float)(2*(p->d/2-p->b/2+1))};
            assert(SDL_RenderFillRect(r,&area));
        } else assert(tera_sprite_draw(r,&textures,res,(unsigned)p->c,p->a,p->b));
    }
}
static void original_figures(SDL_Renderer *r,const TeraResources *res)
{
    unsigned type,background;SDL_Surface *actual,*expected;
    for(type=16;type<=19;++type) if(type!=18) for(background=0;background<2;++background) {
        assert(SDL_SetRenderDrawColor(r,background ? 85 : 0,background ? 255 : 0,background ? 255 : 0,255));
        assert(SDL_RenderClear(r));textures.monster.type=type;
        assert(tera_battle_monster(r,res,&textures,type,11));actual=SDL_RenderReadPixels(r,NULL);assert(actual);
        assert(SDL_SetRenderDrawColor(r,background ? 85 : 0,background ? 255 : 0,background ? 255 : 0,255));
        assert(SDL_RenderClear(r));
        if(type==17) {
            original_primitives(r,res,original_hood0,sizeof original_hood0/sizeof *original_hood0);
            original_primitives(r,res,original_hood84,sizeof original_hood84/sizeof *original_hood84);
            assert(tera_sprite_draw(r,&textures,res,0x1a,49,96));
        } else {
            original_primitives(r,res,original_large,sizeof original_large/sizeof *original_large);
            if(type==16) {
                static const OriginalPrimitive extra[]={{2,19,96,25,180,0},{3,21,100,26,0,0}};
                original_primitives(r,res,extra,sizeof extra/sizeof *extra);
            }
        }
        expected=SDL_RenderReadPixels(r,NULL);assert(expected);
        assert(actual->format==expected->format && actual->w==expected->w && actual->h==expected->h);
        {int y;for(y=0;y<actual->h;++y)
            assert(!memcmp((Uint8 *)actual->pixels+y*actual->pitch,(Uint8 *)expected->pixels+y*expected->pitch,(size_t)actual->w*4));}
        SDL_DestroySurface(actual);SDL_DestroySurface(expected);
    }
}
static void palette_contracts(SDL_Renderer *r)
{
    static const Uint8 expected[4][3][3]={{{0,170,0},{170,0,0},{170,85,0}},
        {{0,170,170},{170,0,170},{170,170,170}},{{85,255,85},{255,85,85},{255,255,85}},
        {{85,255,255},{255,85,255},{255,255,255}}};
    unsigned palette,k;
    for(palette=0;palette<4;++palette) {
        SDL_Surface *raw,*pixels;
        assert(SDL_SetRenderLogicalPresentation(r,320,200,SDL_LOGICAL_PRESENTATION_LETTERBOX));
        for(k=0;k<3;++k) {
            Uint8 c[4];SDL_FRect box={(float)(k*106),0,k==2 ? 108 : 106,200};tera_cga_rgba(k+1,c);
            assert(SDL_SetRenderDrawColor(r,c[0],c[1],c[2],255) && SDL_RenderFillRect(r,&box));
        }
        assert(tera_cga_present_palette(r,palette));
        raw=SDL_RenderReadPixels(r,NULL);assert(raw);pixels=SDL_ConvertSurface(raw,SDL_PIXELFORMAT_RGBA32);assert(pixels);
        for(k=0;k<3;++k) {
            Uint8 *p=(Uint8 *)pixels->pixels+(pixels->h/2)*pixels->pitch+(int)((2*k+1)*(unsigned)pixels->w/6)*4;
            assert(!memcmp(p,expected[palette][k],3));
        }
        SDL_DestroySurface(raw);SDL_DestroySurface(pixels);
    }
}
int main(int argc,char **argv)
{
    TeraResources *res;TeraExplorationState source,e;TeraGameSession s;
    TeraCombatSession direct;TeraCombatGame baseline;TeraGameView v;
    TeraPresentationSink sink={collect,NULL};SDL_Window *w;SDL_Renderer *r;
    char error[256],name[64];unsigned type,calls,k,action;const TeraPresentationEvent *cue;
    assert(argc==3);res=tera_resources_load(argv[1],error,sizeof error);
    if(!res) {fprintf(stderr,"SKIP %s\n",error);return 77;}
    assert(tera_assets_world(&source,argv[1],1986,0,error,sizeof error));
    assert(SDL_Init(SDL_INIT_VIDEO));assert(tera_media_open(&media));
    w=SDL_CreateWindow("battle media",640,400,SDL_WINDOW_HIDDEN);assert(w);
    r=SDL_CreateRenderer(w,NULL);assert(r && tera_textures_create(r,res,&textures));
    original_figures(r,res);
    palette_contracts(r);
    /* Les fixtures sont synthetiques ; VALMON/NOMON/sprites sont DATA.
     * Chaque image est construite avec les vrais points consommes par C. */
    for(type=1;type<=22;++type) {
        memset(&s,0,sizeof s);e=module_fixture();
        memcpy(e.world.characters.combat.monster_values,source.world.characters.combat.monster_values,sizeof baseline.monster_values);
        s.phase=TERA_GAME_AMARANDE;s.active.amarande.state=e;s.active.amarande.phase=TERA_AM_COMBAT;
        s.active.amarande.state.world.characters.combat.encounter.monster_type=type;
        s.active.amarande.state.world.characters.combat.presentation=&sink;
        tera_media_reset(&media);calls=0;
        assert(tera_monster_presentation_random(&s.active.amarande.state.world.characters.combat,type,&calls));
        baseline=s.active.amarande.state.world.characters.combat;baseline.presentation=NULL;
        assert(tera_combat_start(&direct,&baseline,0,NULL,NULL));
        assert(tera_combat_start(&s.active.amarande.combat,&s.active.amarande.state.world.characters.combat,0,NULL,NULL));
        equal(&s.active.amarande.combat,&direct);
        assert(tera_media_start(&media,100));assert(tera_media_busy(&media,100));
        {size_t intro=0;
         while(intro<media.count && media.cues[intro].event.kind!=TERA_PRESENT_COMBAT) ++intro;
         assert(intro<media.count);
         cue=tera_media_current(&media,100+(media.cues[intro].at+47)/48);}
        assert(cue && cue->data.combat.kind==TERA_EVENT_INTRO);
        snprintf(name,sizeof name,"battle-%02u-intro",type);capture(r,&s,res,argv[2],name,&cue->data.combat);
        snprintf(name,sizeof name,"battle-%02u-menu",type);capture(r,&s,res,argv[2],name,NULL);
        if(type==21) {
            assert(tera_battle_flash(r,0,6));
            {SDL_Surface *flash=SDL_RenderReadPixels(r,NULL);char path[4096];assert(flash);
             snprintf(path,sizeof path,"%s/battle-flash.bmp",argv[2]);assert(SDL_SaveBMP(flash,path));SDL_DestroySurface(flash);}
        }
        assert(!tera_media_busy(&media,100+(media.samples+47)/48));
    }
    /* Huit commandes et toutes les confirmations, sans toucher aux regles. */
    for(action=0;action<8;++action) {
        static const char actions[]="TLJDAPCF";
        e=module_fixture();e.world.characters.combat.sound=1;
        e.world.characters.combat.equipment[0][0]=18;e.world.characters.combat.encounter.monster_type=18;
        e.world.characters.combat.presentation=&sink;
        baseline=e.world.characters.combat;baseline.presentation=NULL;
        assert(tera_combat_start(&direct,&baseline,0,NULL,NULL));
        memset(&s,0,sizeof s);s.phase=TERA_GAME_AMARANDE;s.active.amarande.phase=TERA_AM_COMBAT;s.active.amarande.state=e;
        assert(tera_combat_start(&s.active.amarande.combat,&e.world.characters.combat,0,NULL,NULL));
        tera_media_reset(&media);
        assert(tera_combat_key(&s.active.amarande.combat,(unsigned)actions[action]));
        assert(tera_combat_key(&direct,(unsigned)actions[action]));equal(&s.active.amarande.combat,&direct);
        snprintf(name,sizeof name,"battle-confirm-%c",actions[action]);capture(r,&s,res,argv[2],name,NULL);
        assert(tera_combat_key(&s.active.amarande.combat,'O'));assert(tera_combat_key(&direct,'O'));
        if(actions[action]=='C') {
            assert(tera_game_view(&s,&v) && v.prompt.kind==TERA_PROMPT_WORD);
            assert(tera_combat_key(&s.active.amarande.combat,'A') && tera_combat_key(&direct,'A'));
            capture(r,&s,res,argv[2],"battle-word",NULL);
            assert(tera_combat_key(&s.active.amarande.combat,13) && tera_combat_key(&direct,13));
        }
        equal(&s.active.amarande.combat,&direct);
        assert(tera_media_start(&media,1000));
        for(k=0;k<media.count;++k) if(media.cues[k].event.kind==TERA_PRESENT_COMBAT) {
            snprintf(name,sizeof name,"battle-%c-result-%u",actions[action],k);
            capture(r,&s,res,argv[2],name,&media.cues[k].event.data.combat);
        }
    }
    /* Frequence/DAC, horloge et device dummy : delai conserve en silence. */
    tera_media_reset(&media);baseline=module_fixture().world.characters.combat;baseline.presentation=&sink;
    tera_media_tone(&baseline,440,600);tera_media_wait(&baseline,80);tera_media_tone(&baseline,440,600);
    assert(media.samples==1280*48 && !media.failed);wave(argv[2],"combat-original-score");
    assert(tera_media_start(&media,500) && tera_media_busy(&media,1779) && !tera_media_busy(&media,1780));
    assert(SDL_GetAudioStreamQueued(media.stream)>=0);
    {size_t i;unsigned transitions=0;for(i=1;i<600*48;++i) if(media.pcm[i]!=media.pcm[i-1]) ++transitions;assert(transitions>=527 && transitions<=530);}
    /* Les nouvelles partitions traversent le lecteur SDL et produisent
     * un PCM audible, sans changer leur duree documentee. */
    tera_media_reset(&media);baseline.random.seed=1986;
    tera_sound_objective(&baseline);
    assert(media.samples==9315*48 && !media.failed);
    {size_t i;int audible=0;for(i=0;i<media.samples;++i) if(media.pcm[i]!=0) audible=1;assert(audible);}
    wave(argv[2],"progression-objective-score");
    assert(tera_media_start(&media,100) && tera_media_busy(&media,9414) && !tera_media_busy(&media,9415));
    tera_media_reset(&media);baseline.random.seed=1986;tera_sound_entry_meduz(&baseline);
    assert(media.samples==2500*48 && !media.failed);
    tera_media_reset(&media);tera_media_wait(&baseline,1000);assert(media.samples==48000);
    for(k=0;k<48000;++k) assert(media.pcm[k]==0);
    {TeraPresentationEvent flash={0};flash.kind=TERA_PRESENT_FLASH;flash.data.flash.ms=60;
     tera_media_reset(&media);tera_media_collect(&media,&flash);assert(tera_media_start(&media,20));
     assert(tera_media_flash(&media,79) && !tera_media_flash(&media,80));}
    {TeraPresentationEvent ray={0};ray.kind=TERA_PRESENT_RAY;ray.data.ray.ms=36;ray.data.ray.x=155;ray.data.ray.y=90;
     tera_media_reset(&media);tera_media_wait(&baseline,10);tera_media_collect(&media,&ray);tera_media_wait(&baseline,36);
     assert(tera_media_start(&media,100) && !tera_media_ray(&media,109));
     assert(tera_media_ray(&media,110) && tera_media_ray(&media,145) && !tera_media_ray(&media,146));}

    /* La fin succede au combat dans la timeline, sans ecran anticipe. */
    {TeraPresentationEvent e={0};
     tera_media_reset(&media);e.kind=TERA_PRESENT_COMBAT;e.data.combat.kind=TERA_EVENT_END;
     tera_media_collect(&media,&e);tera_media_wait(&baseline,100);
     e.kind=TERA_PRESENT_ENDING;tera_media_collect(&media,&e);tera_media_wait(&baseline,200);
     assert(tera_media_start(&media,500));
     assert(tera_media_current(&media,599) && !tera_media_ending(&media,599));
     assert(!tera_media_current(&media,600) && tera_media_ending(&media,600));
     assert(!tera_media_ending(&media,800));}
    /* Raster et cadence XT : le pilote original s'execute jusqu'a B800,
     * contrairement au premier oracle qui arretait Circle a son entree. */
    {TeraPresentationEvent e={0};SDL_Surface *image;size_t i;char path[4096];uint64_t cycles=0;
     uint8_t *pixels=SDL_malloc(TERA_CGA_PIXELS);assert(pixels);
     for(i=0;i<sizeof original_xt_circles/sizeof original_xt_circles[0];++i) {
         uint64_t actual=tera_xt_circle_cycles(original_xt_circles[i].x,original_xt_circles[i].y,original_xt_circles[i].radius);
         if(actual!=original_xt_circles[i].cycles) fprintf(stderr,"XT circle %u: %lu != %u\n",(unsigned)i,(unsigned long)actual,original_xt_circles[i].cycles);
         assert(actual==original_xt_circles[i].cycles);
     }
     tera_media_reset(&media);e.kind=TERA_PRESENT_TUNNEL;
     for(i=0;i<sizeof original_tunnel_frames/sizeof original_tunnel_frames[0];++i) {
         e.data.tunnel=original_tunnel_frames[i];cycles+=tera_xt_tunnel_cycles(&e.data.tunnel);tera_media_collect(&media,&e);
     }
     cycles+=TERA_XT_SEQUENCE_CLEAR_CYCLES;
     /* Budget issu de l'execution de la sequence native entiere : le profil
      * factorise doit rester a moins de 0.1 % de ce compteur independant. */
     assert(cycles<ORIGINAL_XT_TUNNEL_CYCLES && (ORIGINAL_XT_TUNNEL_CYCLES-cycles)*1000<ORIGINAL_XT_TUNNEL_CYCLES);
     assert(!media.failed && media.samples==cycles*48000/TERA_XT_CLOCK_HZ && media.count==245);
     assert(media.cues[0].until-media.cues[0].at>media.cues[30].until-media.cues[30].at);
     for(i=0;i<media.samples;++i) assert(media.pcm[i]==0);
     for(i=0;i<media.count;++i) {
         uint32_t hash=UINT32_C(2166136261);unsigned pixel;
         tera_media_tunnel_raster(&media,&media.cues[i].event,pixels);
         for(pixel=0;pixel<TERA_CGA_PIXELS;++pixel) hash=(hash^pixels[pixel])*UINT32_C(16777619);
         if(hash!=original_tunnel_hashes[i]) fprintf(stderr,"Native tunnel %u: %08lx != %08lx\n",(unsigned)i,(unsigned long)hash,(unsigned long)original_tunnel_hashes[i]);
         assert(hash==original_tunnel_hashes[i]);
     }
     assert(tera_media_start(&media,100) && tera_media_tunnel(&media,100));
     assert(tera_media_tunnel(&media,100+(media.cues[0].until-1)/48)==&media.cues[0].event);
     assert(tera_media_tunnel(&media,100+(media.cues[1].at+47)/48)==&media.cues[1].event);
     assert(!tera_media_tunnel(&media,100+(media.samples+47)/48));
     assert(tera_media_tunnel_draw(r,&media,100+(media.cues[30].at+47)/48));
     image=SDL_RenderReadPixels(r,NULL);assert(image);
     snprintf(path,sizeof path,"%s/meduz-tunnel.bmp",argv[2]);assert(SDL_SaveBMP(image,path));SDL_DestroySurface(image);
     printf("XT 4.77 MHz, 245 images : %.3f s (profil nominal)\n",(double)media.samples/48000);
     e.data.tunnel=original_tunnel_frames[0];e.data.tunnel.hz=7840;
     tera_media_reset(&media);tera_media_collect(&media,&e);
     assert(media.samples==(tera_xt_tunnel_cycles(&e.data.tunnel)+TERA_XT_SEQUENCE_CLEAR_CYCLES)*48000/TERA_XT_CLOCK_HZ);
     assert(media.pcm[0]!=0);assert(tera_media_start(&media,0));
     e.data.tunnel.background=1;tera_media_reset(&media);tera_media_collect(&media,&e);
     assert(tera_media_start(&media,0) && tera_media_tunnel_draw(r,&media,0));
     SDL_free(pixels);
    }
    tera_media_close(&media);tera_textures_destroy(&textures);SDL_DestroyRenderer(r);SDL_DestroyWindow(w);SDL_Quit();free(res);
    puts("Battle/media: 22 types, 8 commands, core equality, virtual clock, PCM and audio device OK");return 0;
}
