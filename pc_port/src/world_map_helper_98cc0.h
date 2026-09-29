/*
 * World-map helper 0x80098CC0 (asset loader with queue management).
 *
 * Retail boundary: [0x80098CC0, 0x8009932C), 411 instructions / 1644 bytes.
 * Paged terrain-tile refill: evicts the previous 9x9 window, fills the
 * newly exposed primary/secondary archive edges, and closes four selected
 * holes through the active CD or file-path transfer backend.
 */
#ifndef WORLD_MAP_HELPER_98CC0_H
#define WORLD_MAP_HELPER_98CC0_H

#include "common.h"

void wm_80098CC0(void);

#endif
