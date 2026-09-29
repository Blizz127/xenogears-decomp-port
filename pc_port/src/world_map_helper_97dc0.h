/*
 * World-map helper 0x80097DC0 (asset loader B).
 *
 * Retail boundary: [0x80097DC0, 0x80098044), 161 instructions / 644 bytes.
 * Similar to wm_80098CC0 but with different table layout. Loads assets
 * from archive, allocates buffers, queues data copies.
 */
#ifndef WORLD_MAP_HELPER_97DC0_H
#define WORLD_MAP_HELPER_97DC0_H

#include "common.h"

void wm_80097DC0(void);

#endif
