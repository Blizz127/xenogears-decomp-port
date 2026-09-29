/*
 * W34B6-F production-linked certification test for world callback
 * 0x8008C530.
 *
 * The production translation unit is included below only so this test can
 * replace PSX_ADDR with an address/order trace.  Expected memory images are
 * built from declarative retail route fixtures and the audited write table;
 * the test does not contain a second executable C530 implementation.
 * wm_8008C364 and wm_80093978 are controlled recorders.
 *
 * Example strict build from the repository root:
 *
 *   gcc -std=gnu17 -O0 -g -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
 *     -fno-pie -no-pie \
 *     -Wall -Wextra -Wconversion -Wsign-conversion -Werror \
 *     -Ipc_port/tests/include -Ipc_port/include_shim -Iinclude -Ipc_port/src \
 *     pc_port/tests/w34b6f_8008c530_prod_test.c \
 *     -o scratchpad/w34b6f_c530_implementation/c530_o0
 *
 * Replace -O0 with -O2 for the optimized gate.  Add
 * -fsanitize=undefined -fno-sanitize-recover=undefined for the UB gate.
 */
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"

static void* test_psx_addr(uintptr_t address_bits) __attribute__((noinline));

#undef PSX_ADDR
#define PSX_ADDR(address) test_psx_addr((uintptr_t)(address))

#include "../src/world_map_callback_8c530.c"

#define TEST_RAM_MASK          0x001FFFFFu
#define TEST_POOL_PTR          0x8009BE24u
#define TEST_MODE              0x8009BE10u
#define TEST_EE5A              0x8006EE5Au
#define TEST_F8E5              0x8006F8E5u
#define TEST_F8E6              0x8006F8E6u
#define TEST_F8E7              0x8006F8E7u
#define TEST_C5AC              0x8009C5ACu
#define TEST_C5B4              0x8009C5B4u
#define TEST_C584              0x8009C584u
#define TEST_D55C              0x8009D55Cu
#define TEST_D560              0x8009D560u
#define TEST_D564              0x8009D564u
#define TEST_D568              0x8009D568u
#define TEST_D154              0x8009D154u
#define TEST_D52C              0x8009D52Cu
#define TEST_HISTORY_BASE      0x8009CEC4u
#define TEST_HISTORY_STRIDE    0x14u
#define TEST_HISTORY_ROWS      32u
#define TEST_HISTORY_BYTES     (TEST_HISTORY_STRIDE * TEST_HISTORY_ROWS)
#define TEST_EF90              0x8006EF90u
#define TEST_EF92              0x8006EF92u
#define TEST_WRONG_EF90        0x8007EF90u
#define TEST_WRONG_EF92        0x8007EF92u
#define TEST_D940_BASE         0x8006D940u
#define TEST_BDF8_BASE         0x8009BDF8u
#define TEST_POOL_BASE         0x80180000u
#define TEST_SLOT_STRIDE       0x80u
#define TEST_SLOT_INDEX        4
#define TEST_SLOT_STATE        0x00u
#define TEST_SLOT_CONTROL      0x20u
#define TEST_SLOT_FLAG         0x24u
#define TEST_SLOT_X            0x28u
#define TEST_SLOT_Y            0x2Cu
#define TEST_SLOT_Z            0x30u
#define TEST_SLOT_AUX          0x34u
#define TEST_SLOT_CLEAR38      0x38u
#define TEST_SLOT_CLEAR3C      0x3Cu
#define TEST_SLOT_CLEAR40      0x40u
#define TEST_SLOT_HEADING      0x48u
#define TEST_SLOT_CONST        0x4Au
#define TEST_SLOT_OBJECT       0x4Cu
#define TEST_SLOT_SIGNED       0x5Cu
#define TEST_TRACE_CAPACITY    2048u
#define TEST_OBJECT_TOKEN      0x0C5304C0u

_Static_assert(PSX_RAM_SIZE >= (2 * 1024 * 1024),
               "test requires complete PSX main RAM");

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

typedef enum TraceKind {
    TRACE_ADDRESS = 1,
    TRACE_C364,
    TRACE_TERRAIN
} TraceKind;

typedef struct TraceEvent {
    TraceKind kind;
    u32 value;
} TraceEvent;

typedef enum RouteFamily {
    ROUTE_COMMON = 0,
    ROUTE_EXTENDED,
    ROUTE_MODE_4_7
} RouteFamily;

typedef struct Fixture {
    const char* name;
    s32 mode;
    u8 f8e5;
    RouteFamily route;
    bool reads_f8e5;
    s32 c364_return;
    u16 initial_heading;
    u16 c364_flag;
    u32 c364_x;
    u32 c364_y;
    u32 c364_z;
    u32 c364_aux;
    u32 c364_object;
    u32 shared_x;
    u32 shared_z;
    u32 c584;
    u32 terrain_return;
} Fixture;

typedef struct RunResult {
    s32 return_value;
    uint64_t ram_hash;
    int terrain_calls;
} RunResult;

static u8 s_ram_before[PSX_RAM_SIZE];
static u8 s_ram_expected[PSX_RAM_SIZE];
static u8 s_scratch_before[sizeof(g_PsxScratchpad)];
static TraceEvent s_trace[TEST_TRACE_CAPACITY];
static size_t s_trace_count;
static bool s_trace_overflow;
static bool s_trace_armed;
static const Fixture* s_active;
static u32 s_slot_addr;

static int s_c364_calls;
static u32 s_c364_slot;
static s32 s_c364_channel;
static bool s_c364_saw_pristine_slot;
static int s_terrain_calls;
static u32 s_terrain_x;
static u32 s_terrain_z;

static int s_pass_count;
static int s_total_count;
static int s_failure_count;

static size_t raw_index(u32 address)
{
    return (size_t)(address & TEST_RAM_MASK);
}

static void* raw_address(u32 address)
{
    return (void*)(g_PsxRam + raw_index(address));
}

static const void* before_address(u32 address)
{
    return (const void*)(s_ram_before + raw_index(address));
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

    if (s_trace_armed)
        trace_append(TRACE_ADDRESS, address);
    return raw_address(address);
}

static void buffer_store_u8(u8* buffer, u32 address, u8 value)
{
    buffer[raw_index(address)] = value;
}

static void buffer_store_u16(u8* buffer, u32 address, u16 value)
{
    memcpy(buffer + raw_index(address), &value, sizeof(value));
}

static void buffer_store_u32(u8* buffer, u32 address, u32 value)
{
    memcpy(buffer + raw_index(address), &value, sizeof(value));
}

static u8 buffer_load_u8(const u8* buffer, u32 address)
{
    return buffer[raw_index(address)];
}

static u16 buffer_load_u16(const u8* buffer, u32 address)
{
    u16 value;

    memcpy(&value, buffer + raw_index(address), sizeof(value));
    return value;
}

static u32 buffer_load_u32(const u8* buffer, u32 address)
{
    u32 value;

    memcpy(&value, buffer + raw_index(address), sizeof(value));
    return value;
}

static void raw_store_u8(u32 address, u8 value)
{
    buffer_store_u8(g_PsxRam, address, value);
}

static void raw_store_u16(u32 address, u16 value)
{
    buffer_store_u16(g_PsxRam, address, value);
}

static void raw_store_u32(u32 address, u32 value)
{
    buffer_store_u32(g_PsxRam, address, value);
}

static u8 raw_load_u8(u32 address)
{
    return buffer_load_u8(g_PsxRam, address);
}

static u16 raw_load_u16(u32 address)
{
    return buffer_load_u16(g_PsxRam, address);
}

static u32 raw_load_u32(u32 address)
{
    return buffer_load_u32(g_PsxRam, address);
}

static u32 s32_as_u32(s32 value)
{
    u32 bits;

    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

static s32 u32_as_s32(u32 bits)
{
    s32 value;

    memcpy(&value, &bits, sizeof(value));
    return value;
}

static u32 sign_extend_u16_bits(u16 value)
{
    if ((value & 0x8000u) == 0u)
        return (u32)value;
    return 0xFFFF0000u | (u32)value;
}

/* Independent bit-level definition of MIPS SRA by 12. */
static u32 oracle_sra12(u32 value)
{
    u32 shifted = value >> 12;

    if ((value & 0x80000000u) != 0u)
        shifted |= 0xFFF00000u;
    return shifted;
}

static uint64_t hash_ram(void)
{
    uint64_t hash = UINT64_C(1469598103934665603);
    size_t index;

    for (index = 0u; index < (size_t)PSX_RAM_SIZE; index++) {
        hash ^= (uint64_t)g_PsxRam[index];
        hash *= UINT64_C(1099511628211);
    }
    return hash;
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

static void report_mutant(const char* name, bool detected)
{
    check_case("mutant-detection", name, detected);
    if (detected)
        printf("MUTANT DETECTED: %s\n", name);
}

static size_t trace_index(TraceKind kind, u32 value, int occurrence)
{
    size_t index;
    size_t available = s_trace_count;
    int seen = 0;

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

static int trace_address_count(u32 address)
{
    size_t index;
    size_t available = s_trace_count;
    int count = 0;

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

static int trace_address_range_count(u32 start, u32 width)
{
    size_t index;
    size_t available = s_trace_count;
    u32 end = start + width;
    int count = 0;

    if (available > (size_t)TEST_TRACE_CAPACITY)
        available = (size_t)TEST_TRACE_CAPACITY;
    for (index = 0u; index < available; index++) {
        u32 address = s_trace[index].value;

        if (s_trace[index].kind == TRACE_ADDRESS && address >= start &&
            address < end) {
            count++;
        }
    }
    return count;
}

static int trace_address_total(void)
{
    size_t index;
    size_t available = s_trace_count;
    int count = 0;

    if (available > (size_t)TEST_TRACE_CAPACITY)
        available = (size_t)TEST_TRACE_CAPACITY;
    for (index = 0u; index < available; index++) {
        if (s_trace[index].kind == TRACE_ADDRESS)
            count++;
    }
    return count;
}

static bool history_field_address(u32 address)
{
    u32 relative;
    u32 field;

    if (address < TEST_HISTORY_BASE ||
        address >= TEST_HISTORY_BASE + TEST_HISTORY_BYTES) {
        return false;
    }
    relative = address - TEST_HISTORY_BASE;
    field = relative % TEST_HISTORY_STRIDE;
    return field == 0x00u || field == 0x04u || field == 0x08u ||
           field == 0x0Cu || field == 0x10u;
}

static bool trace_address_authorized(u32 address, const Fixture* fixture)
{
    if (address == TEST_POOL_PTR || address == TEST_EE5A ||
        address == TEST_MODE || address == TEST_EF90 ||
        address == TEST_EF92 ||
        address == s_slot_addr + TEST_SLOT_CLEAR40 ||
        address == s_slot_addr + TEST_SLOT_CLEAR3C ||
        address == s_slot_addr + TEST_SLOT_CLEAR38 ||
        address == s_slot_addr + TEST_SLOT_HEADING ||
        address == s_slot_addr + TEST_SLOT_CONST ||
        address == s_slot_addr + TEST_SLOT_SIGNED ||
        address == s_slot_addr + TEST_SLOT_X ||
        address == s_slot_addr + TEST_SLOT_Z) {
        return true;
    }
    if (fixture->reads_f8e5 && address == TEST_F8E5)
        return true;
    if (fixture->route == ROUTE_MODE_4_7 &&
        (address == s_slot_addr + TEST_SLOT_CONTROL ||
         address == s_slot_addr + TEST_SLOT_FLAG)) {
        return true;
    }
    if (fixture->route != ROUTE_EXTENDED)
        return false;
    if (address == s_slot_addr + TEST_SLOT_CONTROL ||
        address == s_slot_addr + TEST_SLOT_Y ||
        address == s_slot_addr + TEST_SLOT_AUX ||
        address == TEST_C5AC || address == TEST_C5B4 ||
        address == TEST_C584 || address == TEST_D55C ||
        address == TEST_D560 || address == TEST_D564 ||
        address == TEST_D568 || address == TEST_D154 ||
        address == TEST_D52C || history_field_address(address)) {
        return true;
    }
    return false;
}

static bool all_trace_addresses_authorized(const Fixture* fixture)
{
    size_t index;
    size_t available = s_trace_count;

    if (available > (size_t)TEST_TRACE_CAPACITY)
        available = (size_t)TEST_TRACE_CAPACITY;
    for (index = 0u; index < available; index++) {
        if (s_trace[index].kind == TRACE_ADDRESS &&
            !trace_address_authorized(s_trace[index].value, fixture)) {
            return false;
        }
    }
    return true;
}

static bool trace_has_render_or_scratch_address(void)
{
    size_t index;
    size_t available = s_trace_count;

    if (available > (size_t)TEST_TRACE_CAPACITY)
        available = (size_t)TEST_TRACE_CAPACITY;
    for (index = 0u; index < available; index++) {
        u32 address = s_trace[index].value;

        if (s_trace[index].kind != TRACE_ADDRESS)
            continue;
        if ((address >= 0x1F800000u && address < 0x1F803000u) ||
            (address >= 0x1F801810u && address < 0x1F801818u)) {
            return true;
        }
    }
    return false;
}

/* ------------------------- controlled dependencies ------------------ */

s32 wm_8008C364(u32 slot_addr, s32 channel)
{
    trace_append(TRACE_C364, slot_addr);
    s_c364_calls++;
    s_c364_slot = slot_addr;
    s_c364_channel = channel;
    s_c364_saw_pristine_slot =
        memcmp(raw_address(slot_addr), before_address(slot_addr),
               (size_t)TEST_SLOT_STRIDE) == 0;

    raw_store_u16(slot_addr + TEST_SLOT_FLAG, s_active->c364_flag);
    raw_store_u32(slot_addr + TEST_SLOT_X, s_active->c364_x);
    raw_store_u32(slot_addr + TEST_SLOT_Y, s_active->c364_y);
    raw_store_u32(slot_addr + TEST_SLOT_Z, s_active->c364_z);
    raw_store_u32(slot_addr + TEST_SLOT_AUX, s_active->c364_aux);
    raw_store_u32(slot_addr + TEST_SLOT_OBJECT, s_active->c364_object);
    return s_active->c364_return;
}

s32 wm_80093978(s32 x, s32 z)
{
    trace_append(TRACE_TERRAIN, 0u);
    s_terrain_calls++;
    s_terrain_x = s32_as_u32(x);
    s_terrain_z = s32_as_u32(z);
    return u32_as_s32(s_active->terrain_return);
}

/* --------------------------- declarative oracle --------------------- */

static void apply_c364_expected(const Fixture* fixture)
{
    buffer_store_u16(s_ram_expected, s_slot_addr + TEST_SLOT_FLAG,
                     fixture->c364_flag);
    buffer_store_u32(s_ram_expected, s_slot_addr + TEST_SLOT_X,
                     fixture->c364_x);
    buffer_store_u32(s_ram_expected, s_slot_addr + TEST_SLOT_Y,
                     fixture->c364_y);
    buffer_store_u32(s_ram_expected, s_slot_addr + TEST_SLOT_Z,
                     fixture->c364_z);
    buffer_store_u32(s_ram_expected, s_slot_addr + TEST_SLOT_AUX,
                     fixture->c364_aux);
    buffer_store_u32(s_ram_expected, s_slot_addr + TEST_SLOT_OBJECT,
                     fixture->c364_object);
}

static void build_expected_ram(const Fixture* fixture)
{
    u32 x;
    u32 y;
    u32 z;
    u32 aux;
    u16 heading;
    u32 row;

    memcpy(s_ram_expected, s_ram_before, sizeof(s_ram_expected));
    apply_c364_expected(fixture);

    buffer_store_u32(s_ram_expected, s_slot_addr + TEST_SLOT_CLEAR40, 0u);
    buffer_store_u32(s_ram_expected, s_slot_addr + TEST_SLOT_CLEAR3C, 0u);
    buffer_store_u32(s_ram_expected, s_slot_addr + TEST_SLOT_CLEAR38, 0u);
    buffer_store_u16(s_ram_expected, s_slot_addr + TEST_SLOT_HEADING,
                     fixture->initial_heading);
    buffer_store_u16(s_ram_expected, s_slot_addr + TEST_SLOT_CONST, 12u);
    buffer_store_u32(s_ram_expected, s_slot_addr + TEST_SLOT_SIGNED,
                     sign_extend_u16_bits(fixture->initial_heading));

    if (fixture->route == ROUTE_EXTENDED) {
        buffer_store_u16(s_ram_expected, s_slot_addr + TEST_SLOT_CONTROL, 1u);
        buffer_store_u32(s_ram_expected, s_slot_addr + TEST_SLOT_X,
                         fixture->shared_x);
        buffer_store_u32(s_ram_expected, s_slot_addr + TEST_SLOT_Z,
                         fixture->shared_z);
        buffer_store_u32(s_ram_expected, s_slot_addr + TEST_SLOT_Y,
                         fixture->terrain_return);
        buffer_store_u16(s_ram_expected, s_slot_addr + TEST_SLOT_HEADING,
                         (u16)fixture->c584);

        x = buffer_load_u32(s_ram_expected, s_slot_addr + TEST_SLOT_X);
        y = buffer_load_u32(s_ram_expected, s_slot_addr + TEST_SLOT_Y);
        z = buffer_load_u32(s_ram_expected, s_slot_addr + TEST_SLOT_Z);
        aux = buffer_load_u32(s_ram_expected, s_slot_addr + TEST_SLOT_AUX);
        heading = buffer_load_u16(s_ram_expected,
                                  s_slot_addr + TEST_SLOT_HEADING);
        buffer_store_u32(s_ram_expected, TEST_D55C, x);
        buffer_store_u32(s_ram_expected, TEST_D560, y);
        buffer_store_u32(s_ram_expected, TEST_D564, z);
        buffer_store_u32(s_ram_expected, TEST_D568, aux);
        buffer_store_u16(s_ram_expected, TEST_D154, 0u);
        buffer_store_u16(s_ram_expected, TEST_D52C, heading);

        for (row = 0u; row < TEST_HISTORY_ROWS; row++) {
            u32 record = TEST_HISTORY_BASE + row * TEST_HISTORY_STRIDE;

            buffer_store_u32(s_ram_expected, record + 0x00u, x);
            buffer_store_u32(s_ram_expected, record + 0x04u, y);
            buffer_store_u32(s_ram_expected, record + 0x08u, z);
            buffer_store_u32(s_ram_expected, record + 0x0Cu, aux);
            buffer_store_u16(s_ram_expected, record + 0x10u, heading);
        }
    } else if (fixture->route == ROUTE_MODE_4_7) {
        buffer_store_u16(s_ram_expected, s_slot_addr + TEST_SLOT_CONTROL, 2u);
        buffer_store_u16(s_ram_expected, s_slot_addr + TEST_SLOT_FLAG, 1u);
    }

    x = buffer_load_u32(s_ram_expected, s_slot_addr + TEST_SLOT_X);
    z = buffer_load_u32(s_ram_expected, s_slot_addr + TEST_SLOT_Z);
    heading = buffer_load_u16(s_ram_expected,
                              s_slot_addr + TEST_SLOT_HEADING);
    buffer_store_u16(s_ram_expected, TEST_EF90, (u16)oracle_sra12(x));
    buffer_store_u16(s_ram_expected, TEST_EF92, (u16)oracle_sra12(z));
    buffer_store_u16(s_ram_expected, TEST_EE5A, heading);
}

static void fill_slot_region(void)
{
    u32 offset;
    u32 start = TEST_POOL_BASE - TEST_SLOT_STRIDE;

    for (offset = 0u; offset < TEST_SLOT_STRIDE * 9u; offset++) {
        raw_store_u8(start + offset,
                     (u8)(0x31u + (offset * 37u + (offset >> 3))));
    }
}

static void prepare_fixture(const Fixture* fixture)
{
    memset(g_PsxRam, 0xA5, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0x5A, sizeof(g_PsxScratchpad));
    fill_slot_region();
    s_slot_addr = TEST_POOL_BASE + ((u32)TEST_SLOT_INDEX << 7);

    raw_store_u32(TEST_POOL_PTR, TEST_POOL_BASE);
    raw_store_u32(TEST_MODE, s32_as_u32(fixture->mode));
    raw_store_u16(TEST_EE5A, fixture->initial_heading);
    raw_store_u8(TEST_F8E5, fixture->f8e5);
    raw_store_u8(TEST_F8E6, 0x62u);
    raw_store_u8(TEST_F8E7, 0xE7u);
    raw_store_u32(TEST_C5AC, fixture->shared_x);
    raw_store_u32(TEST_C5B4, fixture->shared_z);
    raw_store_u32(TEST_C584, fixture->c584);

    raw_store_u16(TEST_EF90, 0xE190u);
    raw_store_u16(TEST_EF92, 0xE192u);
    raw_store_u16(TEST_WRONG_EF90, 0x7190u);
    raw_store_u16(TEST_WRONG_EF92, 0x7192u);
    raw_store_u32(TEST_D55C, 0xD55CD55Cu);
    raw_store_u32(TEST_D560, 0xD560D560u);
    raw_store_u32(TEST_D564, 0xD564D564u);
    raw_store_u32(TEST_D568, 0xD568D568u);
    raw_store_u16(TEST_D154, 0xD154u);
    raw_store_u16(TEST_D52C, 0xD52Cu);
    raw_store_u32(TEST_D940_BASE, 0xD940D940u);
    raw_store_u32(TEST_BDF8_BASE, 0xBDF8BDF8u);

    s_active = fixture;
    s_trace_count = 0u;
    s_trace_overflow = false;
    s_trace_armed = false;
    s_c364_calls = 0;
    s_c364_slot = 0u;
    s_c364_channel = -1;
    s_c364_saw_pristine_slot = false;
    s_terrain_calls = 0;
    s_terrain_x = 0u;
    s_terrain_z = 0u;

    memcpy(s_ram_before, g_PsxRam, sizeof(s_ram_before));
    memcpy(s_scratch_before, g_PsxScratchpad, sizeof(s_scratch_before));
    build_expected_ram(fixture);
}

static bool ram_matches_expected(const char* case_name)
{
    size_t index;

    for (index = 0u; index < (size_t)PSX_RAM_SIZE; index++) {
        if (g_PsxRam[index] != s_ram_expected[index]) {
            printf("FAIL [%s]: RAM mismatch at raw 0x%06lX "
                   "actual=%02X expected=%02X\n",
                   case_name, (unsigned long)index,
                   (unsigned int)g_PsxRam[index],
                   (unsigned int)s_ram_expected[index]);
            return false;
        }
    }
    return true;
}

static void verify_initial_order(const Fixture* fixture)
{
    size_t pool = trace_index(TRACE_ADDRESS, TEST_POOL_PTR, 1);
    size_t c364 = trace_index(TRACE_C364, s_slot_addr, 1);
    size_t clear40 = trace_index(TRACE_ADDRESS,
                                 s_slot_addr + TEST_SLOT_CLEAR40, 1);
    size_t clear3c = trace_index(TRACE_ADDRESS,
                                 s_slot_addr + TEST_SLOT_CLEAR3C, 1);
    size_t clear38 = trace_index(TRACE_ADDRESS,
                                 s_slot_addr + TEST_SLOT_CLEAR38, 1);
    size_t ee5a_read = trace_index(TRACE_ADDRESS, TEST_EE5A, 1);
    size_t heading_store = trace_index(TRACE_ADDRESS,
                                       s_slot_addr + TEST_SLOT_HEADING, 1);
    size_t heading_reload = trace_index(TRACE_ADDRESS,
                                        s_slot_addr + TEST_SLOT_HEADING, 2);
    size_t constant_store = trace_index(TRACE_ADDRESS,
                                        s_slot_addr + TEST_SLOT_CONST, 1);
    size_t mode_read = trace_index(TRACE_ADDRESS, TEST_MODE, 1);
    size_t signed_store = trace_index(TRACE_ADDRESS,
                                      s_slot_addr + TEST_SLOT_SIGNED, 1);

    check_case(fixture->name, "retail initial access order",
               pool < c364 && c364 < clear40 && clear40 < clear3c &&
                   clear3c < clear38 && clear38 < ee5a_read &&
                   ee5a_read < heading_store &&
                   heading_store < heading_reload &&
                   heading_reload < constant_store &&
                   constant_store < mode_read && mode_read < signed_store);
}

static void verify_access_gates(const Fixture* fixture)
{
    int expected_flag_reads = fixture->reads_f8e5 ? 1 : 0;
    int expected_c5_reads = fixture->route == ROUTE_EXTENDED ? 1 : 0;
    int expected_history_accesses =
        fixture->route == ROUTE_EXTENDED ? 160 : 0;
    int expected_address_total = 16 + expected_flag_reads;

    if (fixture->route == ROUTE_EXTENDED)
        expected_address_total = 357;
    else if (fixture->route == ROUTE_MODE_4_7)
        expected_address_total += 2;

    check_case(fixture->name, "all translated addresses retail-authorized",
               all_trace_addresses_authorized(fixture));
    check_case(fixture->name, "complete translated access count exact",
               trace_address_total() == expected_address_total);
    check_case(fixture->name, "pool-pointer access count exact",
               trace_address_count(TEST_POOL_PTR) == 1);
    check_case(fixture->name, "clear-field access counts exact",
               trace_address_count(s_slot_addr + TEST_SLOT_CLEAR40) == 1 &&
                   trace_address_count(s_slot_addr + TEST_SLOT_CLEAR3C) == 1 &&
                   trace_address_count(s_slot_addr + TEST_SLOT_CLEAR38) == 1);
    check_case(fixture->name, "heading access counts exact",
               trace_address_count(TEST_EE5A) == 2 &&
                   trace_address_count(s_slot_addr + TEST_SLOT_HEADING) ==
                       (fixture->route == ROUTE_EXTENDED ? 37 : 3));
    check_case(fixture->name, "X/Z access counts exact",
               trace_address_count(s_slot_addr + TEST_SLOT_X) ==
                       (fixture->route == ROUTE_EXTENDED ? 36 : 1) &&
                   trace_address_count(s_slot_addr + TEST_SLOT_Z) ==
                       (fixture->route == ROUTE_EXTENDED ? 35 : 1));
    check_case(fixture->name, "Y/+34 access counts exact",
               trace_address_count(s_slot_addr + TEST_SLOT_Y) ==
                       (fixture->route == ROUTE_EXTENDED ? 34 : 0) &&
                   trace_address_count(s_slot_addr + TEST_SLOT_AUX) ==
                       (fixture->route == ROUTE_EXTENDED ? 33 : 0));
    check_case(fixture->name, "F8E5 read gate exact",
               trace_address_count(TEST_F8E5) == expected_flag_reads);
    check_case(fixture->name, "F8E6 never accessed",
               trace_address_count(TEST_F8E6) == 0);
    check_case(fixture->name, "F8E7 never accessed",
               trace_address_count(TEST_F8E7) == 0);
    check_case(fixture->name, "C5AC read gate exact",
               trace_address_count(TEST_C5AC) == expected_c5_reads);
    check_case(fixture->name, "C5B4 read gate exact",
               trace_address_count(TEST_C5B4) == expected_c5_reads);
    check_case(fixture->name, "C584 read gate exact",
               trace_address_count(TEST_C584) == expected_c5_reads);
    check_case(fixture->name, "history destination access count exact",
               trace_address_range_count(TEST_HISTORY_BASE,
                                         TEST_HISTORY_BYTES) ==
                   expected_history_accesses);
    check_case(fixture->name, "slot+4C direct access count zero",
               trace_address_range_count(s_slot_addr + TEST_SLOT_OBJECT,
                                         4u) == 0);
    check_case(fixture->name, "slot+00 direct access count zero",
               trace_address_range_count(s_slot_addr + TEST_SLOT_STATE,
                                         2u) == 0);
    check_case(fixture->name, "D940 direct access count zero",
               trace_address_range_count(TEST_D940_BASE, 0xA4u) == 0);
    check_case(fixture->name, "BDF8 direct access count zero",
               trace_address_range_count(TEST_BDF8_BASE, 12u) == 0);
    check_case(fixture->name, "no scratchpad/GPU address translation",
               !trace_has_render_or_scratch_address());
}

static void verify_extended_path(const Fixture* fixture)
{
    u32 row;
    u16 heading = (u16)fixture->c584;
    size_t previous_row_end;
    size_t flag = trace_index(TRACE_ADDRESS, TEST_F8E5, 1);
    size_t control = trace_index(TRACE_ADDRESS,
                                 s_slot_addr + TEST_SLOT_CONTROL, 1);
    size_t shared_x = trace_index(TRACE_ADDRESS, TEST_C5AC, 1);
    size_t slot_x_store = trace_index(TRACE_ADDRESS,
                                      s_slot_addr + TEST_SLOT_X, 1);
    size_t shared_z = trace_index(TRACE_ADDRESS, TEST_C5B4, 1);
    size_t slot_x_reload = trace_index(TRACE_ADDRESS,
                                       s_slot_addr + TEST_SLOT_X, 2);
    size_t slot_z_store = trace_index(TRACE_ADDRESS,
                                      s_slot_addr + TEST_SLOT_Z, 1);
    size_t terrain = trace_index(TRACE_TERRAIN, 0u, 1);
    size_t c584 = trace_index(TRACE_ADDRESS, TEST_C584, 1);
    size_t slot_y_store = trace_index(TRACE_ADDRESS,
                                      s_slot_addr + TEST_SLOT_Y, 1);
    size_t heading_replace = trace_index(TRACE_ADDRESS,
                                         s_slot_addr + TEST_SLOT_HEADING, 3);
    size_t snapshot_x_load = trace_index(TRACE_ADDRESS,
                                         s_slot_addr + TEST_SLOT_X, 3);
    size_t snapshot_y_load = trace_index(TRACE_ADDRESS,
                                         s_slot_addr + TEST_SLOT_Y, 2);
    size_t snapshot_z_load = trace_index(TRACE_ADDRESS,
                                         s_slot_addr + TEST_SLOT_Z, 2);
    size_t publish_x = trace_index(TRACE_ADDRESS, TEST_D55C, 1);
    size_t publish_y = trace_index(TRACE_ADDRESS, TEST_D560, 1);
    size_t publish_z = trace_index(TRACE_ADDRESS, TEST_D564, 1);
    size_t snapshot_aux_load = trace_index(TRACE_ADDRESS,
                                           s_slot_addr + TEST_SLOT_AUX, 1);
    size_t publish_aux = trace_index(TRACE_ADDRESS, TEST_D568, 1);
    size_t snapshot_heading_load = trace_index(
        TRACE_ADDRESS, s_slot_addr + TEST_SLOT_HEADING, 4);
    size_t publish_clear = trace_index(TRACE_ADDRESS, TEST_D154, 1);
    size_t publish_heading = trace_index(TRACE_ADDRESS, TEST_D52C, 1);

    check_case(fixture->name, "extended dependency/store order",
               flag < control && control < shared_x &&
                   shared_x < slot_x_store && slot_x_store < shared_z &&
                   shared_z < slot_x_reload && slot_x_reload < slot_z_store &&
                   slot_z_store < terrain && terrain < c584 &&
                   c584 < slot_y_store && slot_y_store < heading_replace);
    check_case(fixture->name, "extended snapshot publication order exact",
               heading_replace < snapshot_x_load &&
                   snapshot_x_load < snapshot_y_load &&
                   snapshot_y_load < snapshot_z_load &&
                   snapshot_z_load < publish_x && publish_x < publish_y &&
                   publish_y < publish_z && publish_z < snapshot_aux_load &&
                   snapshot_aux_load < publish_aux &&
                   publish_aux < snapshot_heading_load &&
                   snapshot_heading_load < publish_clear &&
                   publish_clear < publish_heading);
    check_case(fixture->name, "terrain called exactly once",
               s_terrain_calls == 1);
    check_case(fixture->name, "terrain X exact raw bits",
               s_terrain_x == fixture->shared_x);
    check_case(fixture->name, "terrain Z exact raw bits",
               s_terrain_z == fixture->shared_z);
    check_case(fixture->name, "terrain result only at slot Y",
               raw_load_u32(s_slot_addr + TEST_SLOT_Y) ==
                       fixture->terrain_return &&
                   raw_load_u32(s_slot_addr + TEST_SLOT_X) ==
                       fixture->shared_x &&
                   raw_load_u32(s_slot_addr + TEST_SLOT_Z) ==
                       fixture->shared_z);
    check_case(fixture->name, "C584 low16 replaces +48",
               raw_load_u16(s_slot_addr + TEST_SLOT_HEADING) == heading);
    check_case(fixture->name, "initial heading remains sign-extended at +5C",
               raw_load_u32(s_slot_addr + TEST_SLOT_SIGNED) ==
                   sign_extend_u16_bits(fixture->initial_heading));

    check_case(fixture->name, "D55C snapshot X exact",
               raw_load_u32(TEST_D55C) == fixture->shared_x);
    check_case(fixture->name, "D560 snapshot Y exact",
               raw_load_u32(TEST_D560) == fixture->terrain_return);
    check_case(fixture->name, "D564 snapshot Z exact",
               raw_load_u32(TEST_D564) == fixture->shared_z);
    check_case(fixture->name, "D568 snapshot +34 exact",
               raw_load_u32(TEST_D568) == fixture->c364_aux);
    check_case(fixture->name, "D154 exact halfword clear",
               raw_load_u16(TEST_D154) == 0u);
    check_case(fixture->name, "D52C exact unsigned heading publication",
               raw_load_u16(TEST_D52C) == heading);

    previous_row_end = publish_heading;
    for (row = 0u; row < TEST_HISTORY_ROWS; row++) {
        u32 record = TEST_HISTORY_BASE + row * TEST_HISTORY_STRIDE;
        size_t row_x_load = trace_index(
            TRACE_ADDRESS, s_slot_addr + TEST_SLOT_X, (int)(4u + row));
        size_t row_y_load = trace_index(
            TRACE_ADDRESS, s_slot_addr + TEST_SLOT_Y, (int)(3u + row));
        size_t row_z_load = trace_index(
            TRACE_ADDRESS, s_slot_addr + TEST_SLOT_Z, (int)(3u + row));
        size_t row_aux_load = trace_index(
            TRACE_ADDRESS, s_slot_addr + TEST_SLOT_AUX, (int)(2u + row));
        size_t row_x_store = trace_index(TRACE_ADDRESS, record + 0x00u, 1);
        size_t row_y_store = trace_index(TRACE_ADDRESS, record + 0x04u, 1);
        size_t row_z_store = trace_index(TRACE_ADDRESS, record + 0x08u, 1);
        size_t row_aux_store = trace_index(TRACE_ADDRESS, record + 0x0Cu, 1);
        size_t row_heading_load = trace_index(
            TRACE_ADDRESS, s_slot_addr + TEST_SLOT_HEADING,
            (int)(5u + row));
        size_t row_heading_store = trace_index(TRACE_ADDRESS,
                                               record + 0x10u, 1);

        check_case(fixture->name, "history row X exact",
                   raw_load_u32(record + 0x00u) == fixture->shared_x);
        check_case(fixture->name, "history row Y exact",
                   raw_load_u32(record + 0x04u) ==
                       fixture->terrain_return);
        check_case(fixture->name, "history row Z exact",
                   raw_load_u32(record + 0x08u) == fixture->shared_z);
        check_case(fixture->name, "history row +34 exact",
                   raw_load_u32(record + 0x0Cu) == fixture->c364_aux);
        check_case(fixture->name, "history row heading exact",
                   raw_load_u16(record + 0x10u) == heading);
        check_case(fixture->name, "history row has exactly five stores",
                   trace_address_count(record + 0x00u) == 1 &&
                       trace_address_count(record + 0x04u) == 1 &&
                       trace_address_count(record + 0x08u) == 1 &&
                       trace_address_count(record + 0x0Cu) == 1 &&
                       trace_address_count(record + 0x10u) == 1);
        check_case(fixture->name, "history row access order exact",
                   previous_row_end < row_x_load &&
                       row_x_load < row_y_load && row_y_load < row_z_load &&
                       row_z_load < row_aux_load &&
                       row_aux_load < row_x_store &&
                       row_x_store < row_y_store &&
                       row_y_store < row_z_store &&
                       row_z_store < row_aux_store &&
                       row_aux_store < row_heading_load &&
                       row_heading_load < row_heading_store);
        previous_row_end = row_heading_store;
    }

    check_case(fixture->name, "history-to-common-tail order exact",
               previous_row_end <
                       trace_index(TRACE_ADDRESS,
                                   s_slot_addr + TEST_SLOT_X, 36) &&
                   trace_index(TRACE_ADDRESS,
                               s_slot_addr + TEST_SLOT_X, 36) <
                       trace_index(TRACE_ADDRESS, TEST_EF90, 1) &&
                   trace_index(TRACE_ADDRESS, TEST_EF90, 1) <
                       trace_index(TRACE_ADDRESS,
                                   s_slot_addr + TEST_SLOT_Z, 35) &&
                   trace_index(TRACE_ADDRESS,
                               s_slot_addr + TEST_SLOT_Z, 35) <
                       trace_index(TRACE_ADDRESS, TEST_EF92, 1) &&
                   trace_index(TRACE_ADDRESS, TEST_EF92, 1) <
                       trace_index(TRACE_ADDRESS,
                                   s_slot_addr + TEST_SLOT_HEADING, 37) &&
                   trace_index(TRACE_ADDRESS,
                               s_slot_addr + TEST_SLOT_HEADING, 37) <
                       trace_index(TRACE_ADDRESS, TEST_EE5A, 2));

    check_case(fixture->name, "history pre-canary preserved",
               memcmp(raw_address(TEST_HISTORY_BASE - 8u),
                      before_address(TEST_HISTORY_BASE - 8u), 8u) == 0);
    check_case(fixture->name, "history post-canary preserved",
               memcmp(raw_address(TEST_HISTORY_BASE + TEST_HISTORY_BYTES),
                      before_address(TEST_HISTORY_BASE + TEST_HISTORY_BYTES),
                      16u) == 0);
}

static void verify_common_or_mode47_path(const Fixture* fixture)
{
    if (fixture->route == ROUTE_MODE_4_7) {
        check_case(fixture->name, "mode4..7 slot+20 exact",
                   raw_load_u16(s_slot_addr + TEST_SLOT_CONTROL) == 2u);
        check_case(fixture->name, "mode4..7 slot+24 exact",
                   raw_load_u16(s_slot_addr + TEST_SLOT_FLAG) == 1u);
    }
    check_case(fixture->name, "non-extended terrain call count zero",
               s_terrain_calls == 0);
    check_case(fixture->name, "non-extended history region untouched",
               memcmp(raw_address(TEST_HISTORY_BASE),
                      before_address(TEST_HISTORY_BASE),
                      (size_t)TEST_HISTORY_BYTES) == 0);
}

static void verify_mutants_for_fixture(const Fixture* fixture)
{
    if (strcmp(fixture->name, "mode-0") == 0) {
        report_mutant("mode 0 treated as modes1..3",
                      trace_address_count(TEST_F8E5) == 0);
    }
    if (strcmp(fixture->name, "mode-8") == 0) {
        report_mutant("mode 8 treated as modes4..7",
                      raw_load_u16(s_slot_addr + TEST_SLOT_CONTROL) != 2u ||
                          raw_load_u16(s_slot_addr + TEST_SLOT_FLAG) != 1u);
    }
    if (strcmp(fixture->name, "mode-1-flag-2") == 0) {
        report_mutant("F8E5 nonzero instead of equality-to-1",
                      s_terrain_calls == 0 &&
                          trace_address_range_count(TEST_HISTORY_BASE,
                                                    TEST_HISTORY_BYTES) == 0);
    }
    if (strcmp(fixture->name, "mode-4") == 0) {
        report_mutant("mode4..7 unauthorized F8E5 write",
                      raw_load_u8(TEST_F8E5) == fixture->f8e5);
        report_mutant("history loop on mode4..7",
                      memcmp(raw_address(TEST_HISTORY_BASE),
                             before_address(TEST_HISTORY_BASE),
                             (size_t)TEST_HISTORY_BYTES) == 0);
    }
    if (strcmp(fixture->name, "mode-1-flag-0") == 0) {
        report_mutant("terrain called when F8E5=0", s_terrain_calls == 0);
        report_mutant("history loop on F8E5=0",
                      memcmp(raw_address(TEST_HISTORY_BASE),
                             before_address(TEST_HISTORY_BASE),
                             (size_t)TEST_HISTORY_BYTES) == 0);
    }
    if (strcmp(fixture->name, "mode-1-flag-1") == 0) {
        u32 last = TEST_HISTORY_BASE +
                   (TEST_HISTORY_ROWS - 1u) * TEST_HISTORY_STRIDE;
        u32 after = TEST_HISTORY_BASE + TEST_HISTORY_BYTES;
        u32 wrong_stride_second = TEST_HISTORY_BASE + 0x10u;

        report_mutant("C364 return forced to 1",
                      fixture->c364_return != 1 &&
                          fixture->c364_return == u32_as_s32(0x13579BDFu));
        report_mutant("extended terrain call omitted", s_terrain_calls == 1);
        report_mutant("extended terrain wrong X", s_terrain_x == fixture->shared_x);
        report_mutant("extended terrain wrong Z", s_terrain_z == fixture->shared_z);
        report_mutant("terrain return stored outside slot+2C",
                      raw_load_u32(s_slot_addr + TEST_SLOT_Y) ==
                          fixture->terrain_return);
        report_mutant("EF90 wrong 0x8007xxxx destination",
                      raw_load_u16(TEST_WRONG_EF90) ==
                          buffer_load_u16(s_ram_before, TEST_WRONG_EF90));
        report_mutant("EF92 wrong 0x8007xxxx destination",
                      raw_load_u16(TEST_WRONG_EF92) ==
                          buffer_load_u16(s_ram_before, TEST_WRONG_EF92));
        report_mutant("missing D55C publication",
                      raw_load_u32(TEST_D55C) == fixture->shared_x &&
                          raw_load_u32(TEST_D55C) !=
                              buffer_load_u32(s_ram_before, TEST_D55C));
        report_mutant("missing D52C publication",
                      raw_load_u16(TEST_D52C) == (u16)fixture->c584 &&
                          raw_load_u16(TEST_D52C) !=
                              buffer_load_u16(s_ram_before, TEST_D52C));
        report_mutant("history loop has 31 rows",
                      raw_load_u32(last) == fixture->shared_x);
        report_mutant("history loop has 33 rows",
                      memcmp(raw_address(after), before_address(after), 16u) == 0);
        report_mutant("history loop stride 0x10",
                      raw_load_u32(TEST_HISTORY_BASE + TEST_HISTORY_STRIDE) ==
                              fixture->shared_x &&
                          raw_load_u32(wrong_stride_second) != fixture->shared_x);
        report_mutant("history omitted first field",
                      raw_load_u32(TEST_HISTORY_BASE) == fixture->shared_x);
        report_mutant("history omitted middle field",
                      raw_load_u32(TEST_HISTORY_BASE + 0x08u) ==
                          fixture->shared_z);
        report_mutant("history omitted final field",
                      raw_load_u16(TEST_HISTORY_BASE + 0x10u) ==
                          (u16)fixture->c584);
    }
}

static RunResult run_fixture(const Fixture* fixture)
{
    RunResult result;
    u32 expected_x;
    u32 expected_z;

    prepare_fixture(fixture);
    s_trace_armed = true;
    result.return_value = wm_8008C530(TEST_SLOT_INDEX);
    s_trace_armed = false;
    result.terrain_calls = s_terrain_calls;
    result.ram_hash = hash_ram();

    expected_x = buffer_load_u32(s_ram_expected,
                                 s_slot_addr + TEST_SLOT_X);
    expected_z = buffer_load_u32(s_ram_expected,
                                 s_slot_addr + TEST_SLOT_Z);

    check_case(fixture->name, "return preserves exact C364 value",
               result.return_value == fixture->c364_return);
    check_case(fixture->name, "C364 called exactly once",
               s_c364_calls == 1);
    check_case(fixture->name, "C364 receives derived guest slot",
               s_c364_slot == s_slot_addr);
    check_case(fixture->name, "C364 receives literal channel zero",
               s_c364_channel == 0);
    check_case(fixture->name, "C364 precedes every direct slot write",
               s_c364_saw_pristine_slot);
    check_case(fixture->name, "trace capacity sufficient", !s_trace_overflow);
    verify_initial_order(fixture);
    verify_access_gates(fixture);

    check_case(fixture->name, "whole guest RAM exact",
               ram_matches_expected(fixture->name));
    check_case(fixture->name, "scratchpad unchanged",
               memcmp(g_PsxScratchpad, s_scratch_before,
                      sizeof(s_scratch_before)) == 0);
    check_case(fixture->name, "slot+00 scheduler state preserved",
               memcmp(raw_address(s_slot_addr + TEST_SLOT_STATE),
                      before_address(s_slot_addr + TEST_SLOT_STATE), 2u) == 0);
    check_case(fixture->name, "C364 object publication preserved",
               raw_load_u32(s_slot_addr + TEST_SLOT_OBJECT) ==
                   fixture->c364_object);
    check_case(fixture->name, "slot clear order values exact",
               raw_load_u32(s_slot_addr + TEST_SLOT_CLEAR40) == 0u &&
                   raw_load_u32(s_slot_addr + TEST_SLOT_CLEAR3C) == 0u &&
                   raw_load_u32(s_slot_addr + TEST_SLOT_CLEAR38) == 0u);
    check_case(fixture->name, "slot+4A exact halfword 12",
               raw_load_u16(s_slot_addr + TEST_SLOT_CONST) == 12u);
    check_case(fixture->name, "slot+5C sign-extends initial EE5A",
               raw_load_u32(s_slot_addr + TEST_SLOT_SIGNED) ==
                   sign_extend_u16_bits(fixture->initial_heading));
    check_case(fixture->name, "common EF90 exact SRA-12 low16",
               raw_load_u16(TEST_EF90) == (u16)oracle_sra12(expected_x));
    check_case(fixture->name, "common EF92 exact SRA-12 low16",
               raw_load_u16(TEST_EF92) == (u16)oracle_sra12(expected_z));
    check_case(fixture->name, "common EE5A exact current heading",
               raw_load_u16(TEST_EE5A) ==
                   buffer_load_u16(s_ram_expected,
                                   s_slot_addr + TEST_SLOT_HEADING));
    check_case(fixture->name, "F8E5 never modified",
               raw_load_u8(TEST_F8E5) == fixture->f8e5);
    check_case(fixture->name, "F8E6/F8E7 never modified",
               raw_load_u8(TEST_F8E6) == 0x62u &&
                   raw_load_u8(TEST_F8E7) == 0xE7u);

    if (fixture->route == ROUTE_EXTENDED)
        verify_extended_path(fixture);
    else
        verify_common_or_mode47_path(fixture);
    verify_mutants_for_fixture(fixture);
    return result;
}

#define STANDARD_FIELDS                                                    \
    0x6A24u, 0x00001000u, 0x24681357u, 0xFFFFF000u, 0x6A34BEEFu,          \
        TEST_OBJECT_TOKEN, 0x89ABCDEFu, 0xFEDCBA98u, 0xA55A1234u,         \
        0x87654321u

static const Fixture s_mode_cases[] = {
    { "mode-int32-min", INT32_MIN, 0x91u, ROUTE_COMMON, false, 3,
      0xFFFFu, STANDARD_FIELDS },
    { "mode-minus-1", -1, 0x92u, ROUTE_COMMON, false, 0x13579BDF,
      0x8001u, 0x6A24u, 0xFFFFFFFFu, 0x24681357u, 0x7FFFF000u,
      0x6A34BEEFu, TEST_OBJECT_TOKEN, 0x89ABCDEFu, 0xFEDCBA98u,
      0xA55A1234u, 0x87654321u },
    { "mode-0", 0, 0x93u, ROUTE_COMMON, false, 1,
      0x8001u, 0x6A24u, 0x80000000u, 0x24681357u, 0x00001000u,
      0x6A34BEEFu, TEST_OBJECT_TOKEN, 0x89ABCDEFu, 0xFEDCBA98u,
      0xA55A1234u, 0x87654321u },
    { "mode-1-flag-0", 1, 0x00u, ROUTE_COMMON, true, 1,
      0x0000u, STANDARD_FIELDS },
    { "mode-1-flag-1", 1, 0x01u, ROUTE_EXTENDED, true, 0x13579BDF,
      0x8001u, STANDARD_FIELDS },
    { "mode-1-flag-2", 1, 0x02u, ROUTE_COMMON, true, 3,
      0x7FFFu, STANDARD_FIELDS },
    { "mode-1-flag-ff", 1, 0xFFu, ROUTE_COMMON, true, 1,
      0xFFFFu, STANDARD_FIELDS },
    { "mode-2-flag-0", 2, 0x00u, ROUTE_COMMON, true, 3,
      0x1357u, STANDARD_FIELDS },
    { "mode-2-flag-1", 2, 0x01u, ROUTE_EXTENDED, true, 1,
      0x2468u, 0x6A24u, 0x00001000u, 0x24681357u, 0xFFFFF000u,
      0x6A34BEEFu, TEST_OBJECT_TOKEN, 0x89ABCDEFu, 0xFEDCBA98u,
      0xA55A9234u, 0x87654321u },
    { "mode-3-flag-0", 3, 0x00u, ROUTE_COMMON, true, 1,
      0x8000u, STANDARD_FIELDS },
    { "mode-3-flag-1", 3, 0x01u, ROUTE_EXTENDED, true, 3,
      0xFFFFu, STANDARD_FIELDS },
    { "mode-4", 4, 0x44u, ROUTE_MODE_4_7, false, 1,
      0x4004u, STANDARD_FIELDS },
    { "mode-5", 5, 0x55u, ROUTE_MODE_4_7, false, 3,
      0x5005u, STANDARD_FIELDS },
    { "mode-6", 6, 0x66u, ROUTE_MODE_4_7, false, 0x13579BDF,
      0x6006u, STANDARD_FIELDS },
    { "mode-7", 7, 0x77u, ROUTE_MODE_4_7, false, 1,
      0x7007u, STANDARD_FIELDS },
    { "mode-8", 8, 0x88u, ROUTE_COMMON, false, 3,
      0x8008u, STANDARD_FIELDS },
    { "mode-9", 9, 0x99u, ROUTE_COMMON, false, 1,
      0x9009u, STANDARD_FIELDS },
    { "mode-int32-max", INT32_MAX, 0xFEu, ROUTE_COMMON, false,
      0x13579BDF, 0x7FFFu, STANDARD_FIELDS },
};

static const Fixture s_natural_fixture = {
    "natural-slot4", 1, 0x00u, ROUTE_COMMON, true, 1,
    0x0000u, 1u, 0u, 0u, 0u, 0u, TEST_OBJECT_TOKEN,
    0u, 0u, 0u, 0x13579BDFu
};

static const Fixture s_return_one_fixture = {
    "return-independent-1", 1, 0x00u, ROUTE_COMMON, true, 1,
    0x8001u, STANDARD_FIELDS
};

static const Fixture s_return_three_fixture = {
    "return-independent-3", 1, 0x00u, ROUTE_COMMON, true, 3,
    0x8001u, STANDARD_FIELDS
};

#undef STANDARD_FIELDS

typedef struct SraCase {
    u32 input;
    u32 expected;
} SraCase;

static const SraCase s_sra_cases[] = {
    { 0x00001000u, 0x00000001u },
    { 0x7FFFF000u, 0x0007FFFFu },
    { 0x80000000u, 0xFFF80000u },
    { 0xFFFFF000u, 0xFFFFFFFFu },
    { 0xFFFFFFFFu, 0xFFFFFFFFu },
};

typedef struct ModeClassCase {
    s32 input;
    enum wm_c530_mode_family expected;
} ModeClassCase;

static const ModeClassCase s_mode_class_cases[] = {
    { INT32_MIN, WM_C530_MODE_NONPOSITIVE },
    { -1, WM_C530_MODE_NONPOSITIVE },
    { 0, WM_C530_MODE_NONPOSITIVE },
    { 1, WM_C530_MODE_1_TO_3 },
    { 3, WM_C530_MODE_1_TO_3 },
    { 4, WM_C530_MODE_4_TO_7 },
    { 7, WM_C530_MODE_4_TO_7 },
    { 8, WM_C530_MODE_8_OR_GREATER },
    { INT32_MAX, WM_C530_MODE_8_OR_GREATER },
};

static enum wm_c530_mode_family mutant_classify_mode_unsigned(s32 mode)
{
    u32 mode_bits = s32_as_u32(mode);

    if (mode_bits <= 0u)
        return WM_C530_MODE_NONPOSITIVE;
    if (mode_bits < 4u)
        return WM_C530_MODE_1_TO_3;
    if (mode_bits < 8u)
        return WM_C530_MODE_4_TO_7;
    return WM_C530_MODE_8_OR_GREATER;
}

static void run_mode_classifier_tests(void)
{
    size_t index;
    bool unsigned_mutant_matches_all = true;

    for (index = 0u;
         index < sizeof(s_mode_class_cases) /
                     sizeof(s_mode_class_cases[0]); index++) {
        enum wm_c530_mode_family actual =
            wm_c530_classify_mode(s_mode_class_cases[index].input);
        enum wm_c530_mode_family unsigned_mutant =
            mutant_classify_mode_unsigned(s_mode_class_cases[index].input);

        check_case("signed-mode-classifier",
                   "production signed mode family exact",
                   actual == s_mode_class_cases[index].expected);
        if (actual != unsigned_mutant)
            unsigned_mutant_matches_all = false;
    }
    report_mutant("unsigned mode-family classifier",
                  !unsigned_mutant_matches_all);
}

static void run_full_width_sra_tests(void)
{
    size_t index;
    bool logical_mutant_matches_all = true;

    for (index = 0u;
         index < sizeof(s_sra_cases) / sizeof(s_sra_cases[0]); index++) {
        u32 actual = wm_c530_sra12(s_sra_cases[index].input);
        u32 logical_mutant = s_sra_cases[index].input >> 12;

        check_case("full-width-sra12", "production SRA-12 exact u32 result",
                   actual == s_sra_cases[index].expected);
        if (actual != logical_mutant)
            logical_mutant_matches_all = false;
    }
    report_mutant("logical SRL substituted for arithmetic SRA-12",
                  !logical_mutant_matches_all);
}

int main(void)
{
    size_t index;
    RunResult natural;
    RunResult return_one;
    RunResult return_three;

    printf("W34B6-F wm_8008C530 production-linked test\n");
    printf("ORACLE: declarative retail routes + audited write table\n");

    run_mode_classifier_tests();
    run_full_width_sra_tests();

    for (index = 0u;
         index < sizeof(s_mode_cases) / sizeof(s_mode_cases[0]); index++) {
        (void)run_fixture(&s_mode_cases[index]);
    }

    natural = run_fixture(&s_natural_fixture);
    check_case("natural-slot4", "natural return is one",
               natural.return_value == 1);
    check_case("natural-slot4", "natural direct terrain skipped",
               natural.terrain_calls == 0);
    check_case("natural-slot4", "natural object remains published",
               raw_load_u32(s_slot_addr + TEST_SLOT_OBJECT) ==
                   TEST_OBJECT_TOKEN);

    return_one = run_fixture(&s_return_one_fixture);
    return_three = run_fixture(&s_return_three_fixture);
    check_case("return-independence", "C364 return does not affect writes",
               return_one.ram_hash == return_three.ram_hash);
    check_case("return-independence", "C364 residues returned independently",
               return_one.return_value == 1 && return_three.return_value == 3);

    printf("PASS/TOTAL: %d/%d\n", s_pass_count, s_total_count);
    return s_failure_count == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
