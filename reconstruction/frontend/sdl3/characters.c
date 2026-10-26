#include "characters.h"
#include "text.h"
#include <stdio.h>
#include <string.h>

static bool text(SDL_Renderer *r,int x,int y,const char *s,unsigned cols)
{ return tera_text_draw(r,x,y,s,cols,1); }
static bool frame(SDL_Renderer *r,int x,int y,int w,int h)
{
    SDL_FRect rect={(float)x,(float)y,(float)w,(float)h};
    return SDL_RenderRect(r,&rect);
}
static bool window_title(SDL_Renderer *r,int x,int y,const char *title)
{
    SDL_FRect background={(float)x,(float)y,(float)strlen(title)*8,8};
    return SDL_SetRenderDrawColor(r,0,0,170,255) && SDL_RenderFillRect(r,&background) &&
        SDL_SetRenderDrawColor(r,255,255,85,255) && text(r,x,y,title,60);
}
static bool stats(SDL_Renderer *r,const unsigned char *s,int x,int y)
{
    static const unsigned order[]={2,5,8,4,6,3,9,7,1};
    static const char *const names[]={"Intell.","Volonte","Charme","Enduran.","Vitalite","Chance","Habilete","Agilite","Force"};
    unsigned i;char b[24];
    for(i=0;i<9;++i) {
        snprintf(b,sizeof b,"%-8s %3u",names[i],s[order[i]]);
        if(!text(r,x+(int)(i%3)*104,y+(int)(i/3)*8,b,12)) return false;
    }
    return true;
}
bool tera_party_draw(SDL_Renderer *r,const TeraGameView *v)
{
    unsigned row;char b[64];
    const TeraCombatGame *g=v->combat ? &v->combat->game : &v->characters->combat;
    for(row=0;row<4;++row) {
        int id=tera_inventory_pick_actor(v->characters,'A'+row);const TeraRoundActor *a;
        if(id<0) continue;
        a=&g->encounter.actors[id];
        /* C4D46 : colonnes 1,13,26,37, maxima lus sans les recalculer. */
        snprintf(b,sizeof b,"%c-%.9s",'A'+row,v->characters->names[id]);
        if(!text(r,0,(int)row*8,b,12)) return false;
        snprintf(b,sizeof b,"PV : %d/%d",a->hp,a->hp_limit);
        if(!text(r,96,(int)row*8,b,13)) return false;
        snprintf(b,sizeof b,"PM : %d/%d",a->magic,a->magic_limit);
        if(!text(r,200,(int)row*8,b,11) || !text(r,288,(int)row*8,a->condition,2)) return false;
    }
    return true;
}
static bool card(SDL_Renderer *r,const TeraCharacterCard *c,const TeraResources *res,int details)
{
    static const char *const skill[]={"","lutte","magie","exorcisme","astronaute","pouvoir PSY","incantation","systeme D","negociation","tir"};
    unsigned i,row=0;char b[80];
    if(!SDL_SetRenderDrawColor(r,0,255,255,255) || !text(r,128,32,c->name,10) ||
       !SDL_SetRenderDrawColor(r,255,0,255,255) || !frame(r,0,41,320,38) || !frame(r,2,43,316,34) ||
       !SDL_SetRenderDrawColor(r,255,255,255,255) || !stats(r,c->stats,0,48)) return false;
    if(!details) return true;
    for(i=0;i<10;++i) {
        const char *name=c->equipment[i] ? tera_resource_text(res,TERA_RES_ITEMS,(unsigned)c->equipment[i]) : "";
        snprintf(b,sizeof b,"%u %.16s",i,name ? name : "?");
        if(!text(r,0,80+(int)i*8,b,18)) return false;
        if(c->skills[i]) {
            if(i) snprintf(b,sizeof b,"%-12s niv %u",skill[i],c->skills[i]);
            else snprintf(b,sizeof b,"Competence 0 %u",c->skills[i]);
            if(!text(r,152,80+(int)row++*8,b,21)) return false;
        }
    }
    snprintf(b,sizeof b,"PV %d/%d  PM %d/%d  %.2s",c->hp,c->hp_limit,c->magic,c->magic_limit,c->condition);
    if(!text(r,0,164,b,40)) return false;
    snprintf(b,sizeof b,"Or %d  Experience %d  Cle %d",c->money,c->experience,c->key);
    return text(r,0,176,b,40);
}
bool tera_character_screen(SDL_Renderer *r,const TeraGameView *v,
    const TeraResources *res,const TeraCharacterCard *held)
{
    TeraCharacterCard c;unsigned id=v->prompt.actor;
    if(!tera_video_mode(r,0) ||
       !SDL_SetRenderDrawColor(r,0,0,0,255) || !SDL_RenderClear(r) ||
       !SDL_SetRenderDrawColor(r,255,255,255,255) || !tera_party_draw(r,v)) return false;
    if(held) c=*held;
    else if(!tera_character_card(v->characters,v->combat ? &v->combat->game : NULL,id,&c)) return false;
    if(!card(r,&c,res,1)) return false;
    if(v->prompt.source==TERA_SOURCE_INVENTORY) {
        const char *p=v->prompt.kind==TERA_PROMPT_ACTOR ? (v->prompt.selecting_target ? "A qui ? A-D" : "Qui ? A-D") :
            v->prompt.kind==TERA_PROMPT_SLOT ? "Quel objet ? 0-9" :
            v->prompt.kind==TERA_PROMPT_CONFIRM ? "Confirmation ? O / Entree" :
            v->prompt.command=='A' ? "C : compagnon  O : objet" : "D : donner";
        return text(r,0,192,p,40);
    }
    return text(r,0,192,v->prompt.kind==TERA_PROMPT_PAUSE ? "Presse une touche" : "",40);
}
bool tera_creation_screen(SDL_Renderer *r,const TeraCreationSession *c,const TeraResources *res)
{
    char b[128];TeraCharacterCard cc;const unsigned char *s=c->state.combat.encounter.attributes[0].stats;
    int bonus=c->phase==TERA_CREATE_BONUS;
    int x=bonus ? 144 : 0,w=bonus ? 336 : 624;
    if(!tera_video_mode(r,c->phase!=TERA_CREATE_KEEP) ||
       !SDL_SetRenderDrawColor(r,0,0,c->phase==TERA_CREATE_KEEP ? 0 : 170,255) || !SDL_RenderClear(r)) return false;
    if(c->phase==TERA_CREATE_KEEP) {
        tera_character_card(&c->state,NULL,0,&cc);
        return card(r,&cc,res,0) && SDL_SetRenderDrawColor(r,255,255,255,255) &&
            text(r,0,192,"gardes-tu ce personnage ? ",40);
    }
    if(!SDL_SetRenderDrawColor(r,255,255,85,255) || !tera_text_box(r,x,bonus ? 16 : 0,(unsigned)(w/8+1),bonus ? 19 : 24) ||
       !window_title(r,bonus ? 264 : 120,bonus ? 16 : 0,bonus ? "bonifications" : "cr\202ation de l'aventurier(e)") ||
       !SDL_SetRenderDrawColor(r,255,255,255,255)) return false;
    if(c->phase==TERA_CREATE_NAME) {
        snprintf(b,sizeof b,"Inscris son nom : %s_",c->name_buffer);
        return text(r,8,8,b,77);
    }
    if(c->phase==TERA_CREATE_GENDER) return text(r,8,8,"H(omme) ou F(emme) ? ",77);
    if(bonus) {
        static const unsigned order[]={2,5,8,4,6,3,9,7,1};
        static const char *const labels[]={" Intellig.: "," Volont\202 : "," Charme : ",
            " Endurance: "," Vitalit\202: "," Chance : "," Habilet\202 : "," Agilit\202 : "," Force  : "};
        unsigned row;
        snprintf(b,sizeof b,"%s :",c->state.names[0]);
        if(!SDL_SetRenderDrawColor(r,255,85,85,255) || !text(r,280,56,b,25) ||
           !SDL_SetRenderDrawColor(r,255,255,255,255)) return false;
        for(row=0;row<3;++row) {
            unsigned k=row*3;
            snprintf(b,sizeof b,"%s%2u%s%2u%s%2u",labels[k],s[order[k]],labels[k+1],s[order[k+1]],labels[k+2],s[order[k+2]]);
            if(!text(r,152,72+(int)row*8,b,41)) return false;
        }
        if(!SDL_SetRenderDrawColor(r,255,255,85,255) ||
           !tera_text_draw(r,152,104,"bonifie certaines qualit\202s en attribuant 6 points suppl\202mentaires",41,2) ||
           !text(r,152,120,"taper          I    V    C",41) ||
           !text(r,152,128,"une des   ->   E    T    N",41) ||
           !text(r,152,136,"9 lettres      H    A    F",41)) return false;
        /* GOTOXY(1,12) remplace le premier caractere de la ligne. */
        {SDL_FRect cell={152,112,8,8};
         if(!SDL_SetRenderDrawColor(r,0,0,170,255) || !SDL_RenderFillRect(r,&cell) ||
            !SDL_SetRenderDrawColor(r,255,255,85,255)) return false;}
        snprintf(b,sizeof b,"%u",c->bonuses_left);return text(r,152,112,b,1);
    }
    snprintf(b,sizeof b,"%s a pour qualit\202s principales :",c->state.names[0]);
    if(!text(r,88,16,b,67)) return false;
    snprintf(b,sizeof b,"Force : %u      Intelligence : %u      Chance : %u",s[1],s[2],s[3]);
    if(!text(r,16,56,b,76)) return false;
    {
        static const char *const first[]={"","endurance","volonte","vitalite"};
        static const char *const second[]={"","agilite","charme","habilete"};
        unsigned completed;
        snprintf(b,sizeof b,"Repartis %u points entre %s et %s",s[c->split],first[c->split],second[c->split]);
        if(!text(r,16,88,b,76) || !text(r,16,112,"Combien attribues-tu a la premiere qualite ?",76)) return false;
        /* Ne pas presenter les anciennes valeurs d'une tentative refusee
         * avant que le core ait termine leur nouvelle repartition. */
        for(completed=1;completed<c->split;++completed) {
            snprintf(b,sizeof b,"%s : %u    %s : %u",first[completed],s[completed+3],second[completed],s[completed+6]);
            if(!text(r,16,136+(int)(completed-1)*16,b,76)) return false;
        }
    }
    return true;
}


