/*
 * World-map helper 0x8008E0F0 (angle search via wm_80095414).
 *
 * Retail boundary: [0x8008E0F0, 0x8008E190), 40 instructions / 160 bytes.
 * Iterates angles 0..0xF00 step 0x100, calling rcos/rsin to build a
 * direction vector, then wm_80095414 to test movement. Returns the first
 * angle where wm_80095414 returns 1, or -1 if none found.
 */
#ifndef WORLD_MAP_HELPER_8E0F0_H
#define WORLD_MAP_HELPER_8E0F0_H

#include "common.h"

s32 wm_8008E0F0(u32 pos, u32 unused, u32 out);

#endif
