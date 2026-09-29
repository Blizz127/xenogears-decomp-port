/*
 * Retail world-map typed rectangle lookup 0x80094364.
 *
 * Transcribed from disc/world_map.bin [0x80094364,0x80094434).  The helper
 * searches one 16-byte rectangle list, publishes the matching guest record
 * to D7D8, and otherwise leaves all state untouched.
 */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_94364.h"

#define WM_94364_TABLE_PTR 0x8009BD00u
#define WM_94364_RECORD_PTR 0x8009D7D8u

static u32 wm_94364_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static s16 wm_94364_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void wm_94364_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

s32 wm_80094364(u32 pos_vec, u32 list_index, s32 requested_type)
{
    u32 x_raw = wm_94364_lw(pos_vec + 0u) >> 12;
    u32 table = wm_94364_lw(WM_94364_TABLE_PTR);
    u32 list;
    u32 z_raw;
    s32 x;
    s32 z;
    u32 record;

#if defined(W34N26_MUTANT_IGNORE_TYPE)
    (void)requested_type;
#endif

#if defined(W34N26_MUTANT_WRONG_LIST_SCALE)
    list = wm_94364_lw(table + list_index * 8u);
#else
    list = wm_94364_lw(table + list_index * 4u);
#endif
    z_raw = wm_94364_lw(pos_vec + 8u) >> 12;
    if ((s32)wm_94364_lh(list + 8u) == -1)
        return 0;

#if defined(W34N26_MUTANT_MISSING_COORD_MASK)
    x = (s32)x_raw;
    z = (s32)z_raw;
#else
    x = (s32)(x_raw & 0xFFFFu);
    z = (s32)(z_raw & 0xFFFFu);
#endif
    record = list;

    for (;;) {
        s32 x0 = (s32)wm_94364_lh(record + 0u);
        s32 z0 = (s32)wm_94364_lh(record + 2u);
        s32 width = (s32)wm_94364_lh(record + 4u);
        s32 depth = (s32)wm_94364_lh(record + 6u);
        int inside;

#if defined(W34N26_MUTANT_EXCLUSIVE_BOUND)
        inside = x > x0 && x <= x0 + width &&
                 z >= z0 && z <= z0 + depth;
#else
        inside = x >= x0 && x <= x0 + width &&
                 z >= z0 && z <= z0 + depth;
#endif
        if (inside) {
            s32 record_type;
#if defined(W34N26_MUTANT_UNSIGNED_TYPE)
            record_type = (s32)(u16)wm_94364_lh(record + 0xEu);
#else
            record_type = (s32)wm_94364_lh(record + 0xEu);
#endif
#if defined(W34N26_MUTANT_IGNORE_TYPE)
            (void)record_type;
#else
            if (record_type != requested_type)
                goto next_record;
#endif
            wm_94364_sw(WM_94364_RECORD_PTR, record);
            return 1;
        }

#if !defined(W34N26_MUTANT_IGNORE_TYPE)
next_record:
#endif
#if defined(W34N26_MUTANT_WRONG_STRIDE)
        record += 0x20u;
#else
        record += 0x10u;
#endif
        if ((s32)wm_94364_lh(record + 8u) == -1)
            break;
    }

#if defined(W34N26_MUTANT_PUBLISH_ON_MISS)
    wm_94364_sw(WM_94364_RECORD_PTR, 0xFFFFFFFFu);
#endif
    return 0;
}
