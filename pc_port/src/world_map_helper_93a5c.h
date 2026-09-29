/*
 * World-map helper 0x80093A5C (terrain surface normal computation).
 *
 * Retail boundary: [0x80093A5C, 0x80093E8C), 268 instructions / 1072 bytes.
 * Samples warped terrain height at signed fixed-point world X/Z. It obtains
 * the terrain cell, builds four corner heights, selects the retail triangle,
 * normalizes its plane, and solves Y.
 */
#ifndef WORLD_MAP_HELPER_93A5C_H
#define WORLD_MAP_HELPER_93A5C_H

#include "common.h"

s32 wm_80093A5C(u32 x_bits, u32 z_bits);

#endif
