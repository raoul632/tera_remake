#include "tera/sound.h"
#include "tera/ending.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
/* Real Pascal : exposant biaise 129, mantisse 39 bits + bit implicite.
 * Tronquer chaque operation a 40 bits, pas seulement le resultat final. */
static double real48(double value,uint8_t bytes[6])
{
    int exponent,negative=value<0;unsigned i;double fraction,rounded;
    uint64_t mantissa;
    if(!value) {if(bytes) memset(bytes,0,6);return 0;}
    fraction=frexp(fabs(value),&exponent);
    rounded=floor(ldexp(fraction,40));
    if(rounded>=1099511627776.0) {rounded*=0.5;++exponent;}
    mantissa=(uint64_t)rounded-UINT64_C(549755813888);
    if(bytes) {
        bytes[0]=(uint8_t)(exponent+128);
        for(i=1;i<6;++i) {bytes[i]=(uint8_t)(mantissa&255u);mantissa>>=8;}
        if(negative) bytes[5]|=128;
    }
    return (negative ? -1 : 1)*ldexp(rounded,exponent-40);
}
int tera_ending_start(TeraEndingSession *s,const TeraExplorationState *state)
{
    TeraExplorationState copy;unsigned i;int32_t denominator;double score;
    if(!s || !state) return 0;
    for(i=0;i<4;++i) if(!state->world.tasks_done[i]) return 0;
    copy=*state;memset(s,0,sizeof *s);s->state=copy;s->victory=copy.world.characters.combat.encounter.party_available!=0;
    if(s->victory) {
        denominator=copy.world.characters.days-4000;
        if(denominator< -32768) denominator+=65536;
        if(!denominator) {s->runtime_error=2;s->complete=1;return 1;}
        score=real48((double)copy.world.characters.combat.experience/denominator,NULL);
        score=real48(score*(1+copy.world.characters.combat.difficulty),NULL);
        score=real48(score*(1+(copy.world.characters.combat.difficulty+3)/4),NULL);
        s->score=real48(score*120,s->score_real48);
        snprintf(s->score_text,sizeof s->score_text,"%.0f",s->score);
    }
    {
        TeraPresentationEvent e=tera_presentation_make(&s->state.world.characters.combat,TERA_PRESENT_ENDING,TERA_SOURCE_ENDING);
        e.data.ending.victory=(int)s->victory;memcpy(e.data.ending.score,s->score_text,sizeof e.data.ending.score);
        tera_presentation_emit(&s->state.world.characters.combat,&e);
    }
    if(copy.sound) tera_sound_ending(&s->state.world.characters.combat,s->victory);
    return 1;
}
int tera_ending_key(TeraEndingSession *s,unsigned key)
{
    if(!s || key>255 || s->complete) return 0;
    s->complete=1;return 1;
}
