/*
 * World-map helper 0x80090FB4 (camera control handler).
 *
 * Retail boundary: [0x80090FB4, 0x80091430), 287 instructions / 1148 bytes.
 * Handles camera heading, speed, and zoom from D-pad input. Four sub-states
 * on slot[+0x64]: init(0), main(1), zoom-in(2), zoom-out(3).
 *
 * Heading: accumulates into slot[+0x58] with acceleration (slot[+0x5C]).
 * Speed: slot[+0x60] with +/-0x1000 acceleration, clamped to [-speed, +speed].
 * Zoom: slot[+0x70] with +/-0x4000 per frame, clamped to [-0x80000, 0x80000].
 * Velocity: rcos(heading) for X, rcos(zoom>>12) for Y, -rsin(heading) for Z,
 * then VectorNormal for unit vector.
 */
#ifndef WORLD_MAP_HELPER_90FB4_H
#define WORLD_MAP_HELPER_90FB4_H

#include "common.h"

s32 wm_80090FB4(u32 slot_record);

#endif
