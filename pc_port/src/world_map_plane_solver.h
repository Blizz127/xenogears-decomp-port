/*
 * World-map terrain plane-Y solver (0x800935DC).
 *
 * Given plane coefficients, base point, and surface normal as
 * guest-memory 12-byte s32 records, solves for the terrain height (Y)
 * at a given X/Z point.
 *
 * Inputs (as guest-memory 12-byte s32 records):
 *   a0 = coefficients: {A[0], result_slot, A[2]}
 *   a1 = base point:    {B[0], B[1], B[2]}
 *   a2 = surface normal: {N[0], N[1], N[2]}
 *
 * Returns: a0[1] + a1[1] (computed plane height + base height).
 * Side effect: stores final result to a0[1].
 */
#ifndef XENO_WORLD_MAP_PLANE_SOLVER_H
#define XENO_WORLD_MAP_PLANE_SOLVER_H

#include "common.h"

u32 wm_800935DC(u32 a0, u32 a1, u32 a2);

#endif /* XENO_WORLD_MAP_PLANE_SOLVER_H */
