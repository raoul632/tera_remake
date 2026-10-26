#include "tera/creation.h"
#include <stddef.h>
#include <string.h>

int tera_character_card(const TeraCharacterState *s,const TeraCombatGame *live,
    unsigned id,TeraCharacterCard *out)
{
    TeraCharacterCard c;const TeraCombatGame *g;const TeraRoundActor *a;unsigned i;
    if(!s || !out || id>=10) return 0;
    g=live ? live : &s->combat;a=&g->encounter.actors[id];memset(&c,0,sizeof c);
    c.actor=id;memcpy(c.name,s->names[id],sizeof c.name);c.name[9]=0;
    memcpy(c.condition,a->condition,3);c.condition[2]=0;
    memcpy(c.stats,g->encounter.attributes[id].stats,10);
    memcpy(c.skills,g->encounter.attributes[id].skills,10);
    for(i=0;i<10;++i) c.equipment[i]=g->equipment[id][i];
    c.hp=a->hp;c.magic=a->magic;c.hp_limit=a->hp_limit;c.magic_limit=a->magic_limit;
    c.key=s->keys[id];c.days=s->days;c.money=g->money;c.experience=g->experience;
    *out=c;return 1;
}

static unsigned draw(TeraRandom *rng, unsigned bound)
{
    unsigned result = 0;
    tera_random_draw(rng, bound, &result);
    return result;
}

static int names_valid(const void *names)
{
    const char *bytes = (const char *)names;
    unsigned id;
    for (id = 0; id < 10; ++id)
        if (!memchr(bytes + id*10, 0, 10))
            return 0;
    return 1;
}

int tera_characters_initialize(TeraCharacterState *s, const TeraNameVariants *variants)
{
    unsigned id, stat, variant;
    static const unsigned extra_ids[5] = {3, 6, 8, 7, 4};
    static const unsigned extra_stats[5] = {5, 2, 5, 9, 9};
    if (!s || !variants || !names_valid(s->names) ||
        !names_valid(variants->names[0]) || !names_valid(variants->names[1]))
        return 0;
    memset(s->combat.equipment, 0, sizeof s->combat.equipment);
    memset(s->combat.encounter.attributes, 0, sizeof s->combat.encounter.attributes);
    /* Ne pas intercaler stats et noms : le BIN tire d'abord neuf noms,
     * puis neuf stats pour CHAQUE compagnon, meme la specialite ecrasee. */
    for (id = 1; id < 10; ++id) {
        variant = draw(&s->combat.random, 3);
        if (variant)
            memcpy(s->names[id], variants->names[variant-1][id], 10);
    }
    for (id = 0; id < 10; ++id) {
        memcpy(s->combat.encounter.actors[id].condition, "OK", 3);
        s->combat.encounter.actors[id].active = id == 0;
        if (id)
            s->combat.encounter.attributes[id].skills[id] = 3;
    }
    for (id = 1; id < 10; ++id) {
        for (stat = 1; stat < 10; ++stat)
            s->combat.encounter.attributes[id].stats[stat] = (uint8_t)(2 + draw(&s->combat.random, 6));
        s->combat.encounter.attributes[id].stats[id] = 9;
    }
    for (id = 0; id < 5; ++id) {
        s->combat.encounter.attributes[extra_ids[id]].stats[extra_stats[id]] = 7;
        s->combat.encounter.attributes[extra_ids[id]].skills[extra_stats[id]] = 1;
    }
    /* OVL_0010 fin : equipements personnels, pas un partage de groupe. */
    s->combat.equipment[1][0] = 13; s->combat.equipment[1][1] = 20;
    s->combat.equipment[4][0] = 19; s->combat.equipment[4][1] = 17;
    s->combat.equipment[7][0] = 19; s->combat.equipment[7][1] = 17;
    s->combat.equipment[9][0] = 21; s->combat.equipment[9][1] = 18;
    return 1;
}

int tera_creation_start(TeraCreationSession *s, const TeraCharacterState *state)
{
    TeraCharacterState copied;
    if (!s || !state || !names_valid(state->names))
        return 0;
    /* L'hote peut relancer depuis session.state : copier avant d'effacer
     * les champs de saisie, sans perdre ses personnages par aliasing. */
    copied = *state;
    memset(s, 0, sizeof *s);
    s->state = copied;
    s->phase = TERA_CREATE_NAME;
    s->attempts = 1;
    return 1;
}

/* C446..C543 : restore implique le recalcul des maxima avant copie,
 * et concerne TOUS les personnages, meme ceux absents du groupe. */
static void accept_hero(TeraCreationSession *s)
{
    TeraCharacterState *state = &s->state;
    unsigned id;
    for (id = 0; id < 10; ++id) {
        TeraRoundActor *actor = &state->combat.encounter.actors[id];
        TeraCombatLimits limits;
        tera_combat_actor_limits(&state->combat.encounter.attributes[id], &limits);
        actor->hp = actor->hp_limit = limits.hp;
        actor->magic = actor->magic_limit = limits.magic;
        actor->agility = state->combat.encounter.attributes[id].stats[7];
        memcpy(actor->condition, "OK", 3);
        if (id)
            actor->active = 0;
    }
    if (state->combat.money < 500)
        state->combat.money = 500;
    memset(state->combat.encounter.attributes[0].skills, 0,
           sizeof state->combat.encounter.attributes[0].skills);
    state->dummy_level = 1;
    state->combat.experience = 0;
    state->days = 7300;
    state->level = 0;
    state->combat.encounter.party_available = 1;
    state->combat.position = 1000;
    state->arrival_pending = 1;
    state->combat.world_id = 11;
    state->keys[0] = 0;
    s->phase = TERA_CREATE_DONE;
}

int tera_creation_key(TeraCreationSession *s, unsigned key)
{
    TeraCombatActor *hero;
    unsigned stat = 0, allocation, pool;
    if (!s || key > 255 || s->phase == TERA_CREATE_DONE)
        return 0;
    if (key >= 'a' && key <= 'z')
        key -= 32;
    hero = &s->state.combat.encounter.attributes[0];
    switch (s->phase) {
    case TERA_CREATE_NAME:
        if (key >= 'A' && key <= 'Z')
            s->name_buffer[s->name_length++] = (char)key;
        else if (key == 8 && s->name_length)
            --s->name_length;
        s->name_buffer[s->name_length] = 0;
        if (key == 13 || s->name_length == 10) {
            s->name_buffer[9] = 0;
            memcpy(s->state.names[0], s->name_buffer, 10);
            s->phase = TERA_CREATE_GENDER;
        }
        break;
    case TERA_CREATE_GENDER:
        s->gender = key;
        hero->stats[1] = (uint8_t)(draw(&s->state.combat.random, 5) + 5);
        hero->stats[2] = (uint8_t)(hero->stats[1] > 7
            ? draw(&s->state.combat.random, 3) + 3
            : draw(&s->state.combat.random, 6) + 4);
        hero->stats[3] = (uint8_t)(18 - hero->stats[1] - hero->stats[2]);
        s->split = 1;
        s->phase = TERA_CREATE_SPLIT;
        break;
    case TERA_CREATE_SPLIT:
        if (key < '0' || key > '9')
            break; /* ReadDigit redemande, sans RNG. */
        allocation = key - '0';
        pool = hero->stats[s->split];
        if (allocation > pool - 1)
            allocation = draw(&s->state.combat.random, pool - 1) + 1;
        hero->stats[s->split+3] = (uint8_t)allocation;
        hero->stats[s->split+6] = (uint8_t)(pool - allocation);
        if (++s->split == 4) {
            stat = s->gender == 'F' ? 8 : 1;
            hero->stats[stat] = (uint8_t)(hero->stats[stat] + 2);
            s->bonuses_left = 6;
            s->phase = TERA_CREATE_BONUS;
        }
        break;
    case TERA_CREATE_BONUS:
        switch (key) {
        case 'F': stat = 1; break; case 'I': stat = 2; break;
        case 'N': stat = 3; break; case 'E': stat = 4; break;
        case 'V': stat = 5; break; case 'T': stat = 6; break;
        case 'A': stat = 7; break; case 'C': stat = 8; break;
        case 'H': stat = 9; break; default: break;
        }
        if (stat) {
            hero->stats[stat] = (uint8_t)(hero->stats[stat] + 1);
            if (--s->bonuses_left == 0)
                s->phase = TERA_CREATE_KEEP;
        }
        break;
    case TERA_CREATE_KEEP:
        if (key == 'N') {
            /* N revient au nom et retient la graine deja avancee.
             * Le roster, l'equipement et les stats presentes ne sont pas
             * effaces : les nouvelles saisies les remplaceront a leur tour. */
            s->phase = TERA_CREATE_NAME;
            s->name_length = 0;
            memset(s->name_buffer, 0, sizeof s->name_buffer);
            ++s->attempts;
        } else
            accept_hero(s);
        break;
    default: return 0;
    }
    return 1;
}

int tera_creation_apply_combat(const TeraCreationSession *s, TeraCombatGame *g)
{
    if (!s || !g || s->phase != TERA_CREATE_DONE)
        return 0;
    memcpy(g->encounter.actors, s->state.combat.encounter.actors, sizeof g->encounter.actors);
    memcpy(g->encounter.attributes, s->state.combat.encounter.attributes, sizeof g->encounter.attributes);
    memcpy(g->equipment, s->state.combat.equipment, sizeof g->equipment);
    g->random = s->state.combat.random;
    g->money = s->state.combat.money;
    g->experience = s->state.combat.experience;
    g->position = s->state.combat.position;
    g->world_id = s->state.combat.world_id;
    g->encounter.party_available = s->state.combat.encounter.party_available;
    return 1;
}
