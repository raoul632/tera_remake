#include "tera/recruitment.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Contrats d'integration : abandon atomique du generateur, reintegration
 * limitee de l'overlay heros et particularites du recrutement resident. */
int main(void)
{
    TeraWorldState world = {0}, before;
    TeraWorldReport report;
    TeraNameVariants names = {{{{0}}}};
    TeraRecruitSession recruit, saved;
    TeraCreationSession hero = {0};
    TeraRandom expected;
    unsigned i, value;
    world.characters.combat.random.seed = 1986;
    before = world;
    assert(!tera_world_generate_maps(&world, 10, 100000, &report));
    assert(memcmp(&world, &before, sizeof world) == 0);
    assert(!tera_world_generate_maps(&world, 1, 5, &report));
    assert(report.exhausted && report.random_calls == 5);
    assert(memcmp(&world, &before, sizeof world) == 0);
    assert(tera_world_generate_maps(&world, 1, 2000000, &report));
    assert(report.map_attempts && !report.exhausted);
    for (i=0;i<1100;++i)
        assert(world.maps[i][6] == world.characters.combat.maps_column7[i]);
    before = world;
    assert(!tera_world_initialize_places(&world, &names, 90, &report));
    assert(report.exhausted && report.random_calls == 90);
    assert(memcmp(&world, &before, sizeof world) == 0);
    hero.phase = TERA_CREATE_DONE;
    hero.state.combat.random.seed = 27;
    hero.state.days = 7300;
    assert(tera_world_apply_creation(&world, &hero));
    assert(world.characters.combat.random.seed == 27 && world.characters.days == 7300);
    assert(memcmp(world.maps, before.maps, sizeof world.maps) == 0);
    assert(memcmp(world.characters.combat.maps_column7,
                  before.characters.combat.maps_column7, 1100) == 0);
    assert(world.characters.combat.money == hero.state.combat.money);
    assert(world.characters.combat.difficulty == before.characters.combat.difficulty);

    memset(&world,0,sizeof world);
    world.characters.combat.position = 957;
    world.characters.combat.money = 500;
    world.characters.combat.random.seed = 1986;
    world.locations[7] = world.locations[9] = 957;
    world.characters.combat.encounter.actors[0].active = 1;
    /* HP ne decide pas de la disponibilite : seul le statut HS l'exclut. */
    assert(tera_recruit_available(&world) == ((1u<<7)|(1u<<9)));
    assert(tera_recruit_start(&recruit,&world,TERA_RECRUIT_SOLICIT,0));
    assert(tera_recruit_key(&recruit,13));
    assert(recruit.ready && recruit.rolls == 0 && recruit.world.characters.combat.random.seed == 1986);
    assert(recruit.world.characters.combat.recruit_ready == 1);
    world = recruit.world;
    assert(tera_recruit_start(&recruit,&world,TERA_RECRUIT_SEDUCE,1));
    assert(tera_recruit_key(&recruit,'N'));
    assert(recruit.ready && recruit.outcome == TERA_RECRUIT_CANCELLED);
    saved = recruit;
    assert(!tera_recruit_key(&recruit,'O'));
    assert(memcmp(&saved,&recruit,sizeof recruit) == 0);

    /* Dix jets meme avec un seul actif ; paiement active aussi les HS. */
    strcpy(world.characters.combat.encounter.actors[9].condition,"HS");
    expected = world.characters.combat.random;
    for (i=0;i<10;++i) assert(tera_random_draw(&expected,15,&value));
    assert(tera_recruit_start(&recruit,&world,TERA_RECRUIT_SEDUCE,1));
    assert(tera_recruit_key(&recruit,'o'));
    assert(recruit.outcome == TERA_RECRUIT_PAID && recruit.rolls == 10);
    assert(recruit.world.characters.combat.money == 250);
    assert(recruit.world.characters.combat.random.seed == expected.seed);
    assert(recruit.joined_mask == ((1u<<7)|(1u<<9)));
    assert(strcmp(recruit.world.characters.combat.encounter.actors[9].condition,"HS") == 0);
    assert(!recruit.world.characters.combat.recruit_ready);

    /* Le succes lie au lieu du personnage 8 conserve les dix jets et XP
     * deborde comme un Integer DOS. Plusieurs recrues passent ensemble. */
    world.locations[8] = 957;
    world.characters.combat.experience = 32767;
    assert(tera_recruit_start(&recruit,&world,TERA_RECRUIT_SEDUCE,1));
    assert(tera_recruit_key(&recruit,'O'));
    assert(recruit.outcome == TERA_RECRUIT_NEGOTIATED && recruit.rolls == 10);
    assert(recruit.world.characters.combat.experience == -32764);
    assert(recruit.world.characters.combat.money == 500);
    assert(recruit.joined_mask == ((1u<<7)|(1u<<8)));
    world.characters.combat.encounter.actors[1].active = 1;
    world.characters.combat.encounter.actors[2].active = 1;
    world.characters.combat.encounter.actors[3].active = 1;
    assert(tera_recruit_start(&recruit,&world,TERA_RECRUIT_SEDUCE,1));
    assert(tera_recruit_key(&recruit,'O'));
    assert(recruit.outcome == TERA_RECRUIT_FULL && recruit.rolls == 0);
    assert(!recruit.ready && recruit.world.characters.combat.random.seed == 1986);
    puts("World/recruitment integration contracts passed");
    return 0;
}
