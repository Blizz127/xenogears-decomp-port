/*
 * World-map scheduler callback 0x80092FD8 (slot-13 cb1).
 *
 * Retail boundary: [0x80092FD8, 0x800931B0), 472 bytes / 118 instructions.
 * Similar to 92C70 but reads from D_8009CE68 instead of D_8009BD24.
 * UI callback with string lookup + palette color blending.
 */
#ifndef WORLD_MAP_CALLBACK_92FD8_H
#define WORLD_MAP_CALLBACK_92FD8_H

#include "common.h"

s32 wm_80092FD8(s32 slot_idx);

#endif
