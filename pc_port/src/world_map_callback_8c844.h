/*
 * World-map scheduler callback 0x8008C844 (slot-4 cb1).
 *
 * Retail boundary: [0x8008C844, 0x8008D3F0), 2972 bytes / 747
 * instructions in world_map.bin loaded at 0x8006FAF0.
 *
 * ABI: $a0 = slot index (observed 4), $v0 = CONSTANT 1.
 *
 * Two-level state machine:
 *   1. Pre-dispatch: if/else on slot[+0x04] (values 3,4,7,8).
 *   2. Main dispatch: JT @0x800707CC, 65 entries (sltiu 0x41).
 *
 * Live main targets:
 *   0x00/0x01 — ring buffer follower + wm_80090C68 + wm_80095414
 *   0x02      — neighbor copy (pool+0x3A8)
 *   0x08      — approach step with presence gating
 *   0x09      — approach completion with presence gating
 *   0x0A      — approach via wm_800941C4
 *   0x0B      — completion check + mirror write
 *   0x0C      — wm_80097770 check
 *   0x10      — mode_word dispatch (1→state1, 2→presence check, 3→gate)
 *   0x11      — heading completion check
 *   0x12      — presence check → state 0x40
 *   0x18      — timer start (wm_80089160)
 *   0x19      — timer countdown
 *   0x1A      — timer end (anim frame 3)
 *   0x20      — presence release + wm_80097770 for slots 5/6
 *   0x21      — wm_80097770 check → advance
 *   0x22      — state reset to 0
 *   0x30      — neighbor approach (wm_8008DFF4 + rcos/rsin)
 *   0x31      — completion + mirror write
 *   0x32      — ring buffer reset (copy to 32 ring entries)
 */
#ifndef WORLD_MAP_CALLBACK_8C844_H
#define WORLD_MAP_CALLBACK_8C844_H

#include "common.h"

s32 wm_8008C844(s32 slot_idx);

#endif /* WORLD_MAP_CALLBACK_8C844_H */
