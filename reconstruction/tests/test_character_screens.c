#ifdef NDEBUG
#undef NDEBUG
#endif
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "characters.h"
#include "renderer.h"
#include "assets.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static TeraTextures textures;
static TeraCharacterCard held;
static unsigned cards;
static void collect(void *ctx,const TeraPresentationEvent *e)
{
    (void)ctx;
    if(e->kind==TERA_PRESENT_CHARACTER) {held=e->data.character;++cards;}
}
static void capture(SDL_Renderer *r,TeraGameSession *s,const TeraResources *res,
    const TeraCharacterCard *c,const char *dir,const char *name)
{
    TeraGameSession before=*s;SDL_Surface *surface;char path[4096];
    assert(tera_sdl_render(r,s,res,&textures,"","",NULL,0,c));
    assert(!memcmp(&before,s,sizeof before));
    surface=SDL_RenderReadPixels(r,NULL);assert(surface);
    if(!strcmp(name,"creation-name")) {
        /* Capture DOSBox CGA reelle tera_001.png, SHA256
         * 1d9f7867721dfc5d256b8e3249ef991df305f3608562579ac6411a4b627012c3.
         * Bande 632x16 RGB : titre et bordure, hors curseur clignotant.
         * PAS OVL0022 INNER_BA2F : D5/CD/B8 en haut, C0/C4/D9 en bas. */
        SDL_Surface *rgb=SDL_ConvertSurface(surface,SDL_PIXELFORMAT_RGB24);
        uint32_t hash=UINT32_C(2166136261);unsigned x,y;
        assert(rgb && rgb->w==640 && rgb->h==400);
        for(y=0;y<16;++y) for(x=0;x<632*3;++x)
            hash=(hash^((const unsigned char *)rgb->pixels)[y*rgb->pitch+x])*UINT32_C(16777619);
        SDL_DestroySurface(rgb);
        if(hash!=UINT32_C(0xc3f17565)) fprintf(stderr,"DOS creation frame: %08lx != c3f17565\n",(unsigned long)hash);
        assert(hash==UINT32_C(0xc3f17565));
    }
    snprintf(path,sizeof path,"%s/%s.bmp",dir,name);
    assert(SDL_SaveBMP(surface,path));SDL_DestroySurface(surface);
    assert(SDL_RenderPresent(r));
}
int main(int argc,char **argv)
{
    TeraExplorationState world;TeraGameSession s,before;TeraResources *res;
    TeraPresentationSink sink={collect,NULL};TeraCharacterCard copy;
    SDL_Window *window;SDL_Renderer *r;char error[256];unsigned i;
    assert(argc==3);
    res=tera_resources_load(argv[1],error,sizeof error);
    if(!res) {fprintf(stderr,"SKIP: %s\n",error);return 77;}
    assert(tera_assets_world(&world,argv[1],1986,0,error,sizeof error));
    assert(tera_game_create(&s,&world,NULL,NULL));
    assert(SDL_Init(SDL_INIT_VIDEO));
    window=SDL_CreateWindow("screens",640,400,SDL_WINDOW_HIDDEN);assert(window);
    r=SDL_CreateRenderer(window,NULL);assert(r);
    assert(tera_textures_create(r,res,&textures));
    capture(r,&s,res,NULL,argv[2],"creation-name");
    assert(tera_game_key(&s,'X'));assert(tera_game_key(&s,8));
    assert(!s.active.creation.name_length);
    for(i=0;i<4;++i) assert(tera_game_key(&s,(unsigned char)"TERA"[i]));
    assert(tera_game_key(&s,13));capture(r,&s,res,NULL,argv[2],"creation-gender");
    assert(tera_game_key(&s,'F'));
    for(i=0;i<3;++i) {
        char name[40];snprintf(name,sizeof name,"creation-split-%u",i+1);
        capture(r,&s,res,NULL,argv[2],name);
        /* Zero and excessive digits are delegated unchanged to the core. */
        assert(tera_game_key(&s,i ? '9' : '0'));
    }
    capture(r,&s,res,NULL,argv[2],"creation-bonus");
    before=s;assert(tera_game_key(&s,'Z'));assert(!memcmp(&before,&s,sizeof s));
    for(i=0;i<6;++i) assert(tera_game_key(&s,(unsigned char)"VVTTAA"[i]));
    capture(r,&s,res,NULL,argv[2],"creation-keep");
    assert(tera_game_key(&s,'N'));assert(s.active.creation.phase==TERA_CREATE_NAME);
    for(i=0;i<16;++i) assert(tera_game_key(&s,(unsigned char)"HERO\rH333VVTTAAO"[i]));
    assert(s.phase==TERA_GAME_READY);assert(tera_game_set_presentation(&s,&sink));
    assert(tera_game_key(&s,'H'));capture(r,&s,res,NULL,argv[2],"character-menu");
    assert(tera_game_key(&s,'I'));assert(cards==1 && s.phase==TERA_GAME_READY);
    assert(held.actor==0 && !strcmp(held.name,"HERO"));
    copy=held;capture(r,&s,res,&held,argv[2],"character-hero");
    assert(!memcmp(&copy,&held,sizeof copy));
    assert(held.hp_limit==s.state.world.characters.combat.encounter.actors[0].hp_limit);
    /* Synthetic roster verifies non-consecutive IDs, HS and each real H/S pause. */
    s.state.world.characters.combat.encounter.actors[3].active=1;
    s.state.world.characters.combat.encounter.actors[8].active=1;
    strcpy(s.state.world.characters.combat.encounter.actors[3].condition,"HS");
    s.state.world.characters.combat.equipment[3][0]=1;
    s.state.world.characters.combat.equipment[3][9]=25;
    s.state.world.characters.combat.encounter.attributes[0].stats[5]=100;
    assert(tera_inventory_pick_actor(&s.state.world.characters,'B')==3);
    assert(tera_inventory_pick_actor(&s.state.world.characters,'C')==8);
    assert(tera_game_key(&s,'H'));assert(tera_game_key(&s,'S'));assert(tera_game_key(&s,'O'));
    assert(cards==2 && held.actor==3 && !strcmp(held.condition,"HS"));
    capture(r,&s,res,&held,argv[2],"character-companion");
    assert(tera_game_key(&s,'x'));assert(cards==3 && held.actor==8);
    assert(tera_game_key(&s,'x'));assert(s.phase==TERA_GAME_READY);
    assert(tera_game_key(&s,'B'));capture(r,&s,res,NULL,argv[2],"character-inventory");
    tera_textures_destroy(&textures);SDL_DestroyRenderer(r);SDL_DestroyWindow(window);SDL_Quit();free(res);
    puts("Character screens: creation/reroll, snapshots, H/I, H/S and read-only rendering OK");
    return 0;
}


