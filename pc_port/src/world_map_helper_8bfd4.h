/*
 * World-map helper 0x8008BFD4 (ring buffer write).
 *
 * Retail boundary: [0x8008BFD4, 0x8008C040), 27 instructions / 108 bytes.
 * Leaf function. Writes a tagged entry to the 32-entry ring buffer at
 * 0x8009BE6C (stride 24), index at D_8009BD04.
 */
#ifndef WORLD_MAP_HELPER_8BFD4_H
#define WORLD_MAP_HELPER_8BFD4_H

#include "common.h"

void wm_8008BFD4(u16 tag, u32 src_vec, u16 half2, s16 sh_val);
/* Entry for the matched C callers (host pointer to the vector). */
void func_8008BFD4(s32 tag, s32 src_vec_host, s32 half2, s32 sh_val);

#endif
