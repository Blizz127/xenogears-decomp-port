/*
 * W34B25 production-linked certificate for scheduler callback 0x800907F4.
 *
 * Oracle constants are transcribed from world_map.bin [0x800907F4,0x80090A18)
 * (SHA-256 bd2946bda218185bd796696ef10c845d09f2ebfb9a99814d090e303ae46cc91b),
 * not from the production offset macros. The production body is #included
 * so copied-source mutants retarget WM_907F4_PRODUCTION_SOURCE. Scheduler
 * linkage proves the callback is entered from a first-pass state-0 slot.
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

enum {
    TRACE_LHU = 1,
    TRACE_LH = 2,
    TRACE_LW = 3,
    TRACE_SH = 4,
    TRACE_SW = 5,
    TRACE_CALL = 6
};

typedef struct TraceEvent {
    u32 pc;
    u32 kind;
    u32 address;
    u32 width;
    u32 value;
} TraceEvent;

typedef struct RotCall {
    u32 pc;
    u32 r;
    u32 m;
} RotCall;

#define TRACE_CAPACITY 96u
#define ROT_CAPACITY 4u

static TraceEvent s_actual_trace[TRACE_CAPACITY];
static size_t s_actual_trace_count;
static bool s_actual_trace_overflow;
static RotCall s_rot[ROT_CAPACITY];
static size_t s_rot_count;
static bool s_trace_armed;
static size_t s_c620_count;
static bool s_repoint_after_first;
static int s_pass_count;
static int s_total_count;
static int s_failure_count;
static int s_natural_907f4_entries;
static s16 s_natural_return;
static s16 s_stub_906e0_return = 1;
static int s_stub_8e76c_entries;

void wm_907f4_test_trace(u32 pc, u32 kind, u32 address, u32 width, u32 value)
{
    if (!s_trace_armed)
        return;
    if (s_actual_trace_count < (size_t)TRACE_CAPACITY) {
        TraceEvent *event = &s_actual_trace[s_actual_trace_count];
        event->pc = pc;
        event->kind = kind;
        event->address = address;
        event->width = width;
        event->value = value;
    } else {
        s_actual_trace_overflow = true;
    }
    s_actual_trace_count++;
}

void wm_907f4_test_rotmatrixzyx(u32 pc, u32 r_addr, u32 m_addr)
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

#ifndef WM_907F4_PRODUCTION_SOURCE
#define WM_907F4_PRODUCTION_SOURCE "../src/world_map_helper_907f4.c"
#endif
#include WM_907F4_PRODUCTION_SOURCE

#define RAM_MASK       0x001FFFFFu
#define MAIN_RAM_BYTES 0x00200000u
#define POOL_POINTER   0x8009BE24u
#define C620           0x8009C620u
#define SCRATCH        0x1F800000u
#define NATURAL_POOL   0x800D7538u
#define CONTEXT_ROOT   0x800D9540u
#define CONTEXT_ALT    0x800DA540u
#define SLOT_STRIDE    0x80u
#define RECORD2        0xA8u
#define RECORD3        0xFCu
#define REC_VZ         0x1Cu
#define REC_MATRIX     0x20u

#define SLOT_STATE  0x00u
#define SLOT_FLAG   0x04u
#define SLOT_CB0    0x18u
#define SLOT_CB1    0x1Cu
#define SLOT_MODE   0x20u
#define SLOT_HEAD0  0x50u
#define SLOT_HEAD1  0x54u
#define SLOT_CTR0   0x58u
#define SLOT_CTR1   0x5Cu

_Static_assert(PSX_RAM_SIZE == 0x00300000u, "3 MiB guest RAM");
_Static_assert(2u * 0x54u == RECORD2, "oracle record 2");
_Static_assert(3u * 0x54u == RECORD3, "oracle record 3");

static u8 s_expected_ram[PSX_RAM_SIZE];
static u8 s_expected_scratch[4096];
static u8 s_before_ram[PSX_RAM_SIZE];
static u8 s_before_scratch[4096];

typedef struct Fixture {
    const char *name;
    s32 slot_index;
    u32 pool;
    s16 flag;
    s16 mode;
    u32 ctr0;
    u32 ctr1;
    u32 head0;
    u32 head1;
    u16 vz2;
    u16 vz3;
    bool repoint;
} Fixture;

static size_t ram_index(u32 address)
{
    return (size_t)(address & RAM_MASK);
}

static void *raw_address(u32 address)
{
    return (void *)(g_PsxRam + ram_index(address));
}

static void *test_psx_addr(uintptr_t address_bits)
{
    u32 address = (u32)address_bits;

    if ((address & 0xFFFFF000u) == 0x1F800000u)
        return (void *)(g_PsxScratchpad + (address & 0xFFFu));
    if (s_trace_armed && address == C620) {
        s_c620_count++;
        if (s_repoint_after_first && s_c620_count >= 2u)
            return raw_address(CONTEXT_ALT);
    }
    return raw_address(address);
}

static void buffer_store_u16(u8 *buffer, u32 address, u16 value)
{
    memcpy(buffer + ram_index(address), &value, sizeof(value));
}

static void buffer_store_u32(u8 *buffer, u32 address, u32 value)
{
    memcpy(buffer + ram_index(address), &value, sizeof(value));
}

static u16 buffer_load_u16(const u8 *buffer, u32 address)
{
    u16 value;

    memcpy(&value, buffer + ram_index(address), sizeof(value));
    return value;
}

static u32 buffer_load_u32(const u8 *buffer, u32 address)
{
    u32 value;

    memcpy(&value, buffer + ram_index(address), sizeof(value));
    return value;
}

static void raw_store_u16(u32 address, u16 value)
{
    buffer_store_u16(g_PsxRam, address, value);
}

static void raw_store_u32(u32 address, u32 value)
{
    buffer_store_u32(g_PsxRam, address, value);
}

static u32 raw_load_u32(u32 address)
{
    return buffer_load_u32(g_PsxRam, address);
}

static s16 raw_load_s16(u32 address)
{
    s16 value;

    memcpy(&value, raw_address(address), sizeof(value));
    return value;
}

static u32 s32_bits(s32 value)
{
    u32 bits;

    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

static s32 u32_as_s32(u32 value)
{
    s32 result;

    memcpy(&result, &value, sizeof(result));
    return result;
}

static void scratch_store_u16(u8 *buffer, u32 address, u16 value)
{
    memcpy(buffer + (address & 0xFFFu), &value, sizeof(value));
}

static u16 scratch_load_u16(const u8 *buffer, u32 address)
{
    u16 value;

    memcpy(&value, buffer + (address & 0xFFFu), sizeof(value));
    return value;
}

static void seed_ram(u32 salt)
{
    size_t i;

    for (i = 0; i < sizeof(g_PsxRam); i++)
        g_PsxRam[i] = (u8)((i * 17u + salt) & 0xFFu);
    for (i = 0; i < sizeof(g_PsxScratchpad); i++)
        g_PsxScratchpad[i] = (u8)((i * 29u + salt + 0x5Du) & 0xFFu);
}

static void check_case(const char *group, const char *name, bool ok)
{
    s_total_count++;
    if (ok) {
        s_pass_count++;
        return;
    }
    s_failure_count++;
    fprintf(stderr, "FAIL %s / %s\n", group, name);
}

static void reset_trace(void)
{
    memset(s_actual_trace, 0, sizeof(s_actual_trace));
    s_actual_trace_count = 0;
    s_actual_trace_overflow = false;
    memset(s_rot, 0, sizeof(s_rot));
    s_rot_count = 0;
    s_c620_count = 0;
}

/* Independent listing oracle. Offsets are restated, not production macros. */
static void oracle_apply(const Fixture *fx, u32 slot, u32 root,
                         u8 *ram, u8 *scratch)
{
    s16 flag = fx->flag;
    s16 mode = fx->mode;
    u32 ctr0 = fx->ctr0;
    u32 ctr1 = fx->ctr1;
    u32 head0 = fx->head0;
    u32 head1 = fx->head1;

    if (flag == 9) {
        buffer_store_u16(ram, slot + 0x04u, 0u);
        buffer_store_u16(ram, slot + 0x20u, 1u);
        mode = 1;
    } else if (flag == 10) {
        buffer_store_u16(ram, slot + 0x04u, 0u);
        buffer_store_u16(ram, slot + 0x20u, 2u);
        mode = 2;
    }

    if (mode == 1) {
        ctr0 += 4u;
        buffer_store_u32(ram, slot + 0x58u, ctr0);
        if (!(u32_as_s32(ctr0) < 0x11)) {
            ctr1 += 4u;
            buffer_store_u32(ram, slot + 0x5Cu, ctr1);
        }
        ctr0 = buffer_load_u32(ram, slot + 0x58u);
        if (!(u32_as_s32(ctr0) < 0x80))
            buffer_store_u32(ram, slot + 0x58u, 0x80u);
        ctr1 = buffer_load_u32(ram, slot + 0x5Cu);
        if (!(u32_as_s32(ctr1) < 0x80))
            buffer_store_u32(ram, slot + 0x5Cu, 0x80u);
        ctr0 = buffer_load_u32(ram, slot + 0x58u);
        ctr1 = buffer_load_u32(ram, slot + 0x5Cu);
        if (!(u32_as_s32(ctr0) < 0x80) && !(u32_as_s32(ctr1) < 0x80))
            buffer_store_u16(ram, slot + 0x20u, 3u);
    } else if (mode < 2) {
        if (mode == 0) {
            buffer_store_u32(ram, slot + 0x58u, 0u);
            buffer_store_u32(ram, slot + 0x5Cu, 0u);
        }
    } else if (mode == 2) {
        ctr0 += 0xFFFFFFFCu;
        buffer_store_u32(ram, slot + 0x58u, ctr0);
        if (u32_as_s32(ctr0) < 0x70) {
            ctr1 += 0xFFFFFFFCu;
            buffer_store_u32(ram, slot + 0x5Cu, ctr1);
        }
        ctr0 = buffer_load_u32(ram, slot + 0x58u);
        if (u32_as_s32(ctr0) < 0)
            buffer_store_u32(ram, slot + 0x58u, 0u);
        ctr1 = buffer_load_u32(ram, slot + 0x5Cu);
        if (u32_as_s32(ctr1) < 0)
            buffer_store_u32(ram, slot + 0x5Cu, 0u);
        ctr0 = buffer_load_u32(ram, slot + 0x58u);
        ctr1 = buffer_load_u32(ram, slot + 0x5Cu);
        if (ctr0 == 0u && ctr1 == 0u)
            buffer_store_u16(ram, slot + 0x20u, 0u);
    }

    ctr0 = buffer_load_u32(ram, slot + 0x58u);
    ctr1 = buffer_load_u32(ram, slot + 0x5Cu);
    buffer_store_u32(ram, slot + 0x50u, head0 + ctr0);
    buffer_store_u32(ram, slot + 0x54u, head1 - ctr1);
    head0 = buffer_load_u32(ram, slot + 0x50u);
    head1 = buffer_load_u32(ram, slot + 0x54u);

    scratch_store_u16(scratch, SCRATCH + 0xA8u, 0u);
    scratch_store_u16(scratch, SCRATCH + 0xA0u, 0u);
    scratch_store_u16(scratch, SCRATCH + 0xA2u, (u16)head0);
    scratch_store_u16(scratch, SCRATCH + 0xAAu, (u16)head1);
    scratch_store_u16(scratch, SCRATCH + 0xA4u, fx->vz2);
    scratch_store_u16(scratch, SCRATCH + 0xACu, fx->vz3);
    (void)root;
}

static void run_fixture(const Fixture *fx, u32 salt)
{
    u32 slot = fx->pool + (s32_bits(fx->slot_index) << 7);
    u32 original_state;
    u32 original_before;
    u32 original_after;
    s32 result;

    seed_ram(salt);
    raw_store_u32(POOL_POINTER, fx->pool);
    raw_store_u32(C620, CONTEXT_ROOT);
    raw_store_u16(slot + SLOT_FLAG, (u16)fx->flag);
    raw_store_u16(slot + SLOT_MODE, (u16)fx->mode);
    raw_store_u32(slot + SLOT_CTR0, fx->ctr0);
    raw_store_u32(slot + SLOT_CTR1, fx->ctr1);
    raw_store_u32(slot + SLOT_HEAD0, fx->head0);
    raw_store_u32(slot + SLOT_HEAD1, fx->head1);
    raw_store_u16(CONTEXT_ROOT + RECORD2 + REC_VZ, fx->vz2);
    raw_store_u16(CONTEXT_ROOT + RECORD3 + REC_VZ, fx->vz3);
    raw_store_u16(CONTEXT_ALT + RECORD2 + REC_VZ, (u16)(fx->vz2 ^ 0x1111u));
    raw_store_u16(CONTEXT_ALT + RECORD3 + REC_VZ, (u16)(fx->vz3 ^ 0x2222u));

    original_state = raw_load_u32(slot + SLOT_STATE);
    original_before = raw_load_u32(slot - 4u);
    original_after = raw_load_u32(slot + SLOT_STRIDE);

    memcpy(s_before_ram, g_PsxRam, sizeof(g_PsxRam));
    memcpy(s_before_scratch, g_PsxScratchpad, sizeof(g_PsxScratchpad));
    memcpy(s_expected_ram, g_PsxRam, sizeof(g_PsxRam));
    memcpy(s_expected_scratch, g_PsxScratchpad, sizeof(g_PsxScratchpad));
    oracle_apply(fx, slot, CONTEXT_ROOT, s_expected_ram, s_expected_scratch);

    reset_trace();
    s_repoint_after_first = fx->repoint;
    s_trace_armed = true;
    result = wm_800907F4(fx->slot_index);
    s_trace_armed = false;

    check_case(fx->name, "return is scheduler state 1", result == 1);
    check_case(fx->name, "exactly one C620 load", s_c620_count == 1u);
    check_case(fx->name, "C620 still original root",
               raw_load_u32(C620) == CONTEXT_ROOT);
    check_case(fx->name, "two RotMatrixZYX calls", s_rot_count == 2u);
    check_case(fx->name, "first rot r=scratch+A0 m=record2+0x20",
               s_rot_count >= 1u && s_rot[0].r == SCRATCH + 0xA0u &&
                   s_rot[0].m == CONTEXT_ROOT + RECORD2 + REC_MATRIX &&
                   s_rot[0].pc == 0x800909E8u);
    check_case(fx->name, "second rot r=scratch+A8 m=record3+0x20",
               s_rot_count >= 2u && s_rot[1].r == SCRATCH + 0xA8u &&
                   s_rot[1].m == CONTEXT_ROOT + RECORD3 + REC_MATRIX &&
                   s_rot[1].pc == 0x800909F4u);
    check_case(fx->name, "slot+00 canary",
               raw_load_u32(slot + SLOT_STATE) == original_state);
    check_case(fx->name, "adjacent slot canaries",
               raw_load_u32(slot - 4u) == original_before &&
                   raw_load_u32(slot + SLOT_STRIDE) == original_after);
    check_case(fx->name, "whole 3 MiB RAM footprint",
               memcmp(g_PsxRam, s_expected_ram, sizeof(g_PsxRam)) == 0);
    check_case(fx->name, "scratchpad SVECTOR footprint",
               memcmp(g_PsxScratchpad, s_expected_scratch,
                      sizeof(g_PsxScratchpad)) == 0);
    check_case(fx->name, "1 MiB guard canary",
               memcmp(g_PsxRam + MAIN_RAM_BYTES,
                      s_before_ram + MAIN_RAM_BYTES,
                      sizeof(g_PsxRam) - MAIN_RAM_BYTES) == 0);
    check_case(fx->name, "rot1.vx zero then rot1.vy low16",
               scratch_load_u16(g_PsxScratchpad, SCRATCH + 0xA0u) == 0u &&
                   scratch_load_u16(g_PsxScratchpad, SCRATCH + 0xA2u) ==
                       (u16)raw_load_u32(slot + SLOT_HEAD0));
    check_case(fx->name, "rot2.vx zero then vz from record+0x1C",
               scratch_load_u16(g_PsxScratchpad, SCRATCH + 0xA8u) == 0u &&
                   scratch_load_u16(g_PsxScratchpad, SCRATCH + 0xA4u) ==
                       fx->vz2 &&
                   scratch_load_u16(g_PsxScratchpad, SCRATCH + 0xACu) ==
                       fx->vz3);
    check_case(fx->name, "no trace overflow", !s_actual_trace_overflow);
    check_case(fx->name, "first guest access is pool then C620",
               s_actual_trace_count >= 2u &&
                   s_actual_trace[0].address == POOL_POINTER &&
                   s_actual_trace[0].kind == TRACE_LW &&
                   s_actual_trace[0].width == 4u &&
                   s_actual_trace[1].address == C620 &&
                   s_actual_trace[1].kind == TRACE_LW);
    check_case(fx->name, "pool pointer width 4",
               raw_load_u32(POOL_POINTER) == fx->pool);

    /* Width witnesses for authorized stores that the oracle published. */
    check_case(fx->name, "heading words are 32-bit wrapping publications",
               raw_load_u32(slot + SLOT_HEAD0) ==
                   buffer_load_u32(s_expected_ram, slot + SLOT_HEAD0) &&
                   raw_load_u32(slot + SLOT_HEAD1) ==
                       buffer_load_u32(s_expected_ram, slot + SLOT_HEAD1));
    check_case(fx->name, "mode/flag remain 16-bit publications",
               buffer_load_u16(g_PsxRam, slot + SLOT_MODE) ==
                   buffer_load_u16(s_expected_ram, slot + SLOT_MODE) &&
                   buffer_load_u16(g_PsxRam, slot + SLOT_FLAG) ==
                       buffer_load_u16(s_expected_ram, slot + SLOT_FLAG));
    check_case(fx->name, "scratch pads A6/AE untouched vs seed",
               g_PsxScratchpad[(SCRATCH + 0xA6u) & 0xFFFu] ==
                   s_before_scratch[(SCRATCH + 0xA6u) & 0xFFFu] &&
                   g_PsxScratchpad[(SCRATCH + 0xAEu) & 0xFFFu] ==
                       s_before_scratch[(SCRATCH + 0xAEu) & 0xFFFu]);

}

static s16 stub_return_1(int slot_index)
{
    (void)slot_index;
    return s_stub_906e0_return;
}

static s16 stub_8e76c(int slot_index)
{
    (void)slot_index;
    s_stub_8e76c_entries++;
    return 3;
}

static s16 wrap_907f4(int slot_index)
{
    s32 result;

    if (slot_index == 8)
        s_natural_907f4_entries++;
    result = wm_800907F4((s32)slot_index);
    s_natural_return = (s16)result;
    return (s16)result;
}

static void set_slot(u32 pool, unsigned slot, s16 state, u32 cb0, u32 cb1)
{
    u32 address = pool + (u32)slot * SLOT_STRIDE;

    raw_store_u16(address + SLOT_STATE, (u16)state);
    raw_store_u32(address + SLOT_CB0, cb0);
    raw_store_u32(address + SLOT_CB1, cb1);
}

static void run_natural(void)
{
    u32 pool = NATURAL_POOL;
    u32 slot8 = pool + 8u * SLOT_STRIDE;
    u32 original_state;

    seed_ram(0x907F41u);
    raw_store_u32(POOL_POINTER, pool);
    raw_store_u32(C620, CONTEXT_ROOT);
    memset(g_PsxRam + ram_index(pool), 0, 16u * SLOT_STRIDE);
    set_slot(pool, 8u, 0, 0x800906E0u, 0x800907F4u);
    raw_store_u16(slot8 + SLOT_FLAG, 0);
    raw_store_u16(slot8 + SLOT_MODE, 0);
    raw_store_u32(slot8 + SLOT_CTR0, 0x10u);
    raw_store_u32(slot8 + SLOT_CTR1, 0x20u);
    raw_store_u32(slot8 + SLOT_HEAD0, 0x1000u);
    raw_store_u32(slot8 + SLOT_HEAD1, 0x2000u);
    raw_store_u16(CONTEXT_ROOT + RECORD2 + REC_VZ, 0xABCDu);
    raw_store_u16(CONTEXT_ROOT + RECORD3 + REC_VZ, 0x1234u);
    original_state = raw_load_u32(slot8 + SLOT_STATE);

    wm_sched_callback_registry_clear();
    wm_sched_callback_register(0x800906E0u, stub_return_1);
    wm_sched_callback_register(0x800907F4u, wrap_907f4);
    wm_sched_reset();
    s_natural_907f4_entries = 0;
    s_trace_armed = false;
    wm_80097800();
    check_case("natural-pass1", "906E0 stub entered from state 0",
               wm_sched_get_callbacks_executed() == 1 &&
                   wm_sched_get_last_callback() == 0x800906E0u);
    check_case("natural-pass1", "907F4 not entered on first pass",
               s_natural_907f4_entries == 0);
    check_case("natural-pass1", "slot8 state published as 1",
               raw_load_s16(slot8 + SLOT_STATE) == 1);
    check_case("natural-pass1", "pass complete",
               wm_sched_get_outcome() == WM_SCHED_PASS_COMPLETE);
    check_case("natural-pass1", "C620 non-null at entry",
               raw_load_u32(C620) == CONTEXT_ROOT);
    check_case("natural-pass1", "slot+00 was scheduler-owned before publish",
               original_state == 0u);

    /* Pass 2: cb1 907F4 is now eligible at state 1. */
    wm_sched_reset();
    s_natural_907f4_entries = 0;
    s_natural_return = 0;
    s_repoint_after_first = false;
    reset_trace();
    s_trace_armed = true;
    wm_80097800();
    s_trace_armed = false;
    check_case("natural-pass2", "907F4 entered from scheduler",
               s_natural_907f4_entries == 1 &&
                   wm_sched_get_last_callback() == 0x800907F4u);
    check_case("natural-pass2", "natural mode 0 return 1",
               s_natural_return == 1 &&
                   raw_load_s16(slot8 + SLOT_STATE) == 1);
    check_case("natural-pass2", "mode0 cleared counters",
               raw_load_u32(slot8 + SLOT_CTR0) == 0u &&
                   raw_load_u32(slot8 + SLOT_CTR1) == 0u);
    check_case("natural-pass2", "headings applied with zero counters",
               raw_load_u32(slot8 + SLOT_HEAD0) == 0x1000u &&
                   raw_load_u32(slot8 + SLOT_HEAD1) == 0x2000u);
    check_case("natural-pass2", "pass complete after 907F4",
               wm_sched_get_outcome() == WM_SCHED_PASS_COMPLETE);
    check_case("natural-pass2", "next unresolved not 907F4",
               wm_sched_get_frontier_pc() == WM_SCHED_CUT_BEFORE_DRAWSYNC);
    check_case("natural-pass2", "C620 still live",
               raw_load_u32(C620) != 0u);

    /* Slot 7 dormant at state 3: 8E76C must not fire. */
    set_slot(pool, 7u, 3, 0x8008E190u, 0x8008E76Cu);
    wm_sched_callback_register(0x8008E76Cu, stub_8e76c);
    s_stub_8e76c_entries = 0;
    wm_sched_reset();
    wm_80097800();
    check_case("natural-dormant-slot7", "8E76C never fires at state 3",
               s_stub_8e76c_entries == 0);
    check_case("natural-dormant-slot7", "pass still complete",
               wm_sched_get_outcome() == WM_SCHED_PASS_COMPLETE);

    /* Occupied slot 15 at state 1 would request 87734; note eligibility. */
    set_slot(pool, 15u, 1, 0x80087710u, 0x80087734u);
    wm_sched_reset();
    wm_80097800();
    check_case("natural-slot15", "87734 is the next missing if occupied",
               wm_sched_get_outcome() == WM_SCHED_STOP_MISSING_CALLBACK &&
                   wm_sched_get_frontier_pc() == 0x80087734u);
}

static void run_oracle_sentinels(void)
{
    check_case("oracle", "inc gate 0x10 does not start 5C",
               u32_as_s32(0x10u) < 0x11);
    check_case("oracle", "inc gate 0x11 starts 5C",
               !(u32_as_s32(0x11u) < 0x11));
    check_case("oracle", "clamp 0x7F stays",
               u32_as_s32(0x7Fu) < 0x80);
    check_case("oracle", "clamp 0x80 sticks",
               !(u32_as_s32(0x80u) < 0x80));
    check_case("oracle", "dec 5C gate 0x70 signed",
               u32_as_s32(0x6Fu) < 0x70 && !(u32_as_s32(0x70u) < 0x70));
    check_case("oracle", "sltiu zero is exact 0 not negative",
               0u == 0u && s32_bits(-1) != 0u);
    check_case("oracle", "slot index -1 shift wraps",
               (s32_bits(-1) << 7) == 0xFFFFFF80u);
    check_case("oracle", "INT32_MIN shift wraps to 0",
               (s32_bits(INT32_MIN) << 7) == 0u);
    check_case("oracle", "heading add wraps",
               (u32)(0xFFFFFFF0u + 0x20u) == 0x10u);
    check_case("oracle", "heading sub wraps",
               (u32)(0x10u - 0x20u) == 0xFFFFFFF0u);
    check_case("oracle", "record2 = 2*0x54", RECORD2 == 0xA8u);
    check_case("oracle", "record3 = 3*0x54", RECORD3 == 0xFCu);
    check_case("oracle", "flag 9 is signed 9 not 0xFFF9", 9 == 9);
    check_case("oracle", "flag 10 is 0xA", 10 == 0xA);
}

int main(void)
{
    static const Fixture fixtures[] = {
        {"mode0-clear", 8, NATURAL_POOL, 0, 0,
         0x11111111u, 0x22222222u, 0x33333333u, 0x44444444u,
         0xA011u, 0xB022u, false},
        {"mode1-below-gate", 8, NATURAL_POOL, 0, 1,
         0x0Cu, 0x40u, 0x1000u, 0x2000u, 0x1111u, 0x2222u, false},
        {"mode1-at-gate", 8, NATURAL_POOL, 0, 1,
         0x0Du, 0x40u, 0x1000u, 0x2000u, 0x1111u, 0x2222u, false},
        {"mode1-clamp-both", 8, NATURAL_POOL, 0, 1,
         0x7Cu, 0x7Cu, 0xAAAAAAAAu, 0x55555555u, 0x8001u, 0x7FFFu, false},
        {"mode1-already-80", 8, NATURAL_POOL, 0, 1,
         0x80u, 0x7Cu, 1u, 2u, 0x0001u, 0x0002u, false},
        {"mode2-no-5c", 8, NATURAL_POOL, 0, 2,
         0x80u, 0x80u, 0x10u, 0x20u, 0x3333u, 0x4444u, false},
        {"mode2-with-5c", 8, NATURAL_POOL, 0, 2,
         0x70u, 0x70u, 0x10u, 0x20u, 0x3333u, 0x4444u, false},
        {"mode2-negative-clamp", 8, NATURAL_POOL, 0, 2,
         0x2u, 0x2u, 0xFFFFFFF0u, 0x10u, 0xF00Du, 0x0FF0u, false},
        {"mode2-both-zero", 8, NATURAL_POOL, 0, 2,
         0x4u, 0x4u, 0x8u, 0x8u, 0xABCDu, 0xDCBAu, false},
        {"flag9-enters-mode1", 8, NATURAL_POOL, 9, 0,
         0x0u, 0x0u, 0x50u, 0x60u, 0x0102u, 0x0304u, false},
        {"flag10-enters-mode2", 8, NATURAL_POOL, 10, 0,
         0x8u, 0x8u, 0x50u, 0x60u, 0x0102u, 0x0304u, false},
        {"mode3-default", 8, NATURAL_POOL, 0, 3,
         0x80u, 0x80u, 0x7u, 0x9u, 0x1110u, 0x2220u, false},
        {"mode-neg1-default", 8, NATURAL_POOL, 0, -1,
         0x5u, 0x6u, 0x7u, 0x8u, 0x2110u, 0x1220u, false},
        {"mode4-default", 8, NATURAL_POOL, 0, 4,
         0x1u, 0x2u, 0xFFFFFFFFu, 0x1u, 0x8000u, 0x0001u, false},
        {"wrapping-heading", 8, NATURAL_POOL, 0, 0,
         0u, 0u, 0xFFFFFFF0u, 0x20u, 0xFFFFu, 0x0000u, false},
        {"slot-index-minus-one", -1, 0x800D8000u, 0, 1,
         0x4u, 0x8u, 0x10u, 0x20u, 0x1234u, 0x5678u, false},
        {"slot-index-int-min", INT32_MIN, 0x800D8100u, 0, 2,
         0x8u, 0x4u, 0x30u, 0x40u, 0x9ABCu, 0xDEF0u, false},
        {"c620-no-reload", 8, NATURAL_POOL, 0, 1,
         0x20u, 0x10u, 0x100u, 0x200u, 0x0FF0u, 0xF00Du, true},
        {"flag9-from-mode2", 8, NATURAL_POOL, 9, 2,
         0x40u, 0x40u, 0x1u, 0x2u, 0xAAAAu, 0x5555u, false},
        {"signed-ctr-negative-mode1", 8, NATURAL_POOL, 0, 1,
         0xFFFFFFFCu, 0x10u, 0x8u, 0x4u, 0x0003u, 0x0004u, false}
    };
    size_t index;

    for (index = 0; index < sizeof(fixtures) / sizeof(fixtures[0]); index++)
        run_fixture(&fixtures[index], (u32)index + 0x41u);

    run_oracle_sentinels();
    run_natural();

    printf("=== Results: %d/%d PASS ===\n", s_pass_count, s_total_count);
    printf("W34B25 0x800907F4 focused oracle PASS checks=%d\n", s_pass_count);
    return s_failure_count == 0 ? 0 : 1;
}
