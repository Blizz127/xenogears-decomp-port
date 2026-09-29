/*
 * World-map helper 0x800981C8 (tile coordinate table builder).
 *
 * Retail boundary: [0x800981C8, 0x800983A0), 118 instructions / 472 bytes.
 * Leaf function. Reads position vector, computes tile coordinates,
 * wraps using world boundaries, copies 160-byte record, fills 8x8
 * tile lookup table.
 */
#ifndef WORLD_MAP_HELPER_981C8_H
#define WORLD_MAP_HELPER_981C8_H

#include "common.h"

void wm_800981C8(u32 pos_vec);

#endif
