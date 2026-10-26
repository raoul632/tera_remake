#include "battle.h"
#include "characters.h"
#include "text.h"
#include "graph.h"
#include <stdio.h>
#include <string.h>

#define TRY(x) do {if(!(x)) return false;} while(0)
static bool ink(SDL_Renderer *r,unsigned color)
{uint8_t c[4];tera_cga_rgba(color,c);return SDL_SetRenderDrawColor(r,c[0],c[1],c[2],255);}
static bool sprite(SDL_Renderer *r,const TeraResources *res,const TeraTextures *t,unsigned id,int x,int y)
{return tera_sprite_draw(r,t,res,id,x,y);}
/* C4544 : x2 exclusif, derniere paire de lignes incluse. */
static bool clear_bytes(SDL_Renderer *r,int x1,int y1,int x2,int y2)
{
    return ink(r,0) && tera_graph_bytes(r,x1,y1,x2,y2);
}
/* Tables BIN C73ED/C76D2. DRAW utilise des pixels, les sprites des octets.
 * Aucun point ne depend du hasard ni de l'etat du jeu. */
static bool segment(SDL_Renderer *r,int x,int y,int x1,int y1,int x2,int y2)
{return tera_graph_line(r,x+x1,y+y1,x+x2,y+y2);}
static bool humanoid(SDL_Renderer *r,int x,int y,unsigned color)
{
    static const unsigned char xs[]={100,96,98,106,100,105,111,116,118,123,119,125,134,138,135,126,111,106,105,107,110,122,121,111};
    static const unsigned char ys[]={85,90,90,88,83,65,25,30,88,86,30,20,82,86,87,85,1,5,10,15,25,17,14,17};
    static const int detail[][4]={{114,16,110,11},{127,4,126,18},{127,4,120,-1},{115,6,121,1},
        {115,6,111,7},{111,7,107,5},{122,13,125,10},{120,20,123,21}};
    unsigned i;
    TRY(ink(r,color));
    for(i=1;i<24;++i) if(i!=9 && i!=16) TRY(segment(r,x,y,xs[i-1],ys[i-1],xs[i],ys[i]));
    TRY(segment(r,x,y,134,83,113,89));
    for(i=0;i<8;++i) TRY(segment(r,x,y,detail[i][0],detail[i][1],detail[i][2],detail[i][3]));
    return true;
}
static bool large_figure(SDL_Renderer *r)
{
    static const unsigned char xs[]={180,155,153,155,168,172,174,177,178,173,170,170,169,170,171,179,181,179,179,189,191,200,200,202,212,214,210,211,210,208,203,201,203,208,220,220,210,195,180};
    static const unsigned char ys[]={100,113,116,119,118,116,116,120,132,145,157,180,185,190,191,190,189,184,161,150,150,160,188,190,190,188,185,168,155,145,135,125,117,114,113,109,104,100,100};
    unsigned i;TRY(ink(r,1));
    for(i=1;i<39;++i) TRY(segment(r,32,0,xs[i-1],ys[i-1],xs[i],ys[i]));
    TRY(segment(r,32,0,180,130,187,145));
    return segment(r,32,0,198,130,191,145);
}
static bool hooded(SDL_Renderer *r,const TeraResources *res,const TeraTextures *t,int x)
{
    static const int lines[][4]={{111,93,117,92},{117,92,120,93},{111,93,109,95},{120,93,122,95},
        {109,95,108,105},{122,95,124,104},{124,104,125,108},{108,105,106,110}};
    unsigned i;TRY(clear_bytes(r,26+x/4,92,33+x/4,192));TRY(humanoid(r,x,106,1));
    for(i=0;i<8;++i) TRY(segment(r,x,0,lines[i][0],lines[i][1],lines[i][2],lines[i][3]));
    return sprite(r,res,t,0x19,28+x/4,96);
}
bool tera_scene_portrait(SDL_Renderer *r,const TeraResources *res,const TeraTextures *t,
    int x,unsigned color,unsigned variant)
{
    if(variant==2) return hooded(r,res,t,x);
    TRY(clear_bytes(r,26+x/4,104,33+x/4,192));
    TRY(humanoid(r,x+(variant ? 2 : 1),104,color));
    return sprite(r,res,t,variant ? 0x17 : 0x16,28+x/4,variant ? 90 : 88);
}
/* C9B4E : coordonnees en octets et indices conserves. Les elements
 * aleatoires proviennent uniquement des points captures dans le core. */
bool tera_battle_monster(SDL_Renderer *r,const TeraResources *res,const TeraTextures *t,unsigned type,unsigned world)
{
    const TeraMonsterPicture *p=&t->monster;unsigned i;int x,y;
    if(p->type!=type) return true; /* C838A direct ne rappelle pas C9B4E. */
#define S(id,x,y) TRY(sprite(r,res,t,id,x,y))
    switch(type) {
    case 1:if(p->type==type && p->count) {S(0x12,20+p->values[0],148);}break;
    case 2:if(p->type==type && p->count) {S(0x41,48+p->values[0],140);}break;
    case 3:case 6:case 9:
        if(p->type==type) for(i=0;i+1<p->count;i+=2) {S(type==6 ? 0x32 : 2,20+p->values[i],(type==6 ? 170 : 164)+2*p->values[i+1]);}
        if(type==9) {S(0x34,57,170);}break;
    case 4:S(7,33,156);break;
    case 5:S(0x33,30,156);break;
    case 7:S(0x34,57,160);break;
    case 8:S(0x0e,20,124);S(0x3f,57,106);break;
    case 10:S(0x0e,50,122);S(0x3f,57,106);S(4,18,110);break;
    case 11:S(0x0e,41,122);S(0x0e,45,124);S(0x0e,52,120);S(0x3f,18,106);break;
    case 12:S(0x10,world==11 ? 60 : 56,world==11 ? 132 : 112);break;
    case 13:
        if(world==11) TRY(clear_bytes(r,52,134,54,140));
        S(0x0a,world==11 ? 51 : 48,world==11 ? 132 : 112);break;
    case 14:S(0x10,world==11 ? 55 : 60,world==11 ? 132 : 112);
        if(world==11) TRY(clear_bytes(r,49,134,51,140));
        S(0x0a,world==11 ? 48 : 50,world==11 ? 132 : 112);
        if(world==11) TRY(clear_bytes(r,32,120,36,136));
        S(0x3d,world==11 ? 30 : 29,world==11 ? 116 : 100);break;
    case 15:if(p->type==type && p->count>=3) {i=p->count-3;S(3,10+p->values[i],26+2*p->values[i+1]);}break;
    case 16:case 19:
        TRY(clear_bytes(r,52,102,61,180));TRY(clear_bytes(r,47,108,66,118));
        TRY(large_figure(r));
        S(0x13,53,96);S(0x0f,46,114);S(0x0f,62,108);
        if(type==16) {TRY(clear_bytes(r,19,96,25,180));S(0x1a,21,100);}break;
    case 17:TRY(hooded(r,res,t,0));TRY(hooded(r,res,t,84));S(0x1a,49,96);break;
    case 18:S(0x11,20,130);break;
    case 20:S(0x40,14,110);break;
    case 21:
        TRY(clear_bytes(r,52,40,63,180));
        if(p->type==type) {
            TRY(ink(r,1));
            for(i=0;i+1<p->count;i+=2) {x=210+p->values[i];y=70+p->values[i+1];TRY(SDL_RenderPoint(r,(float)x,(float)y));}
        }
        S(0x0b,53,45);S(0x0f,50,76);S(0x0f,57,80);break;
    /* Aucun CASE graphique 22 dans C9B4E : conserver le decor. */
    default:break;
    }
#undef S
    return true;
}
static unsigned verb(char letter)
{
    switch(letter) {
    case 'T':return 26;case 'L':return 27;case 'J':return 28;case 'D':return 29;
    case 'A':return 31;case 'P':return 32;case 'C':return 33;case 'F':return 6;
    default:return 0;
    }
}
static void result(const TeraBattleFrame *f,const TeraGameView *v,const TeraResources *res,char out[128])
{
    const char *name=f->actor<10 ? v->characters->names[f->actor] : "";
    const char *s;TeraPresentationEvent e;
    out[0]=0;
    switch(f->kind) {
    case TERA_EVENT_INTRO:
        if(f->value<0) snprintf(out,128,"agression de %s !",name);
        else {
            static const char *const intro[]={"danger ! "," aargh ! ","aie.. "};
            s=tera_resource_text(res,TERA_RES_MONSTERS,f->type);
            snprintf(out,128,"%s%s",intro[f->value%3],s ? s : "");
        }break;
    case TERA_EVENT_ACTION:
        memset(&e,0,sizeof e);e.kind=TERA_PRESENT_COMBAT;e.data.combat=*f;
        s=tera_resource_message(res,&e,&v->exploration->world);
        snprintf(out,128,"%s",s ? s : "");break;
    case TERA_EVENT_HERO_HIT:
        s=tera_resource_text(res,TERA_RES_VERBS,!f->eligible ? 38 : f->value>0 ? 36 : 37);
        if(f->eligible && f->value>0 && f->item) {
            const char *item=tera_resource_text(res,TERA_RES_ITEMS,f->item);
            snprintf(out,128,"%s (%s)",s,item ? item : "");
        } else snprintf(out,128,"%s",s);
        break;
    case TERA_EVENT_MONSTER_HIT:snprintf(out,128,"%s%s",name,tera_resource_text(res,TERA_RES_VERBS,f->value>0 ? 36 : 37));break;
    case TERA_EVENT_EQUIPMENT:
        s=tera_resource_text(res,TERA_RES_ITEMS,(unsigned)f->value);snprintf(out,128," (%s)",s ? s : "");break;
    case TERA_EVENT_ESCAPE:snprintf(out,128,"%s",tera_resource_text(res,TERA_RES_MESSAGE,91));break;
    case TERA_EVENT_END:if(f->value) snprintf(out,128,"%s%d%s",tera_resource_text(res,TERA_RES_MESSAGE,40),
        f->reward,tera_resource_text(res,TERA_RES_VERBS,34));break;
    case TERA_EVENT_PASS:case TERA_EVENT_CANCEL:if(f->value) snprintf(out,128," passe..");break;
    default:break;
    }
}
bool tera_battle_screen(SDL_Renderer *r,const TeraGameView *view,const TeraResources *res,
    const TeraTextures *t,const TeraPresentationEvent *events,unsigned count,const TeraBattleFrame *f)
{
    TeraGameView v=*view;TeraCharacterState characters=*v.characters;
    unsigned type,i;char b[128];SDL_FRect bar={0,192,320,8};
    if(f) {
        for(i=0;i<10;++i) {
            TeraRoundActor *a=&characters.combat.encounter.actors[i];const TeraBattleVitals *s=&f->actors[i];
            a->active=(uint8_t)s->active;a->hp=(int16_t)s->hp;a->magic=(int16_t)s->magic;
            a->hp_limit=(int16_t)s->hp_limit;a->magic_limit=(int16_t)s->magic_limit;memcpy(a->condition,s->condition,3);
        }
        v.combat=NULL;v.characters=&characters;type=f->type;
    } else type=v.combat->game.encounter.monster_type;
    TRY(tera_scene_draw(r,&v,res,t));TRY(tera_battle_monster(r,res,t,type,v.scene.world));
    {SDL_FRect party={0,0,320,32};TRY(ink(r,0));TRY(SDL_RenderFillRect(r,&party));}
    TRY(ink(r,1));TRY(tera_party_draw(r,&v));TRY(ink(r,0));TRY(SDL_RenderFillRect(r,&bar));TRY(ink(r,3));
    if(f) {result(f,&v,res,b);return tera_text_draw(r,0,192,b,40,1) && (v.scene.palette==3 || tera_cga_present_palette(r,v.scene.palette));}
    if(v.prompt.kind==TERA_PROMPT_CONFIRM) snprintf(b,sizeof b,"%s ? ",tera_resource_text(res,TERA_RES_VERBS,verb((char)v.prompt.command)));
    else if(v.prompt.kind==TERA_PROMPT_WORD) snprintf(b,sizeof b,"%s %.10s_",tera_resource_text(res,TERA_RES_VERBS,33),v.prompt.input);
    else snprintf(b,sizeof b,"%s -> ",v.characters->names[v.prompt.actor]);
    (void)events;(void)count;
    return tera_text_draw(r,0,192,b,40,1) && (v.scene.palette==3 || tera_cga_present_palette(r,v.scene.palette));
}

bool tera_cga_present_palette(SDL_Renderer *r,unsigned palette)
{
    SDL_Surface *raw,*pixels;SDL_Texture *t;bool ok;int x,y;SDL_FRect area;SDL_Rect crop;
    static const Uint8 colors[4][3][3]={
        {{0,170,0},{170,0,0},{170,85,0}},{{0,170,170},{170,0,170},{170,170,170}},
        {{85,255,85},{255,85,85},{255,255,85}},{{85,255,255},{255,85,255},{255,255,255}}};
    if(!SDL_GetRenderLogicalPresentationRect(r,&area)) return false;
    crop.x=(int)area.x;crop.y=(int)area.y;crop.w=(int)area.w;crop.h=(int)area.h;
    raw=SDL_RenderReadPixels(r,&crop);if(!raw) return false;
    pixels=SDL_ConvertSurface(raw,SDL_PIXELFORMAT_RGBA32);SDL_DestroySurface(raw);if(!pixels) return false;
    for(y=0;y<pixels->h;++y) for(x=0;x<pixels->w;++x) {
        Uint8 *p=(Uint8 *)pixels->pixels+y*pixels->pitch+x*4;int index=-1;
        unsigned source,k;
        for(source=0;source<4 && index<0;++source) for(k=0;k<3;++k)
            if(!memcmp(p,colors[source][k],3)) {index=(int)k;break;}
        if(index>=0) memcpy(p,colors[palette&3][index],3);
    }
    t=SDL_CreateTextureFromSurface(r,pixels);SDL_DestroySurface(pixels);if(!t) return false;
    ok=SDL_SetTextureScaleMode(t,SDL_SCALEMODE_NEAREST) && SDL_RenderTexture(r,t,NULL,NULL);
    SDL_DestroyTexture(t);
    return ok;
}
bool tera_battle_flash(SDL_Renderer *r,unsigned palette,unsigned border)
{
    bool ok=tera_cga_present_palette(r,palette);
    /* La bordure physique DOS est abstraite par le bord logique. */
    {SDL_FRect edge={0,0,319,199};unsigned bright=(border&8) ? 85 : 0;
     ok=ok && SDL_SetRenderDrawColor(r,(Uint8)((border&4 ? 170 : 0)+bright),
         (Uint8)((border&2 ? 170 : 0)+bright),(Uint8)((border&1 ? 170 : 0)+bright),255) && SDL_RenderRect(r,&edge);}
    return ok;
}
