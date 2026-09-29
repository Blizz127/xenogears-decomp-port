/*
 * World-map scheduler callback 0x80091B54.
 *
 * Retail boundary: [0x80091B54, 0x80091C18), 0xC4 bytes / 49
 * instructions. Slice SHA-256:
 * 4ad4bec23e98f56c2b7133231457fba791da1b7274c5f53837278dfcad81de9c
 */
#ifndef WORLD_MAP_CALLBACK_91B54_H
#define WORLD_MAP_CALLBACK_91B54_H

#include "common.h"

/* Test-only exact guest-access observer. Ordinary production builds do not
 * define WM_91B54_TEST_TRACE and therefore have no observer dependency. */
#define WM_91B54_TRACE_LW 1u
#define WM_91B54_TRACE_SH 2u
#define WM_91B54_TRACE_SW 3u

#if defined(WM_91B54_TEST_TRACE)
void wm_91b54_test_trace(u32 pc, u32 kind, u32 address,
                         u32 width, u32 value);
#endif

s32 wm_80091B54(s32 slot_index);

#endif /* WORLD_MAP_CALLBACK_91B54_H */
