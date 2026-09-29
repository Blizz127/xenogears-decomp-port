/*
 * World-map helper 0x80089580 (particle system tick).
 *
 * Retail boundary: [0x80089580, 0x80089748), 114 instructions / 456 bytes.
 * Leaf function. Iterates 256 particle entries (stride 0x4c). A nonzero
 * owner halfword and positive remaining-life halfword select two-stage vector
 * integration, UV motion, and clamped RGB motion. Expired owned records
 * decrement their 0x54-byte owner-slot count and are cleared.
 */
#ifndef WORLD_MAP_HELPER_89580_H
#define WORLD_MAP_HELPER_89580_H

#include "common.h"

void wm_80089580(void);

#endif
