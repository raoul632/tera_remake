#ifndef TERA_XT_TIMING_H
#define TERA_XT_TIMING_H
#include "tera/presentation.h"
#include <stdint.h>
#define TERA_XT_CLOCK_HZ UINT64_C(4772727)
/* INNER_BC26 : mode initial, puis sept modes internes deja profiles.
 * Un clear de 16 Ko : 8192 STOSW nominaux a 18 cycles sur bus 8 bits. */
#define TERA_XT_SEQUENCE_CLEAR_CYCLES UINT64_C(147456)
/* Profil nominal 8088 du pilote original : cout variable du raster et clipping.
 * Ni mesure murale, ni emulation de la contention du bus CGA/refresh DMA. */
uint64_t tera_xt_circle_cycles(int x,int y,unsigned radius);
uint64_t tera_xt_tunnel_cycles(const TeraTunnelFrame *frame);
#endif
