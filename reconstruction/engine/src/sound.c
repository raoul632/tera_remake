#include "tera/sound.h"
#include <math.h>

void tera_media_wait(const TeraCombatGame *g,unsigned ms)
{
    TeraPresentationEvent e=tera_presentation_make(g,TERA_PRESENT_WAIT,TERA_SOURCE_COMBAT);
    e.data.wait.ms=ms;tera_presentation_emit(g,&e);
}
void tera_media_tone(const TeraCombatGame *g,unsigned hz,unsigned ms)
{
    TeraPresentationEvent e=tera_presentation_make(g,TERA_PRESENT_TONES,TERA_SOURCE_COMBAT);
    e.data.tones.count=1;e.data.tones.hz[0]=(unsigned short)hz;e.data.tones.ms[0]=(unsigned short)ms;
    tera_presentation_emit(g,&e);
}
static unsigned draw(TeraCombatGame *g,unsigned bound)
{unsigned v=0;tera_random_draw(&g->random,bound,&v);return v;}
static void note(TeraCombatGame *g,TeraPresentationEvent *e,unsigned hz,unsigned ms)
{
    unsigned i=e->data.tones.count++;
    e->data.tones.hz[i]=(unsigned short)hz;e->data.tones.ms[i]=(unsigned short)ms;
    if(e->data.tones.count==64) {tera_presentation_emit(g,e);e->data.tones.count=0;}
}
static void flush(TeraCombatGame *g,TeraPresentationEvent *e)
{if(e->data.tones.count) tera_presentation_emit(g,e);}
void tera_sound_sweep(TeraCombatGame *g,unsigned from,unsigned to,unsigned step,unsigned bound)
{
    int i=(int)(from/step),end=(int)(to/step),inc=from<=to ? 1 : -1;
    TeraPresentationEvent e=tera_presentation_make(g,TERA_PRESENT_TONES,TERA_SOURCE_COMBAT);
    for(;;i+=inc) {
        unsigned v=draw(g,bound);note(g,&e,(unsigned)i*step+v,v);
        if(i==end) break;
    }
    flush(g,&e);
}
void tera_sound_burst(TeraCombatGame *g,unsigned count)
{
    unsigned i;TeraPresentationEvent e=tera_presentation_make(g,TERA_PRESENT_TONES,TERA_SOURCE_COMBAT);
    for(i=0;i<count;++i) note(g,&e,30*draw(g,255)+10,0);
    flush(g,&e);
}
void tera_sound_hit(TeraCombatGame *g)
{tera_sound_sweep(g,600,640,5,30);tera_sound_sweep(g,2000,100,40,20);}
void tera_sound_action(TeraCombatGame *g,char action)
{
    unsigned i,j;
    if(action=='T') tera_sound_burst(g,600);
    else if(action=='L') {
        tera_sound_sweep(g,50,600,20,20);tera_sound_burst(g,2000);
        tera_sound_sweep(g,2000,100,60,20);
    } else if(action=='P') {
        tera_media_tone(g,40,1500);tera_sound_sweep(g,40,10,1,25);
    } else if(action=='C') for(i=0;i<3;++i) {
        unsigned hz=i==2 ? 160 : 80;
        for(j=0;j<30;++j) {tera_media_tone(g,hz,10);tera_media_tone(g,draw(g,2*hz),1);}
        tera_sound_sweep(g,240,30,4,3);tera_sound_sweep(g,40,320,4,2);
    }
}
unsigned tera_sound_noise(TeraCombatGame *g,unsigned base,unsigned duration,unsigned delay,
    unsigned threshold,unsigned gate,unsigned noise1,unsigned noise2)
{
    unsigned i,hz,v,calls=0;
    TeraPresentationEvent e=tera_presentation_make(g,TERA_PRESENT_TONES,TERA_SOURCE_COMBAT);
    for(i=1;i<duration;++i) {
        hz=base;v=draw(g,20);++calls;
        if(v<threshold) {unsigned side=draw(g,2);hz=(side ? noise1 : noise2)+10*draw(g,side ? 256 : 255);calls+=2;}
        else if(v>gate) hz=0;
        note(g,&e,hz,delay);
    }
    flush(g,&e);return calls;
}
unsigned tera_sound_melody(TeraCombatGame *g,int alternate,unsigned multiplier,unsigned numerator,unsigned denominator)
{
    static const unsigned factors[2][3]={{210,264,352},{264,210,176}},durations[]={60,30,120};
    unsigned part,calls=0;
    for(part=0;part<3;++part) {
        unsigned factor=factors[alternate!=0][part];
        calls+=tera_sound_noise(g,factor*(alternate ? 1 : multiplier)*numerator/denominator,
            durations[part],15,5,18,factor*(alternate ? 5 : 10),8000);
    }
    return calls;
}
unsigned tera_sound_objective(TeraCombatGame *g)
{
    unsigned calls=tera_sound_melody(g,1,1,9,8);
    calls+=tera_sound_melody(g,0,1,9,8);
    calls+=tera_sound_melody(g,0,2,9,8);
    return calls;
}
unsigned tera_sound_random_tones(TeraCombatGame *g,unsigned bound,unsigned delay,unsigned gap,unsigned count)
{
    unsigned i;TeraPresentationEvent e=tera_presentation_make(g,TERA_PRESENT_TONES,TERA_SOURCE_EXPLORATION);
    for(i=0;i<count;++i) note(g,&e,10*draw(g,bound),delay);
    flush(g,&e);if(gap) tera_media_wait(g,gap);
    return count;
}
void tera_sound_entry_meduz(TeraCombatGame *g)
{
    unsigned i;
    for(i=1;i<=25;++i) tera_sound_random_tones(g,10*i,10,0,10);
}
void tera_sound_refusal(TeraCombatGame *g)
{
    tera_sound_burst(g,100);tera_media_tone(g,165,250);
    tera_sound_burst(g,100);tera_sound_sweep(g,250,20,5,15);
}
void tera_sound_victory(TeraCombatGame *g)
{(void)tera_sound_melody(g,0,2,9,8);}
unsigned tera_sound_country(TeraCombatGame *g,unsigned numerator,unsigned denominator)
{
    static const unsigned factors[]={264,330,396,440,465,440,396},durations[]={10,5,15,10,5,15,30};
    unsigned part,calls=0;
    for(part=0;part<7;++part) {
        calls+=tera_sound_noise(g,factors[part]*numerator/denominator,durations[part],15,7,17,9000,3000);
        tera_media_wait(g,200);
    }
    return calls;
}
void tera_sound_wave(TeraCombatGame *g,unsigned base,unsigned delay,unsigned count)
{
    TeraPresentationEvent e=tera_presentation_make(g,TERA_PRESENT_TONES,TERA_SOURCE_SPACE);
    unsigned n;
    for(n=count;n>0;--n) note(g,&e,(unsigned)(base*(100.0+80.0*cos(0.2*n))/100.0),delay);
    flush(g,&e);
}
void tera_sound_hyperspace(TeraCombatGame *g,int descending)
{
    TeraPresentationEvent e=tera_presentation_make(g,TERA_PRESENT_TONES,TERA_SOURCE_SPACE);
    int i=descending ? 1200 : 50,end=descending ? 50 : 1200,step=descending ? -1 : 1;
    unsigned divisor,hz;
    for(;;i+=step) {
        hz=(unsigned)i;note(g,&e,hz,1);
        for(divisor=2;divisor<=4;++divisor) {
            if(i>1200/(int)divisor) hz=(unsigned)i/divisor;
            note(g,&e,hz,1);
        }
        if(i==end) break;
    }
    flush(g,&e);
}
unsigned tera_sound_steps(TeraCombatGame *g,int fast)
{
    unsigned part,i;TeraPresentationEvent e=tera_presentation_make(g,TERA_PRESENT_TONES,TERA_SOURCE_EXPLORATION);
    for(part=0;part<(fast ? 1u : 3u);++part) {
        for(i=0;i<500;++i) note(g,&e,10+draw(g,155),0);
        flush(g,&e);e.data.tones.count=0;
        if(!fast) {
            tera_media_wait(g,450);
            for(i=0;i<450;++i) note(g,&e,10+draw(g,110),0);
            flush(g,&e);e.data.tones.count=0;tera_media_wait(g,500);
        }
    }
    return fast ? 500 : 2850;
}
unsigned tera_sound_alert(TeraCombatGame *g)
{
    unsigned part,i;static const unsigned bounds[]={30,90,270};
    TeraPresentationEvent e=tera_presentation_make(g,TERA_PRESENT_TONES,TERA_SOURCE_EXPLORATION);
    for(part=0;part<3;++part) for(i=0;i<60;++i) note(g,&e,10*draw(g,bounds[part]),30);
    flush(g,&e);return 180;
}

/* OVL00CB : memes quatre melodies et memes tirages que les anciens appels
 * muets ; le DELAY de deuil n'existe que dans la branche sonore. */
void tera_sound_ending(TeraCombatGame *g,int victory)
{
    if(victory) {
        tera_sound_melody(g,1,1,9,8);tera_sound_melody(g,0,1,9,8);
        tera_sound_melody(g,0,2,9,8);tera_sound_melody(g,0,4,9,8);
    } else {
        tera_sound_melody(g,1,1,1,1);tera_sound_melody(g,0,1,1,1);
        tera_sound_melody(g,0,2,1,1);tera_media_wait(g,1000);
        tera_sound_melody(g,1,1,9,8);
    }
}
