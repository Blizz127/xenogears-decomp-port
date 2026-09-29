/*
 * World-map helper 0x80089748 (particle effect spawner/updater).
 *
 * Retail boundary: [0x80089748, 0x80089C78), 332 instructions / 1328 bytes.
 * Walks 512 effect-source records at 0x54-byte stride, spawns eligible
 * sources into the dynamic 256 x 0x4c particle pool through the retail
 * matrix/random-vector construction, then calls wm_80089580.
 */
#ifndef WORLD_MAP_HELPER_89748_H
#define WORLD_MAP_HELPER_89748_H

#include "common.h"

void wm_80089748(void);

#endif
