/*
 * WorldMapMain main loop 0x80071034.
 *
 * Retail boundary: [0x80071034, 0x800710E4), ~44 instructions.
 * Mode dispatch + scheduler + frame driver + sync loop.
 */
#ifndef WORLD_MAP_MAIN_LOOP_71034_H
#define WORLD_MAP_MAIN_LOOP_71034_H

#include "common.h"

void wm_80071034(void);

#endif
