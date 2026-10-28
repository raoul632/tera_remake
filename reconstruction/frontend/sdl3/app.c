#include "text.h"
#include "app.h"
#include "assets.h"
#include "input.h"
#include "renderer.h"
#include "sprites.h"
#include "battle.h"
#include "media.h"
#include "tera/save.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    TeraGameSession game;
    TeraInputSource input;
    TeraPresentationSink presentation;
    TeraPresentationEvent recent[64];
    TeraResources *resources;
    TeraTextures textures;
    TeraMedia media;
    TeraGameSession *before_render;
    TeraGameView animation_view;
    int smoke_mode,purge_after_media,smoke_animation_key;
    TeraExplorationState battle_background;
    TeraSceneView battle_scene;
    TeraLandscape battle_landscape;
    TeraPresentationEvent text_events[16];
    unsigned text_count;
    TeraCharacterCard held_card;
    int card_visible;
    const char *save_path;
    char message[128],event[128];
    Uint64 shot_started;
    Uint64 shop_preview_until;
    Uint64 notice_until;
    char notice[128];
    int running,shot_active,title;
    unsigned keys,events;
} App;

/* F5/F9 appartiennent au port : afficher leur resultat meme si le renderer
 * historique n'affiche pas les messages de diagnostic de l'application. */
static void show_notice(App *a,const char *text)
{
    snprintf(a->notice,sizeof a->notice,"%s",text);
    a->notice_until=SDL_GetTicks()+3500;
}
static int present(App *a)
{
    if(a->notice_until && SDL_GetTicks()<a->notice_until) {
        SDL_FRect band={0,184,320,16};
        if(!tera_video_mode(a->renderer,0) ||
           !SDL_SetRenderDrawColor(a->renderer,0,0,0,255) || !SDL_RenderFillRect(a->renderer,&band) ||
           !SDL_SetRenderDrawColor(a->renderer,255,255,255,255) ||
           !tera_text_draw(a->renderer,0,184,a->notice,40,2)) return 0;
    }
    return SDL_RenderPresent(a->renderer);
}

/* Le callback SDL ne rappelle jamais le core. Les purges semantiques sont
 * demandees par celui-ci ; elles n'effacent pas les evenements de fermeture. */
static int input_probe(void *context,TeraInputBoundary boundary,unsigned frame)
{
    App *a=context;(void)frame;
    SDL_PumpEvents();
    if(boundary==TERA_INPUT_MONSTER15_FRAME && !a->smoke_mode) {
        TeraBattleFrame picture;unsigned i;SDL_Event closing;
        if(a->smoke_animation_key) {
            SDL_Event key;SDL_zero(key);key.type=SDL_EVENT_KEY_DOWN;key.key.key=SDLK_SPACE;
            SDL_PushEvent(&key);a->smoke_animation_key=0;
        }
        memset(&picture,0,sizeof picture);picture.type=15;picture.kind=TERA_EVENT_ROUND;
        for(i=0;i<10;++i) {
            const TeraRoundActor *actor=&a->animation_view.characters->combat.encounter.actors[i];
            TeraBattleVitals *v=&picture.actors[i];
            v->hp=actor->hp;v->magic=actor->magic;v->hp_limit=actor->hp_limit;v->magic_limit=actor->magic_limit;
            v->active=actor->active;memcpy(v->condition,actor->condition,3);
        }
        if(!tera_media_start(&a->media,SDL_GetTicks())) {a->running=0;return 1;}
        while(a->running && tera_media_busy(&a->media,SDL_GetTicks())) {
            SDL_PumpEvents();
            if(SDL_PeepEvents(&closing,1,SDL_GETEVENT,SDL_EVENT_QUIT,SDL_EVENT_QUIT)>0) a->running=0;
            if(!tera_battle_screen(a->renderer,&a->animation_view,a->resources,&a->textures,NULL,0,&picture) ||
               !present(a)) {a->running=0;break;}
            SDL_Delay(8);
        }
        tera_media_reset(&a->media);
        /* La touche reste tamponnee ; prepare_read la purgera. */
        return !a->running || SDL_HasEvent(SDL_EVENT_KEY_DOWN) || SDL_HasEvent(SDL_EVENT_TEXT_INPUT);
    }
    if(boundary==TERA_INPUT_PURGE) {
        a->purge_after_media=1;
        SDL_FlushEvent(SDL_EVENT_KEY_DOWN);SDL_FlushEvent(SDL_EVENT_TEXT_INPUT);return 0;
    }
    return SDL_HasEvent(SDL_EVENT_KEY_DOWN) || SDL_HasEvent(SDL_EVENT_TEXT_INPUT);
}
static void observed(void *context,const TeraPresentationEvent *e)
{
    static const char *const names[]={"MOVE","BLOCKED","ZONE","PLACE","TEXT","CHEST",
        "TRAP","ITEM","SHOP","RECRUIT","OBJECTIVE","ENDING","COMBAT","PANEL","ACTOR","CHARACTER","INTERACTION",
        "TONES","WAIT","FLASH","MONSTER","SCENERY","RAY","TUNNEL"};
    const char *name=(unsigned)e->kind<sizeof names/sizeof names[0] ? names[e->kind] : "UNKNOWN";
    App *a=context;
    /* OVL00BC : C4EFE, DELAY(2000), restauration, puis ReadChar/C4210.
     * La photographie n'est pas une nouvelle phase du moteur. */
    if(e->kind==TERA_PRESENT_SHOP && e->data.shop.item>0 && e->data.shop.item<26 &&
       (e->data.shop.outcome==1 || e->data.shop.outcome==5 || e->data.shop.outcome==6) && !a->smoke_mode)
        a->shop_preview_until=SDL_GetTicks()+2000;
    tera_media_collect(&a->media,e);
    tera_scenery_collect(&a->textures,e);
    if(e->kind==TERA_PRESENT_MONSTER) a->textures.monster=e->data.monster;
    if(e->kind==TERA_PRESENT_COMBAT && e->data.combat.kind==TERA_EVENT_INTRO && e->data.combat.actor)
        memset(&a->textures.monster,0,sizeof a->textures.monster);
    if(e->kind==TERA_PRESENT_CHARACTER) {a->held_card=e->data.character;a->card_visible=1;}
    a->recent[a->events%64]=*e;
    ++a->events;
    if(e->kind!=TERA_PRESENT_MOVE && e->kind!=TERA_PRESENT_PLACE && e->kind!=TERA_PRESENT_ZONE &&
       e->kind!=TERA_PRESENT_CHARACTER && e->kind!=TERA_PRESENT_TONES && e->kind!=TERA_PRESENT_WAIT &&
       e->kind!=TERA_PRESENT_FLASH && e->kind!=TERA_PRESENT_MONSTER && e->kind!=TERA_PRESENT_SCENERY && e->kind!=TERA_PRESENT_RAY && e->kind!=TERA_PRESENT_TUNNEL) {
        if(a->text_count==16) {memmove(a->text_events,a->text_events+1,15*sizeof a->text_events[0]);a->text_count=15;}
        a->text_events[a->text_count++]=*e;
    }
    /* Le diagnostic conserve les 64 derniers evenements. Le nombre de
     * remplacements est visible ; le core, lui, n'en abandonne aucun. */
    snprintf(a->event,sizeof a->event,"Ev%u %s W%u P%d old=%u",a->events,name,e->world,e->position,a->events>64 ? a->events-64 : 0);
}
/* Apres le delai, retirer uniquement la photo et purger le tampon comme
 * C4210. Aucun prix, inventaire, etat de jeu ou tirage ne change ici. */
static void advance_shop_preview(App *a)
{
    unsigned i,n=0;
    if(!a->shop_preview_until || SDL_GetTicks()<a->shop_preview_until) return;
    a->shop_preview_until=0;
    for(i=0;i<a->text_count;++i) {
        const TeraPresentationEvent *e=&a->text_events[i];
        if(e->kind==TERA_PRESENT_SHOP && e->data.shop.item>0 && e->data.shop.item<26 &&
           (e->data.shop.outcome==1 || e->data.shop.outcome==5 || e->data.shop.outcome==6)) continue;
        a->text_events[n++]=*e;
    }
    a->text_count=n;
    SDL_PumpEvents();
    SDL_FlushEvent(SDL_EVENT_KEY_DOWN);SDL_FlushEvent(SDL_EVENT_TEXT_INPUT);
}
/* Tir reel : le moteur fixe le delai, les purges, la premiere touche et
 * l'echeance. SDL ne fournit que le temps ecoule, borne au prochain poll.
 * Un nouveau tir commence a l'instant present, sans surplus reporte. */
static void sync_shot(App *a)
{
    TeraGameView v;
    tera_game_view(&a->game,&v);
    a->shot_active=v.space && v.space->phase==TERA_SPACE_SHOT;
    if(a->shot_active) a->shot_started=SDL_GetTicks();
}
static int shot_input(App *a,int key)
{
    TeraGameView v;Uint64 elapsed;unsigned deadline;int ended;
    tera_game_view(&a->game,&v);
    if(!v.space || v.space->phase!=TERA_SPACE_SHOT) return 0;
    if(!a->media.active) {
        tera_media_reset(&a->media);
        if(a->before_render) *a->before_render=a->game;
    }
    if(!a->shot_active) sync_shot(a);
    deadline=v.space->delay_ms+990;
    elapsed=SDL_GetTicks()-a->shot_started;
    ended=elapsed>=deadline;
    /* Une touche arrivee APRES l'echeance ne tire pas dans le passe. */
    if(elapsed>deadline) key=-1;
    if(elapsed>deadline) elapsed=deadline;
    if(!tera_game_space_input_at(&a->game,(unsigned)elapsed,key)) return 0;
    if(a->media.samples && !a->smoke_mode && !tera_media_start(&a->media,SDL_GetTicks())) return 0;
    if(ended) sync_shot(a);
    return 1;
}
static int send_key(App *a,unsigned key)
{
    int accepted,was_visible=a->card_visible;
    unsigned old_count=a->text_count;TeraPresentationEvent old_events[16];
    if(a->shop_preview_until || tera_media_busy(&a->media,SDL_GetTicks())) return 0;
    if(a->before_render) {*a->before_render=a->game;tera_game_view(a->before_render,&a->animation_view);}
    tera_media_reset(&a->media);a->purge_after_media=0;
    memcpy(old_events,a->text_events,sizeof old_events);a->text_count=0;
    a->card_visible=0;
    if(a->shot_active) accepted=shot_input(a,(int)key);
    else {
        accepted=tera_game_key(&a->game,key);
        sync_shot(a);
    }
    ++a->keys;
    {TeraGameView v;tera_game_view(&a->game,&v);
     if(v.combat) {
         a->battle_background=*v.exploration;a->battle_scene=v.scene;
         if(v.scene.landscape) {a->battle_landscape=*v.scene.landscape;a->battle_scene.landscape=&a->battle_landscape;}
     }}
    if(accepted && !a->smoke_mode && !a->media.active && !tera_media_start(&a->media,SDL_GetTicks())) return 0;
    if(!accepted) {a->card_visible=was_visible;a->text_count=old_count;memcpy(a->text_events,old_events,sizeof old_events);}
    if(a->game.phase==TERA_GAME_QUIT) a->running=0;
    snprintf(a->message,sizeof a->message,"Key %u: %s",key,accepted ? "accepted by core" : "refused by core");
    return accepted;
}
/* Le format et ses restrictions viennent exclusivement de save.h. Le
 * fichier precedent n'est remplace qu'apres ecriture/flush/fermeture OK. */
static int save_game(App *a)
{
    char *temporary;FILE *file;TeraSaveResult result;int closed;
    if(a->game.phase!=TERA_GAME_READY) {
        snprintf(a->message,sizeof a->message,"Save requires READY");return 0;
    }
    temporary=SDL_malloc(strlen(a->save_path)+5);
    if(!temporary) return 0;
    snprintf(temporary,strlen(a->save_path)+5,"%s.tmp",a->save_path);
    file=fopen(temporary,"wb");
    if(!file) {SDL_free(temporary);snprintf(a->message,sizeof a->message,"Cannot open save temporary file");return 0;}
    result=tera_save_write(file,&a->game);closed=fclose(file)==0;
    if(result!=TERA_SAVE_OK || !closed || !SDL_RenamePath(temporary,a->save_path)) {
        SDL_RemovePath(temporary);SDL_free(temporary);
        snprintf(a->message,sizeof a->message,"Save failed (%d)",(int)result);return 0;
    }
    SDL_free(temporary);snprintf(a->message,sizeof a->message,"Saved");return 1;
}
static int load_game(App *a)
{
    TeraGameSession next;FILE *file;TeraSaveResult result;int closed;
    if(a->game.phase!=TERA_GAME_READY) {
        snprintf(a->message,sizeof a->message,"Load requires READY");return 0;
    }
    file=fopen(a->save_path,"rb");
    if(!file) {snprintf(a->message,sizeof a->message,"Cannot open save");return 0;}
    result=tera_save_read(file,&next,&a->input,NULL,NULL);closed=fclose(file)==0;
    if(result!=TERA_SAVE_OK || !closed) {
        snprintf(a->message,sizeof a->message,"Load failed (%d)",(int)result);return 0;
    }
    a->game=next;a->text_count=0;a->card_visible=0;tera_game_set_presentation(&a->game,&a->presentation);
    tera_media_reset(&a->media);memset(&a->textures.monster,0,sizeof a->textures.monster);
    memset(a->textures.scenery,0,sizeof a->textures.scenery);
    sync_shot(a);input_probe(a,TERA_INPUT_PURGE,0);
    snprintf(a->message,sizeof a->message,"Loaded");return 1;
}
static void handle_event(App *a,const SDL_Event *e)
{
    int key;
    if(e->type==SDL_EVENT_QUIT) a->running=0;
    else if(a->shop_preview_until && (e->type==SDL_EVENT_KEY_DOWN || e->type==SDL_EVENT_TEXT_INPUT)) {
        if(e->type==SDL_EVENT_KEY_DOWN && !e->key.repeat && (e->key.key==SDLK_F5 || e->key.key==SDLK_F9))
            show_notice(a,"Attends la fin de l'animation");
        return;
    }
    else if(a->title && (e->type==SDL_EVENT_KEY_DOWN || e->type==SDL_EVENT_TEXT_INPUT)) {
        a->title=0;input_probe(a,TERA_INPUT_PURGE,0);
    }
    else if(e->type==SDL_EVENT_KEY_DOWN) {
        if(e->key.repeat) return;
        if(tera_media_busy(&a->media,SDL_GetTicks()) && e->key.key!=SDLK_F11) {
            if(e->key.key==SDLK_F5 || e->key.key==SDLK_F9) show_notice(a,"Attends la fin de l'animation");
            return;
        }
        switch(e->key.key) {
        case SDLK_F5:
            if(save_game(a)) show_notice(a,"Partie sauvegardee");
            else show_notice(a,a->game.phase!=TERA_GAME_READY ?
                "Sauvegarde indisponible\nReviens a l'exploration" : "Echec de sauvegarde\nVerifie le dossier du jeu");
            return;
        case SDLK_F9:
            if(load_game(a)) show_notice(a,"Partie chargee");
            else show_notice(a,a->game.phase!=TERA_GAME_READY ?
                "Chargement indisponible\nReviens a l'exploration" : "Chargement impossible\nVerifie la sauvegarde tera.sav");
            return;
        case SDLK_F11:
            if(!SDL_SetWindowFullscreen(a->window,!(SDL_GetWindowFlags(a->window)&SDL_WINDOW_FULLSCREEN)))
                snprintf(a->message,sizeof a->message,"Fullscreen failed");
            return;
        default:break;
        }
        key=tera_sdl_key(&e->key);
        if(key>=0) send_key(a,(unsigned)key);
    } else if(e->type==SDL_EVENT_TEXT_INPUT) {
        /* Une seule lecture ; ne pas reutiliser un collage/tampon pour les
         * menus suivants. Le core n'accepte que des octets ASCII ici. */
        unsigned char key_byte=(unsigned char)e->text.text[0];
        if(key_byte>=32 && key_byte<=126 && e->text.text[1]==0) send_key(a,key_byte);
    }
}
static int draw(App *a)
{
    const TeraPresentationEvent *cue=tera_media_current(&a->media,SDL_GetTicks());
    if(a->title) return SDL_SetRenderDrawColor(a->renderer,0,0,0,255) && SDL_RenderClear(a->renderer) &&
        tera_background_draw(a->renderer,&a->textures,1) && present(a);
    if(tera_media_tunnel(&a->media,SDL_GetTicks())) return tera_media_tunnel_draw(a->renderer,&a->media,SDL_GetTicks()) && present(a);
    if(tera_media_ending(&a->media,SDL_GetTicks()) || (!tera_media_busy(&a->media,SDL_GetTicks()) &&
       (a->game.phase==TERA_GAME_ENDING || (a->game.phase==TERA_GAME_LIFE && a->game.active.life.phase==TERA_LIFE_ACK))))
        return tera_sdl_render(a->renderer,&a->game,a->resources,&a->textures,"","",NULL,0,NULL) && present(a);
    if(cue) {
        const TeraPresentationEvent *flash=tera_media_flash(&a->media,SDL_GetTicks());
        TeraGameView v;tera_game_view(&a->game,&v);
        v.exploration=&a->battle_background;v.characters=&a->battle_background.world.characters;
        v.scene=a->battle_scene;
        return tera_battle_screen(a->renderer,&v,a->resources,&a->textures,NULL,0,&cue->data.combat) &&
            (!flash || tera_battle_flash(a->renderer,flash->data.flash.palette,flash->data.flash.border)) && present(a);
    }
    if(tera_media_busy(&a->media,SDL_GetTicks()) && a->before_render) {
        const TeraPresentationEvent *flash=tera_media_flash(&a->media,SDL_GetTicks());
        const TeraPresentationEvent *ray=tera_media_ray(&a->media,SDL_GetTicks());
        return tera_sdl_render(a->renderer,a->before_render,a->resources,&a->textures,"","",NULL,0,NULL) &&
            (!ray || (SDL_SetRenderDrawColor(a->renderer,255,255,255,255) &&
             SDL_RenderLine(a->renderer,155,134,(float)ray->data.ray.x,(float)ray->data.ray.y) &&
             SDL_RenderLine(a->renderer,156,134,(float)ray->data.ray.x,(float)ray->data.ray.y))) &&
            (!flash || tera_battle_flash(a->renderer,flash->data.flash.palette,flash->data.flash.border)) && present(a);
    }
    return tera_sdl_render(a->renderer,&a->game,a->resources,&a->textures,a->message,a->event,a->text_events,a->text_count,a->card_visible ? &a->held_card : NULL) && present(a);
}
/* Preuve native : un evenement SDL a la fois traverse le vrai mapping,
 * creation puis navigation. Comparaison octet pour octet des sauvegardes
 * core, pas une simple observation visuelle ni un monde synthetique. */
static int smoke(App *a,const char *capture)
{
    static const char script[]="TERA\rH333VVTTAAO";
    TeraGameSession direct=a->game;SDL_Event e;unsigned i;
    unsigned char before[TERA_SAVE_CAPACITY],after[TERA_SAVE_CAPACITY];size_t n,m;
    int initial_position;char text_input[2]={0,0};
    /* Le titre est une presentation sans lecture de jeu. La touche qui
     * le ferme ne doit pas devenir le premier caractere du nom. */
    a->title=1;if(!draw(a)) return 0;
    SDL_zero(e);e.type=SDL_EVENT_KEY_DOWN;e.key.key=SDLK_RETURN;
    if(!SDL_PushEvent(&e)) return 0;
    while(SDL_PollEvent(&e)) handle_event(a,&e);
    if(a->title || memcmp(&direct,&a->game,sizeof direct)) return 0;
    direct.observe=NULL;direct.observer_context=NULL;
    tera_game_set_presentation(&direct,NULL);
    for(i=0;i<sizeof script-1;++i) {
        SDL_zero(e);
        if(script[i]=='\r') {e.type=SDL_EVENT_KEY_DOWN;e.key.key=SDLK_RETURN;}
        else {e.type=SDL_EVENT_TEXT_INPUT;text_input[0]=script[i];e.text.text=text_input;}
        if(!SDL_PushEvent(&e)) return 0;
        while(SDL_PollEvent(&e)) handle_event(a,&e);
        if(!tera_game_key(&direct,(unsigned char)script[i]) || !draw(a)) return 0;
    }
    if(a->game.phase!=TERA_GAME_READY) return 0;
    initial_position=a->game.state.world.characters.combat.position;
    SDL_zero(e);e.type=SDL_EVENT_KEY_DOWN;e.key.key=SDLK_RIGHT;
    if(!SDL_PushEvent(&e)) return 0;
    while(SDL_PollEvent(&e)) handle_event(a,&e);
    if(!tera_game_key(&direct,'6') || a->game.phase!=TERA_GAME_READY ||
       a->game.state.world.characters.combat.position==initial_position) return 0;
    if(!a->events || a->recent[(a->events-1)%64].kind!=TERA_PRESENT_MOVE ||
       a->recent[(a->events-1)%64].data.move.from!=initial_position ||
       a->recent[(a->events-1)%64].data.move.to!=direct.state.world.characters.combat.position) return 0;
    if(tera_save_encode(&direct,before,sizeof before,&n)!=TERA_SAVE_OK ||
       tera_save_encode(&a->game,after,sizeof after,&m)!=TERA_SAVE_OK ||
       n!=m || memcmp(before,after,n)) return 0;
    if(!save_game(a) || !send_key(a,'4') || !load_game(a) ||
       tera_save_encode(&a->game,after,sizeof after,&m)!=TERA_SAVE_OK ||
       n!=m || memcmp(before,after,n)) return 0;
    /* Une vraie phrase originale, choisie par le RNG du core, est resolue
     * et affichee sans ajouter un tirage de presentation. */
    if(!send_key(a,'H') || !tera_game_key(&direct,'H') ||
       !send_key(a,'I') || !tera_game_key(&direct,'I') || !a->card_visible ||
       a->held_card.actor!=0 || a->game.phase!=TERA_GAME_READY || !draw(a)) return 0;
    if(!send_key(a,'L') || !tera_game_key(&direct,'L') || !a->text_count ||
       a->card_visible ||
       !tera_resource_message(a->resources,&a->text_events[a->text_count-1],&a->game.state.world) ||
       tera_save_encode(&direct,before,sizeof before,&n)!=TERA_SAVE_OK ||
       tera_save_encode(&a->game,after,sizeof after,&m)!=TERA_SAVE_OK ||
       n!=m || memcmp(before,after,n)) return 0;
    if(!tera_sdl_render(a->renderer,&a->game,a->resources,&a->textures,a->message,a->event,a->text_events,a->text_count,a->card_visible ? &a->held_card : NULL)) return 0;
    if(capture) {
        SDL_Surface *surface=SDL_RenderReadPixels(a->renderer,NULL);bool saved;
        if(!surface) return 0;
        saved=SDL_SaveBMP(surface,capture);SDL_DestroySurface(surface);
        if(!saved) return 0;
    }
    if(!SDL_RenderPresent(a->renderer)) return 0;
    /* Exercer le vrai service SDL de l'animation interrompable : une
     * touche arrive pendant la premiere image ; trois tirages seulement,
     * puis purge, sans transformer la touche en commande de combat. */
    {
        TeraCombatGame probe=a->game.state.world.characters.combat;unsigned calls=0;
        uint32_t expected=probe.random.seed;unsigned step;
        *a->before_render=a->game;tera_game_view(a->before_render,&a->animation_view);
        tera_media_reset(&a->media);a->smoke_mode=0;a->smoke_animation_key=1;
        for(step=0;step<3;++step) expected=expected*UINT32_C(129)+UINT32_C(0x361962e9);
        if(!tera_monster_presentation_random(&probe,15,&calls) || calls!=3 || probe.random.seed!=expected) return 0;
        input_probe(a,TERA_INPUT_PURGE,0);a->smoke_mode=1;
        if(SDL_HasEvent(SDL_EVENT_KEY_DOWN) || SDL_HasEvent(SDL_EVENT_TEXT_INPUT)) return 0;
    }
    puts("SDL smoke OK: original title/text, creation, arrow -> core, state equality, save/load, render");
    return 1;
}
static int number(const char *text,unsigned long max,unsigned long *out)
{
    char *end;unsigned long n;
    if(!text[0] || text[0]=='-') return 0;
    errno=0;n=strtoul(text,&end,0);
    if(errno || *end || n>max) return 0;
    *out=n;return 1;
}
/* OVL0000 appelle RANDOMIZE apres le code scenario ; BIN 0F14 stocke
 * CX (heure/minute) et DX (seconde/centieme). Garder les graines imposees
 * pour les replays et le smoke, sans toucher au RNG d'une sauvegarde. */
static int startup_seed(int supplied,unsigned long requested,int run_smoke,uint32_t *seed)
{
    SDL_Time ticks;SDL_DateTime local;TeraRandom rng;
    if(supplied || run_smoke) {*seed=(uint32_t)requested;return 1;}
    if(!SDL_GetCurrentTime(&ticks) || !SDL_TimeToDateTime(ticks,&local,true) ||
       !tera_random_clock_seed(&rng,(unsigned)local.hour,(unsigned)local.minute,
            (unsigned)local.second,(unsigned)local.nanosecond/10000000u)) return 0;
    *seed=rng.seed;return 1;
}
int tera_sdl_main(int argc,char **argv)
{
    App a;TeraExplorationState world;SDL_Event event;const char *data=NULL,*capture=NULL,*load=NULL;
    char error[512];unsigned long seed=1986,difficulty=0;
    uint32_t initial_seed;int i,result=1,run_smoke=0,seed_supplied=0;
    SDL_zero(a);a.running=1;a.save_path="tera.sav";
    for(i=1;i<argc;++i) {
        if(!strcmp(argv[i],"--smoke")) run_smoke=1;
        else if(!strcmp(argv[i],"--help")) {
            puts("tera_sdl3 [--data DIR] [--seed N] [--difficulty 0..9] [--save FILE] [--load FILE] [--smoke] [--capture BMP]");return 0;
        } else if(i+1<argc && !strcmp(argv[i],"--data")) data=argv[++i];
        else if(i+1<argc && !strcmp(argv[i],"--save")) a.save_path=argv[++i];
        else if(i+1<argc && !strcmp(argv[i],"--load")) load=argv[++i];
        else if(i+1<argc && !strcmp(argv[i],"--capture")) capture=argv[++i];
        else if(i+1<argc && !strcmp(argv[i],"--seed")) {if(!number(argv[++i],UINT32_MAX,&seed)) goto usage;seed_supplied=1;}
        else if(i+1<argc && !strcmp(argv[i],"--difficulty")) {if(!number(argv[++i],9,&difficulty)) goto usage;}
        else {usage:fprintf(stderr,"Invalid arguments; use --help\n");return 1;}
    }
    if(run_smoke && load) {fprintf(stderr,"--smoke requires a new game\n");return 1;}
    a.smoke_mode=run_smoke;
    if(!SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS)) goto sdl_error;
    a.before_render=SDL_malloc(sizeof *a.before_render);if(!a.before_render) goto sdl_error;
    a.input.probe=input_probe;a.input.context=&a;
    a.presentation.emit=observed;a.presentation.context=&a;
    a.resources=tera_resources_load(data,error,sizeof error);
    if(!a.resources) {fprintf(stderr,"%s\n",error);goto done;}
    if(load) {
        FILE *file=fopen(load,"rb");TeraSaveResult status;
        if(!file) {fprintf(stderr,"Cannot open %s\n",load);goto done;}
        status=tera_save_read(file,&a.game,&a.input,NULL,NULL);
        if(fclose(file)!=0 || status!=TERA_SAVE_OK) {fprintf(stderr,"Invalid save (%d)\n",(int)status);goto done;}
        tera_game_set_presentation(&a.game,&a.presentation);
    } else {
        if(!startup_seed(seed_supplied,seed,run_smoke,&initial_seed)) {
            fprintf(stderr,"Cannot initialize original clock seed\n");goto done;
        }
        if(!tera_assets_world(&world,data,initial_seed,(unsigned)difficulty,error,sizeof error)) {
            fprintf(stderr,"%s\n",error);goto done;
        }
        world.world.characters.combat.input=&a.input;
        world.world.characters.combat.presentation=&a.presentation;
        if(!tera_game_create(&a.game,&world,NULL,NULL)) {fprintf(stderr,"Core creation failed\n");goto done;}
    }
    a.window=SDL_CreateWindow("TERA - SDL3 / C",1280,800,SDL_WINDOW_RESIZABLE);
    if(!a.window) goto sdl_error;
    a.renderer=SDL_CreateRenderer(a.window,NULL);
    if(!a.renderer || !tera_video_mode(a.renderer,0) ||
       !SDL_StartTextInput(a.window)) goto sdl_error;
    if(!tera_textures_create(a.renderer,a.resources,&a.textures)) goto sdl_error;
    if(!tera_media_open(&a.media)) fprintf(stderr,"Audio indisponible : %s (temporisations conservees)\n",SDL_GetError());
    a.title=!run_smoke && !load;
    SDL_SetRenderVSync(a.renderer,1);
    snprintf(a.message,sizeof a.message,"Core ready - no DOS / no Python");
    if(run_smoke) {result=smoke(&a,capture) ? 0 : 1;if(result) fprintf(stderr,"SDL smoke failed: %s\n",SDL_GetError());goto done;}
    while(a.running) {
        advance_shop_preview(&a);
        while(SDL_PollEvent(&event)) handle_event(&a,&event);
        if(a.media.active && !tera_media_busy(&a.media,SDL_GetTicks())) {
            a.media.active=0;
            if(a.purge_after_media) input_probe(&a,TERA_INPUT_PURGE,0);
            sync_shot(&a);
        }
        if(a.shot_active && !tera_media_busy(&a.media,SDL_GetTicks())) shot_input(&a,-1);
        if(!draw(&a)) goto sdl_error;
        SDL_Delay(8);
    }
    result=0;goto done;
    sdl_error:fprintf(stderr,"SDL3: %s\n",SDL_GetError());
    done:
    tera_media_close(&a.media);
    SDL_free(a.before_render);
    tera_textures_destroy(&a.textures);free(a.resources);
    SDL_DestroyRenderer(a.renderer);SDL_DestroyWindow(a.window);SDL_Quit();return result;
}





