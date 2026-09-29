/* World-overlay terrain surface-normal helper (retail 0x80093740). */
#ifndef WORLD_MAP_TERRAIN_NORMAL_H
#define WORLD_MAP_TERRAIN_NORMAL_H

#include "common.h"

/*
 * out_normal_addr is a 32-bit guest address in main RAM or PSX scratchpad.
 * x/z are the same signed fixed-point coordinates accepted by wm_80093660.
 * Returns the 32-bit v0 result left by retail VectorNormal.
 */
s32 wm_80093740(u32 out_normal_addr, s32 x, s32 z);

#endif /* WORLD_MAP_TERRAIN_NORMAL_H */
