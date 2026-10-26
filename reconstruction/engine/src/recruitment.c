#include "tera/recruitment.h"
#include <stddef.h>
#include <string.h>

static int is_hs(const TeraRoundActor *a)
{
    return strcmp(a->condition,"HS")==0;
}

static int16_t signed_word(int32_t v)
{
    uint32_t bits=(uint32_t)v&UINT32_C(65535);
    return (int16_t)(bits<32768 ? (int32_t)bits : (int32_t)bits-65536);
}

unsigned tera_recruit_available(const TeraWorldState *world)
{
    unsigned id,mask=0;
    if (!world) return 0;
    for (id=1;id<10;++id)
        if (world->locations[id]==world->characters.combat.position &&
            !world->characters.combat.encounter.actors[id].active &&
            !is_hs(&world->characters.combat.encounter.actors[id])) mask |= 1u<<id;
    return mask;
}

int tera_recruit_start(TeraRecruitSession *s, const TeraWorldState *world,
                        TeraRecruitRequest request, int ready)
{
    TeraWorldState copy;
    if (!s || !world || (request!=TERA_RECRUIT_SOLICIT && request!=TERA_RECRUIT_SEDUCE) ||
        (ready!=0 && ready!=1)) return 0;
    copy=*world;
    memset(s,0,sizeof *s);s->world=copy;s->request=request;s->ready=ready;
    s->negotiator=-1;
    s->world.characters.combat.recruit_ready=ready;
    return 1;
}

static int finish(TeraRecruitSession *s)
{
    TeraPresentationEvent e;
    s->world.characters.combat.recruit_ready=s->ready;
    e=tera_presentation_make(&s->world.characters.combat,TERA_PRESENT_RECRUIT,TERA_SOURCE_DIALOGUE);
    e.data.recruit.outcome=(int)s->outcome;e.data.recruit.negotiator=s->negotiator;
    e.data.recruit.available_mask=s->available_mask;e.data.recruit.joined_mask=s->joined_mask;
    tera_presentation_emit(&s->world.characters.combat,&e);
    return 1;
}

int tera_recruit_key(TeraRecruitSession *s, unsigned key)
{
    TeraCombatGame *game;
    unsigned id,jet;
    if (!s || key>255 || s->outcome!=TERA_RECRUIT_WAIT) return 0;
    if (key>='a' && key<='z') key-=32;
    if (key!='O' && key!=13) { s->outcome=TERA_RECRUIT_CANCELLED;return finish(s); }
    game=&s->world.characters.combat;
    if (s->request==TERA_RECRUIT_SOLICIT) {
        s->available_mask=tera_recruit_available(&s->world);
        s->ready=s->available_mask!=0;
        s->outcome=s->ready ? TERA_RECRUIT_AVAILABLE : TERA_RECRUIT_NOBODY;
        return finish(s);
    }
    if (!s->ready) {
        tera_random_draw(&game->random,3,&jet);
        tera_presentation_message(game,TERA_SOURCE_DIALOGUE,TERA_TEXT_COMMAND_RESPONSE,'S',jet);
        s->rolls=1;s->outcome=TERA_RECRUIT_NO_CONTEXT;
        return finish(s);
    }
    s->ready=0;
    s->initial_party_count=1;
    for (id=1;id<10;++id) s->initial_party_count+=game->encounter.actors[id].active!=0;
    if (s->initial_party_count>=4) { s->outcome=TERA_RECRUIT_FULL;return finish(s); }
    /* B6D4 consomme les dix jets avant de tester le dernier succes,
     * meme pour un inactif : un court-circuit decalerait toute la partie. */
    for (id=0;id<10;++id) {
        int16_t roll;
        tera_random_draw(&game->random,15,&jet);
        ++s->rolls;
        tera_combat_base_roll(&game->encounter.attributes[id],8,8,jet+3,&roll);
        if (game->encounter.actors[id].active && !is_hs(&game->encounter.actors[id]) &&
            (roll>0 || s->world.locations[8]==game->position)) {
            s->ready=1;s->negotiator=(int)id;
        }
    }
    if (s->ready) {
        s->available_mask=tera_recruit_available(&s->world);
        for (id=1;id<10;++id)
            if (s->available_mask&(1u<<id)) {
                game->encounter.actors[id].active=1;s->joined_mask|=1u<<id;
            }
        game->experience=signed_word((int32_t)game->experience+5);
        s->ready=0;s->outcome=TERA_RECRUIT_NEGOTIATED;
    } else if (game->money>249) {
        /* Le paiement omet le filtre HS de C_A074 : conserver ce cas. */
        game->money=signed_word((int32_t)game->money-250);
        for (id=1;id<10;++id)
            if (s->world.locations[id]==game->position) {
                if (!game->encounter.actors[id].active) s->joined_mask|=1u<<id;
                game->encounter.actors[id].active=1;
            }
        s->outcome=TERA_RECRUIT_PAID;
    } else s->outcome=TERA_RECRUIT_TOO_POOR;
    return finish(s);
}
