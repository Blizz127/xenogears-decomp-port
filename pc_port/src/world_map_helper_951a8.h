/*
 * World-map movement resolver 0x800951A8.
 *
 * Retail boundary: [0x800951A8, 0x800952B0), 0x108 bytes / 66
 * instructions in world_map.bin loaded at 0x8006FAF0.
 *
 * ABI:
 *   $a0        = base vector (guest address; forwarded to 0x80094A5C)
 *   $a1        = direction vector (guest address; s32 X at +0, Z at +8)
 *   $a2        = output vector (guest address; s32 X at +0, Y at +4,
 *                Z at +8)
 *   $a3        = signed scale (forwarded to 0x80094A5C)
 *   0x10($sp)  = mode; retail loads it with `lh 0x30($sp)` so only the
 *                sign-extended low halfword reaches 0x80094A5C
 *   $v0        = 1 when 0x80094A5C returned 0 (move blocked by the
 *                generic tail), else 0 with the output vector filled:
 *                  94A5C == 1: slide vector from wm_80094088
 *                              (workspace nibble at 0x1F800000); if the
 *                              dot was zero the output is zeroed instead
 *                  94A5C == 2: out = (sign(dir.X) * 0x1000, 0, 0)
 *                  94A5C == 3: out = (0, 0, sign(dir.Z) * 0x1000)
 *
 * 0x80094A5C only returns {0, 1, 2, 3}; the retail fall-through for any
 * other value would return the caller's saved $s2 register and is
 * unreachable.  The port aborts on it instead of inventing a value.
 */
#ifndef WORLD_MAP_HELPER_951A8_H
#define WORLD_MAP_HELPER_951A8_H

#include "common.h"

s32 wm_800951A8(u32 base_vec, u32 dir_vec, u32 out_vec, s32 scale, s32 mode);

#endif /* WORLD_MAP_HELPER_951A8_H */
