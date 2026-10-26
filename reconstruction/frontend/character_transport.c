#include "character_transport.h"
#include <stdio.h>
#include <string.h>

int tera_frontend_read_int(int *v, int low, int high)
{
    return scanf("%d", v) == 1 && *v >= low && *v <= high;
}

int tera_frontend_read_characters(TeraCharacterState *s, TeraNameVariants *variants, int *initialize)
{
    int v, id, j, group;
    unsigned low, high;
    memset(s, 0, sizeof *s);
#define READ(field, lo, hi) do { if (!tera_frontend_read_int(&v, lo, hi)) return 0; field = v; } while (0)
    READ(*initialize, 0, 1); READ(low, 0, 65535); READ(high, 0, 65535);
    s->combat.random.seed = low | ((uint32_t)high << 16);
    READ(s->combat.money, -32768, 32767); READ(s->combat.experience, -32768, 32767);
    READ(s->days, -32768, 32767); READ(s->level, -32768, 32767);
    READ(s->dummy_level, -32768, 32767); READ(s->energy, -32768, 32767);
    READ(s->combat.position, -32768, 32767); READ(s->combat.world_id, 0, 11);
    READ(s->arrival_pending, 0, 1); READ(s->combat.encounter.party_available, 0, 1);
    for (id = 0; id < 10; ++id) READ(s->keys[id], -32768, 32767);
    for (id = 0; id < 10; ++id) {
        TeraRoundActor *a = &s->combat.encounter.actors[id];
        READ(a->active, 0, 1); READ(a->hp, -32768, 32767); READ(a->magic, -32768, 32767);
        READ(a->hp_limit, -32768, 32767); READ(a->magic_limit, -32768, 32767);
        READ(a->condition[0], 0, 255); READ(a->condition[1], 0, 255);
        for (j = 0; j < 10; ++j) READ(s->names[id][j], 0, 255);
        for (j = 0; j < 10; ++j) READ(s->combat.encounter.attributes[id].stats[j], 0, 255);
        for (j = 0; j < 10; ++j) READ(s->combat.encounter.attributes[id].skills[j], 0, 255);
        for (j = 0; j < 10; ++j) READ(s->combat.equipment[id][j], 0, 255);
    }
    for (group = 0; group < 2; ++group)
        for (id = 0; id < 10; ++id)
            for (j = 0; j < 10; ++j) READ(variants->names[group][id][j], 0, 255);
#undef READ
    return 1;
}

void tera_frontend_show_creation(const TeraCreationSession *s)
{
    unsigned id, j;
    const TeraCharacterState *g = &s->state;
    printf("S %d %lu %d %d %d %d %d %d %d %u %d %d", (int)s->phase,
        (unsigned long)g->combat.random.seed, g->combat.money, g->combat.experience,
        g->days, g->level, g->dummy_level, g->energy, g->combat.position,
        g->combat.world_id, g->arrival_pending, g->combat.encounter.party_available);
    for (id = 0; id < 10; ++id) printf(" %d", g->keys[id]);
    for (id = 0; id < 10; ++id) {
        const TeraRoundActor *a = &g->combat.encounter.actors[id];
        int ended = 0;
        printf(" %u %d %d %d %d %u %u", a->active, a->hp, a->magic,
            a->hp_limit, a->magic_limit, (unsigned char)a->condition[0], (unsigned char)a->condition[1]);
        for (j = 0; j < 10; ++j) {
            ended |= g->names[id][j] == 0;
            printf(" %u", ended ? 0 : (unsigned char)g->names[id][j]);
        }
        for (j = 0; j < 10; ++j) printf(" %u", g->combat.encounter.attributes[id].stats[j]);
        for (j = 0; j < 10; ++j) printf(" %u", g->combat.encounter.attributes[id].skills[j]);
        for (j = 0; j < 10; ++j) printf(" %u", g->combat.equipment[id][j]);
    }
    putchar('\n');
    /* P expose uniquement le contexte d'affichage, separe des donnees
     * durables comparees au binaire original par ligne S. */
    printf("P %u %u %u %u\n", s->split, s->bonuses_left, s->name_length, s->attempts);
    fflush(stdout);
}

