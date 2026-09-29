/*
 * World-map helper 0x800965A4 (queue record allocator variant B).
 *
 * Retail boundary: [0x800965A4, 0x80096668), 49 instructions / 196 bytes.
 * Same pattern as 96328 but uses D_8009D3C0 base and D_8009C624 table.
 * Record stride = ((idx*3*4 - idx) << 7) = idx * 1408.
 */
#ifndef WORLD_MAP_HELPER_965A4_H
#define WORLD_MAP_HELPER_965A4_H

#include "common.h"

s32 wm_800965A4(void);

#endif
