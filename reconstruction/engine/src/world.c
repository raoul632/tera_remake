#include "tera/world.h"
#include <stddef.h>
#include <string.h>

typedef struct {
    TeraWorldState *world;
    TeraWorldReport report;
    uint32_t limit;
} Generator;

static unsigned random_draw(Generator *g, unsigned bound)
{
    unsigned value = 0;
    if (g->report.random_calls >= g->limit) {
        g->report.exhausted = 1;
        return 0;
    }
    ++g->report.random_calls;
    tera_random_draw(&g->world->characters.combat.random, bound, &value);
    return value;
}

/* OVL0000:BB8C : second jet uniquement si le premier n'a pas donne 1. */
static uint8_t terrain(Generator *g)
{
    if (random_draw(g, 9) == 1)
        return 3;
    return (uint8_t)(random_draw(g, 4) == 1 ? 2 : 1);
}

static void carve_world(Generator *g, unsigned world, unsigned *reverse)
{
    static const int deltas[4] = {1, -10, -1, 10};
    static const unsigned walls[4] = {0, 1, 2, 3};
    static const unsigned opposite[4] = {2, 3, 0, 1};
    unsigned step, direction = 0, position = world*100;
    uint8_t wall;
    g->world->maps[position][6] = terrain(g);
    for (step = 0; step < 225 && !g->report.exhausted; ++step) {
        do {
            direction = random_draw(g, 4);
            if (g->report.exhausted) return;
        } while (direction == *reverse || (direction == 0 && position%10 == 9) ||
                 (direction == 1 && position < world*100+10) ||
                 (direction == 2 && position%10 == 0) ||
                 (direction == 3 && position > world*100+89));
        *reverse = opposite[direction];
        wall = terrain(g);
        g->world->maps[position][walls[direction]] = wall;
        position = (unsigned)((int)position + deltas[direction]);
        g->world->maps[position][opposite[direction]] = wall;
        g->world->maps[position][6] = terrain(g);
    }
    g->world->characters.combat.position = (int16_t)position;
    /* Corrections de raccords appliquees apres CHAQUE marche de monde. */
    g->world->maps[100][2]=2; g->world->maps[0][0]=2;
    g->world->maps[900][2]=1; g->world->maps[1000][2]=1;
    g->world->maps[1][2]=2; g->world->maps[800][2]=2;
    g->world->maps[0][3]=2; g->world->maps[800][0]=3;
    g->world->maps[10][1]=2; g->world->maps[801][2]=3;
    g->world->maps[810][1]=3; g->world->maps[800][3]=3;
}

static unsigned normalize_item(unsigned item)
{
    return item == 21 || item == 18 || item == 17 ? 25 : item;
}

/* BD96 : les AND Pascal ne court-circuitent PAS les RANDOM(2).
 * Les tirer avant les tests de drapeaux pour conserver l'ordre du BIN. */
static void populate_maps(Generator *g, unsigned difficulty)
{
    uint8_t flags[9][6] = {{0}};
    unsigned demon = 1, town_marker = 1, monster = 0, item, jet;
    int i;
    for (i = 899; i >= 1 && !g->report.exhausted; --i) {
        unsigned world = (unsigned)i/100;
        uint8_t *cell = g->world->maps[i];
        if (random_draw(g, 13-difficulty) == 2 && i != 800) {
            if (i < 200) monster = random_draw(g, 11)+1;
            if (i > 200) {
                monster = random_draw(g, 13)+8;
                if (monster >= 12 && monster <= 14) monster = 0;
            }
            /* i=200 conserve BBP_08 du passage precedent, comme dans le COM. */
        } else monster = 0;
        if (monster && i != 800) {
            cell[5] = (uint8_t)monster;
            item = normalize_item(random_draw(g, 75)+1);
            cell[4] = (uint8_t)item;
        } else if (random_draw(g, 7+difficulty) == 1) {
            cell[4] = (uint8_t)normalize_item(random_draw(g, 65)+1);
        } else cell[4] = 255;
        if (i < 800) {
            jet = random_draw(g, 2);
            if (!flags[world][3] && flags[world][0] && cell[6] == 2 && jet == 0) {
                flags[world][3] = 1; cell[4] = 99;
            }
            if (i > 99) {
                jet = random_draw(g, 2);
                if (!flags[world][4] && flags[world][1] && cell[6] == 3 && jet == 0) {
                    flags[world][4] = 1; cell[4] = 103;
                }
                jet = random_draw(g, 2);
                if (!flags[world][5] && cell[6] == 1 && jet == 0 && flags[world][2]) {
                    flags[world][5] = 1; cell[4] = (uint8_t)(104+demon++);
                }
                if (!flags[world][0] && cell[6] == 2) {
                    flags[world][0] = 1;
                    if (i < 700) cell[4] = 101;
                    else { cell[5] = 21; g->world->arioch_position = (int16_t)i; }
                }
                if (!flags[world][1] && cell[6] == 3) {
                    flags[world][1] = 1; cell[4] = (uint8_t)(114+town_marker++);
                }
            } else if (!flags[0][0] && cell[0] == 3) {
                flags[0][0] = 1; cell[4] = 101;
            }
        }
        if (!flags[world][2] && cell[6] == 1) {
            flags[world][2] = 1; cell[4] = (uint8_t)(91+world);
        }
    }
    for (i = 2; i <= 7; ++i) g->world->maps[i*100][4] = 100;
    i = 900;
    do {
        --i;
        if (g->world->maps[i][6] == 3) g->world->maps[i][4] = 110;
    } while (g->world->maps[i][6] != 3 && i >= 25);
}

static unsigned acceptance_count(const TeraWorldState *s)
{
    unsigned i, count = 0, found = 0;
    for (i = 0; i < 700; ++i) count += s->maps[i][4] == 101;
    count += s->arioch_position > 699 && s->arioch_position < 800;
    for (i = 824; i <= 899; ++i) count += s->maps[i][4] == 110;
    for (i = 950; i <= 959; ++i) found |= s->maps[i][6];
    count += found != 0;
    found = 0;
    for (i = 1050; i <= 1059; ++i) found |= s->maps[i][6];
    return count + (found != 0);
}

int tera_world_generate_maps(TeraWorldState *world, unsigned difficulty,
                              uint32_t max_draws, TeraWorldReport *report)
{
    TeraWorldState next;
    Generator g;
    unsigned w, i, column, reverse, pick, marker;
    if (!world || !report || difficulty > 9 || !max_draws) return 0;
    next = *world;
    memset(&g, 0, sizeof g);
    g.world = &next; g.limit = max_draws;
    next.characters.combat.money = 500;
    next.characters.energy = 50;
    next.characters.combat.difficulty = difficulty;
    next.fast_moves = 0;
    next.initial_flag = 1;
    memset(next.has_map, 0, sizeof next.has_map);
    for (i = 0; i < 10; ++i) next.characters.keys[i] = 10;
    do {
        ++g.report.map_attempts;
        reverse = 3;
        memset(next.maps, 0, sizeof next.maps);
        for (w = 0; w < 11 && !g.report.exhausted; ++w) carve_world(&g, w, &reverse);
        if (!g.report.exhausted) populate_maps(&g, difficulty);
    } while (!g.report.exhausted && acceptance_count(&next) != 11);
    if (!g.report.exhausted) {
        for (i = 1000; i < 1100; ++i)
            for (column = 0; column < 5; ++column)
                if (next.maps[i][column] > 1) next.maps[i][column] = 1;
        for (marker = 1; marker <= 2 && !g.report.exhausted; ++marker) {
            do {
                pick = random_draw(&g, 50)+1050;
                if (marker == 1 && next.maps[pick][6]) next.maps[pick][4] = 2;
                if (marker == 2 && next.maps[pick][6] && next.maps[pick][4] != 2) next.maps[pick][4] = 3;
            } while (!g.report.exhausted && !((next.maps[pick][4] == 2 && marker == 1) || next.maps[pick][4] == 3));
        }
        do {
            pick = random_draw(&g, 50)+950;
            if (next.maps[pick][6]) next.maps[pick][4] = 1;
        } while (!g.report.exhausted && !next.maps[pick][6]);
        next.alfol_position = (int16_t)(random_draw(&g, 50)+50);
        memset(next.tasks_done, 0, sizeof next.tasks_done);
        next.characters.combat.world_id = 6; /* Verification finale FOR WorldId : 0..6. */
        for (i = 0; i < 1100; ++i) next.characters.combat.maps_column7[i] = next.maps[i][6];
    }
    *report = g.report;
    if (g.report.exhausted) return 0;
    *world = next;
    return 1;
}

int tera_world_apply_creation(TeraWorldState *world, const TeraCreationSession *session)
{
    if (!world || !session || session->phase != TERA_CREATE_DONE) return 0;
    /* OVL0022 ne remplace pas le contexte geographique de destination. */
    tera_creation_apply_combat(session, &world->characters.combat);
    memcpy(world->characters.names, session->state.names, sizeof world->characters.names);
    memcpy(world->characters.keys, session->state.keys, sizeof world->characters.keys);
    world->characters.days = session->state.days;
    world->characters.level = session->state.level;
    world->characters.dummy_level = session->state.dummy_level;
    world->characters.arrival_pending = session->state.arrival_pending;
    return 1;
}

/* Chaines Pascal String[39] : conserver les octets CP437 et la troncature,
 * sans faire dependre la longueur du texte d'un encodage UTF8 moderne. */
static void hint(TeraWorldState *s, unsigned id, const char *a, const char *b, const char *c)
{
    const char *parts[3] = {a,b,c};
    unsigned p, n = 0;
    memset(s->hints[id-1],0,40);
    for (p=0;p<3;++p)
        if (parts[p])
            while (*parts[p] && n<39) s->hints[id-1][n++]=*parts[p]++;
}

/* BA2F oublie D_A744 dans sa condition de sortie : une case occupee
 * termine quand meme. Si le resultat n'a pas ete assigne, le mot de
 * resultat preexistant revient. Le conserver, pas corriger cette collision. */
static int16_t place_companion(Generator *g, unsigned world, int16_t residue)
{
    unsigned candidate, id, collision;
    do {
        candidate = world*100+random_draw(g,99)+1;
        collision=0;
        for (id=1;id<10;++id) collision |= g->world->locations[id] == (int16_t)candidate;
        if (g->report.exhausted) return residue;
        if (g->world->maps[candidate][6] && !g->world->maps[candidate][5]) {
            if (collision) { ++g->report.placement_collisions; return residue; }
            return (int16_t)candidate;
        }
    } while (!g->report.exhausted);
    return residue;
}

static void generate_hints_and_words(Generator *g)
{
    TeraWorldState *s=g->world;
    unsigned id, choice, h[4], i, j, draw_value, helper_order[4]={1,3,2,0};
    unsigned town_a[3]={6,0,9},town_b[3]={8,7,5},actors[3]={8,5,3};
    static const char *dest_a[3]={"\x85 Balgence","dans le temple de SETH","\x85 Mortefontaine"};
    static const char *dest_b[3]={"\x85 la Porte des sables","\x85 la Roche aux Goules","\x85 Mahalore"};
    static const char *syllables[14]={"EL","A","HA","KHY","MA","KHA","RAH","THY","TOH","HAM","HAAR","LORK","RMA","LU"};
    static const char *helper_text[4]={"un talisman","un cristal","un anneau","une pierre"};
    static const unsigned helper_items[4]={6,3,9,12};
    static const char *descriptions[4]={"au controle PSY","\x85 dissiper les esprits","\x85 vaincre les cr\x82" "atures","\x85 faire fuir les d\x82" "mons"};
    char word[40];
    for (i=0;i<3;++i) {
        id=actors[i]; choice=random_draw(g,2);
        hint(s,7+i*2,"tu trouveras ",s->characters.names[id],NULL);
        hint(s,8+i*2,choice ? dest_b[i] : dest_a[i],NULL,NULL);
        s->locations[id]=(int16_t)(choice ? s->towns[town_b[i]] : (i==1 ? 800 : s->towns[town_a[i]]));
        s->extra_locations[id]=(int16_t)(choice ? (i==1 ? 800 : s->towns[town_a[i]]) : s->towns[town_b[i]]);
    }
    s->locations[4]=s->towns[1];
    s->locations[7]=place_companion(g,9,0); /* Aucun PNJ precedent n'est en monde9. */
    s->locations[9]=place_companion(g,9,s->locations[7]);
    for (i=0;i<3;++i) {
        id=i==2 ? 6 : i+1;
        hint(s,1+i*2,"tu trouveras ",s->characters.names[id],NULL);
        choice=random_draw(g,2);
        s->locations[id]=place_companion(g,choice ? 1 : 0,s->placement_result_residue);
        hint(s,2+i*2,choice ? "sous" : "dans"," la cit\x82 des cranes",NULL);
    }
    hint(s,13,"le bureau des MEDIAS se trouve",NULL,NULL);
    hint(s,14,"sur la base d'ALFOL",NULL,NULL);
    hint(s,15,"les hommes de MERENEN",NULL,NULL);
    hint(s,16,"gardent l'acc\x8a" "s de la base",NULL,NULL);
    hint(s,19,"il faut renvoyer ARIOCH au chaos",NULL,NULL);
    hint(s,20,"d'o\x97 il n'aurait jamais d\x97 sortir",NULL,NULL);
    hint(s,17,"avec beaucoup d'argent",NULL,NULL);
    hint(s,18,"tu obtiendras l'appui des MEDIAS",NULL,NULL);
    hint(s,21,"il faut L'invoquer..",NULL,NULL);
    hint(s,22,"..devant l'autel des t\x82" "n\x8a" "bres",NULL,NULL);
    for (i=0;i<4;++i) {
        unsigned first, middle, last;
        first=random_draw(g,3);
        middle=random_draw(g,6)+3;
        last=random_draw(g,5)+9;
        /* Les trois tirages sont sequencies explicitement, sans ordre
         * d'evaluation indefini des arguments d'un appel C. */
        memset(word,0,sizeof word);
        strcpy(word,syllables[first]);strcat(word,syllables[middle]);strcat(word,syllables[last]);
        /* word est deja rempli de zeros : copie des neuf octets Pascal,
         * avec troncature volontaire et terminateur conserve en position 9. */
        memset(s->secret_words[i],0,10);memcpy(s->secret_words[i],word,9);
    }
    hint(s,23,"invoquer ",s->secret_words[1],NULL);
    hint(s,24,"en haut de la pyramide",NULL,NULL);
    hint(s,33,"pour LE vaincre: crier ",s->secret_words[0],NULL);
    hint(s,34,"IL se nomme ",s->secret_words[3],NULL);
    hint(s,35,"invoquer ",s->secret_words[2]," sur MEDUZ");
    for (i=0;i<4 && !g->report.exhausted;++i) {
        do {
            draw_value=random_draw(g,4);
            for (j=0;j<i;++j) if (h[j]==draw_value) break;
        } while (!g->report.exhausted && j<i);
        h[i]=draw_value;
        hint(s,25+i*2,helper_text[h[i]]," aide ",NULL);
        hint(s,26+i*2,descriptions[i],NULL,NULL);
    }
    if (!g->report.exhausted)
        for (i=0;i<4;++i) s->characters.combat.helpers[i]=(uint8_t)helper_items[h[helper_order[i]]];
    memcpy(s->characters.combat.secret_word,s->secret_words[0],10);
}

int tera_world_initialize_places(TeraWorldState *world, const TeraNameVariants *variants,
                                  uint32_t max_draws, TeraWorldReport *report)
{
    TeraWorldState next;
    Generator g;
    unsigned i,j,position,unique;
    static const unsigned extra_ids[6]={1,2,4,6,7,9};
    if (!world || !variants || !report || max_draws<90 || world->characters.combat.difficulty>9) return 0;
    next=*world;
    memset(&g,0,sizeof g);g.world=&next;g.limit=max_draws;
    if (!tera_characters_initialize(&next.characters,variants)) return 0;
    g.report.random_calls=90;
    next.hidden_positions[0]=(int16_t)(1000+random_draw(&g,255));
    next.hidden_positions[1]=(int16_t)(1000+next.hidden_positions[0]+random_draw(&g,255)+3);
    next.horizontal_mountains[0]=(int16_t)(1000+random_draw(&g,255));
    for (i=1;i<9;++i) next.horizontal_mountains[i]=(int16_t)(next.horizontal_mountains[i-1]+30);
    for (i=0;i<5;++i) {
        next.vertical_mountains[i*5]=(int16_t)(1000+3*random_draw(&g,255));
        for (j=1;j<5;++j) next.vertical_mountains[i*5+j]=(int16_t)(next.vertical_mountains[i*5+j-1]+1);
    }
    for (i=0;i<20 && !g.report.exhausted;++i) {
        do {
            position=1000+4*random_draw(&g,225);unique=1;
            for (j=0;j<i;++j) if (next.towns[j]==(int16_t)position) unique=0;
        } while (!g.report.exhausted && !unique);
        next.towns[i]=(int16_t)position;
    }
    memset(next.locations+1,0,9*sizeof next.locations[0]);
    if (!g.report.exhausted) generate_hints_and_words(&g);
    for (i=0;i<6 && !g.report.exhausted;++i) {
        do {
            position=1000+3*random_draw(&g,255);unique=1;
            for (j=0;j<10;++j) if (next.towns[j]==(int16_t)position) unique=0;
        } while (!g.report.exhausted && !unique);
        next.extra_locations[extra_ids[i]]=(int16_t)position;
    }
    for (i=900;i<1000 && !g.report.exhausted;++i)
        if ((int16_t)i!=next.locations[7] && (int16_t)i!=next.locations[9] && next.maps[i][4]!=1)
            if (random_draw(&g,14-next.characters.combat.difficulty)==0) next.maps[i][4]=2;
    for (i=900;i<1000 && !g.report.exhausted;++i)
        if ((int16_t)i!=next.locations[9] && (int16_t)i!=next.locations[7] && next.maps[i][4]!=1) {
            unsigned jet=random_draw(&g,12-next.characters.combat.difficulty);
            if (jet==0 && next.maps[i][4]!=2) next.maps[i][5]=(uint8_t)(random_draw(&g,3)+12);
        }
    next.mental_extension=0;next.tasks_counter=0;
    *report=g.report;
    if (g.report.exhausted) return 0;
    *world=next;
    return 1;
}
