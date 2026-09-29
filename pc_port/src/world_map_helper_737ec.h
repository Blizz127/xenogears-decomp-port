/*
 * World-map helper 0x800737EC (sky dome renderer).
 *
 * Retail boundary: [0x800737EC, 0x800739B8), 115 instructions / 460 bytes.
 * Renders the sky dome backdrop using GTE matrix operations.
 * Sets up rotation from heading, composites camera matrix,
 * iterates dome vertices with RotTransPers4, colors primitives.
 */
#ifndef WORLD_MAP_HELPER_737EC_H
#define WORLD_MAP_HELPER_737EC_H

#include "common.h"

void wm_800737EC(void);

#endif
