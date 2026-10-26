#include "cga.h"
void tera_cga_plot(uint8_t *p,int x,int y,unsigned color)
{if(x>=0 && x<320 && y>=0 && y<200) p[y*320+x]=(uint8_t)color;}
void tera_cga_walk_line(int x,int y,int xx,int yy,void (*plot)(void *,int,int),void *context)
{
    int dx=xx>x ? xx-x : x-xx,sx=x<xx ? 1 : -1;
    int dy=yy>y ? yy-y : y-yy,sy=y<yy ? 1 : -1;
    int major=dx>=dy ? dx : dy,minor=dx>=dy ? dy : dx,err=major/2,i;
    for(i=0;i<=major;++i) {
        if(x>=0 && x<320 && y>=0 && y<200) plot(context,x,y);
        err-=minor;
        if(err<0) {err+=major;if(dx>=dy) y+=sy;else x+=sx;}
        if(dx>=dy) x+=sx;else y+=sy;
    }
}
typedef struct {uint8_t *pixels;unsigned color;} LinePixels;
static void line_pixel(void *context,int x,int y)
{LinePixels *p=context;tera_cga_plot(p->pixels,x,y,p->color);}
void tera_cga_line(uint8_t *p,int x,int y,int xx,int yy,unsigned color)
{LinePixels context={p,color};tera_cga_walk_line(x,y,xx,yy,line_pixel,&context);}
void tera_cga_fill(uint8_t *p,int x,int y,int xx,int yy,unsigned color)
{
    int a,b,t;if(xx<x) {t=x;x=xx;xx=t;}if(yy<y) {t=y;y=yy;yy=t;}
    for(b=y;b<=yy;++b) for(a=x;a<=xx;++a) tera_cga_plot(p,a,b,color);
}
void tera_cga_bytes(uint8_t *p,int x,int y,int xx,int yy,unsigned color)
{
    int a,b;y=2*(y/2);yy=2*(yy/2)+1;
    for(b=y;b<=yy;++b) for(a=x*4;a<xx*4;++a)
        if(a>=0 && a<320 && b>=0 && b<200) p[b*320+a]=(uint8_t)color;
}
/* GRAPH 3617 : Newton entier, arrondi du quotient puis symetries. */
void tera_cga_circle(uint8_t *p,int x,int y,unsigned radius,unsigned color)
{
    unsigned u=radius ? radius : 1,v=0,square=u*u;
    do {
        int a=(int)u,b=(int)v;
        tera_cga_plot(p,x+a,y+b,color);tera_cga_plot(p,x-a,y+b,color);
        tera_cga_plot(p,x+a,y-b,color);tera_cga_plot(p,x-a,y-b,color);
        tera_cga_plot(p,x+b,y+a,color);tera_cga_plot(p,x-b,y+a,color);
        tera_cga_plot(p,x+b,y-a,color);tera_cga_plot(p,x-b,y-a,color);
        ++v;
        {unsigned n=square-v*v,q=n/u,rem=n%u;u=(q+u+(2*rem>u))/2;}
    } while(u>=v);
}
void tera_cga_flood(uint8_t *p,int x,int y,unsigned color,unsigned border)
{
    uint16_t queue[64000];unsigned head=0,tail=0;
    if(x<0 || x>=320 || y<0 || y>=200 || p[y*320+x]==color || p[y*320+x]==border) return;
    p[y*320+x]=(uint8_t)color;queue[tail++]=(uint16_t)(y*320+x);
    while(head<tail) {
        unsigned n=queue[head++],i;int neighbors[4]={(int)n-320,(int)n+320,(int)n-1,(int)n+1};
        for(i=0;i<4;++i) {
            int next=neighbors[i];
            if(next<0 || next>=64000 || (i==2 && n%320==0) || (i==3 && n%320==319)) continue;
            if(p[next]==color || p[next]==border) continue;
            p[next]=(uint8_t)color;queue[tail++]=(uint16_t)next;
        }
    }
}
