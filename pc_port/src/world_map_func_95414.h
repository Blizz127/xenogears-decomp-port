/*
 * World-map path-node movement router 0x80095414 (keystone).
 *
 * Retail boundary: [0x80095414, 0x80095CD4), 2240 bytes / 560
 * instructions in world_map.bin loaded at 0x8006FAF0.
 *
 * ABI:
 *   $a0        = position vector (guest ptr; s32 X +0, Y +4, Z +8)
 *   $a1        = direction vector (guest ptr; s32 X +0, Z +8)
 *   $a2        = output vector (guest ptr; X +0, heading/height +4, Z +8)
 *   $a3        = signed scale (20.12; mult/mflo + sra 12)
 *   0x10($sp)  = mode (raw 32-bit passthrough to wm_800951A8's stack arg)
 *   $v0        = 1  (landed on / snapped to a path node; out[4] = height<<12,
 *                    node cache updated)
 *                0  (redirected along a path link via wm_800952B0, projected
 *                    via wm_80095324, or tie-break dead-end zero-out)
 *                r  (wm_800951A8 result {0,1} on the fallback paths; node
 *                    cache reset to -1)
 *
 * Mode select on D_8009C840: == -1 -> candidate search (case 1);
 * != -1 -> cached-node graph walk (case 3) driven by wm_80085760's
 * class through the 8-slot jump table at 0x80070C50+0x30 (0x80070C80).
 * Globals: D_8009C840 (cached attr), D_8009C16C (cached node id),
 * D_8009C620 (region record table ptr), D_8009D718 (candidate list).
 * Scratchpad: 0x1F800030-0x8F workspace shared with the helper family.
 */
#ifndef WORLD_MAP_FUNC_95414_H
#define WORLD_MAP_FUNC_95414_H

#include "common.h"

s32 wm_80095414(u32 pos_vec, u32 dir_vec, u32 out_vec, s32 scale, s32 mode);

#endif /* WORLD_MAP_FUNC_95414_H */
