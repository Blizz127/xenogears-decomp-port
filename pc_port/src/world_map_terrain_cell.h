/*
 * World-map terrain grid-cell pointer lookup (0x80093660).
 *
 * Retail slice: 0x80093660–0x8009373C (56 instructions, leaf).
 * Given signed fixed-point X/Z coordinates, returns the guest address
 * of the 4-byte terrain cell containing that point within the
 * quadrant-organized terrain grid.
 *
 * Reads globals:
 *   0x8009D160  (s32)  terrain row stride
 *   0x8009C184  (u32[]) terrain pointer table (indexed by signed tile index)
 *
 * No writes, no calls, no allocation.  Pure leaf computation.
 */
#ifndef WORLD_MAP_TERRAIN_CELL_H
#define WORLD_MAP_TERRAIN_CELL_H

#include "common.h"

/* Terrain grid-cell pointer lookup.
 * Exact transcription of retail 0x80093660–0x8009373C.
 * x, z: signed 32-bit fixed-point world coordinates.
 * Returns: 32-bit guest address of the terrain cell sample. */
u32 wm_80093660(s32 x, s32 z);

#endif /* WORLD_MAP_TERRAIN_CELL_H */
