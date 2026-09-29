/*
 * World-map axis approach stepper 0x8008BEC8.
 *
 * Retail boundary: [0x8008BEC8, 0x8008BFD4), 0x10C bytes / 67
 * instructions in world_map.bin loaded at 0x8006FAF0.
 *
 * ABI:
 *   $a0 = guest object record.  Fields used:
 *         +0x28 s32 pos.X (20.12)      +0x30 s32 pos.Z (20.12)
 *         +0x2C s32 heading out        +0x38 s32 vel.X  +0x40 s32 vel.Z
 *         +0x4A u16 step (sign16, then arithmetic /2 toward zero)
 *         +0x50 s32 target.X (integer) +0x54 s32 target.Z (integer)
 *   Per axis: d = |target - (pos sra 12)| (32-bit wrap abs); d < 5 sets
 *   the close bit (X=1, Z=2), else pos += lo32(vel * (step/2)).
 *   Tail: wm_80093354(obj+0x28) wrap, then obj[+0x2C] =
 *   wm_80093978(pos.X, pos.Z).
 *   $v0 = close mask 0..3.
 */
#ifndef WORLD_MAP_HELPER_8BEC8_H
#define WORLD_MAP_HELPER_8BEC8_H

#include "common.h"

s32 wm_8008BEC8(u32 obj);

#endif /* WORLD_MAP_HELPER_8BEC8_H */
