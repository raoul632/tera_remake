#include "tera/game.h"
#include <string.h>

/* TERAMAIN et les specs des overlays fournissent les regles ; ce module
 * assure seulement leur ordre et la reintegration des etats. Les anciens
 * outils Python servent de trace des raccordements, pas d'oracle de jeu. */
static int objectives_complete(const TeraWorldState *w)
{
    return w->tasks_done[0] && w->tasks_done[1] && w->tasks_done[2] && w->tasks_done[3];
}
static int16_t word16(int32_t value)
{
    uint32_t bits=(uint32_t)value&65535u;
    return (int16_t)(bits<32768u ? (int32_t)bits : (int32_t)bits-65536);
}
static int fail(TeraGameSession *s)
{
    s->phase=TERA_GAME_ERROR;return 0;
}
/* C4210 precede une nouvelle lecture semantique. Une touche qui abrege
 * une animation ne devient pas automatiquement l'action du combat.
 * tera_game_key fournit deja une NOUVELLE touche acceptee, pas le tampon
 * entier de l'application. Les tirs spatiaux ont leurs propres purges. */
static void prepare_read(TeraGameSession *s)
{
    if(s->phase<TERA_GAME_COMPLETE && !(s->phase==TERA_GAME_SPACE && s->active.space.phase==TERA_SPACE_SHOT))
        tera_input_boundary(s->state.world.characters.combat.input,TERA_INPUT_PURGE,0);
}

/* Sortir de l'overlay AVANT condoleances/succession. Le retour de Meduz
 * inclut sept jours et les tirages de voyage ; Alfol conserve OnAlfol. */
static int leave_dead_overlay(TeraGameSession *s)
{
    unsigned world=s->state.world.characters.combat.world_id;
    if(world==10) {
        if(!tera_meduz_resume(&s->active.meduz,&s->state,s->control.meduz_backup_position)) return fail(s);
        s->state=s->active.meduz.state;
    } else if(world==9) {
        if(!tera_alfol_leave(&s->active.alfol,&s->state,s->control.status_actor)) return fail(s);
        s->state=s->active.alfol.state;s->control.status_actor=(int16_t)s->active.alfol.status_actor;
    } else if(world!=11) {
        if(!tera_navigation_leave(&s->state,s->state.city_backup_position)) return fail(s);
        tera_places_present(&s->state,1);
    }
    return 1;
}

/* Apres UNE commande obligatoire, le test UNTIL redevient permis meme si
 * la commande etait residente (inventaire/dialogue) et non un deplacement. */
static int finish_operation(TeraGameSession *s,int resident)
{
    TeraCombatGame *g=&s->state.world.characters.combat;
    if(s->after_mandatory && !g->encounter.party_available) {
        if(!leave_dead_overlay(s)) return 0;
    } else if(resident && g->world_id==10) {
        if(!tera_meduz_resume(&s->active.meduz,&s->state,s->control.meduz_backup_position)) return fail(s);
        s->state=s->active.meduz.state;
    }
    s->after_mandatory=0;s->phase=TERA_GAME_READY;return 1;
}

/* Reprendre tant qu'il n'y a pas de lecture. Chaque branche publie son
 * etat avant de reutiliser l'union ; aucun pointeur vers une ancienne
 * sous-session ne survit au changement de phase. */
static void settle(TeraGameSession *s)
{
    for(;;) {
        TeraCombatGame *g=&s->state.world.characters.combat;
        switch(s->phase) {
        case TERA_GAME_CREATE:
            if(s->active.creation.phase!=TERA_CREATE_DONE) return;
            if(!tera_world_apply_creation(&s->state.world,&s->active.creation)) {fail(s);return;}
            if(!finish_operation(s,0)) return;
            break;
        case TERA_GAME_DAY:
            if(s->active.day.phase==TERA_DAY_ERROR) {fail(s);return;}
            if(s->active.day.phase!=TERA_DAY_DONE) return;
            s->state=s->active.day.state;
            tera_places_present(&s->state,0);
            if(!finish_operation(s,0)) return;
            break;
        case TERA_GAME_AMARANDE:
            if(s->active.amarande.phase==TERA_AM_ERROR) {fail(s);return;}
            if(s->active.amarande.phase!=TERA_AM_DONE) return;
            s->state=s->active.amarande.state;
            s->control.status_actor=s->active.amarande.status_actor;
            if(s->active.amarande.quit_requested) {s->phase=TERA_GAME_QUIT;return;}
            if(!finish_operation(s,0)) return;
            break;
        case TERA_GAME_NAVIGATION:
            if(s->active.navigation.phase==TERA_NAV_ERROR) {fail(s);return;}
            if(s->active.navigation.phase!=TERA_NAV_DONE) return;
            s->state=s->active.navigation.state;
            if(s->active.navigation.mandatory_temple_command) s->control.mandatory_command=1;
            if(g->world_id<9 && !s->control.mandatory_command &&
               (!g->encounter.party_available || (!g->position && !g->world_id && !s->state.navigation_overlay))) {
                if(!tera_navigation_leave(&s->state,s->state.city_backup_position)) {fail(s);return;}
            }
            /* Tout retour d'overlay redessine la facade, meme apres une
             * commande immobile. Un refus d'entree ne la redessine pas. */
            if(g->world_id==11 && (!s->active.navigation.enter_city || s->active.navigation.moved))
                tera_places_present(&s->state,1);
            if(!finish_operation(s,0)) return;
            break;
        case TERA_GAME_ALFOL:
            if(s->active.alfol.phase==TERA_ALFOL_ERROR) {fail(s);return;}
            if(s->active.alfol.phase!=TERA_ALFOL_DONE && s->active.alfol.phase!=TERA_ALFOL_RETURN) return;
            s->state=s->active.alfol.state;s->control.status_actor=(int16_t)s->active.alfol.status_actor;
            if(s->active.alfol.initial_render && g->world_id==9 && !g->encounter.party_available)
                s->control.mandatory_command=2;
            if(s->active.alfol.phase==TERA_ALFOL_RETURN && g->encounter.party_available) {
                if(!tera_space_travel_start(&s->active.space,&s->state.world.characters,1,
                    s->control.pirates,s->control.space_backup_position)) {fail(s);return;}
                s->phase=TERA_GAME_SPACE;break;
            }
            if(!finish_operation(s,0)) return;
            break;
        case TERA_GAME_MEDUZ:
            if(s->active.meduz.phase==TERA_MEDUZ_ERROR) {fail(s);return;}
            if(s->active.meduz.phase!=TERA_MEDUZ_DONE) return;
            s->state=s->active.meduz.state;s->control.meduz_backup_position=s->active.meduz.backup_position;
            if(s->active.meduz.mandatory_command) s->control.mandatory_command=3;
            if(!finish_operation(s,0)) return;
            break;
        case TERA_GAME_SHOP:
            if(s->active.shop.phase==TERA_SHOP_ERROR) {fail(s);return;}
            if(s->active.shop.phase!=TERA_SHOP_DONE) return;
            s->state=s->active.shop.state;
            if(!finish_operation(s,1)) return;
            break;
        case TERA_GAME_SPACE: {
            int returning,departed;
            if(s->active.space.phase==TERA_SPACE_ERROR) {fail(s);return;}
            if(s->active.space.phase<TERA_SPACE_DONE) return;
            s->state.world.characters=s->active.space.state;
            s->control.pirates=s->active.space.pirates;
            s->control.space_backup_position=s->active.space.backup_position;
            if(s->active.space.phase==TERA_SPACE_HALTED) {s->phase=TERA_GAME_HALTED;return;}
            returning=s->active.space.returning;departed=s->active.space.departed;
            if(!returning && departed) {
                if(!tera_alfol_start(&s->active.alfol,&s->state,0,1,s->control.status_actor,s->observe,s->observer_context)) {fail(s);return;}
                s->phase=TERA_GAME_ALFOL;break;
            }
            if(returning) tera_places_present(&s->state,1);
            if(!finish_operation(s,0)) return;
            break;
        }
        case TERA_GAME_DIALOGUE:
            if(s->active.dialogue.phase==TERA_DIALOGUE_ERROR) {fail(s);return;}
            if(s->active.dialogue.phase!=TERA_DIALOGUE_DONE) return;
            s->state=s->active.dialogue.state;
            if(!finish_operation(s,1)) return;
            break;
        case TERA_GAME_INVENTORY:
            if(s->active.inventory.phase!=TERA_INV_DONE) return;
            s->state.world.characters=s->active.inventory.state;
            if(!finish_operation(s,1)) return;
            break;
        case TERA_GAME_LIFE:
            if(s->active.life.phase==TERA_LIFE_ERROR) {fail(s);return;}
            if(s->active.life.phase==TERA_LIFE_COMPLETE) {s->state=s->active.life.state;s->completed_from=TERA_GAME_LIFE;s->phase=TERA_GAME_COMPLETE;return;}
            if(s->active.life.phase!=TERA_LIFE_READY) return;
            s->state=s->active.life.state;
            if(!finish_operation(s,0)) return;
            break;
        case TERA_GAME_ENDING:
            if(s->active.ending.runtime_error) {fail(s);return;}
            if(!s->active.ending.complete) return;
            s->state=s->active.ending.state;s->completed_from=TERA_GAME_ENDING;s->phase=TERA_GAME_COMPLETE;return;
        case TERA_GAME_READY:
            /* Les objectifs ne terminent pas la partie dans un overlay.
             * Sur Amarande, la fin prime sur la recreation du heros mort. */
            if(g->world_id==11 && !g->on_alfol && objectives_complete(&s->state.world)) {
                if(!tera_ending_start(&s->active.ending,&s->state)) {fail(s);return;}
                s->phase=TERA_GAME_ENDING;break;
            }
            if(!g->encounter.party_available && !s->control.mandatory_command) {
                if(!leave_dead_overlay(s)) return;
                if(objectives_complete(&s->state.world)) {
                    if(!tera_ending_start(&s->active.ending,&s->state)) {fail(s);return;}
                    s->phase=TERA_GAME_ENDING;
                } else {
                    if(!tera_life_start(&s->active.life,&s->state)) {fail(s);return;}
                    s->phase=TERA_GAME_LIFE;
                }
                break;
            }
            if(g->world_id==11 && !g->on_alfol && g->encounter.party_available &&
               (s->state.world.characters.arrival_pending || g->encounter.fled)) {
                /* TERAMAIN A74F/AFD1 : pas nord apres Position--, pas une
                 * teleportation. Les montagnes et le bouclage restent actifs. */
                s->state.world.characters.arrival_pending=0;g->encounter.fled=0;
                g->position=word16(g->position-1);
                if(!tera_exploration_move(&s->state,'8',&s->move)) {fail(s);return;}
                if(s->move.npc_present) s->control.npc_variant=s->move.npc_variant;
                if(!s->move.moved) {tera_places_present(&s->state,0);return;}
                if(!tera_day_start(&s->active.day,&s->state,s->observe,s->observer_context)) {fail(s);return;}
                s->phase=TERA_GAME_DAY;break;
            }
            return;
        default:return;
        }
    }
}

/* Validation structurelle du routage ; les modules gardent la validation
 * de leurs donnees metier. Un contexte invalide ne doit pas ecraser s. */
static int valid_start(const TeraExplorationState *state,const TeraGameContext *c)
{
    const TeraCombatGame *g;
    if(!state) return 0;
    g=&state->world.characters.combat;
    if(g->world_id>11 || g->difficulty>9 || g->position<0 ||
       g->position>(g->world_id==11 ? (g->on_alfol ? 99 : 1899) : 1099)) return 0;
    if((g->world_id==9 && !g->on_alfol) || (g->world_id==10 && g->on_alfol)) return 0;
    if((g->on_alfol && g->world_id!=9 && g->world_id!=11) ||
       (g->world_id<9 && g->position>899) ||
       (g->world_id==9 && (g->position<900 || g->position>999)) ||
       (g->world_id==10 && g->position<1000)) return 0;
    if((g->world_id==10 || g->on_alfol) && !c) return 0;
    if(c && (c->npc_variant>2 || c->mandatory_command>3 ||
        ((c->status_actor<0 || c->status_actor>9) && c->status_actor!=16))) return 0;
    if(c && ((c->mandatory_command==1 && g->world_id!=8) ||
        (c->mandatory_command==2 && g->world_id!=9) ||
        (c->mandatory_command==3 && g->world_id!=10))) return 0;
    if(c && c->mandatory_command==1 && state->navigation_overlay!=2) return 0;
    if(g->world_id<9 && (state->city_backup_position<1000 || state->city_backup_position>1899)) return 0;
    if(g->world_id==10 && (c->meduz_backup_position<1000 || c->meduz_backup_position>1899)) return 0;
    if(g->on_alfol && (c->space_backup_position<1000 || c->space_backup_position>1899)) return 0;
    return 1;
}
int tera_game_start(TeraGameSession *s,const TeraExplorationState *state,
    const TeraGameContext *control,TeraCombatObserver observer,void *context)
{
    if(!s || !valid_start(state,control)) return 0;
    memset(s,0,sizeof *s);s->state=*state;s->observe=observer;s->observer_context=context;
    s->phase=TERA_GAME_READY;
    if(control) s->control=*control;
    else {s->control.status_actor=9;s->control.pirates=4;}
    settle(s);prepare_read(s);return 1;
}
int tera_game_create(TeraGameSession *s,const TeraExplorationState *state,
    TeraCombatObserver observer,void *context)
{
    TeraCreationSession creation;
    if(!s || !state || !tera_creation_start(&creation,&state->world.characters)) return 0;
    memset(s,0,sizeof *s);s->state=*state;s->observe=observer;s->observer_context=context;
    s->control.status_actor=9;s->control.pirates=4;
    s->active.creation=creation;s->phase=TERA_GAME_CREATE;prepare_read(s);return 1;
}

int tera_game_restore_ready(TeraGameSession *s,const TeraExplorationState *state,
    const TeraGameContext *control,const TeraInputSource *input,
    TeraCombatObserver observer,void *context)
{
    TeraExplorationState copy;
    TeraGameContext globals;
    const TeraCombatGame *g;
    if(!s || !control || !valid_start(state,control)) return 0;
    g=&state->world.characters.combat;
    if(!g->encounter.party_available && !control->mandatory_command) return 0;
    if(g->world_id==11 && !g->on_alfol &&
       (objectives_complete(&state->world) || state->world.characters.arrival_pending || g->encounter.fled)) return 0;
    /* Copier avant memset autorise aussi une restauration sur la session
     * source ; aucun pointeur vers un ancien service ne doit survivre. */
    copy=*state;globals=*control;copy.world.characters.combat.input=input;
    copy.world.characters.combat.presentation=NULL;
    memset(s,0,sizeof *s);s->state=copy;s->control=globals;
    s->phase=TERA_GAME_READY;s->observe=observer;s->observer_context=context;
    return 1;
}

/* Choisir un seul module pour la commande. Les touches des menus suivants
 * passent directement a ce module, jamais dans ce routage principal. */
static int dispatch_command(TeraGameSession *s,unsigned key)
{
    TeraCombatGame *g=&s->state.world.characters.combat;
    int ok=0;
    if(key=='?' || key=='+' || key=='H') {
        ok=tera_amarande_start(&s->active.amarande,&s->state,key,s->control.status_actor,s->observe,s->observer_context);
        if(ok) s->phase=TERA_GAME_AMARANDE;
    } else if(g->world_id==10 && strchr("2468ERIDMLOP",(int)key)) {
        ok=tera_meduz_start(&s->active.meduz,&s->state,key,0,0,1,s->control.meduz_backup_position,s->observe,s->observer_context);
        if(ok) s->phase=TERA_GAME_MEDUZ;
    } else if(g->on_alfol && strchr("2468ERIDMLOP",(int)key)) {
        ok=tera_alfol_start(&s->active.alfol,&s->state,key,0,s->control.status_actor,s->observe,s->observer_context);
        if(ok) s->phase=TERA_GAME_ALFOL;
    } else if(g->world_id!=11 && strchr("2468DMLIOP",(int)key)) {
        ok=tera_navigation_start(&s->active.navigation,&s->state,key,0,s->state.city_backup_position,s->observe,s->observer_context);
        if(ok) s->phase=TERA_GAME_NAVIGATION;
    } else if(key=='N' || key=='S') {
        ok=tera_dialogue_start(&s->active.dialogue,&s->state,key);
        if(ok) s->phase=TERA_GAME_DIALOGUE;
    } else if(key=='A' || key=='B') {
        ok=tera_inventory_start(&s->active.inventory,&s->state.world.characters,key);
        if(ok) s->phase=TERA_GAME_INVENTORY;
    } else if(g->world_id==11 && !g->on_alfol && strchr("2468",(int)key)) {
        TeraLandscape previous=s->move.landscape;
        ok=tera_exploration_move(&s->state,key,&s->move);
        if(ok && !s->move.moved && previous.valid && previous.position==g->position)
            s->move.landscape=previous; /* Un obstacle ne redessine pas le decor. */
        if(ok && s->move.npc_present) s->control.npc_variant=s->move.npc_variant;
        if(ok && s->move.moved) {
            if(!tera_day_start(&s->active.day,&s->state,s->observe,s->observer_context)) {fail(s);return 1;}
            s->phase=TERA_GAME_DAY;
        }
    } else if(g->world_id==11 && !g->on_alfol) {
        if(s->state.town_found && s->state.town_id==3 && key=='M') {
            ok=tera_meduz_start(&s->active.meduz,&s->state,key,1,0,1,g->position,s->observe,s->observer_context);
            if(ok) s->phase=TERA_GAME_MEDUZ;
        } else if(key=='E' && s->state.town_found && s->state.town_id==0) {
            ok=tera_shop_start(&s->active.shop,&s->state);
            if(ok) {s->active.shop.phase=TERA_SHOP_CONFIRM;s->active.shop.command='E';s->phase=TERA_GAME_SHOP;}
        } else if(key=='E' && s->state.town_found && s->state.town_id==1) {
            ok=tera_space_travel_start(&s->active.space,&s->state.world.characters,0,s->control.pirates,g->position);
            if(ok) s->phase=TERA_GAME_SPACE;
        } else if(key=='E' && s->state.town_found && (s->state.town_id==2 || s->state.town_id==4)) {
            ok=tera_navigation_enter(&s->active.navigation,&s->state,s->observe,s->observer_context);
            if(ok) s->phase=TERA_GAME_NAVIGATION;
        } else if(strchr("CEHI#DFLMOPU",(int)key)) {
            ok=tera_amarande_start(&s->active.amarande,&s->state,key,s->control.status_actor,s->observe,s->observer_context);
            if(ok) {s->active.amarande.extra_variant=s->control.npc_variant;s->phase=TERA_GAME_AMARANDE;}
        }
    }
    return ok;
}
int tera_game_key(TeraGameSession *s,unsigned key)
{
    int ok=0;
    if(!s || key>255 || s->phase>=TERA_GAME_COMPLETE) return 0;
    if(s->phase==TERA_GAME_READY) {
        /* strchr accepte aussi NUL : l'exclure avant les tables de touches. */
        if(!key) return 0;
        if(key>='a' && key<='z') key-=32;
        /* Un refus de routage ne doit modifier ni le monde ni les globals,
         * meme si un module a initialise sa session avant de refuser. */
        TeraGameSession next=*s;
        if(!dispatch_command(&next,key)) return 0;
        next.after_mandatory=next.control.mandatory_command;
        next.control.mandatory_command=0;*s=next;
        if(s->phase==TERA_GAME_READY && !finish_operation(s,0)) return 1;
    } else {
        switch(s->phase) {
        case TERA_GAME_CREATE:ok=tera_creation_key(&s->active.creation,key);break;
        case TERA_GAME_DAY:ok=tera_day_key(&s->active.day,key);break;
        case TERA_GAME_AMARANDE:ok=tera_amarande_key(&s->active.amarande,key);break;
        case TERA_GAME_NAVIGATION:ok=tera_navigation_key(&s->active.navigation,key);break;
        case TERA_GAME_ALFOL:ok=tera_alfol_key(&s->active.alfol,key);break;
        case TERA_GAME_MEDUZ:ok=tera_meduz_key(&s->active.meduz,key);break;
        case TERA_GAME_SHOP:ok=tera_shop_key(&s->active.shop,key);break;
        case TERA_GAME_SPACE:ok=tera_space_key(&s->active.space,key);break;
        case TERA_GAME_DIALOGUE:ok=tera_dialogue_key(&s->active.dialogue,key);break;
        case TERA_GAME_INVENTORY:ok=tera_inventory_key(&s->active.inventory,key);break;
        case TERA_GAME_LIFE:ok=tera_life_key(&s->active.life,key);break;
        case TERA_GAME_ENDING:ok=tera_ending_key(&s->active.ending,key);break;
        default:break;
        }
        if(!ok) return 0;
    }
    settle(s);prepare_read(s);return 1;
}
int tera_game_space_input_at(TeraGameSession *s,unsigned elapsed_ms,int key)
{
    if(!s || s->phase!=TERA_GAME_SPACE ||
       !tera_space_input_at(&s->active.space,elapsed_ms,key)) return 0;
    settle(s);prepare_read(s);return 1;
}
