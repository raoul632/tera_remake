/* Compile les controleurs de production dans cette unite de test, sans
 * exposer leurs details internes dans l'API publique du frontend. */
#include <SDL3/SDL.h>
/* Seule l'horloge murale est remplacee pour ce binaire de test : le tir
 * traverse shot_input, conserve sa premiere touche et attend son echeance. */
static Uint64 validation_time=100000;
static Uint64 validation_ticks(void) {return validation_time;}
static const char *pending_capture;
/* Lire le vrai framebuffer AVANT Present : son contenu apres Present
 * n'est pas garanti par SDL, meme si le pilote logiciel le conserve. */
static bool validation_present(SDL_Renderer *renderer)
{
    if(pending_capture) {
        SDL_Surface *surface=SDL_RenderReadPixels(renderer,NULL);bool saved;
        if(!surface) return false;
        saved=SDL_SaveBMP(surface,pending_capture);SDL_DestroySurface(surface);
        if(!saved) return false;
    }
    return SDL_RenderPresent(renderer);
}
#define SDL_GetTicks validation_ticks
#define SDL_RenderPresent validation_present
#include "../frontend/sdl3/app.c"
#undef SDL_GetTicks
#undef SDL_RenderPresent
static void campaign_before(TeraGameSession *s,unsigned key);
static void campaign_after(TeraGameSession *s,unsigned key);
#define TERA_CAMPAIGN_BEFORE campaign_before
#define TERA_CAMPAIGN_AFTER campaign_after
#define main campaign_core_main
#include "test_campaign_c.c"
#undef main

static App frontend;
static unsigned rendered,compared,reloaded,phases;
static int initialized;
static const char *gallery_dir;
static FILE *gallery_index;
static unsigned screenshots,gallery_worlds,gallery_scenes,gallery_tasks,gallery_combat;
static unsigned gallery_phases;

/* Observation uniquement : aucun evenement, tirage ou changement du jeu.
 * Le dossier et le CSV optionnels restent des artefacts locaux de validation. */
static void snapshot(const char *label)
{
    char path[4096];TeraGameView view;
    if(!gallery_dir) return;
    require(tera_game_view(&frontend.game,&view),"gallery view");
    require(snprintf(path,sizeof path,"%s/%03u.bmp",gallery_dir,++screenshots)>0,"gallery path");
    pending_capture=path;require(draw(&frontend),"gallery capture");pending_capture=NULL;
    fprintf(gallery_index,"%u,%u,%u,%d,%d,%s\n",screenshots,rendered,
        view.scene.world,view.scene.position,frontend.game.state.world.tasks_counter,label);
    fflush(gallery_index);
}
static void gallery_step(void)
{
    TeraGameView view;char label[128];unsigned bit;
    if(!gallery_dir) return;
    require(tera_game_view(&frontend.game,&view),"gallery current view");
    bit=1u<<view.phase;
    if(!(gallery_phases&bit)) {
        gallery_phases|=bit;
        if(view.phase==TERA_GAME_CREATE) snapshot("Creation du personnage");
        else if(view.phase==TERA_GAME_SHOP) snapshot("Achats et equipement");
        else if(view.phase==TERA_GAME_SPACE) snapshot("Preparation du voyage spatial");
        else if(view.phase==TERA_GAME_LIFE) snapshot("Defaite et succession");
        else if(view.phase==TERA_GAME_COMPLETE) snapshot("Victoire finale");
    }
    if(view.combat && gallery_combat<4) {
        /* Une seule capture par type, pas une image de chaque action. */
        static unsigned monsters;
        unsigned type=view.combat->game.encounter.monster_type;
        if(type<32 && !(monsters&(1u<<type))) {
            monsters|=1u<<type;++gallery_combat;
            snprintf(label,sizeof label,"Combat contre le monstre %u",type);snapshot(label);
        }
    }
    if(view.space && view.space->phase==TERA_SPACE_SHOT) {
        static int shot;
        if(!shot) {shot=1;snapshot("Tir pendant le vol spatial");}
    }
    if(view.phase==TERA_GAME_READY) {
        bit=1u<<view.scene.world;
        if(!(gallery_worlds&bit)) {
            gallery_worlds|=bit;snprintf(label,sizeof label,"Exploration du monde %u",view.scene.world);snapshot(label);
        }
        bit=1u<<view.scene.kind;
        if(!(gallery_scenes&bit)) {
            gallery_scenes|=bit;snprintf(label,sizeof label,"Nouveau decor - type %u",(unsigned)view.scene.kind);snapshot(label);
        }
    }
    if((unsigned)frontend.game.state.world.tasks_counter>gallery_tasks) {
        gallery_tasks=(unsigned)frontend.game.state.world.tasks_counter;
        snprintf(label,sizeof label,"Objectif accompli - progression %u sur 4",gallery_tasks);snapshot(label);
    }
    /* Les tunnels sont transitoires : afficher une image de la sequence
     * reellement emise, au temps virtuel correspondant, puis reprendre. */
    {
        static int tunnel;size_t i;
        for(i=0;!tunnel && i<frontend.media.count;++i) if(frontend.media.cues[i].event.kind==TERA_PRESENT_TUNNEL) {
            Uint64 saved=validation_time;int active=frontend.media.active;
            Uint64 started=frontend.media.started;
            if(i+30<frontend.media.count && frontend.media.cues[i+30].event.kind==TERA_PRESENT_TUNNEL) i+=30;
            frontend.media.active=1;frontend.media.started=validation_time;
            validation_time+=(frontend.media.cues[i].at+47)/48;
            snapshot("Traversee du tunnel de Meduz");tunnel=1;
            validation_time=saved;frontend.media.active=active;frontend.media.started=started;
        }
    }
}

/* Injecter une seule lecture : les purges de production peuvent donc agir
 * normalement sans avaler les entrees futures du conducteur. */
static void dispatch(SDL_Keycode code,const char *text)
{
    SDL_Event event;SDL_zero(event);
    event.type=text ? SDL_EVENT_TEXT_INPUT : SDL_EVENT_KEY_DOWN;
    if(text) event.text.text=(char *)text;else event.key.key=code;
    require(SDL_PushEvent(&event),"SDL push event");
    while(SDL_PollEvent(&event)) handle_event(&frontend,&event);
}
static void campaign_before(TeraGameSession *s,unsigned key)
{
    char text[2]={(char)key,0};SDL_Keycode code=0;
    unsigned deadline=0;
    if(!initialized) {
        TeraExplorationState world=s->state;
        world.world.characters.combat.input=&frontend.input;
        world.world.characters.combat.presentation=&frontend.presentation;
        require(tera_game_create(&frontend.game,&world,NULL,NULL),"SDL campaign creation");
        frontend.title=1;require(draw(&frontend),"SDL title render");snapshot("Ecran titre");
        dispatch(SDLK_RETURN,NULL);require(!frontend.title,"SDL title dismissal");
        initialized=1;
    }
    switch(key) {
    case 13:code=SDLK_RETURN;break;
    case 8:code=SDLK_BACKSPACE;break;
    case 27:code=SDLK_ESCAPE;break;
    case '8':code=SDLK_UP;break;case '2':code=SDLK_DOWN;break;
    case '4':code=SDLK_LEFT;break;case '6':code=SDLK_RIGHT;break;
    default:break;
    }
    if(frontend.shot_active) {
        deadline=frontend.game.active.space.delay_ms+990;
        validation_time=frontend.shot_started+deadline-1;
    }
    dispatch(code,code ? NULL : text);
    if(deadline) {
        ++validation_time;
        require(shot_input(&frontend,-1),"SDL shot deadline");
    }
}
static void campaign_after(TeraGameSession *s,unsigned key)
{
    unsigned char expected[TERA_SAVE_CAPACITY],actual[TERA_SAVE_CAPACITY];size_t n,m;
    (void)key;
    if(frontend.game.phase!=s->phase) fprintf(stderr,"SDL mismatch input=%u key=%u phase=%d expected=%d\n",rendered,key,frontend.game.phase,s->phase);
    require(frontend.game.phase==s->phase,"SDL campaign phase");
    require(frontend.game.state.world.characters.combat.random.seed==s->state.world.characters.combat.random.seed,"SDL campaign RNG");
    require(!frontend.media.failed,"SDL media allocation/collection");
    phases|=1u<<s->phase;
    require(draw(&frontend),"SDL render each input");++rendered;
    gallery_step();
    if(s->phase==TERA_GAME_READY) {
        require(tera_save_encode(s,expected,sizeof expected,&n)==TERA_SAVE_OK,"direct encoding");
        require(tera_save_encode(&frontend.game,actual,sizeof actual,&m)==TERA_SAVE_OK,"SDL encoding");
        require(n==m && !memcmp(expected,actual,n),"SDL persistent state divergence");
        if(++compared%97==1) {
            dispatch(SDLK_F5,NULL);require(!strcmp(frontend.message,"Saved"),"SDL F5");
            dispatch(SDLK_F9,NULL);require(!strcmp(frontend.message,"Loaded"),"SDL F9");
            require(tera_save_encode(&frontend.game,actual,sizeof actual,&m)==TERA_SAVE_OK && n==m && !memcmp(expected,actual,n),"SDL file roundtrip");
            ++reloaded;require(draw(&frontend),"SDL render after reload");
        }
    }
}
int main(int argc,char **argv)
{
    char error[512];int result;SDL_Surface *surface;
    require(argc>=4,"usage: test_campaign_sdl3 DATA SEED SAVE [CAPTURE]");
    require(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS),"SDL init");
    frontend.running=1;frontend.smoke_mode=1;frontend.save_path=argv[3];
    gallery_dir=getenv("TERA_CAMPAIGN_GALLERY");
    if(gallery_dir) {
        char path[4096];
        require(snprintf(path,sizeof path,"%s/parcours.csv",gallery_dir)>0,"gallery index path");
        gallery_index=fopen(path,"w");require(gallery_index!=NULL,"gallery index");
        fputs("image,input,world,position,objectives,label\n",gallery_index);
    }
    frontend.input.probe=input_probe;frontend.input.context=&frontend;
    frontend.presentation.emit=observed;frontend.presentation.context=&frontend;
    frontend.before_render=SDL_malloc(sizeof *frontend.before_render);
    require(frontend.before_render!=NULL,"SDL before render allocation");
    frontend.resources=tera_resources_load(argv[1],error,sizeof error);
    require(frontend.resources!=NULL,error);
    frontend.window=SDL_CreateWindow("TERA campaign validation",640,480,SDL_WINDOW_HIDDEN);
    require(frontend.window!=NULL,"SDL window");
    frontend.renderer=SDL_CreateRenderer(frontend.window,NULL);
    require(frontend.renderer && tera_video_mode(frontend.renderer,0),"SDL renderer");
    require(tera_textures_create(frontend.renderer,frontend.resources,&frontend.textures),"SDL textures");
    /* Audio dummy ouvert : collecte PCM reelle, attentes murales accelerees. */
    require(tera_media_open(&frontend.media),"SDL audio");
    result=campaign_core_main(3,argv);
    require(initialized && reloaded>10 && frontend.game.phase==TERA_GAME_COMPLETE,"SDL campaign completion");
    if(argc>4) {
        surface=SDL_RenderReadPixels(frontend.renderer,NULL);require(surface!=NULL,"SDL final capture");
        require(SDL_SaveBMP(surface,argv[4]),"SDL final BMP");SDL_DestroySurface(surface);
    }
    printf("SDL CAMPAIGN OK: inputs=%u READY comparisons=%u F5/F9=%u phases=%08x events=%u\n",rendered,compared,reloaded,phases,frontend.events);
    if(gallery_index) {require(fclose(gallery_index)==0,"gallery index close");printf("Gallery: %u captures\n",screenshots);}
    tera_media_close(&frontend.media);SDL_free(frontend.before_render);
    tera_textures_destroy(&frontend.textures);free(frontend.resources);
    SDL_DestroyRenderer(frontend.renderer);SDL_DestroyWindow(frontend.window);SDL_Quit();
    return result;
}
