/*
 * World-map scheduler callback 0x8008BB40.
 *
 * Retail boundary: [0x8008BB40, 0x8008BD1C), 0x1DC bytes / 119
 * instructions.  Scheduler slot index is the only argument.  The callback
 * returns 1 when resource channel 2 is present and 3 when it is suppressed;
 * the scheduler, not this callback, stores that return as slot state.
 */
#ifndef WORLD_MAP_CALLBACK_8BB40_H
#define WORLD_MAP_CALLBACK_8BB40_H

#include "common.h"

s32 wm_8008BB40(s32 slot_index);

#endif /* WORLD_MAP_CALLBACK_8BB40_H */
