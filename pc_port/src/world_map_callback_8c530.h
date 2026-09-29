/*
 * World-map scheduler callback 0x8008C530.
 *
 * Retail boundary: [0x8008C530, 0x8008C6EC), 0x1BC bytes / 111
 * instructions.  The scheduler supplies a slot index; this callback derives
 * the direct guest slot address, initializes channel zero, publishes the
 * resulting position/history state, and returns the C364 result unchanged.
 */
#ifndef WORLD_MAP_CALLBACK_8C530_H
#define WORLD_MAP_CALLBACK_8C530_H

#include "common.h"

s32 wm_8008C530(s32 slot_index);

#endif /* WORLD_MAP_CALLBACK_8C530_H */
