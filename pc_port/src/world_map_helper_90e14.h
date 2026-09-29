/*
 * World-map helper 0x80090E14 (heading input handler variant).
 *
 * Retail boundary: [0x80090E14, 0x80090FB4), 104 instructions / 416 bytes.
 * Same structure as wm_80090C68 but with different boundary checks:
 * flag bit 0x20 gates area check, flag bit 0x40 forces return 4,
 * flag bit 0x10 checks D_8009CE68 and area, writes D_8009D804.
 */
#ifndef WORLD_MAP_HELPER_90E14_H
#define WORLD_MAP_HELPER_90E14_H

#include "common.h"

s32 wm_80090E14(u32 slot_record);

#endif
