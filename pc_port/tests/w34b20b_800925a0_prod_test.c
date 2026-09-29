/*
 * W34B20-B production-linked certificate for world callback 0x800925A0.
 *
 * The expected PCs, typed accesses, packet fields, OT links, arithmetic, and
 * return states below are independently transcribed from retail world_map.bin
 * [0x800925A0,0x80092BE4).  The subject is linked from the production source;
 * this file contains no duplicate callback implementation.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_925a0.h"
#include "world_map_frame_driver.h"
#include "world_map_scheduler.h"

#define RAM_MASK             0x001FFFFFu
#define TEST_POOL            0x800A8000u
#define TEST_CONTEXT         0x800D5000u
#define TEST_OT              0x800D6000u
#define TEST_WRONG_OT        0x800D6040u
#define TEST_OT0             0x800F0000u
#define TEST_OT1             0x800F1000u

#define RETAIL_POOL_PTR      0x8009BE24u
#define RETAIL_CONTEXT_PTR   0x8009BE3Cu
#define RETAIL_ACTIVE        0x8009C178u
#define RETAIL_TPAGE_ABR     0x8009CCA4u
#define RETAIL_QUADS         0x8009CE6Cu
#define RETAIL_DR_TPAGE      0x8009D310u
#define RETAIL_SPEED         0x8009D3CCu
#define RETAIL_BUFFER_INDEX  0x8009D7F0u

#define RETAIL_SLOT_MODE      0x04u
#define RETAIL_SLOT_FADE_MODE 0x20u
#define RETAIL_SLOT_COUNTER   0x22u
#define RETAIL_SLOT_FADE      0x50u
#define RETAIL_SLOT_STRIDE    0x80u
#define RETAIL_QUAD_STRIDE    0x24u

#define RETAIL_CB0_SLOT0     0x800923A8u
#define RETAIL_CB1_SLOT0     0x800925A0u
#define RETAIL_CB0_SLOT1     0x8008A2C8u
#define RETAIL_CB1_SLOT1     0x8008A72Cu

#define OR_ENVREC0           0x8009BBC8u
#define OR_ENVREC1           0x8009BC40u
#define OR_D554              0x8009D554u
#define OR_OT_OFF            0x70u

typedef struct TraceEvent {
    u32 pc;
    u32 kind;
    u32 address;
    u32 width;
    u32 value;
} TraceEvent;

typedef struct RetailRenderLayout {
    u32 index_pc[12];
    u32 fade_pc[12];
    u32 color_pc[12];
    u32 setup_index_pc;
    u32 setup_context_pc;
    u32 quad_context_pc[2];
    u32 quad_tag_pc;
    u32 quad_ot_pc[2];
    u32 quad_tag_store_pc;
    u32 quad_ot_store_pc;
    u32 dr_context_pc[2];
    u32 dr_tag_pc;
    u32 dr_ot_pc[2];
    u32 dr_tag_store_pc;
    u32 dr_ot_store_pc;
} RetailRenderLayout;

typedef struct Fixture {
    const char* name;
    s32 slot_index;
    s16 mode;
    s16 fade_mode;
    u32 fade;
    u32 speed;
    u16 counter;
    u32 active;
    s32 expected_return;
    u32 expected_fade;
    u16 expected_counter;
    u32 expected_active;
    int initializes;
} Fixture;

typedef struct TestDrTPage {
    u32 tag;
    u32 code[1];
} TestDrTPage;

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

u16 g_C1ButtonState;
u16 g_C2ButtonState;
u16 g_C1ButtonStateReleased;
u16 g_C2ButtonStateReleased;
u16 g_C1ButtonStatePressedOnce;
u16 g_C2ButtonStatePressedOnce;

#define EVENT_CAPACITY 256u
static TraceEvent s_actual[EVENT_CAPACITY];
static TraceEvent s_expected[EVENT_CAPACITY];
static size_t s_actual_count;
static size_t s_expected_count;
static int s_trace_overflow;

static int s_pass;
static int s_total;
static int s_fail;

static int s_get_tpage_calls;
static int s_set_draw_tpage_calls;
static int s_get_tp;
static int s_get_abr;
static int s_get_x;
static int s_get_y;
static int s_set_dfe;
static int s_set_dtd;
static int s_set_tpage;
static u32 s_set_packet;

static int s_controller_calls;
static int s_vsync_calls;
static int s_cdsync_calls;
static int s_clear_ot_calls;
static int s_250e0_calls;
static int s_1d468_calls;
static int s_natural_cb0_calls;

static const u32 s_color_offsets[12] = {
    0x04u, 0x05u, 0x06u,
    0x0Cu, 0x0Du, 0x0Eu,
    0x14u, 0x15u, 0x16u,
    0x1Cu, 0x1Du, 0x1Eu
};

static const RetailRenderLayout s_retail_fade_out = {
    { 0x80092664u, 0x8009268Cu, 0x800926B4u, 0x800926DCu,
      0x80092704u, 0x8009272Cu, 0x80092754u, 0x8009277Cu,
      0x800927A4u, 0x800927CCu, 0x800927F4u, 0x8009281Cu },
    { 0x80092674u, 0x8009269Cu, 0x800926C4u, 0x800926ECu,
      0x80092714u, 0x8009273Cu, 0x80092764u, 0x8009278Cu,
      0x800927B4u, 0x800927DCu, 0x80092804u, 0x8009282Cu },
    { 0x80092684u, 0x800926ACu, 0x800926D4u, 0x800926FCu,
      0x80092724u, 0x8009274Cu, 0x80092774u, 0x8009279Cu,
      0x800927C4u, 0x800927ECu, 0x80092814u, 0x8009283Cu },
    0x8009284Cu, 0x80092854u,
    { 0x80092868u, 0x80092884u },
    0x8009286Cu, { 0x80092870u, 0x8009288Cu },
    0x80092880u, 0x8009289Cu,
    { 0x800928A8u, 0x800928C4u },
    0x800928ACu, { 0x800928B0u, 0x800928CCu },
    0x800928C0u, 0x800928DCu
};

static const RetailRenderLayout s_retail_fade_in = {
    { 0x8009290Cu, 0x80092934u, 0x8009295Cu, 0x80092984u,
      0x800929ACu, 0x800929D4u, 0x800929FCu, 0x80092A24u,
      0x80092A4Cu, 0x80092A74u, 0x80092A9Cu, 0x80092AC4u },
    { 0x8009291Cu, 0x80092944u, 0x8009296Cu, 0x80092994u,
      0x800929BCu, 0x800929E4u, 0x80092A0Cu, 0x80092A34u,
      0x80092A5Cu, 0x80092A84u, 0x80092AACu, 0x80092AD4u },
    { 0x8009292Cu, 0x80092954u, 0x8009297Cu, 0x800929A4u,
      0x800929CCu, 0x800929F4u, 0x80092A1Cu, 0x80092A44u,
      0x80092A6Cu, 0x80092A94u, 0x80092ABCu, 0x80092AE4u },
    0x80092AF4u, 0x80092AFCu,
    { 0x80092B10u, 0x80092B2Cu },
    0x80092B14u, { 0x80092B18u, 0x80092B34u },
    0x80092B28u, 0x80092B44u,
    { 0x80092B50u, 0x80092B6Cu },
    0x80092B54u, { 0x80092B58u, 0x80092B74u },
    0x80092B68u, 0x80092B84u
};

static size_t ram_index(u32 address)
{
    return (size_t)(address & RAM_MASK);
}

static u8 raw_load_u8(u32 address)
{
    return g_PsxRam[ram_index(address)];
}

static u16 raw_load_u16(u32 address)
{
    u16 value;
    memcpy(&value, g_PsxRam + ram_index(address), sizeof(value));
    return value;
}

static s16 raw_load_s16(u32 address)
{
    return (s16)raw_load_u16(address);
}

static u32 raw_load_u32(u32 address)
{
    u32 value;
    memcpy(&value, g_PsxRam + ram_index(address), sizeof(value));
    return value;
}

static void raw_store_u8(u32 address, u8 value)
{
    g_PsxRam[ram_index(address)] = value;
}

static void raw_store_u16(u32 address, u16 value)
{
    memcpy(g_PsxRam + ram_index(address), &value, sizeof(value));
}

static void raw_store_u32(u32 address, u32 value)
{
    memcpy(g_PsxRam + ram_index(address), &value, sizeof(value));
}

static u32 host_pointer_to_guest(const void* pointer)
{
    uintptr_t base = (uintptr_t)(const void*)g_PsxRam;
    uintptr_t current = (uintptr_t)pointer;
    return 0x80000000u | (u32)(current - base);
}

static void append_event(TraceEvent* events, size_t* count,
                         u32 pc, u32 kind, u32 address,
                         u32 width, u32 value)
{
    if (*count >= (size_t)EVENT_CAPACITY) {
        s_trace_overflow = 1;
        (*count)++;
        return;
    }
    events[*count].pc = pc;
    events[*count].kind = kind;
    events[*count].address = address;
    events[*count].width = width;
    events[*count].value = value;
    (*count)++;
}

void wm_925a0_test_trace(u32 pc, u32 kind, u32 address,
                         u32 width, u32 value)
{
    append_event(s_actual, &s_actual_count, pc, kind, address, width, value);
}

void wm_712d0_test_trace(u32 pc, u32 kind, u32 address,
                         u32 width, u32 value)
{
    (void)pc;
    (void)kind;
    (void)address;
    (void)width;
    (void)value;
}

static void expect_event(u32 pc, u32 kind, u32 address,
                         u32 width, u32 value)
{
    append_event(s_expected, &s_expected_count,
                 pc, kind, address, width, value);
}

static void check_case(const char* fixture, const char* assertion, int ok)
{
    s_total++;
    if (ok) {
        s_pass++;
    } else {
        s_fail++;
        printf("FAIL [%s]: ASSERTION %s\n", fixture, assertion);
    }
}

static int traces_equal(void)
{
    size_t i;
    if (s_trace_overflow != 0 || s_actual_count != s_expected_count)
        return 0;
    for (i = 0u; i < s_actual_count; i++) {
        if (s_actual[i].pc != s_expected[i].pc ||
            s_actual[i].kind != s_expected[i].kind ||
            s_actual[i].address != s_expected[i].address ||
            s_actual[i].width != s_expected[i].width ||
            s_actual[i].value != s_expected[i].value)
            return 0;
    }
    return 1;
}

static int first_event(u32 kind, u32 address)
{
    size_t i;
    for (i = 0u; i < s_actual_count && i < (size_t)EVENT_CAPACITY; i++) {
        if (s_actual[i].kind == kind && s_actual[i].address == address)
            return (int)i;
    }
    return -1;
}

static u16 retail_get_tpage(int tp, int abr, int x, int y)
{
    u32 value = (((u32)tp & 3u) << 7) |
                (((u32)abr & 3u) << 5) |
                (((u32)y & 0x100u) >> 4) |
                (((u32)x & 0x3FFu) >> 6) |
                (((u32)y & 0x200u) << 2);
    return (u16)value;
}

u16 GetTPage(int tp, int abr, int x, int y)
{
    s_get_tpage_calls++;
    s_get_tp = tp;
    s_get_abr = abr;
    s_get_x = x;
    s_get_y = y;
    return retail_get_tpage(tp, abr, x, y);
}

void SetDrawTPage(TestDrTPage* packet, int dfe, int dtd, int tpage)
{
    u32 mode;
    s_set_draw_tpage_calls++;
    s_set_packet = host_pointer_to_guest(packet);
    s_set_dfe = dfe;
    s_set_dtd = dtd;
    s_set_tpage = tpage;
    packet->tag = (packet->tag & 0x00FFFFFFu) | 0x01000000u;
    mode = 0xE1000000u | ((dtd != 0) ? 0x0200u : 0u) |
           ((dfe != 0) ? 0x0400u : 0u) | ((u32)tpage & 0x9FFu);
    packet->code[0] = mode;
}

void SetSemiTrans(void* primitive, int enabled)
{
    u8 code = raw_load_u8(host_pointer_to_guest(primitive) + 7u);
    if (enabled != 0)
        code = (u8)(code | 2u);
    else
        code = (u8)(code & (u8)~2u);
    raw_store_u8(host_pointer_to_guest(primitive) + 7u, code);
}

static u32 fixture_slot(const Fixture* fixture)
{
    return TEST_POOL + (u32)fixture->slot_index * RETAIL_SLOT_STRIDE;
}

static u32 fixture_quad(void)
{
    return RETAIL_QUADS + RETAIL_QUAD_STRIDE;
}

static void reset_observers(void)
{
    s_actual_count = 0u;
    s_expected_count = 0u;
    s_trace_overflow = 0;
    s_get_tpage_calls = 0;
    s_set_draw_tpage_calls = 0;
    s_get_tp = s_get_abr = s_get_x = s_get_y = 0;
    s_set_dfe = s_set_dtd = s_set_tpage = 0;
    s_set_packet = 0u;
}

static void seed_fixture(const Fixture* fixture)
{
    u32 slot = fixture_slot(fixture);
    u32 quad = fixture_quad();
    memset(g_PsxRam, 0xA5, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0x5A, sizeof(g_PsxScratchpad));
    raw_store_u32(RETAIL_POOL_PTR, TEST_POOL);
    raw_store_u32(RETAIL_CONTEXT_PTR, TEST_CONTEXT);
    raw_store_u32(RETAIL_ACTIVE, fixture->active);
    raw_store_u32(RETAIL_TPAGE_ABR, 2u);
    raw_store_u32(RETAIL_SPEED, fixture->speed);
    raw_store_u32(RETAIL_BUFFER_INDEX, 1u);
    raw_store_u32(TEST_CONTEXT + 0x70u, TEST_OT);
    raw_store_u32(TEST_CONTEXT + 0x74u, TEST_WRONG_OT);
    raw_store_u32(TEST_OT, 0xAA005678u);
    raw_store_u32(TEST_WRONG_OT, 0xBB006789u);
    raw_store_u32(quad, 0x08ABCDEFu);
    raw_store_u32(RETAIL_DR_TPAGE, 0x01FEDCBAu);
    raw_store_u32(RETAIL_DR_TPAGE + 4u, 0xE1A5A5A5u);
    raw_store_u16(slot + RETAIL_SLOT_MODE, (u16)fixture->mode);
    raw_store_u16(slot + RETAIL_SLOT_FADE_MODE, (u16)fixture->fade_mode);
    raw_store_u16(slot + RETAIL_SLOT_COUNTER, fixture->counter);
    raw_store_u32(slot + RETAIL_SLOT_FADE, fixture->fade);
    reset_observers();
}

static void build_expected_render(u32 slot, u32 fade,
                                  const RetailRenderLayout* layout)
{
    u32 quad = fixture_quad();
    u32 linked_quad = 0x08005678u;
    u32 linked_ot_quad = 0xAA09CE90u;
    u32 linked_dr = 0x0109CE90u;
    unsigned i;

    for (i = 0u; i < 12u; i++) {
        expect_event(layout->index_pc[i], WM_925A0_TRACE_LW,
                     RETAIL_BUFFER_INDEX, 4u, 1u);
        expect_event(layout->fade_pc[i], WM_925A0_TRACE_LW,
                     slot + RETAIL_SLOT_FADE, 4u, fade);
        expect_event(layout->color_pc[i], WM_925A0_TRACE_SB,
                     quad + s_color_offsets[i], 1u, (u32)(u8)fade);
    }
    expect_event(layout->setup_index_pc, WM_925A0_TRACE_LW,
                 RETAIL_BUFFER_INDEX, 4u, 1u);
    expect_event(layout->setup_context_pc, WM_925A0_TRACE_LW,
                 RETAIL_CONTEXT_PTR, 4u, TEST_CONTEXT);

    expect_event(layout->quad_context_pc[0], WM_925A0_TRACE_LW,
                 TEST_CONTEXT + 0x70u, 4u, TEST_OT);
    expect_event(layout->quad_tag_pc, WM_925A0_TRACE_LW,
                 quad, 4u, 0x08ABCDEFu);
    expect_event(layout->quad_ot_pc[0], WM_925A0_TRACE_LW,
                 TEST_OT, 4u, 0xAA005678u);
    expect_event(layout->quad_tag_store_pc, WM_925A0_TRACE_SW,
                 quad, 4u, linked_quad);
    expect_event(layout->quad_context_pc[1], WM_925A0_TRACE_LW,
                 TEST_CONTEXT + 0x70u, 4u, TEST_OT);
    expect_event(layout->quad_ot_pc[1], WM_925A0_TRACE_LW,
                 TEST_OT, 4u, 0xAA005678u);
    expect_event(layout->quad_ot_store_pc, WM_925A0_TRACE_SW,
                 TEST_OT, 4u, linked_ot_quad);

    expect_event(layout->dr_context_pc[0], WM_925A0_TRACE_LW,
                 TEST_CONTEXT + 0x70u, 4u, TEST_OT);
    expect_event(layout->dr_tag_pc, WM_925A0_TRACE_LW,
                 RETAIL_DR_TPAGE, 4u, 0x01FEDCBAu);
    expect_event(layout->dr_ot_pc[0], WM_925A0_TRACE_LW,
                 TEST_OT, 4u, linked_ot_quad);
    expect_event(layout->dr_tag_store_pc, WM_925A0_TRACE_SW,
                 RETAIL_DR_TPAGE, 4u, linked_dr);
    expect_event(layout->dr_context_pc[1], WM_925A0_TRACE_LW,
                 TEST_CONTEXT + 0x70u, 4u, TEST_OT);
    expect_event(layout->dr_ot_pc[1], WM_925A0_TRACE_LW,
                 TEST_OT, 4u, linked_ot_quad);
    expect_event(layout->dr_ot_store_pc, WM_925A0_TRACE_SW,
                 TEST_OT, 4u, 0xAA09D310u);
}

static void build_expected(const Fixture* fixture)
{
    u32 slot = fixture_slot(fixture);
    u16 tpage = retail_get_tpage(0, 2, 896, 256);
    u32 arithmetic;
    s16 effective_fade_mode = fixture->fade_mode;

    expect_event(0x800925A0u, WM_925A0_TRACE_ENTRY,
                 (u32)fixture->slot_index, 0u, 0u);
    expect_event(0x800925ACu, WM_925A0_TRACE_LW,
                 RETAIL_POOL_PTR, 4u, TEST_POOL);
    expect_event(0x800925C0u, WM_925A0_TRACE_LH,
                 slot + RETAIL_SLOT_MODE, 2u, (u32)(u16)fixture->mode);

    if (fixture->mode == 13) {
        expect_event(0x800925E8u, WM_925A0_TRACE_LW,
                     RETAIL_TPAGE_ABR, 4u, 2u);
        expect_event(0x800925F0u, WM_925A0_TRACE_SH,
                     slot + RETAIL_SLOT_MODE, 2u, 0u);
        expect_event(0x800925F8u, WM_925A0_TRACE_SH,
                     slot + RETAIL_SLOT_FADE_MODE, 2u, 1u);
        effective_fade_mode = 1;
    } else if (fixture->mode == 12) {
        expect_event(0x80092608u, WM_925A0_TRACE_LW,
                     RETAIL_TPAGE_ABR, 4u, 2u);
        expect_event(0x80092610u, WM_925A0_TRACE_SH,
                     slot + RETAIL_SLOT_MODE, 2u, 0u);
        expect_event(0x80092614u, WM_925A0_TRACE_SH,
                     slot + RETAIL_SLOT_FADE_MODE, 2u, 0u);
        effective_fade_mode = 0;
    }
    if (fixture->initializes != 0) {
        expect_event(0x8009261Cu, WM_925A0_TRACE_SW,
                     RETAIL_ACTIVE, 4u, 1u);
        expect_event(0x80092620u, WM_925A0_TRACE_CALL,
                     0x80043A1Cu, 0u, 2u);
        expect_event(0x80092628u, WM_925A0_TRACE_RETURN,
                     0x80043A1Cu, 0u, (u32)tpage);
        expect_event(0x80092638u, WM_925A0_TRACE_CALL,
                     0x80043E20u, 0u, (u32)tpage);
        expect_event(0x80092640u, WM_925A0_TRACE_RETURN,
                     0x80043E20u, 0u, 0u);
    }

    expect_event(0x80092640u, WM_925A0_TRACE_LH,
                 slot + RETAIL_SLOT_FADE_MODE, 2u,
                 (u32)(u16)effective_fade_mode);
    if (effective_fade_mode != 0 && effective_fade_mode != 1) {
        expect_event(0x80092658u, WM_925A0_TRACE_CALLBACK_RETURN,
                     0u, 0u, 1u);
        return;
    }

    if (effective_fade_mode == 0) {
        build_expected_render(slot, fixture->fade, &s_retail_fade_out);
        expect_event(0x800928E0u, WM_925A0_TRACE_LW,
                     slot + RETAIL_SLOT_FADE, 4u, fixture->fade);
        expect_event(0x800928E8u, WM_925A0_TRACE_LW,
                     RETAIL_SPEED, 4u, fixture->speed);
        arithmetic = fixture->fade - fixture->speed;
        expect_event(0x800928F8u, WM_925A0_TRACE_SW,
                     slot + RETAIL_SLOT_FADE, 4u, arithmetic);
        if ((arithmetic & 0x80000000u) != 0u) {
            expect_event(0x80092904u, WM_925A0_TRACE_SW,
                         slot + RETAIL_SLOT_FADE, 4u, 0u);
            expect_event(0x80092BB4u, WM_925A0_TRACE_SW,
                         RETAIL_ACTIVE, 4u, 0u);
        }
    } else {
        build_expected_render(slot, fixture->fade, &s_retail_fade_in);
        expect_event(0x80092B88u, WM_925A0_TRACE_LW,
                     slot + RETAIL_SLOT_FADE, 4u, fixture->fade);
        expect_event(0x80092B90u, WM_925A0_TRACE_LW,
                     RETAIL_SPEED, 4u, fixture->speed);
        arithmetic = fixture->fade + fixture->speed;
        expect_event(0x80092B9Cu, WM_925A0_TRACE_SW,
                     slot + RETAIL_SLOT_FADE, 4u, arithmetic);
        if ((arithmetic & 0x80000000u) == 0u && arithmetic >= 255u) {
            expect_event(0x80092BACu, WM_925A0_TRACE_SW,
                         slot + RETAIL_SLOT_FADE, 4u, 255u);
            expect_event(0x80092BB4u, WM_925A0_TRACE_SW,
                         RETAIL_ACTIVE, 4u, 0u);
        }
    }
    expect_event(0x80092BB8u, WM_925A0_TRACE_LHU,
                 slot + RETAIL_SLOT_COUNTER, 2u, (u32)fixture->counter);
    expect_event(0x80092BC4u, WM_925A0_TRACE_SH,
                 slot + RETAIL_SLOT_COUNTER, 2u,
                 (u32)fixture->expected_counter);
    expect_event(0x80092BC8u, WM_925A0_TRACE_CALLBACK_RETURN,
                 0u, 0u, (u32)fixture->expected_return);
}

static int colors_equal(u32 quad, u8 color)
{
    unsigned i;
    for (i = 0u; i < 12u; i++)
        if (raw_load_u8(quad + s_color_offsets[i]) != color)
            return 0;
    return 1;
}

static void run_fixture(const Fixture* fixture)
{
    u32 slot = fixture_slot(fixture);
    u32 quad = fixture_quad();
    s32 result;
    int quad_store;
    int first_ot_store;
    int dr_store;
    int second_ot_store;
    u16 expected_tpage = retail_get_tpage(0, 2, 896, 256);

    seed_fixture(fixture);
    build_expected(fixture);
    result = wm_800925A0(fixture->slot_index);

    check_case(fixture->name, "retail-ordered-access-trace", traces_equal());
    check_case(fixture->name, "return-exact", result == fixture->expected_return);
    check_case(fixture->name, "fade-exact",
               raw_load_u32(slot + RETAIL_SLOT_FADE) == fixture->expected_fade);
    check_case(fixture->name, "counter-exact",
               raw_load_u16(slot + RETAIL_SLOT_COUNTER) == fixture->expected_counter);
    check_case(fixture->name, "active-flag-exact",
               raw_load_u32(RETAIL_ACTIVE) == fixture->expected_active);

    if (fixture->fade_mode == 0 || fixture->fade_mode == 1 ||
        fixture->initializes != 0) {
        quad_store = first_event(WM_925A0_TRACE_SW, quad);
        first_ot_store = first_event(WM_925A0_TRACE_SW, TEST_OT);
        dr_store = first_event(WM_925A0_TRACE_SW, RETAIL_DR_TPAGE);
        second_ot_store = -1;
        if (first_ot_store >= 0) {
            size_t i;
            for (i = (size_t)first_ot_store + 1u;
                 i < s_actual_count && i < (size_t)EVENT_CAPACITY; i++) {
                if (s_actual[i].kind == WM_925A0_TRACE_SW &&
                    s_actual[i].address == TEST_OT) {
                    second_ot_store = (int)i;
                    break;
                }
            }
        }
        check_case(fixture->name, "M6-four-vertex-rgb",
                   colors_equal(quad, (u8)fixture->fade));
        check_case(fixture->name, "M8-first-ot-insertion",
                   raw_load_u32(quad) == 0x08005678u && quad_store >= 0);
        check_case(fixture->name, "M9-second-ot-insertion",
                   raw_load_u32(RETAIL_DR_TPAGE) == 0x0109CE90u &&
                   dr_store >= 0);
        check_case(fixture->name, "M10-ot-insertion-order",
                   quad_store >= 0 && first_ot_store > quad_store &&
                   dr_store > first_ot_store && second_ot_store > dr_store &&
                   raw_load_u32(TEST_OT) == 0xAA09D310u);
        check_case(fixture->name, "M11-ot-bucket-address",
                   raw_load_u32(TEST_OT) == 0xAA09D310u &&
                   raw_load_u32(TEST_WRONG_OT) == 0xBB006789u);
        check_case(fixture->name, "M13-link-write-order",
                   quad_store >= 0 && first_ot_store > quad_store);
    }

    if (fixture->mode == 13) {
        check_case(fixture->name, "M4-255-clamp",
                   raw_load_u32(slot + RETAIL_SLOT_FADE) == 255u);
        check_case(fixture->name, "M7-tpage-dr-mode",
                   s_get_tpage_calls == 1 && s_set_draw_tpage_calls == 1 &&
                   s_get_tp == 0 && s_get_abr == 2 &&
                   s_get_x == 896 && s_get_y == 256 &&
                   s_set_packet == RETAIL_DR_TPAGE && s_set_dfe == 1 &&
                   s_set_dtd == 0 && s_set_tpage == (int)expected_tpage &&
                   raw_load_u32(RETAIL_DR_TPAGE + 4u) ==
                       (0xE1000400u | (u32)expected_tpage));
        check_case(fixture->name, "M12-slot-field-write",
                   raw_load_s16(slot + RETAIL_SLOT_MODE) == 0 &&
                   raw_load_s16(slot + RETAIL_SLOT_FADE_MODE) == 1);
    }
    if (strcmp(fixture->name, "fade-out-intermediate") == 0) {
        check_case(fixture->name, "M1-return-value", result == 1);
        check_case(fixture->name, "M2-state-dispatch",
                   raw_load_u32(slot + RETAIL_SLOT_FADE) == fixture->fade - fixture->speed);
        check_case(fixture->name, "M3-fade-arithmetic",
                   raw_load_u32(slot + RETAIL_SLOT_FADE) == 33u);
    }
    if (strcmp(fixture->name, "fade-out-terminal") == 0) {
        check_case(fixture->name, "M5-terminal-return-3", result == 3);
        check_case(fixture->name, "M14-terminal-state-published", result == 3);
    }
}

int ControllerPopState(void)
{
    s_controller_calls++;
    return 0;
}

int VSync(int mode)
{
    (void)mode;
    s_vsync_calls++;
    return 0;
}

int CdSync(int mode, u8* result)
{
    (void)mode;
    (void)result;
    s_cdsync_calls++;
    return 0;
}

u32* ClearOTagR(u32* ot, int count)
{
    (void)count;
    s_clear_ot_calls++;
    return ot;
}

void func_800250E0(int context)
{
    (void)context;
    s_250e0_calls++;
}

void func_8001D468(void)
{
    s_1d468_calls++;
}

u32 wm_800967E4_dispatch_cd_work(void)
{
    return 0u;
}

static s16 natural_cb0(int slot_index)
{
    (void)slot_index;
    s_natural_cb0_calls++;
    return 1;
}

static void plant_slot(int index, s16 state, u32 cb0, u32 cb1)
{
    u32 slot = TEST_POOL + (u32)index * RETAIL_SLOT_STRIDE;
    memset(PSX_ADDR(slot), 0, RETAIL_SLOT_STRIDE);
    raw_store_u16(slot + WM_SCHED_OFF_STATE, (u16)state);
    raw_store_u32(slot + WM_SCHED_OFF_CB0, cb0);
    raw_store_u32(slot + WM_SCHED_OFF_CB1, cb1);
}

static void run_terminal_scheduler_publication(void)
{
    const Fixture terminal = {
        "scheduler-terminal", 0, 0, 0, 3u, 7u, 21u, 0x12345678u,
        3, 0u, 22u, 0u, 0
    };

    seed_fixture(&terminal);
    memset(PSX_ADDR(TEST_POOL), 0,
           (size_t)WM_SCHED_SLOT_COUNT * RETAIL_SLOT_STRIDE);
    plant_slot(0, 1, RETAIL_CB0_SLOT0, RETAIL_CB1_SLOT0);
    raw_store_u16(TEST_POOL + RETAIL_SLOT_FADE_MODE, 0u);
    raw_store_u16(TEST_POOL + RETAIL_SLOT_COUNTER, terminal.counter);
    raw_store_u32(TEST_POOL + RETAIL_SLOT_FADE, terminal.fade);
    wm_sched_callback_registry_clear();
    wm_sched_reset();

    wm_80097800();
    check_case("scheduler-terminal", "M14-terminal-state-published",
               raw_load_s16(TEST_POOL + WM_SCHED_OFF_STATE) == 3 &&
               wm_sched_get_callbacks_executed() == 1 &&
               wm_sched_get_completed_passes() == 1);
    wm_80097800();
    check_case("scheduler-terminal", "state-3-is-dormant-next-pass",
               raw_load_s16(TEST_POOL + WM_SCHED_OFF_STATE) == 3 &&
               wm_sched_get_callbacks_executed() == 1 &&
               wm_sched_get_completed_passes() == 2);
}

static void run_natural_route(void)
{
    u32 slot0 = TEST_POOL;
    u32 slot1 = TEST_POOL + RETAIL_SLOT_STRIDE;
    s16 state_after_first;

    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0, sizeof(g_PsxScratchpad));
    raw_store_u32(RETAIL_POOL_PTR, TEST_POOL);
    raw_store_u32(OR_ENVREC0 + OR_OT_OFF, TEST_OT0);
    raw_store_u32(OR_ENVREC1 + OR_OT_OFF, TEST_OT1);
    raw_store_u32(RETAIL_CONTEXT_PTR, OR_ENVREC0);
    raw_store_u32(RETAIL_BUFFER_INDEX, 0u);
    raw_store_u32(RETAIL_SPEED, 8u);
    raw_store_u32(RETAIL_ACTIVE, 1u);
    raw_store_u32(RETAIL_DR_TPAGE, 0x01000000u);
    raw_store_u32(TEST_OT0, 0xAA001111u);
    raw_store_u32(TEST_OT1, 0xBB002222u);
    raw_store_u32(RETAIL_QUADS, 0x08000000u);
    raw_store_u32(RETAIL_QUADS + RETAIL_QUAD_STRIDE, 0x08000000u);

    plant_slot(0, 0, RETAIL_CB0_SLOT0, RETAIL_CB1_SLOT0);
    plant_slot(1, 0, RETAIL_CB0_SLOT1, RETAIL_CB1_SLOT1);
    raw_store_u32(slot0 + RETAIL_SLOT_FADE, 248u);
    raw_store_u16(slot0 + RETAIL_SLOT_COUNTER, 9u);

    s_controller_calls = s_vsync_calls = s_cdsync_calls = 0;
    s_clear_ot_calls = s_250e0_calls = s_1d468_calls = 0;
    s_natural_cb0_calls = 0;
    reset_observers();
    wm_sched_callback_registry_clear();
    wm_sched_reset();
    wm_fp_reset();
    wm_sched_callback_register(RETAIL_CB0_SLOT0, natural_cb0);
    wm_sched_callback_register(RETAIL_CB0_SLOT1, natural_cb0);

    wm_80097800();
    state_after_first = raw_load_s16(slot0 + WM_SCHED_OFF_STATE);
    check_case("controller-natural", "first-pass-complete",
               wm_sched_get_completed_passes() == 1 &&
               wm_sched_get_callbacks_executed() == 2 &&
               s_natural_cb0_calls == 2 && state_after_first == 1 &&
               raw_load_s16(slot1 + WM_SCHED_OFF_STATE) == 1);

    s_actual_count = 0u;
    wm_800712D0_frame_prologue();

    check_case("controller-natural", "real-wm-800925A0-0-executed",
               s_actual_count != 0u &&
               s_actual[0].kind == WM_925A0_TRACE_ENTRY &&
               s_actual[0].address == 0u &&
               wm_sched_get_callbacks_executed() == 3);
    check_case("controller-natural", "callback-return-1-published",
               raw_load_s16(slot0 + WM_SCHED_OFF_STATE) == 1 &&
               raw_load_u32(slot0 + RETAIL_SLOT_FADE) == 240u &&
               raw_load_u16(slot0 + RETAIL_SLOT_COUNTER) == 10u);
    check_case("controller-natural", "next-frontier-slot1-cb1-8008A72C",
               wm_sched_get_outcome() == WM_SCHED_STOP_MISSING_CALLBACK &&
               wm_sched_get_frontier_pc() == RETAIL_CB1_SLOT1 &&
               wm_sched_get_last_slot() == 1 &&
               wm_sched_get_last_callback_state() == 1);
    check_case("controller-natural", "frame-frontier-remains-80071490",
               wm_fp_get_cut_pc() == 0x80071490u &&
               wm_fp_get_scheduler_calls() == 1 &&
               s_controller_calls == 1 && s_vsync_calls == 0 &&
               s_cdsync_calls == 1 && s_clear_ot_calls == 1 &&
               s_250e0_calls == 1 && s_1d468_calls == 1);

    printf("NATURAL_925A0 slot=0 a0=0 return=1 scheduler_state=1\n");
    printf("NEXT_UNRESOLVED slot=1 state=1 selector=cb1 target=0x8008A72C a0=1 body=not-executed\n");
    printf("FRAME_DRIVER_FRONTIER 0x80071490\n");
}

int main(void)
{
    static const Fixture fixtures[] = {
        { "init-mode-12", 3, 12, 7, 50u, 7u, 0xFFFEu, 0x13572468u,
          1, 43u, 0xFFFFu, 1u, 1 },
        { "init-mode-13-clamp", 7, 13, -2, 250u, 11u, 4u, 0x24681357u,
          1, 255u, 5u, 0u, 1 },
        { "fade-out-intermediate", 11, 0, 0, 40u, 7u, 8u, 0x11223344u,
          1, 33u, 9u, 0x11223344u, 0 },
        { "fade-out-terminal", 15, 0, 0, 3u, 7u, 9u, 0x55667788u,
          3, 0u, 10u, 0u, 0 },
        { "fade-in-intermediate", 19, 0, 1, 40u, 7u, 10u, 0x99AABBCCu,
          1, 47u, 11u, 0x99AABBCCu, 0 },
        { "fade-in-clamp", 23, 0, 1, 250u, 11u, 11u, 0xCCDDEEFFu,
          1, 255u, 12u, 0u, 0 },
        { "dormant-neighbor-2", 27, 0, 2, 77u, 9u, 12u, 0x10203040u,
          1, 77u, 12u, 0x10203040u, 0 },
        { "invalid-neighbor-minus-1", 31, 0, -1, 88u, 9u, 13u, 0x50607080u,
          1, 88u, 13u, 0x50607080u, 0 }
    };
    size_t i;

    printf("W34B20-B REAL PRODUCTION 0x800925A0 CERTIFICATE\n");
    printf("RETAIL_BOUNDARY [0x800925A0,0x80092BE4) bytes=1604 insns=401\n");
    printf("RETAIL_SLICE_SHA256 70e58e7760c93020bca9e099e3d89185797eec64560a4ab0cb8ea60caa7f4671\n");
    printf("SUBJECT pc_port/src/world_map_callback_925a0.c::wm_800925A0\n");
    for (i = 0u; i < sizeof(fixtures) / sizeof(fixtures[0]); i++)
        run_fixture(&fixtures[i]);
    run_terminal_scheduler_publication();
    run_natural_route();
    printf("=== Results: %d/%d PASS ===\n", s_pass, s_total);
    return s_fail == 0 ? 0 : 1;
}
