#include "xt.h"
#include "xt_profile.h"
static unsigned point_cycles(int x,int y)
{
    if(x<0) return xt_point_clip[0];
    if(x>=320) return xt_point_clip[1];
    if(y<0) return xt_point_clip[2];
    if(y>=200) return xt_point_clip[3];
    return xt_point_visible[4*(y&1)+(x&3)];
}
uint64_t tera_xt_circle_cycles(int x,int y,unsigned radius)
{
    unsigned u=radius ? radius : 1,v=0,square=u*u;uint64_t cycles;
    if(u>100) return 0;
    cycles=xt_circle_core[u];
    do {
        int a=(int)u,b=(int)v;unsigned i,j;
        const int points[8][2]={{a,b},{-a,b},{a,-b},{-a,-b},{b,a},{-b,a},{b,-a},{-b,-a}};
        /* GRAPH ne repete pas les axes ni la diagonale, contrairement a
         * une boucle qui tracerait toujours huit pixels identiques. */
        for(i=0;i<8;++i) {
            for(j=0;j<i;++j) if(points[j][0]==points[i][0] && points[j][1]==points[i][1]) break;
            if(j==i) cycles+=point_cycles(x+points[i][0],y+points[i][1]);
        }
        ++v;{unsigned n=square-v*v,q=n/u,rem=n%u;u=(q+u+(2*rem>u))/2;}
    } while(u>=v);
    return cycles;
}
uint64_t tera_xt_tunnel_cycles(const TeraTunnelFrame *f)
{
    unsigned i,flags=(f->palette<3 ? 1u : 0u)|(f->background ? 2u : 0u)|
        (f->erase_radius ? 4u : 0u)|(f->hz ? 8u : 0u);
    uint64_t cycles=xt_frame_overhead[flags]+tera_xt_circle_cycles(f->x,f->y,f->radius);
    if(f->erase_radius) cycles+=tera_xt_circle_cycles(f->erase_x,f->erase_y,f->erase_radius);
    if(f->reset) {
        cycles+=TERA_XT_TUNNEL_SETUP;
        for(i=0;i<24;++i) cycles+=point_cycles(f->stars[2*i],f->stars[2*i+1]);
    }
    return cycles;
}
