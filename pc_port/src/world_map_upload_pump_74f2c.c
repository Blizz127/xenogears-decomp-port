/*
 * W34B34 — native retail 0x80074F2C upload pump.
 *
 * Fresh retail decode: [0x80074F2C, 0x8007502C), 64 instructions / 256
 * bytes, disc/world_map.bin load base 0x8006FAF0.
 *
 * Retail state:
 *   CC9C = signed record count, D780 = guest record-array pointer
 *   record stride 0x0C: +0 source pointer, +4 RECT/descriptor pointer,
 *   +8 signed frame index (stored as halfword), +A upload timer (halfword)
 *   descriptor +0x0C = guest halfword table pointer
 *   0x80074FEC -> LoadImage(rect, source + (table[index] << 4))
 *
 * Guest pointers are mapped only for known PSX/KSEG1 addresses. Unknown
 * values are logged and counted, and only that transfer is skipped.
 */
#include <stdio.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_upload_pump_74f2c.h"

typedef struct {
    s16 x;
    s16 y;
    s16 w;
    s16 h;
} Wm74Rect;
typedef unsigned long Wm74ULong;

extern int LoadImage(Wm74Rect *rect, Wm74ULong *data);

#define WM74_COUNT  0x8009CC9Cu
#define WM74_ARRAY  0x8009D780u
#define WM74_STRIDE 0x0Cu

static int s_wm74_unknowns;
static int s_wm74_transfers;

void wm_74f2c_reset(void)
{
    s_wm74_unknowns = 0;
    s_wm74_transfers = 0;
}

int wm_74f2c_get_unknowns(void)
{
    return s_wm74_unknowns;
}

int wm_74f2c_get_transfers(void)
{
    return s_wm74_transfers;
}

static int wm74_guest_ptr_known(u32 value)
{
#if defined(WM_74F2C_MUTANT_UNKNOWN_GUARD)
    (void)value;
    return 1;
#else
    return ((value & 0xFFE00000u) == 0x80000000u) ||
           ((value & 0xFFE00000u) == 0xA0000000u);
#endif
}

static void wm74_unknown(const char *kind, u32 value, u32 pc)
{
    s_wm74_unknowns++;
    fprintf(stderr,
            "[worldmap-upload-pump] unknown %s=0x%08x at 0x%08x "
            "count=%d\n",
            kind, value, pc, s_wm74_unknowns);
}

static u16 wm74_load_u16(u32 address)
{
    return *(volatile u16 *)PSX_ADDR(address);
}

static s16 wm74_load_s16(u32 address)
{
    return *(volatile s16 *)PSX_ADDR(address);
}

static void wm74_store_u16(u32 address, u16 value)
{
    *(volatile u16 *)PSX_ADDR(address) = value;
}

static u32 wm74_load_u32(u32 address)
{
    return *(volatile u32 *)PSX_ADDR(address);
}

static int wm74_transfer(u32 record, u16 index)
{
    u32 rect = wm74_load_u32(record + 4u);
    u32 source = wm74_load_u32(record);
    u32 table;
    u32 table_entry;
    s16 image_offset;
    u32 source_offset;
    u32 data;

    /* 0x80074FCC: a0 = record + 4; 0x80074FE4: source = record + 0. */
    if (!wm74_guest_ptr_known(rect)) {
        wm74_unknown("rect", rect, 0x80074FCCu);
        return 0;
    }
    table = wm74_load_u32(rect + 0x0Cu);
    if (!wm74_guest_ptr_known(table)) {
        wm74_unknown("table", table, 0x80074FD4u);
        return 0;
    }

    /* 0x80074FD0/90: index is sign-extended before the halfword lookup. */
#if defined(WM_74F2C_MUTANT_INDEX_UNSIGNED)
    table_entry = table + ((u32)index * 4u);
#else
    table_entry = table + ((u32)(s32)(s16)index * 4u);
#endif
    if (!wm74_guest_ptr_known(table_entry)) {
        wm74_unknown("table_entry", table_entry, 0x80074FDCu);
        return 0;
    }
    image_offset = wm74_load_s16(table_entry);
    source_offset = (u32)(s32)image_offset << 4;
    data = source + source_offset;

    if (!wm74_guest_ptr_known(source)) {
        wm74_unknown("source", source, 0x80074FE4u);
        return 0;
    }
    if (!wm74_guest_ptr_known(data)) {
        wm74_unknown("data", data, 0x80074FF0u);
        return 0;
    }

    (void)LoadImage((Wm74Rect *)PSX_ADDR(rect),
                    (Wm74ULong *)PSX_ADDR(data));
    s_wm74_transfers++;
    return 1;
}

int wm_80074F2C(void)
{
    s32 count;
    u32 records;
    s32 i;

    /* 0x80074F2C..0x80074F50: count is tested as signed; array is loaded
     * before the gate but is not dereferenced when count <= 0. */
    count = (s32)wm74_load_u32(WM74_COUNT);
    records = wm74_load_u32(WM74_ARRAY);
    fprintf(stderr,
            "[worldmap-upload-pump] entry count=%d array=0x%08x\n",
            count, records);
    if (count <= 0)
        return 0;
    if (!wm74_guest_ptr_known(records)) {
        wm74_unknown("records", records, 0x80074F40u);
        return -1;
    }

    for (i = 0; i < count; i++) {
        u32 record;
        u16 timer;
        u16 next_timer;
        u16 index;

#if defined(WM_74F2C_MUTANT_STRIDE)
        record = records + (u32)i * 0x10u;
#else
        record = records + (u32)i * WM74_STRIDE;
#endif
        if (!wm74_guest_ptr_known(record)) {
            wm74_unknown("record", record, 0x80074F58u);
            continue;
        }

        /* 0x80074F5C..0x80074F68: lhu + addiu -1 + sh, u16 wrap. */
        timer = wm74_load_u16(record + 0x0Au);
        next_timer = (u16)(timer - 1u);
        wm74_store_u16(record + 0x0Au, next_timer);
#if defined(WM_74F2C_MUTANT_TRIGGER)
        if (next_timer != 1u)
#else
        if (next_timer != 0u)
#endif
            continue;

        /* 0x80074F78..0x80074FA0: advance the frame index and refresh the
         * timer from the signed-indexed halfword table. */
        index = wm74_load_u16(record + 8u);
        index = (u16)(index + 1u);
        wm74_store_u16(record + 8u, index);
        {
            u32 descriptor = wm74_load_u32(record + 4u);
            u32 table;
            u32 timer_entry;
            u16 refreshed;

            if (!wm74_guest_ptr_known(descriptor)) {
                wm74_unknown("rect", descriptor, 0x80074F7Cu);
                continue;
            }
            table = wm74_load_u32(descriptor + 0x0Cu);
            if (!wm74_guest_ptr_known(table)) {
                wm74_unknown("table", table, 0x80074F8Cu);
                continue;
            }
#if defined(WM_74F2C_MUTANT_INDEX_UNSIGNED)
            timer_entry = table + ((u32)index * 4u) + 2u;
#else
            timer_entry = table + ((u32)(s32)(s16)index * 4u) + 2u;
#endif
            if (!wm74_guest_ptr_known(timer_entry)) {
                wm74_unknown("timer_entry", timer_entry, 0x80074F94u);
                continue;
            }
            refreshed = wm74_load_u16(timer_entry);
            wm74_store_u16(record + 0x0Au, refreshed);
            if ((s16)refreshed < 0) {
                u32 zero_timer_entry = table + 2u;
                wm74_store_u16(record + 8u, 0u);
                if (!wm74_guest_ptr_known(zero_timer_entry)) {
                    wm74_unknown("timer_entry", zero_timer_entry,
                                 0x80074FBCu);
                    continue;
                }
                wm74_store_u16(record + 0x0Au,
                               wm74_load_u16(zero_timer_entry));
            }
        }

        /* 0x80074FCC..0x80074FF0: submit the selected image. */
        index = wm74_load_u16(record + 8u);
        (void)wm74_transfer(record, index);
    }

    fprintf(stderr,
            "[worldmap-upload-pump] exit transfers=%d unknowns=%d\n",
            s_wm74_transfers, s_wm74_unknowns);
    return 0;
}
