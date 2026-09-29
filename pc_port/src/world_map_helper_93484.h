/*
 * World-map helper 0x80093484 (position wrap/clamp).
 *
 * Retail boundary: [0x80093484, 0x80093534), 44 instructions / 176 bytes.
 * Leaf function. Wraps X and Z components of a position vector to
 * stay within world boundaries using D_8009D160 and D_8009D2B4 globals.
 */
#ifndef WORLD_MAP_HELPER_93484_H
#define WORLD_MAP_HELPER_93484_H

#include "common.h"

void wm_80093484(u32 pos_vec);

#endif
