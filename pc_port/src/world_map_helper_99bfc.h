/*
 * World-map helper 0x80099BFC (wrapped-entry FT4 submitter).
 *
 * Retail boundary: [0x80099BFC, 0x80099E88), 163 instructions / 652 bytes.
 * Wraps camera-relative X/Z, projects the shared four-vertex template,
 * culls it, selects its depth-cue CLUT, and compacts accepted FT4 packets.
 */
#ifndef WORLD_MAP_HELPER_99BFC_H
#define WORLD_MAP_HELPER_99BFC_H

#include "common.h"

void wm_80099BFC(u32 entries, s32 entry_count, u32 ot_base, u32 packet);

#endif
