/*
 * World-map terrain-attribute nibble wrapper 0x80093FE4.
 *
 * Retail boundary: [0x80093FE4, 0x80094004), 0x20 bytes / 8
 * instructions in world_map.bin loaded at 0x8006FAF0.
 *
 * ABI:
 *   $a0 = guest VECTOR pointer (passed through to 0x80093E8C)
 *   $v0 = wm_80093E8C(a0) & 0xF   (andi: low nibble of the terrain
 *         attribute halfword; always in [0, 15])
 */
#ifndef WORLD_MAP_HELPER_93FE4_H
#define WORLD_MAP_HELPER_93FE4_H

#include "common.h"

s32 wm_80093FE4(u32 vec_addr);

#endif /* WORLD_MAP_HELPER_93FE4_H */
