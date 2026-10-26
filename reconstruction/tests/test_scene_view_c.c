#include "module_test_support.h"
#include "tera/game.h"
#include "tera/save.h"
#include <stdio.h>

int main(void)
{
    static const int headings[]={1,-1,10,-10};
    static const unsigned faces[4][3]={{1,0,3},{3,2,1},{0,3,2},{2,1,0}};
    TeraGameSession s={0},before,loaded;TeraGameView v;unsigned i,j;
    TeraMoveReport report;TeraExplorationState e=module_fixture();
    unsigned char bytes[TERA_SAVE_CAPACITY];size_t size;
    s.phase=TERA_GAME_READY;s.state=e;
    s.state.world.characters.combat.world_id=2;s.state.world.characters.combat.position=245;
    s.state.navigation_overlay=1;
    for(j=0;j<4;++j) s.state.world.maps[245][j]=(uint8_t)j;
    for(i=0;i<4;++i) {
        s.state.world.characters.combat.previous_position=(int16_t)(245-headings[i]);before=s;
        assert(tera_game_view(&s,&v) && v.scene.heading_valid);
        assert(v.scene.left==faces[i][0] && v.scene.front==faces[i][1] && v.scene.right==faces[i][2]);
        assert(!memcmp(&s,&before,sizeof s));
    }
    for(i=0;i<100;++i) {
        s.state.world.maps[245][4]=(uint8_t)i;assert(tera_game_view(&s,&v));
        assert(v.scene.feature==TERA_FEATURE_CHEST && !v.scene.visible_item);
    }
    s.state.chest_open=1;s.state.world.maps[245][4]=17;
    assert(tera_game_view(&s,&v));assert(v.scene.feature==TERA_FEATURE_OPEN_CHEST && v.scene.visible_item==17);
    s.state.chest_open=0;s.state.world.maps[245][4]=103;
    assert(tera_game_view(&s,&v));assert(v.scene.feature==TERA_FEATURE_ALTAR);
    s.state.world.maps[245][4]=255;
    assert(tera_game_view(&s,&v));assert(v.scene.feature==TERA_FEATURE_NONE);
    s.state.world.characters.combat.world_id=10;s.state.world.characters.combat.position=1001;
    s.state.world.maps[1001][4]=2;assert(tera_game_view(&s,&v));
    assert(v.scene.kind==TERA_SCENE_MEDUZ && v.scene.feature==TERA_FEATURE_MEDITATION);
    s.state.world.characters.combat.world_id=9;s.state.world.characters.combat.position=900;
    s.state.world.characters.combat.on_alfol=1;s.state.world.maps[900][4]=2;
    assert(tera_game_view(&s,&v));assert(v.scene.kind==TERA_SCENE_ALFOL_INTERIOR && v.scene.feature==TERA_FEATURE_RADIATION);
    s.state.world.characters.combat.world_id=11;s.state.world.characters.combat.position=0;
    assert(tera_game_view(&s,&v));assert(v.scene.kind==TERA_SCENE_ALFOL_SURFACE);
    /* Vecteur OVL00FB, case1201 %23=5 : trois blocs de 13 sprites,
     * 78 tirages de sol puis 34 d'horizon. Aucun effet d'une consultation. */
    for(i=0;i<20;++i) e.world.towns[i]=-3000;
    assert(tera_exploration_move(&e,'8',&report));
    assert(report.landscape.count==39 && report.random_calls==112);
    assert(e.world.characters.combat.random.seed==seed_after(1986,112));
    for(i=0;i<report.landscape.count;++i) {
        assert(report.landscape.ground[i].x<76);
        assert(report.landscape.ground[i].y>=148 && report.landscape.ground[i].y<=186);
    }
    memset(&s,0,sizeof s);s.phase=TERA_GAME_READY;s.state=e;s.move=report;
    before=s;assert(tera_game_view(&s,&v));assert(v.scene.landscape==&s.move.landscape);
    assert(!memcmp(&s,&before,sizeof s));
    assert(tera_save_encode(&s,bytes,sizeof bytes,&size)==TERA_SAVE_OK && size==11521);
    assert(tera_save_decode(&loaded,bytes,size,NULL,NULL,NULL)==TERA_SAVE_OK);
    assert(tera_game_view(&loaded,&v) && !v.scene.landscape);
    s.state.world.horizontal_mountains[0]=s.state.world.characters.combat.position;
    assert(tera_game_key(&s,'8') && !s.move.moved);
    assert(!memcmp(&s.move.landscape,&report.landscape,sizeof report.landscape));
    puts("Scene projection, hidden contents, RNG and save boundaries OK");return 0;
}
