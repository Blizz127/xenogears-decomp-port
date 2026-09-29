/*
 * World-map scheduler callback 0x8008A72C (slot-1 world player handler).
 *
 * Retail boundary: [0x8008A72C, 0x8008B2BC), 2960 bytes / 740
 * instructions in world_map.bin loaded at 0x8006FAF0.
 *
 * ABI:
 *   $a0 = slot index (observed 1 on the natural route)
 *   $v0 = CONSTANT 1 on every path ($s5 preset in the prologue): the
 *         scheduler stores (s16)1 to slot+0x00, keeping the slot in
 *         state 1 — this is the persistent per-frame player handler.
 *
 * Three-level state machine:
 *   1. transient substate lh slot[+0x04] (3 -> state 1; 2 -> state 0x28;
 *      6 -> lap-counter bump gated on D_8009C170),
 *   2. 65-way main-state dispatch lh slot[+0x20] via JT1 @0x80070518
 *      (sltiu 0x41; 19 live targets, 46 parked, OOR -> common tail),
 *   3. init sub-dispatch on wm_80090A84's class via JT2 @0x80070620
 *      (idx = class-1, sltiu 5; OOR = the live movement block).
 * Common tail mirrors pos/heading to resident 0x8006EE54/56/58 and calls
 * wm_80074794 while slot[+0x24] == 0.
 */
#ifndef WORLD_MAP_CALLBACK_8A72C_H
#define WORLD_MAP_CALLBACK_8A72C_H

#include "common.h"

s32 wm_8008A72C(s32 slot_idx);

#endif /* WORLD_MAP_CALLBACK_8A72C_H */
