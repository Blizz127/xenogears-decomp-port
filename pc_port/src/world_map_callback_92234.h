/*
 * World-map scheduler callback 0x80092234.
 *
 * Retail boundary: [0x80092234, 0x800922AC), 0x78 bytes / 30
 * instructions. Slice SHA-256:
 * f64cd5af4bff134fa29765c63c52aa83429df8a379323f97196d70ccd73705f9
 */
#ifndef WORLD_MAP_CALLBACK_92234_H
#define WORLD_MAP_CALLBACK_92234_H

#include "common.h"

/* Test-only exact guest-access observer. Ordinary production builds do not
 * define WM_92234_TEST_TRACE and therefore have no observer dependency. */
#define WM_92234_TRACE_LW 1u
#define WM_92234_TRACE_SH 2u
#define WM_92234_TRACE_SW 3u

#if defined(WM_92234_TEST_TRACE)
void wm_92234_test_trace(u32 pc, u32 kind, u32 address,
                         u32 width, u32 value);
#endif

s32 wm_80092234(s32 slot_index);

#endif /* WORLD_MAP_CALLBACK_92234_H */
