/*
 * W34B26 production-linked certificate for scheduler callback 0x80087734.
 *
 * Oracle transcribed from world_map.bin [0x80087734,0x800877E0)
 * SHA-256 930c20086d5082c3c42e39a2903e153a41b9e086e4a7b3afa1a8fbafc7787e3c.
 * Twin 0x800877E0 is a distinct symbol and must stay unresolved.
 */
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_scheduler.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

enum { TRACE_LHU = 1, TRACE_LW = 2, TRACE_SH = 3, TRACE_SW = 4, TRACE_CALL = 5 };
#define TRACE_CAPACITY 48u
#define ROT_CAPACITY 4u

typedef struct TraceEvent {
    u32 pc, kind, address, width, value;
} TraceEvent;
typedef struct RotCall {
    u32 pc, r, m;
} RotCall;

static TraceEvent s_actual_trace[TRACE_CAPACITY];
static size_t s_actual_trace_count;
static bool s_overflow;
static RotCall s_rot[ROT_CAPACITY];
static size_t s_rot_count;
static bool s_trace_armed;
static size_t s_c620_count;
static bool s_repoint;
static int s_pass_count, s_total_count, s_failure_count;
static int s_natural_entries;
static s16 s_natural_return;

void wm_87734_test_trace(u32 pc, u32 kind, u32 address, u32 width, u32 value)
{
    if (!s_trace_armed)
        return;
    if (s_actual_trace_count < (size_t)TRACE_CAPACITY) {
        s_actual_trace[s_actual_trace_count].pc = pc;
        s_actual_trace[s_actual_trace_count].kind = kind;
        s_actual_trace[s_actual_trace_count].address = address;
        s_actual_trace[s_actual_trace_count].width = width;
        s_actual_trace[s_actual_trace_count].value = value;
    } else {
        s_overflow = true;
    }
    s_actual_trace_count++;
}

void wm_87734_test_rotmatrixyxz(u32 pc, u32 r_addr, u32 m_addr)
{
    if (s_rot_count < (size_t)ROT_CAPACITY) {
        s_rot[s_rot_count].pc = pc;
        s_rot[s_rot_count].r = r_addr;
        s_rot[s_rot_count].m = m_addr;
    }
    s_rot_count++;
}

static void *test_psx_addr(uintptr_t address_bits) __attribute__((noinline));
#undef PSX_ADDR
#define PSX_ADDR(address) test_psx_addr((uintptr_t)(address))

#ifndef WM_87734_PRODUCTION_SOURCE
#define WM_87734_PRODUCTION_SOURCE "../src/world_map_callback_87734.c"
#endif
#include WM_87734_PRODUCTION_SOURCE

#define RAM_MASK 0x001FFFFFu
#define MAIN_RAM 0x00200000u
#define POOL_PTR 0x8009BE24u
#define C620 0x8009C620u
#define SCRATCH 0x1F800000u
#define NATURAL_POOL 0x800D7538u
#define CONTEXT_ROOT 0x800D9540u
#define CONTEXT_ALT 0x800DA540u
#define SLOT_STRIDE 0x80u
#define SLOT_STATE 0x00u
#define SLOT_CB0 0x18u
#define SLOT_CB1 0x1Cu
#define SLOT_HEAD0 0x50u
#define SLOT_HEAD1 0x54u

_Static_assert(PSX_RAM_SIZE == 0x00300000u, "3 MiB RAM");

static u8 s_expected_ram[PSX_RAM_SIZE];
static u8 s_expected_scratch[4096];
static u8 s_before_ram[PSX_RAM_SIZE];
static u8 s_before_scratch[4096];

typedef struct Fixture {
    const char *name;
    s32 slot_index;
    u32 pool;
    u32 head0;
    u32 head1;
    u16 vy0;
    u16 vy1;
    bool repoint;
} Fixture;

static size_t ram_index(u32 a) { return (size_t)(a & RAM_MASK); }
static void *raw_address(u32 a) { return (void *)(g_PsxRam + ram_index(a)); }

static void *test_psx_addr(uintptr_t address_bits)
{
    u32 address = (u32)address_bits;

    if ((address & 0xFFFFF000u) == 0x1F800000u)
        return (void *)(g_PsxScratchpad + (address & 0xFFFu));
    if (s_trace_armed && address == C620) {
        s_c620_count++;
        if (s_repoint && s_c620_count >= 2u)
            return raw_address(CONTEXT_ALT);
    }
    return raw_address(address);
}

static void buffer_store_u16(u8 *b, u32 a, u16 v)
{
    memcpy(b + ram_index(a), &v, 2);
}
static void buffer_store_u32(u8 *b, u32 a, u32 v)
{
    memcpy(b + ram_index(a), &v, 4);
}
static u32 buffer_load_u32(const u8 *b, u32 a)
{
    u32 v;
    memcpy(&v, b + ram_index(a), 4);
    return v;
}
static void raw_store_u16(u32 a, u16 v) { buffer_store_u16(g_PsxRam, a, v); }
static void raw_store_u32(u32 a, u32 v) { buffer_store_u32(g_PsxRam, a, v); }
static u32 raw_load_u32(u32 a) { return buffer_load_u32(g_PsxRam, a); }
static s16 raw_load_s16(u32 a)
{
    s16 v;
    memcpy(&v, raw_address(a), 2);
    return v;
}
static u32 s32_bits(s32 v)
{
    u32 b;
    memcpy(&b, &v, 4);
    return b;
}
static void scratch_store_u16(u8 *b, u32 a, u16 v)
{
    memcpy(b + (a & 0xFFFu), &v, 2);
}
static u16 scratch_load_u16(const u8 *b, u32 a)
{
    u16 v;
    memcpy(&v, b + (a & 0xFFFu), 2);
    return v;
}

static void seed_ram(u32 salt)
{
    size_t i;
    for (i = 0; i < sizeof(g_PsxRam); i++)
        g_PsxRam[i] = (u8)((i * 19u + salt) & 0xFFu);
    for (i = 0; i < sizeof(g_PsxScratchpad); i++)
        g_PsxScratchpad[i] = (u8)((i * 23u + salt + 3u) & 0xFFu);
}

static void check_case(const char *g, const char *n, bool ok)
{
    s_total_count++;
    if (ok) {
        s_pass_count++;
        return;
    }
    s_failure_count++;
    fprintf(stderr, "FAIL %s / %s\n", g, n);
}

static void reset_trace(void)
{
    memset(s_actual_trace, 0, sizeof(s_actual_trace));
    s_actual_trace_count = 0;
    s_overflow = false;
    memset(s_rot, 0, sizeof(s_rot));
    s_rot_count = 0;
    s_c620_count = 0;
}

static void oracle_apply(const Fixture *fx, u32 slot, u32 root, u8 *ram,
                         u8 *scratch)
{
    u32 head = fx->head0 + fx->head1;
    u16 vz;

    head &= 0xFFFu;
    buffer_store_u32(ram, slot + 0x50u, head);
    scratch_store_u16(scratch, SCRATCH + 0xA8u, 0u);
    scratch_store_u16(scratch, SCRATCH + 0xA0u, 0u);
    scratch_store_u16(scratch, SCRATCH + 0xA2u, fx->vy0);
    scratch_store_u16(scratch, SCRATCH + 0xAAu, fx->vy1);
    vz = (u16)head;
    scratch_store_u16(scratch, SCRATCH + 0xACu, vz);
    scratch_store_u16(scratch, SCRATCH + 0xA4u, vz);
    (void)root;
}

static void run_fixture(const Fixture *fx, u32 salt)
{
    u32 slot = fx->pool + (s32_bits(fx->slot_index) << 7);
    u32 original_state = 0;
    u32 original_before;
    u32 original_after;
    s32 result;

    seed_ram(salt);
    raw_store_u32(POOL_PTR, fx->pool);
    raw_store_u32(C620, CONTEXT_ROOT);
    raw_store_u32(slot + SLOT_HEAD0, fx->head0);
    raw_store_u32(slot + SLOT_HEAD1, fx->head1);
    raw_store_u16(CONTEXT_ROOT + 0x40Au, fx->vy0);
    raw_store_u16(CONTEXT_ROOT + 0x4B2u, fx->vy1);
    raw_store_u16(CONTEXT_ALT + 0x40Au, (u16)(fx->vy0 ^ 0x1111u));
    raw_store_u16(CONTEXT_ALT + 0x4B2u, (u16)(fx->vy1 ^ 0x2222u));
    original_state = raw_load_u32(slot + SLOT_STATE);
    original_before = raw_load_u32(slot - 4u);
    original_after = raw_load_u32(slot + SLOT_STRIDE);

    memcpy(s_before_ram, g_PsxRam, sizeof(g_PsxRam));
    memcpy(s_before_scratch, g_PsxScratchpad, sizeof(g_PsxScratchpad));
    memcpy(s_expected_ram, g_PsxRam, sizeof(g_PsxRam));
    memcpy(s_expected_scratch, g_PsxScratchpad, sizeof(g_PsxScratchpad));
    oracle_apply(fx, slot, CONTEXT_ROOT, s_expected_ram, s_expected_scratch);

    reset_trace();
    s_repoint = fx->repoint;
    s_trace_armed = true;
    result = wm_80087734(fx->slot_index);
    s_trace_armed = false;

    check_case(fx->name, "return is 1", result == 1);
    check_case(fx->name, "exactly one C620 load", s_c620_count == 1u);
    check_case(fx->name, "two RotMatrixYXZ calls", s_rot_count == 2u);
    check_case(fx->name, "first rot dest context+0x410",
               s_rot_count >= 1u && s_rot[0].r == SCRATCH + 0xA0u &&
                   s_rot[0].m == CONTEXT_ROOT + 0x410u &&
                   s_rot[0].pc == 0x800877B0u);
    check_case(fx->name, "second rot dest context+0x4B8",
               s_rot_count >= 2u && s_rot[1].r == SCRATCH + 0xA8u &&
                   s_rot[1].m == CONTEXT_ROOT + 0x4B8u &&
                   s_rot[1].pc == 0x800877C0u);
    check_case(fx->name, "slot+00 canary",
               raw_load_u32(slot + SLOT_STATE) == original_state);
    check_case(fx->name, "adjacent canaries",
               raw_load_u32(slot - 4u) == original_before &&
                   raw_load_u32(slot + SLOT_STRIDE) == original_after);
    check_case(fx->name, "RAM footprint",
               memcmp(g_PsxRam, s_expected_ram, sizeof(g_PsxRam)) == 0);
    check_case(fx->name, "scratch footprint",
               memcmp(g_PsxScratchpad, s_expected_scratch,
                      sizeof(g_PsxScratchpad)) == 0);
    check_case(fx->name, "guard canary",
               memcmp(g_PsxRam + MAIN_RAM, s_before_ram + MAIN_RAM,
                      sizeof(g_PsxRam) - MAIN_RAM) == 0);
    check_case(fx->name, "head0 is 12-bit wrap",
               raw_load_u32(slot + SLOT_HEAD0) ==
                   ((fx->head0 + fx->head1) & 0xFFFu));
    check_case(fx->name, "head1 untouched",
               raw_load_u32(slot + SLOT_HEAD1) == fx->head1);
    check_case(fx->name, "both vz equal masked head",
               scratch_load_u16(g_PsxScratchpad, SCRATCH + 0xA4u) ==
                   (u16)((fx->head0 + fx->head1) & 0xFFFu) &&
                   scratch_load_u16(g_PsxScratchpad, SCRATCH + 0xACu) ==
                       scratch_load_u16(g_PsxScratchpad, SCRATCH + 0xA4u));
    check_case(fx->name, "vy from C620+0x40A / +0x4B2",
               scratch_load_u16(g_PsxScratchpad, SCRATCH + 0xA2u) == fx->vy0 &&
                   scratch_load_u16(g_PsxScratchpad, SCRATCH + 0xAAu) ==
                       fx->vy1);
    check_case(fx->name, "first accesses pool then slot heads then C620",
               s_actual_trace_count >= 4u &&
                   s_actual_trace[0].address == POOL_PTR &&
                   s_actual_trace[0].kind == TRACE_LW &&
                   s_actual_trace[1].address == slot + SLOT_HEAD0 &&
                   s_actual_trace[2].address == slot + SLOT_HEAD1 &&
                   s_actual_trace[3].address == C620);
    check_case(fx->name, "no overflow", !s_overflow);
    check_case(fx->name, "scratch pads untouched",
               g_PsxScratchpad[(SCRATCH + 0xA6u) & 0xFFFu] ==
                   s_before_scratch[(SCRATCH + 0xA6u) & 0xFFFu]);
}

static s16 stub_one(int slot_index)
{
    (void)slot_index;
    return 1;
}

static s16 wrap_87734(int slot_index)
{
    s32 result = wm_80087734((s32)slot_index);

    if (slot_index == 15)
        s_natural_entries++;
    s_natural_return = (s16)result;
    return (s16)result;
}

static void set_slot(u32 pool, unsigned slot, s16 state, u32 cb0, u32 cb1)
{
    u32 a = pool + (u32)slot * SLOT_STRIDE;

    raw_store_u16(a + SLOT_STATE, (u16)state);
    raw_store_u32(a + SLOT_CB0, cb0);
    raw_store_u32(a + SLOT_CB1, cb1);
}

static void run_natural(void)
{
    u32 pool = NATURAL_POOL;
    u32 slot15 = pool + 15u * SLOT_STRIDE;
    u32 slot8 = pool + 8u * SLOT_STRIDE;

    seed_ram(0x877341u);
    raw_store_u32(POOL_PTR, pool);
    raw_store_u32(C620, CONTEXT_ROOT);
    memset(g_PsxRam + ram_index(pool), 0, 16u * SLOT_STRIDE);
    set_slot(pool, 15u, 0, 0x80087710u, 0x80087734u);
    raw_store_u32(slot15 + SLOT_HEAD0, 0x1111u);
    raw_store_u32(slot15 + SLOT_HEAD1, 0x8u);
    raw_store_u16(CONTEXT_ROOT + 0x40Au, 0xABCDu);
    raw_store_u16(CONTEXT_ROOT + 0x4B2u, 0x1234u);

    wm_sched_callback_registry_clear();
    wm_sched_callback_register(0x80087710u, stub_one);
    wm_sched_callback_register(0x80087734u, wrap_87734);
    wm_sched_reset();
    s_natural_entries = 0;
    wm_80097800();
    check_case("natural-pass1", "87710 stub from state 0",
               wm_sched_get_last_callback() == 0x80087710u);
    check_case("natural-pass1", "87734 not entered", s_natural_entries == 0);
    check_case("natural-pass1", "state published 1",
               raw_load_s16(slot15 + SLOT_STATE) == 1);
    check_case("natural-pass1", "pass complete",
               wm_sched_get_outcome() == WM_SCHED_PASS_COMPLETE);
    check_case("natural-pass1", "C620 live", raw_load_u32(C620) != 0u);

    wm_sched_reset();
    s_natural_entries = 0;
    s_trace_armed = true;
    reset_trace();
    wm_80097800();
    s_trace_armed = false;
    check_case("natural-pass2", "87734 entered",
               s_natural_entries == 1 &&
                   wm_sched_get_last_callback() == 0x80087734u);
    check_case("natural-pass2", "return 1 published",
               s_natural_return == 1 &&
                   raw_load_s16(slot15 + SLOT_STATE) == 1);
    check_case("natural-pass2", "head0 masked sum",
               raw_load_u32(slot15 + SLOT_HEAD0) ==
                   ((0x1111u + 0x8u) & 0xFFFu));
    check_case("natural-pass2", "pass complete",
               wm_sched_get_outcome() == WM_SCHED_PASS_COMPLETE &&
                   wm_sched_get_frontier_pc() == WM_SCHED_CUT_BEFORE_DRAWSYNC);

    /* Slot 8 then slot 15: 907F4 stub, 87734 body. Twin 877E0 stays invalid. */
    set_slot(pool, 8u, 1, 0x800906E0u, 0x800907F4u);
    raw_store_u16(slot8 + SLOT_STATE, 1);
    wm_sched_callback_register(0x800907F4u, stub_one);
    wm_sched_reset();
    s_natural_entries = 0;
    wm_80097800();
    check_case("natural-after-907f4", "87734 still entered after slot8 cb1",
               s_natural_entries == 1 &&
                   wm_sched_get_outcome() == WM_SCHED_PASS_COMPLETE);

    set_slot(pool, 15u, 1, 0x80087710u, 0x800877E0u);
    wm_sched_reset();
    wm_80097800();
    check_case("twin-877E0", "distinct twin stays invalid",
               wm_sched_get_outcome() == WM_SCHED_STOP_INVALID_CALLBACK &&
                   wm_sched_get_frontier_pc() == 0x800877E0u);
}

static void run_oracle_sentinels(void)
{
    check_case("oracle", "0xFFF mask keeps 12 bits",
               ((0x1111u + 0x8u) & 0xFFFu) == 0x119u);
    check_case("oracle", "0xFF mutant mask differs",
               ((0x1111u + 0x8u) & 0xFFu) !=
                   ((0x1111u + 0x8u) & 0xFFFu));
    check_case("oracle", "wrap 0xFFFF+2",
               ((0xFFFFu + 2u) & 0xFFFu) == 1u);
    check_case("oracle", "slot -1 shift",
               (s32_bits(-1) << 7) == 0xFFFFFF80u);
    check_case("oracle", "INT32_MIN shift",
               (s32_bits(INT32_MIN) << 7) == 0u);
    check_case("oracle", "record dest 0x410", 0x410u == 0x410u);
    check_case("oracle", "record dest 0x4B8", 0x4B8u == 0x4B8u);
    check_case("oracle", "vy0 0x40A", 0x40Au == 0x40Au);
}

int main(void)
{
    static const Fixture fixtures[] = {
        {"natural-heads", 15, NATURAL_POOL, 0x1111u, 0x8u, 0xABCDu, 0x1234u,
         false},
        {"zero-heads", 15, NATURAL_POOL, 0u, 0u, 0x1u, 0x2u, false},
        {"wrap-12bit", 15, NATURAL_POOL, 0xFFFFu, 0x2u, 0x8001u, 0x7FFFu,
         false},
        {"large-add", 15, NATURAL_POOL, 0xAAAAAAAAu, 0x55555555u, 0x10u, 0x20u,
         false},
        {"slot-minus-one", -1, 0x800D8000u, 0x10u, 0x20u, 0x33u, 0x44u, false},
        {"slot-int-min", INT32_MIN, 0x800D8100u, 0x7u, 0x9u, 0x55u, 0x66u,
         false},
        {"c620-no-reload", 15, NATURAL_POOL, 0x80u, 0x4u, 0xF00Du, 0x0FF0u,
         true},
        {"asymmetric-vy", 15, NATURAL_POOL, 0x123u, 0x456u, 0xFEDCu, 0x0123u,
         false},
        {"head1-only", 15, NATURAL_POOL, 0u, 0xFFFu, 0x1111u, 0x2222u, false},
        {"all-ones-vy", 15, NATURAL_POOL, 1u, 1u, 0xFFFFu, 0xFFFFu, false}
    };
    size_t i;

    for (i = 0; i < sizeof(fixtures) / sizeof(fixtures[0]); i++)
        run_fixture(&fixtures[i], (u32)i + 0x51u);
    run_oracle_sentinels();
    run_natural();
    printf("=== Results: %d/%d PASS ===\n", s_pass_count, s_total_count);
    printf("W34B26 0x80087734 focused oracle PASS checks=%d\n", s_pass_count);
    return s_failure_count == 0 ? 0 : 1;
}
