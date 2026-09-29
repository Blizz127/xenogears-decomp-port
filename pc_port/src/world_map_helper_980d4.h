/*
 * World-map helper 0x800980D4 (position wrap / paging gate).
 *
 * Retail boundary: [0x800980D4, 0x800981C8), 61 instructions. Leaf.
 * a0 = position vector (x at +0, z at +8, 9.23 fixed). Clears D_8009D558,
 * wraps x/z back into (-0x800000, 0x800000] setting D558 bits 4/8 (x) and
 * 1/2 (z), then publishes map_x/map_z = (pos >> 23) + 2.
 */
#ifndef WORLD_MAP_HELPER_980D4_H
#define WORLD_MAP_HELPER_980D4_H

#include "common.h"

void wm_800980D4(u32 pos_vec);

#endif
