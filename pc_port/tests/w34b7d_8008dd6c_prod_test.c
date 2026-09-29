/*
 * W34B7-D production-linked certification test for world callback
 * 0x8008DD6C.
 *
 * The actual production translation unit is included below so PSX_ADDR can
 * be replaced with a deterministic address/order trace.  Expected images are
 * constructed from declarative retail fixtures and the accepted DD6C write
 * table.  They do not call production helpers or duplicate DD6C control flow.
 * wm_8008C364 and wm_80093978 are controlled recorders.
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

#include "../src/world_map_callback_8dd6c.c"

#define TEST_RAM_MASK       0x001FFFFFu
#define TEST_POOL_PTR       0x8009BE24u
#define TEST_MODE           0x8009BE10u
#define TEST_EE5A           0x8006EE5Au
#define TEST_EE5C           0x8006EE5Cu
#define TEST_EE5E           0x8006EE5Eu
#define TEST_F8E5           0x8006F8E5u
#define TEST_F8E6           0x8006F8E6u
#define TEST_F8E7           0x8006F8E7u
#define TEST_PRIMARY_2      0x8006F36Au
#define TEST_D940_BASE      0x8006D940u
#define TEST_D940_RECORDS   256u
#define TEST_D940_STRIDE    0xA4u
#define TEST_BDF8_BASE      0x8009BDF8u
#define TEST_BDF8_BYTES     12u
#define TEST_C5AC           0x8009C5ACu
#define TEST_C5B4           0x8009C5B4u
#define TEST_C584           0x8009C584u
#define TEST_EF90           0x8006EF90u
#define TEST_EF92           0x8006EF92u
#define TEST_EF96           0x8006EF96u
#define TEST_EF98           0x8006EF98u
#define TEST_EF9C           0x8006EF9Cu
#define TEST_EF9E           0x8006EF9Eu
#define TEST_BAD7_EF9C      0x8007EF9Cu
#define TEST_BAD7_EF9E      0x8007EF9Eu
#define TEST_HISTORY_BASE   0x8009CEC4u
#define TEST_HISTORY_BYTES  (0x14u * 32u)
#define TEST_D55C           0x8009D55Cu
#define TEST_D560           0x8009D560u
#define TEST_D564           0x8009D564u
#define TEST_D568           0x8009D568u
#define TEST_D154           0x8009D154u
#define TEST_D52C           0x8009D52Cu
#define TEST_POOL_BASE      0x800D7538u
#define TEST_SLOT_INDEX     6
#define TEST_SLOT_STRIDE    0x80u
#define TEST_SLOT_STATE     0x00u
#define TEST_SLOT_CONTROL   0x20u
#define TEST_SLOT_FLAG      0x24u
#define TEST_SLOT_X         0x28u
#define TEST_SLOT_Y         0x2Cu
#define TEST_SLOT_Z         0x30u
#define TEST_SLOT_AUX       0x34u
#define TEST_SLOT_CLEAR38   0x38u
#define TEST_SLOT_CLEAR3C   0x3Cu
#define TEST_SLOT_CLEAR40   0x40u
#define TEST_SLOT_HEADING   0x48u
#define TEST_SLOT_CONST     0x4Au
#define TEST_SLOT_OBJECT    0x4Cu
#define TEST_SLOT_58        0x58u
#define TEST_SLOT_SIGNED    0x5Cu
#define TEST_OBJECT_TOKEN   0x0DD64C02u
#define TEST_TRACE_CAPACITY 96u

_Static_assert(PSX_RAM_SIZE >= (2 * 1024 * 1024),
               "test requires complete PSX main RAM");
_Static_assert(WM_DD6C_POOL_PTR == TEST_POOL_PTR,
               "production pool pointer must match retail DD6C");
_Static_assert(WM_DD6C_MODE == TEST_MODE,
               "production mode address must match retail DD6C");
_Static_assert(WM_DD6C_HEADING == TEST_EE5E,
               "D3F0 EE5C heading substitution must fail");
_Static_assert(WM_DD6C_CHANNEL2_FLAG == TEST_F8E7,
               "D3F0 F8E6 flag substitution must fail");
_Static_assert(WM_DD6C_SHARED_X == TEST_C5AC &&
                   WM_DD6C_SHARED_Z == TEST_C5B4 &&
                   WM_DD6C_SHARED_HEADING == TEST_C584,
               "shared extended-path sources must match retail DD6C");
_Static_assert(WM_DD6C_PUBLISHED_X == TEST_EF9C,
               "D3F0 EF96 publication substitution must fail");
_Static_assert(WM_DD6C_PUBLISHED_Z == TEST_EF9E,
               "D3F0 EF98 publication substitution must fail");

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
    u8 f8e7;
    RouteFamily route;
    s32 c364_return;
    u16 initial_heading;
    u16 c364_flag;
    u32 c364_x;
    u32 c364_y;
    u32 c364_z;
    u32 c364_object;
    u32 shared_x;
    u32 shared_z;
    u32 shared_heading;
    u32 terrain_return;
} Fixture;

typedef struct RunResult {
    s32 return_value;
    uint64_t ram_hash;
    int terrain_calls;
    int flag_reads;
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
static bool s_terrain_saw_published_xz;

static int s_pass_count;
static int s_total_count;
static int s_failure_count;
static const char* s_reported_mutants[32];
static size_t s_reported_mutant_count;

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

static u32 oracle_sign_extend_u16(u16 value)
{
    if (value <= 0x7FFFu)
        return (u32)value;
    return 0xFFFF0000u | (u32)value;
}

static u32 oracle_sra12(u32 value)
{
    u32 result = value >> 12;

    if ((value & 0x80000000u) != 0u)
        result |= 0xFFF00000u;
    return result;
}

static uint64_t hash_ram(void)
{
    uint64_t hash = UINT64_C(1469598103934665603);
    size_t index;

    for (index = 0u; index < sizeof(g_PsxRam); index++) {
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
    size_t index;
    bool already_reported = false;

    check_case("mutant-detection", name, detected);
    for (index = 0u; index < s_reported_mutant_count; index++) {
        if (strcmp(s_reported_mutants[index], name) == 0)
            already_reported = true;
    }
    if (detected && !already_reported &&
        s_reported_mutant_count <
            sizeof(s_reported_mutants) / sizeof(s_reported_mutants[0])) {
        printf("MUTANT DETECTED: %s\n", name);
        s_reported_mutants[s_reported_mutant_count] = name;
        s_reported_mutant_count++;
    }
}

static size_t trace_index(TraceKind kind, u32 value, int occurrence)
{
    size_t index;
    int seen = 0;

    for (index = 0u; index < s_trace_count &&
                         index < (size_t)TEST_TRACE_CAPACITY; index++) {
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
    int count = 0;

    for (index = 0u; index < s_trace_count &&
                         index < (size_t)TEST_TRACE_CAPACITY; index++) {
        if (s_trace[index].kind == TRACE_ADDRESS &&
            s_trace[index].value == address)
            count++;
    }
    return count;
}

static int trace_address_range_count(u32 start, u32 width)
{
    size_t index;
    int count = 0;
    u32 end = start + width;

    for (index = 0u; index < s_trace_count &&
                         index < (size_t)TEST_TRACE_CAPACITY; index++) {
        u32 value = s_trace[index].value;

        if (s_trace[index].kind == TRACE_ADDRESS && value >= start && value < end)
            count++;
    }
    return count;
}

static int trace_address_total(void)
{
    size_t index;
    int count = 0;

    for (index = 0u; index < s_trace_count &&
                         index < (size_t)TEST_TRACE_CAPACITY; index++) {
        if (s_trace[index].kind == TRACE_ADDRESS)
            count++;
    }
    return count;
}

static int trace_d940_record_count(void)
{
    u32 record;
    int count = 0;

    for (record = 0u; record < TEST_D940_RECORDS; record++)
        count += trace_address_count(TEST_D940_BASE +
                                     record * TEST_D940_STRIDE);
    return count;
}

static bool address_is_one_of(u32 address, const u32* values, size_t count)
{
    size_t index;

    for (index = 0u; index < count; index++) {
        if (address == values[index])
            return true;
    }
    return false;
}

static bool trace_address_authorized(u32 address, const Fixture* fixture)
{
    const u32 common[] = {
        TEST_POOL_PTR, TEST_MODE, TEST_EE5E, TEST_EF9C, TEST_EF9E,
        s_slot_addr + TEST_SLOT_CLEAR40,
        s_slot_addr + TEST_SLOT_CLEAR3C,
        s_slot_addr + TEST_SLOT_CLEAR38,
        s_slot_addr + TEST_SLOT_HEADING,
        s_slot_addr + TEST_SLOT_CONST,
        s_slot_addr + TEST_SLOT_58,
        s_slot_addr + TEST_SLOT_SIGNED,
        s_slot_addr + TEST_SLOT_X,
        s_slot_addr + TEST_SLOT_Z
    };
    const u32 extended[] = {
        TEST_F8E7, TEST_C5AC, TEST_C5B4, TEST_C584,
        s_slot_addr + TEST_SLOT_CONTROL,
        s_slot_addr + TEST_SLOT_Y
    };
    const u32 mode47[] = {
        s_slot_addr + TEST_SLOT_CONTROL,
        s_slot_addr + TEST_SLOT_FLAG
    };

    if (address_is_one_of(address, common, sizeof(common) / sizeof(common[0])))
        return true;
    if (fixture->mode >= 1 && fixture->mode <= 3 && address == TEST_F8E7)
        return true;
    if (fixture->route == ROUTE_EXTENDED &&
        address_is_one_of(address, extended,
                          sizeof(extended) / sizeof(extended[0])))
        return true;
    if (fixture->route == ROUTE_MODE_4_7 &&
        address_is_one_of(address, mode47, sizeof(mode47) / sizeof(mode47[0])))
        return true;
    return false;
}

static bool all_trace_addresses_authorized(const Fixture* fixture)
{
    size_t index;

    for (index = 0u; index < s_trace_count &&
                         index < (size_t)TEST_TRACE_CAPACITY; index++) {
        if (s_trace[index].kind == TRACE_ADDRESS &&
            !trace_address_authorized(s_trace[index].value, fixture))
            return false;
    }
    return true;
}

s32 wm_8008C364(u32 slot_addr, s32 channel)
{
    trace_append(TRACE_C364, slot_addr);
    s_c364_calls++;
    s_c364_slot = slot_addr;
    s_c364_channel = channel;
    s_c364_saw_pristine_slot =
        memcmp(raw_address(slot_addr),
               s_ram_before + raw_index(slot_addr), TEST_SLOT_STRIDE) == 0;

    raw_store_u16(slot_addr + TEST_SLOT_FLAG, s_active->c364_flag);
    raw_store_u32(slot_addr + TEST_SLOT_X, s_active->c364_x);
    raw_store_u32(slot_addr + TEST_SLOT_Y, s_active->c364_y);
    raw_store_u32(slot_addr + TEST_SLOT_Z, s_active->c364_z);
    if (s_active->c364_object != 0u)
        raw_store_u32(slot_addr + TEST_SLOT_OBJECT, s_active->c364_object);
    return s_active->c364_return;
}

s32 wm_80093978(s32 x, s32 z)
{
    u32 x_bits = s32_as_u32(x);
    u32 z_bits = s32_as_u32(z);

    trace_append(TRACE_TERRAIN, x_bits);
    s_terrain_calls++;
    s_terrain_x = x_bits;
    s_terrain_z = z_bits;
    s_terrain_saw_published_xz =
        raw_load_u32(s_slot_addr + TEST_SLOT_X) == s_active->shared_x &&
        raw_load_u32(s_slot_addr + TEST_SLOT_Z) == s_active->shared_z;
    return u32_as_s32(s_active->terrain_return);
}

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
    if (fixture->c364_object != 0u) {
        buffer_store_u32(s_ram_expected, s_slot_addr + TEST_SLOT_OBJECT,
                         fixture->c364_object);
    }
}

static void build_expected_ram(const Fixture* fixture)
{
    u32 x;
    u32 z;
    u16 heading;

    memcpy(s_ram_expected, s_ram_before, sizeof(s_ram_expected));
    apply_c364_expected(fixture);

    buffer_store_u32(s_ram_expected, s_slot_addr + TEST_SLOT_CLEAR40, 0u);
    buffer_store_u32(s_ram_expected, s_slot_addr + TEST_SLOT_CLEAR3C, 0u);
    buffer_store_u32(s_ram_expected, s_slot_addr + TEST_SLOT_CLEAR38, 0u);
    buffer_store_u16(s_ram_expected, s_slot_addr + TEST_SLOT_CONST, 12u);
    buffer_store_u32(s_ram_expected, s_slot_addr + TEST_SLOT_58, 31u);
    buffer_store_u16(s_ram_expected, s_slot_addr + TEST_SLOT_HEADING,
                     fixture->initial_heading);
    buffer_store_u32(s_ram_expected, s_slot_addr + TEST_SLOT_SIGNED,
                     oracle_sign_extend_u16(fixture->initial_heading));

    if (fixture->route == ROUTE_EXTENDED) {
        buffer_store_u16(s_ram_expected, s_slot_addr + TEST_SLOT_CONTROL, 1u);
        buffer_store_u32(s_ram_expected, s_slot_addr + TEST_SLOT_X,
                         fixture->shared_x);
        buffer_store_u32(s_ram_expected, s_slot_addr + TEST_SLOT_Z,
                         fixture->shared_z);
        buffer_store_u32(s_ram_expected, s_slot_addr + TEST_SLOT_Y,
                         fixture->terrain_return);
        buffer_store_u16(s_ram_expected, s_slot_addr + TEST_SLOT_HEADING,
                         (u16)fixture->shared_heading);
    } else if (fixture->route == ROUTE_MODE_4_7) {
        buffer_store_u16(s_ram_expected, s_slot_addr + TEST_SLOT_CONTROL, 2u);
        buffer_store_u16(s_ram_expected, s_slot_addr + TEST_SLOT_FLAG, 1u);
    }

    x = buffer_load_u32(s_ram_expected, s_slot_addr + TEST_SLOT_X);
    z = buffer_load_u32(s_ram_expected, s_slot_addr + TEST_SLOT_Z);
    heading = buffer_load_u16(s_ram_expected,
                              s_slot_addr + TEST_SLOT_HEADING);
    buffer_store_u16(s_ram_expected, TEST_EF9C, (u16)oracle_sra12(x));
    buffer_store_u16(s_ram_expected, TEST_EF9E, (u16)oracle_sra12(z));
    buffer_store_u16(s_ram_expected, TEST_EE5E, heading);
}

static void fill_test_memory(void)
{
    size_t index;

    for (index = 0u; index < sizeof(g_PsxRam); index++)
        g_PsxRam[index] = (u8)((index * 37u + 0x5Au) & 0xFFu);
    for (index = 0u; index < sizeof(g_PsxScratchpad); index++)
        g_PsxScratchpad[index] = (u8)((index * 19u + 0xA5u) & 0xFFu);
}

static void prepare_fixture(const Fixture* fixture)
{
    s_active = fixture;
    s_slot_addr = TEST_POOL_BASE + ((u32)TEST_SLOT_INDEX << 7);
    fill_test_memory();

    raw_store_u32(TEST_POOL_PTR, TEST_POOL_BASE);
    raw_store_u32(TEST_MODE, s32_as_u32(fixture->mode));
    raw_store_u16(TEST_EE5E, fixture->initial_heading);
    raw_store_u8(TEST_F8E5, 0xA5u);
    raw_store_u8(TEST_F8E6, 0xE6u);
    raw_store_u8(TEST_F8E7, fixture->f8e7);
    raw_store_u8(TEST_PRIMARY_2, 0xFFu);
    raw_store_u32(TEST_C5AC, fixture->shared_x);
    raw_store_u32(TEST_C5B4, fixture->shared_z);
    raw_store_u32(TEST_C584, fixture->shared_heading);
    if (fixture->c364_object == 0u)
        raw_store_u32(s_slot_addr + TEST_SLOT_OBJECT, 0u);

    memcpy(s_ram_before, g_PsxRam, sizeof(s_ram_before));
    memcpy(s_scratch_before, g_PsxScratchpad, sizeof(s_scratch_before));
    build_expected_ram(fixture);

    memset(s_trace, 0, sizeof(s_trace));
    s_trace_count = 0u;
    s_trace_overflow = false;
    s_c364_calls = 0;
    s_c364_slot = 0u;
    s_c364_channel = -1;
    s_c364_saw_pristine_slot = false;
    s_terrain_calls = 0;
    s_terrain_x = 0u;
    s_terrain_z = 0u;
    s_terrain_saw_published_xz = false;
    s_trace_armed = true;
}

static bool ram_matches_expected(const char* case_name)
{
    size_t index;

    for (index = 0u; index < sizeof(g_PsxRam); index++) {
        if (g_PsxRam[index] != s_ram_expected[index]) {
            printf("RAM mismatch [%s] address=%08X actual=%02X expected=%02X\n",
                   case_name, (unsigned int)(0x80000000u + (u32)index),
                   (unsigned int)g_PsxRam[index],
                   (unsigned int)s_ram_expected[index]);
            return false;
        }
    }
    return true;
}

static void verify_initial_order(const Fixture* fixture)
{
    const TraceKind kinds[] = {
        TRACE_ADDRESS, TRACE_C364, TRACE_ADDRESS, TRACE_ADDRESS,
        TRACE_ADDRESS, TRACE_ADDRESS, TRACE_ADDRESS, TRACE_ADDRESS,
        TRACE_ADDRESS, TRACE_ADDRESS, TRACE_ADDRESS, TRACE_ADDRESS
    };
    const u32 values[] = {
        TEST_POOL_PTR, s_slot_addr,
        s_slot_addr + TEST_SLOT_CLEAR40,
        s_slot_addr + TEST_SLOT_CLEAR3C,
        s_slot_addr + TEST_SLOT_CLEAR38,
        TEST_EE5E,
        s_slot_addr + TEST_SLOT_CONST,
        s_slot_addr + TEST_SLOT_58,
        TEST_MODE,
        s_slot_addr + TEST_SLOT_HEADING,
        s_slot_addr + TEST_SLOT_HEADING,
        s_slot_addr + TEST_SLOT_SIGNED
    };
    size_t index;
    bool exact = s_trace_count >= sizeof(values) / sizeof(values[0]);

    for (index = 0u; exact && index < sizeof(values) / sizeof(values[0]); index++) {
        exact = s_trace[index].kind == kinds[index] &&
                s_trace[index].value == values[index];
    }
    check_case(fixture->name, "retail initial access/store order exact", exact);
}

static void verify_common_tail_order(const Fixture* fixture)
{
    const u32 values[] = {
        s_slot_addr + TEST_SLOT_X,
        TEST_EF9C,
        s_slot_addr + TEST_SLOT_Z,
        TEST_EF9E,
        s_slot_addr + TEST_SLOT_HEADING,
        TEST_EE5E
    };
    size_t count = sizeof(values) / sizeof(values[0]);
    size_t start = s_trace_count >= count ? s_trace_count - count : 0u;
    size_t index;
    bool exact = s_trace_count >= count &&
                 s_trace_count <= (size_t)TEST_TRACE_CAPACITY;

    for (index = 0u; exact && index < count; index++) {
        size_t at = start + index;

        exact = s_trace[at].kind == TRACE_ADDRESS &&
                s_trace[at].value == values[index];
    }
    check_case(fixture->name,
               "common-tail translation order exact on selected route", exact);
}

static void verify_access_matrix(const Fixture* fixture)
{
    int expected_total = 17;
    int expected_flag_reads = 0;
    int expected_c5_reads = fixture->route == ROUTE_EXTENDED ? 1 : 0;

    if (fixture->mode >= 1 && fixture->mode <= 3) {
        expected_total++;
        expected_flag_reads = 1;
    }
    if (fixture->route == ROUTE_EXTENDED)
        expected_total += 9;
    if (fixture->route == ROUTE_MODE_4_7)
        expected_total += 2;

    check_case(fixture->name, "all translated addresses retail-authorized",
               all_trace_addresses_authorized(fixture));
    check_case(fixture->name, "complete translated address count exact",
               trace_address_total() == expected_total);
    check_case(fixture->name, "pool-pointer read count exact",
               trace_address_count(TEST_POOL_PTR) == 1);
    check_case(fixture->name, "F8E7 read gate exact",
               trace_address_count(TEST_F8E7) == expected_flag_reads);
    check_case(fixture->name, "F8E5/F8E6 direct access zero",
               trace_address_count(TEST_F8E5) == 0 &&
               trace_address_count(TEST_F8E6) == 0);
    check_case(fixture->name, "C5 global read gate exact",
               trace_address_count(TEST_C5AC) == expected_c5_reads &&
               trace_address_count(TEST_C5B4) == expected_c5_reads &&
               trace_address_count(TEST_C584) == expected_c5_reads);
    check_case(fixture->name, "slot+00 direct access zero",
               trace_address_range_count(s_slot_addr + TEST_SLOT_STATE, 2u) == 0);
    check_case(fixture->name, "slot+34 direct access zero",
               trace_address_range_count(s_slot_addr + TEST_SLOT_AUX, 4u) == 0);
    check_case(fixture->name, "slot+4C direct access zero",
               trace_address_range_count(s_slot_addr + TEST_SLOT_OBJECT, 4u) == 0);
    check_case(fixture->name, "primary/D940/BDF8 direct access zero",
               trace_address_count(TEST_PRIMARY_2) == 0 &&
               trace_d940_record_count() == 0 &&
               trace_address_range_count(TEST_BDF8_BASE, TEST_BDF8_BYTES) == 0);
    check_case(fixture->name, "heading global access exact",
               trace_address_count(TEST_EE5E) == 2 &&
               trace_address_count(TEST_EE5A) == 0 &&
               trace_address_count(TEST_EE5C) == 0);
    check_case(fixture->name, "position destination access exact",
               trace_address_count(TEST_EF9C) == 1 &&
               trace_address_count(TEST_EF9E) == 1 &&
               trace_address_count(TEST_EF90) == 0 &&
               trace_address_count(TEST_EF92) == 0 &&
               trace_address_count(TEST_EF96) == 0 &&
               trace_address_count(TEST_EF98) == 0 &&
               trace_address_count(TEST_BAD7_EF9C) == 0 &&
               trace_address_count(TEST_BAD7_EF9E) == 0);
    check_case(fixture->name, "history/snapshot direct access zero",
               trace_address_range_count(TEST_HISTORY_BASE,
                                         TEST_HISTORY_BYTES) == 0 &&
               trace_address_count(TEST_D55C) == 0 &&
               trace_address_count(TEST_D560) == 0 &&
               trace_address_count(TEST_D564) == 0 &&
               trace_address_count(TEST_D568) == 0 &&
               trace_address_count(TEST_D154) == 0 &&
               trace_address_count(TEST_D52C) == 0);
    check_case(fixture->name, "scratchpad/GPU direct access zero",
               trace_address_range_count(0x1F800000u, 0x3000u) == 0 &&
               trace_address_range_count(0x1F801810u, 8u) == 0);
}

static void verify_extended_order(const Fixture* fixture)
{
    const TraceKind kinds[] = {
        TRACE_ADDRESS, TRACE_ADDRESS, TRACE_ADDRESS, TRACE_ADDRESS,
        TRACE_ADDRESS, TRACE_ADDRESS, TRACE_TERRAIN, TRACE_ADDRESS,
        TRACE_ADDRESS, TRACE_ADDRESS
    };
    const u32 values[] = {
        s_slot_addr + TEST_SLOT_CONTROL,
        TEST_C5AC,
        s_slot_addr + TEST_SLOT_X,
        TEST_C5B4,
        s_slot_addr + TEST_SLOT_X,
        s_slot_addr + TEST_SLOT_Z,
        fixture->shared_x,
        TEST_C584,
        s_slot_addr + TEST_SLOT_Y,
        s_slot_addr + TEST_SLOT_HEADING
    };
    size_t start = trace_index(TRACE_ADDRESS, TEST_F8E7, 1);
    size_t index;
    bool exact = start != SIZE_MAX;

    if (exact)
        start++;
    for (index = 0u; exact && index < sizeof(values) / sizeof(values[0]); index++) {
        size_t at = start + index;
        exact = at < s_trace_count && at < (size_t)TEST_TRACE_CAPACITY &&
                s_trace[at].kind == kinds[index] &&
                s_trace[at].value == values[index];
    }
    check_case(fixture->name, "extended dependency/store order exact", exact);
    check_case(fixture->name, "terrain called exactly once",
               s_terrain_calls == 1);
    check_case(fixture->name, "terrain X uses fresh published C5AC bits",
               s_terrain_x == fixture->shared_x);
    check_case(fixture->name, "terrain Z uses raw C5B4 bits",
               s_terrain_z == fixture->shared_z);
    check_case(fixture->name, "X/Z are published before terrain call",
               s_terrain_saw_published_xz);
    check_case(fixture->name, "terrain return only reaches slot+2C",
               raw_load_u32(s_slot_addr + TEST_SLOT_Y) ==
                   fixture->terrain_return);
    check_case(fixture->name, "C584 low16 replaces slot+48",
               raw_load_u16(s_slot_addr + TEST_SLOT_HEADING) ==
                   (u16)fixture->shared_heading);
    check_case(fixture->name, "+48/+5C initial-heading divergence exact",
               raw_load_u32(s_slot_addr + TEST_SLOT_SIGNED) ==
                   oracle_sign_extend_u16(fixture->initial_heading));
}

static void verify_fixture_mutants(const Fixture* fixture, s32 actual_return)
{
    if (fixture->c364_return != 1)
        report_mutant("C364 return forced to one",
                      actual_return == fixture->c364_return);
    report_mutant("D3F0 channel one substituted for DD6C channel two",
                  s_c364_channel == 2);
    report_mutant("D3F0 EE5C or channel-zero EE5A substituted for EE5E",
                  trace_address_count(TEST_EE5E) == 2 &&
                  trace_address_count(TEST_EE5A) == 0 &&
                  trace_address_count(TEST_EE5C) == 0);
    report_mutant("D3F0 EF96 substituted for DD6C EF9C",
                  trace_address_count(TEST_EF9C) == 1 &&
                  trace_address_count(TEST_EF96) == 0);
    report_mutant("D3F0 EF98 substituted for DD6C EF9E",
                  trace_address_count(TEST_EF9E) == 1 &&
                  trace_address_count(TEST_EF98) == 0);
    report_mutant("channel-zero EF90/EF92 or 0x8007xxxx destination",
                  trace_address_count(TEST_EF90) == 0 &&
                  trace_address_count(TEST_EF92) == 0 &&
                  trace_address_count(TEST_BAD7_EF9C) == 0 &&
                  trace_address_count(TEST_BAD7_EF9E) == 0);
    report_mutant("D3F0 slot+58 constant 15 substituted for DD6C 31",
                  raw_load_u32(s_slot_addr + TEST_SLOT_58) == 31u);
    report_mutant("slot+58 near-miss constant 30",
                  raw_load_u32(s_slot_addr + TEST_SLOT_58) == 31u);

    if (fixture->mode == 0) {
        report_mutant("mode zero treated as modes1..3",
                      trace_address_count(TEST_F8E7) == 0);
    }
    if (fixture->mode < 0) {
        report_mutant("unsigned mode-family classification",
                      trace_address_count(TEST_F8E7) == 0 &&
                      s_terrain_calls == 0);
    }
    if (fixture->mode == 8) {
        report_mutant("mode eight treated as modes4..7",
                      buffer_load_u16(g_PsxRam,
                                      s_slot_addr + TEST_SLOT_CONTROL) ==
                          buffer_load_u16(s_ram_before,
                                          s_slot_addr + TEST_SLOT_CONTROL));
    }
    if (fixture->mode >= 1 && fixture->mode <= 3) {
        report_mutant("D3F0 F8E6 substituted for DD6C F8E7",
                      trace_address_count(TEST_F8E7) == 1 &&
                      trace_address_count(TEST_F8E6) == 0);
    }
    if (fixture->mode >= 1 && fixture->mode <= 3 && fixture->f8e7 != 1u) {
        report_mutant("F8E7 nonzero instead of exact equality-to-one",
                      s_terrain_calls == 0 &&
                      trace_address_count(TEST_C5AC) == 0);
        report_mutant("terrain called when F8E7 is not one",
                      s_terrain_calls == 0);
    }
    if (fixture->route == ROUTE_EXTENDED) {
        report_mutant("extended terrain call omitted", s_terrain_calls == 1);
        report_mutant("extended terrain wrong or stale X",
                      s_terrain_x == fixture->shared_x &&
                      s_terrain_x != fixture->c364_x);
        report_mutant("extended terrain wrong Z",
                      s_terrain_z == fixture->shared_z);
        report_mutant("terrain return stored to wrong destination",
                      raw_load_u32(s_slot_addr + TEST_SLOT_Y) ==
                          fixture->terrain_return);
        report_mutant("extended branch misses relocated DD6C common tail",
                      trace_address_count(TEST_EF9C) == 1 &&
                      trace_address_count(TEST_EF9E) == 1 &&
                      trace_address_count(TEST_EE5E) == 2);
    }
}

static RunResult run_fixture(const Fixture* fixture)
{
    RunResult result;
    u32 expected_x;
    u32 expected_z;

    prepare_fixture(fixture);
    result.return_value = wm_8008DD6C(TEST_SLOT_INDEX);
    s_trace_armed = false;

    expected_x = buffer_load_u32(s_ram_expected,
                                 s_slot_addr + TEST_SLOT_X);
    expected_z = buffer_load_u32(s_ram_expected,
                                 s_slot_addr + TEST_SLOT_Z);

    check_case(fixture->name, "return preserves exact C364 residue",
               result.return_value == fixture->c364_return);
    check_case(fixture->name, "C364 called exactly once",
               s_c364_calls == 1);
    check_case(fixture->name, "C364 receives direct guest slot",
               s_c364_slot == s_slot_addr);
    check_case(fixture->name, "C364 receives literal channel two",
               s_c364_channel == 2);
    check_case(fixture->name, "C364 precedes all direct slot stores",
               s_c364_saw_pristine_slot &&
               trace_index(TRACE_C364, s_slot_addr, 1) == 1u);
    check_case(fixture->name, "trace capacity sufficient", !s_trace_overflow);
    verify_initial_order(fixture);
    verify_access_matrix(fixture);
    verify_common_tail_order(fixture);
    check_case(fixture->name, "whole guest RAM exact declarative image",
               ram_matches_expected(fixture->name));
    check_case(fixture->name, "scratchpad unchanged",
               memcmp(g_PsxScratchpad, s_scratch_before,
                      sizeof(g_PsxScratchpad)) == 0);
    check_case(fixture->name, "slot+00 scheduler state preserved",
               memcmp(raw_address(s_slot_addr + TEST_SLOT_STATE),
                      s_ram_before + raw_index(s_slot_addr + TEST_SLOT_STATE),
                      2u) == 0);
    check_case(fixture->name, "slot+34 preserved",
               raw_load_u32(s_slot_addr + TEST_SLOT_AUX) ==
                   buffer_load_u32(s_ram_before,
                                   s_slot_addr + TEST_SLOT_AUX));
    check_case(fixture->name, "C364 object publication preserved",
               raw_load_u32(s_slot_addr + TEST_SLOT_OBJECT) ==
                   fixture->c364_object);
    check_case(fixture->name, "clear order values exact",
               raw_load_u32(s_slot_addr + TEST_SLOT_CLEAR40) == 0u &&
               raw_load_u32(s_slot_addr + TEST_SLOT_CLEAR3C) == 0u &&
               raw_load_u32(s_slot_addr + TEST_SLOT_CLEAR38) == 0u);
    check_case(fixture->name, "slot+4A exact halfword 12",
               raw_load_u16(s_slot_addr + TEST_SLOT_CONST) == 12u);
    check_case(fixture->name, "slot+58 exact word 31",
               raw_load_u32(s_slot_addr + TEST_SLOT_58) == 31u);
    check_case(fixture->name, "slot+5C sign-extends initial EE5E",
               raw_load_u32(s_slot_addr + TEST_SLOT_SIGNED) ==
                   oracle_sign_extend_u16(fixture->initial_heading));
    check_case(fixture->name, "common EF9C exact SRA12 low16",
               raw_load_u16(TEST_EF9C) == (u16)oracle_sra12(expected_x));
    check_case(fixture->name, "common EF9E exact SRA12 low16",
               raw_load_u16(TEST_EF9E) == (u16)oracle_sra12(expected_z));
    check_case(fixture->name, "common EE5E exact current heading",
               raw_load_u16(TEST_EE5E) ==
                   buffer_load_u16(s_ram_expected,
                                   s_slot_addr + TEST_SLOT_HEADING));
    check_case(fixture->name, "F8 bytes never modified",
               raw_load_u8(TEST_F8E5) == buffer_load_u8(s_ram_before, TEST_F8E5) &&
               raw_load_u8(TEST_F8E6) == buffer_load_u8(s_ram_before, TEST_F8E6) &&
               raw_load_u8(TEST_F8E7) == buffer_load_u8(s_ram_before, TEST_F8E7));
    check_case(fixture->name, "no history/snapshot writes",
               memcmp(raw_address(TEST_HISTORY_BASE),
                      s_ram_before + raw_index(TEST_HISTORY_BASE),
                      TEST_HISTORY_BYTES) == 0);

    if (fixture->route == ROUTE_EXTENDED)
        verify_extended_order(fixture);
    else
        check_case(fixture->name, "nonextended direct terrain count zero",
                   s_terrain_calls == 0);

    if (fixture->route == ROUTE_MODE_4_7) {
        check_case(fixture->name, "mode4..7 slot+20 exact",
                   raw_load_u16(s_slot_addr + TEST_SLOT_CONTROL) == 2u);
        check_case(fixture->name, "mode4..7 slot+24 exact",
                   raw_load_u16(s_slot_addr + TEST_SLOT_FLAG) == 1u);
    }

    verify_fixture_mutants(fixture, result.return_value);
    result.ram_hash = hash_ram();
    result.terrain_calls = s_terrain_calls;
    result.flag_reads = trace_address_count(TEST_F8E7);
    return result;
}

#define MODE_CASE(case_name, mode_value, flag_value, route_value) \
    { case_name, mode_value, flag_value, route_value, (s32)0x13579BDF, \
      0x8001u, 0x2468u, 0x80000000u, 0x11223344u, 0xFFFFF000u, \
      TEST_OBJECT_TOKEN, 0x7FFFF000u, 0xC3A5B6D7u, 0x89AB1234u, \
      0xDEADBEEFu }

static const Fixture s_mode_cases[] = {
    MODE_CASE("mode-int-min", INT32_MIN, 0x01u, ROUTE_COMMON),
    MODE_CASE("mode-neg-one", -1, 0x01u, ROUTE_COMMON),
    MODE_CASE("mode-zero", 0, 0x01u, ROUTE_COMMON),
    MODE_CASE("mode1-f0", 1, 0x00u, ROUTE_COMMON),
    MODE_CASE("mode1-f1", 1, 0x01u, ROUTE_EXTENDED),
    MODE_CASE("mode1-f2", 1, 0x02u, ROUTE_COMMON),
    MODE_CASE("mode1-fff", 1, 0xFFu, ROUTE_COMMON),
    MODE_CASE("mode2-f0", 2, 0x00u, ROUTE_COMMON),
    MODE_CASE("mode2-f1", 2, 0x01u, ROUTE_EXTENDED),
    MODE_CASE("mode3-f0", 3, 0x00u, ROUTE_COMMON),
    MODE_CASE("mode3-f1", 3, 0x01u, ROUTE_EXTENDED),
    MODE_CASE("mode4", 4, 0x01u, ROUTE_MODE_4_7),
    MODE_CASE("mode5", 5, 0x01u, ROUTE_MODE_4_7),
    MODE_CASE("mode6", 6, 0x01u, ROUTE_MODE_4_7),
    MODE_CASE("mode7", 7, 0x01u, ROUTE_MODE_4_7),
    MODE_CASE("mode8", 8, 0x01u, ROUTE_COMMON),
    MODE_CASE("mode9", 9, 0x01u, ROUTE_COMMON),
    MODE_CASE("mode-int-max", INT32_MAX, 0x01u, ROUTE_COMMON),
};

static const Fixture s_natural_fixture = {
    "natural-slot6", 1, 0u, ROUTE_COMMON, 3,
    0u, 1u, 0u, 0u, 0u, 0u,
    0u, 0u, 0u, 0x55555555u
};

static const Fixture s_return_fixture =
    MODE_CASE("return-residue", 1, 0u, ROUTE_COMMON);

typedef struct SraCase {
    u32 input;
    u32 expected;
} SraCase;

typedef struct ModeClassCase {
    s32 input;
    enum wm_dd6c_mode_family expected;
} ModeClassCase;

static const SraCase s_sra_cases[] = {
    { 0x00001000u, 0x00000001u },
    { 0x7FFFF000u, 0x0007FFFFu },
    { 0x80000000u, 0xFFF80000u },
    { 0xFFFFF000u, 0xFFFFFFFFu },
    { 0xFFFFFFFFu, 0xFFFFFFFFu },
};

static const ModeClassCase s_mode_class_cases[] = {
    { INT32_MIN, WM_DD6C_MODE_NONPOSITIVE },
    { -1,        WM_DD6C_MODE_NONPOSITIVE },
    { 0,         WM_DD6C_MODE_NONPOSITIVE },
    { 1,         WM_DD6C_MODE_1_TO_3 },
    { 3,         WM_DD6C_MODE_1_TO_3 },
    { 4,         WM_DD6C_MODE_4_TO_7 },
    { 7,         WM_DD6C_MODE_4_TO_7 },
    { 8,         WM_DD6C_MODE_8_OR_GREATER },
    { INT32_MAX, WM_DD6C_MODE_8_OR_GREATER },
};

static enum wm_dd6c_mode_family mutant_classify_mode_unsigned(s32 mode)
{
    u32 bits = s32_as_u32(mode);

    if (bits == 0u)
        return WM_DD6C_MODE_NONPOSITIVE;
    if (bits < 4u)
        return WM_DD6C_MODE_1_TO_3;
    if (bits < 8u)
        return WM_DD6C_MODE_4_TO_7;
    return WM_DD6C_MODE_8_OR_GREATER;
}

static void run_mode_classifier_tests(void)
{
    size_t index;
    bool unsigned_mutant_matches_all = true;

    for (index = 0u;
         index < sizeof(s_mode_class_cases) / sizeof(s_mode_class_cases[0]);
         index++) {
        enum wm_dd6c_mode_family actual =
            wm_dd6c_classify_mode(s_mode_class_cases[index].input);
        enum wm_dd6c_mode_family mutant =
            mutant_classify_mode_unsigned(s_mode_class_cases[index].input);

        check_case("signed-mode-classifier",
                   "production signed family result exact",
                   actual == s_mode_class_cases[index].expected);
        if (actual != mutant)
            unsigned_mutant_matches_all = false;
    }
    report_mutant("unsigned mode-family classifier wrong model",
                  !unsigned_mutant_matches_all);
}

static void run_full_width_sra_tests(void)
{
    size_t index;
    bool logical_mutant_matches_all = true;

    for (index = 0u; index < sizeof(s_sra_cases) / sizeof(s_sra_cases[0]);
         index++) {
        u32 actual = wm_dd6c_sra12(s_sra_cases[index].input);
        u32 logical_mutant = s_sra_cases[index].input >> 12;

        check_case("full-width-sra12", "production SRA12 exact full result",
                   actual == s_sra_cases[index].expected);
        if (actual != logical_mutant)
            logical_mutant_matches_all = false;
    }
    report_mutant("logical SRL substituted for arithmetic SRA12",
                  !logical_mutant_matches_all);
}

int main(void)
{
    size_t index;
    Fixture return_case;
    RunResult natural;
    RunResult residue[3];
    const s32 returns[3] = { 1, 3, (s32)0x13579BDF };

    printf("W34B7-D wm_8008DD6C production-linked test\n");
    printf("ORACLE: retail DD6C listing/write table + declarative routes\n");
    run_mode_classifier_tests();
    run_full_width_sra_tests();
    for (index = 0u; index < sizeof(s_mode_cases) / sizeof(s_mode_cases[0]);
         index++)
        (void)run_fixture(&s_mode_cases[index]);

    natural = run_fixture(&s_natural_fixture);
    check_case("natural-slot6", "natural return is three",
               natural.return_value == 3);
    check_case("natural-slot6", "natural direct terrain skipped",
               natural.terrain_calls == 0);
    check_case("natural-slot6", "natural object is zero before C364",
               buffer_load_u32(s_ram_before,
                               s_slot_addr + TEST_SLOT_OBJECT) == 0u);
    check_case("natural-slot6", "natural missing-primary object remains zero",
               raw_load_u32(s_slot_addr + TEST_SLOT_OBJECT) == 0u);
    check_case("natural-slot6", "natural C364 missing-primary +24 is one",
               raw_load_u16(s_slot_addr + TEST_SLOT_FLAG) == 1u);
    check_case("natural-slot6", "natural C364 XYZ remain zero",
               raw_load_u32(s_slot_addr + TEST_SLOT_X) == 0u &&
               raw_load_u32(s_slot_addr + TEST_SLOT_Y) == 0u &&
               raw_load_u32(s_slot_addr + TEST_SLOT_Z) == 0u);
    check_case("natural-slot6", "natural mode1 reads F8E7 once",
               natural.flag_reads == 1);

    for (index = 0u; index < 3u; index++) {
        return_case = s_return_fixture;
        return_case.c364_return = returns[index];
        residue[index] = run_fixture(&return_case);
    }
    check_case("return-independence", "all writes independent of return residue",
               residue[0].ram_hash == residue[1].ram_hash &&
               residue[1].ram_hash == residue[2].ram_hash);
    check_case("return-independence", "all residues returned bit-exact",
               residue[0].return_value == returns[0] &&
               residue[1].return_value == returns[1] &&
               residue[2].return_value == returns[2]);

    printf("PASS/TOTAL: %d/%d\n", s_pass_count, s_total_count);
    return s_failure_count == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
