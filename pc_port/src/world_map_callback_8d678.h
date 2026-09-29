/*
 * World-map scheduler callback 0x8008D678 (slot-5/6 NPC follower handler).
 *
 * Retail boundary: [0x8008D678, 0x8008DD6C), 1780 bytes / 445
 * instructions in world_map.bin loaded at 0x8006FAF0.
 *
 * ABI:
 *   $a0 = slot index (observed 5 or 6 on the natural route)
 *   $v0 = CONSTANT 1 on every path ($v0 preset before return)
 *
 * Two-level state machine:
 *   1. Pre-dispatch: lhu(slot[+0x04])-1, sltiu 8, JT @0x800708D4.
 *   2. Main dispatch: lhu(slot[+0x20]), sltiu 0x41, JT @0x800708F4.
 *
 * Live pre-dispatch targets:
 *   [0] set main=0x08, load source from table
 *   [1] set main=0x20
 *   [2] set main=0x30
 *   [3] set main=0x40, write presence byte=7
 *   [4] set main=0x10
 *   [5,6] fall through (no state change)
 *   [7] set main=0x18
 *
 * Live main targets:
 *   0x00,0x01 — ring buffer follower (same shape as 8B644)
 *   0x02      — neighbor copy (pool+0x3A8 pos, pool+0x3C8 heading)
 *   0x08      — approach step (wm_800941C4)
 *   0x09      — approach completion (wm_8008BEC8)
 *   0x0A      — teleport init (wm_80097770)
 *   0x10      — heading init (ratan2 + rcos/rsin)
 *   0x11      — heading completion (wm_8008BEC8)
 *   0x12      — presence check (wm_80097770)
 *   0x18      — wm_80089160 presence registration
 *   0x19      — timer countdown
 *   0x1A      — timer → anim frame 3
 *   0x1B      — timer countdown
 *   0x20      — presence check with mode_word gating
 *   0x21      — presence init (wm_80089160)
 *   0x30      — neighbor copy + anim
 *   0x32      — wm_80097770 check
 *   0x34      — state reset to 0
 *   0x35      — idle/anim clear
 *   0x30      — neighbor approach (wm_8008DFF4 + rcos/rsin)
 *   0x31      — completion check (wm_8008BEC8)
 *   0x32      — state reset to 1
 *
 * Common tail: gates wm_80074794 on main_state!=2 &&
 *   (mode_word!=2 || presence!=7).  Epilogue writes pos.x>>12, pos.z>>12,
 *   and heading to the resident mirror at 0x8006EE54/56/E58+.
 */
#ifndef WORLD_MAP_CALLBACK_8D678_H
#define WORLD_MAP_CALLBACK_8D678_H

#include "common.h"

s32 wm_8008D678(s32 slot_idx);

#endif /* WORLD_MAP_CALLBACK_8D678_H */
