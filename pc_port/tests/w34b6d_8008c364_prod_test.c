/*
 * W34B6-D production-linked certification test for world helper 0x8008C364.
 *
 * The actual production translation unit is included below so a test-only
 * PSX_ADDR translator can observe retail memory-read order.  The translator
 * is the only memory seam: expected results come from the declarative retail
 * route table and adversarial fixtures in this file, not from a second C364
 * implementation.  The two direct dependencies are controlled recorders.
 *
 * Example strict build from the repository root:
 *
 *   gcc -std=gnu17 -O0 -g -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
 *     -fno-pie -no-pie \
 *     -Wall -Wextra -Wconversion -Wsign-conversion -Werror \
 *     -Ipc_port/tests/include -Ipc_port/include_shim -Iinclude -Ipc_port/src \
 *     pc_port/tests/w34b6d_8008c364_prod_test.c \
 *     -o scratchpad/w34b6d_c364_implementation/c364_o0
 *
 * Replace -O0 with -O2 for the optimized gate.  For the UB gate add:
 *
 *   -fsanitize=undefined -fno-sanitize-recover=undefined
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"

/* Pre-including psx_memory.h closes its include guard.  The production source
 * then sees this deterministic translator in place of the ordinary macro. */
static void* test_psx_addr(uintptr_t address_bits) __attribute__((noinline));

#undef PSX_ADDR
#define PSX_ADDR(address) test_psx_addr((uintptr_t)(address))

#include "../src/world_map_helper_c364.c"

#define TEST_RAM_MASK          0x001FFFFFu
#define TEST_PRIMARY_BASE      0x8006F368u
#define TEST_POSITION_BASE     0x8006EF8Eu
#define TEST_FLAG_BASE         0x8006F8E5u
#define TEST_GEAR_BASE         0x8006D940u
#define TEST_PACKAGE_BASE      0x8009BDF8u
#define TEST_SHARED_X          0x8009C5ACu
#define TEST_SHARED_Z          0x8009C5B4u
#define TEST_SLOT_BASE         0x80180080u
#define TEST_SLOT_STRIDE       0x80u
#define TEST_SLOT_CONTROL      0x24u
#define TEST_SLOT_X            0x28u
#define TEST_SLOT_Y            0x2Cu
#define TEST_SLOT_Z            0x30u
#define TEST_SLOT_OBJECT       0x4Cu
#define TEST_CHANNEL_COUNT     3u
#define TEST_CHARACTER_STRIDE  0xA4u
#define TEST_TRACE_CAPACITY    128u
#define TEST_OBJECT_TOKEN_BASE 0xC3644C00u

_Static_assert(PSX_RAM_SIZE >= (2 * 1024 * 1024),
               "test requires complete PSX main RAM");

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

typedef enum TraceKind {
    TRACE_ADDRESS = 1,
    TRACE_C28C,
    TRACE_TERRAIN
} TraceKind;

typedef struct TraceEvent {
    TraceKind kind;
    u32 value;
} TraceEvent;

typedef enum PositionKind {
    POSITION_ZERO = 0,
    POSITION_CHANNEL,
    POSITION_SHARED
} PositionKind;

/* Independent semantic description of raw jump-table entries 0..7. */
typedef struct RouteSpec {
    s32 return_state;
    u16 control;
    PositionKind position;
    bool calls_c28c;
    bool calls_terrain;
    bool reads_gear;
    bool writes_position_control;
} RouteSpec;

static const RouteSpec s_routes[8] = {
    { 3, 1u, POSITION_ZERO,    false, false, false, false },
    { 1, 1u, POSITION_ZERO,    true,  false, true,  false },
    { 3, 1u, POSITION_ZERO,    false, false, false, false },
    { 1, 0u, POSITION_CHANNEL, true,  true,  false, false },
    { 3, 1u, POSITION_ZERO,    false, false, false, false },
    { 1, 0u, POSITION_SHARED,  true,  true,  false, true  },
    { 3, 1u, POSITION_ZERO,    false, false, false, false },
    { 1, 0u, POSITION_SHARED,  true,  true,  false, true  },
};

typedef struct Fixture {
    const char* name;
    u32 channel;
    u32 expected_index;
    u8 primary;
    u8 gear;
    u8 flag;
    u16 raw_position;
    u16 channel_x;
    u16 channel_z;
    u32 shared_x;
    u32 shared_z;
    u32 terrain_result;
    bool mutate_primary;
    u8 mutated_primary;
    u8 mutated_gear;
} Fixture;

typedef struct ExpectedResult {
    s32 return_state;
    u16 control;
    u32 x;
    u32 y;
    u32 z;
    int c28c_calls;
    int terrain_calls;
    bool reads_gear;
    bool writes_position_control;
} ExpectedResult;

typedef struct ObservedResult {
    s32 return_state;
    int c28c_calls;
    int terrain_calls;
    u16 control;
    u32 x;
    u32 y;
    u32 z;
    u32 terrain_x_bits;
    u32 terrain_z_bits;
    u16 position_control[TEST_CHANNEL_COUNT];
    u16 position_x[TEST_CHANNEL_COUNT];
    u16 position_z[TEST_CHANNEL_COUNT];
} ObservedResult;

typedef struct CoreOutcome {
    s32 return_state;
    int c28c_calls;
    int terrain_calls;
    u16 control;
    u32 x;
    u32 y;
    u32 z;
    u32 terrain_x_bits;
    u32 terrain_z_bits;
} CoreOutcome;

static const u8 s_default_primary[TEST_CHANNEL_COUNT] = {
    0x31u, 0x52u, 0x73u
};
static const u8 s_default_flag[TEST_CHANNEL_COUNT] = {
    0x00u, 0x02u, 0xFFu
};
static const u16 s_default_raw_position[TEST_CHANNEL_COUNT] = {
    0x0123u, 0x4567u, 0xC321u
};
static const u16 s_default_channel_x[TEST_CHANNEL_COUNT] = {
    0x8123u, 0x7FFEu, 0xF00Du
};
static const u16 s_default_channel_z[TEST_CHANNEL_COUNT] = {
    0xFEDCu, 0x8001u, 0x1357u
};
static const u32 s_package_bits[TEST_CHANNEL_COUNT] = {
    0x800AB458u, 0x800BC560u, 0x800CD678u
};

static u8 s_ram_before[PSX_RAM_SIZE];
static u8 s_scratchpad_before[sizeof(g_PsxScratchpad)];
static TraceEvent s_trace[TEST_TRACE_CAPACITY];
static size_t s_trace_count;
static bool s_trace_overflow;
static bool s_trace_armed;
static u32 s_watched_primary;
static int s_primary_translation_count;
static bool s_mutate_primary;
static u8 s_mutated_primary;

static int s_pass_count;
static int s_total_count;
static int s_failure_count;

static int s_c28c_calls;
static u32 s_c28c_slot;
static s32 s_c28c_channel;
static int s_terrain_calls;
static u32 s_terrain_x_bits;
static u32 s_terrain_z_bits;
static u32 s_terrain_return_bits;
static u32 s_current_slot;

static size_t raw_index(u32 address)
{
    return (size_t)(address & TEST_RAM_MASK);
}

static void* raw_address(u32 address)
{
    return (void*)(g_PsxRam + raw_index(address));
}

static void trace_append(TraceKind kind, u32 value)
{
    if (s_trace_count < (size_t)TEST_TRACE_CAPACITY) {
        s_trace[s_trace_count].kind = kind;
        s_trace[s_trace_count].value = value;
    } else {
        s_trace_overflow = true;
    }
    s_trace_count++;
}

static void* test_psx_addr(uintptr_t address_bits)
{
    u32 address = (u32)address_bits;
    size_t index = raw_index(address);

    if (s_trace_armed) {
        trace_append(TRACE_ADDRESS, address);
        if (address == s_watched_primary) {
            s_primary_translation_count++;
            if (s_mutate_primary && s_primary_translation_count == 2)
                g_PsxRam[index] = s_mutated_primary;
        }
    }
    return (void*)(g_PsxRam + index);
}

static void store_u8_raw(u32 address, u8 value)
{
    *(u8*)raw_address(address) = value;
}

static u8 load_u8_raw(u32 address)
{
    return *(const u8*)raw_address(address);
}

static void store_u16_raw(u32 address, u16 value)
{
    memcpy(raw_address(address), &value, sizeof(value));
}

static u16 load_u16_raw(u32 address)
{
    u16 value;
    memcpy(&value, raw_address(address), sizeof(value));
    return value;
}

static void store_u32_raw(u32 address, u32 value)
{
    memcpy(raw_address(address), &value, sizeof(value));
}

static u32 load_u32_raw(u32 address)
{
    u32 value;
    memcpy(&value, raw_address(address), sizeof(value));
    return value;
}

static s32 u32_as_s32(u32 bits)
{
    s32 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static u32 s32_as_u32(s32 value)
{
    u32 bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

static u32 channel_offset6(u32 channel)
{
    return channel * 6u;
}

static u32 gear_address(u8 primary)
{
    return TEST_GEAR_BASE + (u32)primary * TEST_CHARACTER_STRIDE;
}

static void check_case(const char* case_name, const char* description,
                       bool condition)
{
    s_total_count++;
    if (condition) {
        s_pass_count++;
    } else {
        s_failure_count++;
        printf("FAIL [%s]: %s\n", case_name, description);
    }
}

static void check_global(const char* description, bool condition)
{
    check_case("mutant-detection", description, condition);
}

static void report_mutant(const char* name, bool detected)
{
    check_global(name, detected);
    if (detected)
        printf("MUTANT DETECTED: %s\n", name);
}

static int trace_address_count(u32 address)
{
    size_t index;
    int count = 0;
    size_t available = s_trace_count;

    if (available > (size_t)TEST_TRACE_CAPACITY)
        available = (size_t)TEST_TRACE_CAPACITY;
    for (index = 0u; index < available; index++) {
        if (s_trace[index].kind == TRACE_ADDRESS &&
            s_trace[index].value == address) {
            count++;
        }
    }
    return count;
}

static int trace_address_range_count(u32 address, u32 width)
{
    size_t index;
    int count = 0;
    size_t available = s_trace_count;
    u32 end = address + width;

    if (available > (size_t)TEST_TRACE_CAPACITY)
        available = (size_t)TEST_TRACE_CAPACITY;
    for (index = 0u; index < available; index++) {
        if (s_trace[index].kind == TRACE_ADDRESS &&
            s_trace[index].value >= address &&
            s_trace[index].value < end) {
            count++;
        }
    }
    return count;
}

static size_t trace_event_index(TraceKind kind, u32 value, int occurrence)
{
    size_t index;
    int seen = 0;
    size_t available = s_trace_count;

    if (available > (size_t)TEST_TRACE_CAPACITY)
        available = (size_t)TEST_TRACE_CAPACITY;
    for (index = 0u; index < available; index++) {
        if (s_trace[index].kind == kind && s_trace[index].value == value) {
            seen++;
            if (seen == occurrence)
                return index;
        }
    }
    return SIZE_MAX;
}

static bool trace_address_is_gear(u32 address)
{
    u32 maximum = TEST_GEAR_BASE + 0xFFu * TEST_CHARACTER_STRIDE;
    u32 difference;

    if (address < TEST_GEAR_BASE || address > maximum)
        return false;
    difference = address - TEST_GEAR_BASE;
    return difference % TEST_CHARACTER_STRIDE == 0u;
}

static int trace_gear_count(void)
{
    size_t index;
    int count = 0;
    size_t available = s_trace_count;

    if (available > (size_t)TEST_TRACE_CAPACITY)
        available = (size_t)TEST_TRACE_CAPACITY;
    for (index = 0u; index < available; index++) {
        if (s_trace[index].kind == TRACE_ADDRESS &&
            trace_address_is_gear(s_trace[index].value)) {
            count++;
        }
    }
    return count;
}

static bool trace_touched_scratchpad(void)
{
    size_t index;
    size_t available = s_trace_count;

    if (available > (size_t)TEST_TRACE_CAPACITY)
        available = (size_t)TEST_TRACE_CAPACITY;
    for (index = 0u; index < available; index++) {
        if (s_trace[index].kind == TRACE_ADDRESS &&
            s_trace[index].value >= 0x1F800000u &&
            s_trace[index].value < 0x1F800400u) {
            return true;
        }
    }
    return false;
}

/* ------------------------- controlled dependencies -------------------- */

void wm_8008C28C(u32 slot_addr, s32 channel)
{
    u32 object_token = TEST_OBJECT_TOKEN_BASE | (u32)channel;

    trace_append(TRACE_C28C, 0u);
    s_c28c_calls++;
    s_c28c_slot = slot_addr;
    s_c28c_channel = channel;
    store_u32_raw(slot_addr + TEST_SLOT_OBJECT, object_token);
}

s32 wm_80093978(s32 x, s32 z)
{
    trace_append(TRACE_TERRAIN, 0u);
    s_terrain_calls++;
    s_terrain_x_bits = s32_as_u32(x);
    s_terrain_z_bits = s32_as_u32(z);
    return u32_as_s32(s_terrain_return_bits);
}

static ExpectedResult expected_for(const Fixture* fixture)
{
    RouteSpec route = s_routes[fixture->expected_index];
    ExpectedResult expected;
    u8 effective_gear = fixture->mutate_primary
                            ? fixture->mutated_gear
                            : fixture->gear;

    expected.return_state = route.return_state;
    expected.control = route.control;
    expected.c28c_calls = route.calls_c28c ? 1 : 0;
    expected.terrain_calls = route.calls_terrain ? 1 : 0;
    expected.reads_gear = route.reads_gear;
    expected.writes_position_control = route.writes_position_control;

    if (route.reads_gear && effective_gear == 0xFFu) {
        expected.return_state = 3;
        expected.c28c_calls = 0;
    }

    switch (route.position) {
    case POSITION_ZERO:
        expected.x = 0u;
        expected.y = 0u;
        expected.z = 0u;
        break;
    case POSITION_CHANNEL:
        expected.x = (u32)fixture->channel_x << 12;
        expected.y = fixture->terrain_result;
        expected.z = (u32)fixture->channel_z << 12;
        break;
    case POSITION_SHARED:
        expected.x = fixture->shared_x;
        expected.y = fixture->terrain_result;
        expected.z = fixture->shared_z;
        break;
    default:
        expected.x = 0xFFFFFFFFu;
        expected.y = 0xFFFFFFFFu;
        expected.z = 0xFFFFFFFFu;
        break;
    }
    return expected;
}

static void reset_instrumentation(const Fixture* fixture)
{
    memset(s_trace, 0, sizeof(s_trace));
    s_trace_count = 0u;
    s_trace_overflow = false;
    s_trace_armed = false;
    s_primary_translation_count = 0;
    s_watched_primary = TEST_PRIMARY_BASE + fixture->channel;
    s_mutate_primary = fixture->mutate_primary;
    s_mutated_primary = fixture->mutated_primary;
    s_c28c_calls = 0;
    s_c28c_slot = 0u;
    s_c28c_channel = -1;
    s_terrain_calls = 0;
    s_terrain_x_bits = 0u;
    s_terrain_z_bits = 0u;
    s_terrain_return_bits = fixture->terrain_result;
}

static void prepare_fixture(const Fixture* fixture)
{
    u32 channel;

    memset(g_PsxRam, 0xA5, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0x5A, sizeof(g_PsxScratchpad));
    s_current_slot = TEST_SLOT_BASE + fixture->channel * TEST_SLOT_STRIDE;

    /* Three distinct slot images make fixed-slot and adjacent-slot writes
     * visible to the whole-RAM write-set scan. */
    memset(raw_address(TEST_SLOT_BASE - TEST_SLOT_STRIDE), 0xB6,
           (size_t)(TEST_SLOT_STRIDE * 5u));

    for (channel = 0u; channel < TEST_CHANNEL_COUNT; channel++) {
        u8 primary = s_default_primary[channel];
        u32 offset = channel_offset6(channel);

        store_u8_raw(TEST_PRIMARY_BASE + channel, primary);
        store_u8_raw(TEST_FLAG_BASE + channel, s_default_flag[channel]);
        store_u16_raw(TEST_POSITION_BASE + offset,
                      s_default_raw_position[channel]);
        store_u16_raw(TEST_POSITION_BASE + offset + 2u,
                      s_default_channel_x[channel]);
        store_u16_raw(TEST_POSITION_BASE + offset + 4u,
                      s_default_channel_z[channel]);
        store_u8_raw(gear_address(primary), (u8)(0x10u + channel));
        store_u32_raw(TEST_PACKAGE_BASE + channel * 4u,
                      s_package_bits[channel]);
    }

    store_u8_raw(TEST_PRIMARY_BASE + fixture->channel, fixture->primary);
    store_u8_raw(TEST_FLAG_BASE + fixture->channel, fixture->flag);
    store_u16_raw(TEST_POSITION_BASE + channel_offset6(fixture->channel),
                  fixture->raw_position);
    store_u16_raw(TEST_POSITION_BASE + channel_offset6(fixture->channel) + 2u,
                  fixture->channel_x);
    store_u16_raw(TEST_POSITION_BASE + channel_offset6(fixture->channel) + 4u,
                  fixture->channel_z);
    store_u8_raw(gear_address(fixture->primary), fixture->gear);
    if (fixture->mutate_primary) {
        store_u8_raw(gear_address(fixture->mutated_primary),
                     fixture->mutated_gear);
    }
    store_u32_raw(TEST_SHARED_X, fixture->shared_x);
    store_u32_raw(TEST_SHARED_Z, fixture->shared_z);

    reset_instrumentation(fixture);
    memcpy(s_ram_before, g_PsxRam, sizeof(s_ram_before));
    memcpy(s_scratchpad_before, g_PsxScratchpad,
           sizeof(s_scratchpad_before));
}

static bool index_in_range(size_t index, u32 guest_address, size_t width)
{
    size_t start = raw_index(guest_address);
    return index >= start && index - start < width;
}

static bool authorized_changed_byte(size_t index, const Fixture* fixture,
                                    const ExpectedResult* expected)
{
    u32 position_address = TEST_POSITION_BASE +
                           channel_offset6(fixture->channel);

    if (index_in_range(index, s_current_slot + TEST_SLOT_CONTROL, 2u) ||
        index_in_range(index, s_current_slot + TEST_SLOT_X, 4u) ||
        index_in_range(index, s_current_slot + TEST_SLOT_Y, 4u) ||
        index_in_range(index, s_current_slot + TEST_SLOT_Z, 4u)) {
        return true;
    }
    if (expected->c28c_calls == 1 &&
        index_in_range(index, s_current_slot + TEST_SLOT_OBJECT, 4u)) {
        return true;
    }
    if (expected->writes_position_control &&
        index_in_range(index, position_address, 2u)) {
        return true;
    }
    if (fixture->mutate_primary &&
        index_in_range(index, TEST_PRIMARY_BASE + fixture->channel, 1u)) {
        return true;
    }
    return false;
}

static bool ram_write_set_is_exact(const Fixture* fixture,
                                   const ExpectedResult* expected)
{
    size_t index;

    for (index = 0u; index < (size_t)PSX_RAM_SIZE; index++) {
        if (g_PsxRam[index] != s_ram_before[index] &&
            !authorized_changed_byte(index, fixture, expected)) {
            return false;
        }
    }
    return true;
}

static void verify_global_and_slot_state(const Fixture* fixture,
                                         const ExpectedResult* expected)
{
    u32 channel;
    u32 selected_position = TEST_POSITION_BASE +
                            channel_offset6(fixture->channel);
    u32 expected_object = TEST_OBJECT_TOKEN_BASE | fixture->channel;
    u16 before_control_high;

    memcpy(&before_control_high,
           s_ram_before +
               raw_index(s_current_slot + TEST_SLOT_CONTROL + 2u),
           sizeof(before_control_high));

    check_case(fixture->name, "slot+24 exact halfword",
               load_u16_raw(s_current_slot + TEST_SLOT_CONTROL) ==
                   expected->control);
    check_case(fixture->name, "slot+26/+27 preserved",
               load_u16_raw(s_current_slot + TEST_SLOT_CONTROL + 2u) ==
                   before_control_high);
    check_case(fixture->name, "slot X exact",
               load_u32_raw(s_current_slot + TEST_SLOT_X) == expected->x);
    check_case(fixture->name, "slot Y exact",
               load_u32_raw(s_current_slot + TEST_SLOT_Y) == expected->y);
    check_case(fixture->name, "slot Z exact",
               load_u32_raw(s_current_slot + TEST_SLOT_Z) == expected->z);
    check_case(fixture->name, "scheduler state preserved",
               memcmp(raw_address(s_current_slot),
                      s_ram_before + raw_index(s_current_slot), 4u) == 0);

    if (expected->c28c_calls == 1) {
        check_case(fixture->name, "C28C object publication retained",
                   load_u32_raw(s_current_slot + TEST_SLOT_OBJECT) ==
                       expected_object);
    } else {
        check_case(fixture->name, "slot+4C untouched without C28C",
                   memcmp(raw_address(s_current_slot + TEST_SLOT_OBJECT),
                          s_ram_before +
                              raw_index(s_current_slot + TEST_SLOT_OBJECT),
                          4u) == 0);
    }

    for (channel = 0u; channel < TEST_CHANNEL_COUNT; channel++) {
        u32 address = TEST_POSITION_BASE + channel_offset6(channel);
        u16 before_raw;
        u16 expected_raw;
        u16 before_x;
        u16 before_z;

        memcpy(&before_raw, s_ram_before + raw_index(address),
               sizeof(before_raw));
        memcpy(&before_x, s_ram_before + raw_index(address + 2u),
               sizeof(before_x));
        memcpy(&before_z, s_ram_before + raw_index(address + 4u),
               sizeof(before_z));
        expected_raw = expected->writes_position_control &&
                               channel == fixture->channel
                           ? (u16)0x0400u
                           : before_raw;
        check_case(fixture->name, "EF8E triplet control exact channel",
                   load_u16_raw(address) == expected_raw);
        check_case(fixture->name, "EF90 triplet member unchanged",
                   load_u16_raw(address + 2u) == before_x);
        check_case(fixture->name, "EF92 triplet member unchanged",
                   load_u16_raw(address + 4u) == before_z);
    }

    if (expected->writes_position_control) {
        check_case(fixture->name, "selected EF8E exact SH literal",
                   load_u16_raw(selected_position) == (u16)0x0400u);
    }
    check_case(fixture->name, "whole-RAM write set exact",
               ram_write_set_is_exact(fixture, expected));
    check_case(fixture->name, "scratchpad untouched",
               memcmp(g_PsxScratchpad, s_scratchpad_before,
                      sizeof(s_scratchpad_before)) == 0);
    check_case(fixture->name, "no scratchpad address translated",
               !trace_touched_scratchpad());
    check_case(fixture->name, "D940 selected record unchanged",
               load_u8_raw(gear_address(fixture->primary)) == fixture->gear);
    if (fixture->mutate_primary) {
        check_case(fixture->name, "fresh D940 record unchanged",
                   load_u8_raw(gear_address(fixture->mutated_primary)) ==
                       fixture->mutated_gear);
    } else {
        check_case(fixture->name, "selected primary byte unchanged",
                   load_u8_raw(TEST_PRIMARY_BASE + fixture->channel) ==
                       fixture->primary);
    }
    check_case(fixture->name, "F8E5 selected byte unchanged",
               load_u8_raw(TEST_FLAG_BASE + fixture->channel) ==
                   fixture->flag);
    check_case(fixture->name, "package table unchanged",
               load_u32_raw(TEST_PACKAGE_BASE + fixture->channel * 4u) ==
                   s_package_bits[fixture->channel]);
}

static void verify_trace_and_dependencies(const Fixture* fixture,
                                          const ExpectedResult* expected)
{
    static const u32 forbidden_slot_offsets[] = {
        0x00u, 0x02u, 0x04u, 0x18u, 0x1Cu, 0x20u, 0x22u,
        0x34u, 0x38u, 0x3Cu, 0x40u, 0x48u, 0x4Au, 0x4Cu,
        0x50u, 0x54u, 0x58u, 0x5Cu
    };
    u32 channel;
    size_t forbidden_index;
    u32 primary_address = TEST_PRIMARY_BASE + fixture->channel;
    u32 flag_address = TEST_FLAG_BASE + fixture->channel;
    u32 position_address = TEST_POSITION_BASE +
                           channel_offset6(fixture->channel);
    u8 effective_primary = fixture->mutate_primary
                               ? fixture->mutated_primary
                               : fixture->primary;
    u32 selected_gear = gear_address(effective_primary);
    int expected_primary_reads = expected->reads_gear ? 2 : 1;
    size_t c28c_event = trace_event_index(TRACE_C28C, 0u, 1);
    size_t terrain_event = trace_event_index(TRACE_TERRAIN, 0u, 1);
    size_t primary_event = trace_event_index(
        TRACE_ADDRESS, primary_address, 1);
    size_t position_event = trace_event_index(
        TRACE_ADDRESS, position_address, 1);
    size_t flag_event = trace_event_index(TRACE_ADDRESS, flag_address, 1);
    size_t control_store_event = trace_event_index(
        TRACE_ADDRESS, s_current_slot + TEST_SLOT_CONTROL, 1);
    size_t x_store_event = trace_event_index(
        TRACE_ADDRESS, s_current_slot + TEST_SLOT_X, 1);
    size_t y_store_event = trace_event_index(
        TRACE_ADDRESS, s_current_slot + TEST_SLOT_Y, 1);
    size_t z_store_event = trace_event_index(
        TRACE_ADDRESS, s_current_slot + TEST_SLOT_Z, 1);

    check_case(fixture->name, "trace capacity sufficient",
               !s_trace_overflow && s_trace_count <=
                                        (size_t)TEST_TRACE_CAPACITY);
    check_case(fixture->name, "selected primary read count",
               trace_address_count(primary_address) ==
                   expected_primary_reads);
    check_case(fixture->name, "selected F8E5 read exactly once",
               trace_address_count(flag_address) == 1);
    check_case(fixture->name, "selected EF8E initial read",
               trace_address_count(position_address) ==
                   (expected->writes_position_control ? 2 : 1));
    check_case(fixture->name, "dispatch input read order",
               primary_event < position_event &&
                   position_event < flag_event);
    check_case(fixture->name, "all four direct slot stores observed",
               control_store_event != SIZE_MAX &&
                   x_store_event != SIZE_MAX && y_store_event != SIZE_MAX &&
                   z_store_event != SIZE_MAX);
    for (forbidden_index = 0u;
         forbidden_index < sizeof(forbidden_slot_offsets) /
                                   sizeof(forbidden_slot_offsets[0]);
         forbidden_index++) {
        check_case(fixture->name, "forbidden slot field not translated",
                   trace_address_count(
                       s_current_slot +
                       forbidden_slot_offsets[forbidden_index]) == 0);
    }
    check_case(fixture->name, "C364 never translates slot+4C object field",
               trace_address_range_count(
                   s_current_slot + TEST_SLOT_OBJECT, 4u) == 0);
    check_case(fixture->name, "preceding adjacent slot not translated",
               trace_address_range_count(
                   s_current_slot - TEST_SLOT_STRIDE, TEST_SLOT_STRIDE) == 0);
    check_case(fixture->name, "following adjacent slot not translated",
               trace_address_range_count(
                   s_current_slot + TEST_SLOT_STRIDE, TEST_SLOT_STRIDE) == 0);
    for (channel = 0u; channel < TEST_CHANNEL_COUNT; channel++) {
        if (channel == fixture->channel)
            continue;
        check_case(fixture->name, "other primary channels not read",
                   trace_address_count(TEST_PRIMARY_BASE + channel) == 0);
        check_case(fixture->name, "other flag channels not read",
                   trace_address_count(TEST_FLAG_BASE + channel) == 0);
    }
    for (channel = 0u; channel < TEST_CHANNEL_COUNT; channel++) {
        check_case(fixture->name, "C364 does not read BDF8 package table",
                   trace_address_count(TEST_PACKAGE_BASE + channel * 4u) == 0);
    }

    check_case(fixture->name, "D940 read count exact",
               trace_gear_count() == (expected->reads_gear ? 1 : 0));
    if (expected->reads_gear) {
        check_case(fixture->name, "D940 uses fresh primary address",
                   trace_address_count(selected_gear) == 1);
        if (fixture->mutate_primary) {
            check_case(fixture->name, "stale primary D940 not read",
                       trace_address_count(gear_address(fixture->primary)) ==
                           0);
        }
    }

    check_case(fixture->name, "C28C call count exact",
               s_c28c_calls == expected->c28c_calls);
    check_case(fixture->name, "C28C call count never exceeds one",
               s_c28c_calls <= 1);
    check_case(fixture->name, "terrain call count exact",
               s_terrain_calls == expected->terrain_calls);
    if (expected->c28c_calls == 1) {
        check_case(fixture->name, "C28C original direct slot argument",
                   s_c28c_slot == s_current_slot);
        check_case(fixture->name, "C28C unchanged channel argument",
                   s_c28c_channel == (s32)fixture->channel);
    }

    if (expected->terrain_calls == 1) {
        check_case(fixture->name, "terrain X source exact",
                   s_terrain_x_bits == expected->x);
        check_case(fixture->name, "terrain Z source exact",
                   s_terrain_z_bits == expected->z);
        check_case(fixture->name, "C28C precedes terrain",
                   c28c_event != SIZE_MAX && terrain_event != SIZE_MAX &&
                       c28c_event < terrain_event);
    }

    if (fixture->expected_index == 1u) {
        size_t gear_event = trace_event_index(TRACE_ADDRESS, selected_gear, 1);
        size_t fresh_primary_event = trace_event_index(
            TRACE_ADDRESS, primary_address, 2);

        check_case(fixture->name, "index1 fresh-read and gear order",
                   flag_event < fresh_primary_event &&
                       fresh_primary_event < gear_event);
        if (expected->c28c_calls == 1) {
            check_case(fixture->name, "index1 gear gate precedes C28C",
                       gear_event != SIZE_MAX && c28c_event != SIZE_MAX &&
                           gear_event < c28c_event);
            check_case(fixture->name, "index1 C28C precedes slot stores",
                       c28c_event < control_store_event);
        }
        check_case(fixture->name, "index1 performs no terrain call",
                   terrain_event == SIZE_MAX);
        check_case(fixture->name, "index1/missing slot store order",
                   control_store_event < y_store_event &&
                       y_store_event < z_store_event &&
                       z_store_event < x_store_event);
    } else if (fixture->expected_index == 3u) {
        size_t x_event = trace_event_index(
            TRACE_ADDRESS, position_address + 2u, 1);
        size_t z_event = trace_event_index(
            TRACE_ADDRESS, position_address + 4u, 1);
        check_case(fixture->name, "index3 local-position call order",
                   c28c_event < x_event && x_event < z_event &&
                       z_event < terrain_event);
        check_case(fixture->name, "index3 slot/dependency store order",
                   c28c_event < control_store_event &&
                       control_store_event < x_event &&
                       x_event < x_store_event && x_store_event < z_event &&
                       z_event < z_store_event &&
                       z_store_event < terrain_event &&
                       terrain_event < y_store_event);
        check_case(fixture->name, "index3 does not read shared X/Z",
                   trace_address_count(TEST_SHARED_X) == 0 &&
                       trace_address_count(TEST_SHARED_Z) == 0);
    } else if (fixture->expected_index == 5u ||
               fixture->expected_index == 7u) {
        size_t x_event = trace_event_index(TRACE_ADDRESS, TEST_SHARED_X, 1);
        size_t z_event = trace_event_index(TRACE_ADDRESS, TEST_SHARED_Z, 1);
        size_t ef_store_event = trace_event_index(
            TRACE_ADDRESS, position_address, 2);
        check_case(fixture->name, "index5/7 shared-position call order",
                   c28c_event < x_event && x_event < z_event &&
                       z_event < terrain_event &&
                       terrain_event < ef_store_event);
        check_case(fixture->name, "index5/7 slot/dependency store order",
                   c28c_event < control_store_event &&
                       control_store_event < x_event &&
                       x_event < x_store_event && x_store_event < z_event &&
                       z_event < z_store_event &&
                       z_store_event < terrain_event &&
                       terrain_event < y_store_event &&
                       y_store_event < ef_store_event);
        check_case(fixture->name, "index5/7 skips local X/Z reads",
                   trace_address_count(position_address + 2u) == 0 &&
                       trace_address_count(position_address + 4u) == 0);
    } else {
        check_case(fixture->name, "common missing slot store order",
                   control_store_event < y_store_event &&
                       y_store_event < z_store_event &&
                       z_store_event < x_store_event);
    }
}

static ObservedResult run_fixture(const Fixture* fixture)
{
    ExpectedResult expected = expected_for(fixture);
    ObservedResult observed;
    u32 channel;

    check_case(fixture->name, "fixture channel in semantic domain",
               fixture->channel < TEST_CHANNEL_COUNT);
    check_case(fixture->name, "fixture index in jump-table domain",
               fixture->expected_index < 8u);
    prepare_fixture(fixture);

    s_trace_armed = true;
    observed.return_state = wm_8008C364(s_current_slot,
                                        (s32)fixture->channel);
    s_trace_armed = false;
    observed.c28c_calls = s_c28c_calls;
    observed.terrain_calls = s_terrain_calls;
    observed.control = load_u16_raw(s_current_slot + TEST_SLOT_CONTROL);
    observed.x = load_u32_raw(s_current_slot + TEST_SLOT_X);
    observed.y = load_u32_raw(s_current_slot + TEST_SLOT_Y);
    observed.z = load_u32_raw(s_current_slot + TEST_SLOT_Z);
    observed.terrain_x_bits = s_terrain_x_bits;
    observed.terrain_z_bits = s_terrain_z_bits;
    for (channel = 0u; channel < TEST_CHANNEL_COUNT; channel++) {
        u32 position_address = TEST_POSITION_BASE +
                               channel_offset6(channel);
        observed.position_control[channel] =
            load_u16_raw(position_address);
        observed.position_x[channel] = load_u16_raw(position_address + 2u);
        observed.position_z[channel] = load_u16_raw(position_address + 4u);
    }

    check_case(fixture->name, "return state exact",
               observed.return_state == expected.return_state);
    verify_trace_and_dependencies(fixture, &expected);
    verify_global_and_slot_state(fixture, &expected);
    if (fixture->mutate_primary) {
        check_case(fixture->name, "primary changed at second translation",
                   s_primary_translation_count == 2 &&
                       load_u8_raw(TEST_PRIMARY_BASE + fixture->channel) ==
                           fixture->mutated_primary);
    }
    return observed;
}

/* ---------------------------- fixture matrices ------------------------ */

#define FIXTURE(name_, channel_, index_, primary_, gear_, flag_, raw_) \
    { (name_), (channel_), (index_), (primary_), (gear_), (flag_), (raw_), \
      (u16)(0x8123u + (channel_)), (u16)(0xFEDCu - (channel_)), \
      0x89ABCDEFu, 0xFEDCBA98u, 0x87654321u, false, 0u, 0u }

static const Fixture s_index_cases[] = {
    FIXTURE("index-0", 0u, 0u, 0xFFu, 0x11u, 0u,    0x0000u),
    FIXTURE("index-1", 0u, 1u, 0x02u, 0x00u, 0u,    0x03FFu),
    FIXTURE("index-2", 1u, 2u, 0xFFu, 0x22u, 2u,    0x0400u),
    FIXTURE("index-3", 1u, 3u, 0x07u, 0xFFu, 2u,    0x0401u),
    FIXTURE("index-4", 2u, 4u, 0xFFu, 0x33u, 1u,    0x0000u),
    FIXTURE("index-5", 2u, 5u, 0x0Bu, 0xFFu, 1u,    0x03FFu),
    FIXTURE("index-6", 0u, 6u, 0xFFu, 0x44u, 1u,    0x0400u),
    FIXTURE("index-7", 1u, 7u, 0x09u, 0xFFu, 1u,    0x3FFFu),
};

static const Fixture s_primary_ff_cases[] = {
    FIXTURE("primary-ff-channel-0", 0u, 0u, 0xFFu, 0x00u, 0u, 0x0000u),
    FIXTURE("primary-ff-channel-1", 1u, 0u, 0xFFu, 0x01u, 2u, 0x8000u),
    FIXTURE("primary-ff-channel-2", 2u, 0u, 0xFFu, 0xFEu, 0xFFu, 0x4000u),
};

static const Fixture s_index1_gear_cases[] = {
    FIXTURE("index1-gear-ff", 0u, 1u, 0x12u, 0xFFu, 0u, 0x0000u),
    FIXTURE("index1-gear-00", 0u, 1u, 0x12u, 0x00u, 0u, 0x0000u),
    FIXTURE("index1-gear-01", 1u, 1u, 0x23u, 0x01u, 2u, 0x03FFu),
    FIXTURE("index1-gear-fe", 2u, 1u, 0x34u, 0xFEu, 0xFFu, 0x0001u),
};

static const Fixture s_nonindex1_gear_ff_cases[] = {
    FIXTURE("index3-gear-ff", 0u, 3u, 0x41u, 0xFFu, 2u, 0x0400u),
    FIXTURE("index5-gear-ff", 1u, 5u, 0x42u, 0xFFu, 1u, 0x0000u),
    FIXTURE("index7-gear-ff", 2u, 7u, 0x43u, 0xFFu, 1u, 0x0401u),
};

static const Fixture s_threshold_cases[] = {
    FIXTURE("threshold-0000", 0u, 1u, 0x50u, 0u, 0u, 0x0000u),
    FIXTURE("threshold-03ff", 1u, 1u, 0x51u, 0u, 2u, 0x03FFu),
    FIXTURE("threshold-0400", 2u, 3u, 0x52u, 0u, 0xFFu, 0x0400u),
    FIXTURE("threshold-0401", 0u, 3u, 0x53u, 0u, 2u, 0x0401u),
    FIXTURE("threshold-3fff", 1u, 3u, 0x54u, 0u, 0u, 0x3FFFu),
    FIXTURE("threshold-4000", 2u, 1u, 0x55u, 0u, 2u, 0x4000u),
    FIXTURE("threshold-8000", 0u, 1u, 0x56u, 0u, 0xFFu, 0x8000u),
    FIXTURE("threshold-c3ff", 1u, 1u, 0x57u, 0u, 0u, 0xC3FFu),
    FIXTURE("threshold-8400", 2u, 3u, 0x58u, 0u, 2u, 0x8400u),
};

static const Fixture s_flag_cases[] = {
    FIXTURE("flag-00", 0u, 1u, 0x60u, 0u, 0u,    0x0000u),
    FIXTURE("flag-01", 1u, 5u, 0x61u, 0u, 1u,    0x0000u),
    FIXTURE("flag-02", 2u, 1u, 0x62u, 0u, 2u,    0x0000u),
    FIXTURE("flag-ff", 0u, 1u, 0x63u, 0u, 0xFFu, 0x0000u),
};

static const Fixture s_channel_position_cases[] = {
    { "channel0-local", 0u, 3u, 0x70u, 0xFFu, 2u, 0x0400u,
      0x8001u, 0xFFFFu, 0x81234567u, 0xFEDCBA98u, 0x92345678u,
      false, 0u, 0u },
    { "channel1-local", 1u, 3u, 0x71u, 0xFFu, 0xFFu, 0x0401u,
      0x1234u, 0xFEDCu, 0x82345678u, 0xEDCBA987u, 0xA3456789u,
      false, 0u, 0u },
    { "channel2-local", 2u, 3u, 0x72u, 0xFFu, 2u, 0x3FFFu,
      0x7FFFu, 0x8000u, 0x83456789u, 0xDCBA9876u, 0xB456789Au,
      false, 0u, 0u },
};

static const Fixture s_channel_shared_cases[] = {
    { "channel0-shared", 0u, 5u, 0x78u, 0xFFu, 1u, 0x0000u,
      0x1111u, 0xAAAAu, 0x81234567u, 0xFEDCBA98u, 0xC1234567u,
      false, 0u, 0u },
    { "channel1-shared", 1u, 7u, 0x79u, 0xFFu, 1u, 0x0401u,
      0x2222u, 0xBBBBu, 0x92345678u, 0xEDCBA987u, 0xD2345678u,
      false, 0u, 0u },
    { "channel2-shared", 2u, 5u, 0x7Au, 0xFFu, 1u, 0x03FFu,
      0x3333u, 0xCCCCu, 0xA3456789u, 0xDCBA9876u, 0xE3456789u,
      false, 0u, 0u },
};

static const Fixture s_fresh_primary_case = {
    "fresh-primary-reread", 1u, 1u, 0x20u, 0x00u, 2u, 0x0000u,
    0x2468u, 0xACE0u, 0x89ABCDEFu, 0xFEDCBA98u, 0x87654321u,
    true, 0x21u, 0xFFu
};

static const Fixture s_natural_case = {
    "natural-channel-0", 0u, 1u, 0x00u, 0x00u, 0x00u, 0x0000u,
    0x0000u, 0x0000u, 0x00000000u, 0x00000000u, 0x13579BDFu,
    false, 0u, 0u
};

#undef FIXTURE

static void run_fixture_array(const Fixture* fixtures, size_t count)
{
    size_t index;

    for (index = 0u; index < count; index++)
        (void)run_fixture(&fixtures[index]);
}

static u32 mutant_dispatch_missing_mask(const Fixture* fixture)
{
    u32 index = fixture->primary != 0xFFu ? 1u : 0u;
    if ((u32)fixture->raw_position >= 0x0400u)
        index |= 2u;
    if (fixture->flag == 1u)
        index |= 4u;
    return index;
}

static u32 mutant_dispatch_wrong_boundary(const Fixture* fixture)
{
    u32 index = fixture->primary != 0xFFu ? 1u : 0u;
    if (((u32)fixture->raw_position & 0x3FFFu) > 0x0400u)
        index |= 2u;
    if (fixture->flag == 1u)
        index |= 4u;
    return index;
}

static u32 mutant_dispatch_signed_raw(const Fixture* fixture)
{
    u32 index = fixture->primary != 0xFFu ? 1u : 0u;
    s32 signed_raw = fixture->raw_position >= 0x8000u
                         ? (s32)(u32)fixture->raw_position - 0x10000
                         : (s32)(u32)fixture->raw_position;
    if (signed_raw >= 0x0400)
        index |= 2u;
    if (fixture->flag == 1u)
        index |= 4u;
    return index;
}

static u32 mutant_dispatch_flag_nonzero(const Fixture* fixture)
{
    u32 index = fixture->primary != 0xFFu ? 1u : 0u;
    if (((u32)fixture->raw_position & 0x3FFFu) >= 0x0400u)
        index |= 2u;
    if (fixture->flag != 0u)
        index |= 4u;
    return index;
}

static CoreOutcome core_from_observed(const ObservedResult* observed)
{
    CoreOutcome core;
    core.return_state = observed->return_state;
    core.c28c_calls = observed->c28c_calls;
    core.terrain_calls = observed->terrain_calls;
    core.control = observed->control;
    core.x = observed->x;
    core.y = observed->y;
    core.z = observed->z;
    core.terrain_x_bits = observed->terrain_x_bits;
    core.terrain_z_bits = observed->terrain_z_bits;
    return core;
}

static CoreOutcome oracle_core(const Fixture* fixture)
{
    ExpectedResult expected = expected_for(fixture);
    CoreOutcome core;
    core.return_state = expected.return_state;
    core.c28c_calls = expected.c28c_calls;
    core.terrain_calls = expected.terrain_calls;
    core.control = expected.control;
    core.x = expected.x;
    core.y = expected.y;
    core.z = expected.z;
    core.terrain_x_bits = expected.x;
    core.terrain_z_bits = expected.z;
    return core;
}

static bool core_equal(const CoreOutcome* left, const CoreOutcome* right)
{
    return left->return_state == right->return_state &&
           left->c28c_calls == right->c28c_calls &&
           left->terrain_calls == right->terrain_calls &&
           left->control == right->control && left->x == right->x &&
           left->y == right->y && left->z == right->z &&
           left->terrain_x_bits == right->terrain_x_bits &&
           left->terrain_z_bits == right->terrain_z_bits;
}

static CoreOutcome mutant_index3_as_index1(const Fixture* fixture)
{
    CoreOutcome mutant = oracle_core(fixture);
    mutant.terrain_calls = 0;
    mutant.control = 1u;
    mutant.x = 0u;
    mutant.y = 0u;
    mutant.z = 0u;
    mutant.terrain_x_bits = 0u;
    mutant.terrain_z_bits = 0u;
    return mutant;
}

static CoreOutcome mutant_index57_as_index3(const Fixture* fixture)
{
    CoreOutcome mutant = oracle_core(fixture);
    mutant.x = (u32)fixture->channel_x << 12;
    mutant.z = (u32)fixture->channel_z << 12;
    mutant.terrain_x_bits = mutant.x;
    mutant.terrain_z_bits = mutant.z;
    return mutant;
}

static CoreOutcome mutant_terrain_wrong_x(const Fixture* fixture)
{
    CoreOutcome mutant = oracle_core(fixture);
    mutant.terrain_x_bits = mutant.z;
    return mutant;
}

static CoreOutcome mutant_terrain_wrong_z(const Fixture* fixture)
{
    CoreOutcome mutant = oracle_core(fixture);
    mutant.terrain_z_bits = mutant.x;
    return mutant;
}

static CoreOutcome mutant_terrain_omitted(const Fixture* fixture)
{
    CoreOutcome mutant = oracle_core(fixture);
    mutant.terrain_calls = 0;
    mutant.y = 0u;
    return mutant;
}

static CoreOutcome mutant_terrain_wrong_destination(const Fixture* fixture)
{
    CoreOutcome mutant = oracle_core(fixture);
    mutant.x = fixture->terrain_result;
    mutant.y = 0u;
    return mutant;
}

static void run_mutant_detection(const ObservedResult* fresh_result)
{
    ObservedResult index1_observed = run_fixture(&s_index_cases[1]);
    ObservedResult index3_observed =
        run_fixture(&s_channel_position_cases[0]);
    ObservedResult index5_observed =
        run_fixture(&s_channel_shared_cases[0]);
    CoreOutcome index3_actual = core_from_observed(&index3_observed);
    CoreOutcome index5_actual = core_from_observed(&index5_observed);
    CoreOutcome mutant;
    u32 wrong_channel =
        (s_channel_shared_cases[0].channel + 1u) % TEST_CHANNEL_COUNT;

    report_mutant("stale primary cache",
                  (s_fresh_primary_case.gear == 0xFFu ? 3 : 1) !=
                          fresh_result->return_state ||
                      (s_fresh_primary_case.gear == 0xFFu ? 0 : 1) !=
                          fresh_result->c28c_calls);
    report_mutant("missing 0x3FFF mask",
                  mutant_dispatch_missing_mask(&s_threshold_cases[5]) !=
                      s_threshold_cases[5].expected_index);
    report_mutant("wrong 0x0400 boundary",
                  mutant_dispatch_wrong_boundary(&s_threshold_cases[2]) !=
                      s_threshold_cases[2].expected_index);
    report_mutant("signed raw threshold",
                  mutant_dispatch_signed_raw(&s_threshold_cases[8]) !=
                      s_threshold_cases[8].expected_index);
    report_mutant("F8E5 nonzero instead of equality-to-1",
                  mutant_dispatch_flag_nonzero(&s_flag_cases[2]) !=
                      s_flag_cases[2].expected_index);

    mutant = mutant_index3_as_index1(&s_channel_position_cases[0]);
    report_mutant("index3 routed through index1 post-path",
                  !core_equal(&index3_actual, &mutant));
    mutant = mutant_index57_as_index3(&s_channel_shared_cases[0]);
    report_mutant("index5/7 routed through index3 position path",
                  !core_equal(&index5_actual, &mutant));

    mutant = mutant_terrain_wrong_x(&s_channel_position_cases[0]);
    report_mutant("terrain C4AC wrong X source",
                  !core_equal(&index3_actual, &mutant));
    mutant = mutant_terrain_wrong_z(&s_channel_position_cases[0]);
    report_mutant("terrain C4AC wrong Z source",
                  !core_equal(&index3_actual, &mutant));
    mutant = mutant_terrain_omitted(&s_channel_position_cases[0]);
    report_mutant("terrain C4AC omitted call",
                  !core_equal(&index3_actual, &mutant));
    mutant = mutant_terrain_wrong_destination(
        &s_channel_position_cases[0]);
    report_mutant("terrain C4AC wrong return destination",
                  !core_equal(&index3_actual, &mutant));
    mutant = mutant_terrain_wrong_x(&s_channel_shared_cases[0]);
    report_mutant("terrain C4E8 wrong X source",
                  !core_equal(&index5_actual, &mutant));
    mutant = mutant_terrain_wrong_z(&s_channel_shared_cases[0]);
    report_mutant("terrain C4E8 wrong Z source",
                  !core_equal(&index5_actual, &mutant));
    mutant = mutant_terrain_omitted(&s_channel_shared_cases[0]);
    report_mutant("terrain C4E8 omitted call",
                  !core_equal(&index5_actual, &mutant));
    mutant = mutant_terrain_wrong_destination(
        &s_channel_shared_cases[0]);
    report_mutant("terrain C4E8 wrong return destination",
                  !core_equal(&index5_actual, &mutant));

    report_mutant("EF8E literal 0x4000",
                  index5_observed.position_control[
                      s_channel_shared_cases[0].channel] != 0x4000u);
    report_mutant("EF8E wrong channel",
                  index5_observed.position_control[wrong_channel] !=
                      0x0400u);
    report_mutant("EF8E word-width store",
                  index5_observed.position_x[
                      s_channel_shared_cases[0].channel] != 0u);
    report_mutant("EF8E always-on store",
                  index1_observed.position_control[
                      s_index_cases[1].channel] != 0x0400u);
}

int main(void)
{
    ObservedResult fresh_result;

    printf("W34B6-D wm_8008C364 production-linked test\n");

    run_fixture_array(s_index_cases,
                      sizeof(s_index_cases) / sizeof(s_index_cases[0]));
    run_fixture_array(s_primary_ff_cases,
                      sizeof(s_primary_ff_cases) /
                          sizeof(s_primary_ff_cases[0]));
    run_fixture_array(s_index1_gear_cases,
                      sizeof(s_index1_gear_cases) /
                          sizeof(s_index1_gear_cases[0]));
    run_fixture_array(s_nonindex1_gear_ff_cases,
                      sizeof(s_nonindex1_gear_ff_cases) /
                          sizeof(s_nonindex1_gear_ff_cases[0]));
    run_fixture_array(s_threshold_cases,
                      sizeof(s_threshold_cases) /
                          sizeof(s_threshold_cases[0]));
    run_fixture_array(s_flag_cases,
                      sizeof(s_flag_cases) / sizeof(s_flag_cases[0]));
    run_fixture_array(s_channel_position_cases,
                      sizeof(s_channel_position_cases) /
                          sizeof(s_channel_position_cases[0]));
    run_fixture_array(s_channel_shared_cases,
                      sizeof(s_channel_shared_cases) /
                          sizeof(s_channel_shared_cases[0]));

    fresh_result = run_fixture(&s_fresh_primary_case);
    (void)run_fixture(&s_natural_case);
    check_case("natural-channel-0", "accepted BDF8[0] fixture",
               load_u32_raw(TEST_PACKAGE_BASE) == s_package_bits[0]);

    run_mutant_detection(&fresh_result);

    printf("PASS/TOTAL: %d/%d\n", s_pass_count, s_total_count);
    return s_failure_count == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
