/*
 * W34B9-A production-linked certification for world callback 0x8008E190.
 *
 * The actual production translation unit is included after renaming its four
 * direct dependency symbols to controlled recorders.  The oracle below is
 * independently table-driven from the retail jump tables and write set; it
 * does not call production helpers or derive expected routes from production
 * switch statements.  Every fixture starts with asymmetric full-RAM, slot,
 * scratchpad, and 18 x 84-byte context contents.
 */
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"

static void* test_psx_addr(uintptr_t address_bits) __attribute__((noinline));

#undef PSX_ADDR
#define PSX_ADDR(address) test_psx_addr((uintptr_t)(address))
#define wm_8008DFF4 test_wm_8008DFF4
#define wm_80093978 test_wm_80093978
#define RotMatrixYXZ test_RotMatrixYXZ
#define wm_8008E034 test_wm_8008E034

#ifndef WM_E190_PRODUCTION_SOURCE
#define WM_E190_PRODUCTION_SOURCE "../src/world_map_callback_8e190.c"
#endif
#include WM_E190_PRODUCTION_SOURCE

#undef wm_8008DFF4
#undef wm_80093978
#undef RotMatrixYXZ
#undef wm_8008E034

#define TEST_RAM_MASK       0x001FFFFFu
#define TEST_MAIN_RAM_BYTES 0x00200000u
#define TEST_POOL_PTR       0x8009BE24u
#define TEST_MODE           0x8009BE10u
#define TEST_EE60           0x8006EE60u
#define TEST_EE62           0x8006EE62u
#define TEST_EE64           0x8006EE64u
#define TEST_EE66           0x8006EE66u
#define TEST_EE68           0x8006EE68u
#define TEST_BD3C           0x8009BD3Cu
#define TEST_C170           0x8009C170u
#define TEST_C584           0x8009C584u
#define TEST_C5A8           0x8009C5A8u
#define TEST_C5AC           0x8009C5ACu
#define TEST_C5B0           0x8009C5B0u
#define TEST_C5B4           0x8009C5B4u
#define TEST_C620           0x8009C620u
#define TEST_D52C           0x8009D52Cu
#define TEST_D55C           0x8009D55Cu
#define TEST_POOL_BASE      0x800D7538u
#define TEST_CONTEXT_ROOT   0x800D9000u
#define TEST_CONTEXT_ALT    0x800DA000u
#define TEST_CONTEXT_COUNT  18u
#define TEST_RECORD_STRIDE  0x54u
#define TEST_CONTEXT_BYTES  (TEST_CONTEXT_COUNT * TEST_RECORD_STRIDE)
#define TEST_SLOT_INDEX     7
#define TEST_SLOT_STRIDE    0x80u
#define TEST_TRACE_CAPACITY 192u
#define TEST_NO_HALF        0xFFFFFFFFu

#define SLOT_STATE   0x00u
#define SLOT_CONTROL 0x20u
#define SLOT_FLAG    0x24u
#define SLOT_X       0x28u
#define SLOT_Y       0x2Cu
#define SLOT_Z       0x30u
#define SLOT_W       0x34u
#define SLOT_CLEAR38 0x38u
#define SLOT_CLEAR3C 0x3Cu
#define SLOT_CLEAR40 0x40u
#define SLOT_HEADING 0x48u
#define SLOT_VARIANT 0x4Au
#define SLOT_OBJECT  0x4Cu
#define SLOT_CLEAR60 0x60u
#define SLOT_CLEAR64 0x64u
#define SLOT_CONST68 0x68u
#define SLOT_CLEAR6C 0x6Cu
#define SLOT_ROTX    0x70u
#define SLOT_AUX     0x74u
#define SLOT_CLEAR7C 0x7Cu

#define CTX_KIND    0x00u
#define CTX_X       0x08u
#define CTX_Y       0x0Cu
#define CTX_Z       0x10u
#define CTX_W       0x14u
#define CTX_MATRIX0 0x20u
#define CTX_REC1    0x54u
#define CTX_MATRIX1 0x74u
#define CTX_REC2    0xA8u
#define CTX_REC3    0xFCu

_Static_assert(PSX_RAM_SIZE >= TEST_MAIN_RAM_BYTES,
               "test requires complete PSX main RAM");
_Static_assert(WM_E190_POOL_PTR == TEST_POOL_PTR,
               "retail pool pointer address");
_Static_assert(WM_E190_MODE == TEST_MODE, "retail signed mode address");
_Static_assert(WM_E190_HEADING == TEST_EE66, "retail heading address");
_Static_assert(WM_E190_VARIANT == TEST_EE68, "retail variant address");
_Static_assert(WM_E190_CONTEXT_PTR == TEST_C620,
               "retail C620 root address");
_Static_assert(WM_E190_CONTEXT_RECORD1 == TEST_RECORD_STRIDE,
               "retail context record stride");
_Static_assert(WM_E190_CONTEXT_RECORD2 == 2u * TEST_RECORD_STRIDE &&
                   WM_E190_CONTEXT_RECORD3 == 3u * TEST_RECORD_STRIDE,
               "retail adjacent-record offsets");
_Static_assert(WM_E190_CONTEXT_MATRIX0 == CTX_MATRIX0 &&
                   WM_E190_CONTEXT_MATRIX1 == CTX_MATRIX1,
               "retail matrix destinations");

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

typedef enum TraceKind {
    TRACE_ADDRESS = 1,
    TRACE_DFF4,
    TRACE_TERRAIN,
    TRACE_MATRIX,
    TRACE_E034
} TraceKind;

typedef struct TraceEvent {
    TraceKind kind;
    u32 first;
    u32 second;
} TraceEvent;

typedef enum ModeRoute {
    ROUTE_DEFAULT = 0,
    ROUTE_MODE1,
    ROUTE_MODE23,
    ROUTE_MODE45,
    ROUTE_MODE7
} ModeRoute;

typedef struct Fixture {
    const char* name;
    s32 slot_index;
    s32 mode;
    ModeRoute route;
    u16 variant;
    s32 expected_return;
    u32 variant_halfword;
    s32 tail_mode;
    u32 tail_control;
    bool clear_7c;
    bool matrix_repoints_root;
} Fixture;

typedef struct RunObservation {
    bool return_ok;
    bool trace_ok;
    bool ram_ok;
    bool scratch_ok;
    bool seam_ok;
    int dff4_calls;
    int terrain_calls;
    int matrix_calls;
    int e034_calls;
} RunObservation;

static u8 s_expected_ram[PSX_RAM_SIZE];
static u8 s_expected_scratch[sizeof(g_PsxScratchpad)];
static TraceEvent s_trace[TEST_TRACE_CAPACITY];
static TraceEvent s_expected_trace[TEST_TRACE_CAPACITY];
static size_t s_trace_count;
static size_t s_expected_trace_count;
static bool s_trace_overflow;
static bool s_trace_armed;
static const Fixture* s_active_fixture;
static u32 s_slot_addr;
static u32 s_expected_terrain_x;
static u32 s_expected_terrain_z;
static u32 s_terrain_return;
static u16 s_expected_rotation_x;
static u16 s_expected_rotation_y;
static u16 s_expected_rotation_z;
static u32 s_expected_matrix_dest[2];
static bool s_seam_ok;
static int s_dff4_calls;
static int s_terrain_calls;
static int s_matrix_calls;
static int s_e034_calls;
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

static void trace_append(TraceEvent* trace, size_t* count,
                         TraceKind kind, u32 first, u32 second)
{
    if (*count < (size_t)TEST_TRACE_CAPACITY) {
        trace[*count].kind = kind;
        trace[*count].first = first;
        trace[*count].second = second;
    } else {
        s_trace_overflow = true;
    }
    (*count)++;
}

static void actual_trace(TraceKind kind, u32 first, u32 second)
{
    trace_append(s_trace, &s_trace_count, kind, first, second);
}

static void expected_trace(TraceKind kind, u32 first, u32 second)
{
    trace_append(s_expected_trace, &s_expected_trace_count,
                 kind, first, second);
}

static void expected_address(u32 address)
{
    expected_trace(TRACE_ADDRESS, address, 0u);
}

static void* test_psx_addr(uintptr_t address_bits)
{
    u32 address = (u32)address_bits;

    if (s_trace_armed)
        actual_trace(TRACE_ADDRESS, address, 0u);
    if ((address & 0xFFFFF000u) == 0x1F800000u)
        return (void*)(g_PsxScratchpad + (address & 0xFFFu));
    return raw_address(address);
}

static void buffer_store_u16(u8* buffer, u32 address, u16 value)
{
    memcpy(buffer + raw_index(address), &value, sizeof(value));
}

static void buffer_store_u32(u8* buffer, u32 address, u32 value)
{
    memcpy(buffer + raw_index(address), &value, sizeof(value));
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

static void raw_store_u16(u32 address, u16 value)
{
    buffer_store_u16(g_PsxRam, address, value);
}

static void raw_store_u32(u32 address, u32 value)
{
    buffer_store_u32(g_PsxRam, address, value);
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
    u32 result;
    memcpy(&result, &value, sizeof(result));
    return result;
}

static s32 u32_as_s32(u32 value)
{
    s32 result;
    memcpy(&result, &value, sizeof(result));
    return result;
}

static u32 oracle_sra12(u32 value)
{
    u32 result = value >> 12;
    if ((value & 0x80000000u) != 0u)
        result |= 0xFFF00000u;
    return result;
}

static u32 oracle_dff4(u16 value)
{
    u32 extended = (value <= 0x7FFFu)
                       ? (u32)value
                       : 0xFFFF0000u | (u32)value;
    return extended << 12;
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

static void report_mutant(const char* name, bool detected)
{
    check_case("mutant", name, detected);
    if (detected)
        printf("MUTANT DETECTED: %s\n", name);
}

static u16 matrix_marker(size_t call_index, size_t element)
{
    return (u16)(0x6100u + (u32)(call_index * 0x20u + element));
}

static void oracle_write_matrix(u8* buffer, u32 destination,
                                size_t call_index)
{
    size_t element;
    for (element = 0u; element < 9u; element++) {
        buffer_store_u16(buffer, destination + (u32)(element * 2u),
                         matrix_marker(call_index, element));
    }
}

static u32 host_ram_to_guest(const void* pointer)
{
    uintptr_t base = (uintptr_t)(const void*)g_PsxRam;
    uintptr_t value = (uintptr_t)pointer;
    uintptr_t offset = value - base;
    return 0x80000000u | (u32)offset;
}

/* Controlled DFF4 seam. Retail call order/argument are verified separately;
 * the seam's data effects use the independently accepted helper authority. */
void test_wm_8008DFF4(u32 out_addr)
{
    u32 x_bits;
    u32 y_bits;
    u32 z_bits;

    s_dff4_calls++;
    actual_trace(TRACE_DFF4, out_addr, 0u);
    if (out_addr != s_slot_addr + SLOT_X ||
        raw_load_u16(s_slot_addr + SLOT_FLAG) != 0u)
        s_seam_ok = false;

    x_bits = oracle_dff4(raw_load_u16(TEST_EE60));
    y_bits = oracle_dff4(raw_load_u16(TEST_EE62));
    z_bits = oracle_dff4(raw_load_u16(TEST_EE64));
    raw_store_u32(out_addr + 0u, x_bits);
    raw_store_u32(out_addr + 4u, y_bits);
    raw_store_u32(out_addr + 8u, z_bits);
}

s32 test_wm_80093978(s32 x, s32 z)
{
    u32 x_bits = s32_as_u32(x);
    u32 z_bits = s32_as_u32(z);

    s_terrain_calls++;
    actual_trace(TRACE_TERRAIN, x_bits, z_bits);
    if (x_bits != s_expected_terrain_x || z_bits != s_expected_terrain_z)
        s_seam_ok = false;
    return u32_as_s32(s_terrain_return);
}

MATRIX* test_RotMatrixYXZ(SVECTOR* rotation, MATRIX* matrix)
{
    u32 destination = host_ram_to_guest((const void*)matrix);
    size_t call_index = (size_t)s_matrix_calls;
    u16 rx;
    u16 ry;
    u16 rz;
    size_t element;

    s_matrix_calls++;
    actual_trace(TRACE_MATRIX, destination, 0u);
    memcpy(&rx, (const u8*)(const void*)rotation + 0u, sizeof(rx));
    memcpy(&ry, (const u8*)(const void*)rotation + 2u, sizeof(ry));
    memcpy(&rz, (const u8*)(const void*)rotation + 4u, sizeof(rz));
    if (call_index >= 2u || destination != s_expected_matrix_dest[call_index] ||
        rx != s_expected_rotation_x || ry != s_expected_rotation_y ||
        rz != s_expected_rotation_z)
        s_seam_ok = false;

    if (call_index == 0u) {
        if (raw_load_u32(TEST_C620) != TEST_CONTEXT_ROOT ||
            raw_load_u32(TEST_CONTEXT_ROOT + CTX_X) !=
                raw_load_u32(s_slot_addr + SLOT_X) ||
            raw_load_u16(TEST_CONTEXT_ROOT + CTX_KIND) != 0u)
            s_seam_ok = false;
    }

    for (element = 0u; element < 9u; element++) {
        u16 marker = matrix_marker(call_index, element);
        memcpy((u8*)(void*)matrix + element * 2u, &marker, sizeof(marker));
    }

    if (call_index == 0u && s_active_fixture->matrix_repoints_root)
        raw_store_u32(TEST_C620, TEST_CONTEXT_ALT);
    return matrix;
}

void test_wm_8008E034(u32 in_addr)
{
    u32 component;

    s_e034_calls++;
    actual_trace(TRACE_E034, in_addr, 0u);
    if (in_addr != s_slot_addr + SLOT_X || s_matrix_calls != 2)
        s_seam_ok = false;
    for (component = 0u; component < 3u; component++) {
        u32 bits = raw_load_u32(in_addr + component * 4u);
        raw_store_u16(TEST_EE60 + component * 2u,
                      (u16)oracle_sra12(bits));
    }
}

static void seed_fixture(const Fixture* fixture)
{
    size_t index;
    u32 slot_offset = (u32)fixture->slot_index << 7;

    for (index = 0u; index < sizeof(g_PsxRam); index++)
        g_PsxRam[index] = (u8)((index * 37u + 11u) & 0xFFu);
    for (index = 0u; index < sizeof(g_PsxScratchpad); index++)
        g_PsxScratchpad[index] = (u8)((index * 29u + 0x5Du) & 0xFFu);

    raw_store_u32(TEST_POOL_PTR, TEST_POOL_BASE);
    raw_store_u32(TEST_MODE, s32_as_u32(fixture->mode));
    raw_store_u16(TEST_EE60, 0x8001u);
    raw_store_u16(TEST_EE62, 0x7FFFu);
    raw_store_u16(TEST_EE64, 0xFFFFu);
    raw_store_u16(TEST_EE66, 0xA55Au);
    raw_store_u16(TEST_EE68, fixture->variant);
    raw_store_u16(TEST_BD3C, 0xBEEFu);
    raw_store_u32(TEST_C170, 0xCAFE5678u);
    raw_store_u32(TEST_C584, 0xDEAD1234u);
    raw_store_u32(TEST_C5A8, s32_as_u32(fixture->tail_mode));
    raw_store_u32(TEST_C5AC, 0x81234567u);
    raw_store_u32(TEST_C5B0, 0x87654321u);
    raw_store_u32(TEST_C5B4, 0xFEDCBA98u);
    raw_store_u32(TEST_C620, TEST_CONTEXT_ROOT);
    raw_store_u32(TEST_D52C, 0xD52CD52Cu);
    raw_store_u32(TEST_D55C + 0u, 0xD55CD55Cu);
    raw_store_u32(TEST_D55C + 4u, 0xD560D560u);
    raw_store_u32(TEST_D55C + 8u, 0xD564D564u);
    raw_store_u32(TEST_D55C + 12u, 0xD568D568u);

    s_slot_addr = TEST_POOL_BASE + slot_offset;
    for (index = 0u; index < TEST_SLOT_STRIDE; index++)
        g_PsxRam[raw_index(s_slot_addr) + index] =
            (u8)((index * 13u + 0x41u) & 0xFFu);
    for (index = 0u; index < TEST_CONTEXT_BYTES; index++) {
        g_PsxRam[raw_index(TEST_CONTEXT_ROOT) + index] =
            (u8)((index * 17u + 0x23u) & 0xFFu);
        g_PsxRam[raw_index(TEST_CONTEXT_ALT) + index] =
            (u8)((index * 19u + 0x71u) & 0xFFu);
    }

    memcpy(s_expected_ram, g_PsxRam, sizeof(g_PsxRam));
    memcpy(s_expected_scratch, g_PsxScratchpad,
           sizeof(g_PsxScratchpad));
}

static void oracle_publish_position(u32 slot_addr)
{
    u32 x_bits = buffer_load_u32(s_expected_ram, slot_addr + SLOT_X);
    u32 y_bits = buffer_load_u32(s_expected_ram, slot_addr + SLOT_Y);
    u32 z_bits = buffer_load_u32(s_expected_ram, slot_addr + SLOT_Z);
    u32 w_bits = buffer_load_u32(s_expected_ram, slot_addr + SLOT_W);

    buffer_store_u32(s_expected_ram, TEST_D55C + 0u, x_bits);
    buffer_store_u32(s_expected_ram, TEST_D55C + 4u, y_bits);
    buffer_store_u32(s_expected_ram, TEST_D55C + 8u, z_bits);
    buffer_store_u32(s_expected_ram, TEST_D55C + 12u, w_bits);
}

static void oracle_apply_image(const Fixture* fixture)
{
    u32 slot = s_slot_addr;
    u32 x_bits = oracle_dff4(buffer_load_u16(s_expected_ram, TEST_EE60));
    u32 y_bits = oracle_dff4(buffer_load_u16(s_expected_ram, TEST_EE62));
    u32 z_bits = oracle_dff4(buffer_load_u16(s_expected_ram, TEST_EE64));
    u16 heading = buffer_load_u16(s_expected_ram, TEST_EE66);
    u32 context = TEST_CONTEXT_ROOT;
    u32 component;

    buffer_store_u16(s_expected_ram, slot + SLOT_FLAG, 0u);
    buffer_store_u32(s_expected_ram, slot + SLOT_X, x_bits);
    buffer_store_u32(s_expected_ram, slot + SLOT_Y, y_bits);
    buffer_store_u32(s_expected_ram, slot + SLOT_Z, z_bits);
    buffer_store_u32(s_expected_ram, slot + SLOT_CLEAR40, 0u);
    buffer_store_u32(s_expected_ram, slot + SLOT_CLEAR3C, 0u);
    buffer_store_u32(s_expected_ram, slot + SLOT_CLEAR38, 0u);
    buffer_store_u32(s_expected_ram, slot + SLOT_CLEAR64, 0u);
    buffer_store_u32(s_expected_ram, slot + SLOT_CLEAR60, 0u);
    buffer_store_u32(s_expected_ram, slot + SLOT_AUX, 0u);
    buffer_store_u32(s_expected_ram, slot + SLOT_CONST68, 0xFFD80000u);
    buffer_store_u32(s_expected_ram, slot + SLOT_ROTX, 0u);
    buffer_store_u32(s_expected_ram, slot + SLOT_CLEAR6C, 0u);
    buffer_store_u16(s_expected_ram, slot + SLOT_HEADING, heading);
    if (fixture->variant_halfword != TEST_NO_HALF)
        buffer_store_u16(s_expected_ram, slot + SLOT_VARIANT,
                         (u16)fixture->variant_halfword);

    switch (fixture->route) {
    case ROUTE_MODE1:
        buffer_store_u32(s_expected_ram, slot + SLOT_Y, s_terrain_return);
        break;
    case ROUTE_MODE23:
        buffer_store_u16(s_expected_ram, slot + SLOT_CONTROL, 1u);
        buffer_store_u32(s_expected_ram, slot + SLOT_Y, s_terrain_return);
        break;
    case ROUTE_MODE45:
        buffer_store_u16(s_expected_ram, slot + SLOT_CONTROL, 3u);
        buffer_store_u32(s_expected_ram, slot + SLOT_X,
                         buffer_load_u32(s_expected_ram, TEST_C5AC));
        buffer_store_u32(s_expected_ram, slot + SLOT_Z,
                         buffer_load_u32(s_expected_ram, TEST_C5B4));
        buffer_store_u32(s_expected_ram, slot + SLOT_Y,
                         s_terrain_return + 0xFFFC0000u);
        heading = (u16)buffer_load_u32(s_expected_ram, TEST_C584);
        buffer_store_u16(s_expected_ram, slot + SLOT_HEADING, heading);
        buffer_store_u32(s_expected_ram, slot + SLOT_AUX,
                         buffer_load_u32(s_expected_ram, TEST_C170));
        oracle_publish_position(slot);
        buffer_store_u16(s_expected_ram,
                         TEST_CONTEXT_ROOT + CTX_REC3, 1u);
        buffer_store_u16(s_expected_ram,
                         TEST_CONTEXT_ROOT + CTX_REC2, 1u);
        buffer_store_u16(s_expected_ram,
                         TEST_CONTEXT_ROOT + CTX_REC1, 1u);
        buffer_store_u16(s_expected_ram, TEST_D52C, heading);
        break;
    case ROUTE_MODE7:
        buffer_store_u16(s_expected_ram, slot + SLOT_CONTROL, 2u);
        buffer_store_u32(s_expected_ram, slot + SLOT_X,
                         buffer_load_u32(s_expected_ram, TEST_C5AC));
        buffer_store_u32(s_expected_ram, slot + SLOT_Z,
                         buffer_load_u32(s_expected_ram, TEST_C5B4));
        buffer_store_u32(s_expected_ram, slot + SLOT_Y,
                         buffer_load_u32(s_expected_ram, TEST_C5B0));
        heading = (u16)buffer_load_u32(s_expected_ram, TEST_C584);
        buffer_store_u16(s_expected_ram, slot + SLOT_HEADING, heading);
        buffer_store_u32(s_expected_ram, slot + SLOT_AUX,
                         buffer_load_u32(s_expected_ram, TEST_C170));
        oracle_publish_position(slot);
        buffer_store_u16(s_expected_ram, TEST_D52C, heading);
        break;
    case ROUTE_DEFAULT:
        break;
    }

    x_bits = buffer_load_u32(s_expected_ram, slot + SLOT_X);
    y_bits = buffer_load_u32(s_expected_ram, slot + SLOT_Y);
    z_bits = buffer_load_u32(s_expected_ram, slot + SLOT_Z);
    buffer_store_u32(s_expected_ram, context + CTX_X, x_bits);
    buffer_store_u32(s_expected_ram, context + CTX_Y, y_bits);
    buffer_store_u32(s_expected_ram, context + CTX_Z, z_bits);
    buffer_store_u32(s_expected_ram, context + CTX_W,
                     buffer_load_u32(s_expected_ram, slot + SLOT_W));
    buffer_store_u16(s_expected_ram, context + CTX_KIND,
                     buffer_load_u16(s_expected_ram, slot + SLOT_FLAG));

    buffer_store_u16(s_expected_scratch, WM_E190_SCRATCH_VECTOR_OFFSET + 0u,
                     0u);
    buffer_store_u16(s_expected_scratch, WM_E190_SCRATCH_VECTOR_OFFSET + 4u,
                     buffer_load_u16(s_expected_ram, TEST_BD3C));
    buffer_store_u16(s_expected_scratch, WM_E190_SCRATCH_VECTOR_OFFSET + 2u,
                     heading);
    oracle_write_matrix(s_expected_ram, context + CTX_MATRIX0, 0u);
    if (fixture->matrix_repoints_root) {
        buffer_store_u32(s_expected_ram, TEST_C620, TEST_CONTEXT_ALT);
        context = TEST_CONTEXT_ALT;
    }
    oracle_write_matrix(s_expected_ram, context + CTX_MATRIX1, 1u);

    for (component = 0u; component < 3u; component++) {
        u32 bits = buffer_load_u32(s_expected_ram,
                                   slot + SLOT_X + component * 4u);
        buffer_store_u16(s_expected_ram, TEST_EE60 + component * 2u,
                         (u16)oracle_sra12(bits));
    }
    buffer_store_u16(s_expected_ram, TEST_EE66, heading);
    if (fixture->tail_control != TEST_NO_HALF)
        buffer_store_u16(s_expected_ram, slot + SLOT_CONTROL,
                         (u16)fixture->tail_control);
    if (fixture->clear_7c)
        buffer_store_u32(s_expected_ram, slot + SLOT_CLEAR7C, 0u);

    s_expected_terrain_x = (fixture->route == ROUTE_MODE45)
                               ? buffer_load_u32(s_expected_ram, TEST_C5AC)
                               : oracle_dff4(0x8001u);
    s_expected_terrain_z = (fixture->route == ROUTE_MODE45)
                               ? buffer_load_u32(s_expected_ram, TEST_C5B4)
                               : oracle_dff4(0xFFFFu);
    s_expected_rotation_x = 0u;
    s_expected_rotation_y = heading;
    s_expected_rotation_z = buffer_load_u16(s_expected_ram, TEST_BD3C);
    s_expected_matrix_dest[0] = TEST_CONTEXT_ROOT + CTX_MATRIX0;
    s_expected_matrix_dest[1] =
        (fixture->matrix_repoints_root ? TEST_CONTEXT_ALT : TEST_CONTEXT_ROOT) +
        CTX_MATRIX1;
}

static void oracle_trace_prefix(const Fixture* fixture)
{
    u32 slot = s_slot_addr;

    expected_address(TEST_POOL_PTR);
    expected_address(slot + SLOT_FLAG);
    expected_trace(TRACE_DFF4, slot + SLOT_X, 0u);
    expected_address(TEST_EE66);
    expected_address(slot + SLOT_CLEAR40);
    expected_address(slot + SLOT_CLEAR3C);
    expected_address(slot + SLOT_CLEAR38);
    expected_address(slot + SLOT_CLEAR64);
    expected_address(slot + SLOT_CLEAR60);
    expected_address(slot + SLOT_AUX);
    expected_address(slot + SLOT_CONST68);
    expected_address(slot + SLOT_ROTX);
    expected_address(slot + SLOT_CLEAR6C);
    expected_address(slot + SLOT_HEADING);
    expected_address(TEST_EE68);
    if (fixture->variant_halfword != TEST_NO_HALF)
        expected_address(slot + SLOT_VARIANT);
    expected_address(TEST_MODE);
}

static void oracle_trace_position_publish(void)
{
    expected_address(s_slot_addr + SLOT_X);
    expected_address(s_slot_addr + SLOT_Y);
    expected_address(s_slot_addr + SLOT_Z);
    expected_address(TEST_D55C + 0u);
    expected_address(TEST_D55C + 4u);
    expected_address(TEST_D55C + 8u);
    expected_address(s_slot_addr + SLOT_W);
    expected_address(TEST_D55C + 12u);
}

static void oracle_trace_route(const Fixture* fixture)
{
    u32 slot = s_slot_addr;

    switch (fixture->route) {
    case ROUTE_MODE1:
        expected_address(slot + SLOT_X);
        expected_address(slot + SLOT_Z);
        expected_trace(TRACE_TERRAIN, s_expected_terrain_x,
                       s_expected_terrain_z);
        expected_address(slot + SLOT_Y);
        break;
    case ROUTE_MODE23:
        expected_address(slot + SLOT_X);
        expected_address(slot + SLOT_Z);
        expected_address(slot + SLOT_CONTROL);
        expected_trace(TRACE_TERRAIN, s_expected_terrain_x,
                       s_expected_terrain_z);
        expected_address(slot + SLOT_Y);
        break;
    case ROUTE_MODE45:
        expected_address(slot + SLOT_CONTROL);
        expected_address(TEST_C5AC);
        expected_address(slot + SLOT_X);
        expected_address(TEST_C5B4);
        expected_address(slot + SLOT_X);
        expected_address(slot + SLOT_Z);
        expected_trace(TRACE_TERRAIN, s_expected_terrain_x,
                       s_expected_terrain_z);
        expected_address(TEST_C584);
        expected_address(TEST_C170);
        expected_address(slot + SLOT_Y);
        expected_address(slot + SLOT_HEADING);
        expected_address(slot + SLOT_AUX);
        oracle_trace_position_publish();
        expected_address(slot + SLOT_HEADING);
        expected_address(TEST_C620);
        expected_address(TEST_CONTEXT_ROOT + CTX_REC3);
        expected_address(TEST_CONTEXT_ROOT + CTX_REC2);
        expected_address(TEST_CONTEXT_ROOT + CTX_REC1);
        expected_address(TEST_D52C);
        break;
    case ROUTE_MODE7:
        expected_address(slot + SLOT_CONTROL);
        expected_address(TEST_C5AC);
        expected_address(TEST_C584);
        expected_address(slot + SLOT_X);
        expected_address(TEST_C5B4);
        expected_address(slot + SLOT_Z);
        expected_address(TEST_C5B0);
        expected_address(TEST_C170);
        expected_address(slot + SLOT_HEADING);
        expected_address(slot + SLOT_AUX);
        expected_address(slot + SLOT_Y);
        oracle_trace_position_publish();
        expected_address(slot + SLOT_HEADING);
        expected_address(TEST_D52C);
        break;
    case ROUTE_DEFAULT:
        break;
    }
}

static void oracle_trace_common(const Fixture* fixture)
{
    u32 second_root = fixture->matrix_repoints_root
                          ? TEST_CONTEXT_ALT
                          : TEST_CONTEXT_ROOT;

    expected_address(TEST_C620);
    expected_address(s_slot_addr + SLOT_X);
    expected_address(s_slot_addr + SLOT_Y);
    expected_address(s_slot_addr + SLOT_Z);
    expected_address(s_slot_addr + SLOT_W);
    expected_address(TEST_CONTEXT_ROOT + CTX_X);
    expected_address(TEST_CONTEXT_ROOT + CTX_Y);
    expected_address(TEST_CONTEXT_ROOT + CTX_Z);
    expected_address(TEST_CONTEXT_ROOT + CTX_W);
    expected_address(TEST_C620);
    expected_address(s_slot_addr + SLOT_FLAG);
    expected_address(TEST_CONTEXT_ROOT + CTX_KIND);
    expected_address(s_slot_addr + SLOT_ROTX);
    expected_address(s_slot_addr + SLOT_HEADING);
    expected_address(TEST_BD3C);
    expected_address(TEST_CONTEXT_ROOT + CTX_MATRIX0);
    expected_trace(TRACE_MATRIX, TEST_CONTEXT_ROOT + CTX_MATRIX0, 0u);
    expected_address(TEST_C620);
    expected_address(second_root + CTX_MATRIX1);
    expected_trace(TRACE_MATRIX, second_root + CTX_MATRIX1, 0u);
    expected_trace(TRACE_E034, s_slot_addr + SLOT_X, 0u);
    expected_address(s_slot_addr + SLOT_HEADING);
    expected_address(TEST_C5A8);
    expected_address(TEST_EE66);
    if (fixture->tail_control != TEST_NO_HALF)
        expected_address(s_slot_addr + SLOT_CONTROL);
    if (fixture->clear_7c)
        expected_address(s_slot_addr + SLOT_CLEAR7C);
}

static bool compare_trace(const Fixture* fixture)
{
    size_t index;

    if (s_trace_overflow || s_trace_count != s_expected_trace_count) {
        printf("TRACE [%s]: got=%zu expected=%zu overflow=%d\n",
               fixture->name, s_trace_count, s_expected_trace_count,
               s_trace_overflow ? 1 : 0);
        return false;
    }
    for (index = 0u; index < s_trace_count; index++) {
        if (s_trace[index].kind != s_expected_trace[index].kind ||
            s_trace[index].first != s_expected_trace[index].first ||
            s_trace[index].second != s_expected_trace[index].second) {
            printf("TRACE [%s] #%zu got=(%d,%08X,%08X) "
                   "expected=(%d,%08X,%08X)\n",
                   fixture->name, index, (int)s_trace[index].kind,
                   s_trace[index].first, s_trace[index].second,
                   (int)s_expected_trace[index].kind,
                   s_expected_trace[index].first,
                   s_expected_trace[index].second);
            return false;
        }
    }
    return true;
}

static bool compare_bytes(const u8* actual, const u8* expected, size_t size,
                          const char* fixture, const char* region)
{
    size_t index;
    for (index = 0u; index < size; index++) {
        if (actual[index] != expected[index]) {
            printf("BYTES [%s:%s] offset=%08zX got=%02X expected=%02X\n",
                   fixture, region, index, actual[index], expected[index]);
            return false;
        }
    }
    return true;
}

static RunObservation run_fixture(const Fixture* fixture)
{
    RunObservation observation;
    s32 result;

    seed_fixture(fixture);
    s_active_fixture = fixture;
    s_terrain_return = 0x10203040u;
    oracle_apply_image(fixture);

    s_trace_count = 0u;
    s_expected_trace_count = 0u;
    s_trace_overflow = false;
    s_seam_ok = true;
    s_dff4_calls = 0;
    s_terrain_calls = 0;
    s_matrix_calls = 0;
    s_e034_calls = 0;
    oracle_trace_prefix(fixture);
    oracle_trace_route(fixture);
    oracle_trace_common(fixture);

    s_trace_armed = true;
    result = wm_8008E190(fixture->slot_index);
    s_trace_armed = false;

    observation.return_ok = result == fixture->expected_return;
    observation.trace_ok = compare_trace(fixture);
    observation.ram_ok = compare_bytes(g_PsxRam, s_expected_ram,
                                       sizeof(g_PsxRam), fixture->name,
                                       "full-ram-plus-guard");
    observation.scratch_ok = compare_bytes(g_PsxScratchpad,
                                           s_expected_scratch,
                                           sizeof(g_PsxScratchpad),
                                           fixture->name, "scratchpad");
    observation.seam_ok = s_seam_ok;
    observation.dff4_calls = s_dff4_calls;
    observation.terrain_calls = s_terrain_calls;
    observation.matrix_calls = s_matrix_calls;
    observation.e034_calls = s_e034_calls;

    check_case(fixture->name, "retail return", observation.return_ok);
    check_case(fixture->name, "exact retail direct-access/call trace",
               observation.trace_ok);
    check_case(fixture->name, "complete RAM authorized write image",
               observation.ram_ok);
    check_case(fixture->name, "complete scratchpad authorized write image",
               observation.scratch_ok);
    check_case(fixture->name, "helper arguments/order/call-time state",
               observation.seam_ok);
    check_case(fixture->name, "DFF4 exactly once",
               observation.dff4_calls == 1);
    check_case(fixture->name, "terrain exact gating",
               observation.terrain_calls ==
                   ((fixture->route == ROUTE_MODE1 ||
                     fixture->route == ROUTE_MODE23 ||
                     fixture->route == ROUTE_MODE45) ? 1 : 0));
    check_case(fixture->name, "RotMatrixYXZ exactly twice",
               observation.matrix_calls == 2);
    check_case(fixture->name, "E034 exactly once",
               observation.e034_calls == 1);
    return observation;
}

static void test_sra_authority(void)
{
    static const struct {
        u32 input;
        u32 output;
    } vectors[] = {
        {0x00000000u, 0x00000000u},
        {0x00001000u, 0x00000001u},
        {0x7FFFF000u, 0x0007FFFFu},
        {0x80000000u, 0xFFF80000u},
        {0xFFFFF000u, 0xFFFFFFFFu},
        {0xFFFFFFFFu, 0xFFFFFFFFu},
    };
    size_t index;

    for (index = 0u; index < sizeof(vectors) / sizeof(vectors[0]); index++)
        check_case("finite-width-sra", "SRA12 retail vector",
                   wm_e190_sra12(vectors[index].input) ==
                       vectors[index].output);
}

static const Fixture s_mode_fixtures[] = {
    {"mode-int-min", TEST_SLOT_INDEX, INT32_MIN, ROUTE_DEFAULT, 1u, 1, 12u, 0, TEST_NO_HALF, false, false},
    {"mode-minus-1", TEST_SLOT_INDEX, -1, ROUTE_DEFAULT, 1u, 1, 12u, 0, TEST_NO_HALF, false, false},
    {"mode-0", TEST_SLOT_INDEX, 0, ROUTE_DEFAULT, 1u, 1, 12u, 0, TEST_NO_HALF, false, false},
    {"mode-1-natural", TEST_SLOT_INDEX, 1, ROUTE_MODE1, 0u, 3, TEST_NO_HALF, 0, TEST_NO_HALF, false, false},
    {"mode-2", TEST_SLOT_INDEX, 2, ROUTE_MODE23, 1u, 1, 12u, 0, TEST_NO_HALF, false, false},
    {"mode-3", TEST_SLOT_INDEX, 3, ROUTE_MODE23, 1u, 1, 12u, 0, TEST_NO_HALF, false, false},
    {"mode-4", TEST_SLOT_INDEX, 4, ROUTE_MODE45, 1u, 1, 12u, 0, TEST_NO_HALF, false, false},
    {"mode-5", TEST_SLOT_INDEX, 5, ROUTE_MODE45, 1u, 1, 12u, 0, TEST_NO_HALF, false, false},
    {"mode-6-default", TEST_SLOT_INDEX, 6, ROUTE_DEFAULT, 1u, 1, 12u, 0, TEST_NO_HALF, false, false},
    {"mode-7", TEST_SLOT_INDEX, 7, ROUTE_MODE7, 1u, 1, 12u, 0, TEST_NO_HALF, false, false},
    {"mode-8", TEST_SLOT_INDEX, 8, ROUTE_DEFAULT, 1u, 1, 12u, 0, TEST_NO_HALF, false, false},
    {"mode-9", TEST_SLOT_INDEX, 9, ROUTE_DEFAULT, 1u, 1, 12u, 0, TEST_NO_HALF, false, false},
    {"mode-int-max", TEST_SLOT_INDEX, INT32_MAX, ROUTE_DEFAULT, 1u, 1, 12u, 0, TEST_NO_HALF, false, false},
};

static const Fixture s_variant_fixtures[] = {
    {"variant-0", TEST_SLOT_INDEX, 6, ROUTE_DEFAULT, 0u, 3, TEST_NO_HALF, 0, TEST_NO_HALF, false, false},
    {"variant-1", TEST_SLOT_INDEX, 6, ROUTE_DEFAULT, 1u, 1, 12u, 0, TEST_NO_HALF, false, false},
    {"variant-2", TEST_SLOT_INDEX, 6, ROUTE_DEFAULT, 2u, 1, 12u, 0, TEST_NO_HALF, false, false},
    {"variant-3", TEST_SLOT_INDEX, 6, ROUTE_DEFAULT, 3u, 1, 32u, 0, TEST_NO_HALF, false, false},
    {"variant-4", TEST_SLOT_INDEX, 6, ROUTE_DEFAULT, 4u, 1, 32u, 0, TEST_NO_HALF, false, false},
    {"variant-5-default", TEST_SLOT_INDEX, 6, ROUTE_DEFAULT, 5u, 1, TEST_NO_HALF, 0, TEST_NO_HALF, false, false},
    {"variant-mask-to-0", TEST_SLOT_INDEX, 6, ROUTE_DEFAULT, 0x2000u, 3, TEST_NO_HALF, 0, TEST_NO_HALF, false, false},
    {"variant-ffff-default", TEST_SLOT_INDEX, 6, ROUTE_DEFAULT, 0xFFFFu, 1, TEST_NO_HALF, 0, TEST_NO_HALF, false, false},
};

static const Fixture s_tail_fixtures[] = {
    {"tail-int-min", TEST_SLOT_INDEX, 6, ROUTE_DEFAULT, 1u, 1, 12u, INT32_MIN, TEST_NO_HALF, false, false},
    {"tail-minus-1", TEST_SLOT_INDEX, 6, ROUTE_DEFAULT, 1u, 1, 12u, -1, TEST_NO_HALF, false, false},
    {"tail-0", TEST_SLOT_INDEX, 6, ROUTE_DEFAULT, 1u, 1, 12u, 0, TEST_NO_HALF, false, false},
    {"tail-1", TEST_SLOT_INDEX, 6, ROUTE_DEFAULT, 1u, 1, 12u, 1, TEST_NO_HALF, false, false},
    {"tail-2", TEST_SLOT_INDEX, 6, ROUTE_DEFAULT, 1u, 1, 12u, 2, 36u, false, false},
    {"tail-3", TEST_SLOT_INDEX, 6, ROUTE_DEFAULT, 1u, 1, 12u, 3, 40u, true, false},
    {"tail-4", TEST_SLOT_INDEX, 6, ROUTE_DEFAULT, 1u, 1, 12u, 4, 48u, true, false},
    {"tail-5", TEST_SLOT_INDEX, 6, ROUTE_DEFAULT, 1u, 1, 12u, 5, 52u, true, false},
    {"tail-6", TEST_SLOT_INDEX, 6, ROUTE_DEFAULT, 1u, 1, 12u, 6, TEST_NO_HALF, false, false},
    {"tail-int-max", TEST_SLOT_INDEX, 6, ROUTE_DEFAULT, 1u, 1, 12u, INT32_MAX, TEST_NO_HALF, false, false},
};

int main(void)
{
    size_t index;
    RunObservation natural = {0};
    RunObservation mode45 = {0};
    Fixture reload_fixture = {
        "fresh-c620-reload", TEST_SLOT_INDEX, 1, ROUTE_MODE1,
        1u, 1, 12u, 0, TEST_NO_HALF, false, true
    };
    Fixture wrapped_slot_fixture = {
        "wrapped-slot-minus-1", -1, 6, ROUTE_DEFAULT,
        1u, 1, 12u, 0, TEST_NO_HALF, false, false
    };

    printf("=== W34B9-A: wm_8008E190 production certification ===\n");
    test_sra_authority();

    for (index = 0u;
         index < sizeof(s_mode_fixtures) / sizeof(s_mode_fixtures[0]);
         index++) {
        RunObservation current = run_fixture(&s_mode_fixtures[index]);
        if (s_mode_fixtures[index].mode == 1)
            natural = current;
        if (s_mode_fixtures[index].mode == 4)
            mode45 = current;
    }
    for (index = 0u;
         index < sizeof(s_variant_fixtures) / sizeof(s_variant_fixtures[0]);
         index++)
        (void)run_fixture(&s_variant_fixtures[index]);
    for (index = 0u;
         index < sizeof(s_tail_fixtures) / sizeof(s_tail_fixtures[0]);
         index++)
        (void)run_fixture(&s_tail_fixtures[index]);
    (void)run_fixture(&reload_fixture);
    (void)run_fixture(&wrapped_slot_fixture);

    report_mutant("wrong mode dispatch", mode45.terrain_calls == 1 &&
                  natural.terrain_calls == 1);
    report_mutant("mode1 skips terrain", natural.terrain_calls == 1);
    report_mutant("terrain wrong X", natural.seam_ok);
    report_mutant("terrain wrong Z", natural.seam_ok);
    report_mutant("C620 wrong global", natural.trace_ok);
    report_mutant("context record stride not 0x54", mode45.ram_ok);
    report_mutant("context +00/+08/+0C/+10/+14 swaps", natural.ram_ok);
    report_mutant("matrix +0x20 destination", natural.seam_ok);
    report_mutant("matrix +0x74 destination", natural.seam_ok);
    report_mutant("DFF4 omitted", natural.dff4_calls == 1);
    report_mutant("E034 omitted", natural.e034_calls == 1);
    report_mutant("E034 before matrices", natural.trace_ok);
    report_mutant("RotMatrixYXZ call omitted", natural.matrix_calls == 2);
    report_mutant("wrong return state", natural.return_ok);
    report_mutant("scheduler state/object direct write", natural.ram_ok);
    report_mutant("SRL instead of SRA", wm_e190_sra12(0x80000000u) ==
                  0xFFF80000u);

    printf("RESULT %d/%d PASS\n", s_pass_count, s_total_count);
    if (s_failure_count != 0)
        printf("FAILURES %d\n", s_failure_count);
    return s_failure_count == 0 ? 0 : 1;
}
