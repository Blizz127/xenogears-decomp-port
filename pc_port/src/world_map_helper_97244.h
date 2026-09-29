/*
 * World-map helpers 0x80097244 and 0x80097440 — GTE matrix builders.
 *
 * 0x80097244 [0x80097244, 0x80097440): 127 insns. Builds rotation/normal
 * matrix from 3 edge vectors via VectorNormal + OuterProduct12.
 *
 * 0x80097440 [0x80097440, 0x8009766C): 139 insns. Matrix composition
 * with MulMatrix0, RotMatrixX/Y/Z, ApplyMatrix, TransMatrix.
 */
#ifndef WORLD_MAP_HELPER_97244_H
#define WORLD_MAP_HELPER_97244_H

#include "common.h"

void wm_80097244(u32 input_data);
void wm_80097440(u32 input_data);

#endif
