#include "tera/world.h"
#include <stdio.h>
#include <string.h>

/* Transport prive de generation. Les 270 octets NOMPERS viennent des
 * ressources locales de l'hote, jamais d'un profil invente dans le C. */
static int read_input(TeraWorldState *s, TeraNameVariants *v,
                       unsigned *difficulty, unsigned *mode, uint32_t *limit)
{
    unsigned low, high, group, id, j, byte, length;
    unsigned long requested_limit;
    memset(s, 0, sizeof *s); memset(v, 0, sizeof *v);
    if (scanf("%u %u %u %u %lu", &low, &high, difficulty, mode,
              &requested_limit) != 5 || low > 65535 || high > 65535 ||
        requested_limit > UINT32_MAX) return 0;
    *limit = (uint32_t)requested_limit;
    s->characters.combat.random.seed = low | ((uint32_t)high << 16);
    s->placement_result_residue = -211; /* SP=FFFE du contexte DOS de reference. */
    for (group = 0; group < 3; ++group)
        for (id = 1; id < 10; ++id) {
            char *name = group == 0 ? s->characters.names[id] : v->names[group-1][id];
            if (scanf("%u", &length) != 1 || length > 9) return 0;
            for (j = 0; j < 9; ++j) {
                if (scanf("%u", &byte) != 1 || byte > 255) return 0;
                if (j < length) name[j] = (char)byte;
            }
        }
    return 1;
}

static void show_world(const TeraWorldState *s, const TeraWorldReport *r)
{
    unsigned i, j;
    printf("W %lu %lu %lu %lu %d %u %d %d %u %d %d %d %d",
        (unsigned long)s->characters.combat.random.seed, (unsigned long)r->random_calls,
        (unsigned long)r->map_attempts, (unsigned long)r->placement_collisions,
        s->characters.combat.position,s->characters.combat.world_id,
        s->characters.combat.money,s->characters.energy,s->characters.combat.difficulty,
        s->arioch_position,s->alfol_position,s->hidden_positions[0],s->hidden_positions[1]);
    for (i=0;i<9;++i) printf(" %d",s->horizontal_mountains[i]);
    for (i=0;i<25;++i) printf(" %d",s->vertical_mountains[i]);
    for (i=0;i<20;++i) printf(" %d",s->towns[i]);
    for (i=1;i<10;++i) printf(" %d",s->locations[i]);
    for (i=1;i<10;++i) printf(" %d",s->extra_locations[i]);
    putchar('\n');
    printf("F");
    for (i=0;i<10;++i) printf(" %d",s->characters.keys[i]);
    for (i=0;i<11;++i) printf(" %u",s->has_map[i]);
    for (i=0;i<4;++i) printf(" %u",s->tasks_done[i]);
    printf(" %d %d %d %d\n",s->fast_moves,s->initial_flag,s->mental_extension,s->tasks_counter);
    printf("M");
    for (i=0;i<1100;++i) for (j=0;j<7;++j) printf(" %u",s->maps[i][j]);
    putchar('\n');
    printf("H");
    for (i=0;i<38;++i) for (j=0;j<40;++j) printf(" %u",(unsigned char)s->hints[i][j]);
    putchar('\n');
    printf("K");
    for (i=0;i<4;++i) for (j=0;j<10;++j) printf(" %u",(unsigned char)s->secret_words[i][j]);
    for (i=0;i<4;++i) printf(" %u",s->characters.combat.helpers[i]);
    putchar('\n');
    printf("A");
    for (i=0;i<10;++i) {
        const TeraRoundActor *a=&s->characters.combat.encounter.actors[i];
        printf(" %u %d %d %d %d %u %u",a->active,a->hp,a->magic,a->hp_limit,a->magic_limit,
               (unsigned char)a->condition[0],(unsigned char)a->condition[1]);
        for (j=0;j<10;++j) printf(" %u",(unsigned char)s->characters.names[i][j]);
        for (j=0;j<10;++j) printf(" %u",s->characters.combat.encounter.attributes[i].stats[j]);
        for (j=0;j<10;++j) printf(" %u",s->characters.combat.encounter.attributes[i].skills[j]);
        for (j=0;j<10;++j) printf(" %u",s->characters.combat.equipment[i][j]);
    }
    putchar('\n');
}

int main(void)
{
    TeraWorldState s;
    TeraNameVariants v;
    TeraWorldReport r;
    TeraWorldReport places;
    unsigned difficulty,mode;
    uint32_t limit;
    if (!read_input(&s,&v,&difficulty,&mode,&limit) || mode>1 ||
        !tera_world_generate_maps(&s,difficulty,limit,&r)) {
        fputs("Generation invalide ou limite technique atteinte\n",stderr); return 2;
    }
    if (mode==1) {
        if (!tera_world_initialize_places(&s,&v,limit,&places)) {
            fputs("Initialisation des lieux incomplete\n",stderr);return 3;
        }
        r.random_calls+=places.random_calls;
        r.placement_collisions+=places.placement_collisions;
    }
    show_world(&s,&r);
    return 0;
}
