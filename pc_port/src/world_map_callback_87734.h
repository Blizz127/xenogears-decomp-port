/*
 * World-map scheduler callback 0x80087734 (slot 15 Table-B cb1).
 *
 * Retail boundary: [0x80087734, 0x800877E0), 172 bytes / 43
 * instructions. Slice SHA-256:
 * 930c20086d5082c3c42e39a2903e153a41b9e086e4a7b3afa1a8fbafc7787e3c
 *
 * Twin 0x800877E0 is a distinct Table-B stream and is not aliased here.
 */
#ifndef WORLD_MAP_CALLBACK_87734_H
#define WORLD_MAP_CALLBACK_87734_H

#include "common.h"

#define WM_87734_TRACE_LHU  1u
#define WM_87734_TRACE_LW   2u
#define WM_87734_TRACE_SH   3u
#define WM_87734_TRACE_SW   4u
#define WM_87734_TRACE_CALL 5u

#if defined(WM_87734_TEST_TRACE)
void wm_87734_test_trace(u32 pc, u32 kind, u32 address, u32 width,
                         u32 value);
void wm_87734_test_rotmatrixyxz(u32 pc, u32 r_addr, u32 m_addr);
#endif

s32 wm_80087734(s32 slot_index);

#endif /* WORLD_MAP_CALLBACK_87734_H */
