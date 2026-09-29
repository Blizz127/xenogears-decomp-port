/*
 * World-map scheduler callback 0x800907F4 (slot 8 Table-A cb1).
 *
 * Retail boundary: [0x800907F4, 0x80090A18), 0x224 bytes / 137
 * instructions. Slice SHA-256:
 * bd2946bda218185bd796696ef10c845d09f2ebfb9a99814d090e303ae46cc91b
 *
 * Host RotMatrixZYX is bound to PsyCross RotMatrixZYX_gte (the unmangled
 * RotMatrixZYX name is a production stub). Focused tests intercept the
 * two calls and do not require GTE math.
 */
#ifndef WORLD_MAP_HELPER_907F4_H
#define WORLD_MAP_HELPER_907F4_H

#include "common.h"

#define WM_907F4_TRACE_LHU  1u
#define WM_907F4_TRACE_LH   2u
#define WM_907F4_TRACE_LW   3u
#define WM_907F4_TRACE_SH   4u
#define WM_907F4_TRACE_SW   5u
#define WM_907F4_TRACE_CALL 6u

#if defined(WM_907F4_TEST_TRACE)
void wm_907f4_test_trace(u32 pc, u32 kind, u32 address, u32 width,
                         u32 value);
void wm_907f4_test_rotmatrixzyx(u32 pc, u32 r_addr, u32 m_addr);
#endif

s32 wm_800907F4(s32 slot_index);

#endif /* WORLD_MAP_HELPER_907F4_H */
