/*
 * World-map helper 0x8009980C (GTE quad renderer).
 *
 * Retail boundary: [0x8009980C, 0x80099BFC), 252 instructions / 1008 bytes.
 * Leaf function. Processes an 8×8 cell grid as two GTE triangles per cell,
 * depth-buckets accepted packets into the guest OT, and advances the shared
 * packet count at 0x8009D7DC.
 */
#ifndef WORLD_MAP_HELPER_9980C_H
#define WORLD_MAP_HELPER_9980C_H

#include "common.h"

void wm_8009980C(u32 tile_data, u32 ot_base, u32 packet_base);

#endif
