/* Regression de l'adaptateur RANDOMIZE. Horloge injectee, aucun rendu
 * ni ressource originale necessaire. Les constantes viennent du format
 * DOS CX:DX de BIN 0F14, pas du prototype Python. */
#include "module_test_support.h"
#include <SDL3/SDL.h>
static int clock_calls,clock_failure,conversion_failure;
static SDL_DateTime clock_value;
static bool audit_current_time(SDL_Time *ticks)
{
    ++clock_calls;*ticks=0;return !clock_failure;
}
static bool audit_local_time(SDL_Time ticks,SDL_DateTime *date,bool local)
{
    (void)ticks;assert(local);*date=clock_value;return !conversion_failure;
}
#define SDL_GetCurrentTime audit_current_time
#define SDL_TimeToDateTime audit_local_time
#include "../frontend/sdl3/app.c"
#undef SDL_GetCurrentTime
#undef SDL_TimeToDateTime
int main(void)
{
    uint32_t seed;
    clock_value.hour=23;clock_value.minute=59;clock_value.second=59;
    clock_value.nanosecond=999999999;
    assert(startup_seed(0,1986,0,&seed) && seed==UINT32_C(0x173b3b63));
    clock_value.hour=12;clock_value.minute=34;clock_value.second=56;
    clock_value.nanosecond=780000000;
    assert(startup_seed(0,1986,0,&seed) && seed==UINT32_C(0x0c22384e));
    clock_value.nanosecond=9999999;
    assert(startup_seed(0,1986,0,&seed) && seed==UINT32_C(0x0c223800));
    assert(clock_calls==3);
    assert(startup_seed(1,0,0,&seed) && seed==0);
    assert(startup_seed(1,UINT32_MAX,0,&seed) && seed==UINT32_MAX);
    assert(startup_seed(0,1986,1,&seed) && seed==1986 && clock_calls==3);
    clock_failure=1;seed=42;
    assert(!startup_seed(0,1986,0,&seed) && seed==42);
    clock_failure=0;conversion_failure=1;
    assert(!startup_seed(0,1986,0,&seed) && seed==42);
    conversion_failure=0;clock_value.second=60;
    assert(!startup_seed(0,1986,0,&seed) && seed==42);
    puts("Startup RANDOMIZE: local DOS clock, explicit seed, smoke and failures OK");
    return 0;
}
