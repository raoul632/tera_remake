#include "module_test_support.h"
#include "tera/meduz.h"
#include "original_tunnel_vectors.h"
#include <stdio.h>

static TeraExplorationState meduz_fixture(void)
{
    TeraExplorationState s=module_fixture();
    s.world.characters.combat.world_id=10;s.world.characters.combat.position=1000;
    s.world.characters.combat.previous_position=999;return s;
}
static void keys(TeraMeduzSession *s,const char *input)
{
    while(*input) assert(tera_meduz_key(s,(unsigned char)*input++));
}
static void test_meditation_and_objective(void)
{
    TeraExplorationState w=meduz_fixture();TeraMeduzSession s,before;unsigned i;
    w.world.maps[1000][4]=2;w.world.characters.combat.encounter.actors[3].active=1;
    strcpy(w.world.characters.combat.encounter.actors[3].condition,"HS");
    w.world.characters.combat.encounter.attributes[3].stats[5]=255;
    assert(tera_meduz_start(&s,&w,'M',0,0,0,1200,NULL,NULL));keys(&s,"n");
    assert(!s.state.world.mental_extension);
    assert(tera_meduz_start(&s,&w,'M',0,0,0,1200,NULL,NULL));keys(&s,"\r");
    assert(s.state.world.mental_extension && s.state.world.characters.days==7300);
    assert(s.state.world.characters.combat.random.seed==1986);
    assert(s.state.world.characters.combat.encounter.attributes[0].stats[2]==11);
    assert(!s.state.world.characters.combat.encounter.attributes[3].stats[5]);
    assert(s.state.world.characters.combat.encounter.attributes[4].stats[2]==10);
    assert(strcmp(s.state.world.characters.combat.encounter.actors[3].condition,"HS")==0);
    w=s.state;assert(tera_meduz_start(&s,&w,'M',0,0,0,1200,NULL,NULL));keys(&s,"o");
    assert(memcmp(&s.state,&w,sizeof w)==0); /* Gain unique, pas de soin. */
    w.world.maps[1000][4]=3;w.world.tasks_counter=32767;
    for(i=0;i<3;++i) {
        w.world.mental_extension=i!=0;
        assert(tera_meduz_start(&s,&w,'I',0,0,0,1200,NULL,NULL));
        keys(&s,i==1 ? "oFAUX\r" : "oMENTAX\bL\r");
        assert(s.state.world.tasks_done[0]==(i==2));
    }
    assert(s.state.world.tasks_counter==-32768 && s.state.world.characters.days==7300);
    w=s.state;assert(tera_meduz_start(&s,&w,'I',0,0,0,1200,NULL,NULL));keys(&s,"oMENTAL\r");
    assert(memcmp(&s.state,&w,sizeof w)==0);
    before=s;assert(!tera_meduz_key(&s,'O'));assert(memcmp(&s,&before,sizeof s)==0);
}
static void test_blocked_day_forced_combat_and_return(void)
{
    TeraExplorationState w=meduz_fixture();TeraMeduzSession s;TeraDaySession day;
    assert(tera_day_start(&day,&w,NULL,NULL) && day.phase==TERA_DAY_DONE);
    assert(tera_meduz_start(&s,&w,'8',0,0,0,1200,NULL,NULL));
    assert(s.phase==TERA_MEDUZ_DONE && !s.moved);
    assert(memcmp(&s.state,&day.state,sizeof w)==0); /* Jour meme devant un mur. */
    w.world.maps[1000][0]=1;w.world.maps[1001][4]=2;
    assert(tera_meduz_start(&s,&w,'8',0,0,0,1200,NULL,NULL));
    assert(s.phase==TERA_MEDUZ_DAY && s.fixed_combat==1 && s.combat.game.encounter.monster_type==22);
    assert(s.combat.game.position==1001 && s.state.information_context);
    w.world.characters.combat.encounter.party_available=0;w.world.characters.days=32766;
    assert(tera_meduz_resume(&s,&w,1204));
    assert(s.phase==TERA_MEDUZ_DONE && s.state.world.characters.combat.world_id==11);
    assert(s.state.world.characters.combat.position==1204 && s.state.world.characters.days==-32763);
    assert(!s.state.world.characters.combat.encounter.party_available);
}
static void test_word_tenth_key(void)
{
    TeraExplorationState w=module_fixture();TeraMeduzSession s;
    strcpy(w.world.secret_words[1],"ABCDEFGHI");
    assert(tera_meduz_start(&s,&w,'M',1,0,1,1200,NULL,NULL));
    keys(&s,"oABCDEFGHX\bI");assert(s.phase==TERA_MEDUZ_ENTRY_WORD);
    assert(s.word_length==9 && !strcmp(s.word,"ABCDEFGHI"));
    /* Dix lettres termine la lecture sans Entree, neuf sont comparees. */
    assert(tera_meduz_key(&s,'Z'));assert(s.phase!=TERA_MEDUZ_ENTRY_WORD);
    assert(s.state.world.characters.combat.world_id==10);
}

static unsigned tunnel_count;
static void tunnel_observe(void *ctx,const TeraPresentationEvent *e)
{
    const TeraTunnelFrame *a,*b;(void)ctx;
    if(e->kind!=TERA_PRESENT_TUNNEL) return;
    assert(tunnel_count<sizeof original_tunnel_frames/sizeof original_tunnel_frames[0]);
    a=&e->data.tunnel;b=&original_tunnel_frames[tunnel_count++];
    assert(a->reset==b->reset && a->palette==b->palette && a->background==b->background);
    assert(a->x==b->x && a->y==b->y && a->radius==b->radius && a->color==b->color);
    assert(a->erase_radius==b->erase_radius);
    if(a->erase_radius) assert(a->erase_x==b->erase_x && a->erase_y==b->erase_y);
    if(a->reset) assert(!memcmp(a->stars,b->stars,sizeof a->stars));
    assert(!a->hz);
}
static void test_original_tunnels(void)
{
    TeraExplorationState e=module_fixture();TeraMeduzSession s,plain;
    TeraPresentationSink sink={tunnel_observe,NULL};
    assert(tera_meduz_start(&plain,&e,'M',1,0,1,1200,NULL,NULL));keys(&plain,"OVOYAGE\r");
    e.world.characters.combat.presentation=&sink;tunnel_count=0;
    assert(tera_meduz_start(&s,&e,'M',1,0,1,1200,NULL,NULL));keys(&s,"OVOYAGE\r");
    assert(tunnel_count==245);
    s.state.world.characters.combat.presentation=NULL;
    /* La graine native a la sortie du tunnel est 01CAFDBB ; arrive()
     * poursuit ensuite normalement ses tirages de scene/jour. */
    assert(s.state.world.characters.combat.random.seed==plain.state.world.characters.combat.random.seed);
}
int main(void)
{
    test_meditation_and_objective();test_blocked_day_forced_combat_and_return();test_word_tenth_key();test_original_tunnels();return 0;
}
