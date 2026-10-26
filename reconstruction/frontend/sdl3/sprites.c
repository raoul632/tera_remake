#include "sprites.h"
void tera_scenery_collect(TeraTextures *t,const TeraPresentationEvent *e)
{
    unsigned k;
    if(!t || !e || e->kind!=TERA_PRESENT_SCENERY || e->data.scenery.kind>=TERA_PICTURE_COUNT || e->data.scenery.count>256) return;
    k=e->data.scenery.kind;t->scenery[k].world=e->world;t->scenery[k].position=e->position;
    t->scenery[k].on_alfol=e->on_alfol;t->scenery[k].picture=e->data.scenery;
}
#include <stdlib.h>
#include <string.h>

void tera_cga_rgba(unsigned index,uint8_t color[4])
{
    static const uint8_t colors[4][4]={{0,0,0,255},{85,255,255,255},{255,85,255,255},{255,255,255,255}};
    memcpy(color,colors[index&3u],4);
}
static SDL_Texture *texture(SDL_Renderer *r,const uint8_t *indices,unsigned w,unsigned h,int skip_byte_zero)
{
    size_t i,count=(size_t)w*h;uint8_t *rgba=malloc(count*4);SDL_Texture *t;
    if(!rgba) return NULL;
    for(i=0;i<count;++i) tera_cga_rgba(indices[i],rgba+4*i);
    if(skip_byte_zero) for(i=0;i<count;i+=4) {
        unsigned j,nonzero=indices[i]|indices[i+1]|indices[i+2]|indices[i+3];
        for(j=0;j<4;++j) rgba[4*(i+j)+3]=nonzero ? 255 : 0;
    }
    t=SDL_CreateTexture(r,SDL_PIXELFORMAT_RGBA32,SDL_TEXTUREACCESS_STATIC,(int)w,(int)h);
    if(t && (!SDL_UpdateTexture(t,NULL,rgba,(int)w*4) || !SDL_SetTextureScaleMode(t,SDL_SCALEMODE_NEAREST) ||
        !SDL_SetTextureBlendMode(t,skip_byte_zero ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE))) {SDL_DestroyTexture(t);t=NULL;}
    free(rgba);return t;
}
bool tera_textures_create(SDL_Renderer *r,const TeraResources *resources,TeraTextures *out)
{
    unsigned i;
    if(!r || !resources || !out) return false;
    memset(out,0,sizeof *out);
    for(i=0;i<TERA_SPRITE_COUNT;++i) {
        const TeraSprite *s=&resources->atlas.sprites[i];
        unsigned x,y;uint8_t *extended=malloc(s->width*(s->height+2));
        if(!extended) {tera_textures_destroy(out);return false;}
        for(y=0;y<s->height+2;++y) for(x=0;x<s->width;++x) {
            size_t offset=s->plane_offset+(y/2)*(s->width/4)+x/4;
            unsigned b=offset<TERA_ATLAS_BYTES/2 ? resources->atlas.planes[y%2][offset] : 0;
            extended[y*s->width+x]=(uint8_t)((b>>(6-2*(x%4)))&3);
        }
        out->sprites[i]=texture(r,resources->atlas.pixels+s->pixel_offset,s->width,s->height,0);
        out->byte_sprites[i]=texture(r,extended,s->width,s->height+2,1);free(extended);
        if(!out->sprites[i] || !out->byte_sprites[i]) {tera_textures_destroy(out);return false;}
    }
    for(i=0;i<TERA_BACKGROUND_COUNT;++i) {
        out->backgrounds[i]=texture(r,resources->backgrounds[i],320,200,0);
        if(!out->backgrounds[i]) {tera_textures_destroy(out);return false;}
    }
    out->scene_layer=SDL_CreateTexture(r,SDL_PIXELFORMAT_RGBA32,SDL_TEXTUREACCESS_TARGET,320,200);
    if(!out->scene_layer || !SDL_SetTextureScaleMode(out->scene_layer,SDL_SCALEMODE_NEAREST) ||
       !SDL_SetTextureBlendMode(out->scene_layer,SDL_BLENDMODE_BLEND)) {tera_textures_destroy(out);return false;}
    return true;
}
void tera_textures_destroy(TeraTextures *t)
{
    unsigned i;if(!t) return;
    for(i=0;i<TERA_SPRITE_COUNT;++i) {SDL_DestroyTexture(t->sprites[i]);SDL_DestroyTexture(t->byte_sprites[i]);}
    SDL_DestroyTexture(t->scene_layer);
    for(i=0;i<TERA_BACKGROUND_COUNT;++i) SDL_DestroyTexture(t->backgrounds[i]);
    memset(t,0,sizeof *t);
}
bool tera_background_draw(SDL_Renderer *r,const TeraTextures *t,unsigned index)
{
    return r && t && index<TERA_BACKGROUND_COUNT && SDL_RenderTexture(r,t->backgrounds[index],NULL,NULL);
}
bool tera_sprite_draw(SDL_Renderer *r,const TeraTextures *t,const TeraResources *res,unsigned id,int x,int y)
{
    const TeraSprite *s;SDL_FRect src,dst;int height,row,col;
    if(!r || !t || !res || !id || id>TERA_SPRITE_COUNT) return false;
    /* C5051/C5E04 take BYTE coordinates, then write linear CGA addresses.
     * Horizontal overflow wraps to the next paired row, not a viewport clip. */
    s=&res->atlas.sprites[id-1];x&=255;y&=255;
    height=2*((y+(int)(s->raw_height*s->vscale)-2)/2-y/2+1);y-=y%2;
    if(y>=192) return true;
    if(y+height>192) height=192-y;
    if(x+(int)s->width/4>80) {
        for(row=0;row<height;++row) for(col=0;col<(int)s->width/4;++col) {
            int address=((y+row)%2)*8192+((y+row)/2)*80+x+col;
            int bank=address/8192,offset=address%8192,py=2*(offset/80)+bank;
            if(bank>1 || py>=200) continue;
            src.x=(float)(4*col);src.y=(float)row;src.w=4;src.h=1;
            dst.x=(float)(4*(offset%80));dst.y=(float)py;dst.w=4;dst.h=1;
            if(!SDL_RenderTexture(r,t->byte_sprites[id-1],&src,&dst)) return false;
        }
        return true;
    }
    src.x=src.y=0;src.w=(float)s->width;src.h=(float)height;
    dst.x=(float)(x*4);dst.y=(float)y;dst.w=src.w;dst.h=src.h;
    return SDL_RenderTexture(r,t->byte_sprites[id-1],&src,&dst);
}
