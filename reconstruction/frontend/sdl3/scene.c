#include "panels.h"
#include "scene.h"
#include "characters.h"
#include "text.h"
#include "graph.h"
#include "battle.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

#define TRY(call) do {if(!(call)) return false;} while(0)
typedef struct {SDL_Renderer *r;const TeraResources *res;const TeraTextures *tex;int offscreen;SDL_Texture *previous_target;} Canvas;
static bool color(Canvas *c,unsigned index)
{
    uint8_t rgba[4];tera_cga_rgba(index,rgba);
    return SDL_SetRenderDrawColor(c->r,rgba[0],rgba[1],rgba[2],c->offscreen && !index ? 0 : 255);
}
static bool line(Canvas *c,int x,int y,int xx,int yy,unsigned ink)
{ return color(c,ink) && tera_graph_line(c->r,x,y,xx,yy); }
static bool rect(Canvas *c,int x,int y,int xx,int yy,unsigned ink,int fill)
{
    SDL_FRect r;int t;
    if(xx<x) {t=x;x=xx;xx=t;}if(yy<y) {t=y;y=yy;yy=t;}
    r.x=(float)x;r.y=(float)y;r.w=(float)(xx-x+1);r.h=(float)(yy-y+1);
    if(!color(c,ink)) return false;
    if(fill) return SDL_RenderFillRect(c->r,&r);
    return tera_graph_line(c->r,x,y,xx,y) && tera_graph_line(c->r,xx,y,xx,yy) &&
        tera_graph_line(c->r,xx,yy,x,yy) && tera_graph_line(c->r,x,yy,x,y);
}
/* PutSprite travaille en octets CGA ; DRAW travaille en pixels. */
static bool sprite(Canvas *c,int x,int y,unsigned id)
{
    return tera_sprite_draw(c->r,c->tex,c->res,id,x,y);
}
static bool byte_rect(Canvas *c,int x,int y,int xx,int yy,unsigned ink)
{return color(c,ink) && tera_graph_bytes(c->r,x,y,xx,yy);}
static bool disc(Canvas *c,int x,int y,int radius,unsigned ink)
{
    return color(c,ink) && tera_graph_disc(c->r,x,y,(unsigned)radius);
}
/* C6DC5 : x2 est exclusif ; les deux lignes de chaque paire sont remplies.
 * Les fenetres sont des points noirs, pas des sprites ajoutes. */
static bool skyscraper(Canvas *c,int x,int base,int height,int width,unsigned ink,int windows)
{
    int row,col;
    TRY(byte_rect(c,x,base-height,x+width,base,ink));
    TRY(line(c,4*(x+width)+1,base-height-1,4*(x+width)+1,base-1,0));
    TRY(line(c,4*(x+width),base-height-1,4*(x+width),base-1,0));
    TRY(line(c,4*(x+width)+2,base-height-1,4*(x+width)+2,base-1,ink));
    if(windows) {
        col=0;do {++col;row=0;do {++row;TRY(color(c,0));
            TRY(SDL_RenderPoint(c->r,(float)(4*x+3*col),(float)(base-3*row)));
        } while(3*row+3<=height);} while(3*col+5<=4*width);
    }
    return true;
}
/* C70A0 : trois profondeurs de trois immeubles. */
static bool skyline(Canvas *c,const unsigned short *v,unsigned count)
{
    unsigned row,col,n=0;if(count<27) return true;
    for(row=1;row<=3;++row) for(col=1;col<=3;++col) {
        int x=8*(int)col+v[n]-7,h=40+v[n+1],w=5+v[n+2];n+=3;
        TRY(skyscraper(c,x,132+2*(int)row,h,w,row,1));
    }
    return true;
}
static bool mountains(Canvas *c,const unsigned short *v,unsigned count)
{
    unsigned i;if(count<35) return true;
    for(i=0;i<7;++i) {
        unsigned n=5*i;
        TRY(sprite(c,12*(int)i+v[n+1]-2,50+2*v[n+2],v[n]==0 ? 0x4b : 0x4c));
        TRY(sprite(c,6+12*(int)i+v[n+3]-2,74+2*v[n+4],0x4b));
    }
    return true;
}
static const TeraScenePicture *details(Canvas *c,const TeraSceneView *s,unsigned kind)
{
    if(c->tex->scenery[kind].world!=s->world || c->tex->scenery[kind].position!=s->position) return NULL;
    return &c->tex->scenery[kind].picture;
}
static bool layer_begin(Canvas *c)
{
    c->previous_target=SDL_GetRenderTarget(c->r);
    TRY(SDL_SetRenderTarget(c->r,c->tex->scene_layer));c->offscreen=1;
    TRY(SDL_SetRenderLogicalPresentation(c->r,320,200,SDL_LOGICAL_PRESENTATION_DISABLED));
    TRY(SDL_SetRenderScale(c->r,1,1));
    TRY(SDL_SetRenderDrawBlendMode(c->r,SDL_BLENDMODE_NONE));TRY(color(c,0));
    return SDL_RenderClear(c->r);
}
static bool layer_end(Canvas *c)
{
    SDL_FRect dst={0,0,320,200};SDL_Surface *raw,*rgba;SDL_Texture *layer;bool ok;unsigned x,y,k;
    /* C60BB tests a whole CGA byte. Its black pixels overwrite the scene
     * whenever another pixel in the same byte is nonzero. */
    raw=SDL_RenderReadPixels(c->r,NULL);if(!raw) return false;
    rgba=SDL_ConvertSurface(raw,SDL_PIXELFORMAT_RGBA32);SDL_DestroySurface(raw);if(!rgba) return false;
    for(y=0;y<200;++y) for(x=0;x<320;x+=4) {
        uint8_t *p=(uint8_t *)rgba->pixels+y*rgba->pitch+x*4;unsigned nonzero=0;
        for(k=0;k<4;++k) nonzero|=p[4*k]|p[4*k+1]|p[4*k+2];
        for(k=0;k<4;++k) p[4*k+3]=nonzero ? 255 : 0;
    }
    layer=SDL_CreateTextureFromSurface(c->r,rgba);SDL_DestroySurface(rgba);if(!layer) return false;
    ok=SDL_SetTextureScaleMode(layer,SDL_SCALEMODE_NEAREST) && SDL_SetTextureBlendMode(layer,SDL_BLENDMODE_BLEND);
    if(!SDL_SetRenderTarget(c->r,c->previous_target)) {SDL_DestroyTexture(layer);return false;}
    c->offscreen=0;
    if(ok) ok=SDL_RenderTexture(c->r,layer,NULL,&dst);
    SDL_DestroyTexture(layer);return ok;
}
bool tera_scene_catalyst(SDL_Renderer *r)
{Canvas c={r,NULL,NULL,0,NULL};return skyscraper(&c,38,164,140,4,2,0);}
bool tera_scene_mountains(SDL_Renderer *r,const TeraResources *res,const TeraTextures *tex,
    const unsigned short *values,unsigned count)
{
    Canvas c={r,res,tex,0,NULL};
    return layer_begin(&c) && mountains(&c,values,count) && layer_end(&c);
}
static bool rocks(Canvas *c,int x,int y)
{ return sprite(c,x,y,0x20) && sprite(c,x+3,y-4,0x20) && sprite(c,x+8,y+2,0x20); }
static bool bushes(Canvas *c,int x,int y,int large)
{
    TRY(sprite(c,x,y,0x21));
    if(large) return sprite(c,x+5,y-4,0x21) && sprite(c,x+13,y-2,0x21) &&
        sprite(c,x+9,y+6,0x21) && sprite(c,x+16,y+8,0x21);
    return sprite(c,x+4,y-2,0x21) && sprite(c,x+7,y+6,0x21);
}
static bool foreground(Canvas *c)
{ return sprite(c,9,176,0x21) && sprite(c,15,182,0x21) && sprite(c,1,158,0x44); }
static bool ruin(Canvas *c,int x,unsigned kind)
{
    if(kind==0) return sprite(c,x+15,112,0x2a) && sprite(c,x,118,0x3a) &&
        sprite(c,x+12,122,0x2b) && sprite(c,x+17,114,0x39) && sprite(c,x+10,130,0x2c);
    if(kind==1) return sprite(c,x+19,110,0x2a) && sprite(c,x,106,0x50) &&
        sprite(c,x+18,124,0x3a) && sprite(c,x+12,132,0x2c);
    return sprite(c,x,108,0x2b) && sprite(c,x+4,112,0x4f) &&
        sprite(c,x+5,128,0x2c) && sprite(c,x+14,124,0x50);
}
static bool spires(Canvas *c)
{
    TRY(sprite(c,48,24,0x45));TRY(sprite(c,58,19,0x45));TRY(sprite(c,64,17,0x45));
    TRY(rect(c,184,50,319,80,0,1));TRY(sprite(c,52,34,0x45));
    TRY(rect(c,216,56,319,90,0,1));TRY(sprite(c,54,42,0x45));
    TRY(sprite(c,59,37,0x45));TRY(sprite(c,64,35,0x45));
    TRY(rect(c,208,64,319,100,0,1));return sprite(c,58,64,0x47);
}
static bool foliage_tower(Canvas *c,int offset)
{
    TRY(sprite(c,64,82-offset,0x21));TRY(sprite(c,69,80-offset,0x21));TRY(sprite(c,72,80-offset,0x21));
    TRY(rect(c,256,94-offset,319,110-offset,0,1));TRY(sprite(c,66,82-offset,0x21));
    TRY(rect(c,264,96-offset,319,110-offset,0,1));TRY(sprite(c,67,92-offset,0x21));
    TRY(sprite(c,69,90-offset,0x21));TRY(sprite(c,72,88-offset,0x21));
    TRY(rect(c,268,102-offset,319,120-offset,0,1));return sprite(c,69,102-offset,0x22);
}
static bool landscape(Canvas *c,const TeraLandscape *p)
{
    unsigned i;int h;
    if(!p) return true;
    if(p->command=='6') TRY(disc(c,60,40,8,3));
    h=(int)p->horizon_height;
    if(!p->horizon_kind) {
        TRY(line(c,0,126,80,126-h,2));TRY(line(c,80,126-h,110,125-h,2));
        TRY(line(c,110,125-h,140,126-h,2));TRY(line(c,140,126-h,220,126,2));
        TRY(line(c,180,126-h/2,240,126-2*h/3,2));TRY(line(c,240,126-2*h/3,280,126-3*h/5,2));
        TRY(line(c,280,126-3*h/5,319,126-h/3,2));
    } else {
        TRY(line(c,0,126,60,126-h/2,2));TRY(line(c,60,126-h/2,100,124-h/2,2));
        TRY(line(c,100,124-h/2,160,126-h/2,2));TRY(line(c,160,126-h/2,200,126,2));
        TRY(line(c,180,126-h/4,240,126-h,2));TRY(line(c,240,126-h,280,126-h,2));
        TRY(line(c,280,126-h,319,126-3*h/5,2));
    }
    TRY(color(c,3));
    for(i=0;i<16;++i) TRY(SDL_RenderPoint(c->r,(float)p->stars[i].x,(float)p->stars[i].y));
    if(p->mountain_count) {
        TRY(layer_begin(c));TRY(mountains(c,p->mountains,p->mountain_count));TRY(layer_end(c));
    }
    return true;
}
static bool ground(Canvas *c,const TeraLandscape *p)
{
    unsigned i;if(!p) return true;
    for(i=0;i<p->count && i<65;++i) TRY(sprite(c,(int)p->ground[i].x,(int)p->ground[i].y,p->ground[i].sprite));
    return true;
}
/* OVL00FB : groupes fixes des 23 compositions. Le sol etoile est fourni
 * par les tirages du core. Les montagnes detaillees restent a restituer. */
static bool country(Canvas *c,unsigned scene,const TeraLandscape *p)
{
    switch(scene) {
    case 0:TRY(bushes(c,35,114,1));TRY(bushes(c,18,118,0));TRY(ground(c,p));TRY(foliage_tower(c,0));TRY(bushes(c,54,118,0));TRY(foreground(c));TRY(sprite(c,2,100,0x45));return rocks(c,67,174);
    case 1:TRY(bushes(c,20,112,1));TRY(ground(c,p));TRY(spires(c));TRY(sprite(c,2,100,0x45));return rocks(c,1,178);
    case 2:TRY(bushes(c,1,120,1));TRY(bushes(c,30,116,0));TRY(bushes(c,44,114,1));TRY(ground(c,p));TRY(rocks(c,1,174));TRY(sprite(c,73,142,0x20));TRY(sprite(c,64,102,0x45));return sprite(c,71,156,0x44);
    case 3:TRY(bushes(c,1,120,1));TRY(bushes(c,28,114,1));TRY(bushes(c,54,116,1));TRY(ground(c,p));TRY(rocks(c,1,174));return sprite(c,71,156,0x44);
    case 4:TRY(ruin(c,0,0));TRY(ruin(c,40,1));TRY(ground(c,p));TRY(sprite(c,42,144,0x27));TRY(sprite(c,37,150,0x29));TRY(sprite(c,51,148,0x28));return sprite(c,73,30,0x52);
    case 5:TRY(ruin(c,0,1));TRY(ruin(c,36,0));TRY(ground(c,p));TRY(sprite(c,2,176,0x29));return sprite(c,70,166,0x2c);
    case 6:TRY(ruin(c,4,0));TRY(ruin(c,40,2));TRY(ground(c,p));TRY(sprite(c,68,50,0x52));TRY(sprite(c,74,36,0x52));TRY(foliage_tower(c,70));TRY(sprite(c,26,160,0x27));TRY(foreground(c));return sprite(c,54,176,0x4f);
    case 7:TRY(ruin(c,22,0));TRY(ruin(c,36,2));TRY(bushes(c,1,120,1));TRY(ground(c,p));TRY(rocks(c,1,174));return sprite(c,71,156,0x44);
    case 8:TRY(ruin(c,26,2));TRY(bushes(c,6,120,1));TRY(sprite(c,57,104,0x45));TRY(ground(c,p));TRY(rocks(c,63,152));return sprite(c,73,30,0x52);
    case 9:TRY(bushes(c,41,114,0));TRY(bushes(c,20,118,1));TRY(bushes(c,1,112,0));TRY(ground(c,p));TRY(foliage_tower(c,0));TRY(foreground(c));return rocks(c,67,174);
    case 10:TRY(ruin(c,44,0));TRY(ruin(c,24,1));TRY(ruin(c,10,2));TRY(ground(c,p));TRY(sprite(c,7,130,1));TRY(sprite(c,2,100,0x3b));TRY(sprite(c,70,138,1));TRY(sprite(c,56,146,0x27));TRY(sprite(c,50,152,0x27));TRY(sprite(c,45,158,0x29));TRY(sprite(c,59,156,0x28));TRY(sprite(c,26,164,0x27));return sprite(c,6,168,0x28);
    case 11:TRY(ruin(c,18,0));TRY(bushes(c,52,114,0));TRY(bushes(c,1,112,0));TRY(ground(c,p));TRY(foliage_tower(c,0));TRY(foreground(c));return rocks(c,67,174);
    case 12:TRY(bushes(c,1,120,0));TRY(bushes(c,22,114,1));TRY(bushes(c,54,118,1));TRY(ground(c,p));TRY(rocks(c,68,174));TRY(foreground(c));return sprite(c,1,156,0x44);
    case 13:TRY(ruin(c,0,2));TRY(ruin(c,44,1));TRY(ground(c,p));TRY(sprite(c,76,110,0x3b));TRY(sprite(c,70,140,1));TRY(sprite(c,10,160,0x27));TRY(sprite(c,5,166,0x29));return sprite(c,19,164,0x28);
    case 14:TRY(ruin(c,10,0));TRY(ruin(c,46,2));TRY(ground(c,p));TRY(sprite(c,22,154,0x28));TRY(sprite(c,66,150,0x29));return sprite(c,73,30,0x52);
    case 15:TRY(ruin(c,34,1));TRY(bushes(c,1,120,1));TRY(bushes(c,30,116,0));TRY(ground(c,p));TRY(rocks(c,1,174));TRY(sprite(c,73,142,0x20));TRY(sprite(c,64,102,0x45));return sprite(c,71,156,0x44);
    case 16:TRY(ruin(c,39,2));TRY(bushes(c,14,112,1));TRY(ground(c,p));TRY(spires(c));TRY(sprite(c,0,100,0x45));return rocks(c,1,178);
    case 17:TRY(bushes(c,1,120,1));TRY(bushes(c,27,114,1));TRY(sprite(c,63,102,0x45));TRY(sprite(c,50,108,0x45));TRY(rocks(c,56,148));TRY(ground(c,p));return sprite(c,71,156,0x44);
    case 18:TRY(ruin(c,30,1));TRY(ruin(c,-1,2));TRY(ruin(c,48,0));TRY(ground(c,p));TRY(sprite(c,64,164,0x28));TRY(sprite(c,72,172,0x29));return foreground(c);
    case 19:TRY(ruin(c,36,2));TRY(bushes(c,2,112,0));TRY(ground(c,p));TRY(spires(c));TRY(sprite(c,22,100,0x45));TRY(rocks(c,1,178));TRY(rocks(c,45,146));return foreground(c);
    case 20:TRY(ruin(c,6,1));TRY(bushes(c,32,114,1));TRY(bushes(c,54,118,1));TRY(ground(c,p));TRY(sprite(c,68,50,0x52));TRY(sprite(c,74,36,0x52));TRY(foliage_tower(c,70));TRY(sprite(c,26,160,0x27));TRY(sprite(c,21,166,0x29));TRY(sprite(c,35,164,0x28));TRY(foreground(c));return sprite(c,54,176,0x4f);
    case 21:TRY(ruin(c,10,0));TRY(bushes(c,38,114,1));TRY(ground(c,p));TRY(foliage_tower(c,0));TRY(bushes(c,58,118,0));TRY(foreground(c));TRY(sprite(c,0,100,0x45));return rocks(c,67,174);
    case 22:TRY(bushes(c,2,112,0));TRY(ground(c,p));TRY(spires(c));TRY(sprite(c,22,100,0x45));TRY(rocks(c,1,178));TRY(rocks(c,45,146));return foreground(c);
    default:return true;
    }
}
static bool battlements(Canvas *c,int x,int y,int n)
{
    int i;for(i=0;i<n;++i) {
        TRY(line(c,x+12*i,y,x+12*i+6,y,2));TRY(line(c,x+12*i+6,y,x+12*i+6,y-4,2));
        TRY(line(c,x+12*i+6,y-4,x+12*i+12,y-4,2));TRY(line(c,x+12*i+12,y-4,x+12*i+12,y,2));
    }return true;
}
static bool castle_front(Canvas *c,unsigned wall)
{
    if(wall==1) return true;
    TRY(line(c,124,140,196,140,2));
    if(!wall) {TRY(battlements(c,124,60,6));return sprite(c,37,80,0x2e);}
    TRY(line(c,124,140,124,30,2));TRY(line(c,196,140,196,30,2));TRY(battlements(c,124,30,6));
    /* C7A55(156,140,20,12,2), puis FillShape(158,138,2,2). */
    TRY(color(c,2));TRY(tera_graph_arch(c->r,156,140,20,12));
    TRY(tera_graph_flood(c->r,158,138,2,2));
    TRY(rect(c,144,64,147,80,2,0));return rect(c,173,64,176,80,2,0);
}
static bool slanted_battlements(Canvas *c,int first,int right)
{
    int i,sum=right ? 1 : 1+first,sign=right ? 1 : -1,origin=right ? 196 : 124;
    for(i=first;i<=7;++i) {
        int x,x2,x3,y,y2,y3,y4;sum+=i;
        x=origin+sign*(8*i+sum);x2=x+sign*4;x3=origin+sign*(8*(i+1)+sum+i);
        y=60-4*i-sum;y2=58-4*i-sum;y3=54-5*i-sum;y4=52-6*i-sum;
        TRY(line(c,x,y,x2,y2,2));TRY(line(c,x2,y2,x2,y3,2));
        TRY(line(c,x2,y3,x3,y4,2));TRY(line(c,x3,y4,x3,60-4*(i+1)-sum-i,2));
    }
    return true;
}
static bool castle_side(Canvas *c,unsigned wall,int right)
{
    int inner=right ? 196 : 124,outer=right ? 316 : 4;
    if(wall==1) {
        TRY(line(c,inner,140,outer,140,2));TRY(battlements(c,right ? 196 : 4,60,10));
        {int i;for(i=1;i<=5;++i) TRY(line(c,24*i+(right ? 193 : -11),64,24*i+(right ? 193 : -11),84,2));}
        TRY(line(c,inner,140,inner,60,2));TRY(sprite(c,right ? 54 : 21,80,0x2e));
        return sprite(c,right ? 72 : 3,80,0x2e);
    }
    TRY(slanted_battlements(c,wall>=2 ? 3 : 0,right));
    if(wall>=2) {
        int a=right ? 199 : 121,b=right ? 225 : 93;
        TRY(line(c,a,154,b,154,2));TRY(line(c,a,154,a,4,2));TRY(line(c,b,154,b,right ? 44 : 42,2));
        TRY(battlements(c,right ? 199 : 50,4,6));TRY(rect(c,right ? 210 : 110,65,right ? 208 : 112,40,2,0));
        TRY(line(c,b,154,right ? 319 : 0,199,2));TRY(byte_rect(c,right ? 50 : 29,112,right ? 51 : 30,152,2));
    } else {TRY(line(c,inner,140,right ? 319 : 0,199,2));TRY(line(c,inner,140,inner,60,2));}
    TRY(sprite(c,right ? 58 : 19,74,right ? 0x2f : 0x30));TRY(sprite(c,right ? 72 : 2,50,right ? 0x49 : 0x4a));
    return true;
}
/* BIN C6AF6 : les deux arguments sont largeur et hauteur, pas x/y.
 * La derniere marche depasse intentionnellement la hauteur demandee. */
static bool stairs(Canvas *c,int width,int height)
{
    int i=0,y=160;
    TRY(line(c,164-width,160,156+width,160,2));
    do {
        int left,right;++i;y=160-3*i;left=160-width+4*i;right=160+width-4*i;
        TRY(line(c,left,y,right,y,2));TRY(line(c,left,y+3,left,y,2));TRY(line(c,right,y+3,right,y,2));
    } while(3*(i-1)<=height);
    return true;
}
/* BIN C6FDB : grille de perspective employee par Meduz et le spatioport.
 * Les etoiles aleatoires ne sont pas inventees si aucune trace n'est fournie. */
static bool grid(Canvas *c)
{
    int i,y;
    for(i=1;i<=8;++i) {
        y=130+i*(i-1);TRY(line(c,0,y,319,y,2));TRY(line(c,i*40-40,199,160,130,2));
        TRY(line(c,0,y,160,130,2));TRY(line(c,319,y,160,130,2));
    }
    return line(c,319,199,160,130,2);
}
static bool star_points(Canvas *c,const unsigned short *v,unsigned count,int x,int y)
{
    unsigned i;TRY(color(c,3));
    for(i=0;i+1<count;i+=2) TRY(SDL_RenderPoint(c->r,(float)(x+v[i]),(float)(y+v[i+1])));
    return true;
}
static bool spaceport(Canvas *c,const TeraScenePicture *p)
{
    unsigned n=59,i;TRY(grid(c));
    if(!p || p->count<89) return true;
    TRY(star_points(c,p->values,32,10,4));TRY(skyline(c,p->values+32,27));
    for(i=1;i<=6;++i) {TRY(sprite(c,25+6*(int)i+p->values[n],130+2*p->values[n+1],0x1e));n+=2;}
    for(i=1;i<=6;++i) {TRY(sprite(c,27+6*(int)i+p->values[n],136+p->values[n+1],0x1e));n+=2;}
    for(i=1;i<=3;++i) {TRY(sprite(c,30+10*(int)i+p->values[n],144+2*p->values[n+1],0x1d));n+=2;}
    return true;
}
static bool pit_side(Canvas *c,int right)
{
    static const int right_points[][4]={{319,199,160,130},{258,186,317,186},{234,172,277,172},
        {214,160,244,160},{195,150,215,150},{182,142,193,142},{172,136,175,136}};
    static const int left_points[][4]={{0,199,160,130},{61,186,2,186},{86,172,43,172},
        {106,160,76,160},{125,150,105,150},{138,142,127,142},{148,136,145,136}};
    const int (*points)[4]=right ? right_points : left_points;unsigned i;
    for(i=0;i<7;++i) TRY(line(c,points[i][0],points[i][1],points[i][2],points[i][3],0));
    TRY(line(c,right ? 245 : 75,160,right ? 245 : 75,177,2));
    return line(c,right ? 216 : 105,150,right ? 216 : 105,160,2);
}
/* OVL0049 BDBB/C02C : trois perspectives, passage ou porte pleine.
 * Le polygone conserve les sommets du contour de FillShape original. */
static bool side_door(Canvas *c,int size,unsigned wall,int right)
{
    int outer=right ? 295 : 25,inner=right ? (size==1 ? 285 : size==2 ? 280 : 270) : (size==1 ? 35 : size==2 ? 40 : 50);
    int base=size==1 ? 139 : size==2 ? 165 : 168,edge=size==1 ? 136 : size==2 ? 157 : 161;
    int top=size==1 ? 98 : size==2 ? 46 : -1;
    if(!wall) return true;
    if(size==2) {
        TRY(line(c,outer,right ? base : top,outer,right ? top : base,1));
        TRY(line(c,inner,right ? edge : 56,inner,right ? 56 : edge,1));
        TRY(line(c,right ? inner : outer,right ? 56 : top,right ? outer : inner,right ? top : 56,1));
    } else {
        TRY(rect(c,outer,edge,inner,top,1,0));TRY(line(c,outer,edge,outer,base,1));
    }
    if(wall==1) {
        if(size==2) {TRY(line(c,outer,edge,inner,edge,1));TRY(line(c,outer,56,inner,56,1));}
        if(size==1 && right) TRY(line(c,inner,edge,outer,base,0));
        else TRY(line(c,outer,base,inner,edge,0));
    } else {
        TRY(line(c,outer+(right ? -1 : 1),edge,inner+(right ? 1 : -1),edge,0));
        if(size==1 && right) TRY(line(c,inner,edge,outer,base,1));
        else TRY(line(c,outer,base,inner,edge,1));
        TRY(tera_graph_flood(c->r,right ? 288 : size==1 ? 27 : 28,130,3,1));
        if(size>=2) TRY(line(c,right ? 297 : 23,base,right ? 297 : 23,size==2 ? top : 0,1));
    }
    return true;
}
static bool room(Canvas *c,const TeraSceneView *s,int heading)
{
    int x=90,y=50,xx=230,yy=150,size=3,side;
    if(s->kind==TERA_SCENE_RUINS) {
        const TeraScenePicture *p=details(c,s,TERA_PICTURE_RUINS);unsigned pass,i;
        if(!s->heading_valid) return true;
        for(pass=0;pass<2;++pass) {
            int right=heading==10 ? pass!=0 : pass==0;unsigned wall=right ? s->right : s->left;
            TRY(castle_side(c,wall,right));
            if(p) for(i=0;i<3;++i) {
                unsigned n=9*pass+3*i;
                if(n+2<p->count) TRY(sprite(c,p->values[n],p->values[n+1],p->values[n+2]));
            }
            if(!right && wall==1) {TRY(sprite(c,9,176,0x21));TRY(sprite(c,15,182,0x21));TRY(sprite(c,3,176,0x20));}
        }
        return castle_front(c,s->front);
    }
    if(s->kind==TERA_SCENE_MEDUZ) {
        TRY(grid(c));
        {const TeraScenePicture *p=details(c,s,TERA_PICTURE_MEDUZ);
         if(p && p->count>=32) TRY(star_points(c,p->values,32,10,4));}
        if(!s->heading_valid) return true;
        if(!s->front) {TRY(byte_rect(c,0,152,80,158,0));TRY(line(c,0,151,319,151,0));
            TRY(line(c,195,150,195,158,2));TRY(line(c,125,150,125,158,2));}
        if(!s->left) TRY(pit_side(c,0));
        if(!s->right) TRY(pit_side(c,1));
        return true;
    }
    if(s->kind!=TERA_SCENE_ALFOL_INTERIOR) {
        if(s->variant==2) {x=80;y=60;xx=240;yy=140;size=2;}
        else if(s->variant==3) {x=60;y=70;xx=260;yy=130;size=1;}
        else if(s->variant!=1) return true; /* Aucun type de salle invente. */
    } else {x=80;y=60;xx=240;yy=140;size=2;}
    TRY(line(c,size==3 ? 55 : size==2 ? 10 : 1,size==1 ? 50 : 1,x,y,1));
    TRY(line(c,size==3 ? 255 : size==2 ? 309 : 319,size==1 ? 50 : 1,xx,y,1));
    TRY(line(c,1,size==1 ? 145 : 175,x,yy,1));TRY(line(c,319,size==1 ? 145 : 175,xx,yy,1));
    TRY(rect(c,x,y,xx,yy,1,0));
    if(s->kind!=TERA_SCENE_ALFOL_INTERIOR) {
        if(size==3) {TRY(sprite(c,20,60,0x26));TRY(sprite(c,56,60,0x0d));}
        if(size==2) {TRY(sprite(c,15,78,0x25));TRY(sprite(c,62,78,0x24));TRY(sprite(c,24,80,0x23));TRY(sprite(c,50,80,0x23));}
        if(size==1) {TRY(sprite(c,76,60,0x0d));TRY(sprite(c,0,60,0x26));}
    }
    for(side=0;side<2;++side) {
        unsigned w=side ? s->right : s->left;
        if(s->kind==TERA_SCENE_ALFOL_INTERIOR) {
            if(w) {int a=side ? 298 : 21,b=side ? 294 : 25,d=side ? 279 : 40,e=side ? 275 : 44;
                TRY(line(c,a,165,b,86,1));TRY(line(c,b,86,d,86,1));TRY(line(c,d,86,e,157,1));
                TRY(color(c,1));TRY(SDL_RenderPoint(c->r,(float)e,120));}
        } else TRY(side_door(c,size,w,side));
    }
    if(s->front) {
        if(s->kind==TERA_SCENE_ALFOL_INTERIOR) {
            TRY(line(c,176,86,182,140,1));TRY(line(c,138,140,144,86,1));TRY(line(c,144,86,176,86,1));
            return sprite(c,46,110,0x2d);
        }
        TRY(rect(c,160-12*size,yy-32*size,160+12*size,yy,1,0));
        if(s->kind!=TERA_SCENE_ALFOL_INTERIOR && s->front>=2) {
            unsigned id=size==1 ? 0x1b : size==2 ? 0x42 : 0x51;int row,col;
            for(col=1;col<=2;++col) for(row=1;row<=3;++row)
                TRY(sprite(c,(163+size+(col-2)*10*size)/4,yy-row*size*10+size,id));
            TRY(rect(c,159-10*size,yy-30*size,161+10*size,yy-1,3,0));
        } else if(s->kind!=TERA_SCENE_ALFOL_INTERIOR) {
            int low=size==1 ? 1 : size==2 ? 8 : 11,high=size==1 ? 30 : size==2 ? 50 : 54;
            TRY(line(c,160-12*size,yy,160+12*size,yy,0));
            TRY(rect(c,150,yy-low,170,yy-high,1,0));
            TRY(line(c,150,yy-low,170,yy-low,0));
            TRY(line(c,160-12*size,yy,150,yy-low,1));TRY(line(c,160+12*size,yy,170,yy-low,1));
            TRY(line(c,160-12*size,yy-32*size,150,yy-high,1));TRY(line(c,160+12*size,yy-32*size,170,yy-high,1));
        }
    }
    return true;
}
static bool temple(Canvas *c)
{
    int i;TRY(stairs(c,162,19));
    for(i=1;i<=7;++i) {
        int x=i*40;TRY(byte_rect(c,(x-4)/4,128,(x+8)/4,134,2));TRY(byte_rect(c,(x-4)/4,46,(x+8)/4,54,2));
        TRY(line(c,x-2,126,x-2,54,2));TRY(line(c,x+5,126,x+5,54,2));
    }
    TRY(line(c,6,43,314,43,2));TRY(line(c,6,43,160,1,2));TRY(line(c,314,43,160,1,2));
    return true;
}
static bool place(Canvas *c,unsigned id,const TeraScenePicture *p)
{
    unsigned i,n;
    switch(id) {
    case 0:return tera_background_draw(c->r,c->tex,3);
    case 6:case 9:return tera_background_draw(c->r,c->tex,2);
    case 7:return tera_background_draw(c->r,c->tex,4);
    case 8:return tera_background_draw(c->r,c->tex,5);
    case 1:
        TRY(spaceport(c,p));return sprite(c,73,36,0x52);
    case 2:
        TRY(castle_side(c,1,0));
        if(p && p->count>=23) for(i=0;i<3;++i) {
            unsigned k=1+4*i;static const unsigned ids[]={0x31,0x37,0x38};
            TRY(sprite(c,p->values[k],148+2*p->values[k+1],ids[p->values[k+3]%3]));
        }
        TRY(sprite(c,9,176,0x21));TRY(sprite(c,15,182,0x21));TRY(sprite(c,3,176,0x20));
        TRY(castle_front(c,2));TRY(castle_side(c,2,1));
        if(p && p->count>=23) {
            unsigned bound=2+2*p->values[13];static const unsigned ids[]={0x31,0x37,0x38};
            for(i=0;i<3;++i) {unsigned k=14+3*i;
                TRY(sprite(c,40+p->values[k],160+(int)bound,ids[p->values[k+2]%3]));bound=2+2*p->values[k+1];}
        }
        TRY(sprite(c,38,40,6));TRY(sprite(c,34,170,0x27));TRY(sprite(c,29,176,0x29));return sprite(c,43,174,0x28);
    case 3:
        TRY(disc(c,76,30,8,3));
        if(p && p->count>=76) TRY(star_points(c,p->values+52,24,10,4));
        TRY(stairs(c,162,100));TRY(layer_begin(c));
        TRY(sprite(c,24,150,0x2a));TRY(sprite(c,22,148,0x2b));TRY(sprite(c,16,148,0x2c));TRY(sprite(c,12,154,0x2c));
        TRY(bushes(c,45,140,1));TRY(sprite(c,37,162,0x3a));
        if(p && p->count>=52) for(i=0;i<26;++i) TRY(sprite(c,p->values[2*i],176+2*p->values[2*i+1],i<14 ? 0x36 : 0x35));
        TRY(sprite(c,72,32,0x52));TRY(sprite(c,68,36,0x52));
        TRY(sprite(c,58,168,0x4f));TRY(foreground(c));
        return layer_end(c);
    case 4:case 5:
        TRY(temple(c));
        if(id==5) TRY(sprite(c,34,15,0x4e));
        TRY(layer_begin(c));
        if(id==4) {TRY(sprite(c,34,15,0x4e));TRY(spires(c));TRY(sprite(c,1,158,0x44));}
        else {TRY(spires(c));TRY(rocks(c,0,170));}
        if(p && p->count>=18) for(n=0;n<9;++n) TRY(sprite(c,p->values[2*n],168+2*p->values[2*n+1],n<4 ? 0x35 : 0x36));
        return layer_end(c);
    default:return true;
    }
}
static bool alfol_surface(Canvas *c,const TeraSceneView *s)
{
    const TeraScenePicture *p=details(c,s,TERA_PICTURE_ALFOL);TeraLandscape land={0};unsigned i,h,n;
    if(!p || !p->count) {
        if(!s->variant) {TRY(spaceport(c,NULL));return foreground(c);}
        if(s->variant==1) {TRY(ruin(c,40,2));TRY(sprite(c,72,32,0x52));return foreground(c);}
        return country(c,(unsigned)s->position%23,NULL);
    }
    if(p->variant==0) {TRY(spaceport(c,p));TRY(layer_begin(c));TRY(foreground(c));return layer_end(c);}
    if(p->count<33 || p->count>256) return true;
    h=p->variant==1 ? 0 : p->count-33;
    if(h+33>p->count) return true;
    land.horizon_kind=p->values[h]==1 ? 0 : 1;land.horizon_height=20;
    for(i=0;i<16;++i) {land.stars[i].x=10+p->values[h+1+2*i];land.stars[i].y=4+p->values[h+2+2*i];}
    if(p->variant==1 && p->count>=173) {
        land.mountain_count=35;memcpy(land.mountains,p->values+33,sizeof land.mountains);
        TRY(landscape(c,&land));TRY(skyline(c,p->values+68,27));
        n=95;land.count=39;
    } else {TRY(landscape(c,&land));n=0;land.count=h/2;}
    for(i=0;i<land.count && i<65;++i) {
        land.ground[i].x=p->values[n+2*i];land.ground[i].y=148+2*p->values[n+2*i+1];
        land.ground[i].sprite=i%13<4 ? 0x14 : i%13<8 ? 0x35 : 0x36;
    }
    TRY(layer_begin(c));
    if(p->variant==1) {TRY(ruin(c,40,2));TRY(ground(c,&land));TRY(sprite(c,72,32,0x52));TRY(foreground(c));}
    else TRY(country(c,(unsigned)s->position%23,&land));
    return layer_end(c);
}
/* C724A : deux marques possibles par cellule, sans nouveau tirage. */
static bool scriptures(Canvas *c,int x,int y,const TeraScenePicture *p)
{
    unsigned col,row,n=0;if(!p || p->count<96) return true;
    TRY(color(c,0));
    for(col=1;col<=12;++col) for(row=1;row<=4;++row) {
        if(p->values[n++]==1) TRY(SDL_RenderPoint(c->r,(float)(x+(int)col),(float)(y+3*(int)row)));
        if(p->values[n++]==1) TRY(SDL_RenderPoint(c->r,(float)(x+(int)col),(float)(y-1+3*(int)row)));
    }
    return true;
}
static bool writing(Canvas *c,const TeraSceneView *s)
{
    const TeraScenePicture *p=details(c,s,TERA_PICTURE_FACADE);
    if(s->writing_kind==2) {
        TRY(tera_scene_monolith(c->r));
        return scriptures(c,106,90,p);
    }
    TRY(byte_rect(c,31,140,35,156,2));TRY(byte_rect(c,45,140,49,156,2));
    TRY(byte_rect(c,28,126,52,136,2));TRY(byte_rect(c,31,138,35,140,0));TRY(byte_rect(c,45,138,49,140,0));
    return scriptures(c,152,125,p);
}
bool tera_scene_monolith(SDL_Renderer *r)
{
    Canvas canvas={r,NULL,NULL,0,NULL};Canvas *c=&canvas;
    TRY(byte_rect(c,30,40,32,156,0));TRY(byte_rect(c,26,40,30,156,2));
    TRY(line(c,119,36,104,40,2));TRY(line(c,119,36,119,40,2));
    TRY(line(c,119,36,128,40,2));TRY(line(c,128,40,128,155,2));
    TRY(line(c,128,156,119,158,2));TRY(line(c,119,158,104,156,2));
    return tera_graph_flood(r,117,39,2,2);
}
static bool feature(Canvas *c,const TeraSceneView *s)
{
    unsigned i;const TeraScenePicture *p=details(c,s,TERA_PICTURE_ROOM);
    switch(s->feature) {
    case TERA_FEATURE_CHEST:return sprite(c,35,168,0x3c);
    case TERA_FEATURE_OPEN_CHEST:return sprite(c,35,168,0x3c);
    case TERA_FEATURE_UP:case TERA_FEATURE_DOWN:
        TRY(rect(c,113,s->feature==TERA_FEATURE_UP ? 33 : 153,135,s->feature==TERA_FEATURE_UP ? 39 : 159,1,0));
        for(i=s->feature==TERA_FEATURE_UP ? 3 : 12;i<=14;++i) TRY(sprite(c,29,(int)i*10+6,0x46));
        return true;
    case TERA_FEATURE_ALTAR:
        if(s->writing_kind) return writing(c,s);
        TRY(sprite(c,34,80,0x4e));TRY(sprite(c,6,80,6));return sprite(c,70,80,6);
    case TERA_FEATURE_WRITING:return writing(c,s);
    case TERA_FEATURE_MEDITATION:case TERA_FEATURE_INVOCATION:return skyscraper(c,38,164,140,4,2,0);
    case TERA_FEATURE_OFFICE:
        TRY(byte_rect(c,28,142,52,172,1));TRY(byte_rect(c,27,136,53,138,1));
        TRY(byte_rect(c,28,138,52,140,0));TRY(byte_rect(c,46,70,56,90,3));
        if(p && p->variant==1 && p->count>=8) {
            int x=190,y=88;
            for(i=0;i<4;++i) {int w=2+p->values[2*i],h=2+p->values[2*i+1];
                TRY(line(c,x,y,x+w,y-h,0));TRY(line(c,x+w,y-h,x+2*w,y-h+w,0));x+=2*w;y+=w-h;}
        }
        TRY(sprite(c,38,112,0x18));TRY(line(c,152,128,144,130,3));TRY(line(c,164,128,172,130,3));
        return color(c,3) && tera_text_draw(c->r,96,64,"BUREAU DES MEDIAS",20,1);
    case TERA_FEATURE_RADIATION:
        if(p && p->variant==2 && p->count>=18) {
            unsigned row,col,stack,n=0;
            for(row=1;row<=3;++row) for(col=1;col<=6;++col) {
                unsigned height=4u+p->values[n++];
                for(stack=1;stack<=height;++stack) TRY(sprite(c,27+4*(int)col-(int)row,160-12*(int)stack+4*(int)row,9));
            }
        }
        return true;
    default:return true;
    }
}
static bool flight(Canvas *c,const TeraSpaceSession *s)
{
    unsigned elapsed;int mx,my,ox,oy;unsigned id;
    if(s->phase==TERA_SPACE_CONFIRM) return place(c,1,NULL);
    TRY(tera_background_draw(c->r,c->tex,0));
    if(c->tex->scenery[TERA_PICTURE_SPACE].picture.count>=48)
        TRY(star_points(c,c->tex->scenery[TERA_PICTURE_SPACE].picture.values,48,60,40));
    if(s->phase!=TERA_SPACE_SHOT || s->input_ms<s->delay_ms) return true;
    elapsed=s->input_ms-s->delay_ms;
    /* OVL00AE : le core expose le temps et la cible. Ceci dessine seulement
     * l'image courante ; aucun choix de touche juste ou de fin du tir. */
    if(elapsed<300) return color(c,3) && SDL_RenderPoint(c->r,(float)(155+s->alien_x*4),(float)(90+s->alien_y*2));
    if(elapsed<530) {mx=8;my=4;ox=3;oy=2;id=0x1f;}
    else if(elapsed<730) {mx=16;my=6;ox=6;oy=4;id=0x1e;}
    else if(elapsed<900) {mx=32;my=12;ox=12;oy=6;id=0x1d;}
    else {mx=64;my=24;ox=24;oy=12;id=0x43;}
    return sprite(c,(155+s->alien_x*mx-ox)/4,90+s->alien_y*my-oy,id);
}
bool tera_scene_draw(SDL_Renderer *r,const TeraGameView *v,const TeraResources *res,const TeraTextures *tex)
{
    Canvas c={r,res,tex,0,NULL};const TeraSceneView *s=&v->scene;
    TRY(tera_video_mode(r,0));
    TRY(color(&c,0));TRY(SDL_RenderClear(r));
    if(v->space) return flight(&c,v->space);
    if(s->kind==TERA_SCENE_PLACE) TRY(place(&c,s->place,details(&c,s,TERA_PICTURE_PLACE)));
    else if(s->kind==TERA_SCENE_AMARANDE) {
        TRY(landscape(&c,s->landscape));TRY(layer_begin(&c));
        TRY(country(&c,s->variant,s->landscape));TRY(layer_end(&c));
    }
    else if(s->kind==TERA_SCENE_ALFOL_SURFACE) {
        TRY(alfol_surface(&c,s));
    } else TRY(room(&c,s,v->characters->combat.position-v->characters->combat.previous_position));
    TRY(color(&c,3));TRY(feature(&c,s));
    if(s->kind==TERA_SCENE_MEDUZ) {
        const TeraScenePicture *p=details(&c,s,TERA_PICTURE_MEDUZ);
        if(p && p->count>=32) {
            unsigned i;
            for(i=32;i<p->count && i<37;++i) TRY(sprite(&c,5+p->values[i],96+12*(int)(i-31),9));
        }
    }
    /* Seulement les associations explicites du Pascal, jamais une silhouette
     * choisie par ressemblance. Les autres PNJ sont signales en texte. */
    if(s->kind==TERA_SCENE_RUINS || s->kind==TERA_SCENE_INTERIOR) {
        if(s->npc_mask&(1u<<1)) TRY(sprite(&c,21,86,0x48));
        if(s->npc_mask&(1u<<2)) TRY(tera_scene_portrait(r,res,tex,0,1,2));
        if(s->npc_mask&(1u<<6)) {TRY(byte_rect(&c,43,96,49,180,0));TRY(sprite(&c,45,100,0x1a));}
    }
    if(s->kind==TERA_SCENE_PLACE && s->place==1 && (s->npc_mask&(1u<<4))) TRY(sprite(&c,70,150,0x10));
    if(s->kind==TERA_SCENE_PLACE && s->place_npc) {
        if(s->place==5 || s->place==9) TRY(tera_scene_portrait(r,res,tex,s->place==5 ? 12 : -64,s->place==5 ? 3 : 2,0));
        else if(s->place==6 || s->place==8) TRY(tera_scene_portrait(r,res,tex,s->place==6 ? -18 : -90,s->place==6 ? 2 : 3,1));
        else if(s->place==7) {TRY(sprite(&c,18,112,0x3e));TRY(sprite(&c,38,140,0x41));}
    }
    if(s->ambient_npc) TRY(sprite(&c,s->npc_variant==2 ? 40 : 60,s->npc_variant==2 ? 106 : 130,
        s->npc_variant==2 ? 0x3f : s->npc_variant==1 ? 0x0e : 0x10));
    return true;
}



/* CA709/H : histogramme decoratif du programme (VERBES 40/41),
 * distinct de l'aide OVL00E2. Tirages deja captures par le moteur. */
bool tera_halt_screen(SDL_Renderer *r,const TeraResources *res,const TeraTextures *textures)
{
    Canvas c={r,res,textures,0,NULL};unsigned i;
    const TeraScenePicture *p=&textures->scenery[TERA_PICTURE_HALT].picture;
    TRY(tera_video_mode(r,0));
    TRY(color(&c,0));TRY(SDL_RenderClear(r));
    for(i=1;i<=8;++i) TRY(line(&c,0,20*(int)i,310,20*(int)i,1));
    if(p->count>=27) for(i=0;i<9;++i) TRY(skyscraper(&c,8*(int)(i+1)+p->values[3*i]-7,
        160,40+p->values[3*i+1],5+p->values[3*i+2],2,0));
    TRY(rect(&c,0,0,310,180,1,0));TRY(color(&c,3));
    return tera_text_draw(r,0,0,tera_resource_text(res,TERA_RES_VERBS,40),40,1) &&
        tera_text_draw(r,96,192,tera_resource_text(res,TERA_RES_VERBS,41),28,1);
}
