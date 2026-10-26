#ifdef NDEBUG
#undef NDEBUG
#endif
#include "resources.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void strings_and_encoding(void)
{
    uint8_t raw[]={2,'A',0x82,0,0,0,0,0};char out[102][40],before[102][40],utf8[20];
    memset(out,0x55,sizeof out);
    assert(tera_resource_strings(raw,sizeof raw,2,4,out));
    assert(out[0][0]=='A' && (unsigned char)out[0][1]==0x82 && !out[1][0]);
    assert(tera_cp437_utf8(out[0],utf8,sizeof utf8));assert(!strcmp(utf8,"A\xc3\xa9"));
    strcpy(utf8,"unchanged");assert(!tera_cp437_utf8(out[0],utf8,3));assert(!strcmp(utf8,"unchanged"));
    memcpy(before,out,sizeof out);raw[0]=4;
    assert(!tera_resource_strings(raw,sizeof raw,2,4,out));assert(!memcmp(out,before,sizeof out));
    raw[0]=2;raw[2]=0;assert(!tera_resource_strings(raw,sizeof raw,2,4,out));
    assert(!tera_resource_strings(raw,sizeof raw-1,2,4,out));
    assert(tera_cp437_codepoint(0x82)==0xe9 && tera_cp437_codepoint(0x85)==0xe0);
    assert(tera_cp437_codepoint(0xb3)==0x2502 && tera_cp437_codepoint(0xff)==0xa0);
}
static void cga_planes(void)
{
    uint8_t bytes[16385]={0},pixels[64000];
    bytes[0]=0x1b;bytes[8192]=0xe4;bytes[80]=0xff;bytes[8000]=0xff;
    assert(tera_resource_cga(bytes,16384,pixels));
    assert(pixels[0]==0 && pixels[1]==1 && pixels[2]==2 && pixels[3]==3);
    assert(pixels[320]==3 && pixels[321]==2 && pixels[322]==1 && pixels[323]==0);
    assert(pixels[640]==3 && pixels[643]==3 && pixels[199*320]==0);
    memset(pixels,7,sizeof pixels);
    assert(!tera_resource_cga(bytes,16383,pixels) && pixels[0]==7);
    assert(!tera_resource_cga(bytes,16385,pixels));
}
static void atlas_bounds(void)
{
    uint8_t raw[10000];size_t length=0,plane;unsigned id,j,scale_x,scale_y,height;
    TeraSpriteAtlas *atlas=malloc(sizeof *atlas),*before=malloc(sizeof *before);
    assert(atlas && before);
    memset(atlas,0,sizeof *atlas);
    /* Fixture synthetique : paires de plans 0x1b/0xe4, largeur brute 1.
     * Les seuils d'echelle sont verifies, et les lignes impaires tronquees
     * du premier groupe ne deviennent pas des lignes noires inventees. */
    for(id=1;id<=82;++id) {
        scale_x=id<=50 ? 1 : id==59 ? 1 : id<=76 ? 2 : id==82 ? 2 : 3;
        scale_y=id<59 ? 1 : id<=74 ? 2 : id<=76 ? 3 : id==81 ? 3 : id==82 ? 6 : 1;
        height=id<59 ? 3 : 4;
        plane=scale_x*scale_y*(id<59 ? 1 : height/2);
        raw[length++]=1;raw[length++]=(uint8_t)height;
        for(j=0;j<plane;++j) {raw[length++]=0x1b;raw[length++]=0xe4;}
    }
    assert(length<sizeof raw);assert(tera_resource_atlas(raw,length,atlas));
    for(id=0;id<82;++id) {
        const TeraSprite *s=&atlas->sprites[id];const uint8_t *p=atlas->pixels+s->pixel_offset;
        assert(p[0]==0 && p[1]==1 && p[2]==2 && p[3]==3);
        assert(p[s->width]==3 && p[s->width+3]==0);
    }
    assert(atlas->sprites[2].height==2);
    assert(atlas->sprites[58].hscale==1 && atlas->sprites[58].vscale==2);
    assert(atlas->sprites[74].vscale==3 && atlas->sprites[76].hscale==3);
    assert(atlas->sprites[81].height==24 && atlas->sprites[81].width==8);
    memcpy(before,atlas,sizeof *before);
    assert(!tera_resource_atlas(raw,length-1,atlas));assert(!memcmp(before,atlas,sizeof *atlas));
    raw[length]=0;assert(!tera_resource_atlas(raw,length+1,atlas));
    raw[0]=0;assert(!tera_resource_atlas(raw,length,atlas));
    free(before);free(atlas);
}
static int originals(const char *directory)
{
    char error[512],path[4096],utf8[128];TeraResources *r;FILE *file;unsigned i,j;
    snprintf(path,sizeof path,"%s/CRAPIM.IMA",directory);file=fopen(path,"rb");
    if(!file) {puts("SKIP: original resources absent");return 77;}fclose(file);
    r=tera_resources_load(directory,error,sizeof error);
    if(!r) {fprintf(stderr,"%s\n",error);return 1;}
    assert(r->atlas.used==92168);
    assert(r->atlas.sprites[0].width==16 && r->atlas.sprites[0].height==32);
    assert(r->atlas.sprites[81].file_offset==20324 && r->atlas.sprites[81].height==144);
    for(i=0;i<TERA_RES_TEXT_COUNT;++i) for(j=0;j<102;++j)
        assert(tera_cp437_utf8(r->text[i][j],utf8,sizeof utf8));
    assert(tera_resource_text(r,TERA_RES_VERBS,56) && !tera_resource_text(r,TERA_RES_VERBS,57));
    assert(!tera_resource_text(r,TERA_RES_ITEMS,0));assert(!tera_resource_text(r,TERA_RES_MESSAGE2,34));
    {
        TeraPresentationEvent e={0};TeraWorldState world={0};
        e.kind=TERA_PRESENT_MESSAGE;e.data.message.resource=TERA_TEXT_RESPONSE;e.data.message.id=22;e.data.message.variant=2;
        assert(tera_resource_message(r,&e,&world)==tera_resource_text(r,TERA_RES_MESSAGE3,22));
        e.data.message.resource=TERA_TEXT_COMMAND_RESPONSE;e.data.message.id='L';e.data.message.variant=1;
        assert(tera_resource_message(r,&e,&world)==tera_resource_text(r,TERA_RES_MESSAGE2,8));
        e.data.message.variant=3;assert(!tera_resource_message(r,&e,&world));
        e.data.message.resource=TERA_TEXT_HINT;e.data.message.id=38;strcpy(world.hints[37],"TEST");
        assert(!strcmp(tera_resource_message(r,&e,&world),"TEST"));
        e.data.message.id=39;assert(!tera_resource_message(r,&e,&world));
        e.data.message.resource=TERA_TEXT_SETH;e.data.message.id=100;
        assert(strstr(tera_resource_message(r,&e,&world),"\ndans ces lieux"));
        e.data.message.id=101;assert(!strcmp(tera_resource_message(r,&e,&world),"AUTEL DES TENEBRES"));
        e.data.message.id=102;assert(!tera_resource_message(r,&e,&world));
        e.kind=TERA_PRESENT_COMBAT;e.data.combat.kind=TERA_EVENT_ACTION;e.data.combat.letter='C';e.data.combat.value=2;
        assert(tera_resource_message(r,&e,&world)==tera_resource_text(r,TERA_RES_MESSAGE3,33));
    }
    printf("Original assets OK: 7 string tables, 82 sprites (%lu pixels), 6 CGA pages\n",(unsigned long)r->atlas.used);
    free(r);return 0;
}
int main(int argc,char **argv)
{
    strings_and_encoding();cga_planes();atlas_bounds();
    if(argc>1) return originals(argv[1]);
    puts("resource decoder contracts OK");return 0;
}
