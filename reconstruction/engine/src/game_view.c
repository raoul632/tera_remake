#include "tera/game.h"
#include <string.h>

/* Projection des CASE graphiques PAS ; ni deplacement ni revelation du
 * contenu d'un coffre ferme. Ne pas consulter les cartes hors grille. */
static void scene(const TeraGameSession *s,TeraGameView *v)
{
    const TeraExplorationState *e=v->exploration;
    const TeraCombatGame *g=v->combat ? &v->combat->game : &v->characters->combat;
    TeraSceneView *p=&v->scene;const uint8_t *cell;int heading;
    unsigned left,front,right,content,i;
    p->world=g->world_id;p->position=g->position;p->kind=TERA_SCENE_UNKNOWN;
    p->palette=g->world_id<=10 && g->world_id!=9 ? 2 : 3;
    if(g->world_id==11 && !g->on_alfol) {
        p->kind=e->town_found && e->town_id<10 ? TERA_SCENE_PLACE : TERA_SCENE_AMARANDE;
        p->place=e->town_id;p->variant=g->position>=0 ? (unsigned)g->position%23 : 0;
        if(p->kind==TERA_SCENE_PLACE && (p->place==2 || p->place==4 || p->place==7 || p->place==9)) p->palette=2;
        if(p->kind==TERA_SCENE_PLACE && p->place>=5) {
            unsigned id=p->place==7 ? 5 : p->place==6 || p->place==8 ? 8 : 3;
            const TeraRoundActor *a=&g->encounter.actors[id];
            p->place_npc=e->world.locations[id]!=e->world.towns[p->place] ||
                (!a->active && strcmp(a->condition,"HS")!=0);
        }
        if(s->move.landscape.valid && s->move.landscape.position==g->position)
            p->landscape=&s->move.landscape;
        if(e->town_found && e->town_id>=10) {p->feature=TERA_FEATURE_WRITING;p->writing_kind=2;}
        for(i=1;i<10;++i) if(i!=3 && i!=5 && i!=8 && e->world.extra_locations[i]==g->position) {
            p->ambient_npc=1;p->npc_variant=s->control.npc_variant;
        }
    } else if(g->world_id==11 && g->on_alfol) {
        p->kind=TERA_SCENE_ALFOL_SURFACE;
        p->variant=g->position==0 ? 0 : g->position==e->world.alfol_position ? 1 : 2;
    }
    else if(g->world_id<=10 && g->position>=0 && g->position<1100) {
        p->kind=g->world_id==10 ? TERA_SCENE_MEDUZ :
            g->world_id==9 ? TERA_SCENE_ALFOL_INTERIOR :
            !e->navigation_overlay ? TERA_SCENE_RUINS : TERA_SCENE_INTERIOR;
        cell=e->world.maps[g->position];p->variant=cell[6];content=cell[4];
        if(content>103 && content!=255) p->writing_kind=content>113 && e->navigation_overlay!=2 ? 2 : 1;
        heading=g->position-g->previous_position;
        if(heading==1) {left=1;front=0;right=3;}
        else if(heading==-1) {left=3;front=2;right=1;}
        else if(heading==10) {left=0;front=3;right=2;}
        else if(heading==-10) {left=2;front=1;right=0;}
        else {left=front=right=0;}
        p->heading_valid=heading==1 || heading==-1 || heading==10 || heading==-10;
        if(p->heading_valid) {p->left=cell[left];p->front=cell[front];p->right=cell[right];}
        if(g->world_id==10) p->feature=content==2 ? TERA_FEATURE_MEDITATION :
            content==3 ? TERA_FEATURE_INVOCATION : TERA_FEATURE_NONE;
        else if(g->world_id==9) p->feature=content==1 ? TERA_FEATURE_OFFICE :
            content==2 ? TERA_FEATURE_RADIATION : TERA_FEATURE_NONE;
        else if(g->world_id!=9) {
            if(content==100) p->feature=TERA_FEATURE_UP;
            else if(content==101) p->feature=TERA_FEATURE_DOWN;
            else if(content==103 || (e->navigation_overlay==2 && content==110)) p->feature=TERA_FEATURE_ALTAR;
            else if(content>103 && content!=255) p->feature=TERA_FEATURE_WRITING;
            else if(content<100) {
                p->feature=e->chest_open ? TERA_FEATURE_OPEN_CHEST : TERA_FEATURE_CHEST;
                if(e->chest_open && content>0 && content<=25) p->visible_item=content;
            }
        }
    }
    p->npc_mask=tera_recruit_available(&e->world);
}

static void inventory_prompt(const TeraInventorySession *s,TeraGamePrompt *p)
{
    static const TeraPromptKind kinds[]={TERA_PROMPT_MENU,TERA_PROMPT_CONFIRM,
        TERA_PROMPT_ACTOR,TERA_PROMPT_SLOT,TERA_PROMPT_ACTOR,TERA_PROMPT_MENU,
        TERA_PROMPT_TERMINAL,TERA_PROMPT_TERMINAL};
    p->source=TERA_SOURCE_INVENTORY;p->command=s->command;
    p->actor=s->subject;p->slot=s->slot;
    p->selecting_target=s->phase==TERA_INV_TARGET;
    if((unsigned)s->phase<sizeof kinds/sizeof kinds[0]) p->kind=kinds[s->phase];
}
/* Les invites sont une projection des lecteurs existants, jamais une liste
 * de commandes que le frontend devrait revalider lui-meme. */
static void prompt(const TeraGameSession *s,TeraGameView *v)
{
    TeraGamePrompt *p=&v->prompt;
    p->kind=s->phase>=TERA_GAME_COMPLETE ? TERA_PROMPT_TERMINAL : TERA_PROMPT_COMMAND;
    p->source=TERA_SOURCE_EXPLORATION;p->confirm_key='O';p->accepts_enter=1;
    if(v->creation) {
        static const TeraPromptKind kinds[]={TERA_PROMPT_NAME,TERA_PROMPT_GENDER,
            TERA_PROMPT_SPLIT,TERA_PROMPT_BONUS,TERA_PROMPT_KEEP,TERA_PROMPT_TERMINAL};
        const TeraCreationSession *c=v->creation;
        p->source=TERA_SOURCE_CREATION;
        p->kind=kinds[c->phase];p->remaining=c->bonuses_left;p->slot=c->split;
        if(c->phase==TERA_CREATE_SPLIT) p->pool=c->state.combat.encounter.attributes[0].stats[c->split];
        memcpy(p->input,c->name_buffer,sizeof p->input);return;
    }
    if(v->combat) {
        p->source=TERA_SOURCE_COMBAT;p->actor=v->combat->actor;
        p->kind=v->combat->phase==TERA_COMBAT_WORD ? TERA_PROMPT_WORD :
            v->combat->phase==TERA_COMBAT_CONFIRM ? TERA_PROMPT_CONFIRM : TERA_PROMPT_MENU;
        p->command=(unsigned char)v->combat->letter;memcpy(p->input,v->combat->word,sizeof p->input);return;
    }
    switch(s->phase) {
    case TERA_GAME_AMARANDE:
        p->source=TERA_SOURCE_AMARANDE;p->command=s->active.amarande.command;p->actor=s->active.amarande.shown_actor;
        p->choice=s->active.amarande.choice;
        p->kind=s->active.amarande.phase==TERA_AM_WORD ? TERA_PROMPT_WORD :
            s->active.amarande.phase==TERA_AM_PAUSE ? TERA_PROMPT_PAUSE :
            s->active.amarande.phase==TERA_AM_HALT ? (s->active.amarande.command=='U' ? TERA_PROMPT_PAUSE : TERA_PROMPT_MENU) : TERA_PROMPT_CONFIRM;
        memcpy(p->input,s->active.amarande.word,sizeof s->active.amarande.word);break;
    case TERA_GAME_NAVIGATION:
        p->source=TERA_SOURCE_NAVIGATION;p->command=s->active.navigation.key;
        if(s->active.navigation.phase==TERA_NAV_CHEST) {
            p->source=TERA_SOURCE_CHEST;p->command=s->active.navigation.chest.command;
            p->kind=s->active.navigation.chest.phase==TERA_CHEST_ACTOR ? TERA_PROMPT_ACTOR : TERA_PROMPT_CONFIRM;
        } else if(s->active.navigation.phase==TERA_NAV_GATE) {
            const TeraNavigationSession *n=&s->active.navigation;
            p->kind=TERA_PROMPT_GATE;
            p->confirm_key=n->state.world.maps[n->state.world.characters.combat.position][n->wall]==3 ? 'U' : 'O';
            p->accepts_enter=0;
        } else p->kind=s->active.navigation.phase==TERA_NAV_WORD ? TERA_PROMPT_WORD : TERA_PROMPT_CONFIRM;
        memcpy(p->input,s->active.navigation.word,sizeof p->input);break;
    case TERA_GAME_ALFOL:
        p->source=TERA_SOURCE_ALFOL;p->command=s->active.alfol.command;
        if(s->active.alfol.phase==TERA_ALFOL_GATE) {
            const TeraAlfolSession *a=&s->active.alfol;
            p->kind=TERA_PROMPT_GATE;p->confirm_key=a->state.world.maps[a->state.world.characters.combat.position][a->wall]==3 ? 'U' : 'O';
            p->accepts_enter=0;
        } else p->kind=s->active.alfol.phase==TERA_ALFOL_WORD ? TERA_PROMPT_WORD :
            s->active.alfol.phase==TERA_ALFOL_REBOARD ? TERA_PROMPT_PAUSE : TERA_PROMPT_CONFIRM;
        if(s->active.alfol.phase==TERA_ALFOL_MEDIA) {p->accepts_enter=0;p->pool=1000;}
        if(s->active.alfol.phase==TERA_ALFOL_WORD) memcpy(p->input,s->active.alfol.word,sizeof p->input);
        break;
    case TERA_GAME_MEDUZ:
        p->source=TERA_SOURCE_MEDUZ;p->command=s->active.meduz.command;
        p->kind=s->active.meduz.phase==TERA_MEDUZ_WORD || s->active.meduz.phase==TERA_MEDUZ_ENTRY_WORD ? TERA_PROMPT_WORD : TERA_PROMPT_CONFIRM;
        memcpy(p->input,s->active.meduz.word,sizeof s->active.meduz.word);break;
    case TERA_GAME_SHOP:
        p->source=TERA_SOURCE_SHOP;p->command=s->active.shop.command;p->actor=s->active.shop.actor;
        p->remaining=s->active.shop.first_menu; /* Le premier ESC ne termine pas REPEAT. */
        p->kind=s->active.shop.phase==TERA_SHOP_CONFIRM ? TERA_PROMPT_CONFIRM :
            s->active.shop.phase==TERA_SHOP_ACTOR ? TERA_PROMPT_ACTOR :
            s->active.shop.phase==TERA_SHOP_SELL ? TERA_PROMPT_SLOT : TERA_PROMPT_MENU;break;
    case TERA_GAME_DIALOGUE:
        p->source=TERA_SOURCE_DIALOGUE;p->command=s->active.dialogue.choice ? s->active.dialogue.choice : s->active.dialogue.command;
        p->kind=s->active.dialogue.phase==TERA_DIALOGUE_MENU ? TERA_PROMPT_MENU : TERA_PROMPT_CONFIRM;
        if(s->active.dialogue.phase==TERA_DIALOGUE_TRANSFER) inventory_prompt(&s->active.dialogue.transfer,p);
        break;
    case TERA_GAME_INVENTORY:inventory_prompt(&s->active.inventory,p);break;
    case TERA_GAME_SPACE:
        p->source=TERA_SOURCE_SPACE;
        if(s->active.space.phase==TERA_SPACE_DIGIT) p->pool=50;
        if(s->active.space.phase==TERA_SPACE_FUEL) p->accepts_enter=0;
        p->kind=s->active.space.phase==TERA_SPACE_SHOT ? TERA_PROMPT_SHOT :
            s->active.space.phase==TERA_SPACE_DIGIT ? TERA_PROMPT_DIGIT : TERA_PROMPT_CONFIRM;break;
    case TERA_GAME_LIFE:p->source=TERA_SOURCE_LIFE;p->kind=TERA_PROMPT_PAUSE;break;
    case TERA_GAME_ENDING:p->source=TERA_SOURCE_ENDING;p->kind=TERA_PROMPT_PAUSE;break;
    default:break;
    }
}

/* Selectionner la sous-session qui attend vraiment une lecture. C'est la
 * meme frontiere publique que tera_game_key, pas une seconde orchestration. */
int tera_game_view(const TeraGameSession *s,TeraGameView *v)
{
    const TeraDaySession *day=NULL;
    if(!s || !v) return 0;
    memset(v,0,sizeof *v);
    v->phase=s->phase;v->subphase=-1;v->exploration=&s->state;
    switch(s->phase) {
    case TERA_GAME_CREATE:v->creation=&s->active.creation;break;
    case TERA_GAME_DAY:day=&s->active.day;v->subphase=(int)day->phase;break;
    case TERA_GAME_AMARANDE:
        v->exploration=&s->active.amarande.state;v->subphase=(int)s->active.amarande.phase;
        if(s->active.amarande.phase==TERA_AM_COMBAT) v->combat=&s->active.amarande.combat;
        if(s->active.amarande.phase==TERA_AM_DAY) day=&s->active.amarande.day;
        break;
    case TERA_GAME_NAVIGATION:
        v->exploration=&s->active.navigation.state;v->subphase=(int)s->active.navigation.phase;
        if(s->active.navigation.phase==TERA_NAV_DAY) day=&s->active.navigation.day;
        break;
    case TERA_GAME_ALFOL:
        v->exploration=&s->active.alfol.state;v->subphase=(int)s->active.alfol.phase;
        if(s->active.alfol.phase==TERA_ALFOL_DAY) {
            if(s->active.alfol.fixed_combat) v->combat=&s->active.alfol.combat;
            else day=&s->active.alfol.day;
        }
        break;
    case TERA_GAME_MEDUZ:
        v->exploration=&s->active.meduz.state;v->subphase=(int)s->active.meduz.phase;
        if(s->active.meduz.phase==TERA_MEDUZ_DAY) {
            if(s->active.meduz.fixed_combat) v->combat=&s->active.meduz.combat;
            else day=&s->active.meduz.day;
        }
        break;
    case TERA_GAME_SHOP:
        v->exploration=&s->active.shop.state;v->subphase=(int)s->active.shop.phase;break;
    case TERA_GAME_DIALOGUE:
        v->exploration=&s->active.dialogue.state;v->subphase=(int)s->active.dialogue.phase;break;
    case TERA_GAME_LIFE:
        v->exploration=&s->active.life.state;v->subphase=(int)s->active.life.phase;
        if(s->active.life.phase==TERA_LIFE_CREATE) v->creation=&s->active.life.creation;
        break;
    case TERA_GAME_ENDING:v->ending=&s->active.ending;v->exploration=&v->ending->state;break;
    case TERA_GAME_COMPLETE:
        if(s->completed_from==TERA_GAME_ENDING) v->ending=&s->active.ending;
        break;
    default:break;
    }
    if(day) {
        v->exploration=&day->state;
        if(day->phase==TERA_DAY_COMBAT) v->combat=&day->combat;
    }
    v->characters=&v->exploration->world.characters;
    if(s->phase==TERA_GAME_INVENTORY) {
        v->characters=&s->active.inventory.state;v->subphase=(int)s->active.inventory.phase;
    } else if(s->phase==TERA_GAME_SPACE) {
        v->space=&s->active.space;v->characters=&v->space->state;v->subphase=(int)v->space->phase;
    } else if(s->phase==TERA_GAME_DIALOGUE && s->active.dialogue.phase==TERA_DIALOGUE_TRANSFER)
        v->characters=&s->active.dialogue.transfer.state;
    else if(s->phase==TERA_GAME_NAVIGATION && s->active.navigation.phase==TERA_NAV_CHEST) {
        v->exploration=&s->active.navigation.chest.state;
        v->characters=&v->exploration->world.characters;
    }
    if(v->creation) {
        v->characters=&v->creation->state;v->subphase=(int)v->creation->phase;
    }
    prompt(s,v);scene(s,v);return 1;
}
