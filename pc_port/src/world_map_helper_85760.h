/* World-map XZ triangle classifier 0x80085760. */
#ifndef WORLD_MAP_HELPER_85760_H
#define WORLD_MAP_HELPER_85760_H

#include "common.h"

/*
 * Retail ABI: a0 = pos VECTOR*, a1 = projected VECTOR*,
 * a2 = cell_index, a3 = tri_index, v0 = 3-bit XZ nclip mask.
 *
 * PC-port arguments are guest addresses of the two VECTOR records so
 * scratchpad (0x1F800060) and main-RAM pointers share one mapping.
 */
s32 wm_80085760(u32 pos_addr, u32 projected_addr, s32 cell_index,
                s32 tri_index);

u32 wm_80085760_get_exec_count(void);
void wm_80085760_reset_exec_count(void);

#if defined(WM_85760_TEST_TRACE)
void wm_85760_test_trace(u32 kind, u32 address, u32 width, u32 value);
#endif

enum wm_85760_trace_kind {
    WM_85760_TRACE_STORE = 1,
    WM_85760_TRACE_CALL_SCALE = 2,
    WM_85760_TRACE_CALL_SETROT = 3,
    WM_85760_TRACE_CALL_SETTRANS = 4,
    WM_85760_TRACE_CALL_RTV0TR = 5,
    WM_85760_TRACE_CALL_NCLIP = 6,
    WM_85760_TRACE_CALL_VN = 7
};

#endif /* WORLD_MAP_HELPER_85760_H */
