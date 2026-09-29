/*
 * World-map channel object/position helper 0x8008C364.
 *
 * Retail boundary: [0x8008C364, 0x8008C530), 0x1CC bytes / 115
 * instructions.  The caller supplies a direct guest scheduler-slot address
 * and a resource channel.  All known retail callers use channels 0, 1, or 2.
 */
#ifndef WORLD_MAP_HELPER_C364_H
#define WORLD_MAP_HELPER_C364_H

#include "common.h"

s32 wm_8008C364(u32 slot_addr, s32 channel);

#endif /* WORLD_MAP_HELPER_C364_H */
