#include "text.h"
#include "resources.h"
#include "vendor/font8x8/ibm_cp437.h"

const unsigned char *tera_text_glyph(unsigned char code,int text_mode)
{return text_mode ? tera_font_cga[code] : tera_font_bios[code];}

bool tera_text_draw(SDL_Renderer *r,int x,int y,const char *text,unsigned columns,unsigned lines)
{
    unsigned column=0,row=0;int width=0,height=0;SDL_RendererLogicalPresentation mode;
    const unsigned char *p=(const unsigned char *)text;
    if(!r || !text || !SDL_GetRenderLogicalPresentation(r,&width,&height,&mode)) return false;
    while(*p && row<lines) {
        unsigned char cp=*p++;const unsigned char *glyph;unsigned gx,gy;
        if(cp=='\r') continue;
        if(cp=='\n') {column=0;++row;continue;}
        if(column==columns) {column=0;if(++row>=lines) break;}
        glyph=tera_text_glyph(cp,width==640);
        for(gy=0;gy<8;++gy) for(gx=0;gx<8;++gx)
            if(glyph[gy]&(128u>>gx))
                if(!SDL_RenderPoint(r,(float)(x+(int)(column*8+gx)),(float)(y+(int)(row*8+gy)))) return false;
        ++column;
    }
    return true;
}

/* OVL0022/00CB INNER_BA2F : haut double, cotes et bas simples.
 * Dimensions en cellules, coins inclus (PAS octets D5/CD/B8, C0/C4/D9). */
bool tera_text_box(SDL_Renderer *r,int x,int y,unsigned columns,unsigned rows)
{
    unsigned i;
    if(columns<2 || rows<2) return false;
    if(!tera_text_draw(r,x,y,"\325",1,1) ||
       !tera_text_draw(r,x+(int)(columns-1)*8,y,"\270",1,1) ||
       !tera_text_draw(r,x,y+(int)(rows-1)*8,"\300",1,1) ||
       !tera_text_draw(r,x+(int)(columns-1)*8,y+(int)(rows-1)*8,"\331",1,1)) return false;
    for(i=1;i+1<columns;++i) if(!tera_text_draw(r,x+(int)i*8,y,"\315",1,1) ||
        !tera_text_draw(r,x+(int)i*8,y+(int)(rows-1)*8,"\304",1,1)) return false;
    for(i=1;i+1<rows;++i) if(!tera_text_draw(r,x,y+(int)i*8,"\263",1,1) ||
        !tera_text_draw(r,x+(int)(columns-1)*8,y+(int)i*8,"\263",1,1)) return false;
    return true;
}

bool tera_video_mode(SDL_Renderer *r,int text_mode)
{
    /* Reset the viewport after a target/mode change. Otherwise the first
     * rectangle can retain the preceding target's letterbox translation. */
    return SDL_SetRenderScale(r,1,text_mode ? 2 : 1) &&
        SDL_SetRenderLogicalPresentation(r,text_mode ? 640 : 320,text_mode ? 400 : 200,
        SDL_LOGICAL_PRESENTATION_LETTERBOX) && SDL_SetRenderViewport(r,NULL);
}
