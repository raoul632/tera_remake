#include "graph.h"
#include "cga.h"
#include "sprites.h"
#include <string.h>
typedef struct {SDL_Renderer *r;SDL_FPoint points[256];unsigned count;bool ok;} Points;
static void point(void *context,int x,int y)
{
    Points *p=context;
    p->points[p->count].x=(float)x;p->points[p->count++].y=(float)y;
    if(p->count==256) {if(!SDL_RenderPoints(p->r,p->points,256)) p->ok=false;p->count=0;}
}
bool tera_graph_line(SDL_Renderer *r,int x,int y,int xx,int yy)
{
    Points p={0};p.r=r;p.ok=true;
    tera_cga_walk_line(x,y,xx,yy,point,&p);
    return p.ok && (!p.count || SDL_RenderPoints(r,p.points,(int)p.count));
}
bool tera_graph_bytes(SDL_Renderer *r,int x,int y,int xx,int yy)
{
    SDL_FRect area={(float)(4*x),(float)(2*(y/2)),(float)(4*(xx-x)),(float)(2*(yy/2-y/2+1))};
    return xx<=x || yy<y || SDL_RenderFillRect(r,&area);
}
bool tera_graph_disc(SDL_Renderer *r,int x,int y,unsigned radius)
{
    uint8_t pixels[64000];unsigned row,col;memset(pixels,255,sizeof pixels);
    tera_cga_circle(pixels,x,y,radius,3);tera_cga_flood(pixels,x,y,3,3);
    for(row=0;row<200;++row) for(col=0;col<320;++col) if(pixels[row*320+col]==3)
        if(!SDL_RenderPoint(r,(float)col,(float)row)) return false;
    return true;
}
bool tera_graph_flood(SDL_Renderer *r,int x,int y,unsigned color,unsigned border)
{
    SDL_Surface *raw,*surface;SDL_FRect viewport;SDL_Rect crop;unsigned row,col;bool ok=true;
    uint8_t before[64000],pixels[64000],rgba[4];
    if(!SDL_GetRenderLogicalPresentationRect(r,&viewport)) return false;
    crop.x=(int)viewport.x;crop.y=(int)viewport.y;crop.w=(int)viewport.w;crop.h=(int)viewport.h;
    raw=SDL_RenderReadPixels(r,&crop);
    if(!raw) return false;
    surface=SDL_ConvertSurface(raw,SDL_PIXELFORMAT_RGBA32);SDL_DestroySurface(raw);
    if(!surface) return false;
    /* The scene target is logical 320x200; the window may be an integer scale. */
    for(row=0;row<200;++row) for(col=0;col<320;++col) {
        const uint8_t *p=(uint8_t *)surface->pixels+(row*(unsigned)surface->h/200)*surface->pitch+(col*(unsigned)surface->w/320)*4;
        before[row*320+col]=(uint8_t)(p[0]==85 ? 1 : p[1]==85 ? 2 : p[0] ? 3 : 0);
    }
    SDL_DestroySurface(surface);memcpy(pixels,before,sizeof pixels);
    tera_cga_flood(pixels,x,y,color,border);tera_cga_rgba(color,rgba);
    if(!SDL_SetRenderDrawColor(r,rgba[0],rgba[1],rgba[2],255)) return false;
    for(row=0;row<200 && ok;++row) for(col=0;col<320;++col)
        if(pixels[row*320+col]!=before[row*320+col] && !SDL_RenderPoint(r,(float)col,(float)row)) {ok=false;break;}
    return ok;
}
bool tera_graph_arch(SDL_Renderer *r,int x,int y,int height,int width)
{
    unsigned u=(unsigned)(width/2),v=0,square=u*u;int cx=x+(int)u,cy=y-height;
    if(!tera_graph_line(r,x,y,x,cy) || !tera_graph_line(r,x+width,y,x+width,cy) ||
       !tera_graph_line(r,x,y,x+width,y)) return false;
    if(!u) return true;
    do {
        SDL_FPoint p[4]={{(float)(cx+(int)u),(float)(cy-(int)v)},
            {(float)(cx-(int)u),(float)(cy-(int)v)},
            {(float)(cx+(int)v),(float)(cy-(int)u)},
            {(float)(cx-(int)v),(float)(cy-(int)u)}};
        unsigned n,q,rem;
        if(!SDL_RenderPoints(r,p,4)) return false;
        ++v;n=square-v*v;q=n/u;rem=n%u;
        /* GRAPH 3856 rounds this Newton update upward, unlike Circle. */
        u=(q+u+(2*rem>u)+1)/2;
    } while(u>=v);
    return true;
}
