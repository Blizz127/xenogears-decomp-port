/*
 * World-map callback 0x800923A8 — DRAWENV/tpage/clip initializer (W34B5J).
 *
 * Retail slice: 0x800923A8–0x8009259C (126 instructions, 0x1F8 bytes).
 * Scheduler slot 0, state-0 callback. Returns 1 (transitions to state 1).
 *
 * Initializes:
 *   slot+0x50 = 0xF8
 *   DR_TPAGE at 0x8009D310 (SetDrawTPage, E1h draw-mode cmd)
 *   DRAWENV-like region at 0x8009CE6C (byte fields, SetSemiTrans,
 *     36-byte copy, clip/ofs halfwords 320×216)
 *
 * Direct dependencies: GetTPage, SetDrawTPage, SetSemiTrans.
 *
 * Exact bounded transcription of fresh retail decode (W34B5-I).
 */
#ifndef WORLD_MAP_CALLBACK_923A8_H
#define WORLD_MAP_CALLBACK_923A8_H

#include "common.h"

/* Production callback.  a0 = slot index; returns s16 (new slot state). */
s16 wm_800923A8(int slot_index);

/* Per-world-init reset. */
void wm_cb923a8_reset(void);

/* Instrumentation. */
int wm_cb923a8_get_calls(void);

#endif /* WORLD_MAP_CALLBACK_923A8_H */
