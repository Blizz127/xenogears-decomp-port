/*
 * World-map R4_WORLD callback 0x80071A58 (the final Phase 5 target).
 *
 * Retail boundary: [0x80071A58, 0x80071B98), 81 instructions / 324 bytes.
 * The world-map R4_WORLD rendering callback. Orchestrates all Phase 3-5
 * helpers: GTE matrix setup, particle spawning, object rendering,
 * terrain processing, asset loading, and OT dispatch.
 *
 * Always returns 1.
 */
#ifndef WORLD_MAP_R4WORLD_71A58_H
#define WORLD_MAP_R4WORLD_71A58_H

#include "common.h"

s32 wm_80071A58(s32 slot_idx);

#endif
