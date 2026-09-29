/*
 * World-map terrain selector helper 0x80093F18.
 *
 * Retail boundary: [0x80093F18, 0x80093FE4), 0xCC bytes / 51
 * instructions in world_map.bin loaded at 0x8006FAF0.
 *
 * ABI: $a0 = guest VECTOR address (x at +0, z at +8).  Calls
 * wm_80093E8C(a0) and returns 0..7 extracted from that halfword
 * depending on a height-weighted sum.  Return is a 32-bit integer,
 * not boolean.
 *
 * Global reads: four u32 tables at 0x8009B464, 0x8009B364,
 * 0x8009B46C, 0x8009B36C indexed by (packed_low_nibble << 4).
 */

#ifndef WORLD_MAP_HELPER_93F18_H
#define WORLD_MAP_HELPER_93F18_H

#include "common.h"

s32 wm_80093F18(u32 vec_addr);

#endif /* WORLD_MAP_HELPER_93F18_H */
