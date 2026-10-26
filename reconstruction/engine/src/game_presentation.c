#include "tera/game.h"

static void bind_day(TeraDaySession *day,const TeraPresentationSink *sink)
{
    day->state.world.characters.combat.presentation=sink;day->combat.game.presentation=sink;
}
int tera_game_set_presentation(TeraGameSession *s,const TeraPresentationSink *sink)
{
    if(!s) return 0;
    s->state.world.characters.combat.presentation=sink;
    switch(s->phase) {
    case TERA_GAME_CREATE:s->active.creation.state.combat.presentation=sink;break;
    case TERA_GAME_DAY:bind_day(&s->active.day,sink);break;
    case TERA_GAME_AMARANDE:
        s->active.amarande.state.world.characters.combat.presentation=sink;
        s->active.amarande.combat.game.presentation=sink;bind_day(&s->active.amarande.day,sink);break;
    case TERA_GAME_NAVIGATION:
        s->active.navigation.state.world.characters.combat.presentation=sink;
        s->active.navigation.chest.state.world.characters.combat.presentation=sink;
        bind_day(&s->active.navigation.day,sink);break;
    case TERA_GAME_ALFOL:
        s->active.alfol.state.world.characters.combat.presentation=sink;
        s->active.alfol.combat.game.presentation=sink;bind_day(&s->active.alfol.day,sink);break;
    case TERA_GAME_MEDUZ:
        s->active.meduz.state.world.characters.combat.presentation=sink;
        s->active.meduz.combat.game.presentation=sink;bind_day(&s->active.meduz.day,sink);break;
    case TERA_GAME_SHOP:s->active.shop.state.world.characters.combat.presentation=sink;break;
    case TERA_GAME_SPACE:s->active.space.state.combat.presentation=sink;break;
    case TERA_GAME_DIALOGUE:
        s->active.dialogue.state.world.characters.combat.presentation=sink;
        s->active.dialogue.transfer.state.combat.presentation=sink;
        s->active.dialogue.recruitment.world.characters.combat.presentation=sink;break;
    case TERA_GAME_INVENTORY:s->active.inventory.state.combat.presentation=sink;break;
    case TERA_GAME_LIFE:
        s->active.life.state.world.characters.combat.presentation=sink;
        s->active.life.creation.state.combat.presentation=sink;break;
    case TERA_GAME_ENDING:s->active.ending.state.world.characters.combat.presentation=sink;break;
    default:break;
    }
    return 1;
}
