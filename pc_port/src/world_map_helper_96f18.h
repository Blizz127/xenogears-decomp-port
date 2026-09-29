/*
 * World-map helper 0x80096F18 (GTE view matrix builder).
 *
 * Retail boundary: [0x80096F18, 0x80097070), 86 instructions / 344 bytes.
 * Builds a view matrix from position, height offset, and rotation.
 * Calls RotMatrixYXZ, ApplyMatrixLV, and ApplyMatrix.
 */
#ifndef WORLD_MAP_HELPER_96F18_H
#define WORLD_MAP_HELPER_96F18_H

#include "common.h"

void wm_80096F18(u32 out_matrix, u32 pos_vec, s32 height, u32 rot_svec);

#endif
