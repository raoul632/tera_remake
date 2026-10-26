#include "tera/random.h"
#include <stddef.h>

int tera_random_clock_seed(TeraRandom *rng, unsigned hour, unsigned minute,
                           unsigned second, unsigned hundredth)
{
    if (!rng || hour > 23 || minute > 59 || second > 59 || hundredth > 99)
        return 0;
    rng->seed = ((uint32_t)hour << 24) | ((uint32_t)minute << 16) |
                ((uint32_t)second << 8) | hundredth;
    return 1;
}

int tera_random_draw(TeraRandom *rng, unsigned bound, unsigned *result)
{
    if (rng == NULL || result == NULL || bound == 0 || bound > 65535)
        return 0;
    /* Les operations non signees reproduisent ADD/ADC et leur retenue,
     * sans overflow signe C. Le premier resultat vient de la NOUVELLE graine. */
    rng->seed = rng->seed * UINT32_C(129) + UINT32_C(0x361962e9);
    *result = (unsigned)((rng->seed >> 17) % bound);
    return 1;
}

unsigned tera_random_sound_noise(TeraRandom *rng, unsigned duration, unsigned threshold)
{
    unsigned i,value,calls=0;
    if(!rng || duration<2 || threshold>20) return 0;
    for(i=1;i<duration;++i) {
        tera_random_draw(rng,20,&value);++calls;
        if(value<threshold) {
            tera_random_draw(rng,2,&value);++calls;
            tera_random_draw(rng,value==1 ? 256 : 255,&value);++calls;
        }
    }
    return calls;
}

unsigned tera_random_sound_melody(TeraRandom *rng)
{
    unsigned calls=tera_random_sound_noise(rng,60,5);
    calls+=tera_random_sound_noise(rng,30,5);
    calls+=tera_random_sound_noise(rng,120,5);
    return calls;
}

static void discard_draws(TeraRandom *rng,unsigned count,unsigned bound)
{
    unsigned i,value;
    for(i=0;i<count;++i) tera_random_draw(rng,bound,&value);
}

unsigned tera_random_sound_alert(TeraRandom *rng)
{
    if(!rng) return 0;
    discard_draws(rng,60,30);discard_draws(rng,60,90);discard_draws(rng,60,270);
    return 180;
}

unsigned tera_random_sound_steps(TeraRandom *rng,int fast_moves)
{
    unsigned i;
    if(!rng) return 0;
    if(fast_moves) {discard_draws(rng,500,155);return 500;}
    for(i=0;i<3;++i) {discard_draws(rng,500,155);discard_draws(rng,450,110);}
    return 2850;
}

unsigned tera_random_sound_country(TeraRandom *rng)
{
    const unsigned durations[7]={10,5,15,10,5,15,30};
    unsigned i,calls=0;
    for(i=0;i<7;++i) calls+=tera_random_sound_noise(rng,durations[i],7);
    return calls;
}
