/*
 * World-map walkability-flag lookup 0x80094060.
 *
 * Retail boundary: [0x80094060, 0x80094088), 40 bytes / 10
 * instructions in world_map.bin loaded at 0x8006FAF0.
 *
 * ABI: $a0 / $a1 = s16 indices (row / column); $v0 = sign-extended
 * s16 flag.  Leaf, no stack frame, no saved registers, no stores.
 */
#ifndef WORLD_MAP_HELPER_94060_H
#define WORLD_MAP_HELPER_94060_H

#include "common.h"

s32 wm_80094060(s32 a0, s32 a1);

u32 wm_80094060_get_exec_count(void);
void wm_80094060_reset_exec_count(void);

#endif /* WORLD_MAP_HELPER_94060_H */
