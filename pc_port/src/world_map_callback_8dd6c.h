/*
 * World-map scheduler callback 0x8008DD6C.
 *
 * Retail boundary: [0x8008DD6C, 0x8008DE9C), 0x130 bytes / 76
 * instructions.  The scheduler supplies a slot index; this callback derives
 * the direct guest slot address, initializes channel two, publishes its
 * position/heading state, and returns the C364 result unchanged.
 */
#ifndef WORLD_MAP_CALLBACK_8DD6C_H
#define WORLD_MAP_CALLBACK_8DD6C_H

#include "common.h"

s32 wm_8008DD6C(s32 slot_index);

#endif /* WORLD_MAP_CALLBACK_8DD6C_H */
