/*
 * World-map scheduler callback 0x800922AC (slot-11 cb1).
 *
 * Retail boundary: [0x800922AC, 0x800923A8), 252 bytes / 63 instructions.
 * Leaf function, no external calls. Simple scroll value animator:
 * state 0 = idle (return 3), state 1 = decrement, state 2 = increment.
 * Writes D_8009BE0C = slot[+0x50] >> 12 each frame.
 */
#ifndef WORLD_MAP_CALLBACK_922AC_H
#define WORLD_MAP_CALLBACK_922AC_H

#include "common.h"

s32 wm_800922AC(s32 slot_idx);

#endif
