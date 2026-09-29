/*
 * World-map terrain-class dispatch helper 0x8008C1DC.
 *
 * Retail boundary: [0x8008C1DC, 0x8008C28C), 0xB0 bytes / 44
 * instructions in world_map.bin loaded at 0x8006FAF0.
 *
 * ABI:
 *   $a0 = record index / context (forwarded to the callee)
 *   $a1 = guest object record (vector at +0x28, heading +0x2C,
 *         Z +0x30, extra halfword source +0x5C)
 *   $a2 = guest output context (SVECTOR pair at +0xA0 / +0xA8)
 *   If sign16(wm_80093F18(obj+0x28)) == 3: build the two packed
 *   SVECTORs (position sra 12 into +0xA0..A4, zeros into +0xA8/+0xAC,
 *   low half of obj[+0x5C] into +0xAA) and call
 *   wm_80089160(ctx, out+0xA0, out+0xA8); else call wm_800894C8(ctx).
 *   Retail $v0 is the callee residue; both accepted callees are void
 *   and all 8 retail call sites discard $v0 (verified) -> void in C.
 */
#ifndef WORLD_MAP_HELPER_8C1DC_H
#define WORLD_MAP_HELPER_8C1DC_H

#include "common.h"

void wm_8008C1DC(u32 ctx, u32 obj, u32 out);

#endif /* WORLD_MAP_HELPER_8C1DC_H */
