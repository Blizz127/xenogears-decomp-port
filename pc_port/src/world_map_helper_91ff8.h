/*
 * World-map helper 0x80091FF8 (camera view setup with terrain sampling).
 *
 * Retail boundary: [0x80091FF8, 0x80092234), 143 instructions / 572 bytes.
 * Builds view matrix via wm_80096F18, then samples a 6×6 grid of
 * terrain heights via wm_80093354/wm_80093660 to find the best
 * camera position candidate.
 */
#ifndef WORLD_MAP_HELPER_91FF8_H
#define WORLD_MAP_HELPER_91FF8_H

#include "common.h"

s32 wm_80091FF8(s32 max_candidates, u32 rot_table, u32 height_table);

#endif
