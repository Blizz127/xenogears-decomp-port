/*
 * World-map scheduler callback 0x80092BE4.
 *
 * Retail boundary: [0x80092BE4, 0x80092C70), 0x8C bytes / 35
 * instructions. Slice SHA-256:
 * 924dea8b89908efcdbe30a98381cc889f5ddf59984591c9217f80a11dbe5b194
 */
#ifndef WORLD_MAP_CALLBACK_92BE4_H
#define WORLD_MAP_CALLBACK_92BE4_H

#include "common.h"

/* Test-only typed guest-access observer. Ordinary production builds do not
 * define WM_92BE4_TEST_TRACE and therefore have no observer dependency. */
#define WM_92BE4_TRACE_LHU 1u
#define WM_92BE4_TRACE_SB  2u
#define WM_92BE4_TRACE_SH  3u

#if defined(WM_92BE4_TEST_TRACE)
void wm_92be4_test_trace(u32 pc, u32 kind, u32 address,
                         u32 width, u32 value);
#endif

s32 wm_80092BE4(s32 slot_index);

#endif /* WORLD_MAP_CALLBACK_92BE4_H */
