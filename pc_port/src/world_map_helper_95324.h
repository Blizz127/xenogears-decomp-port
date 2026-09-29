/*
 * World-map wall-tangent projector 0x80095324.
 *
 * Retail boundary: [0x80095324, 0x80095414), 0xF0 bytes / 60
 * instructions in world_map.bin loaded at 0x8006FAF0.
 *
 * ABI:
 *   $a0 = guest surface-normal vector (VECTOR-shaped, s32 X/Y/Z)
 *   $a1 = guest movement vector (s32 X at +0, s32 Z at +8)
 *   $a2 = guest output vector   (s32 X at +0, Y at +4, Z at +8)
 *
 * Behavior:
 *   1. Write the down vector (0, -0x1000, 0) to scratch 0x1F800010
 *      (store order +0x18, +0x10, +0x14).
 *   2. OuterProduct12(normal, 0x1F800010, 0x1F800020) — horizontal
 *      tangent of the surface.
 *   3. VectorNormal(0x1F800020, 0x1F800000) — unit tangent.
 *   4. dot = lo32(tan.X * mov.X) + lo32(tan.Z * mov.Z); out receives
 *      ±(tan.X, 0, tan.Z) by the sign of dot, or (0, 0, 0) when the
 *      wrapped dot is zero; out.Y always written 0.
 *
 * Retail leaves an incidental residue in $v0; the sole retail caller
 * (0x80095414 case-1 post-scan) never reads it, so the port implements
 * the helper as void.
 */
#ifndef WORLD_MAP_HELPER_95324_H
#define WORLD_MAP_HELPER_95324_H

#include "common.h"

void wm_80095324(u32 normal_vec, u32 mov_vec, u32 out_vec);

#endif /* WORLD_MAP_HELPER_95324_H */
