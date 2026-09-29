/*
 * World-map scheduler callback 0x8008B2BC.
 *
 * Retail boundary: [0x8008B2BC, 0x8008B498), 0x1DC bytes / 119
 * instructions.  Scheduler slot index is the only argument.  The callback
 * returns 1 when resource channel 1 is present and 3 when it is suppressed;
 * the scheduler, not this callback, stores that return as slot state.
 */
#ifndef WORLD_MAP_CALLBACK_8B2BC_H
#define WORLD_MAP_CALLBACK_8B2BC_H

#include "common.h"

s32 wm_8008B2BC(s32 slot_index);

#endif /* WORLD_MAP_CALLBACK_8B2BC_H */
