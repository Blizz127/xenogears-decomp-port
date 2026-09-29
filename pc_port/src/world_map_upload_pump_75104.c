/*
 * W34B35 — native retail 0x80075104 upload pump.
 *
 * Fresh retail decode: [0x80075104, 0x80075228), 73 instructions / 292
 * bytes, disc/world_map.bin load base 0x8006FAF0.
 *
 * This sibling shares the 12-byte record timer/index state machine with
 * 0x80074F2C, but its transfer offset is distinct:
 *   signed_width = descriptor[+6], signed_height = descriptor[+4]
 *   signed_factor = table[index], table = descriptor[+0x0C]
 *   data = record[+0] + (width * height * (factor * 2))
 *
 * Known PSX/KSEG1 pointers are mapped before host LoadImage. Unknown record,
 * descriptor, table, or source values are logged/counting and that transfer
 * is skipped; no raw guest pointer reaches a host call.
 */
#include <stdio.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_upload_pump_75104.h"

typedef struct {
    s16 x;
    s16 y;
    s16 w;
    s16 h;
} Wm751Rect;
typedef unsigned long Wm751ULong;

extern int LoadImage(Wm751Rect *rect, Wm751ULong *data);

#define WM751_COUNT  0x8009CD64u
#define WM751_ARRAY  0x8009D7D0u
#define WM751_STRIDE 0x0Cu

static int s_wm751_unknowns;
static int s_wm751_transfers;

void wm_75104_reset(void)
{
    s_wm751_unknowns = 0;
    s_wm751_transfers = 0;
}

int wm_75104_get_unknowns(void)
{
    return s_wm751_unknowns;
}

int wm_75104_get_transfers(void)
{
    return s_wm751_transfers;
}

static int wm751_guest_ptr_known(u32 value)
{
#if defined(WM_75104_MUTANT_UNKNOWN_GUARD)
    (void)value;
    return 1;
#else
    return ((value & 0xFFE00000u) == 0x80000000u) ||
           ((value & 0xFFE00000u) == 0xA0000000u);
#endif
}

static void wm751_unknown(const char *kind, u32 value, u32 pc)
{
    s_wm751_unknowns++;
    fprintf(stderr,
            "[worldmap-upload-pump-b] unknown %s=0x%08x at 0x%08x "
            "count=%d\n",
            kind, value, pc, s_wm751_unknowns);
}

static u16 wm751_load_u16(u32 address)
{
    return *(volatile u16 *)PSX_ADDR(address);
}

static s16 wm751_load_s16(u32 address)
{
    return *(volatile s16 *)PSX_ADDR(address);
}

static void wm751_store_u16(u32 address, u16 value)
{
    *(volatile u16 *)PSX_ADDR(address) = value;
}

static u32 wm751_load_u32(u32 address)
{
    return *(volatile u32 *)PSX_ADDR(address);
}

static int wm751_transfer(u32 record, u16 index)
{
    u32 rect = wm751_load_u32(record + 4u);
    u32 source = wm751_load_u32(record);
    u32 table;
    u32 table_entry;
    s16 factor;
    s16 width;
    s16 height;
    u32 area;
    u32 scaled_area;
    u32 data;

    /* 0x800751A4: descriptor; 0x800751DC: record source. */
    if (!wm751_guest_ptr_known(rect)) {
        wm751_unknown("rect", rect, 0x800751A4u);
        return 0;
    }
    width = wm751_load_s16(rect + 6u);
    height = wm751_load_s16(rect + 4u);
    table = wm751_load_u32(rect + 0x0Cu);
    if (!wm751_guest_ptr_known(table)) {
        wm751_unknown("table", table, 0x800751C0u);
        return 0;
    }
#if defined(WM_75104_MUTANT_INDEX_UNSIGNED)
    table_entry = table + ((u32)index * 4u);
#else
    table_entry = table + ((u32)(s32)(s16)index * 4u);
#endif
    if (!wm751_guest_ptr_known(table_entry)) {
        wm751_unknown("table_entry", table_entry, 0x800751C8u);
        return 0;
    }
    factor = wm751_load_s16(table_entry);
    area = (u32)(s32)width * (u32)(s32)height;
#if defined(WM_75104_MUTANT_DIM_SCALE)
    scaled_area = (u32)(s32)factor * area;
#else
    scaled_area = (u32)(s32)factor * (area << 1);
#endif
    data = source + scaled_area;
    if (!wm751_guest_ptr_known(source)) {
        wm751_unknown("source", source, 0x800751DCu);
        return 0;
    }
    if (!wm751_guest_ptr_known(data)) {
        wm751_unknown("data", data, 0x800751E8u);
        return 0;
    }

    (void)LoadImage((Wm751Rect *)PSX_ADDR(rect),
                    (Wm751ULong *)PSX_ADDR(data));
    s_wm751_transfers++;
    return 1;
}

int wm_80075104(void)
{
    s32 count;
    u32 records;
    s32 i;

    /* 0x80075104..0x80075128: signed count gate and record root load. */
    count = (s32)wm751_load_u32(WM751_COUNT);
    records = wm751_load_u32(WM751_ARRAY);
    fprintf(stderr,
            "[worldmap-upload-pump-b] entry count=%d array=0x%08x\n",
            count, records);
    if (count <= 0)
        return 0;
    if (!wm751_guest_ptr_known(records)) {
        wm751_unknown("records", records, 0x80075118u);
        return -1;
    }

    for (i = 0; i < count; i++) {
        u32 record;
        u16 timer;
        u16 next_timer;
        u16 index;

#if defined(WM_75104_MUTANT_STRIDE)
        record = records + (u32)i * 0x10u;
#else
        record = records + (u32)i * WM751_STRIDE;
#endif
        if (!wm751_guest_ptr_known(record)) {
            wm751_unknown("record", record, 0x80075130u);
            continue;
        }
        /* 0x80075134..0x80075140: u16 timer decrement with wrap. */
        timer = wm751_load_u16(record + 0x0Au);
        next_timer = (u16)(timer - 1u);
        wm751_store_u16(record + 0x0Au, next_timer);
#if defined(WM_75104_MUTANT_TRIGGER)
        if (next_timer != 1u)
#else
        if (next_timer != 0u)
#endif
            continue;

        /* 0x80075150..0x800751A0: advance index, refresh timer, and reset
         * negative refreshed timers back to table index zero. */
        index = wm751_load_u16(record + 8u);
        index = (u16)(index + 1u);
        wm751_store_u16(record + 8u, index);
        {
            u32 descriptor = wm751_load_u32(record + 4u);
            u32 table;
            u32 timer_entry;
            u16 refreshed;

            if (!wm751_guest_ptr_known(descriptor)) {
                wm751_unknown("rect", descriptor, 0x80075154u);
                continue;
            }
            table = wm751_load_u32(descriptor + 0x0Cu);
            if (!wm751_guest_ptr_known(table)) {
                wm751_unknown("table", table, 0x80075168u);
                continue;
            }
#if defined(WM_75104_MUTANT_INDEX_UNSIGNED)
            timer_entry = table + ((u32)index * 4u) + 2u;
#else
            timer_entry = table + ((u32)(s32)(s16)index * 4u) + 2u;
#endif
            if (!wm751_guest_ptr_known(timer_entry)) {
                wm751_unknown("timer_entry", timer_entry, 0x8007516Cu);
                continue;
            }
            refreshed = wm751_load_u16(timer_entry);
            wm751_store_u16(record + 0x0Au, refreshed);
            if ((s16)refreshed < 0) {
                u32 zero_timer_entry = table + 2u;
                wm751_store_u16(record + 8u, 0u);
                if (!wm751_guest_ptr_known(zero_timer_entry)) {
                    wm751_unknown("timer_entry", zero_timer_entry,
                                 0x80075190u);
                    continue;
                }
                wm751_store_u16(record + 0x0Au,
                               wm751_load_u16(zero_timer_entry));
            }
        }

        index = wm751_load_u16(record + 8u);
        (void)wm751_transfer(record, index);
    }

    fprintf(stderr,
            "[worldmap-upload-pump-b] exit transfers=%d unknowns=%d\n",
            s_wm751_transfers, s_wm751_unknowns);
    return 0;
}
