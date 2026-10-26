#include "panels.h"
#include "text.h"
#include <stdio.h>
#include <string.h>
#define TRY(call) do {if(!(call)) return false;} while(0)
static bool ink(SDL_Renderer *r,unsigned c)
{uint8_t rgba[4];tera_cga_rgba(c,rgba);return SDL_SetRenderDrawColor(r,rgba[0],rgba[1],rgba[2],255);}
static bool line(SDL_Renderer *r,int x,int y,int xx,int yy,unsigned c)
{return ink(r,c) && SDL_RenderLine(r,(float)x,(float)y,(float)xx,(float)yy);}
static bool command(SDL_Renderer *r,unsigned col,unsigned row,const char *key,const char *rest)
{return ink(r,2) && tera_text_draw(r,(int)(col-1)*8,(int)(row-1)*8,key,40,1) &&
 ink(r,3) && tera_text_draw(r,(int)(col-1+strlen(key))*8,(int)(row-1)*8,rest,40-col,1);}
bool tera_help_screen(SDL_Renderer *r)
{
    static const struct {unsigned col,row;const char *key,*rest;} items[]={
        {1,1,"A","bandonner"},{1,2,"B","oire"},{6,3,"C","ombattre-> "},
        {18,3,"A","ttaque mentale"},{18,4,"C","rier"},{18,5,"D","issiper"},
        {18,6,"J","eter un sort"},{1,7,"D","escendre"},{18,7,"L","utter"},
        {1,8,"E","ntrer"},{18,8,"P","salmodier"},{1,9,"F","uir"},{18,9,"T","irer"},
        {6,10,"H","alte    -> "},{18,10,"A","cc\202l\202rer les d\202pl-s"},
        {18,11,"C","onsulter la carte"},{18,12,"D","ormir"},{18,13,"H","ELP.."},
        {1,14,"I","nvoquer"},{18,14,"I","n(tro)spection"},{1,15,"L","ire"},
        {18,15,"M","anger"},{1,16,"M","onter"},{18,16,"S","onder"},
        {6,17,"N","\202gocier -> "},{18,17,"A","cheter"},{18,18,"D","onner"},
        {18,19,"I","nterroger"},{1,20,"O","uvrir"},{18,20,"S","olliciter"},
        {1,21,"P","rendre"},{18,21,"V","endre"},{1,22,"Q","uitter et sauver (ou "},
        {22,22,"H","(alte) "},{30,22,"[ESC]",")"},{1,23,"S","\202duire"},
        {1,24,"U","tiliser"},{1,25,"#"," (couper ou remettre le son)"}};
    static const int segments[][4]={{37,71,295,71},{132,71,132,25},{37,25,132,25},
        {37,25,37,15},{37,15,295,15},{37,127,295,127},{132,127,132,81},
        {37,81,132,81},{37,81,37,71},{132,167,295,167},{132,167,132,137},
        {37,137,132,137},{37,127,37,137},{295,15,295,167}};
    unsigned i;
    TRY(tera_video_mode(r,0));
    TRY(ink(r,0));TRY(SDL_RenderClear(r));
    for(i=0;i<sizeof items/sizeof items[0];++i)
        TRY(command(r,items[i].col,items[i].row,items[i].key,items[i].rest));
    for(i=0;i<sizeof segments/sizeof segments[0];++i)
        TRY(line(r,segments[i][0],segments[i][1],segments[i][2],segments[i][3],2));
    return true;
}
static void map_box(uint8_t *p,int x,int y,int xx,int yy,unsigned color)
{
    tera_cga_line(p,x,y,xx,y,color);tera_cga_line(p,x,y,x,yy,color);
    tera_cga_line(p,xx,y,xx,yy,color);tera_cga_line(p,x,yy,xx,yy,color);
}
static void map_text(uint8_t *p,int x,int y,unsigned char code)
{
    unsigned row,col;const unsigned char *glyph=tera_text_glyph(code,0);
    for(row=0;row<8;++row) for(col=0;col<8;++col) if(glyph[row]&(128u>>col))
        p[(y+(int)row)*320+x+(int)col]=3;
}
void tera_map_raster(const TeraGameView *v,Uint64 ticks,uint8_t *pixels)
{
    const TeraWorldState *w=&v->exploration->world;const TeraCombatGame *g=&v->characters->combat;
    static const unsigned colors[]={3,0,1,2};unsigned i,j,k;int x,y,p,found;
    if(g->world_id!=11) {
        tera_cga_bytes(pixels,20,40,54,140,0);tera_cga_line(pixels,100,40,100,140,3);
        for(i=0;i<10;++i) for(j=0;j<10;++j) {
            const uint8_t *cell=w->maps[g->world_id*100+10*i+j];x=100+10*(int)i;y=130-10*(int)j;
            tera_cga_line(pixels,x,y,x+10,y,colors[cell[0]&3]);
            tera_cga_line(pixels,x+10,y+10,x+10,y,colors[cell[3]&3]);
            tera_cga_line(pixels,x,y+10,x+10,y+10,colors[cell[2]&3]);
            if(!cell[6]) tera_cga_fill(pixels,x,y,x+10,y+10,3);
        }
        if(!g->world_id) {tera_cga_line(pixels,100,140,100,130,0);tera_cga_line(pixels,100,140,110,140,0);}
        if(g->difficulty<5) {
            unsigned c=(ticks/500%2) ? 3 : 0;int h=g->position-g->previous_position;
            p=g->position-(int)g->world_id*100;x=10*(p/10);y=10*(p%10);
            if(h==1 || h==-1 || h==10 || h==-10)
                tera_cga_plot(pixels,x+(h==10 ? 102 : h==-10 ? 108 : 105),
                    (h==1 ? 138 : h==-1 ? 132 : 135)-y,c);
        }
        return;
    }
    tera_cga_bytes(pixels,66,4,79,g->on_alfol ? 60 : 136,0);
    map_box(pixels,268,8,316,g->on_alfol ? 56 : 132,3);
    for(i=0;i<10;++i) for(j=0;j<(g->on_alfol ? 10u : 30u);++j) {
        x=272+4*(int)i;y=128-4*(int)j;
        if(g->on_alfol) {
            tera_cga_plot(pixels,x,12+4*(int)j,1);
            if(w->alfol_position==(int)(j+10*i)) tera_cga_bytes(pixels,68+(int)i,48-4*(int)j,69+(int)i,52-4*(int)j,2);
        } else {
            p=g->position-135+(int)j+30*(int)i;if(p<1000) p+=900;if(p>1899) p-=900;
            found=0;for(k=0;k<20;++k) if(w->towns[k]==p) found=1;
            if(found) tera_cga_fill(pixels,x,y,x+4,y+4,2);
            else tera_cga_plot(pixels,x+2,y+2,3);
            for(k=0;k<9;++k) if(w->horizontal_mountains[k]==p) tera_cga_line(pixels,x,y,x+4,y,2);
            for(k=0;k<25;++k) if(w->vertical_mountains[k]==p) tera_cga_line(pixels,x+4,y,x+4,y+4,2);
        }
    }
    if(g->on_alfol) tera_cga_bytes(pixels,68,48,69,52,2);
    else tera_cga_circle(pixels,290,70,4,3);
    map_text(pixels,264,16,'O');map_text(pixels,272,8,'N');
    map_text(pixels,280,16,'E');map_text(pixels,272,24,'S');
}
bool tera_map_overlay(SDL_Renderer *r,const TeraGameView *v,Uint64 ticks)
{
    uint8_t pixels[TERA_CGA_PIXELS];SDL_Surface *layer;SDL_Texture *texture;unsigned i;bool ok;
    memset(pixels,255,sizeof pixels);tera_map_raster(v,ticks,pixels);
    layer=SDL_CreateSurface(320,200,SDL_PIXELFORMAT_RGBA32);if(!layer) return false;
    for(i=0;i<TERA_CGA_PIXELS;++i) {
        uint8_t *dst=(uint8_t *)layer->pixels+(i/320)*layer->pitch+(i%320)*4;
        tera_cga_rgba(pixels[i]&3,dst);dst[3]=pixels[i]==255 ? 0 : 255;
    }
    texture=SDL_CreateTextureFromSurface(r,layer);SDL_DestroySurface(layer);if(!texture) return false;
    ok=SDL_SetTextureBlendMode(texture,SDL_BLENDMODE_BLEND) &&
        SDL_SetTextureScaleMode(texture,SDL_SCALEMODE_NEAREST) && SDL_RenderTexture(r,texture,NULL,NULL);
    SDL_DestroyTexture(texture);return ok;
}
bool tera_end_screen(SDL_Renderer *r,const TeraGameView *v)
{
    char body[1500],score[64];const char *title;
    int victory=v->ending && v->ending->victory;
    TRY(tera_video_mode(r,1));
    TRY(SDL_SetRenderDrawColor(r,0,0,170,255));TRY(SDL_RenderClear(r));
    TRY(SDL_SetRenderDrawColor(r,255,255,85,255));TRY(tera_text_box(r,144,16,43,19));
    title=victory ? "f\202licitations..." : "condol\202ances...";
    {SDL_FRect title_back={264,16,(float)strlen(title)*8,8};
     TRY(SDL_SetRenderDrawColor(r,0,0,170,255));TRY(SDL_RenderFillRect(r,&title_back));
     TRY(SDL_SetRenderDrawColor(r,255,255,85,255));}
    TRY(tera_text_draw(r,264,16,title,23,1));
    if(victory) {
        snprintf(score,sizeof score,"%5s",v->ending->score_text);
        snprintf(body,sizeof body,"     ARIOCH et %.9s sont retourn\202s\nau chaos..\n"
            "      Pour combien de temps ? l'avenir le dira.. AMARANDE vous doit"
            " beaucoup,     o aventuriers courageux et int\212gres. Les TROIS FORCES sont"
            " unies et l'HARMONIE    r\212gne de nouveau sur ce monde.. Graces "
            "  vous soient rendues..\n\n     SCORE -> %s",v->exploration->world.secret_words[3],score);
    } else snprintf(body,sizeof body,"     Cette aventure se termine sur cet\n"
        "\202chec. Dommage, cet aventurier(e) \202tait\n"
        "sympathique... Dans leur bienveillance\n"
        "insigne, les anciens dieux ont fait\n"
        "porter la d\202pouille de %.9s\n"
        "dans un sanctuaire, et son butin sera\n"
        "offert \205 qui osera lui succ\202der dans\n"
        "cette redoutable qu\210te...",v->characters->names[0]);
    TRY(SDL_SetRenderDrawColor(r,255,255,255,255));
    return tera_text_draw(r,152,64,body,41,12);
}
