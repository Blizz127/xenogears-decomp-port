/*
 * World-map helper 0x80073448 (fresh-entry placement).
 *
 * Retail boundary: [0x80073448, 0x80073530), 58 instructions.
 * Called from the entry sequence at 0x80072414 with a0 = lw(0x8009D3D4)
 * (world index) when 0x8009C894 == 0 (0x80072404) and the halfword at
 * 0x8006EE6A is 0 (0x800723E0). Seeds the runtime position
 * 0x8009C5AC/C5B0/C5B4 from the placement table lw(0x8009D3F4)
 * (rows {s16 x, s16 id, s16 z, s16 pad}, terminated by id == -1), or, when
 * bit 0x2000 of 0x8006EE68 is set, clears that bit, calls wm_8008DFF4 on
 * the position and copies 0x8006EE66 to 0x8009C584.
 */
#ifndef WORLD_MAP_HELPER_73448_H
#define WORLD_MAP_HELPER_73448_H

#include "common.h"

void wm_80073448(s32 world_index);

#endif
