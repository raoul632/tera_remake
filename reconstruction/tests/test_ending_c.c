#include "module_test_support.h"
#include "tera/ending.h"

static void test_score_vectors(void)
{
    /* Vecteurs analytiques de la formule documentee : dernier cas conserve
     * la troncature Real48 de 1/3 avant multiplication (pas un double brut). */
    static const struct {
        int16_t days,xp;unsigned difficulty;const char *text;uint8_t bytes[6];
    } vectors[]={
        {4001,1,0,"120",{135,0,0,0,0,112}},
        {4001,1,1,"480",{137,0,0,0,0,112}},
        {3999,1,0,"-120",{135,0,0,0,0,240}},
        {-32768,28768,0,"120",{135,0,0,0,0,112}},
        {4001,0,9,"0",{0,0,0,0,0,0}},
        {4003,1,0,"40",{134,255,255,255,255,31}}
    };
    TeraExplorationState w=module_fixture();TeraEndingSession s,before;unsigned i;
    for(i=0;i<4;++i) w.world.tasks_done[i]=1;
    for(i=0;i<sizeof vectors/sizeof vectors[0];++i) {
        w.world.characters.days=vectors[i].days;w.world.characters.combat.experience=vectors[i].xp;
        w.world.characters.combat.difficulty=vectors[i].difficulty;
        assert(tera_ending_start(&s,&w) && s.victory && !s.complete && !s.runtime_error);
        assert(strcmp(s.score_text,vectors[i].text)==0 && memcmp(s.score_real48,vectors[i].bytes,6)==0);
        assert(s.state.world.characters.combat.random.seed==1986);
        before=s;assert(!tera_ending_key(&s,256));assert(memcmp(&s,&before,sizeof s)==0);
        assert(tera_ending_key(&s,0) && s.complete);assert(!tera_ending_key(&s,13));
    }
}
static void test_flags_death_and_division_error(void)
{
    TeraExplorationState w=module_fixture();TeraEndingSession s,before;unsigned i;
    memset(&s,0x5a,sizeof s);before=s;w.world.tasks_counter=4;
    for(i=0;i<4;++i) {
        memset(w.world.tasks_done,1,4);w.world.tasks_done[i]=0;
        assert(!tera_ending_start(&s,&w));assert(memcmp(&s,&before,sizeof s)==0);
    }
    memset(w.world.tasks_done,1,4);w.world.tasks_counter=0;
    w.world.characters.days=4000;w.sound=1;
    assert(tera_ending_start(&s,&w) && s.runtime_error==2 && s.complete);
    assert(s.state.world.characters.combat.random.seed==1986); /* Avant musique. */
    w.world.characters.combat.encounter.party_available=0;
    assert(tera_ending_start(&s,&w) && !s.runtime_error && !s.victory && !s.complete);
    assert(!s.score && !s.score_text[0]);
    /* Les deux fins ont le meme calendrier sonore, meme si jours=4000
     * n'est pas calculable pour une victoire. */
    before=s;w.world.characters.days=4001;w.world.characters.combat.encounter.party_available=1;
    assert(tera_ending_start(&s,&w));
    assert(s.state.world.characters.combat.random.seed==before.state.world.characters.combat.random.seed);
    assert(s.state.world.characters.combat.random.seed!=1986);
}
int main(void) {test_score_vectors();test_flags_death_and_division_error();return 0;}
