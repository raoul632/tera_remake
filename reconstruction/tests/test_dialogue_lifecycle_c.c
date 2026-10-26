#include "tera/dialogue.h"
#include "tera/lifecycle.h"
#include <assert.h>
#include <string.h>

int main(void)
{
    TeraExplorationState state;
    TeraLifeSession life;
    TeraDialogueSession dialogue;
    unsigned i;
    const char *keys="NEXT\rH333FIVEHAO";
    memset(&state,0,sizeof state);
    state.world.characters.combat.random.seed=1986;
    state.world.characters.combat.encounter.party_available=1;
    assert(!tera_life_start(&life,&state));
    /* Une succession conserve aussi les donnees non transportees au
     * terminal : positions cachees, mots, cartes, flags et equipements. */
    state.world.characters.combat.encounter.party_available=0;
    state.world.characters.combat.encounter.actors[0].active=1;
    state.world.characters.combat.encounter.actors[4].active=1;
    state.world.characters.combat.on_alfol=1;
    state.world.characters.combat.equipment[0][7]=19;
    state.world.characters.combat.equipment[4][9]=23;
    state.world.characters.combat.money=12;
    state.world.maps[1099][6]=255;state.world.hidden_positions[1]=987;
    state.world.tasks_done[2]=1;state.world.tasks_counter=3;
    state.world.mental_extension=1;state.world.has_map[10]=1;
    strcpy(state.world.secret_words[3],"SECRET");state.information_context=1;
    assert(tera_life_start(&life,&state));
    assert(life.state.world.characters.combat.random.seed==1986);
    assert(tera_life_key(&life,27));
    for(i=0;keys[i];++i) assert(tera_life_key(&life,(unsigned char)keys[i]));
    assert(life.phase==TERA_LIFE_READY);
    assert(life.state.world.characters.combat.money==500);
    assert(life.state.world.characters.arrival_pending);
    assert(!life.state.world.characters.combat.on_alfol);
    assert(!life.state.world.characters.combat.encounter.actors[4].active);
    assert(life.state.world.characters.combat.equipment[0][7]==19);
    assert(life.state.world.characters.combat.equipment[4][9]==23);
    assert(life.state.world.maps[1099][6]==255 && life.state.world.hidden_positions[1]==987);
    assert(life.state.world.tasks_done[2] && life.state.world.tasks_counter==3);
    assert(life.state.world.mental_extension && life.state.world.has_map[10]);
    assert(strcmp(life.state.world.secret_words[3],"SECRET")==0 && life.state.information_context);
    assert(!tera_life_key(&life,'Q'));
    for(i=0;i<4;++i) state.world.tasks_done[i]=1;
    assert(tera_life_start(&life,&state));assert(tera_life_key(&life,'O'));
    assert(life.phase==TERA_LIFE_COMPLETE);
    assert(!life.state.world.characters.combat.encounter.party_available);
    assert(tera_dialogue_start(&dialogue,&state,'N'));
    assert(tera_dialogue_key(&dialogue,'I'));assert(tera_dialogue_key(&dialogue,'N'));
    assert(dialogue.phase==TERA_DIALOGUE_DONE);
    assert(memcmp(&dialogue.state,&state,sizeof state)==0);
    assert(!tera_dialogue_key(&dialogue,'O'));
    return 0;
}
