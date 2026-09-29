/*
 * World-map proximity/region query 0x8008C040.
 *
 * Retail boundary: [0x8008C040,0x8008C1DC) 412 bytes / 103 instrs
 * Role: first-match proximity query over table at 0x8009BE6C, count at
 * 0x8009BD04. Uses wm_80093534 for X/Z wrap and SquareRoot0 for distance.
 */

#ifndef WORLD_MAP_HELPER_8C040_H
#define WORLD_MAP_HELPER_8C040_H

#include "common.h"

void wm_8008C040(u32 vec_addr, s32 arg1, s32 arg2, u32 out1_addr, u32 out2_addr);

#endif
