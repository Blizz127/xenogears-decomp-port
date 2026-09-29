/*
 * World-map scheduler callback 0x8008E190.
 *
 * Retail boundary: [0x8008E190, 0x8008E4F4), 0x364 bytes / 217
 * instructions. The scheduler supplies a slot index; this callback derives
 * the corresponding 0x80-byte slot, updates the live world transform
 * context, and returns the retail scheduler state.
 */
#ifndef WORLD_MAP_CALLBACK_8E190_H
#define WORLD_MAP_CALLBACK_8E190_H

#include "common.h"

s32 wm_8008E190(s32 slot_index);

#endif /* WORLD_MAP_CALLBACK_8E190_H */
