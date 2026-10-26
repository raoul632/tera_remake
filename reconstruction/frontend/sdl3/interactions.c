#include "scene.h"
#include "characters.h"
#include "text.h"
#include <stdio.h>
#include <string.h>

static const char *item_name(const TeraResources *r,unsigned item)
{
    const char *s=tera_resource_text(r,TERA_RES_ITEMS,item);
    if(s) return s;
    switch(item) {
    case 26:return "Energie +50";case 27:return "Carte Meduz";
    case 28:return "Carte Alfol";case 29:return "Cle n.9";
    default:return "Objet";
    }
}
static const char *item_result(int outcome)
{
    static const char *const names[]={"","Objet acquis","Inventaire plein","Personnage HS",
        "Vide","Ce n'est pas une potion","Potion bue","Abandonne","Objet donne","Annule","Personnage absent"};
    return outcome>=0 && (unsigned)outcome<sizeof names/sizeof names[0] ? names[outcome] : "";
}
int tera_scene_result(const TeraPresentationEvent *e,const TeraResources *r,char *out,size_t cap)
{
    const char *text=NULL;
    if(!e || !out || !cap) return 0;
    switch(e->kind) {
    case TERA_PRESENT_BLOCKED:text=e->source==TERA_SOURCE_MEDUZ ? "Fosse infranchissable !" : "Passage bloque";break;
    case TERA_PRESENT_CHEST:
        if(e->data.chest.reward) {snprintf(out,cap,"%u ducats trouves",e->data.chest.reward);return 1;}
        if(e->data.chest.outcome==TERA_ITEM_ADDED && e->data.chest.content<=25) {
            snprintf(out,cap,"Acquis : %s",item_name(r,e->data.chest.content));return 1;
        }
        text=e->data.chest.outcome ? item_result(e->data.chest.outcome) : e->data.chest.open ? "Coffre ouvert" : "";
        if(e->data.chest.outcome==TERA_ITEM_ADDED && e->data.chest.content>89) text=e->data.chest.content==99 ? "Carte acquise" : "Cle acquise";
        break;
    case TERA_PRESENT_TRAP:
        {static const char *const traps[]={"Piege : gaz","Piege : explosion","Piege : cyanure","Piege : lame","Piege dejoue"};
        if(e->data.trap.kind>=0 && e->data.trap.kind<5) text=traps[e->data.trap.kind];}break;
    case TERA_PRESENT_INVENTORY:text=item_result(e->data.inventory.outcome);break;
    case TERA_PRESENT_SHOP:
        if(e->data.shop.outcome==1 || e->data.shop.outcome==2) {
            snprintf(out,cap,"%s : %s, %d ducats",e->data.shop.outcome==1 ? "Achat" : "Vente",item_name(r,e->data.shop.item),e->data.shop.price);return 1;
        }
        if(e->data.shop.outcome==3) {snprintf(out,cap,"Hopital : %d semaine(s)",e->data.shop.weeks);return 1;}
        text=e->data.shop.outcome==4 ? "Pas assez de ducats" : e->data.shop.outcome==5 ? "Inventaire plein" : e->data.shop.outcome==6 ? "Personnage HS" : "";break;
    case TERA_PRESENT_RECRUIT:
        switch(e->data.recruit.outcome) {
        case TERA_RECRUIT_CANCELLED:text="Annule";break;
        case TERA_RECRUIT_AVAILABLE:text="Un compagnon peut etre sollicite";break;
        case TERA_RECRUIT_NOBODY:text="Personne a solliciter";break;
        case TERA_RECRUIT_FULL:text="Le groupe est complet";break;
        case TERA_RECRUIT_NEGOTIATED:case TERA_RECRUIT_PAID:text=e->data.recruit.joined_mask ? "Compagnon(s) recrute(s)" : "Aucun nouveau compagnon";break;
        case TERA_RECRUIT_TOO_POOR:text="Pas assez de ducats";break;
        default:break;
        }break;
    case TERA_PRESENT_OBJECTIVE:text="Objectif accompli";break;
    case TERA_PRESENT_INTERACTION:
        if(e->data.interaction.effect==TERA_INTERACTION_MENTAL_EXTENSION) text="Extension des facultes mentales !";
        else if(e->data.interaction.effect==TERA_INTERACTION_TOO_POOR) text="Un cheque en bois !";
        else {snprintf(out,cap,"Medias : %d ducats investis",e->data.interaction.amount);return 1;}
        break;
    case TERA_PRESENT_RESIDENT:
        if(e->data.resident.outcome==7) {snprintf(out,cap,"Est %d, Nord %d",e->data.resident.east,e->data.resident.north);return 1;}
        break;
    default:break;
    }
    if(!text || !*text) return 0;
    snprintf(out,cap,"%s",text);return 1;
}
static bool ink(SDL_Renderer *r,unsigned i)
{uint8_t c[4];tera_cga_rgba(i,c);return SDL_SetRenderDrawColor(r,c[0],c[1],c[2],255);}
static bool band(SDL_Renderer *r,int y,int height)
{
    SDL_FRect rect={0,(float)y,320,(float)height};
    return ink(r,0) && SDL_RenderFillRect(r,&rect) && ink(r,3);
}
/* C81D7 selectionne VERBES, pas un libelle agrege invente par le SDL. */
static const char *action(const TeraResources *res,const TeraGameView *v)
{
    const TeraGamePrompt *p=&v->prompt;unsigned id=0;const char *s;
    if(p->command=='M' && p->source==TERA_SOURCE_MEDUZ) return "m\202diter";
    if(p->source==TERA_SOURCE_AMARANDE && p->command=='H') {
        if(p->choice=='A') return tera_resource_text(res,TERA_RES_MESSAGE,92);
        switch(p->choice) {
        case 'D':id=16;break;case 'M':id=17;break;case 'S':id=19;break;
        case 27:id=39;break;default:break;
        }
    } else if(p->source==TERA_SOURCE_SHOP && p->command=='H') return "hopital";
    else switch(p->command) {
    case 'E':id=5;break;case 'O':id=11;break;case 'P':id=12;break;
    case 'I':id=p->source==TERA_SOURCE_DIALOGUE ? 21 : 30;break;
    case 'L':id=8;break;case 'D':id=p->source==TERA_SOURCE_DIALOGUE ? 24 : 4;break;
    case 'M':id=9;break;case 'S':id=p->source==TERA_SOURCE_DIALOGUE ? 25 : 14;break;
    case 'A':id=22;break;case 'V':id=23;break;case 'U':id=15;break;case 'C':id=3;break;
    default:break;
    }
    s=tera_resource_text(res,TERA_RES_VERBS,id);return s ? s : "Confirmer";
}
static bool prompt(SDL_Renderer *r,const TeraGameView *v,const TeraResources *res)
{
    const TeraGamePrompt *p=&v->prompt;char b[128];const TeraSceneView *s=&v->scene;
    if(!band(r,184,16)) return false;
    switch(p->kind) {
    case TERA_PROMPT_CONFIRM:case TERA_PROMPT_GATE:
        if(p->source==TERA_SOURCE_ALFOL && v->subphase==TERA_ALFOL_MEDIA) {
            snprintf(b,sizeof b,"\210tes-vous pr\210ts \205 investir %u $ ?",p->pool);break;
        }
        if(p->kind==TERA_PROMPT_GATE) snprintf(b,sizeof b,"Porte ? %c",(char)p->confirm_key);
        else snprintf(b,sizeof b,"%s ? ",action(res,v));
        break;
    case TERA_PROMPT_ACTOR:snprintf(b,sizeof b,"%s : qui (A-D) ?",action(res,v));break;
    case TERA_PROMPT_SLOT:snprintf(b,sizeof b,"Quel objet (0-9) ?");break;
    case TERA_PROMPT_WORD:snprintf(b,sizeof b,"Quel mot ? %.10s_",p->input);break;
    case TERA_PROMPT_PAUSE:snprintf(b,sizeof b,"Presse une touche");break;
    case TERA_PROMPT_MENU:snprintf(b,sizeof b,"I infos A acheter V vendre D donner");break;
    case TERA_PROMPT_COMMAND:
        if(s->kind==TERA_SCENE_PLACE && s->place==3) snprintf(b,sizeof b,"8 2 4 6 : marcher   M : mediter");
        else if(s->kind==TERA_SCENE_ALFOL_SURFACE) snprintf(b,sizeof b,"E : batiments   R : rembarquer");
        else if(s->kind==TERA_SCENE_MEDUZ) snprintf(b,sizeof b,"8 2 4 6 : marcher   M mediter I invoquer");
        else if(s->kind==TERA_SCENE_AMARANDE || s->kind==TERA_SCENE_PLACE)
            snprintf(b,sizeof b,"8 2 4 6 : marcher   E : entrer");
        else snprintf(b,sizeof b,"8 2 4 6 : marcher O ouvrir P prendre");
        break;
    default:snprintf(b,sizeof b,"Fin de cette sequence");break;
    }
    if(p->kind==TERA_PROMPT_CONFIRM || p->kind==TERA_PROMPT_GATE)
        return tera_text_draw(r,0,192,b,40,1);
    if(!tera_text_draw(r,0,184,b,40,1)) return false;
    return tera_text_draw(r,0,192,p->kind==TERA_PROMPT_WORD ? "Entree : valider / retour : effacer" :
        p->source==TERA_SOURCE_DIALOGUE && p->kind==TERA_PROMPT_MENU ? "S solliciter" :
        p->kind==TERA_PROMPT_COMMAND && s->feature==TERA_FEATURE_UP ? "M monter N parler I invoquer H" :
        p->kind==TERA_PROMPT_COMMAND && s->feature==TERA_FEATURE_DOWN ? "D descendre N parler I invoquer H" :
        p->kind==TERA_PROMPT_COMMAND ? "N parler S seduire L lire I invoquer H" : "",40,1);
}
static bool result_lines(SDL_Renderer *r,const TeraGameView *v,const TeraResources *res,
    const TeraPresentationEvent *events,unsigned n)
{
    char lines[5][128],b[128];unsigned count=0,i,start;
    /* Garder le piege devant ses evenements de blessures. */
    for(i=0;i<n;++i) if(events[i].kind==TERA_PRESENT_TRAP && tera_scene_result(&events[i],res,b,sizeof b)) {
        snprintf(lines[count++],sizeof lines[0],"%s",b);break;
    }
    start=count;
    for(i=0;i<n;++i) {
        const TeraPresentationEvent *e=&events[i];const char *original;
        if(e->kind==TERA_PRESENT_TRAP || (e->kind==TERA_PRESENT_MESSAGE && e->data.message.resource==TERA_TEXT_SETH)) continue;
        original=tera_resource_message(res,e,&v->exploration->world);
        if(original) snprintf(b,sizeof b,"%s",original);
        else if(e->kind==TERA_PRESENT_MESSAGE) snprintf(b,sizeof b,"[Texte resident non reconstitue]");
        else if(e->kind==TERA_PRESENT_ACTOR && e->data.actor.id<10)
            snprintf(b,sizeof b,"%.9s : PV %d PM %d %.2s",v->characters->names[e->data.actor.id],e->data.actor.hp,e->data.actor.magic,e->data.actor.condition);
        else if(!tera_scene_result(e,res,b,sizeof b)) continue;
        if(count==5) {unsigned j;for(j=start;j+1<5;++j) memcpy(lines[j],lines[j+1],sizeof lines[j]);--count;}
        snprintf(lines[count++],sizeof lines[0],"%s",b);
    }
    if(!count) return true;
    if(!band(r,184-(int)count*8,(int)count*8)) return false;
    for(i=0;i<count;++i) if(!tera_text_draw(r,0,184-(int)count*8+(int)i*8,lines[i],40,1)) return false;
    return true;
}
static bool shop(SDL_Renderer *r,const TeraGameView *v,const TeraResources *res,const TeraPresentationEvent *events,unsigned n)
{
    const TeraCombatGame *g=&v->characters->combat;char b[128];unsigned i;int price;
    SDL_FRect border={0,0,319,32};
    if(!ink(r,0) || !SDL_RenderClear(r)) return false;
    if(v->subphase==TERA_SHOP_BUY || (v->subphase==TERA_SHOP_ACTOR && v->prompt.command=='A')) {
        const char *feedback=NULL;
        if(!ink(r,2) || !SDL_RenderRect(r,&border) || !ink(r,1) || !tera_party_draw(r,v) || !ink(r,3)) return false;
        for(i=0;i<19;++i) {
            const char *entry=tera_resource_text(res,TERA_RES_MESSAGE,72+i);
            if(!entry || !tera_text_draw(r,0,32+(int)i*8,entry,40,1)) return false;
        }
        /* C4EFE : photographie de l'equipement apres acquisition, sans
         * relancer l'acquisition ni la negociation lors du dessin. */
        for(i=n;i>0;--i) if(events[i-1].kind==TERA_PRESENT_SHOP) {
            const TeraPresentationEvent *e=&events[i-1];
            if(e->data.shop.outcome==4) feedback="pas assez de p\202p\212tes..";
            else if(e->data.shop.item>=26 && e->data.shop.outcome==1) feedback="ok";
            else if(e->data.shop.outcome==1 || e->data.shop.outcome==5 || e->data.shop.outcome==6) {
                TeraCharacterCard c;unsigned slot;
                SDL_FRect popup={8,72,159,104};
                if(!tera_character_card(v->characters,NULL,v->prompt.actor,&c) ||
                   !ink(r,0) || !SDL_RenderFillRect(r,&popup) ||
                   !ink(r,2) || !SDL_RenderRect(r,&popup) || !ink(r,3)) return false;
                for(slot=0;slot<10;++slot) {
                    snprintf(b,sizeof b,"%u %.16s",slot,c.equipment[slot] ? item_name(res,c.equipment[slot]) : "");
                    if(!tera_text_draw(r,16,80+(int)slot*8,b,18,1)) return false;
                }
                if(e->data.shop.outcome!=1) feedback=item_result(e->data.shop.outcome==5 ? TERA_ITEM_FULL : TERA_ITEM_HS);
            }
            break;
        }
        snprintf(b,sizeof b,"%d ducats   ",g->money);
        border=(SDL_FRect){8,183,159,8};
        return ink(r,3) && tera_text_draw(r,16,184,b,20,1) &&
            ink(r,2) && SDL_RenderRect(r,&border) && ink(r,1) &&
            tera_text_draw(r,200,184,"[ESC -> sortir]",15,1) && ink(r,3) &&
            tera_text_draw(r,0,192,feedback ? feedback : v->subphase==TERA_SHOP_ACTOR ?
                "qui effectue la transaction (A-D)? " : "taper la lettre correspondante (A-S) ",40,1);
    }
    if(v->subphase==TERA_SHOP_SELL) {
        TeraCharacterCard c;if(!tera_character_card(v->characters,NULL,v->prompt.actor,&c)) return false;
        if(!ink(r,3)) return false;
        /* OVL00BC : la liste s'arrete a 24, meme si l'objet 25 se vend. */
        for(i=1;i<=24;++i) {
            if(!tera_shop_sale_quote(i,&price)) return false;
            snprintf(b,sizeof b,"%s %d",item_name(res,i),price);
            if(!tera_text_draw(r,168,(int)(i-1)*8,b,19,1)) return false;
        }
        border=(SDL_FRect){167,0,152,199};
        if(!ink(r,2) || !SDL_RenderRect(r,&border) || !ink(r,3)) return false;
        for(i=0;i<10;++i) {
            snprintf(b,sizeof b,"%u %-16.16s",i,c.equipment[i] ? item_name(res,(unsigned)c.equipment[i]) : "");
            if(!tera_text_draw(r,8,80+(int)i*8,b,19,1)) return false;
        }
        snprintf(b,sizeof b,"%d ducats",g->money);
        return tera_text_draw(r,16,184,b,18,1) && band(r,192,8) &&
            tera_text_draw(r,0,192,"taper le num\202ro de l'objet \205 vendre ",40,1);
    }
    if(!ink(r,3) ||
       !tera_text_draw(r,0,0,"ne quittez pas la ville sans une visite",40,1) ||
       !tera_text_draw(r,0,8,"       \205 nos grandes surfaces",40,1) ||
       !tera_text_draw(r,0,24," - fournitures spatiales",40,1) ||
       !tera_text_draw(r,0,32," - pharmacie",40,1) ||
       !tera_text_draw(r,0,40," - d\202partement des antiquit\202s",40,1) ||
       !tera_text_draw(r,0,48," - \202quipements divers",40,1) ||
       !tera_text_draw(r,0,64,"CENTRE HOSPITALIER \202galement accessible",40,1) ||
       !result_lines(r,v,res,events,n)) return false;
    if(v->subphase==TERA_SHOP_ACTOR)
        return band(r,192,8) && tera_text_draw(r,0,192,"qui effectue la transaction (A-D) ? ",40,1);
    if(v->subphase==TERA_SHOP_MENU)
        return band(r,192,8) &&
            tera_text_draw(r,0,192,v->prompt.remaining ? "" : "autre chose ?",23,1) &&
            tera_text_draw(r,184,192,"[ESC -> sortir]",16,1);
    return prompt(r,v,res);
}
static bool space_ui(SDL_Renderer *r,const TeraGameView *v)
{
    char b[100];const TeraSpaceSession *s=v->space;
    if(!band(r,152,32)) return false;
    snprintf(b,sizeof b,"Energie %d   %d ducats",s->state.energy,s->state.combat.money);
    if(!tera_text_draw(r,0,152,b,40,1)) return false;
    if(s->phase==TERA_SPACE_CONFIRM) snprintf(b,sizeof b,"Embarquer ? O / Entree");
    else if(s->phase==TERA_SPACE_FUEL) snprintf(b,sizeof b,"Acheter du carburant ? O");
    else if(s->phase==TERA_SPACE_DIGIT) snprintf(b,sizeof b,"Combien ? 0-9 (x %u ducats)",v->prompt.pool);
    else snprintf(b,sizeof b,"Pirates de SHAAM : %d",s->pirates);
    if(!tera_text_draw(r,0,160,b,40,1)) return false;
    return tera_text_draw(r,0,168,s->phase==TERA_SPACE_SHOT ? "Tirer : chiffres 1-9 du pave" : "",40,1);
}
bool tera_scene_ui(SDL_Renderer *r,const TeraGameView *v,const TeraResources *res,
    const TeraPresentationEvent *events,unsigned n)
{
    static const char *const places[]={"Kronopolis","Spatioport","Cite des Cranes","Pyramide","Temple de Seth",
        "Sanctuaire de Mahalore","Domaine de Balgence","Roche aux Goules","Porte des Sables","Mortefontaine"};
    char b[96];const char *name;unsigned i;
    if(v->space) return space_ui(r,v);
    if(v->phase==TERA_GAME_SHOP) return shop(r,v,res,events,n);
    if(!band(r,0,32) || !tera_party_draw(r,v)) return false;
    name=v->scene.kind==TERA_SCENE_PLACE && v->scene.place<10 ? places[v->scene.place] :
        v->scene.kind==TERA_SCENE_AMARANDE ? "Amarande" :
        v->scene.kind==TERA_SCENE_ALFOL_SURFACE || v->scene.kind==TERA_SCENE_ALFOL_INTERIOR ? "Alfol" :
        v->scene.kind==TERA_SCENE_MEDUZ ? "Meduz" : v->scene.world==8 ? "Temple de Seth" : "Cite des Cranes";
    snprintf(b,sizeof b,"%s%s",name,v->scene.kind==TERA_SCENE_PLACE ? " - exterieur" :
        v->scene.npc_mask || v->scene.ambient_npc ? " - PNJ" : "");
    if(v->scene.feature==TERA_FEATURE_OFFICE) snprintf(b,sizeof b,"Alfol - Bureau des medias");
    else if(v->scene.feature==TERA_FEATURE_RADIATION) snprintf(b,sizeof b,"Alfol - Produits radioactifs");
    if(!band(r,32,8) || !tera_text_draw(r,0,32,b,40,1)) return false;
    if(!(v->prompt.command=='H' && v->prompt.kind==TERA_PROMPT_MENU) &&
       !(v->prompt.source==TERA_SOURCE_DIALOGUE && v->prompt.kind==TERA_PROMPT_MENU) &&
       !result_lines(r,v,res,events,n)) return false;
    if(v->scene.visible_item) {
        snprintf(b,sizeof b,"Contenu : %s",item_name(res,v->scene.visible_item));
        if(!band(r,136,8) || !tera_text_draw(r,0,136,b,40,1)) return false;
    }
    /* C47B3 ne remplace pas le decor par la fiche du heros. La reponse H
     * est celle deja tiree et emise par le moteur : ne jamais la rechoisir. */
    if(v->prompt.source==TERA_SOURCE_AMARANDE && v->prompt.command=='H' && v->prompt.kind==TERA_PROMPT_MENU) {
        const char *response="";
        for(i=0;i<n;++i) if(events[i].kind==TERA_PRESENT_MESSAGE) {
            const char *original=tera_resource_message(res,&events[i],&v->exploration->world);
            if(original) response=original;
        }
        return band(r,184,16) && tera_text_draw(r,0,184,"halte",40,1) && tera_text_draw(r,0,192,response,40,1);
    }
    if(v->prompt.source==TERA_SOURCE_DIALOGUE && v->prompt.kind==TERA_PROMPT_MENU)
        return band(r,184,16) && tera_text_draw(r,0,184,"Negocier",40,1) &&
            tera_text_draw(r,0,192,tera_resource_text(res,TERA_RES_MESSAGE,95),40,1);
    for(i=n;i>0;--i) if(events[i-1].kind==TERA_PRESENT_MESSAGE &&
        events[i-1].data.message.resource==TERA_TEXT_SETH) {
        const char *inscription=tera_resource_message(res,&events[i-1],&v->exploration->world);
        if(inscription) return band(r,184,16) && tera_text_draw(r,0,
            events[i-1].data.message.id==100 ? 184 : 192,inscription,40,2);
    }
    return prompt(r,v,res);
}
