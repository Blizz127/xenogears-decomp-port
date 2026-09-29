/*
 * World-map scheduler callback 0x80092DF8.
 *
 * Retail boundary: [0x80092DF8, 0x80092FD8), 0x1E0 bytes / 120
 * instructions. Slice SHA-256:
 * 14691ec31d58fa2c395047e4020be6f72fda580334ed7ca03827c26b31ebf7a5
 */
#ifndef WORLD_MAP_CALLBACK_92DF8_H
#define WORLD_MAP_CALLBACK_92DF8_H

#include "common.h"

/* Test-only typed guest-access observer. Ordinary production builds do not
 * define WM_92DF8_TEST_TRACE and therefore have no observer dependency. */
#define WM_92DF8_TRACE_LHU 1u
#define WM_92DF8_TRACE_SB  2u
#define WM_92DF8_TRACE_SH  3u
#define WM_92DF8_TRACE_LW  4u
#define WM_92DF8_TRACE_SW  5u

#if defined(WM_92DF8_TEST_TRACE)
void wm_92df8_test_trace(u32 pc, u32 kind, u32 address,
                         u32 width, u32 value);
#endif

s32 wm_80092DF8(s32 slot_index);

#endif /* WORLD_MAP_CALLBACK_92DF8_H */
