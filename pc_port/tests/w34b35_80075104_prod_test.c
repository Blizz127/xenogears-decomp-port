/* W34B35 production-linked certificate for retail 0x80075104. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_upload_pump_75104.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

#define COUNT  0x8009CD64u
#define ARRAY  0x800A0000u
#define RECT0  0x800A1000u
#define RECT1  0x800A1100u
#define TABLE  0x800A2000u
#define SOURCE 0x800A4000u

typedef struct {
    s16 x;
    s16 y;
    s16 w;
    s16 h;
} TestRect;
typedef unsigned long TestULong;

static int s_pass;
static int s_fail;
static int s_total;
static int s_load_calls;
static TestRect *s_last_rect;
static TestULong *s_last_data;

static void check_case(const char *name, const char *assertion, int ok)
{
    s_total++;
    if (ok) {
        s_pass++;
    } else {
        s_fail++;
        printf("FAIL [%s]: ASSERTION %s\n", name, assertion);
    }
}

static void store_u32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void store_u16(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u16 load_u16(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void set_table_entry(u32 table, s32 index, s16 factor, u16 timer)
{
    u32 entry = table + (u32)index * 4u;
    store_u16(entry, (u16)factor);
    store_u16(entry + 2u, timer);
}

static void set_rect(u32 rect, s16 height, s16 width)
{
    store_u16(rect + 4u, (u16)height);
    store_u16(rect + 6u, (u16)width);
    store_u32(rect + 0xCu, TABLE);
}

static void set_record(s32 index, u32 source, u32 rect, u16 frame_index,
                       u16 timer)
{
    u32 record = ARRAY + (u32)index * 0x0Cu;
    store_u32(record + 0u, source);
    store_u32(record + 4u, rect);
    store_u16(record + 8u, frame_index);
    store_u16(record + 0xAu, timer);
}

static void reset_fixture(void)
{
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0, sizeof(g_PsxScratchpad));
    s_load_calls = 0;
    s_last_rect = NULL;
    s_last_data = NULL;
    store_u32(ARRAY, 0u);
    set_rect(RECT0, 5, 3);
    set_rect(RECT1, 7, 4);
    wm_75104_reset();
}

static void run_count_and_timer(void)
{
    reset_fixture();
    store_u32(COUNT, 0u);
    store_u32(0x8009D7D0u, 0x12345678u);
    check_case("count-zero", "signed-count-no-op",
               wm_80075104() == 0 && wm_75104_get_unknowns() == 0 &&
               s_load_calls == 0);

    reset_fixture();
    store_u32(COUNT, 0xFFFFFFFFu);
    store_u32(0x8009D7D0u, 0x12345678u);
    check_case("count-negative", "blez-signed-count-no-op",
               wm_80075104() == 0 && wm_75104_get_unknowns() == 0 &&
               s_load_calls == 0);

    reset_fixture();
    store_u32(COUNT, 1u);
    store_u32(0x8009D7D0u, ARRAY);
    set_record(0, SOURCE, RECT0, 0u, 2u);
    set_table_entry(TABLE, 0, 2, 7u);
    check_case("timer-nonzero", "decrement-without-transfer",
               wm_80075104() == 0 && load_u16(ARRAY + 0xAu) == 1u &&
               load_u16(ARRAY + 8u) == 0u && s_load_calls == 0);
}

static void run_scaled_transfer(void)
{
    reset_fixture();
    store_u32(COUNT, 1u);
    store_u32(0x8009D7D0u, ARRAY);
    set_record(0, SOURCE, RECT0, 0u, 1u);
    set_table_entry(TABLE, 1, 2, 5u);
    check_case("scaled-transfer", "width-height-factor-times-two",
               wm_80075104() == 0 && load_u16(ARRAY + 0xAu) == 5u &&
               load_u16(ARRAY + 8u) == 1u && s_load_calls == 1 &&
               s_last_rect == (TestRect *)PSX_ADDR(RECT0) &&
               s_last_data == (TestULong *)PSX_ADDR(SOURCE + 60u));
}

static void run_stride_and_signed_index(void)
{
    reset_fixture();
    store_u32(COUNT, 2u);
    store_u32(0x8009D7D0u, ARRAY);
    set_record(0, SOURCE, RECT0, 0u, 2u);
    set_record(1, SOURCE + 0x100u, RECT1, 0u, 1u);
    set_table_entry(TABLE, 1, 2, 6u);
    check_case("record-stride", "two-asymmetric-12-byte-records",
               wm_80075104() == 0 && s_load_calls == 1 &&
               load_u16(ARRAY + 0x0Au) == 1u &&
               load_u16(ARRAY + 0x0Cu + 0xAu) == 6u &&
               s_last_rect == (TestRect *)PSX_ADDR(RECT1) &&
               s_last_data == (TestULong *)PSX_ADDR(SOURCE + 0x100u + 112u));

    reset_fixture();
    store_u32(COUNT, 1u);
    store_u32(0x8009D7D0u, ARRAY);
    set_record(0, SOURCE, RECT0, 0x7FFFu, 1u);
    set_table_entry(TABLE, 0, 2, 7u);
    set_table_entry(TABLE - 0x20000u, 0, 0, 0xFFFFu);
    check_case("signed-index-wrap", "s16-index-negative-table-address",
               wm_80075104() == 0 && load_u16(ARRAY + 8u) == 0u &&
               load_u16(ARRAY + 0xAu) == 7u && s_load_calls == 1 &&
               s_last_data == (TestULong *)PSX_ADDR(SOURCE + 60u));
}

static void run_unknown_pointer(void)
{
    reset_fixture();
    store_u32(COUNT, 1u);
    store_u32(0x8009D7D0u, ARRAY);
    set_record(0, 0x12345678u, RECT0, 0u, 1u);
    set_table_entry(TABLE, 1, 2, 5u);
    check_case("unknown-source", "log-count-and-skip-load",
               wm_80075104() == 0 && wm_75104_get_unknowns() == 1 &&
               s_load_calls == 0 && load_u16(ARRAY + 8u) == 1u &&
               load_u16(ARRAY + 0xAu) == 5u);
}

int LoadImage(TestRect *rect, TestULong *data)
{
    s_load_calls++;
    s_last_rect = rect;
    s_last_data = data;
    return 0;
}

int main(void)
{
    printf("=== W34B35 0x80075104 upload pump ===\n");
    printf("RETAIL_BOUNDARY [0x80075104,0x80075228) bytes=292 insns=73\n");
    run_count_and_timer();
    run_scaled_transfer();
    run_stride_and_signed_index();
    run_unknown_pointer();
    printf("=== Results: %d/%d PASS ===\n", s_pass, s_total);
    return s_fail == 0 ? 0 : 1;
}
