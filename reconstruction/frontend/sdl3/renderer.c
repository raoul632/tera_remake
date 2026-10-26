#include "panels.h"
#include "renderer.h"
#include <stdio.h>
#include "text.h"
#include "characters.h"
#include "scene.h"
#include "battle.h"

static const char *phase_name(TeraGamePhase phase)
{
    static const char *const names[]={"READY","CREATE","DAY","AMARANDE",
        "NAVIGATION","ALFOL","MEDUZ","SHOP","SPACE","DIALOGUE","INVENTORY",
        "LIFE","ENDING","COMPLETE","QUIT","HALTED","ERROR"};
    return (unsigned)phase<sizeof names/sizeof names[0] ? names[phase] : "UNKNOWN";
}
static bool line(SDL_Renderer *r,int y,const char *text)
{
    return tera_text_draw(r,0,y,text,40,1);
}
bool tera_sdl_render(SDL_Renderer *r,const TeraGameSession *s,const TeraResources *resources,const TeraTextures *textures,
    const char *message,const char *event,const TeraPresentationEvent *text_events,unsigned text_count,const TeraCharacterCard *held)
{
    TeraGameView v;const TeraCombatGame *g;char text[128];unsigned i;
    if(!tera_game_view(s,&v)) return false;
    if(v.creation) return tera_creation_screen(r,v.creation,resources);
    if(v.ending || (v.phase==TERA_GAME_LIFE && v.subphase==TERA_LIFE_ACK) ||
       (v.phase==TERA_GAME_COMPLETE && s->completed_from==TERA_GAME_LIFE)) return tera_end_screen(r,&v);
    if(v.phase==TERA_GAME_AMARANDE && (v.prompt.command=='?' || v.prompt.command=='+'))
        return tera_help_screen(r);
    for(i=0;i<text_count;++i) if(text_events[i].kind==TERA_PRESENT_RESIDENT &&
        text_events[i].data.resident.command=='H') {
        if(text_events[i].data.resident.outcome==3) return tera_scene_draw(r,&v,resources,textures) &&
            tera_scene_ui(r,&v,resources,text_events,text_count) && tera_map_overlay(r,&v,SDL_GetTicks()) &&
            (v.scene.palette==3 || tera_cga_present_palette(r,v.scene.palette));
        if(text_events[i].data.resident.outcome==5) return tera_halt_screen(r,resources,textures);
    }
    if(v.combat) return tera_battle_screen(r,&v,resources,textures,text_events,text_count,NULL);
    if(held || v.prompt.source==TERA_SOURCE_INVENTORY)
        return tera_character_screen(r,&v,resources,held);
    if(!v.combat && !v.ending && v.phase<TERA_GAME_COMPLETE)
        return tera_scene_draw(r,&v,resources,textures) && tera_scene_ui(r,&v,resources,text_events,text_count) &&
            (v.scene.palette==3 || tera_cga_present_palette(r,v.scene.palette));
    if(!tera_video_mode(r,0)) return false;
    g=v.combat ? &v.combat->game : &v.characters->combat;
    if(!SDL_SetRenderDrawColor(r,0,0,0,255) || !SDL_RenderClear(r) ||
       !SDL_SetRenderDrawColor(r,255,255,255,255)) return false;
    if(!line(r,40,"TERA / SDL3 - CORE DIAGNOSTIC")) return false;
    snprintf(text,sizeof text,"%s sub=%d world=%u pos=%d",phase_name(v.phase),v.subphase,g->world_id,g->position);
    if(!line(r,50,text)) return false;
    snprintf(text,sizeof text,"prev=%d days=%d gold=%d xp=%d",g->previous_position,v.characters->days,g->money,g->experience);
    if(!line(r,60,text)) return false;
    snprintf(text,sizeof text,"Objectives: %u %u %u %u",v.exploration->world.tasks_done[0],v.exploration->world.tasks_done[1],v.exploration->world.tasks_done[2],v.exploration->world.tasks_done[3]);
    if(!line(r,70,text)) return false;
    if(!tera_party_draw(r,&v)) return false;
    if(v.combat) {
        const char *name=tera_resource_text(resources,TERA_RES_MONSTERS,g->encounter.monster_type);
        snprintf(text,sizeof text,"%.39s",name ? name : "[MONSTRE : NOM INDISPONIBLE]");
        if(!line(r,82,text)) return false;
        snprintf(text,sizeof text,"HP=%d actor=%u",g->encounter.monster_hp,v.combat->actor);
        if(!line(r,92,text)) return false;
        if(!line(r,102,v.prompt.kind==TERA_PROMPT_WORD ? "Word: letters / Enter" :
            v.prompt.kind==TERA_PROMPT_CONFIRM ? "Confirm: O / Enter" : "Combat: T L J D A P C F")) return false;
    } else if(v.ending) {
        snprintf(text,sizeof text,"%s SCORE %.31s",v.ending->victory ? "VICTORY" : "CONDOLENCES",v.ending->score_text);
        if(!line(r,82,text)) return false;
    } else if(v.space) {
        snprintf(text,sizeof text,"[SPACE] pirates=%d stage=%u",v.space->pirates,v.space->input_stage);
        if(!line(r,82,text)) return false;
    } else {
        snprintf(text,sizeof text,"[PLACE] found=%d id=%u",v.exploration->town_found,v.exploration->town_id);
        if(!line(r,82,text)) return false;
        if(!line(r,92,"2468: move / E: enter / L: read")) return false;
        if(!line(r,102,"N S: talk / A B: items / H: menu")) return false;
    }
    {
        static const char *const prompts[]={"Command","Menu","Confirm","Word","Actor A-D",
            "Slot 0-9","Any key","Name","Gender H/F","Split 0-9","Bonus","Keep?","Door","Digit","Timed shot","Finished"};
        if(v.prompt.kind==TERA_PROMPT_CONFIRM || v.prompt.kind==TERA_PROMPT_GATE)
            snprintf(text,sizeof text,"Read: %s %c%s",prompts[v.prompt.kind],(char)v.prompt.confirm_key,v.prompt.accepts_enter ? " / Enter" : "");
        else snprintf(text,sizeof text,"Read: %s",prompts[v.prompt.kind]);
        if(!line(r,112,text)) return false;
    }
    if(!line(r,122,event)) return false;
    if(text_events && text_count) {
        for(i=0;i<text_count && i<2;++i) {
            const char *original=tera_resource_message(resources,&text_events[i],&v.exploration->world);
            if(!tera_text_draw(r,0,132+(int)i*8,original ? original : "[Texte resident non encore integre]",40,1)) return false;
        }
    }
    if(!line(r,150,message) || !line(r,158,"Arrows=2468 / letters=core keys") ||
       !line(r,168,"F5 save / F9 load (READY only)") ||
       !line(r,178,"F11 fullscreen / window X exits") ||
       !line(r,190,"Scenes originales : integration a suivre")) return false;
    return true;
}


