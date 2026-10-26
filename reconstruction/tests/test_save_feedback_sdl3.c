/* Verifier F5/F9, fichier effectivement ecrit et resultat visible dans
 * le framebuffer de production, pas seulement le message interne. */
#include "module_test_support.h"
#include <SDL3/SDL.h>
static Uint64 audit_time=10000;
static uint32_t frame_hash;
static const char *capture_path;
static Uint64 feedback_ticks(void) {return audit_time;}
static bool feedback_present(SDL_Renderer *r)
{
    SDL_Surface *raw=SDL_RenderReadPixels(r,NULL),*rgb;unsigned x,y;
    assert(raw);rgb=SDL_ConvertSurface(raw,SDL_PIXELFORMAT_RGB24);assert(rgb);
    frame_hash=UINT32_C(2166136261);
    for(y=0;y<(unsigned)rgb->h;++y) for(x=0;x<(unsigned)rgb->w*3;++x)
        frame_hash=(frame_hash^((const unsigned char *)rgb->pixels)[y*rgb->pitch+x])*UINT32_C(16777619);
    if(capture_path) assert(SDL_SaveBMP(raw,capture_path));
    SDL_DestroySurface(rgb);SDL_DestroySurface(raw);return SDL_RenderPresent(r);
}
#define SDL_GetTicks feedback_ticks
#define SDL_RenderPresent feedback_present
#include "../frontend/sdl3/app.c"
#undef SDL_GetTicks
#undef SDL_RenderPresent
static void press(App *a,SDL_Keycode key)
{
    SDL_Event e;SDL_zero(e);e.type=SDL_EVENT_KEY_DOWN;e.key.key=key;handle_event(a,&e);
}
int main(int argc,char **argv)
{
    App a={0};TeraExplorationState state=module_fixture();TeraGameSession before,decoded;
    char save[4096],missing[4096],picture[4096],error[512];FILE *f;uint32_t plain;
    unsigned char expected[TERA_SAVE_CAPACITY],actual[TERA_SAVE_CAPACITY];size_t n,m;
    assert(argc==3 && SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS));
    snprintf(save,sizeof save,"%s/feedback-test.sav",argv[2]);
    snprintf(missing,sizeof missing,"%s/nonexistent-feedback-directory/save.sav",argv[2]);
    snprintf(picture,sizeof picture,"%s/save-feedback.bmp",argv[2]);
    a.save_path=save;a.running=1;
    a.resources=tera_resources_load(argv[1],error,sizeof error);
    if(!a.resources) {fprintf(stderr,"SKIP: %s\n",error);SDL_Quit();return 77;}
    a.window=SDL_CreateWindow("Save feedback",640,400,SDL_WINDOW_HIDDEN);assert(a.window);
    a.renderer=SDL_CreateRenderer(a.window,NULL);assert(a.renderer && tera_textures_create(a.renderer,a.resources,&a.textures));
    state.town_found=1;state.town_id=0;
    assert(tera_game_start(&a.game,&state,NULL,NULL,NULL));before=a.game;
    assert(draw(&a));plain=frame_hash;
    press(&a,SDLK_F5);
    assert(!strcmp(a.message,"Saved") && !strcmp(a.notice,"Partie sauvegardee"));
    assert(!memcmp(&before,&a.game,sizeof before));
    f=fopen(save,"rb");assert(f && tera_save_read(f,&decoded,NULL,NULL,NULL)==TERA_SAVE_OK && !fclose(f));
    assert(tera_save_encode(&a.game,expected,sizeof expected,&n)==TERA_SAVE_OK);
    assert(tera_save_encode(&decoded,actual,sizeof actual,&m)==TERA_SAVE_OK && n==m && !memcmp(expected,actual,n));
    capture_path=picture;assert(draw(&a) && frame_hash!=plain);capture_path=NULL;
    audit_time=13499;assert(draw(&a) && frame_hash!=plain);
    audit_time=13500;assert(draw(&a) && frame_hash==plain);
    a.game.state.world.characters.combat.money-=1;
    press(&a,SDLK_F9);assert(!strcmp(a.notice,"Partie chargee"));
    assert(tera_save_encode(&a.game,actual,sizeof actual,&m)==TERA_SAVE_OK && n==m && !memcmp(expected,actual,n));
    /* Refus hors exploration : le fichier precedent reste lisible. */
    assert(tera_game_key(&a.game,'E') && a.game.phase==TERA_GAME_SHOP);before=a.game;
    press(&a,SDLK_F5);assert(strstr(a.notice,"Sauvegarde indisponible") && !memcmp(&before,&a.game,sizeof before));
    press(&a,SDLK_F9);assert(strstr(a.notice,"Chargement indisponible") && !memcmp(&before,&a.game,sizeof before));
    f=fopen(save,"rb");assert(f && tera_save_read(f,&decoded,NULL,NULL,NULL)==TERA_SAVE_OK && !fclose(f));
    a.game=decoded;a.save_path=missing;before=a.game;
    press(&a,SDLK_F5);assert(strstr(a.notice,"Echec de sauvegarde") && !memcmp(&before,&a.game,sizeof before));
    press(&a,SDLK_F9);assert(strstr(a.notice,"Chargement impossible") && !memcmp(&before,&a.game,sizeof before));
    a.shop_preview_until=audit_time+2000;press(&a,SDLK_F5);assert(strstr(a.notice,"fin de l'animation"));
    tera_textures_destroy(&a.textures);free(a.resources);SDL_DestroyRenderer(a.renderer);SDL_DestroyWindow(a.window);SDL_Quit();
    puts("Save feedback: visible success/expiry, file roundtrip, refusal and I/O errors OK");return 0;
}
