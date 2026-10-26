#ifdef NDEBUG
#undef NDEBUG
#endif
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "resources.h"
#include "sprites.h"
#include "text.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void save_capture(SDL_Renderer *r,const char *directory,const char *name)
{
    char path[4096];SDL_Surface *s=SDL_RenderReadPixels(r,NULL);
    assert(s);snprintf(path,sizeof path,"%s/%s",directory,name);
    assert(SDL_SaveBMP(s,path));SDL_DestroySurface(s);
}
/* Comparaison par pixel de la texture relue au framebuffer, pas seulement
 * de son chargement. Exerce canaux RGBA, pitch, parite et dimensions. */
static void verify_texture(SDL_Renderer *r,SDL_Texture *t,const uint8_t *indices,unsigned w,unsigned h)
{
    SDL_FRect dst={0,0,(float)w,(float)h};SDL_Rect read={0,0,(int)w,(int)h};
    SDL_Surface *raw,*rgba;unsigned x,y;
    assert(SDL_SetRenderDrawColor(r,0,0,0,255));assert(SDL_RenderClear(r));
    assert(SDL_RenderTexture(r,t,NULL,&dst));raw=SDL_RenderReadPixels(r,&read);assert(raw);
    rgba=SDL_ConvertSurface(raw,SDL_PIXELFORMAT_RGBA32);assert(rgba);
    assert(rgba->w==(int)w && rgba->h==(int)h);
    for(y=0;y<h;++y) for(x=0;x<w;++x) {
        uint8_t expected[4];const uint8_t *pixel=(uint8_t *)rgba->pixels+y*rgba->pitch+4*x;
        tera_cga_rgba(indices[y*w+x],expected);assert(!memcmp(pixel,expected,4));
    }
    SDL_DestroySurface(rgba);SDL_DestroySurface(raw);
}
/* BIN C5051/C5E04 : un octet nul ne touche pas le fond ; le noir
 * DANS un octet mixte ecrase le fond. Tester les deux evite la fausse
 * correction consistant a rendre tous les pixels noirs transparents. */
static void verify_byte_transparency(SDL_Renderer *r,SDL_Texture *t,const uint8_t *indices,
    unsigned w,unsigned h,unsigned *empty,unsigned *black)
{
    SDL_FRect dst={0,0,(float)w,(float)h};SDL_Rect area={0,0,(int)w,(int)h};
    /* byte_sprites also stores the extra pair read for an odd Y/header.
     * This asset test inspects the nominal image; native raster cases test
     * those cross-boundary reads through tera_sprite_draw. */
    SDL_FRect src={0,0,(float)w,(float)h};
    SDL_Surface *raw,*rgba;unsigned x,y;
    assert(SDL_SetRenderDrawColor(r,17,34,51,255) && SDL_RenderClear(r));
    assert(SDL_RenderTexture(r,t,&src,&dst));raw=SDL_RenderReadPixels(r,&area);assert(raw);
    rgba=SDL_ConvertSurface(raw,SDL_PIXELFORMAT_RGBA32);assert(rgba);
    for(y=0;y<h;++y) for(x=0;x<w;++x) {
        unsigned base=y*w+(x/4)*4;
        unsigned nonzero=indices[base]|indices[base+1]|indices[base+2]|indices[base+3];
        uint8_t expected[4]={17,34,51,255};
        const uint8_t *pixel=(const uint8_t *)rgba->pixels+y*rgba->pitch+4*x;
        if(nonzero) {tera_cga_rgba(indices[y*w+x],expected);if(!indices[y*w+x]) ++*black;}
        else ++*empty;
        assert(!memcmp(pixel,expected,4));
    }
    SDL_DestroySurface(rgba);SDL_DestroySurface(raw);
}
static void verify_sprite_clip(SDL_Renderer *r,const TeraTextures *t,const TeraResources *res)
{
    unsigned id,x,y;const TeraSprite *s=NULL;const uint8_t *indices=NULL;
    SDL_Surface *raw,*rgba;SDL_Rect area;
    for(id=1;id<=TERA_SPRITE_COUNT;++id) {
        unsigned nonzero=0;s=&res->atlas.sprites[id-1];indices=res->atlas.pixels+s->pixel_offset;
        for(x=0;x<2*s->width;++x) nonzero|=indices[x];
        if(nonzero) break;
    }
    assert(id<=TERA_SPRITE_COUNT);
    assert(SDL_SetRenderDrawColor(r,17,34,51,255) && SDL_RenderClear(r));
    assert(tera_sprite_draw(r,t,res,id,0,191)); /* Original : y/2, limite 96 paires. */
    area.x=0;area.y=188;area.w=(int)s->width;area.h=8;
    raw=SDL_RenderReadPixels(r,&area);assert(raw);rgba=SDL_ConvertSurface(raw,SDL_PIXELFORMAT_RGBA32);assert(rgba);
    for(y=0;y<8;++y) for(x=0;x<s->width;++x) {
        uint8_t expected[4]={17,34,51,255};
        const uint8_t *pixel=(const uint8_t *)rgba->pixels+y*rgba->pitch+4*x;
        if(y==2 || y==3) {
            unsigned base=(y-2)*s->width+(x/4)*4;
            if(indices[base]|indices[base+1]|indices[base+2]|indices[base+3])
                tera_cga_rgba(indices[(y-2)*s->width+x],expected);
        }
        assert(!memcmp(pixel,expected,4));
    }
    SDL_DestroySurface(rgba);SDL_DestroySurface(raw);
}
/* Empreintes independantes des deux fichiers ROM de reference, puis pixels
 * d'un accent et d'une bordure : couvre mapping CP437 et sens des bits. */
static void font_pixels(SDL_Renderer *r)
{
    unsigned mode,c,row;static const uint32_t hashes[]={UINT32_C(0x77163076),UINT32_C(0x2313e6e0)};
    static const unsigned char expected_e[]={0x1c,0,0x78,0xcc,0xfc,0xc0,0x78,0};
    SDL_Surface *surface,*rgba;unsigned x,y;
    for(mode=0;mode<2;++mode) {
        uint32_t hash=UINT32_C(2166136261);
        for(c=0;c<256;++c) for(row=0;row<8;++row) hash=(hash^tera_text_glyph((unsigned char)c,(int)mode)[row])*UINT32_C(16777619);
        assert(hash==hashes[mode]);
    }
    assert(SDL_SetRenderLogicalPresentation(r,320,200,SDL_LOGICAL_PRESENTATION_LETTERBOX));
    assert(SDL_SetRenderDrawColor(r,0,0,0,255) && SDL_RenderClear(r));
    assert(SDL_SetRenderDrawColor(r,255,255,255,255));assert(tera_text_draw(r,0,0,"\202\304",2,1));
    surface=SDL_RenderReadPixels(r,NULL);assert(surface);rgba=SDL_ConvertSurface(surface,SDL_PIXELFORMAT_RGBA32);SDL_DestroySurface(surface);assert(rgba);
    for(y=0;y<8;++y) for(x=0;x<16;++x) {
        unsigned char *p=(unsigned char *)rgba->pixels+y*rgba->pitch+x*4;
        int set=x<8 ? (expected_e[y]&(128u>>x))!=0 : y==4;
        assert(p[0]==(set ? 255 : 0) && p[1]==p[0] && p[2]==p[0]);
    }
    SDL_DestroySurface(rgba);
    {int w,h;float sx,sy;SDL_RendererLogicalPresentation mode;
     assert(tera_video_mode(r,1) && SDL_GetRenderLogicalPresentation(r,&w,&h,&mode));
     assert(w==640 && h==400 && SDL_GetRenderScale(r,&sx,&sy) && sx==1 && sy==2);
     assert(tera_video_mode(r,0) && SDL_GetRenderLogicalPresentation(r,&w,&h,&mode));
     assert(w==320 && h==200 && SDL_GetRenderScale(r,&sx,&sy) && sx==1 && sy==1);}
}
int main(int argc,char **argv)
{
    TeraResources *resources;TeraTextures textures;char error[512],path[4096],label[40];
    SDL_Window *window;SDL_Renderer *r;SDL_Texture *sheet;FILE *file;unsigned i,j,empty=0,black=0;
    assert(argc==3);snprintf(path,sizeof path,"%s/CRAPIM.IMA",argv[1]);file=fopen(path,"rb");
    if(!file) {puts("SKIP: original assets absent");return 77;}fclose(file);
    resources=tera_resources_load(argv[1],error,sizeof error);
    if(!resources) {fprintf(stderr,"%s\n",error);return 1;}
    assert(SDL_Init(SDL_INIT_VIDEO));
    window=SDL_CreateWindow("TERA assets test",320,200,SDL_WINDOW_HIDDEN);assert(window);
    r=SDL_CreateRenderer(window,NULL);assert(r);font_pixels(r);assert(tera_textures_create(r,resources,&textures));
    for(i=0;i<TERA_SPRITE_COUNT;++i) {
        const TeraSprite *s=&resources->atlas.sprites[i];
        verify_texture(r,textures.sprites[i],resources->atlas.pixels+s->pixel_offset,s->width,s->height);
        verify_byte_transparency(r,textures.byte_sprites[i],resources->atlas.pixels+s->pixel_offset,s->width,s->height,&empty,&black);
    }
    assert(empty && black);
    verify_sprite_clip(r,&textures,resources);
    for(i=0;i<TERA_BACKGROUND_COUNT;++i) verify_texture(r,textures.backgrounds[i],resources->backgrounds[i],320,200);
    assert(tera_background_draw(r,&textures,1));save_capture(r,argv[2],"original-title.bmp");
    sheet=SDL_CreateTexture(r,SDL_PIXELFORMAT_RGBA32,SDL_TEXTUREACCESS_TARGET,1024,1760);assert(sheet);
    assert(SDL_SetRenderTarget(r,sheet));assert(SDL_SetRenderDrawColor(r,0,0,0,255));assert(SDL_RenderClear(r));
    for(i=0;i<TERA_SPRITE_COUNT;++i) {
        const TeraSprite *s=&resources->atlas.sprites[i];
        SDL_FRect dst={(float)(i%8*128),(float)(i/8*160),(float)s->width,(float)s->height};
        assert(SDL_RenderTexture(r,textures.sprites[i],NULL,&dst));
        assert(SDL_SetRenderDrawColor(r,255,255,255,255));
        snprintf(label,sizeof label,"SPR_%02X %ux%u",i+1,s->width,s->height);
        assert(tera_text_draw(r,(int)dst.x,(int)dst.y+148,label,16,1));
    }
    save_capture(r,argv[2],"original-sprites.bmp");assert(SDL_SetRenderTarget(r,NULL));SDL_DestroyTexture(sheet);
    sheet=SDL_CreateTexture(r,SDL_PIXELFORMAT_RGBA32,SDL_TEXTUREACCESS_TARGET,640,672);assert(sheet);
    assert(SDL_SetRenderTarget(r,sheet));assert(SDL_SetRenderDrawColor(r,0,0,0,255));assert(SDL_RenderClear(r));
    for(i=0;i<TERA_BACKGROUND_COUNT;++i) {
        SDL_FRect dst={(float)(i%2*320),(float)(i/2*224),320,200};
        assert(SDL_RenderTexture(r,textures.backgrounds[i],NULL,&dst));
        assert(SDL_SetRenderDrawColor(r,255,255,255,255));
        assert(tera_text_draw(r,(int)dst.x,(int)dst.y+204,tera_resource_background_name(i),40,1));
    }
    save_capture(r,argv[2],"original-backgrounds.bmp");assert(SDL_SetRenderTarget(r,NULL));SDL_DestroyTexture(sheet);
    assert(SDL_SetRenderDrawColor(r,0,0,0,255));assert(SDL_RenderClear(r));
    assert(SDL_SetRenderDrawColor(r,255,255,255,255));
    for(i=0;i<TERA_RES_TEXT_COUNT;++i) {
        const char *sample=resources->text[i][0];
        for(j=0;j<102;++j) {
            const unsigned char *p=(const unsigned char *)resources->text[i][j];
            for(;*p;++p) {
                uint32_t cp=tera_cp437_codepoint(*p);
                /* Les tables CP437 completes couvrent chaque octet original. */
                assert(cp<128 || (cp>=160 && cp<=255));
                if(*p==0x82) sample=resources->text[i][j];
            }
        }
        assert(tera_text_draw(r,0,(int)i*24,sample,40,1));
    }
    save_capture(r,argv[2],"original-texts.bmp");
    tera_textures_destroy(&textures);free(resources);
    SDL_DestroyRenderer(r);SDL_DestroyWindow(window);SDL_Quit();
    puts("SDL assets OK: 82 sprites + 6 backgrounds, all pixels round-tripped, CP437 accents rendered");return 0;
}
