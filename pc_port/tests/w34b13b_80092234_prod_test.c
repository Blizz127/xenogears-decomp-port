/*
 * W34B13-B production-linked retail certificate for callback 0x80092234.
 *
 * Expected paths, PCs, typed guest accesses, values, ordering, and wrapping
 * arithmetic are declared below from the fresh retail slice. No production
 * constants or control-flow helpers are used as oracle authority.
 */
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"

enum {
    TRACE_LW = 1,
    TRACE_SH = 2,
    TRACE_SW = 3
};

typedef struct TraceEvent {
    u32 pc;
    u32 kind;
    u32 address;
    u32 width;
    u32 value;
} TraceEvent;

#define TRACE_CAPACITY 16u
#define RAM_MASK       0x001FFFFFu
#define MAIN_RAM_BYTES 0x00200000u

#define POOL_POINTER 0x8009BE24u
#define MODE_GLOBAL  0x8009BE10u
#define SCALAR_GLOBAL 0x8009BE0Cu
#define C620_GLOBAL  0x8009C620u

#define NATURAL_POOL 0x800D7538u
#define SLOT_STRIDE  0x80u
#define SLOT_STATE   0x00u
#define SLOT_CONTROL 0x20u
#define SLOT_VALUE   0x50u

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

static TraceEvent s_actual_trace[TRACE_CAPACITY];
static size_t s_actual_trace_count;
static bool s_actual_trace_overflow;
static bool s_injection_enabled;
static bool s_injection_happened;
static u32 s_inject_after_pc;
static u32 s_inject_value;

static size_t ram_index(u32 address)
{
    return (size_t)(address & RAM_MASK);
}

static void direct_store_u32(u32 address, u32 value)
{
    memcpy(g_PsxRam + ram_index(address), &value, sizeof(value));
}

void wm_92234_test_trace(u32 pc, u32 kind, u32 address, u32 width,
                         u32 value)
{
    if (s_actual_trace_count < (size_t)TRACE_CAPACITY) {
        TraceEvent* event = &s_actual_trace[s_actual_trace_count];
        event->pc = pc;
        event->kind = kind;
        event->address = address;
        event->width = width;
        event->value = value;
    } else {
        s_actual_trace_overflow = true;
    }
    s_actual_trace_count++;

    /* Test-only intervention after a declared retail access. It is deliberately
     * untraced and is modeled separately in the expected RAM postimage. */
    if (s_injection_enabled && !s_injection_happened &&
        pc == s_inject_after_pc) {
        direct_store_u32(SCALAR_GLOBAL, s_inject_value);
        s_injection_happened = true;
    }
}

#define WM_92234_TEST_TRACE 1
#ifndef WM_92234_PRODUCTION_SOURCE
#define WM_92234_PRODUCTION_SOURCE "../src/world_map_callback_92234.c"
#endif
#include WM_92234_PRODUCTION_SOURCE

typedef enum RetailPath {
    RETAIL_OUTER,
    RETAIL_ARM_1_5,
    RETAIL_ARM_6_7
} RetailPath;

typedef struct Fixture {
    const char* name;
    s32 slot_index;
    u32 pool;
    s32 mode;
    u32 initial_scalar;
    bool inject;
    u32 inject_after_pc;
    u32 inject_value;
} Fixture;

typedef struct Oracle {
    u8 memory[PSX_RAM_SIZE];
    TraceEvent trace[TRACE_CAPACITY];
    size_t trace_count;
    bool injection_happened;
} Oracle;

static Oracle s_oracle;
static u8 s_before_ram[PSX_RAM_SIZE];
static u8 s_before_scratch[sizeof(g_PsxScratchpad)];
static int s_pass_count;
static int s_total_count;
static int s_failure_count;

static u16 memory_load_u16(const u8* memory, u32 address)
{
    u16 value;
    memcpy(&value, memory + ram_index(address), sizeof(value));
    return value;
}

static u32 memory_load_u32(const u8* memory, u32 address)
{
    u32 value;
    memcpy(&value, memory + ram_index(address), sizeof(value));
    return value;
}

static void memory_store_u16(u8* memory, u32 address, u16 value)
{
    memcpy(memory + ram_index(address), &value, sizeof(value));
}

static void memory_store_u32(u8* memory, u32 address, u32 value)
{
    memcpy(memory + ram_index(address), &value, sizeof(value));
}

static u16 raw_load_u16(u32 address)
{
    return memory_load_u16(g_PsxRam, address);
}

static u32 raw_load_u32(u32 address)
{
    return memory_load_u32(g_PsxRam, address);
}

static void raw_store_u32(u32 address, u32 value)
{
    memory_store_u32(g_PsxRam, address, value);
}

static u32 s32_bits(s32 value)
{
    u32 result;
    memcpy(&result, &value, sizeof(result));
    return result;
}

static RetailPath retail_path(s32 mode)
{
    if (mode >= 1 && mode <= 5)
        return RETAIL_ARM_1_5;
    if (mode >= 6 && mode <= 7)
        return RETAIL_ARM_6_7;
    return RETAIL_OUTER;
}

static void check_case(const char* fixture, const char* property, bool ok)
{
    s_total_count++;
    if (ok) {
        s_pass_count++;
    } else {
        s_failure_count++;
        printf("FAIL [%s]: %s\n", fixture, property);
    }
}

static const char* kind_name(u32 kind)
{
    switch (kind) {
    case TRACE_LW: return "LW";
    case TRACE_SH: return "SH";
    case TRACE_SW: return "SW";
    default: return "UNKNOWN";
    }
}

static void oracle_event(u32 pc, u32 kind, u32 address, u32 width,
                         u32 value)
{
    TraceEvent* event = &s_oracle.trace[s_oracle.trace_count];
    event->pc = pc;
    event->kind = kind;
    event->address = address;
    event->width = width;
    event->value = value;
    s_oracle.trace_count++;
}

static void oracle_maybe_inject(const Fixture* fixture, u32 pc)
{
    if (fixture->inject && !s_oracle.injection_happened &&
        fixture->inject_after_pc == pc) {
        memory_store_u32(s_oracle.memory, SCALAR_GLOBAL,
                         fixture->inject_value);
        s_oracle.injection_happened = true;
    }
}

static u32 oracle_lw(const Fixture* fixture, u32 pc, u32 address)
{
    u32 value = memory_load_u32(s_oracle.memory, address);
    oracle_event(pc, TRACE_LW, address, 4u, value);
    oracle_maybe_inject(fixture, pc);
    return value;
}

static void oracle_sh(const Fixture* fixture, u32 pc, u32 address, u16 value)
{
    memory_store_u16(s_oracle.memory, address, value);
    oracle_event(pc, TRACE_SH, address, 2u, (u32)value);
    oracle_maybe_inject(fixture, pc);
}

static void oracle_sw(const Fixture* fixture, u32 pc, u32 address, u32 value)
{
    memory_store_u32(s_oracle.memory, address, value);
    oracle_event(pc, TRACE_SW, address, 4u, value);
    oracle_maybe_inject(fixture, pc);
}

static void seed_memory(u32 salt)
{
    size_t index;

    for (index = 0u; index < sizeof(g_PsxRam); index++) {
        u32 low = (u32)(index & 0xFFFFFFFFu);
        g_PsxRam[index] =
            (u8)((low * 43u + (low >> 9) + salt * 53u + 0x71u) & 0xFFu);
    }
    for (index = 0u; index < sizeof(g_PsxScratchpad); index++) {
        u32 low = (u32)index;
        g_PsxScratchpad[index] =
            (u8)((low * 29u + salt * 17u + 0xC3u) & 0xFFu);
    }
}

static void reset_actual_trace(const Fixture* fixture)
{
    memset(s_actual_trace, 0, sizeof(s_actual_trace));
    s_actual_trace_count = 0u;
    s_actual_trace_overflow = false;
    s_injection_enabled = fixture->inject;
    s_injection_happened = false;
    s_inject_after_pc = fixture->inject_after_pc;
    s_inject_value = fixture->inject_value;
}

static void build_oracle(const Fixture* fixture, u32 slot)
{
    RetailPath path = retail_path(fixture->mode);
    u32 scalar;

    memcpy(s_oracle.memory, g_PsxRam, sizeof(g_PsxRam));
    memset(s_oracle.trace, 0, sizeof(s_oracle.trace));
    s_oracle.trace_count = 0u;
    s_oracle.injection_happened = false;

    (void)oracle_lw(fixture, 0x8009223Cu, POOL_POINTER);
    (void)oracle_lw(fixture, 0x80092244u, MODE_GLOBAL);
    if (path == RETAIL_ARM_1_5) {
        oracle_sw(fixture, 0x80092264u, SCALAR_GLOBAL, 140u);
    } else if (path == RETAIL_ARM_6_7) {
        oracle_sw(fixture, 0x80092284u, SCALAR_GLOBAL, 120u);
        oracle_sh(fixture, 0x8009228Cu, slot + SLOT_CONTROL, 1u);
    }
    scalar = oracle_lw(fixture, 0x80092294u, SCALAR_GLOBAL);
    oracle_sw(fixture, 0x800922A0u, slot + SLOT_VALUE,
              scalar << 12);
}

static bool trace_matches(const char* fixture)
{
    size_t index;

    if (s_actual_trace_overflow ||
        s_actual_trace_count != s_oracle.trace_count) {
        printf("TRACE [%s]: actual=%zu expected=%zu overflow=%d\n",
               fixture, s_actual_trace_count, s_oracle.trace_count,
               s_actual_trace_overflow ? 1 : 0);
        return false;
    }
    for (index = 0u; index < s_oracle.trace_count; index++) {
        const TraceEvent* actual = &s_actual_trace[index];
        const TraceEvent* expected = &s_oracle.trace[index];
        if (actual->pc != expected->pc ||
            actual->kind != expected->kind ||
            actual->address != expected->address ||
            actual->width != expected->width ||
            actual->value != expected->value) {
            printf("TRACE [%s] #%zu: actual %08x %s %08x/%u=%08x; "
                   "expected %08x %s %08x/%u=%08x\n",
                   fixture, index,
                   (unsigned int)actual->pc,
                   kind_name(actual->kind),
                   (unsigned int)actual->address,
                   (unsigned int)actual->width,
                   (unsigned int)actual->value,
                   (unsigned int)expected->pc,
                   kind_name(expected->kind),
                   (unsigned int)expected->address,
                   (unsigned int)expected->width,
                   (unsigned int)expected->value);
            return false;
        }
    }
    return true;
}

static bool exact_widths(RetailPath path)
{
    size_t index;
    size_t expected_count = path == RETAIL_OUTER ? 4u :
                            path == RETAIL_ARM_1_5 ? 5u : 6u;

    if (s_actual_trace_count != expected_count)
        return false;
    for (index = 0u; index < s_actual_trace_count; index++) {
        const TraceEvent* event = &s_actual_trace[index];
        if (event->kind == TRACE_SH) {
            if (event->width != 2u)
                return false;
        } else if (event->kind == TRACE_LW || event->kind == TRACE_SW) {
            if (event->width != 4u)
                return false;
        } else {
            return false;
        }
    }
    return true;
}

static size_t fresh_scalar_read_count(void)
{
    size_t count = 0u;
    size_t index;

    for (index = 0u; index < s_actual_trace_count; index++) {
        const TraceEvent* event = &s_actual_trace[index];
        if (event->pc == 0x80092294u && event->kind == TRACE_LW &&
            event->address == SCALAR_GLOBAL && event->width == 4u)
            count++;
    }
    return count;
}

static u32 fresh_scalar_read_value(void)
{
    size_t index;

    for (index = 0u; index < s_actual_trace_count; index++) {
        const TraceEvent* event = &s_actual_trace[index];
        if (event->pc == 0x80092294u && event->kind == TRACE_LW &&
            event->address == SCALAR_GLOBAL)
            return event->value;
    }
    return 0xBAD0BAD0u;
}

static void run_fixture(const Fixture* fixture, u32 salt)
{
    RetailPath path = retail_path(fixture->mode);
    u32 slot = fixture->pool + (s32_bits(fixture->slot_index) << 7);
    u32 original_state;
    u16 original_control;
    u32 original_value;
    u32 original_before_slot;
    u32 original_after_slot;
    u32 original_c620;
    u32 expected_scalar;
    u32 expected_value;
    s32 result;

    seed_memory(salt);
    raw_store_u32(POOL_POINTER, fixture->pool);
    raw_store_u32(MODE_GLOBAL, s32_bits(fixture->mode));
    raw_store_u32(SCALAR_GLOBAL, fixture->initial_scalar);

    original_state = raw_load_u32(slot + SLOT_STATE);
    original_control = raw_load_u16(slot + SLOT_CONTROL);
    original_value = raw_load_u32(slot + SLOT_VALUE);
    original_before_slot = raw_load_u32(slot - SLOT_STRIDE);
    original_after_slot = raw_load_u32(slot + SLOT_STRIDE);
    original_c620 = raw_load_u32(C620_GLOBAL);

    memcpy(s_before_ram, g_PsxRam, sizeof(g_PsxRam));
    memcpy(s_before_scratch, g_PsxScratchpad, sizeof(g_PsxScratchpad));
    build_oracle(fixture, slot);
    reset_actual_trace(fixture);

    result = wm_80092234(fixture->slot_index);
    expected_scalar = memory_load_u32(s_oracle.memory, SCALAR_GLOBAL);
    expected_value = memory_load_u32(s_oracle.memory, slot + SLOT_VALUE);

    check_case(fixture->name, "return is scheduler state 1", result == 1);
    check_case(fixture->name, "exact typed guest access trace",
               trace_matches(fixture->name));
    check_case(fixture->name, "exact path access count and widths",
               exact_widths(path));
    check_case(fixture->name, "BE24 then signed BE10 are first reads",
               s_actual_trace_count >= 2u &&
                   s_actual_trace[0].pc == 0x8009223Cu &&
                   s_actual_trace[0].kind == TRACE_LW &&
                   s_actual_trace[0].address == POOL_POINTER &&
                   s_actual_trace[1].pc == 0x80092244u &&
                   s_actual_trace[1].kind == TRACE_LW &&
                   s_actual_trace[1].address == MODE_GLOBAL);
    check_case(fixture->name, "exactly one fresh BE0C read at 0x80092294",
               fresh_scalar_read_count() == 1u);
    check_case(fixture->name, "fresh BE0C read sees post-hook value",
               fresh_scalar_read_value() == expected_scalar);
    check_case(fixture->name, "test intervention occurrence is exact",
               s_injection_happened == fixture->inject &&
                   s_oracle.injection_happened == fixture->inject);
    check_case(fixture->name, "BE0C postimage matches retail plus hook",
               raw_load_u32(SCALAR_GLOBAL) == expected_scalar);
    check_case(fixture->name, "slot+50 uses wrapping fresh-value shift",
               raw_load_u32(slot + SLOT_VALUE) == expected_value);
    check_case(fixture->name, "slot+20 width/gating/value exact",
               path == RETAIL_ARM_6_7 ?
                   raw_load_u16(slot + SLOT_CONTROL) == 1u :
                   raw_load_u16(slot + SLOT_CONTROL) == original_control);
    check_case(fixture->name, "scheduler-owned slot+00 untouched",
               raw_load_u32(slot + SLOT_STATE) == original_state);
    check_case(fixture->name, "pool and mode globals remain unchanged",
               raw_load_u32(POOL_POINTER) == fixture->pool &&
                   raw_load_u32(MODE_GLOBAL) == s32_bits(fixture->mode));
    check_case(fixture->name, "C620 is untouched",
               raw_load_u32(C620_GLOBAL) == original_c620);
    check_case(fixture->name, "complete selected slot matches oracle",
               memcmp(g_PsxRam + ram_index(slot),
                      s_oracle.memory + ram_index(slot), SLOT_STRIDE) == 0);
    check_case(fixture->name, "adjacent slot canaries untouched",
               raw_load_u32(slot - SLOT_STRIDE) == original_before_slot &&
                   raw_load_u32(slot + SLOT_STRIDE) == original_after_slot);
    check_case(fixture->name, "whole 3 MiB callback plus hook footprint",
               memcmp(g_PsxRam, s_oracle.memory, sizeof(g_PsxRam)) == 0);
    check_case(fixture->name, "1 MiB guard canary untouched",
               memcmp(g_PsxRam + MAIN_RAM_BYTES,
                      s_before_ram + MAIN_RAM_BYTES,
                      sizeof(g_PsxRam) - MAIN_RAM_BYTES) == 0);
    check_case(fixture->name, "scratchpad untouched",
               memcmp(g_PsxScratchpad, s_before_scratch,
                      sizeof(g_PsxScratchpad)) == 0);

    /* Keep these values live in every build; they are also useful in mutant
     * failure diagnostics through the explicit checks above. */
    (void)original_value;
}

static void run_oracle_sentinels(void)
{
    check_case("oracle", "INT32_MIN outer",
               retail_path(INT32_MIN) == RETAIL_OUTER);
    check_case("oracle", "zero outer",
               retail_path(0) == RETAIL_OUTER);
    check_case("oracle", "mode1 begins first arm",
               retail_path(1) == RETAIL_ARM_1_5);
    check_case("oracle", "mode5 ends first arm",
               retail_path(5) == RETAIL_ARM_1_5);
    check_case("oracle", "mode6 begins second arm",
               retail_path(6) == RETAIL_ARM_6_7);
    check_case("oracle", "mode7 ends second arm",
               retail_path(7) == RETAIL_ARM_6_7);
    check_case("oracle", "mode8 outer",
               retail_path(8) == RETAIL_OUTER);
    check_case("oracle", "INT32_MAX outer",
               retail_path(INT32_MAX) == RETAIL_OUTER);
    check_case("oracle", "negative slot shift wraps",
               (s32_bits(-1) << 7) == 0xFFFFFF80u);
    check_case("oracle", "INT32_MIN slot shift wraps to zero",
               (s32_bits(INT32_MIN) << 7) == 0u);
    check_case("oracle", "outer high bits are discarded by SLL12",
               (0xABCDEF01u << 12) == 0xDEF01000u);
    check_case("oracle", "active no-hook reload is numerically redundant A",
               (140u << 12) == 0x0008C000u);
    check_case("oracle", "active no-hook reload is numerically redundant B",
               (120u << 12) == 0x00078000u);
}

int main(void)
{
    static const Fixture fixtures[] = {
        {"mode-int-min", 11, NATURAL_POOL, INT32_MIN,
         0xABCDEFFFu, false, 0u, 0u},
        {"mode-negative-two", 11, NATURAL_POOL, -2,
         0x00000001u, false, 0u, 0u},
        {"mode-negative-one", 11, NATURAL_POOL, -1,
         0x000FFFFFu, false, 0u, 0u},
        {"mode-zero-outer-no-hook", 11, NATURAL_POOL, 0,
         0xFFFFFFFFu, false, 0u, 0u},
        {"mode-one-arm-a-no-hook-equivalent-postimage", 11, NATURAL_POOL, 1,
         0xA1B2C3D4u, false, 0u, 0u},
        {"mode-two-arm-a", 11, NATURAL_POOL, 2,
         0x11223344u, false, 0u, 0u},
        {"mode-three-arm-a", 11, NATURAL_POOL, 3,
         0x55667788u, false, 0u, 0u},
        {"mode-four-arm-a", 11, NATURAL_POOL, 4,
         0x99AABBCCu, false, 0u, 0u},
        {"mode-five-arm-a-edge", 11, NATURAL_POOL, 5,
         0xDDEEFF00u, false, 0u, 0u},
        {"mode-six-arm-b-edge", 11, NATURAL_POOL, 6,
         0x13572468u, false, 0u, 0u},
        {"mode-seven-arm-b-no-hook-equivalent-postimage", 11, NATURAL_POOL, 7,
         0x89ABCDEFu, false, 0u, 0u},
        {"mode-eight-outer", 11, NATURAL_POOL, 8,
         0x80000000u, false, 0u, 0u},
        {"mode-nine-outer", 11, NATURAL_POOL, 9,
         0x00100000u, false, 0u, 0u},
        {"mode-int-max", 11, NATURAL_POOL, INT32_MAX,
         0x00010001u, false, 0u, 0u},
        {"outer-hook-after-mode-read", 11, NATURAL_POOL, 0,
         0xCAFEBABEu, true, 0x80092244u, 0x89ABCDEFu},
        {"arm-a-hook-after-be0c-write", 11, NATURAL_POOL, 1,
         0x0BADF00Du, true, 0x80092264u, 0x13579BDFu},
        {"arm-b-hook-after-be0c-write", 11, NATURAL_POOL, 7,
         0x10203040u, true, 0x80092284u, 0x2468ACE0u},
        {"slot-index-minus-one", -1, 0x800D8000u, 1,
         0x31415926u, false, 0u, 0u},
        {"slot-index-int-min", INT32_MIN, 0x800D8100u, 7,
         0x27182818u, false, 0u, 0u},
        {"slot-addition-wrap", 1, 0xFFFFFFF0u, 8,
         0xFEDCBA98u, false, 0u, 0u}
    };
    size_t index;

    for (index = 0u; index < sizeof(fixtures) / sizeof(fixtures[0]); index++)
        run_fixture(&fixtures[index], (u32)index + 0x61u);

    run_oracle_sentinels();
    printf("=== Results: %d/%d PASS ===\n", s_pass_count, s_total_count);
    return s_failure_count == 0 ? 0 : 1;
}
