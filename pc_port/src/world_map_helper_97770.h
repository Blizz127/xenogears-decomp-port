/*
 * World-map pool slot claim 0x80097770.
 *
 * Retail boundary: [0x80097770, 0x800977A8), 0x38 bytes / 14
 * instructions in world_map.bin loaded at 0x8006FAF0.  Leaf.
 *
 * ABI:
 *   $a0 = slot index (u32; slot address = *(0x8009BE24) + (idx << 7),
 *         128-byte stride — the accepted convergence/scheduler pool)
 *   $a1 = value; only the low halfword is stored (sh)
 *   $v0 = 0 if the slot's halfword at +4 is nonzero (busy), else 1
 *         after writing 1 to +0 and value to +4.
 */
#ifndef WORLD_MAP_HELPER_97770_H
#define WORLD_MAP_HELPER_97770_H

#include "common.h"

s32 wm_80097770(u32 slot_idx, s32 value);

#endif /* WORLD_MAP_HELPER_97770_H */
