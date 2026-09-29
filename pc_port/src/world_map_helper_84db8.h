/*
 * World-map nav-mesh point-in-triangle probe 0x80084DB8.
 *
 * Retail boundary: [0x80084DB8, 0x80085158), 928 bytes / 232
 * instructions in world_map.bin loaded at 0x8006FAF0.
 *
 * ABI:
 *   $a0 = guest position vector (s32 X at +0, s32 Z at +8, 20.12)
 *   $a1 = region index (callers pass sign-extended s16)
 *   $v0 = 2 x number of containing triangles appended to the candidate
 *         list at 0x8009D718 (4-byte records: s16 node id at +0,
 *         u16 node type at +2); 0 when the +-0x800 region gate rejects
 *         or the mesh is empty.
 *
 * The three inline COP2 MVMVA transforms (0x4A480012: sf=1, mx=rot,
 * v=V0, cv=TR, lm=0 + swc2 MAC1/2/3 + dead cfc2 FLAG) are bound to the
 * canonical libgte RotTrans, whose PsyCross body executes the identical
 * opcode payload (doCOP2(0x0480012)) against the same GTE state loaded
 * by SetRotMatrix/SetTransMatrix, stores MAC1..3 exactly like the
 * retail swc2 sequence, and returns FLAG through an out-parameter that
 * this transcription discards (the retail sp[0x10] store is dead).
 */
#ifndef WORLD_MAP_HELPER_84DB8_H
#define WORLD_MAP_HELPER_84DB8_H

#include "common.h"

s32 wm_80084DB8(u32 pos_vec, s32 region_idx);

#endif /* WORLD_MAP_HELPER_84DB8_H */
