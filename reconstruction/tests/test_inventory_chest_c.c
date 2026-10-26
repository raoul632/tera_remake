#include "tera/chest.h"
#include <assert.h>
#include <string.h>

int main(void)
{
    TeraExplorationState state={0};
    TeraChestSession chest,before;
    TeraInventorySession inventory;
    unsigned item=23,i;
    state.world.characters.combat.world_id=2;state.world.characters.combat.position=245;
    for(i=0;i<10;++i) strcpy(state.world.characters.combat.encounter.actors[i].condition,"OK");
    state.world.characters.combat.encounter.actors[0].active=1;
    state.world.characters.combat.encounter.actors[5].active=1;
    state.world.characters.combat.encounter.actors[9].active=1;
    assert(tera_inventory_pick_actor(&state.world.characters,'A')==0);
    assert(tera_inventory_pick_actor(&state.world.characters,'B')==5);
    assert(tera_inventory_pick_actor(&state.world.characters,'D')==-1);
    memset(state.world.characters.combat.equipment[5],18,10);
    assert(tera_inventory_acquire(&state.world.characters,5,&item)==TERA_ITEM_FULL && item==23);
    state.world.characters.combat.equipment[5][9]=0;
    assert(tera_inventory_acquire(&state.world.characters,5,&item)==TERA_ITEM_ADDED && item==0);
    assert(state.world.characters.combat.equipment[5][9]==23);
    assert(tera_inventory_start(&inventory,&state.world.characters,'N'));
    assert(tera_inventory_key(&inventory,'D'));assert(tera_inventory_key(&inventory,'O'));
    assert(tera_inventory_key(&inventory,'B'));assert(tera_inventory_key(&inventory,'9'));
    assert(inventory.phase==TERA_INV_TARGET);
    assert(tera_inventory_key(&inventory,'A'));
    assert(inventory.outcome==TERA_ITEM_GIVEN);
    assert(inventory.state.combat.equipment[5][9]==0 && inventory.state.combat.equipment[0][0]==23);
    assert(tera_inventory_start(&inventory,&state.world.characters,'N'));
    assert(tera_inventory_key(&inventory,'I') && inventory.phase==TERA_INV_ERROR);
    assert(memcmp(&inventory.state,&state.world.characters,sizeof inventory.state)==0);
    state.world.maps[245][4]=99;state.chest_open=1;
    assert(tera_chest_start(&chest,&state,'P'));assert(chest.phase==TERA_CHEST_CONFIRM);
    before=chest;assert(!tera_chest_key(&chest,256));assert(memcmp(&chest,&before,sizeof chest)==0);
    assert(tera_chest_key(&chest,13));
    assert(chest.state.world.has_map[2] && chest.state.world.maps[245][4]==0);
    state.world.maps[245][4]=100;before=chest;
    assert(!tera_chest_start(&chest,&state,'P'));assert(memcmp(&chest,&before,sizeof chest)==0);
    return 0;
}
