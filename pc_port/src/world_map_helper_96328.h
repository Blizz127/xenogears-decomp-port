/*
 * World-map helper 0x80096328 (queue record allocator variant A).
 *
 * Retail boundary: [0x80096328, 0x800963E4), 47 instructions / 188 bytes.
 * Allocates a record from the queue at D_8009BE08, initializes via
 * wm_800963E4, stores pointer in D_8009D788 table, advances index (mod 16).
 * Returns 0 on success, -1 if record already in use.
 */
#ifndef WORLD_MAP_HELPER_96328_H
#define WORLD_MAP_HELPER_96328_H

#include "common.h"

s32 wm_80096328(void);

#endif
