/*
 * World-map collision dispatcher 0x80094A5C.
 *
 * Retail boundary: [0x80094A5C, 0x800951A8), 1868 bytes / 467
 * instructions in world_map.bin loaded at 0x8006FAF0.
 *
 * ABI:
 *   $a0 = base vector (u32 guest address; s32 X at +0, s32 Z at +8)
 *   $a1 = direction vector (u32 guest address; s32 X at +0, s32 Z at +8)
 *   $a2 = signed scale (fixed-point 12-bit fractional)
 *   $a3 = mode (only the low 16 bits are meaningful)
 *   $v0 = 1 on success (position written to workspace[0..0xC]),
 *         0 if the generic tail walkability check blocks, or the
 *         nonzero code of a private probe helper (2 = X-block candidate
 *         chosen, 3 = Z-block candidate chosen).
 *
 * Workspace is the scratchpad 0x1F800000.  The four private probe
 * helpers (0x8009443C/945C8/94750/948D8) operate on that same region.
 *
 * This is a register-width-faithful transcription.  No safety logic,
 * no scratchpad migration, no semantic reinterpretation beyond the
 * exact MIPS control flow recovered in docs/evidence/w34b22-i5-94a5c.
 */
#ifndef WORLD_MAP_FUNC_94A5C_H
#define WORLD_MAP_FUNC_94A5C_H

#include "common.h"

s32 wm_80094A5C(u32 base_vec, u32 direction_vec, s32 scale, s32 mode);

#if defined(WM_94A5C_TEST_HOOK)
/* Test seam: override the external GTE/math probe 0x8004A70C and record
 * dispatch decisions.  In retail builds neither symbol is emitted. */
typedef s32 (*wm_94a5c_probe_fn)(s32 a0, s32 a1, s32 a2, s32 a3);
typedef void (*wm_94a5c_trace_fn)(s32 stage, s32 helper_id, s32 v);
void wm_80094A5C_set_probe(wm_94a5c_probe_fn fn);
void wm_80094A5C_set_trace(wm_94a5c_trace_fn fn);
#endif

#endif /* WORLD_MAP_FUNC_94A5C_H */
