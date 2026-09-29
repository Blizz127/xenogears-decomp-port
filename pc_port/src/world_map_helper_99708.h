/*
 * World-map helper 0x80099708 (tile coordinate processor).
 *
 * Retail boundary: [0x80099708, 0x8009980C), 65 instructions / 260 bytes.
 * Builds the 9×9 transformed vertex grid in scratchpad, then submits the
 * corresponding 8×8 terrain cells through wm_8009980C.
 */
#ifndef WORLD_MAP_HELPER_99708_H
#define WORLD_MAP_HELPER_99708_H

#include "common.h"

void wm_80099708(u32 tile_data, u32 ot_base, u32 packet_base, u32 origin);

#endif
