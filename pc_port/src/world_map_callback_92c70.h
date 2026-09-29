/*
 * World-map scheduler callback 0x80092C70 (slot-12 cb1).
 *
 * Retail boundary: [0x80092C70, 0x80092DD0), 352 bytes / 88 instructions.
 * UI-related callback with area-dependent string lookup.
 * Two states: 0 = init (string lookup + state advance), 1 = active
 * (area change detection + string update).
 */
#ifndef WORLD_MAP_CALLBACK_92C70_H
#define WORLD_MAP_CALLBACK_92C70_H

#include "common.h"

s32 wm_80092C70(s32 slot_idx);

#endif
