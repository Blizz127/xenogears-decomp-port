/*
 * World-map helper 0x8008E078 (area-dependent table selector).
 *
 * Retail boundary: [0x8008E078, 0x8008E0F0), 30 instructions / 120 bytes.
 * Leaf function. Reads boundary byte and area byte, selects one of two
 * table pointer/size pairs based on area (15 or 16), writes to globals.
 */
#ifndef WORLD_MAP_HELPER_8E078_H
#define WORLD_MAP_HELPER_8E078_H

#include "common.h"

void wm_8008E078(void);

#endif
