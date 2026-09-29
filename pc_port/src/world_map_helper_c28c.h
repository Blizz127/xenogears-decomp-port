/*
 * World-map SpriteData allocation helper 0x8008C28C.
 *
 * Retail boundary: [0x8008C28C, 0x8008C364), 0xD8 bytes / 54 instructions.
 * The caller supplies a guest scheduler-slot address and resource channel.
 * All retail callers use channels 0, 1, or 2 and ignore the return registers.
 */
#ifndef WORLD_MAP_HELPER_C28C_H
#define WORLD_MAP_HELPER_C28C_H

#include "common.h"

void wm_8008C28C(u32 slot_addr, s32 channel);

#endif /* WORLD_MAP_HELPER_C28C_H */
