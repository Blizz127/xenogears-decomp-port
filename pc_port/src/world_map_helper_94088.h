/*
 * World-map slide-vector selector 0x80094088.
 *
 * Retail boundary: [0x80094088, 0x80094154), 0xCC bytes / 51
 * instructions in world_map.bin loaded at 0x8006FAF0.
 *
 * ABI:
 *   $a0 = guest VECTOR pointer forwarded to 0x80093FE4 (terrain
 *         attribute nibble source; the live caller 0x800951A8 passes
 *         the scratchpad workspace 0x1F800000)
 *   $a1 = guest source vector (s32 X at +0, s32 Z at +8)
 *   $a2 = guest destination vector (s32 X at +0, s32 Z at +8;
 *         +4 is NEVER written)
 *   $v0 = 0 if dot == 0 (destination receives a copy of the source
 *         X/Z), else 1 (destination receives the table direction
 *         vector, negated when dot < 0)
 *
 * dot = lo32(src.X * tab[n].X) + lo32(src.Z * tab[n].Z), 32-bit
 * wraparound, where n = wm_80093FE4(a0) and tab is the 16-entry,
 * 16-byte-stride unit-direction table at 0x8009B264 (X at +0,
 * Z at +8) inside the overlay image.
 */
#ifndef WORLD_MAP_HELPER_94088_H
#define WORLD_MAP_HELPER_94088_H

#include "common.h"

s32 wm_80094088(u32 attr_vec, u32 src_vec, u32 dst_vec);

#endif /* WORLD_MAP_HELPER_94088_H */
