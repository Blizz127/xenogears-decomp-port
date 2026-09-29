/*
 * World-map scheduler callback 0x800914D0 (slot-9 cb1).
 *
 * Retail boundary: [0x800914D0, 0x80091B54), 1680 bytes / 417 instructions.
 * Camera-relative movement handler with heading-based approach.
 * Two-level dispatch: pre-dispatch on slot[+0x04]-9 (9 entries),
 * main dispatch on slot[+0x20] (17 entries, 4 live).
 */
#ifndef WORLD_MAP_CALLBACK_914D0_H
#define WORLD_MAP_CALLBACK_914D0_H

#include "common.h"

s32 wm_800914D0(s32 slot_idx);

#endif
