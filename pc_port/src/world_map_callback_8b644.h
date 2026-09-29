/*
 * World-map scheduler callback 0x8008B644 (slot-2/3 NPC follower handler).
 *
 * Retail boundary: [0x8008B644, 0x8008BB40), 1276 bytes / 319
 * instructions in world_map.bin loaded at 0x8006FAF0.
 *
 * ABI:
 *   $a0 = slot index (observed 2 or 3 on the natural route)
 *   $v0 = CONSTANT 1 on every path ($s4 preset in prologue): the
 *         scheduler stores (s16)1 to slot+0x00, keeping the slot in
 *         state 1.
 *
 * Two-level state machine:
 *   1. transient substate sh slot[+0x04] (1 -> load from table; 2 ->
 *      set main=0x28, sub=0; 3 -> set main=0x01, sub=0; 5 -> set
 *      main=0x30, sub=0),
 *   2. 65-entry main-state dispatch sh slot[+0x20] via JT @0x80070670
 *      (sltiu 0x41; 6 live targets, 59 parked, OOR -> common tail).
 *
 * Live JT targets:
 *   0x00,0x01 -> breadcrumb path follower (ring buffer or neighbor copy)
 *   0x02      -> copy from neighbor slot+0x180
 *   0x08      -> approach step (wm_800941C4)
 *   0x09      -> approach completion check (wm_8008BEC8)
 *   0x0A      -> teleport init with coord table lookup
 *   0x28      -> teleport step with rcos/rsin heading offset
 *   0x29      -> teleport completion check (wm_8008BEC8)
 *   0x2A      -> state reset to 0x28
 *   0x30      -> presence clear + state reset
 *   0x32      -> copy from neighbor pool+0xA8
 *
 * Common tail: mirrors pos/heading to resident 0x8006EE54/56/58 and
 * calls wm_80074794 while slot[+0x24] == 0.
 */
#ifndef WORLD_MAP_CALLBACK_8B644_H
#define WORLD_MAP_CALLBACK_8B644_H

#include "common.h"

s32 wm_8008B644(s32 slot_idx);

#endif /* WORLD_MAP_CALLBACK_8B644_H */
