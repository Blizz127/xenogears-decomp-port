/*
 * World-map callback 0x800925A0 — second-pass fade/OT callback (W34B20-B).
 *
 * Retail slice: [0x800925A0,0x80092BE4), 0x644 bytes / 401 instructions.
 * a0 is the scheduler slot index; v0 is the next scheduler state (1 or 3).
 */
#ifndef WORLD_MAP_CALLBACK_925A0_H
#define WORLD_MAP_CALLBACK_925A0_H

#include "common.h"

s32 wm_800925A0(s32 slot_index);

#if defined(WM_925A0_TEST_TRACE)
enum wm_925a0_trace_kind {
    WM_925A0_TRACE_LW = 1,
    WM_925A0_TRACE_LH,
    WM_925A0_TRACE_LHU,
    WM_925A0_TRACE_SW,
    WM_925A0_TRACE_SH,
    WM_925A0_TRACE_SB,
    WM_925A0_TRACE_CALL,
    WM_925A0_TRACE_RETURN,
    WM_925A0_TRACE_ENTRY,
    WM_925A0_TRACE_CALLBACK_RETURN
};

void wm_925a0_test_trace(u32 pc, u32 kind, u32 address,
                         u32 width, u32 value);
#endif

#endif /* WORLD_MAP_CALLBACK_925A0_H */
