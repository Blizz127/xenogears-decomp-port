/*
 * World-map helper 0x80090C68 (slot-4 heading input handler).
 *
 * Retail boundary: [0x80090C68, 0x80090E14), 107 instructions.
 *
 * Reads D-pad direction from 0x8009CD4C (u16 >> 12, 1..12 mapped),
 * adjusts slot[+0x48] heading by directional offsets, computes velocity
 * from heading via rcos/rsin when upper button bits are set, and checks
 * boundary conditions via wm_80090A18.
 *
 * Returns 0 on normal path, 3 on boundary-hit path.
 */
#ifndef WORLD_MAP_HELPER_90C68_H
#define WORLD_MAP_HELPER_90C68_H

#include "common.h"

s32 wm_80090C68(u32 slot_record);

#endif /* WORLD_MAP_HELPER_90C68_H */
