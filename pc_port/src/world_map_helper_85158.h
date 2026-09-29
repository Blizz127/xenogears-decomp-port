/*
 * World-map node-plane prober 0x80085158.
 *
 * Retail boundary: [0x80085158, 0x80085418), 0x2C0 bytes / 176
 * instructions in world_map.bin loaded at 0x8006FAF0.
 *
 * ABI:
 *   $a0        = guest position vector (s32 20.12 X at +0, Z at +8)
 *   $a1        = guest coefficient/out block (caller: 0x1F800070);
 *                receives +0 = (pos.X>>12) - rec[8],
 *                         +8 = rec[0x10] - (pos.Z>>12)  (asymmetric!),
 *                and +4 = plane-solved height via wm_800935DC
 *   $a2        = guest normal out vector (caller: 0x1F800080);
 *                receives the unit plane normal via VectorNormal
 *   $a3        = attribute/region index (andi 0xFFFF; record stride 84)
 *   0x10($sp)  = node id (lhu: zero-extended halfword)
 *   $v0        = wm_800935DC result (the solved height); both retail
 *                callers (0x80095414 sites) overwrite $v0 immediately
 *
 * Builds the region MATRIX (rot = rec[0x20..0x3F] copy, trans =
 * (0, rec[0xC], 0)) scaled by (0x800,0x800,0x800) into the GTE, then
 * RotTrans the node triangle's three vertices (s16 indices at
 * node+0/+2/+4, 8-byte SVECTORs at *(rec[0x44]+4)); edges V1-V0 and
 * V2-V0 in place; OuterProduct0(V2-V0, V1-V0) with OUT ALIASING the
 * first input; >>2; VectorNormal into $a2; then
 * wm_800935DC($a1, 0x1F800000 = transformed V0, $a2).
 */
#ifndef WORLD_MAP_HELPER_85158_H
#define WORLD_MAP_HELPER_85158_H

#include "common.h"

u32 wm_80085158(u32 pos_vec, u32 coef_out, u32 normal_out, u32 attr,
                u32 node_id);

#endif /* WORLD_MAP_HELPER_85158_H */
