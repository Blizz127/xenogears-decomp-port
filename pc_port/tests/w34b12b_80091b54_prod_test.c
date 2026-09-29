/*
 * W34B12-B production-linked retail certificate for callback 0x80091B54.
 *
 * The expected PCs, typed guest accesses, values, order, and signed path
 * partition below are a declarative transcription of fresh world_map.bin
 * bytes. They are not derived from production macros or control flow.
 */
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
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

static TraceEvent s_actual_trace[TRACE_CAPACITY];
static size_t s_actual_trace_count;
static bool s_actual_trace_overflow;

void wm_91b54_test_trace(u32 pc, u32 kind, u32 address, u32 width,
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
}

#define WM_91B54_TEST_TRACE 1
#ifndef WM_91B54_PRODUCTION_SOURCE
#define WM_91B54_PRODUCTION_SOURCE "../src/world_map_callback_91b54.c"
#endif
#include WM_91B54_PRODUCTION_SOURCE

#define RAM_MASK       0x001FFFFFu
#define MAIN_RAM_BYTES 0x00200000u

#define POOL_POINTER 0x8009BE24u
#define MODE_GLOBAL  0x8009BE10u
#define CAMERA_SCALE 0x8009D3F0u
#define CAMERA_PITCH 0x8009BD38u

#define NATURAL_POOL 0x800D7538u
#define SLOT_STRIDE  0x80u
#define SLOT_STATE   0x00u
#define SLOT_SELECTOR 0x50u
#define SLOT_TABLE_A  0x64u
#define SLOT_TABLE_B  0x68u

#define TABLE_A1 0x8009B224u
#define TABLE_A7 0x8009B22Cu
#define TABLE_B1 0x8009B234u
#define TABLE_B7 0x8009B23Cu

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

typedef enum RetailPath {
    RETAIL_NONE,
    RETAIL_ARM_A,
    RETAIL_ARM_B
} RetailPath;

typedef struct Fixture {
    const char* name;
    s32 slot_index;
    u32 pool;
    s32 mode;
} Fixture;

typedef struct Oracle {
    u8 memory[PSX_RAM_SIZE];
    TraceEvent trace[TRACE_CAPACITY];
    size_t trace_count;
} Oracle;

static Oracle s_oracle;
static u8 s_before_ram[PSX_RAM_SIZE];
static u8 s_before_scratch[sizeof(g_PsxScratchpad)];
static int s_pass_count;
static int s_total_count;
static int s_failure_count;

static size_t ram_index(u32 address)
{
    return (size_t)(address & RAM_MASK);
}

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

static void raw_store_u16(u32 address, u16 value)
{
    memory_store_u16(g_PsxRam, address, value);
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
        return RETAIL_ARM_A;
    if (mode == 7)
        return RETAIL_ARM_B;
    return RETAIL_NONE;
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

static u32 oracle_lw(u32 pc, u32 address)
{
    u32 value = memory_load_u32(s_oracle.memory, address);
    oracle_event(pc, TRACE_LW, address, 4u, value);
    return value;
}

static void oracle_sh(u32 pc, u32 address, u16 value)
{
    memory_store_u16(s_oracle.memory, address, value);
    oracle_event(pc, TRACE_SH, address, 2u, (u32)value);
}

static void oracle_sw(u32 pc, u32 address, u32 value)
{
    memory_store_u32(s_oracle.memory, address, value);
    oracle_event(pc, TRACE_SW, address, 4u, value);
}

static void seed_memory(u32 salt)
{
    /* 16 retail halfwords at TABLE_A1, read from the user's
     * disc/world_map.bin (overlay loaded at 0x8006FAF0); never committed. */
    static u16 table_halfwords[16];
    static int table_loaded;
    size_t index;

    for (index = 0u; index < sizeof(g_PsxRam); index++) {
        u32 low = (u32)(index & 0xFFFFFFFFu);
        g_PsxRam[index] =
            (u8)((low * 41u + (low >> 10) + salt * 47u + 0x63u) & 0xFFu);
    }
    for (index = 0u; index < sizeof(g_PsxScratchpad); index++) {
        u32 low = (u32)index;
        g_PsxScratchpad[index] =
            (u8)((low * 31u + salt * 19u + 0xA9u) & 0xFFu);
    }
    if (!table_loaded) {
        unsigned char raw[32];
        FILE* f = fopen("disc/world_map.bin", "rb");
        if (f == NULL || fseek(f, (long)(TABLE_A1 - 0x8006FAF0u), SEEK_SET) != 0 ||
            fread(raw, 1, sizeof raw, f) != sizeof raw) {
            fprintf(stderr, "w34b12b: cannot read the TABLE_A1 halfwords from "
                            "disc/world_map.bin (run from the repo root)\n");
            exit(2);
        }
        fclose(f);
        for (index = 0u; index < 16u; index++)
            table_halfwords[index] = (u16)(raw[2 * index] | raw[2 * index + 1] << 8);
        table_loaded = 1;
    }
    for (index = 0u; index < 16u; index++)
        raw_store_u16(TABLE_A1 + (u32)index * 2u,
                      table_halfwords[index]);
}

static void reset_trace(void)
{
    memset(s_actual_trace, 0, sizeof(s_actual_trace));
    s_actual_trace_count = 0u;
    s_actual_trace_overflow = false;
}

static void build_oracle(const Fixture* fixture, u32 slot)
{
    RetailPath path = retail_path(fixture->mode);

    memcpy(s_oracle.memory, g_PsxRam, sizeof(g_PsxRam));
    memset(s_oracle.trace, 0, sizeof(s_oracle.trace));
    s_oracle.trace_count = 0u;

    (void)oracle_lw(0x80091B5Cu, POOL_POINTER);
    (void)oracle_lw(0x80091B64u, MODE_GLOBAL);

    if (path == RETAIL_ARM_A) {
        oracle_sw(0x80091BA0u, slot + SLOT_SELECTOR, 3u);
        oracle_sw(0x80091BACu, CAMERA_SCALE, 0x00460000u);
        oracle_sh(0x80091BB8u, CAMERA_PITCH, 0xFDA0u);
        oracle_sw(0x80091BC4u, slot + SLOT_TABLE_A, TABLE_A1);
        oracle_sw(0x80091BD4u, slot + SLOT_TABLE_B, TABLE_B1);
    } else if (path == RETAIL_ARM_B) {
        oracle_sw(0x80091BDCu, slot + SLOT_SELECTOR, 3u);
        oracle_sw(0x80091BE8u, CAMERA_SCALE, 0x00280000u);
        oracle_sh(0x80091BF4u, CAMERA_PITCH, 0xFDA0u);
        oracle_sw(0x80091C00u, slot + SLOT_TABLE_A, TABLE_A7);
        oracle_sw(0x80091C0Cu, slot + SLOT_TABLE_B, TABLE_B7);
    }
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

static bool active_values_match(u32 slot, RetailPath path,
                                u32 original_selector,
                                u32 original_table_a,
                                u32 original_table_b,
                                u32 original_scale,
                                u16 original_pitch)
{
    if (path == RETAIL_ARM_A) {
        return raw_load_u32(slot + SLOT_SELECTOR) == 3u &&
               raw_load_u32(slot + SLOT_TABLE_A) == TABLE_A1 &&
               raw_load_u32(slot + SLOT_TABLE_B) == TABLE_B1 &&
               raw_load_u32(CAMERA_SCALE) == 0x00460000u &&
               raw_load_u16(CAMERA_PITCH) == 0xFDA0u;
    }
    if (path == RETAIL_ARM_B) {
        return raw_load_u32(slot + SLOT_SELECTOR) == 3u &&
               raw_load_u32(slot + SLOT_TABLE_A) == TABLE_A7 &&
               raw_load_u32(slot + SLOT_TABLE_B) == TABLE_B7 &&
               raw_load_u32(CAMERA_SCALE) == 0x00280000u &&
               raw_load_u16(CAMERA_PITCH) == 0xFDA0u;
    }
    return raw_load_u32(slot + SLOT_SELECTOR) == original_selector &&
           raw_load_u32(slot + SLOT_TABLE_A) == original_table_a &&
           raw_load_u32(slot + SLOT_TABLE_B) == original_table_b &&
           raw_load_u32(CAMERA_SCALE) == original_scale &&
           raw_load_u16(CAMERA_PITCH) == original_pitch;
}

static bool trace_has_exact_widths(RetailPath path)
{
    size_t index;
    size_t expected_count = path == RETAIL_NONE ? 2u : 7u;

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

static void run_fixture(const Fixture* fixture, u32 salt)
{
    RetailPath path = retail_path(fixture->mode);
    u32 slot = fixture->pool + (s32_bits(fixture->slot_index) << 7);
    u32 original_state;
    u32 original_selector;
    u32 original_table_a;
    u32 original_table_b;
    u32 original_scale;
    u16 original_pitch;
    u32 original_before;
    u32 original_after;
    u8 resource_before[0x20];
    s32 result;

    seed_memory(salt);
    raw_store_u32(POOL_POINTER, fixture->pool);
    raw_store_u32(MODE_GLOBAL, s32_bits(fixture->mode));
    raw_store_u32(CAMERA_SCALE, 0xA1B2C3D4u ^ salt);
    raw_store_u16(CAMERA_PITCH, (u16)(0x5A00u ^ (u16)salt));

    original_state = raw_load_u32(slot + SLOT_STATE);
    original_selector = raw_load_u32(slot + SLOT_SELECTOR);
    original_table_a = raw_load_u32(slot + SLOT_TABLE_A);
    original_table_b = raw_load_u32(slot + SLOT_TABLE_B);
    original_scale = raw_load_u32(CAMERA_SCALE);
    original_pitch = raw_load_u16(CAMERA_PITCH);
    original_before = raw_load_u32(slot - 4u);
    original_after = raw_load_u32(slot + SLOT_STRIDE);
    memcpy(resource_before, g_PsxRam + ram_index(TABLE_A1),
           sizeof(resource_before));

    memcpy(s_before_ram, g_PsxRam, sizeof(g_PsxRam));
    memcpy(s_before_scratch, g_PsxScratchpad, sizeof(g_PsxScratchpad));
    build_oracle(fixture, slot);
    reset_trace();

    result = wm_80091B54(fixture->slot_index);

    check_case(fixture->name, "return is scheduler state 1", result == 1);
    check_case(fixture->name, "exact typed guest access trace",
               trace_matches(fixture->name));
    check_case(fixture->name, "exact path access count",
               s_actual_trace_count == (path == RETAIL_NONE ? 2u : 7u));
    check_case(fixture->name, "exact load/store widths",
               trace_has_exact_widths(path));
    check_case(fixture->name, "BE24 then signed BE10 are sole reads",
               s_actual_trace_count >= 2u &&
                   s_actual_trace[0].pc == 0x80091B5Cu &&
                   s_actual_trace[0].kind == TRACE_LW &&
                   s_actual_trace[0].address == POOL_POINTER &&
                   s_actual_trace[1].pc == 0x80091B64u &&
                   s_actual_trace[1].kind == TRACE_LW &&
                   s_actual_trace[1].address == MODE_GLOBAL);
    check_case(fixture->name, "signed path values and exact guest pointers",
               active_values_match(slot, path, original_selector,
                                   original_table_a, original_table_b,
                                   original_scale, original_pitch));
    check_case(fixture->name, "scheduler-owned slot+00 untouched",
               raw_load_u32(slot + SLOT_STATE) == original_state);
    check_case(fixture->name, "adjacent slot canaries untouched",
               raw_load_u32(slot - 4u) == original_before &&
                   raw_load_u32(slot + SLOT_STRIDE) == original_after);
    check_case(fixture->name, "static table targets never dereferenced/written",
               memcmp(g_PsxRam + ram_index(TABLE_A1), resource_before,
                      sizeof(resource_before)) == 0);
    check_case(fixture->name, "whole 3 MiB RAM exact footprint",
               memcmp(g_PsxRam, s_oracle.memory, sizeof(g_PsxRam)) == 0);
    check_case(fixture->name, "complete selected slot matches oracle",
               memcmp(g_PsxRam + ram_index(slot),
                      s_oracle.memory + ram_index(slot), SLOT_STRIDE) == 0);
    check_case(fixture->name, "1 MiB guard canary untouched",
               memcmp(g_PsxRam + MAIN_RAM_BYTES,
                      s_before_ram + MAIN_RAM_BYTES,
                      sizeof(g_PsxRam) - MAIN_RAM_BYTES) == 0);
    check_case(fixture->name, "scratchpad untouched",
               memcmp(g_PsxScratchpad, s_before_scratch,
                      sizeof(g_PsxScratchpad)) == 0);
}

static void run_oracle_sentinels(void)
{
    check_case("oracle", "INT32_MIN is no-write",
               retail_path(INT32_MIN) == RETAIL_NONE);
    check_case("oracle", "negative one is no-write",
               retail_path(-1) == RETAIL_NONE);
    check_case("oracle", "zero is no-write",
               retail_path(0) == RETAIL_NONE);
    check_case("oracle", "mode1 starts arm A",
               retail_path(1) == RETAIL_ARM_A);
    check_case("oracle", "mode5 ends arm A",
               retail_path(5) == RETAIL_ARM_A);
    check_case("oracle", "mode6 is no-write",
               retail_path(6) == RETAIL_NONE);
    check_case("oracle", "mode7 is arm B",
               retail_path(7) == RETAIL_ARM_B);
    check_case("oracle", "mode8 is no-write",
               retail_path(8) == RETAIL_NONE);
    check_case("oracle", "INT32_MAX is no-write",
               retail_path(INT32_MAX) == RETAIL_NONE);
    check_case("oracle", "negative slot shift wraps",
               (s32_bits(-1) << 7) == 0xFFFFFF80u);
    check_case("oracle", "INT32_MIN slot shift wraps to zero",
               (s32_bits(INT32_MIN) << 7) == 0u);
    check_case("oracle", "retail pointer pairs remain asymmetric",
               TABLE_A1 != TABLE_A7 && TABLE_B1 != TABLE_B7 &&
                   TABLE_A1 != TABLE_B1 && TABLE_A7 != TABLE_B7);
}

int main(void)
{
    static const Fixture fixtures[] = {
        {"mode-int-min", 10, NATURAL_POOL, INT32_MIN},
        {"mode-negative-two", 10, NATURAL_POOL, -2},
        {"mode-negative-one", 10, NATURAL_POOL, -1},
        {"mode-zero", 10, NATURAL_POOL, 0},
        {"mode-one-natural-arm-a", 10, NATURAL_POOL, 1},
        {"mode-two-arm-a", 10, NATURAL_POOL, 2},
        {"mode-three-arm-a", 10, NATURAL_POOL, 3},
        {"mode-four-arm-a", 10, NATURAL_POOL, 4},
        {"mode-five-arm-a-edge", 10, NATURAL_POOL, 5},
        {"mode-six-no-write-gap", 10, NATURAL_POOL, 6},
        {"mode-seven-arm-b", 10, NATURAL_POOL, 7},
        {"mode-eight-no-write-edge", 10, NATURAL_POOL, 8},
        {"mode-nine", 10, NATURAL_POOL, 9},
        {"mode-int-max", 10, NATURAL_POOL, INT32_MAX},
        {"slot-index-minus-one", -1, 0x800D8000u, 1},
        {"slot-index-int-min", INT32_MIN, 0x800D8100u, 7},
        {"slot-addition-wrap", 1, 0xFFFFFFF0u, 6}
    };
    size_t index;

    for (index = 0u; index < sizeof(fixtures) / sizeof(fixtures[0]);
         index++)
        run_fixture(&fixtures[index], (u32)index + 0x51u);

    run_oracle_sentinels();
    printf("=== Results: %d/%d PASS ===\n", s_pass_count, s_total_count);
    return s_failure_count == 0 ? 0 : 1;
}
