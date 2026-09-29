/*
 * World-map scheduler callback 0x8008D3F0.
 *
 * Retail boundary: [0x8008D3F0, 0x8008D520), 0x130 bytes / 76
 * instructions.  The scheduler supplies a slot index; this callback derives
 * the direct guest slot address, initializes channel one, publishes its
 * position/heading state, and returns the C364 result unchanged.
 */
#ifndef WORLD_MAP_CALLBACK_8D3F0_H
#define WORLD_MAP_CALLBACK_8D3F0_H

#include "common.h"

s32 wm_8008D3F0(s32 slot_index);

#endif /* WORLD_MAP_CALLBACK_8D3F0_H */
