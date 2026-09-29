/*
 * World-map fixed-point vector initializer 0x8008DFF4.
 *
 * Retail boundary: [0x8008DFF4, 0x8008E034), 0x40 bytes / 16 instructions.
 * Pure leaf that loads three signed halfwords from global positions
 * 0x8006EE60/62/64, sign-extends each to 32 bits, shifts left by 12,
 * and stores the resulting fixed-point words to out+0/4/8.
 */
#ifndef WORLD_MAP_HELPER_8DFF4_H
#define WORLD_MAP_HELPER_8DFF4_H

#include "common.h"

void wm_8008DFF4(u32 out_addr);

#endif /* WORLD_MAP_HELPER_8DFF4_H */
