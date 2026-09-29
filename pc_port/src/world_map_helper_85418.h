/*
 * World-map node-triangle plane-straddle filter 0x80085418.
 *
 * Retail boundary: [0x80085418, 0x80085760), 0x348 bytes / 210
 * instructions in world_map.bin loaded at 0x8006FAF0.  Straight-line:
 * no branches, loops, or early returns.
 *
 * ABI:
 *   $a0 = pos_vec : guest ptr, s32 X/+0, Y/+4, Z/+8 in 20.12 fixed
 *         point (each component read with lw then sra 12)
 *   $a1 = y_offset: PLAIN INTEGER (both retail callers pass the
 *         immediate 0x70) — used once, arithmetically, to derive the
 *         second probe point's Y.  NOT a pointer.
 *   $a2 = attr    : u16 (andi 0xFFFF), attr-record index;
 *         rec = *(0x8009C620) + 84*attr
 *   $a3 = node_id : u16 (andi 0xFFFF); node = rec[0x44] + 8 + 14*id
 *   $v0 = s32 in {0, -1}: ((dotA ^ dotB) >> 31) arithmetic — -1 when
 *         the two probe points straddle the node triangle's plane,
 *         0 when on the same side (dot == 0 counts as non-negative).
 *         The 95414 filter loop discards a candidate on 0.
 *
 * Algorithm: copy the record's MATRIX (rec+0x20..0x3F) to scratch
 * 0x1F8000F0, trans = (0, rec[0xC], 0), ScaleMatrix by
 * (0x800,0x800,0x800), SetRotMatrix + SetTransMatrix; RotTrans the
 * node triangle's three vertices (s16 indices at node +0/+2/+4 into
 * the SVECTOR array at *(rec[0x44]+4)); build edge vectors in place,
 * OuterProduct0, sra 2 each component, VectorNormal; build two packed
 * s16 probe points (row1.Y = row0.Y - y_offset; Z sign-inverted vs X:
 * dz = rec[0x10] - pos.Z - v0.z while dx = pos.X - rec[8] - v0.x);
 * ApplyMatrixLV dots both rows (row2 is uninitialized scratch whose
 * dot is computed but dead); return the xor-sign of the two dots.
 */
#ifndef WORLD_MAP_HELPER_85418_H
#define WORLD_MAP_HELPER_85418_H

#include "common.h"

s32 wm_80085418(u32 pos_vec, s32 y_offset, u32 attr, u32 node_id);

#endif /* WORLD_MAP_HELPER_85418_H */
