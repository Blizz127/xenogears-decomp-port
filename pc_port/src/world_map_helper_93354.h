/*
 * World-map signed X/Z wrap helper 0x80093354.
 *
 * Retail boundary: [0x80093354, 0x800933EC), 152 bytes / 38
 * instructions in world_map.bin loaded at 0x8006FAF0.
 *
 * ABI: $a0 = guest VECTOR address.  No meaningful $v0; callers reload
 * the wrapped coordinates from memory.
 */
#ifndef WORLD_MAP_HELPER_93354_H
#define WORLD_MAP_HELPER_93354_H

#include "common.h"

void wm_80093354(u32 vec_addr);

u32 wm_80093354_get_exec_count(void);
void wm_80093354_reset_exec_count(void);

#endif /* WORLD_MAP_HELPER_93354_H */
