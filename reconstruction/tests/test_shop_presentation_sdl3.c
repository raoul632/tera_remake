/* Fixture synthetique de commerce ; vrai core, mapping et renderer SDL.
 * Horloge virtuelle pour verifier DELAY(2000) sans attente murale. */
#include "module_test_support.h"
#include <SDL3/SDL.h>
static Uint64 audit_time=10000;
static Uint64 audit_ticks(void) {return audit_time;}
#define SDL_GetTicks audit_ticks
#include "../frontend/sdl3/app.c"
#undef SDL_GetTicks

static void key(App *a,unsigned char c)
{
    SDL_Event e;char text[2]={(char)c,0};SDL_zero(e);
    e.type=SDL_EVENT_TEXT_INPUT;e.text.text=text;handle_event(a,&e);
}
static uint32_t screen(App *a,const char *dir,const char *name)
{
    char path[4096];SDL_Surface *surface,*rgba;uint32_t hash=UINT32_C(2166136261);unsigned x,y,k;
    assert(tera_sdl_render(a->renderer,&a->game,a->resources,&a->textures,"","",a->text_events,a->text_count,NULL));
    surface=SDL_RenderReadPixels(a->renderer,NULL);assert(surface);
    snprintf(path,sizeof path,"%s/%s.bmp",dir,name);assert(SDL_SaveBMP(surface,path));
    rgba=SDL_ConvertSurface(surface,SDL_PIXELFORMAT_RGBA32);assert(rgba && rgba->w==640 && rgba->h==400);
    /* Zone recouverte par la photo ; argent/invite exclus. */
    for(y=144;y<352;++y) for(x=16;x<334;++x) for(k=0;k<3;++k)
        hash=(hash^((const unsigned char *)rgba->pixels)[y*rgba->pitch+x*4+k])*UINT32_C(16777619);
    SDL_DestroySurface(rgba);SDL_DestroySurface(surface);return hash;
}
int main(int argc,char **argv)
{
    App a={0};TeraExplorationState state=module_fixture();TeraGameSession purchased;
    SDL_Event pending;char error[512];unsigned i;uint32_t catalog,photo;
    assert(argc==3 && SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS));
    a.resources=tera_resources_load(argv[1],error,sizeof error);
    if(!a.resources) {fprintf(stderr,"SKIP: %s\n",error);SDL_Quit();return 77;}
    a.window=SDL_CreateWindow("Shop regression",640,400,SDL_WINDOW_HIDDEN);assert(a.window);
    a.renderer=SDL_CreateRenderer(a.window,NULL);assert(a.renderer && tera_textures_create(a.renderer,a.resources,&a.textures));
    a.running=1;a.presentation.emit=observed;a.presentation.context=&a;
    a.input.probe=input_probe;a.input.context=&a;
    state.town_found=1;state.town_id=0;
    state.world.characters.combat.input=&a.input;state.world.characters.combat.presentation=&a.presentation;
    assert(tera_game_start(&a.game,&state,NULL,NULL,NULL));
    for(i=0;i<5;++i) key(&a,(unsigned char)"EOAOA"[i]);
    catalog=screen(&a,argv[2],"shop-preview-catalog");key(&a,'A');
    assert(a.game.phase==TERA_GAME_SHOP && a.game.active.shop.outcome==1 && a.game.active.shop.item==17);
    assert(a.shop_preview_until==12000 && a.text_count);
    purchased=a.game;photo=screen(&a,argv[2],"shop-preview-visible");assert(photo!=catalog);
    audit_time=11999;advance_shop_preview(&a);key(&a,'A');
    assert(a.shop_preview_until && !memcmp(&purchased,&a.game,sizeof purchased));
    SDL_zero(pending);pending.type=SDL_EVENT_KEY_DOWN;pending.key.key=SDLK_ESCAPE;
    assert(SDL_PushEvent(&pending));pending.type=SDL_EVENT_TEXT_INPUT;pending.text.text="A";assert(SDL_PushEvent(&pending));
    audit_time=12000;advance_shop_preview(&a);
    assert(!a.shop_preview_until && !SDL_HasEvent(SDL_EVENT_KEY_DOWN) && !SDL_HasEvent(SDL_EVENT_TEXT_INPUT));
    assert(!memcmp(&purchased,&a.game,sizeof purchased));
    for(i=0;i<a.text_count;++i) assert(a.text_events[i].kind!=TERA_PRESENT_SHOP);
    assert(screen(&a,argv[2],"shop-preview-restored")==catalog);
    /* Une touche NOUVELLE apres la purge effectue exactement un achat. */
    key(&a,'A');assert(a.shop_preview_until==14000);
    assert(a.game.active.shop.state.world.characters.combat.equipment[0][1]==17);
    assert(a.game.active.shop.state.world.characters.combat.money<purchased.active.shop.state.world.characters.combat.money);
    /* La fermeture reste disponible pendant le delai. */
    SDL_zero(pending);pending.type=SDL_EVENT_QUIT;handle_event(&a,&pending);assert(!a.running);
    tera_textures_destroy(&a.textures);free(a.resources);SDL_DestroyRenderer(a.renderer);SDL_DestroyWindow(a.window);SDL_Quit();
    puts("Shop preview: 2000 ms, buffered input purged, fresh input accepted, core unchanged during wait OK");
    return 0;
}
