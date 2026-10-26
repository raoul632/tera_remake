/* Parcours de regression, pas une interface de jeu. Le conducteur lit les
 * cartes pour planifier ; APRES generation, seule tera_game_key peut
 * modifier le jeu. Les ressources originales restent locales/read-only. */
#ifdef NDEBUG
#undef NDEBUG
#endif
#include "tera/game.h"
#include "tera/save.h"
#ifdef TERA_BINARY_CAMPAIGN
#include "teramain_oracle.h"
#endif
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

/* Reprendre le plan APRES une succession reelle, jamais revenir a un
 * snapshot anterieur. Le monde et le RNG ne sont pas reinitialises. */
static jmp_buf succession_checkpoint;

typedef struct {
    TeraGameSession game;
    unsigned inputs,commands,wins,escapes,deaths,objectives,visited;
    int avoid_fights;
    uint32_t digest;
    int resume_enabled;
    unsigned checkpoints,checkpoint_worlds;
    TeraPresentationSink presentation;
    unsigned presentation_events,presentation_objectives,presentation_zones;
} Campaign;
static void presentation_event(void *context,const TeraPresentationEvent *event)
{
    Campaign *p=context;++p->presentation_events;
    if(event->kind==TERA_PRESENT_OBJECTIVE) {
        assert(event->data.objective.id<4);
        assert(!(p->presentation_objectives&(1u<<event->data.objective.id)));
        p->presentation_objectives|=1u<<event->data.objective.id;
    }
    if(event->kind==TERA_PRESENT_ZONE) ++p->presentation_zones;
}
static void require(int condition,const char *message)
{
    if(!condition) {fprintf(stderr,"campaign: %s\n",message);exit(1);}
}
static void combat_event(void *context,const TeraCombatEvent *e)
{
    Campaign *p=context;
    if(e->kind==TERA_EVENT_END && e->value) ++p->wins;
    if(e->kind==TERA_EVENT_ESCAPE) ++p->escapes;
}
/* Lire exactement la ressource attendue, sans copier son contenu dans le
 * depot ni dependre de l'endianness/native packing du compilateur. */
static int read_resource(const char *dir,const char *name,unsigned char *bytes,size_t size)
{
    char path[2048];FILE *f;int exact,length;
    length=snprintf(path,sizeof path,"%s/%s",dir,name);
    require(length>0 && (size_t)length<sizeof path,"resource path");
    f=fopen(path,"rb");if(!f) return 0;
    exact=fread(bytes,1,size,f)==size && fgetc(f)==EOF;fclose(f);
    require(exact,"invalid original resource length");return 1;
}
static int prepare_world(TeraExplorationState *s,TeraNameVariants *names,const char *dir,uint32_t seed)
{
    unsigned char raw_names[270],raw_monsters[264];unsigned group,id,j;
    TeraWorldReport maps,places;
    if(!read_resource(dir,"NOMPERS",raw_names,sizeof raw_names) ||
       !read_resource(dir,"VALMON",raw_monsters,sizeof raw_monsters)) return 0;
    memset(s,0,sizeof *s);memset(names,0,sizeof *names);
    for(group=0;group<3;++group) for(id=1;id<10;++id) {
        unsigned offset=(group*9+id-1)*10,length=raw_names[offset];
        char *out=group ? names->names[group-1][id] : s->world.characters.names[id];
        require(length<=9,"Pascal name length");memcpy(out,raw_names+offset+1,length);
    }
    for(id=0;id<22;++id) for(j=0;j<6;++j) {
        unsigned offset=12*id+2*j,bits=raw_monsters[offset]+256u*raw_monsters[offset+1];
        s->world.characters.combat.monster_values[id][j]=(int16_t)(bits<32768 ? (int)bits : (int)bits-65536);
    }
    s->world.characters.combat.random.seed=seed;s->world.placement_result_residue=-211;
    require(tera_world_generate_maps(&s->world,0,2000000,&maps),"map generation");
    require(tera_world_initialize_places(&s->world,names,2000000,&places),"places generation");
    printf("seed=%lu generation draws=%lu attempts=%lu\n",(unsigned long)seed,
        (unsigned long)(maps.random_calls+places.random_calls),(unsigned long)maps.map_attempts);fflush(stdout);
    return 1;
}
/* Test d'integration de persistance : fermer un flux apres restauration,
 * rebrancher l'observateur et continuer le vrai parcours. Ni rechargement
 * d'une ancienne partie ni reinitialisation des objectifs a la defaite. */
static void checkpoint(Campaign *p)
{
    TeraGameSession restored;FILE *file=tmpfile();
    unsigned char before[TERA_SAVE_CAPACITY],after[TERA_SAVE_CAPACITY];size_t n,m;
    unsigned world=p->game.state.world.characters.combat.world_id;
    require(file!=NULL,"checkpoint temporary file");
    require(tera_save_encode(&p->game,before,sizeof before,&n)==TERA_SAVE_OK,"checkpoint encode");
    require(tera_save_write(file,&p->game)==TERA_SAVE_OK,"checkpoint write");
    require(fseek(file,0,SEEK_SET)==0,"checkpoint rewind");
    require(tera_save_read(file,&restored,p->game.state.world.characters.combat.input,combat_event,p)==TERA_SAVE_OK,"checkpoint read");
    require(fclose(file)==0,"checkpoint close");
    require(tera_save_encode(&restored,after,sizeof after,&m)==TERA_SAVE_OK,"checkpoint reencode");
    require(n==m && memcmp(before,after,n)==0,"checkpoint preserves all persistent fields");
    p->game=restored;++p->checkpoints;p->checkpoint_worlds|=1u<<world;
    if(p->presentation.emit) tera_game_set_presentation(&p->game,&p->presentation);
}
/* Tous les effets du parcours passent ici. Le digest est une trace des
 * entrees, frontieres et graines ; il n'est jamais utilise pour jouer. */
static void press(Campaign *p,unsigned key)
{
    TeraGameSession *s=&p->game;
    unsigned i,flags=0,count=0;
    if(++p->inputs>200000) {fprintf(stderr,"budget phase=%d world=%u pos=%d wins=%u\n",s->phase,s->state.world.characters.combat.world_id,s->state.world.characters.combat.position,p->wins);exit(1);}
    if(s->phase==TERA_GAME_READY) {
        if(p->resume_enabled && (!(p->checkpoint_worlds&(1u<<s->state.world.characters.combat.world_id)) || p->commands%97==0)) checkpoint(p);
        ++p->commands;
    }
#ifdef TERA_CAMPAIGN_BEFORE
    TERA_CAMPAIGN_BEFORE(s,key);
#endif
    if(!tera_game_key(s,key)) {fprintf(stderr,"refused key=%u phase=%d world=%u pos=%d\n",key,s->phase,s->state.world.characters.combat.world_id,s->state.world.characters.combat.position);exit(1);}
#ifdef TERA_CAMPAIGN_AFTER
    TERA_CAMPAIGN_AFTER(s,key);
#endif
#ifdef TERA_BINARY_CAMPAIGN
    teramain_oracle_key(key,s);
#endif
    require(s->phase!=TERA_GAME_ERROR,"core ERROR");
    for(i=0;i<4;++i) if(s->state.world.tasks_done[i]) {flags|=1u<<i;++count;}
    require((flags&p->objectives)==p->objectives,"objective erased during route");
    require(s->state.world.tasks_counter==(int)count,"objective counter inconsistent");
    p->objectives=flags;p->visited|=1u<<s->state.world.characters.combat.world_id;
    p->digest=(p->digest^key)*UINT32_C(16777619);
    p->digest=(p->digest^(unsigned)s->phase)*UINT32_C(16777619);
    p->digest=(p->digest^s->state.world.characters.combat.random.seed)*UINT32_C(16777619);
}
static void type(Campaign *p,const char *text)
{
    while(*text) press(p,(unsigned char)*text++);
}
static TeraCombatSession *waiting_combat(TeraGameSession *s)
{
    switch(s->phase) {
    case TERA_GAME_DAY:return &s->active.day.combat;
    case TERA_GAME_NAVIGATION:return s->active.navigation.phase==TERA_NAV_DAY ? &s->active.navigation.day.combat : NULL;
    case TERA_GAME_ALFOL:return s->active.alfol.phase==TERA_ALFOL_DAY ? (s->active.alfol.fixed_combat ? &s->active.alfol.combat : &s->active.alfol.day.combat) : NULL;
    case TERA_GAME_MEDUZ:return s->active.meduz.phase==TERA_MEDUZ_DAY ? (s->active.meduz.fixed_combat ? &s->active.meduz.combat : &s->active.meduz.day.combat) : NULL;
    case TERA_GAME_AMARANDE:
        if(s->active.amarande.phase==TERA_AM_COMBAT) return &s->active.amarande.combat;
        return s->active.amarande.phase==TERA_AM_DAY ? &s->active.amarande.day.combat : NULL;
    default:return NULL;
    }
}
/* Choisir une action selon sa moyenne sur TOUS les facteurs possibles,
 * sans lire, simuler ni choisir le prochain tirage RNG de la partie. */
static unsigned choose_attack(const TeraCombatSession *c,int avoid)
{
    static const char letters[7]={'L','T','A','D','P','C','J'};
    static const unsigned actions[7]={1,2,3,4,5,6,9};
    const TeraCombatGame *g=&c->game;unsigned k,a,b,best=g->encounter.monster_type==21 ? '?' : 'F';long maximum=0;
    if(((avoid==1 || (avoid==2 && (g->encounter.reaction.reaction_category==3 || g->encounter.reaction.reaction_category==4))) && g->encounter.monster_type!=21) || c->round_number>60) return 'F';
    for(k=0;k<7;++k) {
        unsigned required=k==0 ? 15 : k==1 ? 18 : k==2 ? g->helpers[3] :
            k==3 ? g->helpers[0] : k==4 ? g->helpers[2] : k==5 ? g->helpers[1] : 0;
        unsigned bonus=required ? tera_combat_equipment_bonus(g->equipment[c->actor],required) : 0;
        long sum=0;
        for(a=3;a<=17;++a) for(b=3;b<=17;++b) {
            int16_t raw;TeraHeroAttackResult hit;
            require(tera_combat_action(&g->encounter.attributes[c->actor],actions[k],bonus,a,b,&raw),"attack estimate");
            require(tera_combat_resolve_hero_attack(&g->encounter.reaction,letters[k],raw,
                g->encounter.actors[c->actor].magic,g->encounter.monster_hp,1,&hit),"damage estimate");
            sum+=hit.damage;
        }
        if(sum>maximum) {maximum=sum;best=(unsigned)letters[k];}
    }
    return best;
}
static unsigned chest_actor(const TeraCharacterState *c)
{
    unsigned id,key='A',best='A',score=0;
    for(id=0;id<10;++id) {
        const TeraRoundActor *a=&c->combat.encounter.actors[id];
        if(id && !a->active) continue;
        if(a->active && strcmp(a->condition,"HS")) {
            const TeraCombatActor *stats=&c->combat.encounter.attributes[id];
            unsigned power=stats->stats[7]+stats->stats[9]+stats->skills[7];
            if(power>score) {score=power;best=key;}
        }
        ++key;
    }
    return best;
}
/* Ne resout rien a la place du core : fournir uniquement les confirmations,
 * les mots generes et les choix de combat aux lectures deja demandees. */
static void drain(Campaign *p)
{
    while(p->game.phase!=TERA_GAME_READY && p->game.phase<TERA_GAME_COMPLETE) {
        TeraGameSession *s=&p->game;TeraCombatSession *combat=waiting_combat(s);
        if(combat) {
            if(combat->phase==TERA_COMBAT_ACTION) press(p,choose_attack(combat,p->avoid_fights));
            else if(combat->phase==TERA_COMBAT_CONFIRM) press(p,'O');
            else if(combat->phase==TERA_COMBAT_WORD) {char word[10];memcpy(word,combat->game.secret_word,10);type(p,word);press(p,13);}
            else require(0,"finished nested combat not settled");
        } else if(s->phase==TERA_GAME_NAVIGATION) {
            TeraNavigationSession *n=&s->active.navigation;
            if(n->phase==TERA_NAV_GATE) press(p,n->state.world.maps[n->state.world.characters.combat.position][n->wall]==3 ? 'U' : 'O');
            else if(n->phase==TERA_NAV_CHEST) press(p,n->chest.phase==TERA_CHEST_CONFIRM ? 'O' : chest_actor(&n->chest.state.world.characters));
            else if(n->phase==TERA_NAV_WORD) {char word[10];memcpy(word,n->state.world.secret_words[3],10);type(p,word);press(p,13);}
            else press(p,'O');
        } else if(s->phase==TERA_GAME_MEDUZ) {
            if(s->active.meduz.phase==TERA_MEDUZ_ENTRY_WORD || s->active.meduz.phase==TERA_MEDUZ_WORD) {
                char word[10];unsigned index=s->active.meduz.phase==TERA_MEDUZ_ENTRY_WORD ? 1 : 2;
                memcpy(word,s->active.meduz.state.world.secret_words[index],10);type(p,word);press(p,13);
            } else press(p,'O');
        } else if(s->phase==TERA_GAME_ALFOL) {
            if(s->active.alfol.phase==TERA_ALFOL_WORD) press(p,13);
            else if(s->active.alfol.phase==TERA_ALFOL_GATE) press(p,s->active.alfol.state.world.maps[s->active.alfol.state.world.characters.combat.position][s->active.alfol.wall]==3 ? 'U' : 'O');
            else press(p,'O');
        } else if(s->phase==TERA_GAME_SPACE) {
            if(s->active.space.phase==TERA_SPACE_SHOT) {
                static const unsigned keypad[3][3]={{'7','8','9'},{'4','5','6'},{'1','2','3'}};
                press(p,keypad[s->active.space.alien_y+1][s->active.space.alien_x+1]);
            } else press(p,'N');
        } else if(s->phase==TERA_GAME_ENDING) press(p,13);
        else if(s->phase==TERA_GAME_LIFE) {
            require(++p->deaths<=30,"succession budget");
            printf("succession=%u commands=%u tasks=%d\n",p->deaths,p->commands,s->state.world.tasks_counter);fflush(stdout);
            press(p,13);type(p,"TERA\rH333VVTTAAO");
            require(p->game.phase==TERA_GAME_READY,"successor arrival");
            longjmp(succession_checkpoint,1);
        }
        else if(s->phase==TERA_GAME_AMARANDE) press(p,'O');
        else {fprintf(stderr,"unhandled phase=%d\n",s->phase);exit(1);}
    }
    require(p->game.phase!=TERA_GAME_HALTED,"ship destroyed");
}
static void command(Campaign *p,unsigned key) {press(p,key);drain(p);}
static void progress(Campaign *p,const char *stage)
{
    const TeraCharacterState *c=&p->game.state.world.characters;
    printf("%s commands=%u wins=%u flee=%u world=%u pos=%d gold=%d xp=%d level=%d tasks=%d hp=%d,%d,%d,%d\n",
        stage,p->commands,p->wins,p->escapes,c->combat.world_id,c->combat.position,c->combat.money,c->combat.experience,c->level,
        p->game.state.world.tasks_counter,c->combat.encounter.actors[0].hp,c->combat.encounter.actors[4].hp,c->combat.encounter.actors[5].hp,c->combat.encounter.actors[8].hp);fflush(stdout);
}
/* Trouver un premier pas dans le graphe lu, sans appeler le moteur sur une
 * copie. Les portes verrouillees restent interdites sans la vraie cle. */
static int first_step(const TeraExplorationState *s,int target,int surface)
{
    const TeraWorldState *w=&s->world;const TeraCombatGame *g=&w->characters.combat;
    static const int deltas[4]={1,-10,-1,10};
    int queue[1900],parent[1900],first[1900],head=0,tail=0,start=g->position;
    unsigned d,i;int limit=surface==1 ? 1900 : surface==2 ? 100 : 1100;
    if(start==target) return 0;
    for(i=0;i<1900;++i) parent[i]=-1;
    require(start>=0 && start<limit && target>=0 && target<limit,"route endpoint");
    queue[tail++]=start;parent[start]=start;
    while(head<tail) {
        int pos=queue[head++];
        for(d=0;d<4;++d) {
            int delta=surface==1 ? (d==0 ? 1 : d==1 ? -30 : d==2 ? -1 : 30) : surface==2 ? (d==0 ? 1 : d==1 ? -10 : d==2 ? -1 : 10) : deltas[d];
            int next=pos+delta,blocked=0;
            if(surface==1) {
                unsigned count=d%2==0 ? 9 : 25;
                for(i=0;i<count;++i) if((d%2==0 ? w->horizontal_mountains[i] : w->vertical_mountains[i])==pos+(delta<0 ? delta : 0)) blocked=1;
                if(next>1899) next-=900;
                if(delta<0 && next<1000) next+=900;
            } else if(surface==2) {if(next>99) next-=100;if(next<0) next+=100;}
            else {
                unsigned wall=w->maps[pos][d];
                blocked=!wall || (wall==3 && w->characters.keys[g->world_id]==10);
                if(next<0 || next>1099 || (next/100!=start/100 && next!=target)) blocked=1;
                if(g->world_id==0 && next==0 && target!=0) blocked=1;
                /* Le portail de Cite est un changement de monde, pas une
                 * liaison ordinaire utilisable pour traverser la case. */
                if(!s->navigation_overlay && g->world_id==0 && w->maps[pos][4]==101 && delta==1) blocked=1;
            }
            if(blocked || next<0 || next>=limit || parent[next]!=-1) continue;
            parent[next]=pos;first[next]=pos==start ? delta : first[pos];
            if(next==target) return first[next];
            queue[tail++]=next;
        }
    }
    return 0;
}
static unsigned relative_key(int heading,int delta)
{
    if(delta==heading) return '8';
    if(delta==-heading) return '2';
    if(delta==(heading==1 ? 10 : heading==-1 ? -10 : heading==10 ? -1 : 1)) return '6';
    return '4';
}
static void walk(Campaign *p,int target,int surface)
{
    unsigned attempts=0,heals=0,world=p->game.state.world.characters.combat.world_id;
    while(p->game.state.world.characters.combat.position!=target) {
        const TeraCombatGame *g=&p->game.state.world.characters.combat;
        int destination=target,delta;
        if(!surface && world>0 && world<8) {
            unsigned i;int needs_heal=0,altar=-1;
            for(i=0;i<10;++i) {
                const TeraRoundActor *a=&g->encounter.actors[i];
                if(a->active && strcmp(a->condition,"HS") && (a->hp<a->hp_limit*3/4 || a->magic<3)) needs_heal=1;
            }
            for(i=world*100;i<world*100+100;++i) if(p->game.state.world.maps[i][4]==103) altar=(int)i;
            if(needs_heal && altar>=0 && heals<12) {
                if(g->position==altar) {++heals;command(p,'I');continue;}
                if(first_step(&p->game.state,altar,0)) destination=altar;
            }
        }
        delta=first_step(&p->game.state,destination,surface);
        if(!delta) {fprintf(stderr,"no route world=%u pos=%d target=%d key=%d\n",g->world_id,g->position,target,p->game.state.world.characters.keys[g->world_id<10 ? g->world_id : 0]);exit(1);}
        require(++attempts<5000,"route budget (repeated flee?)");
        command(p,surface ? (delta==1 ? '8' : delta==-1 ? '2' : delta>0 ? '6' : '4') : relative_key(g->position-g->previous_position,delta));
        if(!surface && (target==0 || target==799 || target==899 || target==999) && p->game.state.world.characters.combat.world_id==11) return;
        if(!surface && target==99 && p->game.state.world.characters.combat.world_id==0) return;
        if(!surface && world==0 && p->game.state.world.characters.combat.world_id==1) {
            walk(p,99,0);continue; /* Une fuite peut franchir le portail. */
        }
        if(p->game.state.world.characters.combat.world_id!=world) {
            fprintf(stderr,"route world=%u -> %u pos=%d target=%d surface=%d\n",world,p->game.state.world.characters.combat.world_id,p->game.state.world.characters.combat.position,target,surface);
            require(0,"unexpected world during route");
        }
    }
}
static void go_town(Campaign *p,unsigned town) {walk(p,p->game.state.world.towns[town],1);}
static int find_content(const Campaign *p,unsigned content)
{
    unsigned base=p->game.state.world.characters.combat.world_id*100,i;
    for(i=base;i<base+100;++i) if(p->game.state.world.maps[i][4]==content) return (int)i;
    require(0,"required generated marker missing");return -1;
}
static void camp(Campaign *p)
{
    unsigned i,tries=0;
    for(;;) {
        int need=0;
        for(i=0;i<10;++i) {
            const TeraRoundActor *a=&p->game.state.world.characters.combat.encounter.actors[i];
            if(a->active && strcmp(a->condition,"HS") && (a->hp<a->hp_limit || a->magic<a->magic_limit)) need=1;
        }
        if(!need) return;
        if(++tries>24) return; /* Certains jets de vitalite ne peuvent pas reussir. */
        type(p,"HDO");drain(p);
    }
}
static void recruit(Campaign *p,unsigned id)
{
    if(p->game.state.world.characters.combat.encounter.actors[id].active) return;
    walk(p,p->game.state.world.locations[id],1);
    type(p,"NSO"); /* Disponibilite via negociation. */
    require(p->game.phase==TERA_GAME_READY,"solicit return");
    type(p,"SO");require(p->game.phase==TERA_GAME_READY,"seduce return");
    require(p->game.state.world.characters.combat.encounter.actors[id].active,"recruit failed");
    progress(p,"recruited");
}
static void buy(Campaign *p,unsigned actor,char letter)
{
    type(p,"AO");press(p,actor);press(p,(unsigned)letter);press(p,27);
    require(p->game.phase==TERA_GAME_SHOP,"buy/menu return");
}
static void equip_party(Campaign *p)
{
    const TeraCombatGame *g=&p->game.state.world.characters.combat;
    unsigned helper=g->helpers[1];char item=helper==3 ? 'O' : helper==6 ? 'P' : helper==9 ? 'Q' : 'R';
    go_town(p,0);type(p,"EOHO");
    require(p->game.phase==TERA_GAME_SHOP,"hospital/menu return");
    if(!tera_combat_equipment_bonus(p->game.active.shop.state.world.characters.combat.equipment[0],18)) buy(p,'A','A');
    if(!tera_combat_equipment_bonus(p->game.active.shop.state.world.characters.combat.equipment[0],helper)) buy(p,'A',item);
    {unsigned id,key='A';
     for(id=0;id<10;++id) if(p->game.active.shop.state.world.characters.combat.encounter.actors[id].active) {
        TeraCombatGame *current=&p->game.active.shop.state.world.characters.combat;
        unsigned price=helper==3 ? 200 : helper==6 ? 250 : helper==9 ? 300 : 350;
        if(!tera_combat_equipment_bonus(current->equipment[id],helper) && current->money>(int)price+50) buy(p,key,item);
        ++key;
     }}
    press(p,27);
    require(p->game.phase==TERA_GAME_READY,"exit shop");progress(p,"equipped");
}
/* Les clefs et coffres sont acquis au sol, avec les vrais pieges. Ne pas
 * changer la cellule ni l'inventaire depuis le conducteur de parcours. */
static void take_chest(Campaign *p)
{
    command(p,'O');command(p,'P');
}
static void city_quest(Campaign *p)
{
    unsigned level;
    go_town(p,2);camp(p);command(p,'E');progress(p,"city entered");
    if(p->game.state.world.characters.keys[1]==10) {walk(p,find_content(p,91),0);take_chest(p);}
    require(p->game.state.world.characters.keys[1]==1,"key 1 acquired");
    walk(p,find_content(p,101),0);
    {const TeraCombatGame *g=&p->game.state.world.characters.combat;command(p,relative_key(g->position-g->previous_position,1));}
    require(p->game.state.world.characters.combat.world_id==1,"city portal");
    for(level=1;level<=7;++level) {
        require(p->game.state.world.characters.combat.world_id==level,"city level");
        if(p->game.state.world.characters.keys[level+1]==10) {walk(p,find_content(p,91+level),0);take_chest(p);}
        require(p->game.state.world.characters.keys[level+1]==(int16_t)(level+1),"next level key");
        if(level<7) {walk(p,find_content(p,101),0);command(p,'D');}
        else walk(p,p->game.state.world.arioch_position,0);
        progress(p,"city progress");
    }
    require(p->game.state.world.tasks_done[2],"Arioch objective through combat");
    for(level=7;level>1;--level) {walk(p,find_content(p,100),0);command(p,'M');}
    walk(p,99,0);
    walk(p,0,0);progress(p,"city returned");
}
static void seth_quest(Campaign *p)
{
    go_town(p,4);camp(p);command(p,'E');
    walk(p,find_content(p,110),0);progress(p,"Seth altar");command(p,'I');progress(p,"Seth invoked");
    require(p->game.state.world.tasks_done[3],"Seth objective through invocation/combat");
    walk(p,799,0);progress(p,"Seth returned");
}
static void hospital(Campaign *p)
{
    go_town(p,0);type(p,"EOHO\033");
    require(p->game.phase==TERA_GAME_READY,"hospital exit");
}
static void collect_travel_funds(Campaign *p)
{
    unsigned level=0,tries=0;
    if(p->game.state.world.characters.combat.money>=1250) return;
    go_town(p,2);camp(p);command(p,'E');
    while(p->game.state.world.characters.combat.money<1250) {
        unsigned i,base=level*100;int target=-1;
        require(++tries<150,"treasure budget");
        for(i=base;i<base+100;++i) {
            unsigned content=p->game.state.world.maps[i][4];
            if(content>25 && content<90 && first_step(&p->game.state,(int)i,0)) {target=(int)i;break;}
        }
        if(target>=0) {walk(p,target,0);take_chest(p);}
        else {
            require(level<2,"insufficient accessible treasure for media");
            walk(p,find_content(p,101),0);
            if(!level) {const TeraCombatGame *g=&p->game.state.world.characters.combat;command(p,relative_key(g->position-g->previous_position,1));}
            else command(p,'D');
            ++level;
        }
    }
    while(level>1) {walk(p,find_content(p,100),0);command(p,'M');--level;}
    if(level) walk(p,99,0);
    walk(p,0,0);progress(p,"travel funds");
}
static void alfol_quest(Campaign *p)
{
    go_town(p,1);camp(p);press(p,'E');press(p,'O');drain(p);
    require(p->game.state.world.characters.combat.on_alfol,"outbound flight");
    walk(p,p->game.state.world.alfol_position,2);command(p,'E');
    walk(p,find_content(p,1),0);
    /* Entrer dans la case a deja pose la question et paye via drain. */
    require(p->game.state.world.tasks_done[1],"media paid in generated room");
    walk(p,899,0);walk(p,0,2);command(p,'R');
    require(!p->game.state.world.characters.combat.on_alfol,"return flight");progress(p,"Alfol returned");
}
static void meduz_quest(Campaign *p)
{
    go_town(p,3);camp(p);command(p,'M');
    require(p->game.state.world.characters.combat.world_id==10,"pyramid voyage");
    walk(p,find_content(p,2),0);command(p,'M');
    require(p->game.state.world.mental_extension,"catalyst meditation");
    walk(p,find_content(p,3),0);command(p,'I');
    require(p->game.state.world.tasks_done[0],"Meduz objective at sanctuary");
    walk(p,999,0);progress(p,"Meduz returned");
}
int main(int argc,char **argv)
{
    TeraExplorationState world;TeraNameVariants names;static Campaign p;
    uint32_t seed=argc>2 ? (uint32_t)strtoul(argv[2],NULL,0) : 1986;
    require(argc>=2,"usage: test_campaign_c RESOURCE_DIR [SEED]");
    memset(&p,0,sizeof p);p.digest=UINT32_C(2166136261);p.avoid_fights=1;
    p.resume_enabled=argc>3 && strcmp(argv[3],"resume")==0;
    if(!prepare_world(&world,&names,argv[1],seed)) {puts("SKIP: original NOMPERS/VALMON absent");return 77;}
    if(argc>4 && strcmp(argv[4],"presentation")==0) {
        p.presentation.emit=presentation_event;p.presentation.context=&p;
        world.world.characters.combat.presentation=&p.presentation;
    }
    require(tera_game_create(&p.game,&world,combat_event,&p),"campaign creation");
#ifdef TERA_BINARY_CAMPAIGN
    teramain_oracle_start(argv[1],&world,&p.game);
#endif
    type(&p,"TERA\rH333VVTTAAO");drain(&p);progress(&p,"hero");
    (void)setjmp(succession_checkpoint);
    p.avoid_fights=1;
    recruit(&p,8);
    /* Reconstituer le groupe apres chaque succession, puis reprendre les
     * objectifs manquants dans le monde conserve par le moteur. */
    recruit(&p,4);recruit(&p,3);equip_party(&p);p.avoid_fights=2;
    if(!p.game.state.world.tasks_done[2]) city_quest(&p);
    if(!p.game.state.world.tasks_done[3]) {hospital(&p);seth_quest(&p);}
    if(!p.game.state.world.tasks_done[1]) {collect_travel_funds(&p);hospital(&p);alfol_quest(&p);}
    if(!p.game.state.world.tasks_done[0]) {hospital(&p);meduz_quest(&p);}
    require(p.game.phase==TERA_GAME_COMPLETE,"complete game ending");
    require(p.game.active.ending.victory && !p.game.active.ending.runtime_error,"victory score");
    require(p.game.state.world.tasks_counter==4,"four naturally completed tasks");
    require(p.objectives==15 && p.visited==4095,"all objectives and twelve worlds visited");
    require(p.wins && p.escapes && p.deaths,"combat, escape and succession coverage");
    if(p.presentation.emit) {
        require(p.presentation_objectives==15 && p.presentation_zones>10,"observed objectives and zones");
        printf("presentation events=%u zones=%u objectives=%u\n",p.presentation_events,p.presentation_zones,p.presentation_objectives);
    }
    progress(&p,"VICTORY");printf("inputs=%u commands=%u wins=%u escapes=%u deaths=%u digest=%08lx score=%s\n",p.inputs,p.commands,p.wins,p.escapes,p.deaths,(unsigned long)p.digest,p.game.active.ending.score_text);
#ifdef TERA_BINARY_CAMPAIGN
    teramain_oracle_finish(&p.game);
#endif
    /* Traces corrigees apres comparaison integrale BIN/C : inscriptions
     * 104..113 et exclusion du monstre13. Le digest reste un resume C,
     * pas une empreinte cryptographique du DOS. */
    if(seed==1986) {
        require(p.inputs==3886 && p.commands==2181 && p.wins==36 && p.escapes==83 && p.deaths==5,"seed 1986 route counters");
        require(p.digest==UINT32_C(0x2d0ca4bb) && !strcmp(p.game.active.ending.score_text,"8"),"seed 1986 trace/score");
    } else if(seed==12345) {
        require(p.inputs==3257 && p.commands==1807 && p.wins==46 && p.escapes==52 && p.deaths==3,"seed 12345 route counters");
        require(p.digest==UINT32_C(0xb246cd73) && !strcmp(p.game.active.ending.score_text,"10"),"seed 12345 trace/score");
    }
    if(p.resume_enabled) {
        require(p.checkpoints>20 && p.checkpoint_worlds==4095,"resume coverage across twelve worlds");
        printf("checkpoints=%u worlds=%u\n",p.checkpoints,p.checkpoint_worlds);
    }
    return 0;
}
