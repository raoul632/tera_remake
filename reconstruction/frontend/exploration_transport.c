#include "exploration_transport.h"
#include "character_transport.h"
#include <stdio.h>
#include <string.h>

/* Un seul transport pour les deux blocs : aucune regle dans Python.
 * Les donnees geographiques sont fournies par le monde genere de l'hote. */
int tera_frontend_read_exploration(TeraExplorationState *s,unsigned *mode,unsigned *key)
{
    TeraNameVariants variants;
    int v,initialize;
    unsigned i,j;
    memset(s,0,sizeof *s);
#define READ(field,lo,hi) do {if(!tera_frontend_read_int(&v,lo,hi)) return 0;field=v;} while(0)
    READ(*mode,0,2);READ(*key,0,255);
    if(!tera_frontend_read_characters(&s->world.characters,&variants,&initialize) || initialize) return 0;
    READ(s->sound,0,1);READ(s->moved,0,1);READ(s->town_found,0,1);READ(s->town_id,0,19);
    s->world.characters.combat.sound=s->sound;
    READ(s->force_day,0,1);READ(s->encounter_suppressed,0,1);
    READ(s->world.characters.combat.recruit_ready,0,1);
    READ(s->world.characters.combat.on_alfol,0,1);
    READ(s->world.characters.combat.location_changed,0,1);
    READ(s->world.characters.combat.previous_position,-32768,32767);
    READ(s->world.characters.combat.difficulty,0,10);
    READ(s->world.alfol_position,-32768,32767);
    for(i=0;i<9;++i) READ(s->world.horizontal_mountains[i],-32768,32767);
    for(i=0;i<25;++i) READ(s->world.vertical_mountains[i],-32768,32767);
    for(i=0;i<20;++i) READ(s->world.towns[i],-32768,32767);
    for(i=1;i<10;++i) READ(s->world.locations[i],-32768,32767);
    for(i=1;i<10;++i) READ(s->world.extra_locations[i],-32768,32767);
    for(i=0;i<1100;++i) for(j=0;j<7;++j) READ(s->world.maps[i][j],0,255);
    for(i=0;i<1100;++i) s->world.characters.combat.maps_column7[i]=s->world.maps[i][6];
    for(i=0;i<22;++i) for(j=0;j<6;++j) READ(s->world.characters.combat.monster_values[i][j],-32768,32767);
    for(i=0;i<4;++i) READ(s->world.characters.combat.helpers[i],3,25);
    for(i=0;i<10;++i) READ(s->world.characters.combat.secret_word[i],0,255);
    READ(s->world.fast_moves,0,1);
    READ(s->information_context,0,1);
#undef READ
    return 1;
}

