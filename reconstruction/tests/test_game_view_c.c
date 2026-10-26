/* La vue doit suivre la sous-session suspendue, rester const et ne jamais
 * consommer de RNG/callback. Fixtures synthetiques de routage, pas du jeu. */
#include "module_test_support.h"
#include "tera/game.h"
#include <string.h>
int main(void)
{
    TeraGameSession s,before;TeraGameView v;
    TeraCharacterCard card,card_before;TeraCombatGame live;
    memset(&s,0,sizeof s);s.phase=TERA_GAME_READY;
    before=s;
    assert(!tera_game_view(NULL,&v));assert(!tera_game_view(&s,NULL));
    assert(tera_game_view(&s,&v));assert(v.exploration==&s.state && !v.combat);
    assert(memcmp(&before,&s,sizeof s)==0);
    strcpy(s.state.world.characters.names[8],"COMPAGNON");
    s.state.world.characters.keys[8]=42;
    memset(&live,0,sizeof live);live.equipment[8][9]=25;
    live.encounter.actors[8].hp=17;live.encounter.actors[8].hp_limit=21;
    live.encounter.attributes[8].skills[9]=5;
    strcpy(live.encounter.actors[8].condition,"HS");before=s;
    assert(tera_character_card(&s.state.world.characters,&live,8,&card));
    assert(card.hp==17 && card.hp_limit==21 && card.key==42);
    assert(card.equipment[9]==25 && card.skills[9]==5 && !strcmp(card.name,"COMPAGNON"));
    assert(!memcmp(&s,&before,sizeof s));card_before=card;
    assert(!tera_character_card(&s.state.world.characters,NULL,10,&card));
    assert(!memcmp(&card,&card_before,sizeof card));
    live.encounter.actors[8].hp=0;assert(card.hp==17);
    s.phase=TERA_GAME_CREATE;
    assert(tera_game_view(&s,&v));assert(v.creation==&s.active.creation);
    assert(v.characters==&s.active.creation.state);
    s.phase=TERA_GAME_NAVIGATION;s.active.navigation.phase=TERA_NAV_DAY;
    s.active.navigation.day.phase=TERA_DAY_COMBAT;
    s.active.navigation.day.combat.game.position=123;
    before=s;
    assert(tera_game_view(&s,&v));assert(v.combat==&s.active.navigation.day.combat);
    assert(v.combat->game.position==123);assert(memcmp(&before,&s,sizeof s)==0);
    s.phase=TERA_GAME_ALFOL;s.active.alfol.phase=TERA_ALFOL_DAY;s.active.alfol.fixed_combat=1;
    assert(tera_game_view(&s,&v));assert(v.combat==&s.active.alfol.combat);
    s.phase=TERA_GAME_MEDUZ;s.active.meduz.phase=TERA_MEDUZ_DAY;s.active.meduz.fixed_combat=1;
    assert(tera_game_view(&s,&v));assert(v.combat==&s.active.meduz.combat);
    s.phase=TERA_GAME_AMARANDE;s.active.amarande.phase=TERA_AM_COMBAT;
    assert(tera_game_view(&s,&v));assert(v.combat==&s.active.amarande.combat);
    s.active.amarande.phase=TERA_AM_CONFIRM;s.active.amarande.command='H';
    s.active.amarande.choice='D';before=s;
    assert(tera_game_view(&s,&v) && v.prompt.command=='H' && v.prompt.choice=='D');
    assert(v.prompt.kind==TERA_PROMPT_CONFIRM && !memcmp(&s,&before,sizeof s));
    s.phase=TERA_GAME_SPACE;
    assert(tera_game_view(&s,&v));assert(v.space==&s.active.space);
    s.phase=TERA_GAME_LIFE;s.active.life.phase=TERA_LIFE_CREATE;
    assert(tera_game_view(&s,&v));assert(v.creation==&s.active.life.creation);
    s.phase=TERA_GAME_NAVIGATION;s.active.navigation.phase=TERA_NAV_CHEST;
    assert(tera_game_view(&s,&v));assert(v.exploration==&s.active.navigation.chest.state);
    s.phase=TERA_GAME_DIALOGUE;s.active.dialogue.phase=TERA_DIALOGUE_TRANSFER;
    assert(tera_game_view(&s,&v));assert(v.characters==&s.active.dialogue.transfer.state);
    return 0;
}
