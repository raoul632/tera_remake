#include "tera/navigation.h"
#include "module_test_support.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Regression BIN BA2F/C724A : les inscriptions 104..113 ont le meme
 * calendrier de 48 paires R2/R3 que les fenetres >113. Pas le vide 255.
 * Le controle prepare sa graine APRES ce dessin et execute le meme jour. */
static void test_inscription_rng(void)
{
    unsigned content;
    for(content=104;content<=114;++content) {
        TeraExplorationState world=module_fixture(),control;
        TeraNavigationSession a,b;
        world.navigation_overlay=1;world.encounter_suppressed=1;
        world.world.characters.combat.world_id=1;
        world.world.characters.combat.position=145;world.world.characters.combat.previous_position=144;
        world.world.maps[145][4]=(uint8_t)content;control=world;
        control.world.maps[145][4]=255;
        control.world.characters.combat.random.seed=seed_after(1986,96);
        assert(tera_navigation_start(&a,&world,0,1,1200,NULL,NULL));
        assert(tera_navigation_start(&b,&control,0,1,1200,NULL,NULL));
        assert(a.phase==TERA_NAV_DONE && b.phase==TERA_NAV_DONE);
        assert(memcmp(&a.state.world.characters,&b.state.world.characters,sizeof a.state.world.characters)==0);
    }
}
/* BIN A1C3/A1CA : [12..14,16] exclut aussi 13. La graine choisie donne
 * R20=0, R17=12 (type13), R7!=1 ; ce jour ne doit ouvrir aucun combat. */
static void test_excluded_random_monster13(void)
{
    TeraExplorationState world=module_fixture();TeraDaySession day;
    TeraCombatGame *g=&world.world.characters.combat;
    g->world_id=1;g->position=145;g->difficulty=0;g->random.seed=15595;
    world.world.maps[145][4]=255;
    assert(tera_day_start(&day,&world,NULL,NULL));
    assert(day.phase==TERA_DAY_DONE && day.encounters==0 && day.random_calls==3);
    assert(day.state.world.characters.combat.random.seed==seed_after(15595,3));
}

/* Contrats : les portes modifient seulement la case courante ; une
 * annulation garde la cle/position et les champs hors interaction. */
int main(void)
{
    test_inscription_rng();test_excluded_random_monster13();
    TeraExplorationState world={0};
    TeraNavigationSession session,before;
    TeraCombatGame *game=&world.world.characters.combat;
    unsigned i;
    game->world_id=2;game->position=245;game->previous_position=244;
    game->difficulty=1;game->random.seed=1986;world.navigation_overlay=1;
    world.world.maps[245][0]=3;world.world.maps[245][1]=3;world.world.maps[245][2]=1;
    world.world.maps[246][6]=1;world.world.characters.keys[2]=10;
    assert(tera_navigation_start(&session,&world,'8',0,1500,NULL,NULL));
    assert(session.phase==TERA_NAV_DONE && !session.moved);
    world.world.characters.keys[2]=2;
    assert(tera_navigation_start(&session,&world,'8',0,1500,NULL,NULL));
    assert(session.phase==TERA_NAV_GATE);
    assert(tera_navigation_key(&session,13)); /* Entree ne remplace pas U. */
    assert(!session.moved && session.state.world.maps[245][0]==3);
    assert(session.state.world.characters.combat.random.seed==1986);
    assert(tera_navigation_start(&session,&world,'8',0,1500,NULL,NULL));
    assert(tera_navigation_key(&session,'u'));
    assert(session.phase==TERA_NAV_DONE && session.moved);
    assert(session.state.world.maps[245][0]==2 && session.state.world.maps[245][1]==2);
    assert(session.state.world.maps[245][2]==1 && session.state.world.maps[246][2]==0);
    assert(session.state.world.characters.keys[2]==2);
    assert(session.state.world.characters.combat.position==246);
    assert(session.state.world.characters.combat.previous_position==245);

    /* Le niveau change, l'overlay reste 0049 lors de M depuis le niveau1. */
    game->world_id=1;game->position=100;game->previous_position=99;
    world.world.maps[100][4]=100;world.world.maps[45][4]=101;
    assert(tera_navigation_start(&session,&world,'M',0,1500,NULL,NULL));
    assert(session.phase==TERA_NAV_CONFIRM);
    assert(tera_navigation_key(&session,'O'));
    assert(session.state.world.characters.combat.world_id==0);
    assert(session.state.navigation_overlay==1);
    assert(session.state.world.characters.combat.position==45);

    /* Les indices ne consomment pas un jour ; deux indexes Pascal 1..38. */
    game->world_id=2;game->position=245;world.world.maps[245][4]=104;
    assert(tera_navigation_start(&session,&world,'L',0,1500,NULL,NULL));
    assert(tera_navigation_key(&session,'O'));
    assert(session.hint_count==2 && session.hints[0]==29 && session.hints[1]==30);
    assert(session.state.world.characters.combat.random.seed==1986);

    /* L'autel restaure les points, pas le poison : C4CF9 preserve EM. */
    world.world.maps[245][4]=103;
    game->encounter.actors[0].active=1;
    strcpy(game->encounter.actors[0].condition,"EM");
    for(i=1;i<10;++i) game->encounter.attributes[0].stats[i]=12;
    game->encounter.attributes[0].skills[6]=5;
    assert(tera_navigation_start(&session,&world,'I',0,1500,NULL,NULL));
    assert(tera_navigation_key(&session,13));
    assert(session.healed_mask==1 && session.state.world.characters.combat.encounter.actors[0].hp==36);
    assert(strcmp(session.state.world.characters.combat.encounter.actors[0].condition,"EM")==0);
    assert(session.state.world.characters.days==world.world.characters.days);

    /* Annuler l'autel passe a la saisie de l'invocation generique originale. */
    assert(tera_navigation_start(&session,&world,'I',0,1500,NULL,NULL));
    assert(tera_navigation_key(&session,'N') && session.phase==TERA_NAV_WORD);
    assert(tera_navigation_key(&session,'7') && session.word_length==0);
    assert(tera_navigation_key(&session,'a') && session.word_length==1);
    assert(tera_navigation_key(&session,8) && session.word_length==0);
    assert(tera_navigation_key(&session,13) && session.phase==TERA_NAV_DONE);
    before=session;
    assert(!tera_navigation_key(&session,'O') && memcmp(&before,&session,sizeof session)==0);
    assert(!tera_navigation_start(&session,&world,'X',0,1500,NULL,NULL));
    assert(memcmp(&before,&session,sizeof session)==0);

    game->world_id=11;game->position=1500;world.town_found=1;world.town_id=2;
    world.city_backup_position=1600;
    assert(tera_navigation_enter(&session,&world,NULL,NULL));
    assert(tera_navigation_key(&session,'N'));
    assert(session.backup_position==1600 && session.state.world.characters.combat.position==1500);
    game->world_id=0;game->position=0;world.navigation_overlay=0;
    assert(tera_navigation_leave(&world,1600));
    assert(game->world_id==11 && game->position==1600);
    puts("City navigation/location contracts passed");
    return 0;
}
