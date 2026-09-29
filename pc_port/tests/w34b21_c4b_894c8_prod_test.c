/* Focused production-linked oracle for retail world helper 0x800894C8. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_common_tail.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
u8 D_80059179; /* field scene flag read by the linked helper (BSS-zero) */

void *HeapAlloc(u_int allocSize, u_int allocFlags)
{
    (void)allocSize;
    (void)allocFlags;
    return NULL;
}

u_short GetTPage(int tp, int abr, int x, int y)
{
    (void)tp;
    (void)abr;
    (void)x;
    (void)y;
    return 0;
}

u_short GetClut(int x, int y)
{
    (void)x;
    (void)y;
    return 0;
}

void SystemTransferPaletteToVRAM(short xDest, short yDest)
{
    (void)xDest;
    (void)yDest;
}

enum {
    TRACE_LBU = 1,
    TRACE_LW  = 3,
    TRACE_SB  = 4
};

typedef struct TraceEvent {
    u32 pc;
    u32 kind;
    u32 addr;
    u32 width;
    u32 value;
} TraceEvent;

#define TRACE_CAPACITY 64u

static TraceEvent g_trace[TRACE_CAPACITY];
static size_t g_trace_count;
static int g_failures;

void wm_89160_test_trace(u32 pc, u32 kind, u32 address, u32 width, u32 value)
{
    if (g_trace_count < TRACE_CAPACITY) {
        TraceEvent *event = &g_trace[g_trace_count];
        event->pc = pc;
        event->kind = kind;
        event->addr = address;
        event->width = width;
        event->value = value;
    }
    g_trace_count++;
}

#define TABLE_PTR_ADDR  WM_89160_TABLE_BASE_PTR
#define TABLE_BASE      0x80040000u
#define RECORD_STRIDE   WM_89160_RECORD_STRIDE
#define SUB_STRIDE      WM_89160_SUBRECORD_STRIDE
#define SUB_COUNT       WM_89160_SUBRECORD_COUNT
#define FLAG_OFF        WM_89160_FLAG_BYTE_OFFSET
#define FLAG_BIT        WM_89160_FLAG_BIT
#define RECORD_COUNT    66u
#define TABLE_BYTES     (RECORD_COUNT * RECORD_STRIDE)
#define PAINT_BYTE      0xA5u
#define LIVE_FLAG       0xC3u
#define CLEARED_FLAG    0x43u

static void fail(const char *name, unsigned long long got,
                 unsigned long long expected)
{
    fprintf(stderr, "ASSERTION %s: got=%llu expected=%llu\n",
            name, got, expected);
    g_failures++;
}

static void check_u64(const char *name, unsigned long long got,
                      unsigned long long expected)
{
    if (got != expected)
        fail(name, got, expected);
}

static u8 load_u8(u32 address)
{
    u8 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void store_u8(u32 address, u8 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void store_u32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u32 record_addr(u32 index)
{
    return TABLE_BASE + index * RECORD_STRIDE;
}

static u32 flag_addr(u32 index, u32 sub)
{
    return record_addr(index) + sub * SUB_STRIDE + FLAG_OFF;
}

static void paint_table(void)
{
    u32 index;
    u32 sub;

    memset(PSX_ADDR(TABLE_BASE - 16u), PAINT_BYTE, TABLE_BYTES + 32u);
    store_u32(TABLE_PTR_ADDR, TABLE_BASE);
    for (index = 0; index < RECORD_COUNT; index++) {
        for (sub = 0; sub < SUB_COUNT; sub++)
            store_u8(flag_addr(index, sub), LIVE_FLAG);
    }
}

static void expect_record_cleared(u32 index)
{
    u32 sub;
    char name[64];

    for (sub = 0; sub < SUB_COUNT; sub++) {
        snprintf(name, sizeof(name), "record %u sub %u flag", index, sub);
        check_u64(name, load_u8(flag_addr(index, sub)), CLEARED_FLAG);
        snprintf(name, sizeof(name), "record %u sub %u before", index, sub);
        check_u64(name, load_u8(flag_addr(index, sub) - 1u), PAINT_BYTE);
        snprintf(name, sizeof(name), "record %u sub %u after", index, sub);
        check_u64(name, load_u8(flag_addr(index, sub) + 1u), PAINT_BYTE);
    }
}

static void expect_record_untouched(u32 index)
{
    u32 sub;
    char name[64];

    for (sub = 0; sub < SUB_COUNT; sub++) {
        snprintf(name, sizeof(name), "neighbor record %u sub %u", index, sub);
        check_u64(name, load_u8(flag_addr(index, sub)), LIVE_FLAG);
    }
}

static void check_containment(u32 index, const u8 *before)
{
    u32 offset;
    u8 *after = (u8 *)PSX_ADDR(TABLE_BASE);

    for (offset = 0; offset < TABLE_BYTES; offset++) {
        int expected_write = 0;
        u32 sub;

        for (sub = 0; sub < SUB_COUNT; sub++) {
            if (offset == index * RECORD_STRIDE + sub * SUB_STRIDE + FLAG_OFF) {
                expected_write = 1;
                break;
            }
        }
        if (expected_write)
            check_u64("containment write", after[offset], CLEARED_FLAG);
        else if (after[offset] != before[offset])
            fail("containment stray write", offset, before[offset]);
    }
}

static void test_index(u32 index)
{
    u8 before[TABLE_BYTES];

    paint_table();
    memcpy(before, PSX_ADDR(TABLE_BASE), TABLE_BYTES);
    g_trace_count = 0;
    wm_800894C8(index);
    expect_record_cleared(index);
    if (index > 0u)
        expect_record_untouched(index - 1u);
    if (index + 1u < RECORD_COUNT)
        expect_record_untouched(index + 1u);
    check_containment(index, before);
    check_u64("pre-table canary", load_u8(TABLE_BASE - 1u), PAINT_BYTE);
    check_u64("post-table canary",
              load_u8(TABLE_BASE + TABLE_BYTES), PAINT_BYTE);
}

static void test_already_clear_and_full_bits(void)
{
    paint_table();
    store_u8(flag_addr(2u, 0u), 0x7Fu);
    store_u8(flag_addr(2u, 1u), 0xFFu);
    wm_800894C8(2u);
    check_u64("already-clear stays 0x7F", load_u8(flag_addr(2u, 0u)), 0x7Fu);
    check_u64("0xFF becomes 0x7F", load_u8(flag_addr(2u, 1u)), 0x7Fu);
    check_u64("other live bit preserved", load_u8(flag_addr(2u, 2u)),
              CLEARED_FLAG);
}

static void test_access_trace(void)
{
    const u32 index = 0x2Fu;
    size_t i;

    paint_table();
    g_trace_count = 0;
    wm_800894C8(index);

    check_u64("trace count", g_trace_count, 1u + 2u * (u32)SUB_COUNT);
    check_u64("lw pc", g_trace[0].pc, 0x800894E4u);
    check_u64("lw kind", g_trace[0].kind, TRACE_LW);
    check_u64("lw addr", g_trace[0].addr, TABLE_PTR_ADDR);
    check_u64("lw width", g_trace[0].width, 4u);
    check_u64("lw value", g_trace[0].value, TABLE_BASE);

    for (i = 0; i < SUB_COUNT; i++) {
        const TraceEvent *load = &g_trace[1u + i * 2u];
        const TraceEvent *store = &g_trace[2u + i * 2u];
        const u32 addr = flag_addr(index, (u32)i);

        check_u64("lbu pc", load->pc, 0x800894F4u);
        check_u64("lbu kind", load->kind, TRACE_LBU);
        check_u64("lbu addr", load->addr, addr);
        check_u64("lbu width", load->width, 1u);
        check_u64("lbu value", load->value, LIVE_FLAG);
        check_u64("sb pc", store->pc, 0x80089500u);
        check_u64("sb kind", store->kind, TRACE_SB);
        check_u64("sb addr", store->addr, addr);
        check_u64("sb width", store->width, 1u);
        check_u64("sb value", store->value, CLEARED_FLAG);
    }
}

int main(void)
{
    memset(g_PsxRam, 0, sizeof(g_PsxRam));

    test_index(0u);
    test_index(1u);
    test_index(0x2Fu);
    test_index(0x3Fu);
    test_index(0x40u);
    test_already_clear_and_full_bits();
    test_access_trace();

    if (g_failures != 0) {
        fprintf(stderr, "W34B21-C4B FAIL assertions=%d\n", g_failures);
        return 1;
    }
    printf("W34B21-C4B 0x800894C8 focused oracle PASS\n");
    return 0;
}
