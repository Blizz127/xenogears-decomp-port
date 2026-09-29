/*
 * World-map rectangle-region trigger lookup 0x80094238.
 *
 * Retail boundary: [0x80094238, 0x80094364), 0x12C bytes / 75
 * instructions in world_map.bin loaded at 0x8006FAF0.  Leaf.
 *
 * ABI:
 *   $a0 = guest position vector (s32 X at +0, s32 Z at +8; 20.12)
 *   $a1 = list index into the pointer array at *(0x8009BD00)
 *   $v0 = 1 when the srl-12, andi-0xFFFF cell coordinates fall inside a
 *         16-byte rect record (bounds INCLUSIVE both ends, signed
 *         compares); 0 on miss.
 *
 * Record layout (16-byte stride, s16 fields): +0 x0, +2 z0, +4 w, +6 h,
 * +8 valid marker (-1 terminates the list), +0xC id (lhu), +0xE type.
 * Hit type == 4: *(0x8009D7D8) = -1, sh -1 -> 0x8009BD24,
 *                sh id -> 0x8009CE68.
 * Hit type != 4: *(0x8009D7D8) = record guest address,
 *                sh -1 -> 0x8009CE68, sh id -> 0x8009BD24.
 * Miss: all three globals set to -1.
 */
#ifndef WORLD_MAP_HELPER_94238_H
#define WORLD_MAP_HELPER_94238_H

#include "common.h"

s32 wm_80094238(u32 pos_vec, u32 list_index);

#endif /* WORLD_MAP_HELPER_94238_H */
