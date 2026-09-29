/*
 * World-map region scanner 0x80084D00.
 *
 * Retail boundary: [0x80084D00, 0x80084DB8), 184 bytes / 46
 * instructions in world_map.bin loaded at 0x8006FAF0.
 *
 * ABI:
 *   $a0 = guest position vector (forwarded to wm_80084DB8)
 *   $a1 = guest s16* receiving the hit region index (sh)
 *   $v0 = sign16(first nonzero wm_80084DB8 result) = 2 x candidate
 *         count, or 0 when no enabled region contains the position.
 *
 * The region count at 0x8009D7E0 (lh) is RELOADED on every loop
 * iteration; region records are 84 bytes from *(0x8009C620); only
 * regions with bit 0 of the u16 at rec+4 are probed; the scan stops at
 * the FIRST region whose probe returns nonzero (after s16 truncation).
 */
#ifndef WORLD_MAP_HELPER_84D00_H
#define WORLD_MAP_HELPER_84D00_H

#include "common.h"

s32 wm_80084D00(u32 pos_vec, u32 out_attr);

#endif /* WORLD_MAP_HELPER_84D00_H */
