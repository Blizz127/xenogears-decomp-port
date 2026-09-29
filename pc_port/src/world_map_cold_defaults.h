#ifndef WORLD_MAP_COLD_DEFAULTS_H
#define WORLD_MAP_COLD_DEFAULTS_H

#include "common.h"

/* Retail WorldMapMain cold-entry block [0x80070D58, 0x80070F38).
 * host_gamestate is the PC port's authoritative GameState allocation. */
int wm_80070D58_cold_defaults(u8* host_gamestate);

#endif /* WORLD_MAP_COLD_DEFAULTS_H */
