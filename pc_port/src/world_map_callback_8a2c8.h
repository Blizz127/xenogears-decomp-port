/*
 * World-map scheduler callback 0x8008A2C8.
 *
 * Retail boundary: [0x8008A2C8, 0x8008A52C), 0x264 bytes / 153
 * instructions.  Scheduler slot index is the only argument.  Every path
 * returns 1; the scheduler, not this callback, stores that return as state.
 */
#ifndef WORLD_MAP_CALLBACK_8A2C8_H
#define WORLD_MAP_CALLBACK_8A2C8_H

#include "common.h"

s32 wm_8008A2C8(s32 slot_index);

#endif /* WORLD_MAP_CALLBACK_8A2C8_H */
