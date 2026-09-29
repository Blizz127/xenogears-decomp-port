/*
 * World-map helper 0x8009932C (rendering context setup).
 *
 * Retail boundary: [0x8009932C, 0x80099708), 247 instructions / 988 bytes.
 * Sets up world-map rendering context: copies terrain data to scratchpad,
 * composites camera matrix, sets GTE matrices, processes tiles via
 * wm_80099708.
 */
#ifndef WORLD_MAP_HELPER_9932C_H
#define WORLD_MAP_HELPER_9932C_H

#include "common.h"

void wm_8009932C(u32 ot_base, u32 packet_base, u32 position);

#endif
