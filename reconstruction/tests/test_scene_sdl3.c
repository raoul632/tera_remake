#include "module_test_support.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "renderer.h"
#include "scene.h"
#include "panels.h"
#include "original_map_raster_vectors.h"
#include "input.h"
#include "text.h"
#include "battle.h"
#include "graph.h"
#include "original_scene_vectors.h"
#include "original_final_raster_vectors.h"
#include "cga.h"
#include "tera/save.h"
#include <stdio.h>
#include <stdlib.h>

/* Fixtures explicites de presentation et de routage ; aucune nouvelle
 * partie fabriquee. Les textures et textes sont les ressources originales. */
static TeraTextures textures;
static void final_native_pixels(SDL_Renderer *r,const TeraResources *res,const TeraPresentationSink *sink,const char *dir,int width,int height)
{
    SDL_Texture *previous=SDL_GetRenderTarget(r),*target=SDL_CreateTexture(r,SDL_PIXELFORMAT_RGBA32,SDL_TEXTUREACCESS_TARGET,width,height);
    unsigned i,failures=0;assert(target && SDL_SetRenderTarget(r,target));
    assert(tera_video_mode(r,0));
    for(i=0;i<sizeof native_raster_cases/sizeof *native_raster_cases;++i) {
        const NativeRasterCase *v=&native_raster_cases[i];
        uint8_t pixels[64000]={0};uint32_t hash=UINT32_C(2166136261);unsigned x,y;
        if(v->kind==1) {
            uint32_t pure=UINT32_C(2166136261);
            tera_cga_line(pixels,v->a,v->b,v->c,v->d,(unsigned)v->e);
            for(x=0;x<64000;++x) pure=(pure^pixels[x])*UINT32_C(16777619);
            assert(pure==v->hash);
        }
        {
            SDL_Surface *raw,*rgba;
            assert(SDL_SetRenderDrawColor(r,0,0,0,255) && SDL_RenderClear(r));
            if(v->kind!=1 && v->kind!=7 && v->kind!=8 && v->kind!=12 && v->kind!=13 && v->kind<14) for(x=0;x<320;++x) {
                uint8_t c[4];tera_cga_rgba(x%2+1,c);
                assert(SDL_SetRenderDrawColor(r,c[0],c[1],c[2],255));
                {SDL_FRect stripe={(float)x,0,1,200};assert(SDL_RenderFillRect(r,&stripe));}
            }
            if(v->kind==1) assert(SDL_SetRenderDrawColor(r,255,255,255,255) && tera_graph_line(r,v->a,v->b,v->c,v->d));
            else if(v->kind==2) assert(SDL_SetRenderDrawColor(r,0,0,0,255) && tera_graph_bytes(r,v->a,v->b,v->c,v->d));
            else if(v->kind==3) assert(tera_scene_catalyst(r));
            else if(v->kind==4) {textures.monster.type=19;assert(tera_battle_monster(r,res,&textures,19,11));}
            else if(v->kind==5) assert(tera_scene_portrait(r,res,&textures,0,1,2));
            else if(v->kind==6) assert(tera_scene_portrait(r,res,&textures,12,3,0));
            else if(v->kind==7) {
                TeraGameSession s={0};TeraGameView view;s.phase=TERA_GAME_READY;s.state=module_fixture();
                s.state.town_found=s.state.moved=1;s.state.town_id=1;s.state.world.characters.combat.presentation=sink;
                assert(tera_places_present(&s.state,0) && tera_game_view(&s,&view));
                assert(tera_scene_draw(r,&view,res,&textures));
            } else if(v->kind==8) assert(tera_scene_monolith(r));
            else if(v->kind==9) assert(tera_sprite_draw(r,&textures,res,(unsigned)v->a,v->b,v->c));
            else if(v->kind==10) {
                TeraGameSession s={0};TeraGameView view;int position=1380+v->a;
                s.phase=TERA_GAME_READY;s.state=module_fixture();
                s.state.world.characters.combat.position=(int16_t)(position-(v->b=='6' ? 30 : 1));
                if(v->c) s.state.world.vertical_mountains[0]=(int16_t)position;
                assert(tera_exploration_move(&s.state,(unsigned)v->b,&s.move));
                assert(tera_game_view(&s,&view) && tera_scene_draw(r,&view,res,&textures));
            } else if(v->kind==11) {
                TeraCombatGame g=module_fixture().world.characters.combat;unsigned calls=0;
                g.world_id=(unsigned)v->b;g.presentation=sink;
                assert(tera_monster_presentation_random(&g,(unsigned)v->a,&calls));
                assert(tera_battle_monster(r,res,&textures,(unsigned)v->a,(unsigned)v->b));
            } else if(v->kind==12) {
                TeraGameSession s={0};TeraGameView view;s.phase=TERA_GAME_READY;s.state=module_fixture();
                s.state.town_found=s.state.moved=1;s.state.town_id=(unsigned)v->a;s.state.world.characters.combat.presentation=sink;
                assert(tera_places_present(&s.state,0) && tera_game_view(&s,&view));
                assert(tera_scene_draw(r,&view,res,&textures));
            } else if(v->kind==13) {
                uint8_t c[4];tera_cga_rgba((unsigned)v->e,c);
                assert(SDL_SetRenderDrawColor(r,c[0],c[1],c[2],255));
                assert(tera_graph_arch(r,v->a,v->b,v->c,v->d));
            } else if(v->kind==14) {
                TeraGameSession s={0};TeraGameView view;uint8_t *cell;
                s.phase=TERA_GAME_READY;s.state=module_fixture();s.state.navigation_overlay=1;
                s.state.world.characters.combat.world_id=1;s.state.world.characters.combat.position=145;
                s.state.world.characters.combat.previous_position=(int16_t)(145-v->e);
                cell=s.state.world.maps[145];cell[0]=(uint8_t)v->c;cell[1]=(uint8_t)v->b;
                cell[2]=0;cell[3]=(uint8_t)v->d;cell[4]=255;cell[5]=0;cell[6]=(uint8_t)v->a;
                assert(tera_game_view(&s,&view) && tera_scene_draw(r,&view,res,&textures));
            } else if(v->kind>=15 && v->kind<=17) {
                TeraGameSession s={0};TeraGameView view;uint8_t *cell;
                int position=v->kind==15 ? 45 : v->kind==16 ? 1000 : 945,heading=v->d ? v->d : 1,li,fi,ri;
                s.phase=TERA_GAME_READY;s.state=module_fixture();s.state.encounter_suppressed=1;
                s.state.world.characters.combat.world_id=v->kind==15 ? 0 : v->kind==16 ? 10 : 9;
                s.state.world.characters.combat.on_alfol=v->kind==17;
                s.state.world.characters.combat.position=(int16_t)position;
                s.state.world.characters.combat.previous_position=(int16_t)(position-heading);
                s.state.world.characters.combat.presentation=sink;
                if(heading==1) {li=1;fi=0;ri=3;}else if(heading==-1) {li=3;fi=2;ri=1;}
                else if(heading==10) {li=0;fi=3;ri=2;}else {li=2;fi=1;ri=0;}
                cell=s.state.world.maps[position];memset(cell,0,7);
                cell[li]=(uint8_t)v->a;cell[fi]=(uint8_t)v->b;cell[ri]=(uint8_t)v->c;
                cell[4]=v->kind==15 ? 255 : 0;
                if(v->kind==15) {TeraNavigationSession nav;assert(tera_navigation_start(&nav,&s.state,'8',1,1200,NULL,NULL));}
                if(v->kind==16) {TeraMeduzSession meduz;assert(tera_meduz_start(&meduz,&s.state,0,0,1,0,1200,NULL,NULL));}
                assert(tera_game_view(&s,&view) && tera_scene_draw(r,&view,res,&textures));
            } else continue;
            {SDL_FRect area;SDL_Rect crop;assert(SDL_GetRenderLogicalPresentationRect(r,&area));
             crop.x=(int)area.x;crop.y=(int)area.y;crop.w=(int)area.w;crop.h=(int)area.h;
             raw=SDL_RenderReadPixels(r,&crop);}
            assert(raw);rgba=SDL_ConvertSurface(raw,SDL_PIXELFORMAT_RGBA32);assert(rgba);
            for(y=0;y<200;++y) for(x=0;x<320;++x) {
                uint8_t *p=(uint8_t *)rgba->pixels+((2*y+1)*(unsigned)rgba->h/400)*rgba->pitch+((2*x+1)*(unsigned)rgba->w/640)*4;
                pixels[y*320+x]=(uint8_t)(p[0]==85 ? 1 : p[1]==85 ? 2 : p[0] ? 3 : 0);
            }
            SDL_DestroySurface(raw);SDL_DestroySurface(rgba);
        }
        for(x=0;x<64000;++x) hash=(hash^pixels[x])*UINT32_C(16777619);
        if(i==277 || i==337 || i==340 || i==350) {
            char path[4096];FILE *f;
            snprintf(path,sizeof path,"%s/verified-raster-%u.raw",dir,i);f=fopen(path,"wb");assert(f);
            assert(fwrite(pixels,1,64000,f)==64000 && !fclose(f));
        }
        if(hash!=v->hash) {
            char path[4096];FILE *f;++failures;
            fprintf(stderr,"Native raster %u kind %u: %08lx != %08lx\n",i,v->kind,(unsigned long)hash,(unsigned long)v->hash);
            snprintf(path,sizeof path,"%s/final-raster-%u.raw",dir,i);f=fopen(path,"wb");assert(f);
            assert(fwrite(pixels,1,64000,f)==64000 && !fclose(f));
        }
    }
    assert(SDL_SetRenderTarget(r,previous));SDL_DestroyTexture(target);assert(!failures);
}
static TeraPresentationEvent events[16];static unsigned count;
static void collect(void *context,const TeraPresentationEvent *e)
{
    (void)context;
    if(e->kind==TERA_PRESENT_MONSTER) {textures.monster=e->data.monster;return;}
    if(e->kind==TERA_PRESENT_SCENERY) {tera_scenery_collect(&textures,e);return;}
    if(e->kind==TERA_PRESENT_MOVE || e->kind==TERA_PRESENT_PLACE || e->kind==TERA_PRESENT_ZONE ||
       e->kind==TERA_PRESENT_COMBAT || e->kind==TERA_PRESENT_CHARACTER) return;
    if(count==16) {memmove(events,events+1,15*sizeof events[0]);count=15;}
    events[count++]=*e;
}
static void capture(SDL_Renderer *r,const TeraResources *res,TeraGameSession *s,const char *dir,const char *name)
{
    TeraGameSession before=*s;SDL_Surface *surface;char path[4096];
    assert(tera_sdl_render(r,s,res,&textures,"","",events,count,NULL));
    assert(!memcmp(&before,s,sizeof before));
    surface=SDL_RenderReadPixels(r,NULL);assert(surface);
    snprintf(path,sizeof path,"%s/%s.bmp",dir,name);assert(SDL_SaveBMP(surface,path));
    SDL_DestroySurface(surface);assert(SDL_RenderPresent(r));
}
/* Oracle DATA : le catalogue complet doit occuper les lignes PAS 5..23,
 * avec ses prix/libelles originaux. Comparer la region rendue aux 19
 * enregistrements MESSAGE, sans reecrire le catalogue dans le test. */
static void catalogue_pixels(SDL_Renderer *r,const TeraResources *res,TeraGameSession *s)
{
    SDL_Surface *raw,*actual,*expected;unsigned row;int y;
    assert(tera_sdl_render(r,s,res,&textures,"","",NULL,0,NULL));
    raw=SDL_RenderReadPixels(r,NULL);assert(raw);
    actual=SDL_ConvertSurface(raw,SDL_PIXELFORMAT_RGBA32);assert(actual);SDL_DestroySurface(raw);
    assert(SDL_SetRenderDrawColor(r,0,0,0,255) && SDL_RenderClear(r));
    assert(SDL_SetRenderDrawColor(r,255,255,255,255));
    for(row=0;row<19;++row) assert(tera_text_draw(r,0,32+(int)row*8,
        tera_resource_text(res,TERA_RES_MESSAGE,72+row),40,1));
    raw=SDL_RenderReadPixels(r,NULL);assert(raw);
    expected=SDL_ConvertSurface(raw,SDL_PIXELFORMAT_RGBA32);assert(expected);SDL_DestroySurface(raw);
    assert(actual->w==640 && actual->h==400 && expected->w==640 && expected->h==400);
    /* La ligne y=32 porte aussi le bord inferieur magenta du groupe. */
    for(y=66;y<366;++y) assert(!memcmp((char *)actual->pixels+y*actual->pitch,
        (char *)expected->pixels+y*expected->pitch,640*4));
    SDL_DestroySurface(actual);SDL_DestroySurface(expected);
}
static void equal(const TeraGameSession *s,const TeraGameSession *direct)
{
    TeraGameView a,b;TeraCharacterState left,right;
    assert(tera_game_view(s,&a) && tera_game_view(direct,&b));
    assert(a.phase==b.phase && a.subphase==b.subphase && !memcmp(&a.prompt,&b.prompt,sizeof a.prompt));
    left=*a.characters;right=*b.characters;left.combat.presentation=NULL;right.combat.presentation=NULL;
    assert(!memcmp(&left,&right,sizeof left));
    if(s->phase==TERA_GAME_READY) {
        unsigned char x[TERA_SAVE_CAPACITY],y[TERA_SAVE_CAPACITY];size_t n,m;
        assert(tera_save_encode(s,x,sizeof x,&n)==TERA_SAVE_OK);
        assert(tera_save_encode(direct,y,sizeof y,&m)==TERA_SAVE_OK);
        assert(n==m && !memcmp(x,y,n));
    }
}
/* Une seule lecture par evenement SDL ; la baseline recoit le meme octet
 * directement. Les assertions portent sur le vrai core, meme en sous-menu. */
static void key(TeraGameSession *s,TeraGameSession *direct,unsigned k)
{
    SDL_Event e;char input[2]={(char)k,0};int translated=-1,accepted=-1;
    SDL_zero(e);count=0;
    if(k==27 || k==13 || k==8) {
        e.type=SDL_EVENT_KEY_DOWN;e.key.key=k==27 ? SDLK_ESCAPE : k==13 ? SDLK_RETURN : SDLK_BACKSPACE;
    } else {e.type=SDL_EVENT_TEXT_INPUT;e.text.text=input;}
    assert(SDL_PushEvent(&e));
    while(SDL_PollEvent(&e)) {
        if(e.type==SDL_EVENT_TEXT_INPUT) translated=(unsigned char)e.text.text[0];
        else if(e.type==SDL_EVENT_KEY_DOWN) translated=tera_sdl_key(&e.key);
        else continue;
        assert(translated>=0);accepted=tera_game_key(s,(unsigned)translated);
    }
    assert(accepted>=0 && accepted==tera_game_key(direct,k));equal(s,direct);
}
static void type(TeraGameSession *s,TeraGameSession *direct,const char *keys)
{while(*keys) key(s,direct,(unsigned char)*keys++);}
static void setup(TeraGameSession *s,TeraGameSession *direct,const TeraExplorationState *e,const TeraPresentationSink *sink)
{
    TeraGameContext control={0};control.status_actor=9;
    control.space_backup_position=1200;control.meduz_backup_position=1200;
    {TeraExplorationState observed=*e;const TeraGameContext *ctx=e->world.characters.combat.on_alfol || e->world.characters.combat.world_id==10 ? &control : NULL;
     memset(textures.scenery,0,sizeof textures.scenery);observed.world.characters.combat.presentation=sink;
     assert(tera_game_start(direct,e,ctx,NULL,NULL));assert(tera_game_start(s,&observed,ctx,NULL,NULL));}
    count=0;
}
static void replay_scene(SDL_Renderer *r,const TeraResources *res,const OriginalScenePrimitive *p,size_t count)
{
    size_t i;SDL_Texture *main_target=SDL_GetRenderTarget(r);int off=0;
    for(i=0;i<count;++i,++p) {
        Uint8 c[4];tera_cga_rgba((unsigned)p->color,c);
        assert(SDL_SetRenderDrawColor(r,c[0],c[1],c[2],off && !p->color ? 0 : 255));
        switch(p->kind) {
        case 1:assert(tera_graph_line(r,p->a,p->b,p->c,p->d));break;
        case 2:case 8: {
            SDL_FRect box={(float)(4*p->a),(float)(p->b-p->b%2),(float)(4*(p->c-p->a)),(float)(2*(p->d/2-p->b/2+1))};
            assert(SDL_RenderFillRect(r,&box));break;}
        case 3:case 7:assert(tera_sprite_draw(r,&textures,res,(unsigned)p->c,p->a,p->b));break;
        case 4:tera_cga_rgba((unsigned)p->c,c);assert(SDL_SetRenderDrawColor(r,c[0],c[1],c[2],255));
            assert(SDL_RenderPoint(r,(float)p->a,(float)p->b));break;
        case 5:assert(SDL_SetRenderTarget(r,textures.scene_layer));off=1;
            assert(SDL_SetRenderLogicalPresentation(r,320,200,SDL_LOGICAL_PRESENTATION_DISABLED));
            assert(SDL_SetRenderDrawColor(r,0,0,0,0));assert(SDL_RenderClear(r));break;
        case 6:assert(SDL_SetRenderTarget(r,main_target));off=0;
            {SDL_FRect target={0,0,320,200};assert(SDL_RenderTexture(r,textures.scene_layer,NULL,&target));}break;
        default:assert(0);
        }
    }
}
static void original_scenes(SDL_Renderer *r,const TeraResources *res,const TeraPresentationSink *sink,const char *dir)
{
    unsigned i,j,n=0,value;unsigned short values[35];TeraRandom rng={1986};
    for(i=0;i<7;++i) {assert(tera_random_draw(&rng,3,&value));values[n++]=(unsigned short)value;
        for(j=0;j<4;++j) {assert(tera_random_draw(&rng,4,&value));values[n++]=(unsigned short)value;}}
    for(i=0;i<4;++i) {
        SDL_Surface *actual,*expected;const OriginalScenePrimitive *p;size_t size;int y;
        assert(SDL_SetRenderLogicalPresentation(r,320,200,SDL_LOGICAL_PRESENTATION_LETTERBOX));
        assert(SDL_SetRenderDrawColor(r,0,0,0,255) && SDL_RenderClear(r));
        if(i==0) {
            TeraGameSession s={0};TeraGameView v;s.phase=TERA_GAME_READY;s.state=module_fixture();
            s.state.town_found=s.state.moved=1;s.state.town_id=1;s.state.world.characters.combat.presentation=sink;
            assert(tera_places_present(&s.state,0));assert(tera_game_view(&s,&v));assert(tera_scene_draw(r,&v,res,&textures));
            assert(textures.scenery[TERA_PICTURE_PLACE].picture.count==89 &&
                s.state.world.characters.combat.random.seed==UINT32_C(0x1013b9c3));
            p=original_scene_spaceport;size=sizeof original_scene_spaceport/sizeof *p;
        } else if(i==1) {assert(tera_scene_catalyst(r));p=original_scene_catalyst;size=sizeof original_scene_catalyst/sizeof *p;}
        else if(i==2) {assert(tera_scene_mountains(r,res,&textures,values,35));p=original_scene_mountains;size=sizeof original_scene_mountains/sizeof *p;}
        else {assert(tera_scene_portrait(r,res,&textures,12,3,0));p=original_scene_portrait;size=sizeof original_scene_portrait/sizeof *p;}
        actual=SDL_RenderReadPixels(r,NULL);assert(actual);
        assert(SDL_SetRenderDrawColor(r,0,0,0,255) && SDL_RenderClear(r));replay_scene(r,res,p,size);
        if(i==0) assert(tera_sprite_draw(r,&textures,res,0x52,73,36));
        expected=SDL_RenderReadPixels(r,NULL);assert(expected);
        assert(actual->w==expected->w && actual->h==expected->h && actual->format==expected->format);
        for(y=0;y<actual->h;++y) if(memcmp((Uint8 *)actual->pixels+y*actual->pitch,
            (Uint8 *)expected->pixels+y*expected->pitch,(size_t)actual->w*4)) {
            char path[4096];snprintf(path,sizeof path,"%s/original-scene-%u-actual.bmp",dir,i);assert(SDL_SaveBMP(actual,path));
            snprintf(path,sizeof path,"%s/original-scene-%u-expected.bmp",dir,i);assert(SDL_SaveBMP(expected,path));
            fprintf(stderr,"BIN scene %u differs at row %d\n",i,y);assert(0);
        }
        SDL_DestroySurface(actual);SDL_DestroySurface(expected);
    }
}
static uint32_t raster_hash(const uint8_t *pixels)
{
    uint32_t hash=UINT32_C(2166136261);unsigned i;
    for(i=0;i<TERA_CGA_PIXELS;++i) hash=(hash^pixels[i])*UINT32_C(16777619);
    return hash;
}
static void native_map_pixels(const char *dir)
{
    unsigned i,k;uint8_t *pixels=SDL_malloc(TERA_CGA_PIXELS);TeraExplorationState e;TeraGameView v={0};
    assert(pixels);
    for(k=0;k<sizeof original_map_cases/sizeof original_map_cases[0];++k) {
        const OriginalMapCase *c=&original_map_cases[k];TeraCombatGame *g;
        e=module_fixture();g=&e.world.characters.combat;g->world_id=c->world;g->position=c->position;
        g->previous_position=c->position-c->heading;g->on_alfol=c->alfol;g->difficulty=c->difficulty;
        e.world.alfol_position=45;
        for(i=0;i<1100;++i) {
            unsigned j;for(j=0;j<4;++j) e.world.maps[i][j]=(uint8_t)((i+j)%4);
            e.world.maps[i][6]=(uint8_t)(i%11!=0);
        }
        for(i=0;i<20;++i) e.world.towns[i]=(int16_t)(1000+47*i%900);
        for(i=0;i<9;++i) e.world.horizontal_mountains[i]=(int16_t)(1000+30*i);
        for(i=0;i<25;++i) e.world.vertical_mountains[i]=(int16_t)(1000+3*i);
        v.exploration=&e;v.characters=&e.world.characters;memset(pixels,0,TERA_CGA_PIXELS);
        tera_map_raster(&v,c->blink*500,pixels);
        if(raster_hash(pixels)!=c->hash) {
            char path[4096];FILE *file;snprintf(path,sizeof path,"%s/map-actual-%u.raw",dir,k);
            file=fopen(path,"wb");assert(file);assert(fwrite(pixels,1,TERA_CGA_PIXELS,file)==TERA_CGA_PIXELS);fclose(file);
            fprintf(stderr,"Native map %u: %08lx != %08lx\n",k,(unsigned long)raster_hash(pixels),(unsigned long)c->hash);assert(0);
        }
    }
    for(k=0;k<sizeof original_circle_cases/sizeof original_circle_cases[0];++k) {
        const OriginalCircleCase *c=&original_circle_cases[k];memset(pixels,0,TERA_CGA_PIXELS);
        tera_cga_circle(pixels,c->x,c->y,c->radius,3);
        assert(raster_hash(pixels)==c->hash);
    }
    SDL_free(pixels);
}
int main(int argc,char **argv)
{
    TeraExplorationState e;TeraGameSession s={0},direct;TeraResources *res;TeraGameView v;
    TeraPresentationSink sink={collect,NULL};SDL_Window *window;SDL_Renderer *r;
    unsigned i,j,seed;char error[256],name[64],text[128];int trapped=0;
    assert(argc==3);res=tera_resources_load(argv[1],error,sizeof error);
    if(!res) {fprintf(stderr,"SKIP: %s\n",error);return 77;}
    assert(SDL_Init(SDL_INIT_VIDEO));window=SDL_CreateWindow("scenes",640,400,SDL_WINDOW_HIDDEN);assert(window);
    r=SDL_CreateRenderer(window,NULL);assert(r && tera_textures_create(r,res,&textures));
    native_map_pixels(argv[2]);
    final_native_pixels(r,res,&sink,argv[2],320,200);
    final_native_pixels(r,res,&sink,argv[2],640,480); /* Letterbox and scaled flood fills. */
    original_scenes(r,res,&sink,argv[2]);
    /* Les trois vues d'Alfol et les dechets utilisent le core reel. */
    for(i=0;i<4;++i) {
        TeraAlfolSession observed,plain;TeraExplorationState base=module_fixture(),with_sink;
        base.world.characters.combat.on_alfol=1;
        base.world.characters.combat.position=i==1 ? 44 : i==2 ? 20 : 900;
        if(i==3) {base.world.characters.combat.world_id=9;base.world.characters.combat.previous_position=899;
            base.world.maps[900][0]=1;base.world.maps[901][4]=2;}
        with_sink=base;with_sink.world.characters.combat.presentation=&sink;memset(textures.scenery,0,sizeof textures.scenery);
        assert(tera_alfol_start(&observed,&with_sink,'8',i==0,9,NULL,NULL));
        assert(tera_alfol_start(&plain,&base,'8',i==0,9,NULL,NULL));
        assert(observed.state.world.characters.combat.random.seed==plain.state.world.characters.combat.random.seed);
        assert(textures.scenery[i==3 ? TERA_PICTURE_ROOM : TERA_PICTURE_ALFOL].picture.count==(i==0 ? 89u : i==1 ? 173u : i==3 ? 18u : 163u));
        memset(&s,0,sizeof s);s.phase=TERA_GAME_READY;s.state=observed.state;
        snprintf(name,sizeof name,"alfol-complete-%u",i);capture(r,res,&s,argv[2],name);
    }
    /* Lune a l'est et montagnes : meme etat avec ou sans observation. */
    {
        TeraExplorationState plain,observed=module_fixture();TeraMoveReport a,b;
        for(i=0;i<20;++i) observed.world.towns[i]=-3000;
        observed.world.vertical_mountains[0]=1230;plain=observed;observed.world.characters.combat.presentation=&sink;
        assert(tera_exploration_move(&observed,'6',&a) && tera_exploration_move(&plain,'6',&b));
        assert(a.landscape.command=='6' && a.landscape.mountain_count==35 && !memcmp(&a,&b,sizeof a));
        observed.world.characters.combat.presentation=NULL;assert(!memcmp(&observed,&plain,sizeof plain));
        memset(&s,0,sizeof s);s.phase=TERA_GAME_READY;s.state=observed;s.move=a;
        capture(r,res,&s,argv[2],"amarande-moon-mountains");
    }
    for(i=0;i<10;++i) {
        memset(&s,0,sizeof s);s.phase=TERA_GAME_READY;s.state=module_fixture();s.state.town_found=1;s.state.town_id=i;
        s.state.moved=1;s.state.world.characters.combat.presentation=&sink;
        memset(textures.scenery,0,sizeof textures.scenery);assert(tera_places_present(&s.state,0));
        snprintf(name,sizeof name,"place-%02u",i);capture(r,res,&s,argv[2],name);
    }
    for(i=0;i<23;++i) {
        memset(&s,0,sizeof s);s.phase=TERA_GAME_READY;s.state=module_fixture();
        for(j=0;j<20;++j) s.state.world.towns[j]=-3000;
        s.state.world.characters.combat.position=(int16_t)(1218+i);
        assert(tera_exploration_move(&s.state,'8',&s.move));
        assert(tera_game_view(&s,&v) && v.scene.variant==i);
        snprintf(name,sizeof name,"amarande-%02u",i);capture(r,res,&s,argv[2],name);
    }
    for(i=0;i<4;++i) {
        memset(&s,0,sizeof s);s.phase=TERA_GAME_READY;s.state=module_fixture();
        s.state.world.characters.combat.world_id=i ? 2 : 0;
        s.state.world.characters.combat.position=245;s.state.world.characters.combat.previous_position=244;
        s.state.navigation_overlay=i ? 1 : 0;s.state.world.maps[245][6]=(uint8_t)i;
        s.state.world.maps[245][0]=2;s.state.world.maps[245][1]=1;s.state.world.maps[245][3]=3;
        s.state.world.maps[245][4]=255;
        snprintf(name,sizeof name,"interior-%u",i);capture(r,res,&s,argv[2],name);
    }
    s.state.world.characters.combat.on_alfol=1;s.state.world.characters.combat.world_id=11;
    s.state.world.characters.combat.position=0;capture(r,res,&s,argv[2],"alfol-surface");
    s.state.world.characters.combat.world_id=9;s.state.world.characters.combat.position=900;
    s.state.world.characters.combat.previous_position=899;s.state.world.maps[900][4]=1;
    capture(r,res,&s,argv[2],"alfol-office");
    s.state.world.characters.combat.on_alfol=0;s.state.world.characters.combat.world_id=10;
    s.state.world.characters.combat.position=1000;s.state.world.characters.combat.previous_position=999;
    s.state.world.maps[1000][4]=2;capture(r,res,&s,argv[2],"meduz");
    /* Ouvrir, subir un piege, prendre : aucun contenu affiche avant O/O/A. */
    for(seed=0;seed<100 && !trapped;++seed) {
        e=module_fixture();e.world.characters.combat.world_id=1;
        e.world.characters.combat.position=105;e.world.characters.combat.previous_position=104;e.navigation_overlay=1;
        e.world.maps[105][4]=17;e.world.characters.combat.random.seed=seed;
        e.world.characters.combat.encounter.attributes[0].stats[7]=0;
        e.world.characters.combat.encounter.attributes[0].stats[9]=0;
        e.world.characters.combat.encounter.attributes[0].skills[7]=0;
        setup(&s,&direct,&e,&sink);key(&s,&direct,'O');capture(r,res,&s,argv[2],"chest-confirm");
        type(&s,&direct,"OA");
        for(i=0;i<count;++i) if(events[i].kind==TERA_PRESENT_TRAP) trapped=1;
    }
    assert(trapped && s.phase==TERA_GAME_READY);assert(tera_game_view(&s,&v));
    assert(v.scene.visible_item==17);capture(r,res,&s,argv[2],"chest-trap-open");
    type(&s,&direct,"POA");assert(s.state.world.characters.combat.equipment[0][0]==17);
    assert(!s.state.world.maps[105][4]);capture(r,res,&s,argv[2],"chest-taken");
    /* Porte verrouillee : Entree ne remplace jamais U. */
    e=module_fixture();e.world.characters.combat.world_id=1;e.world.characters.combat.position=105;
    e.world.characters.combat.previous_position=104;e.navigation_overlay=1;e.world.maps[105][0]=3;
    setup(&s,&direct,&e,&sink);key(&s,&direct,'8');assert(tera_game_view(&s,&v));
    assert(v.prompt.kind==TERA_PROMPT_GATE && v.prompt.confirm_key=='U' && !v.prompt.accepts_enter);
    capture(r,res,&s,argv[2],"gate-locked");key(&s,&direct,13);assert(s.state.world.characters.combat.position==105);
    /* Le menu resident, la sollicitation puis la seduction restent ceux du core. */
    e=module_fixture();
    setup(&s,&direct,&e,&sink);key(&s,&direct,'H');
    assert(tera_game_view(&s,&v) && v.prompt.kind==TERA_PROMPT_MENU);
    capture(r,res,&s,argv[2],"halte-menu");
    key(&s,&direct,'D');assert(tera_game_view(&s,&v) && v.prompt.choice=='D');
    capture(r,res,&s,argv[2],"halte-dormir");key(&s,&direct,'N');
    type(&s,&direct,"HM");assert(tera_game_view(&s,&v) && v.prompt.choice=='M');
    capture(r,res,&s,argv[2],"halte-manger");key(&s,&direct,'N');
    key(&s,&direct,'H');key(&s,&direct,27);
    assert(tera_game_view(&s,&v) && v.prompt.choice==27);
    capture(r,res,&s,argv[2],"halte-quitter");key(&s,&direct,'N');
    e=module_fixture();e.world.locations[3]=1200;
    setup(&s,&direct,&e,&sink);key(&s,&direct,'N');capture(r,res,&s,argv[2],"dialogue-menu");
    type(&s,&direct,"SO");capture(r,res,&s,argv[2],"recruit-available");
    type(&s,&direct,"SO");assert(s.state.world.characters.combat.encounter.actors[3].active);
    capture(r,res,&s,argv[2],"recruit-joined");
    /* Entree reelle, achat, vente, hopital et retour. */
    e=module_fixture();e.town_found=1;e.town_id=0;e.world.towns[0]=1200;
    e.world.characters.combat.encounter.attributes[0].stats[8]=0;
    e.world.characters.combat.encounter.attributes[0].skills[8]=0;
    e.world.characters.combat.encounter.attributes[0].skills[7]=0;
    setup(&s,&direct,&e,&sink);type(&s,&direct,"EO");assert(s.phase==TERA_GAME_SHOP);
    capture(r,res,&s,argv[2],"shop-menu");
    key(&s,&direct,27);assert(s.phase==TERA_GAME_SHOP && !s.active.shop.first_menu);
    key(&s,&direct,27);assert(s.phase==TERA_GAME_READY);
    capture(r,res,&s,argv[2],"shop-exterior");
    type(&s,&direct,"EOAO");key(&s,&direct,27);assert(s.active.shop.phase==TERA_SHOP_ACTOR);
    key(&s,&direct,'A');
    capture(r,res,&s,argv[2],"shop-catalogue");catalogue_pixels(r,res,&s);key(&s,&direct,'A');
    assert(s.active.shop.outcome==1 && s.active.shop.price==100);
    assert(count && tera_scene_result(&events[count-1],res,text,sizeof text));
    assert(strstr(text,"100"));capture(r,res,&s,argv[2],"shop-purchase");
    key(&s,&direct,27);type(&s,&direct,"VOA");capture(r,res,&s,argv[2],"shop-sell");
    key(&s,&direct,'0');assert(s.active.shop.outcome==2);key(&s,&direct,27);
    type(&s,&direct,"HO");assert(s.active.shop.outcome==3);capture(r,res,&s,argv[2],"shop-hospital");
    key(&s,&direct,27);assert(s.phase==TERA_GAME_READY);
    /* Meduz : mot affiche sans fuite du mot attendu, puis meditation. */
    e=module_fixture();e.town_found=1;e.town_id=3;
    setup(&s,&direct,&e,&sink);type(&s,&direct,"MOAB");key(&s,&direct,8);
    assert(tera_game_view(&s,&v) && !strcmp(v.prompt.input,"A"));
    capture(r,res,&s,argv[2],"pyramid-word");key(&s,&direct,13);
    e=module_fixture();e.world.characters.combat.world_id=10;
    e.world.characters.combat.position=1000;e.world.characters.combat.previous_position=999;e.world.maps[1000][4]=2;
    setup(&s,&direct,&e,&sink);type(&s,&direct,"MO");
    assert(s.state.world.mental_extension && count==1 && events[0].kind==TERA_PRESENT_INTERACTION);
    capture(r,res,&s,argv[2],"meduz-meditation");
    /* Alfol : saisie puis bureau des medias sur les vrais lecteurs. */
    e=module_fixture();e.world.characters.combat.on_alfol=1;e.world.characters.combat.position=45;
    e.world.maps[900][4]=1;
    setup(&s,&direct,&e,&sink);type(&s,&direct,"ITE");key(&s,&direct,8);
    assert(tera_game_view(&s,&v) && !strcmp(v.prompt.input,"T"));
    capture(r,res,&s,argv[2],"alfol-word");key(&s,&direct,13);
    type(&s,&direct,"EO");assert(tera_game_view(&s,&v));
    assert(v.prompt.source==TERA_SOURCE_ALFOL && v.subphase==TERA_ALFOL_MEDIA && v.prompt.pool==1000 && !v.prompt.accepts_enter);
    capture(r,res,&s,argv[2],"alfol-investment");key(&s,&direct,'O');
    for(i=0,j=0;i<count;++i) if(events[i].kind==TERA_PRESENT_INTERACTION && events[i].data.interaction.effect==TERA_INTERACTION_MEDIA_PAID) ++j;
    assert(j==1);capture(r,res,&s,argv[2],"alfol-paid");
    /* Voyage : carburant et chaque image du tir utilisent le temps du core. */
    memset(&s,0,sizeof s);s.state=module_fixture();s.phase=TERA_GAME_SPACE;
    assert(tera_space_start(&s.active.space,&s.state.world.characters,4));direct=s;
    assert(tera_game_set_presentation(&s,&sink));count=0;
    capture(r,res,&s,argv[2],"space-fuel");key(&s,&direct,'O');
    assert(tera_game_view(&s,&v) && v.prompt.kind==TERA_PROMPT_DIGIT && v.prompt.pool==50);
    capture(r,res,&s,argv[2],"space-quantity");key(&s,&direct,'2');
    assert(s.active.space.phase==TERA_SPACE_SHOT && s.active.space.state.energy==200);
    for(i=0;i<5;++i) {
        static const unsigned times[]={100,350,550,750,950};
        unsigned time=s.active.space.delay_ms+times[i];
        assert(tera_game_space_input_at(&s,time,-1) && tera_game_space_input_at(&direct,time,-1));equal(&s,&direct);
        snprintf(name,sizeof name,"space-shot-%u",i);capture(r,res,&s,argv[2],name);
    }

    /* OVL00E2 : pause sans jour/RNG dans chaque contexte, puis retour. */
    for(i=0;i<4;++i) {
        uint32_t original_seed;int days;
        e=module_fixture();
        if(i==1) {e.world.characters.combat.world_id=0;e.world.characters.combat.position=10;e.world.characters.combat.previous_position=9;}
        if(i==2) {e.world.characters.combat.world_id=10;e.world.characters.combat.position=1000;}
        if(i==3) {e.world.characters.combat.on_alfol=1;e.world.characters.combat.position=45;}
        setup(&s,&direct,&e,&sink);original_seed=e.world.characters.combat.random.seed;days=e.world.characters.days;
        key(&s,&direct,i%2 ? '+' : '?');
        assert(tera_game_view(&s,&v) && v.prompt.kind==TERA_PROMPT_PAUSE);
        snprintf(name,sizeof name,"help-%u",i);capture(r,res,&s,argv[2],name);
        key(&s,&direct,'X');assert(s.phase==TERA_GAME_READY);
        assert(s.state.world.characters.combat.random.seed==original_seed && s.state.world.characters.days==days);
        type(&s,&direct,"HC");
        if(i==1 || i==2) {assert(s.active.amarande.outcome==4);s.state.world.has_map[i==1 ? 0 : 10]=direct.state.world.has_map[i==1 ? 0 : 10]=1;type(&s,&direct,"HC");}
        snprintf(name,sizeof name,"map-%u",i);capture(r,res,&s,argv[2],name);
        if(i==1 || i==2) {assert(s.phase==TERA_GAME_AMARANDE);key(&s,&direct,'X');assert(s.phase==TERA_GAME_READY);}
    }
    e=module_fixture();setup(&s,&direct,&e,&sink);type(&s,&direct,"HH");
    capture(r,res,&s,argv[2],"halt-histogram");
    /* OVL0069 : les deux lignes de l'inscription occupent 24/25. */
    e=module_fixture();e.world.characters.combat.world_id=8;e.world.characters.combat.position=800;
    e.world.characters.combat.previous_position=799;e.navigation_overlay=2;
    setup(&s,&direct,&e,&sink);type(&s,&direct,"LO");
    capture(r,res,&s,argv[2],"seth-inscription");
    assert(count && strstr(tera_resource_message(res,&events[count-1],&e.world),"terribles"));
    s.state.world.maps[800][4]=direct.state.world.maps[800][4]=110;
    type(&s,&direct,"LO");capture(r,res,&s,argv[2],"seth-altar");
    assert(count && !strcmp(tera_resource_message(res,&events[count-1],&e.world),"AUTEL DES TENEBRES"));
    /* OVL00CB : vraie mort, puis lecteur de creation du successeur. */
    memset(&s,0,sizeof s);s.state=module_fixture();s.state.world.characters.combat.encounter.party_available=0;
    s.phase=TERA_GAME_LIFE;assert(tera_life_start(&s.active.life,&s.state));count=0;
    capture(r,res,&s,argv[2],"ending-death");
    assert(tera_game_key(&s,'X') && s.phase==TERA_GAME_LIFE && s.active.life.phase==TERA_LIFE_CREATE);
    capture(r,res,&s,argv[2],"ending-successor");
    memset(&s,0,sizeof s);s.state=module_fixture();
    for(i=0;i<4;++i) s.state.world.tasks_done[i]=1;
    strcpy(s.state.world.secret_words[3],"TESTWORD");s.state.world.characters.combat.experience=12345;
    s.phase=TERA_GAME_ENDING;assert(tera_ending_start(&s.active.ending,&s.state));
    capture(r,res,&s,argv[2],"ending-victory");
    assert(tera_game_key(&s,'X') && s.phase==TERA_GAME_COMPLETE);
    capture(r,res,&s,argv[2],"ending-complete");
    tera_textures_destroy(&textures);SDL_DestroyRenderer(r);SDL_DestroyWindow(window);SDL_Quit();free(res);
    puts("SDL scenes/interactions: original assets, read-only captures, event input and core equality OK");return 0;
}

