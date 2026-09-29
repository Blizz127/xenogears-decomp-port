/* W34N36 — production-linked world UI guest-domain certificate.
 * The scheduler callbacks receive guest KSEG table/OT values.  Compiled
 * system helpers require host pointers, while the old generated string stub
 * happened to accept an integer and masked the mismatch until it executed. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_92c70.h"
#include "world_map_callback_92fd8.h"

#define POOL          0x800D0000u
#define POOL_GLOBAL   0x8009BE24u
#define AREA          0x8009BD24u
#define CE68          0x8009CE68u
#define TABLE_GLOBAL  0x8009D784u
#define TABLE         0x80120000u
#define SOURCE_GLOBAL 0x8009BE3Cu
#define SOURCE        0x800F0000u
#define OT             0x80121000u
#define RENDER_CONTEXT 0x8009D7F0u
#define UI_C70         0x8009D498u
#define UI_FD8         0x8009BD64u

static int s_pass;
static int s_total;
static int s_asserted;
static int s_clear_calls;
static int s_lookup_calls;
static int s_assign_calls;
static int s_render_calls;
static void *s_lookup_table;
static s32 s_lookup_index;
static void *s_assign_object;
static void *s_assign_entry;
static void *s_render_object;
static void *s_render_ot;
static s32 s_render_context;
static u8 s_entry_tokens[64];

static void st16(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void st32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u16 ld16(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 ld32(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void check(const char *name, int ok)
{
    s_total++;
    if (ok) {
        s_pass++;
        printf("PASS %s\n", name);
    } else {
        fprintf(stderr, "ASSERTION %s\n", name);
        s_asserted = 1;
    }
}

void func_80034614(void *object)
{
    (void)object;
    s_clear_calls++;
}

void *GetStringEntry(void *table, s32 index)
{
    s_lookup_calls++;
    s_lookup_table = table;
    s_lookup_index = index;
    return &s_entry_tokens[(u32)index & 63u];
}

void func_80034714(void *object, void *entry)
{
    s_assign_calls++;
    s_assign_object = object;
    s_assign_entry = entry;
}

void func_80034888(void *object, void *ot, s32 render_context)
{
    s_render_calls++;
    s_render_object = object;
    s_render_ot = ot;
    s_render_context = render_context;
}

static void reset_trace(void)
{
    s_clear_calls = 0;
    s_lookup_calls = 0;
    s_assign_calls = 0;
    s_render_calls = 0;
    s_lookup_table = NULL;
    s_lookup_index = 0;
    s_assign_object = NULL;
    s_assign_entry = NULL;
    s_render_object = NULL;
    s_render_ot = NULL;
    s_render_context = 0;
}

static void reset_fixture(void)
{
    memset(g_PsxRam, 0, PSX_RAM_SIZE);
    memset(g_PsxScratchpad, 0, 4096u);
    memset(s_entry_tokens, 0xA5, sizeof(s_entry_tokens));
    st32(POOL_GLOBAL, POOL);
    st32(TABLE_GLOBAL, TABLE);
    st32(SOURCE_GLOBAL, SOURCE);
    st32(SOURCE + 0x70u, OT);
    st32(RENDER_CONTEXT, 2u);
    reset_trace();
}

static void test_c70_init(void)
{
    u32 slot = POOL + 12u * 0x80u;
    void *expected_entry;

    reset_fixture();
    st16(AREA, 3u);
    expected_entry = &s_entry_tokens[3];
    check("c70-return", wm_80092C70(12) == 1);
    check("c70-compiled-string-lookup-called", s_lookup_calls == 1);
    check("c70-string-table-rebased-from-guest",
          s_lookup_table == PSX_ADDR(TABLE) && s_lookup_index == 3);
    check("c70-string-entry-published",
          s_assign_calls == 1 && s_assign_object == PSX_ADDR(UI_C70) &&
          s_assign_entry == expected_entry);
    check("c70-guest-ot-rebased-for-system-renderer",
          s_render_calls == 1 && s_render_object == PSX_ADDR(UI_C70) &&
          s_render_ot == PSX_ADDR(OT) && s_render_context == 2);
    check("c70-state-transition",
          ld16(slot + 0x20u) == 1u && ld32(slot + 0x50u) == 3u &&
          s_clear_calls == 1);
}

static void test_fd8_init_and_blend(void)
{
    u32 slot = POOL + 13u * 0x80u;
    u32 table_word = 0x8009D2B8u + 2u * 40u;
    void *expected_entry;

    reset_fixture();
    st16(CE68, 4u);
    st32(table_word, 0xAA102030u);
    st32(OT, 0xBB405060u);
    expected_entry = &s_entry_tokens[4];
    check("fd8-return", wm_80092FD8(13) == 1);
    check("fd8-compiled-string-lookup-called", s_lookup_calls == 1);
    check("fd8-string-table-rebased-from-guest",
          s_lookup_table == PSX_ADDR(TABLE) && s_lookup_index == 4);
    check("fd8-string-entry-published",
          s_assign_calls == 1 && s_assign_object == PSX_ADDR(UI_FD8) &&
          s_assign_entry == expected_entry);
    check("fd8-guest-ot-rebased-for-system-renderer",
          s_render_calls == 1 && s_render_object == PSX_ADDR(UI_FD8) &&
          s_render_ot == PSX_ADDR(OT) && s_render_context == 2);
    check("fd8-state-transition-and-blend",
          ld16(slot + 0x20u) == 1u && ld32(slot + 0x50u) == 4u &&
          ld32(table_word) == 0xAA405060u &&
          ld32(OT) == (0xBB000000u | (table_word & 0x00FFFFFFu)) &&
          s_clear_calls == 1);
}

int main(void)
{
    test_c70_init();
    test_fd8_init_and_blend();
    if (s_asserted != 0 || s_pass != s_total)
        return EXIT_FAILURE;
    printf("W34N36 world UI guest-domain certificate PASS\n");
    return EXIT_SUCCESS;
}
