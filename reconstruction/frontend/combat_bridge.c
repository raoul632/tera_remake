#include "tera/interactive.h"
#include <stdio.h>
#include <string.h>

/* Transport de l'etat entre le frontend Python 64 bits et le moteur C,
 * independant des tailles natives et du padding des structures.
 * Protocole interne decimal documente par pack_game dans combat.py. */
static int read_value(int *value, int low, int high)
{
    return scanf("%d", value) == 1 && *value >= low && *value <= high;
}

static int read_game(TeraCombatGame *g, unsigned *aggressor)
{
    int v, i, j;
    unsigned low, high;
    memset(g, 0, sizeof *g);
#define READ(field, min, max) do { if (!read_value(&v, min, max)) return 0; field = v; } while (0)
    READ(g->encounter.monster_type, 1, 22);
    READ(*aggressor, 0, 9);
    READ(low, 0, 65535); READ(high, 0, 65535);
    g->random.seed = low | ((uint32_t)high << 16);
    READ(g->world_id, 0, 11); READ(g->difficulty, 0, 10);
    READ(g->on_alfol, 0, 1);
    READ(g->position, -32768, 32767); READ(g->previous_position, -32768, 32767);
    READ(g->money, -32768, 32767); READ(g->experience, -32768, 32767);
    READ(g->location_changed, 0, 1);
    for (i = 0; i < 4; ++i) READ(g->helpers[i], 3, 25);
    for (i = 0; i < 10; ++i) READ(g->secret_word[i], 0, 127);
    for (i = 0; i < 22; ++i)
        for (j = 0; j < 6; ++j) READ(g->monster_values[i][j], -32768, 32767);
    for (i = 0; i < 10; ++i) {
        TeraRoundActor *a = &g->encounter.actors[i];
        READ(a->active, 0, 1); READ(a->hp, -32768, 32767); READ(a->magic, -32768, 32767);
        READ(a->hp_limit, -32768, 32767); READ(a->magic_limit, -32768, 32767);
        READ(a->condition[0], 0, 127); READ(a->condition[1], 0, 127);
        for (j = 0; j < 10; ++j) READ(g->encounter.attributes[i].stats[j], 0, 255);
        for (j = 0; j < 10; ++j) READ(g->encounter.attributes[i].skills[j], 0, 255);
        for (j = 0; j < 10; ++j) READ(g->equipment[i][j], 0, 255);
    }
    for (i = 0; i < 1100; ++i) READ(g->maps_column7[i], 0, 255);
    READ(g->recruit_ready, 0, 1);
    READ(g->sound, 0, 1);
#undef READ
    return 1;
}

static void show_event(void *context, const TeraCombatEvent *e)
{
    (void)context;
    printf("E %d %u %u %d\n", (int)e->kind, e->actor, (unsigned char)e->letter, e->value);
}

/* Chaque frontiere fournit l'etat observable et la graine, pour comparer
 * egalement les annulations et reprises ; rien n'est recalcule en Python. */
static void show_state(const TeraCombatSession *s)
{
    unsigned i;
    const TeraCombatGame *g = &s->game;
    const TeraEncounterState *e = &g->encounter;
    printf("S %d %u %u %u %lu %d %d %d %d %d %d %d %d %d %d %u",
        (int)s->phase, s->actor, s->attempt, (unsigned char)s->letter,
        (unsigned long)g->random.seed, s->initial_monster_hp, e->monster_hp,
        e->victory, e->fled, e->party_available, g->position,
        g->previous_position, g->money, g->experience, g->location_changed, s->round_number);
    for (i = 0; i < 10; ++i) {
        const TeraRoundActor *a = &e->actors[i];
        printf(" %d %d %d %d %u %u", a->hp, a->magic, a->hp_limit,
            a->magic_limit, (unsigned char)a->condition[0], (unsigned char)a->condition[1]);
    }
    printf(" %u %d\n", s->bonus_item, g->recruit_ready);
    fflush(stdout);
}

int main(void)
{
    TeraCombatGame game;
    TeraCombatSession session;
    unsigned aggressor;
    int key;
    if (!read_game(&game, &aggressor) ||
        !tera_combat_start(&session, &game, aggressor, show_event, NULL)) {
        fputs("Etat de combat invalide\n", stderr);
        return 2;
    }
    show_state(&session);
    while (session.phase != TERA_COMBAT_DONE && session.phase != TERA_COMBAT_ERROR) {
        if (!read_value(&key, 0, 255)) {
            /* EOF transporte une interruption de l'hote ; ce n'est jamais
             * une annulation originale ni une victoire. Dernier S conserve. */
            return 4;
        }
        tera_combat_key(&session, (unsigned)key);
        show_state(&session);
    }
    return session.phase == TERA_COMBAT_ERROR ? 3 : 0;
}
