#include "tera/save.h"
#include <limits.h>
#include <string.h>

/* Version 1 : magic[8], version u32, longueur payload u32, CRC32 u32,
 * puis champs ci-dessous. Entiers little-endian, signes en complement a 2.
 * Une seule description des champs sert aux deux sens pour eviter les
 * divergences d'ordre entre lecteur et ecrivain. */
enum { HEADER_SIZE=20 };
static const unsigned char magic[8]={'T','E','R','A','S','A','V',0};
typedef struct {
    unsigned char *bytes;
    size_t position,limit;
    int reading,error;
} Codec;

static uint32_t integer(Codec *c,uint32_t value,unsigned width)
{
    unsigned i;uint32_t result=0;
    if(c->position>c->limit || width>c->limit-c->position) {c->error=1;return 0;}
    for(i=0;i<width;++i) {
        if(c->reading) result|=(uint32_t)c->bytes[c->position++]<<(8*i);
        else c->bytes[c->position++]=(unsigned char)(value>>(8*i));
    }
    return c->reading ? result : value;
}
static void unsigned_field(Codec *c,unsigned *value)
{
    uint32_t n=integer(c,(uint32_t)*value,4);
    if(n>UINT_MAX) c->error=1;else if(c->reading) *value=(unsigned)n;
}
static void int_field(Codec *c,int *value)
{
    uint32_t n=integer(c,(uint32_t)*value,4);
    int64_t signed_value=n<=INT32_MAX ? (int64_t)n : (int64_t)n-INT64_C(4294967296);
    if(signed_value<INT_MIN || signed_value>INT_MAX) c->error=1;
    else if(c->reading) *value=(int)signed_value;
}
static void short_field(Codec *c,int16_t *value)
{
    uint32_t n=integer(c,(uint16_t)*value,2);
    if(c->reading) *value=(int16_t)(n<32768 ? (int32_t)n : (int32_t)n-65536);
}
static void octets(Codec *c,void *data,size_t size)
{
    unsigned char *p=data;size_t i;
    for(i=0;i<size;++i) {uint32_t n=integer(c,p[i],1);if(c->reading) p[i]=(unsigned char)n;}
}
static void shorts(Codec *c,int16_t *values,unsigned count)
{
    unsigned i;for(i=0;i<count;++i) short_field(c,&values[i]);
}

/* Champs persistants exhaustifs de l'etat commun. Les tables de monstres
 * et mots sont incluses pour une reprise sans dependance a un ancien
 * pointeur ou chargeur de ressources. Les octets CP437 sont conserves. */
static void payload(Codec *c,TeraExplorationState *s,TeraGameContext *control)
{
    TeraWorldState *w=&s->world;TeraCharacterState *ch=&w->characters;
    TeraCombatGame *g=&ch->combat;TeraEncounterState *e=&g->encounter;
    unsigned i;
    for(i=0;i<10;++i) {
        TeraRoundActor *a=&e->actors[i];
        octets(c,&a->active,1);octets(c,&a->agility,1);
        short_field(c,&a->hp);short_field(c,&a->magic);
        short_field(c,&a->magic_limit);short_field(c,&a->hp_limit);
        octets(c,a->condition,sizeof a->condition);
        octets(c,e->attributes[i].stats,10);octets(c,e->attributes[i].skills,10);
    }
    short_field(c,&e->reaction.resistance_a);short_field(c,&e->reaction.resistance_b);
    short_field(c,&e->reaction.reaction_category);unsigned_field(c,&e->monster_type);
    short_field(c,&e->monster_hp);int_field(c,&e->victory);int_field(c,&e->fled);
    int_field(c,&e->party_available);
    for(i=0;i<22;++i) shorts(c,g->monster_values[i],6);
    octets(c,g->equipment,sizeof g->equipment);octets(c,g->helpers,sizeof g->helpers);
    octets(c,g->secret_word,sizeof g->secret_word);octets(c,g->maps_column7,sizeof g->maps_column7);
    unsigned_field(c,&g->world_id);unsigned_field(c,&g->difficulty);
    int_field(c,&g->on_alfol);int_field(c,&g->sound);
    short_field(c,&g->position);short_field(c,&g->previous_position);
    short_field(c,&g->money);short_field(c,&g->experience);
    int_field(c,&g->location_changed);int_field(c,&g->recruit_ready);
    {uint32_t n=integer(c,g->random.seed,4);if(c->reading) g->random.seed=n;}
    octets(c,ch->names,sizeof ch->names);shorts(c,ch->keys,10);
    short_field(c,&ch->days);short_field(c,&ch->level);short_field(c,&ch->dummy_level);
    short_field(c,&ch->energy);int_field(c,&ch->arrival_pending);
    octets(c,w->maps,sizeof w->maps);
    shorts(c,w->towns,20);shorts(c,w->locations,10);shorts(c,w->extra_locations,10);
    shorts(c,w->horizontal_mountains,9);shorts(c,w->vertical_mountains,25);
    shorts(c,w->hidden_positions,2);short_field(c,&w->arioch_position);short_field(c,&w->alfol_position);
    octets(c,w->hints,sizeof w->hints);octets(c,w->secret_words,sizeof w->secret_words);
    octets(c,w->has_map,sizeof w->has_map);octets(c,w->tasks_done,sizeof w->tasks_done);
    int_field(c,&w->fast_moves);int_field(c,&w->initial_flag);int_field(c,&w->mental_extension);
    short_field(c,&w->tasks_counter);short_field(c,&w->placement_result_residue);
    int_field(c,&s->sound);int_field(c,&s->moved);int_field(c,&s->town_found);
    unsigned_field(c,&s->town_id);int_field(c,&s->force_day);int_field(c,&s->encounter_suppressed);
    short_field(c,&s->city_backup_position);int_field(c,&s->navigation_overlay);
    int_field(c,&s->chest_open);int_field(c,&s->information_context);
    short_field(c,&control->status_actor);short_field(c,&control->space_backup_position);
    short_field(c,&control->meduz_backup_position);short_field(c,&control->pirates);
    unsigned_field(c,&control->npc_variant);unsigned_field(c,&control->mandatory_command);
}

static int boolean(int value) {return value==0 || value==1;}
/* Ne pas imposer de plafonds d'equilibrage inventes : argent, HP, jours,
 * statistiques et cartes gardent les debordements et particularites DOS.
 * Borner les index de routage et les chaines protege les lectures du core. */
static int valid_data(const TeraExplorationState *s)
{
    const TeraWorldState *w=&s->world;const TeraCombatGame *g=&w->characters.combat;
    unsigned i;
    if(g->encounter.monster_type>22 || !boolean(g->encounter.victory) ||
       !boolean(g->encounter.fled) || !boolean(g->encounter.party_available) ||
       !boolean(g->on_alfol) || !boolean(g->sound) || !boolean(g->location_changed) ||
       !boolean(g->recruit_ready) || !boolean(w->characters.arrival_pending) ||
       !boolean(w->fast_moves) || !boolean(w->initial_flag) || !boolean(w->mental_extension) ||
       !boolean(s->sound) || !boolean(s->moved) || !boolean(s->town_found) ||
       !boolean(s->force_day) || !boolean(s->encounter_suppressed) ||
       !boolean(s->chest_open) || !boolean(s->information_context) ||
       s->town_id>=20 || s->navigation_overlay<0 || s->navigation_overlay>2) return 0;
    for(i=0;i<10;++i)
        if(!boolean(g->encounter.actors[i].active) ||
           !memchr(g->encounter.actors[i].condition,0,3) ||
           !memchr(w->characters.names[i],0,10)) return 0;
    if(!memchr(g->secret_word,0,10)) return 0;
    for(i=0;i<38;++i) if(!memchr(w->hints[i],0,40)) return 0;
    for(i=0;i<4;++i) if(!memchr(w->secret_words[i],0,10) || !boolean(w->tasks_done[i])) return 0;
    for(i=0;i<11;++i) if(!boolean(w->has_map[i])) return 0;
    for(i=0;i<4;++i) if(g->helpers[i]<3 || g->helpers[i]>25) return 0;
    return 1;
}

static uint32_t checksum(const unsigned char *bytes,size_t size)
{
    uint32_t crc=UINT32_MAX;size_t i;unsigned bit;
    for(i=0;i<size;++i) {
        crc^=bytes[i];
        for(bit=0;bit<8;++bit) crc=(crc>>1)^((crc&1) ? UINT32_C(0xedb88320) : 0);
    }
    return ~crc;
}

TeraSaveResult tera_save_encode(const TeraGameSession *s,unsigned char *buffer,size_t capacity,size_t *size)
{
    unsigned char bytes[TERA_SAVE_CAPACITY];TeraGameSession checked;
    TeraExplorationState state;TeraGameContext control;Codec c={bytes,HEADER_SIZE,sizeof bytes,0,0};
    size_t count;
    if(!s || !buffer || !size) return TERA_SAVE_ARGUMENT;
    if(s->phase!=TERA_GAME_READY || s->after_mandatory || !valid_data(&s->state) ||
       !tera_game_restore_ready(&checked,&s->state,&s->control,NULL,NULL,NULL)) return TERA_SAVE_STATE;
    state=s->state;control=s->control;payload(&c,&state,&control);count=c.position;
    if(c.error) return TERA_SAVE_BUFFER;
    if(capacity<count) return TERA_SAVE_BUFFER;
    memcpy(bytes,magic,8);c.position=8;
    integer(&c,1,4);integer(&c,(uint32_t)(count-HEADER_SIZE),4);
    integer(&c,checksum(bytes+HEADER_SIZE,count-HEADER_SIZE),4);
    memcpy(buffer,bytes,count);*size=count;return TERA_SAVE_OK;
}

TeraSaveResult tera_save_decode(TeraGameSession *s,const unsigned char *buffer,size_t size,
    const TeraInputSource *input,TeraCombatObserver observer,void *context)
{
    unsigned char bytes[TERA_SAVE_CAPACITY];TeraExplorationState state={0};TeraGameContext control={0};
    Codec c={bytes,8,size,1,0};uint32_t version,length,crc;
    if(!s || !buffer) return TERA_SAVE_ARGUMENT;
    if(size<HEADER_SIZE || size>sizeof bytes || memcmp(buffer,magic,8)) return TERA_SAVE_FORMAT;
    memcpy(bytes,buffer,size);version=integer(&c,0,4);length=integer(&c,0,4);crc=integer(&c,0,4);
    if(version!=1 || length!=size-HEADER_SIZE) return TERA_SAVE_FORMAT;
    if(checksum(bytes+HEADER_SIZE,length)!=crc) return TERA_SAVE_CHECKSUM;
    payload(&c,&state,&control);
    if(c.error || c.position!=size) return TERA_SAVE_FORMAT;
    if(!valid_data(&state) || !tera_game_restore_ready(s,&state,&control,input,observer,context)) return TERA_SAVE_STATE;
    return TERA_SAVE_OK;
}

TeraSaveResult tera_save_write(FILE *file,const TeraGameSession *s)
{
    unsigned char bytes[TERA_SAVE_CAPACITY];size_t size;TeraSaveResult result;
    if(!file) return TERA_SAVE_ARGUMENT;
    result=tera_save_encode(s,bytes,sizeof bytes,&size);
    if(result!=TERA_SAVE_OK) return result;
    if(fwrite(bytes,1,size,file)!=size || fflush(file)!=0 || ferror(file)) return TERA_SAVE_IO;
    return TERA_SAVE_OK;
}
TeraSaveResult tera_save_read(FILE *file,TeraGameSession *s,const TeraInputSource *input,
    TeraCombatObserver observer,void *context)
{
    unsigned char bytes[TERA_SAVE_CAPACITY];size_t size;
    if(!file || !s) return TERA_SAVE_ARGUMENT;
    size=fread(bytes,1,sizeof bytes,file);
    if(ferror(file)) return TERA_SAVE_IO;
    if(size==sizeof bytes && fgetc(file)!=EOF) return TERA_SAVE_FORMAT;
    if(ferror(file)) return TERA_SAVE_IO;
    return tera_save_decode(s,bytes,size,input,observer,context);
}
