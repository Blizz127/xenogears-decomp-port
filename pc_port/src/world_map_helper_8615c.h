/*
 * World-map helper 0x8008615C (vertex setup with rotation).
 *
 * Retail boundary: [0x8008615C, 0x800863E0), 161 instructions / 644 bytes.
 * Builds the shared FT4 projection state and dispatches a 5x5 tile window
 * through the completed wm_80099BFC submitter.
 */
#ifndef WORLD_MAP_HELPER_8615C_H
#define WORLD_MAP_HELPER_8615C_H

#include "common.h"

void wm_8008615C(void);

#endif
