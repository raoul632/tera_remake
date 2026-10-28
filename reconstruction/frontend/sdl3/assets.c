#include "assets.h"
#include "embedded.h"
#include <stdio.h>
#include <string.h>

int tera_assets_world(TeraExplorationState *world,const char *dir,uint32_t seed,
    unsigned difficulty,char *error,size_t capacity)
{
    unsigned char names[270],monsters[264];unsigned group,id,column;
    TeraExplorationState next;TeraNameVariants variants;TeraWorldReport report;
    if(!world || !error || !capacity) return 0;
    if(!tera_resource_read(dir,"NOMPERS",names,sizeof names,error,capacity) ||
       !tera_resource_read(dir,"VALMON",monsters,sizeof monsters,error,capacity)) return 0;
    memset(&next,0,sizeof next);memset(&variants,0,sizeof variants);
    for(group=0;group<3;++group) for(id=1;id<10;++id) {
        unsigned offset=(group*9+id-1)*10,length=names[offset];
        char *target=group ? variants.names[group-1][id] : next.world.characters.names[id];
        if(length>9) {snprintf(error,capacity,"Invalid Pascal string in NOMPERS");return 0;}
        memcpy(target,names+offset+1,length);
    }
    for(id=0;id<22;++id) for(column=0;column<6;++column) {
        unsigned offset=id*12+column*2,bits=monsters[offset]+256u*monsters[offset+1];
        next.world.characters.combat.monster_values[id][column]=
            (int16_t)(bits<32768u ? (int32_t)bits : (int32_t)bits-65536);
    }
    next.world.characters.combat.random.seed=seed;
    /* Contexte original standard documente par spec_generation_monde.md. */
    next.world.placement_result_residue=-211;
    if(!tera_world_generate_maps(&next.world,difficulty,2000000,&report) ||
       !tera_world_initialize_places(&next.world,&variants,2000000,&report)) {
        snprintf(error,capacity,"Core rejected world generation (difficulty/budget/data)");return 0;
    }
    *world=next;error[0]=0;return 1;
}
