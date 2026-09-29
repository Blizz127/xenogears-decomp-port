/*
 * World-map helpers 0x8009623C..0x8009699C — primitive submission queue
 * and related leaf functions for the world-map rendering system.
 *
 * These are small leaf functions (29-70 insns each) that manage a
 * primitive record queue and related operations.
 */
#ifndef WORLD_MAP_HELPER_9623C_H
#define WORLD_MAP_HELPER_9623C_H

#include "common.h"

/* Queue writer: writes 3 words to next record slot. Returns 0 on success, -1 if full. */
s32 wm_8009623C(u32 word0, u32 word1, u32 word2);

/* Queue writer variant with different record layout */
s32 wm_800962B0(u32 word0, u32 word1, u32 word2, u32 word3);

/* Queue record initializer */
void wm_800963E4(u32 record_addr);

/* 16-byte record insertion/sort pass */
void wm_800964B0(u32 list);

/* Queue counter reset */

/* Queue flush helper */
void wm_80099708(u32 tile_data, u32 ot_base, u32 packet_base, u32 origin);

/* CD40 sink (simple record writer) */
void wm_80086700(u32 addr, u32 val);

#endif
