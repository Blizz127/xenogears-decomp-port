/*
 * World-map fixed-point vector unloader 0x8008E034.
 *
 * Retail boundary: [0x8008E034, 0x8008E078), 0x44 bytes / 17 instructions.
 * Pure leaf that loads three 32-bit fixed-point words from the caller-supplied
 * input buffer, performs arithmetic right shift by 12 on each, and stores the
 * low 16 bits of each result as signed halfwords to 0x8006EE60/62/64.
 */
#ifndef WORLD_MAP_HELPER_8E034_H
#define WORLD_MAP_HELPER_8E034_H

#include "common.h"

void wm_8008E034(u32 in_addr);

#endif /* WORLD_MAP_HELPER_8E034_H */
