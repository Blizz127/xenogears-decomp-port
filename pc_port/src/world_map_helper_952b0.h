/*
 * World-map reference-vector sign projector 0x800952B0.
 *
 * Retail boundary: [0x800952B0, 0x80095324), 0x74 bytes / 29
 * instructions in world_map.bin loaded at 0x8006FAF0.  Leaf.
 *
 * ABI:
 *   $a0 = guest movement vector (s32 X at +0, s32 Z at +8)
 *   $a1 = guest output vector   (s32 X at +0, Y at +4, Z at +8)
 *   $a2 = guest reference vector (s32 X at +0, s32 Z at +8)
 *
 * dot = lo32(ref.X * mov.X) + lo32(ref.Z * mov.Z)   (mult/mflo, 32-bit
 * wraparound).  dot < 0 -> out = (-ref.X, 0, -ref.Z); dot > 0 ->
 * out = (ref.X, 0, ref.Z); dot == 0 -> out = (0, 0, 0).  out.Y is
 * always written 0 (the jr delay slot).  Retail leaves an incidental
 * residue in $v0; the sole retail caller (0x80095414's redirect tails)
 * never reads it, so the port implements the helper as void.
 */
#ifndef WORLD_MAP_HELPER_952B0_H
#define WORLD_MAP_HELPER_952B0_H

#include "common.h"

void wm_800952B0(u32 mov_vec, u32 out_vec, u32 ref_vec);

#endif /* WORLD_MAP_HELPER_952B0_H */
