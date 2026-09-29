/*
 * World-map perpendicular-heading vector builder 0x800941C4.
 *
 * Retail boundary: [0x800941C4, 0x80094238), 0x74 bytes / 29
 * instructions in world_map.bin loaded at 0x8006FAF0.
 *
 * ABI:
 *   $a0 = guest vector A (s32 X at +0, s32 Z at +8)
 *   $a1 = guest vector B (s32 X at +0, s32 Z at +8)
 *   $a2 = guest output vector (X at +0, Z at +8)
 *   $a3 = guest angle halfword out
 *   angle = (ratan2(B.Z - A.Z, B.X - A.X) + 0x400) & 0xFFF (sh to *a3)
 *   out.X = rcos(angle); out.Z = -rsin(lh reload of *a3)
 *   $v0 = the negated rsin value (incidental; verified dead at the
 *         sampled retail call sites but returned faithfully).
 */
#ifndef WORLD_MAP_HELPER_941C4_H
#define WORLD_MAP_HELPER_941C4_H

#include "common.h"

s32 wm_800941C4(u32 vec_a, u32 vec_b, u32 out_vec, u32 out_angle);

#endif /* WORLD_MAP_HELPER_941C4_H */
