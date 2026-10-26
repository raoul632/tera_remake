#include "resources.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const struct {const char *name;unsigned count,stride;} tables[]={
    {"VERBES",56,19},{"OBJETS",25,16},{"NOMON",22,40},{"NOMPERS",27,10},
    {"MESSAGE",102,40},{"MESSAGE2",33,40},{"MESSAGE3",33,40}
};
static const char *const backgrounds[]={"ASTRON.IMA","BANNIERE.IMA","FACADE.IMA",
    "KRONO.IMA","MAISONJ.IMA","MAISONS.IMA"};

int tera_resource_strings(const uint8_t *b,size_t size,unsigned count,unsigned stride,char out[102][40])
{
    unsigned i;
    if(!b || !out || !count || count>102 || stride<2 || stride>40 || size!=(size_t)count*stride) return 0;
    for(i=0;i<count;++i) if(b[i*stride]>=stride || memchr(b+i*stride+1,0,b[i*stride])) return 0;
    memset(out,0,102*40);
    for(i=0;i<count;++i) memcpy(out[i],b+i*stride+1,b[i*stride]);
    return 1;
}
int tera_resource_cga(const uint8_t *b,size_t size,uint8_t out[320*200])
{
    unsigned x,y;
    if(!b || !out || size!=16384) return 0;
    for(y=0;y<200;++y) for(x=0;x<320;++x) {
        size_t offset=(y%2)*8192+(y/2)*80+x/4;
        out[y*320+x]=(uint8_t)((b[offset]>>(6-2*(x%4)))&3);
    }
    return 1;
}
int tera_resource_atlas(const uint8_t *b,size_t size,TeraSpriteAtlas *out)
{
    TeraSprite meta[TERA_SPRITE_COUNT];size_t offset=0,pixels=0;unsigned id;
    if(!b || !out) return 0;
    memset(meta,0,sizeof meta);
    for(id=1;id<=TERA_SPRITE_COUNT;++id) {
        TeraSprite *s=&meta[id-1];size_t plane,row;
        if(offset>size || size-offset<2) return 0;
        s->file_offset=offset;s->raw_width=b[offset++];s->raw_height=b[offset++];
        s->hscale=1;s->vscale=1;
        /* OVL_00DB : IF successifs, pas ELSE IF. Les variantes agrandies
         * sont DEJA stockees dans le fichier ; ne pas les agrandir deux fois. */
        if(id>0x32) {
            s->hscale=2;
            if(id==0x3b) {s->vscale=2;s->hscale=1;}
            if(id>0x3b) {s->hscale=2;s->vscale=id>0x4a ? 3 : 2;}
            if(id>0x4c) {s->hscale=3;s->vscale=1;}
            if(id==0x51) s->vscale=3;
            if(id==0x52) {s->vscale=6;s->hscale=2;}
        }
        if(!s->raw_width || !s->raw_height) return 0;
        row=s->raw_width*s->hscale;
        plane=(size_t)s->raw_width*s->raw_height*s->hscale*s->vscale/2;
        if(s->raw_height%2 && id<0x3b) plane=row*s->vscale*((s->raw_height-1)/2);
        if(!plane || plane%row || plane>(size-offset)/2 || plane> (TERA_ATLAS_PIXELS-pixels)/8) return 0;
        s->width=(unsigned)row*4;s->height=(unsigned)(2*plane/row);
        s->pixel_offset=pixels;pixels+=plane*8;offset+=plane*2;
    }
    if(offset!=size) return 0;
    memcpy(out->sprites,meta,sizeof meta);out->used=pixels;
    memset(out->planes,0,sizeof out->planes);
    for(id=0;id<TERA_SPRITE_COUNT;++id) {
        const TeraSprite *s=&meta[id];unsigned x,y;
        size_t plane=s->width*s->height/8,j;
        out->sprites[id].plane_offset=s->pixel_offset/8;
        for(j=0;j<plane;++j) {
            out->planes[0][s->pixel_offset/8+j]=b[s->file_offset+2+2*j];
            out->planes[1][s->pixel_offset/8+j]=b[s->file_offset+3+2*j];
        }
        for(y=0;y<s->height;++y) for(x=0;x<s->width;++x) {
            size_t pos=s->file_offset+2+2*((y/2)*(s->width/4)+x/4)+(y%2);
            out->pixels[s->pixel_offset+y*s->width+x]=(uint8_t)((b[pos]>>(6-2*(x%4)))&3);
        }
    }
    return 1;
}
static int read_exact(const char *dir,const char *name,uint8_t *bytes,size_t size,char *error,size_t capacity)
{
    char path[4096];FILE *file;int n,ok,closed;
    n=snprintf(path,sizeof path,"%s/%s",dir,name);
    if(n<0 || (size_t)n>=sizeof path) {snprintf(error,capacity,"Resource path too long: %s",name);return 0;}
    file=fopen(path,"rb");
    if(!file) {snprintf(error,capacity,"Cannot open resource %s",path);return 0;}
    ok=fread(bytes,1,size,file)==size && fgetc(file)==EOF && !ferror(file);closed=fclose(file)==0;
    if(!ok || !closed) {snprintf(error,capacity,"Invalid resource size/read: %s (expected %lu)",name,(unsigned long)size);return 0;}
    return 1;
}
TeraResources *tera_resources_load(const char *dir,char *error,size_t capacity)
{
    TeraResources *r;uint8_t bytes[TERA_ATLAS_BYTES];unsigned i;
    if(!dir || !error || !capacity) return NULL;
    r=calloc(1,sizeof *r);
    if(!r) {snprintf(error,capacity,"Cannot allocate original resources");return NULL;}
    for(i=0;i<TERA_RES_TEXT_COUNT;++i) {
        if(!read_exact(dir,tables[i].name,bytes,tables[i].count*tables[i].stride,error,capacity)) goto fail;
        if(!tera_resource_strings(bytes,tables[i].count*tables[i].stride,tables[i].count,tables[i].stride,r->text[i])) {
            snprintf(error,capacity,"Invalid Pascal strings: %s",tables[i].name);goto fail;
        }
    }
    if(!read_exact(dir,"CRAPIM.IMA",bytes,TERA_ATLAS_BYTES,error,capacity)) goto fail;
    if(!tera_resource_atlas(bytes,TERA_ATLAS_BYTES,&r->atlas)) {snprintf(error,capacity,"Invalid sprite atlas: CRAPIM.IMA");goto fail;}
    for(i=0;i<TERA_BACKGROUND_COUNT;++i) {
        if(!read_exact(dir,backgrounds[i],bytes,16384,error,capacity)) goto fail;
        if(!tera_resource_cga(bytes,16384,r->backgrounds[i])) goto fail;
    }
    error[0]=0;return r;
    fail:free(r);return NULL;
}
const char *tera_resource_text(const TeraResources *r,TeraTextTable table,unsigned id)
{
    if(!r || (unsigned)table>=TERA_RES_TEXT_COUNT || !id || id>tables[table].count) return NULL;
    return r->text[table][id-1];
}
const char *tera_resource_background_name(unsigned index)
{
    return index<TERA_BACKGROUND_COUNT ? backgrounds[index] : NULL;
}
const char *tera_resource_message(const TeraResources *r,const TeraPresentationEvent *e,const TeraWorldState *w)
{
    unsigned id,variant;
    if(!r || !e) return NULL;
    if(e->kind==TERA_PRESENT_INVENTORY) return tera_resource_text(r,TERA_RES_ITEMS,e->data.inventory.item);
    if(e->kind==TERA_PRESENT_SHOP) return e->data.shop.outcome==3 ? NULL : tera_resource_text(r,TERA_RES_ITEMS,e->data.shop.item);
    if(e->kind==TERA_PRESENT_COMBAT) {
        if(e->data.combat.kind==TERA_EVENT_EQUIPMENT) return tera_resource_text(r,TERA_RES_ITEMS,(unsigned)e->data.combat.value);
        if(e->data.combat.kind!=TERA_EVENT_ACTION || e->data.combat.value<0 || e->data.combat.value>2) return NULL;
        switch(e->data.combat.letter) {
        case 'T':id=26;break;case 'L':id=27;break;case 'J':id=28;break;case 'D':id=29;break;
        case 'A':id=31;break;case 'P':id=32;break;case 'C':id=33;break;default:return NULL;
        }
        return tera_resource_text(r,(TeraTextTable)(TERA_RES_MESSAGE+e->data.combat.value),id);
    }
    if(e->kind!=TERA_PRESENT_MESSAGE) return NULL;
    id=e->data.message.id;variant=e->data.message.variant;
    if(e->data.message.resource==TERA_TEXT_ELLIPSIS) return "...";
    if(e->data.message.resource==TERA_TEXT_HINT) return w && id && id<=38 ? w->hints[id-1] : NULL;
    if(e->data.message.resource==TERA_TEXT_SETH) return id==100 ?
        "O \202tranger gardes-toi de p\202n\202trer\ndans ces lieux sacr\202s et terribles.." :
        id==101 ? "AUTEL DES TENEBRES" : NULL;
    if(e->data.message.resource==TERA_TEXT_COMMAND_RESPONSE) {
        /* CAFE8 : verbe et C4473 utilisent les memes indices. */
        switch(id) {
        case 'A':id=1;break;case 'B':id=2;break;case 'C':id=3;break;
        case 'D':id=4;break;case 'E':id=5;break;case 'F':id=6;break;
        case 'H':id=7;break;case 'L':id=8;break;case 'M':id=9;break;
        case 'O':id=11;break;case 'P':id=12;break;case 'S':id=14;break;
        default:return NULL;
        }
    } else if(e->data.message.resource!=TERA_TEXT_RESPONSE && e->data.message.resource!=TERA_TEXT_MESSAGE) return NULL;
    if(variant==TERA_TEXT_NO_VARIANT) variant=0;
    if(variant>2) return NULL;
    return tera_resource_text(r,(TeraTextTable)(TERA_RES_MESSAGE+variant),id);
}
