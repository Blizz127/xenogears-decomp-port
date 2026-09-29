/*
 * World-map terrain height sampler (0x80093978).
 *
 * Samples terrain height at signed world X/Z coordinates by looking up
 * the terrain cell, computing the surface normal, solving the plane
 * equation, and scaling the result.
 *
 * Retail boundary: [0x80093978, 0x80093A5C), 0xE4 bytes / 57 instructions.
 * Calls: wm_80093660, wm_80093740, wm_800935DC.
 *
 * x, z: signed 32-bit world coordinates.
 * Returns: signed 32-bit terrain height (scaled by << 3).
 */
#ifndef WORLD_MAP_TERRAIN_SAMPLER_H
#define WORLD_MAP_TERRAIN_SAMPLER_H

#include "common.h"

s32 wm_80093978(s32 x, s32 z);

#endif /* WORLD_MAP_TERRAIN_SAMPLER_H */
