/*
 * W34B10 production-linked certification for helper 0x800848B4 and world
 * callback 0x800906E0.
 *
 * The oracle is a direct, declarative transcription of the frozen retail
 * listings.  It deliberately does not call the production offset helper or
 * derive expected mode behavior from the production condition.  Both actual
 * production translation units are included below, with only PSX_ADDR
 * replaced by a noinline guest-address translator.
 *
 * Each production memory operation makes exactly one PSX_ADDR translation.
 * The ordered address trace therefore certifies the retail access sequence;
 * read/write role and width are assigned from the independently declared raw
 * instruction sequence, while asymmetric values and the whole-RAM postimage
 * certify the corresponding widths and values.  No production trace hook is
 * used.
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

#ifndef WM_848B4_PRODUCTION_SOURCE
#define WM_848B4_PRODUCTION_SOURCE "../src/world_map_helper_848b4.c"
#endif
#include WM_848B4_PRODUCTION_SOURCE

#ifndef WM_906E0_PRODUCTION_SOURCE
#define WM_906E0_PRODUCTION_SOURCE "../src/world_map_callback_906e0.c"
#endif
#include WM_906E0_PRODUCTION_SOURCE

#define TEST_RAM_MASK       0x001FFFFFu
#define TEST_MAIN_RAM_BYTES 0x00200000u
#define TEST_POOL_PTR       0x8009BE24u
#define TEST_MODE           0x8009BE10u
#define TEST_C620           0x8009C620u
#define TEST_CONTEXT_ROOT   0x800D9540u
#define TEST_CONTEXT_ALT    0x800DA540u
#define TEST_POOL_BASE      0x800D7538u
#define TEST_SLOT_STRIDE    0x80u
#define TEST_RECORD_STRIDE  0x54u
#define TEST_TRACE_CAPACITY 64u

#define CTX_RECORD2 0xA8u
#define CTX_RECORD3 0xFCu
#define CTX_X       0x08u
#define CTX_Y       0x0Cu
#define CTX_Z       0x10u
#define CTX_W       0x14u
#define CTX_LINK    0x50u

#define SLOT_STATE   0x00u
#define SLOT_CONTROL 0x20u
#define SLOT_X2      0x28u
#define SLOT_Y2      0x2Cu
#define SLOT_Z2      0x30u
#define SLOT_W2      0x34u
#define SLOT_X3      0x38u
#define SLOT_Y3      0x3Cu
#define SLOT_Z3      0x40u
#define SLOT_W3      0x44u
#define SLOT_CLEAR50 0x50u
#define SLOT_CONST54 0x54u
#define SLOT_CONST58 0x58u
#define SLOT_CONST5C 0x5Cu

_Static_assert(PSX_RAM_SIZE == 0x00300000u,
               "test certifies complete 3 MiB RAM including guard");
_Static_assert(WM_848B4_CONTEXT_PTR == TEST_C620,
               "retail helper C620 address");
_Static_assert(WM_848B4_RECORD_STRIDE == TEST_RECORD_STRIDE,
               "retail helper record stride");
_Static_assert(WM_848B4_LINK_OFFSET == CTX_LINK,
               "retail helper link offset");
_Static_assert(WM_906E0_POOL_PTR == TEST_POOL_PTR,
               "retail callback pool pointer address");
_Static_assert(WM_906E0_MODE == TEST_MODE,
               "retail callback signed mode address");
_Static_assert(WM_906E0_CONTEXT_PTR == TEST_C620,
               "retail callback C620 address");
_Static_assert(WM_906E0_CONTEXT_RECORD2 == CTX_RECORD2 &&
                   WM_906E0_CONTEXT_RECORD3 == CTX_RECORD3,
               "retail context record bases");
_Static_assert(WM_906E0_SLOT_CONST5C == SLOT_CONST5C &&
                   WM_906E0_SLOT_CONST58 == SLOT_CONST58,
               "retail mode-arm destinations");

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

typedef struct AddressTrace {
    u32 address;
} AddressTrace;

typedef struct CallbackFixture {
    const char* name;
    s32 slot_index;
    u32 pool;
    s32 mode;
    bool repoint_on_fresh_load;
} CallbackFixture;

typedef struct HelperFixture {
    const char* name;
    u32 root;
    s32 source_record;
    s32 destination_record;
} HelperFixture;

static u8 s_expected_ram[PSX_RAM_SIZE];
static u8 s_expected_scratch[sizeof(g_PsxScratchpad)];
static AddressTrace s_actual_trace[TEST_TRACE_CAPACITY];
static AddressTrace s_expected_trace[TEST_TRACE_CAPACITY];
static size_t s_actual_trace_count;
static size_t s_expected_trace_count;
static bool s_trace_overflow;
static bool s_trace_armed;
static bool s_repoint_on_fresh_load;
static size_t s_c620_translation_count;
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

static void trace_append(AddressTrace* trace, size_t* count, u32 address)
{
    if (*count < (size_t)TEST_TRACE_CAPACITY) {
        trace[*count].address = address;
    } else {
        s_trace_overflow = true;
    }
    (*count)++;
}

static void expected_access(u32 address)
{
    trace_append(s_expected_trace, &s_expected_trace_count, address);
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

static void raw_store_u32(u32 address, u32 value)
{
    buffer_store_u32(g_PsxRam, address, value);
}

static u32 raw_load_u32(u32 address)
{
    return buffer_load_u32(g_PsxRam, address);
}

static void* test_psx_addr(uintptr_t address_bits)
{
    u32 address = (u32)address_bits;

    if (s_trace_armed) {
        trace_append(s_actual_trace, &s_actual_trace_count, address);
        if (address == TEST_C620) {
            s_c620_translation_count++;
            /* Helper roots are translations one and two; callback record two
             * is three.  Retail's fresh 0x80090748 load is translation four. */
            if (s_repoint_on_fresh_load &&
                s_c620_translation_count == 4u)
                raw_store_u32(TEST_C620, TEST_CONTEXT_ALT);
        }
    }
    return raw_address(address);
}

static u32 s32_bits(s32 value)
{
    u32 result;
    memcpy(&result, &value, sizeof(result));
    return result;
}

static u32 oracle_record_offset(s32 index)
{
    /* Declarative authority: retail's record stride is exactly 84 bytes and
     * MIPS finite-width multiplication wraps modulo 2^32. */
    return s32_bits(index) * UINT32_C(84);
}

static bool mode_is_armed(s32 mode)
{
    /* Independent CFG partition: signed INT32_MIN..3, 4..7, >=8. */
    return mode >= 4 && mode <= 7;
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

static bool trace_matches(const char* fixture)
{
    size_t index;
    size_t shared = s_actual_trace_count < s_expected_trace_count
                        ? s_actual_trace_count
                        : s_expected_trace_count;

    if (s_trace_overflow ||
        s_actual_trace_count != s_expected_trace_count) {
        printf("TRACE [%s]: actual=%zu expected=%zu overflow=%d\n",
               fixture, s_actual_trace_count, s_expected_trace_count,
               s_trace_overflow ? 1 : 0);
        return false;
    }
    for (index = 0u; index < shared; index++) {
        if (s_actual_trace[index].address !=
            s_expected_trace[index].address) {
            printf("TRACE [%s] #%zu: actual=%08x expected=%08x\n",
                   fixture, index,
                   (unsigned int)s_actual_trace[index].address,
                   (unsigned int)s_expected_trace[index].address);
            return false;
        }
    }
    return true;
}

static void seed_ram(u32 salt)
{
    size_t index;

    for (index = 0u; index < sizeof(g_PsxRam); index++) {
        u32 low = (u32)(index & 0xFFFFFFFFu);
        g_PsxRam[index] =
            (u8)((low * 37u + (low >> 8) + salt * 29u + 0x5Bu) & 0xFFu);
    }
    for (index = 0u; index < sizeof(g_PsxScratchpad); index++) {
        u32 low = (u32)index;
        g_PsxScratchpad[index] =
            (u8)((low * 53u + salt * 11u + 0xA7u) & 0xFFu);
    }
}

static void reset_trace(void)
{
    s_actual_trace_count = 0u;
    s_expected_trace_count = 0u;
    s_trace_overflow = false;
    s_c620_translation_count = 0u;
}

static u32 context_value(u32 root_tag, u32 offset, u32 salt)
{
    return root_tag ^ (offset * 0x01010101u) ^ (salt * 0x10203041u);
}

static void seed_context(u32 root, u32 root_tag, u32 salt)
{
    static const u32 offsets[] = {
        CTX_RECORD2 + CTX_X,
        CTX_RECORD2 + CTX_Y,
        CTX_RECORD2 + CTX_Z,
        CTX_RECORD2 + CTX_W,
        CTX_RECORD3 + CTX_X,
        CTX_RECORD3 + CTX_Y,
        CTX_RECORD3 + CTX_Z,
        CTX_RECORD3 + CTX_W
    };
    size_t index;

    for (index = 0u; index < sizeof(offsets) / sizeof(offsets[0]); index++)
        raw_store_u32(root + offsets[index],
                      context_value(root_tag, offsets[index], salt));
}

static void oracle_helper(u8* image, u32 root,
                          s32 source_record, s32 destination_record)
{
    u32 source = root + oracle_record_offset(source_record);
    u32 destination = root + oracle_record_offset(destination_record) +
                      CTX_LINK;

    expected_access(TEST_C620);      /* LW at 0x800848DC */
    expected_access(destination);    /* SW at 0x800848F0 */
    buffer_store_u32(image, destination, source);
}

static void oracle_callback(const CallbackFixture* fixture, u32 slot)
{
    u32 first_root = TEST_CONTEXT_ROOT;
    u32 second_root = fixture->repoint_on_fresh_load
                          ? TEST_CONTEXT_ALT
                          : TEST_CONTEXT_ROOT;
    u32 x;
    u32 y;
    u32 z;
    u32 w;

    /* Two unconditional JALs and their complete helper effects. */
    oracle_helper(s_expected_ram, first_root, 0, 2);
    oracle_helper(s_expected_ram, first_root, 0, 3);

    expected_access(TEST_POOL_PTR); /* 0x80090710 */
    expected_access(TEST_C620);     /* 0x80090718 */
    expected_access(slot + SLOT_CONTROL);

    /* Retail loads all record-two words before its first record-two store. */
    expected_access(first_root + CTX_RECORD2 + CTX_X);
    expected_access(first_root + CTX_RECORD2 + CTX_Y);
    expected_access(first_root + CTX_RECORD2 + CTX_Z);
    expected_access(first_root + CTX_RECORD2 + CTX_W);
    x = buffer_load_u32(s_expected_ram,
                        first_root + CTX_RECORD2 + CTX_X);
    y = buffer_load_u32(s_expected_ram,
                        first_root + CTX_RECORD2 + CTX_Y);
    z = buffer_load_u32(s_expected_ram,
                        first_root + CTX_RECORD2 + CTX_Z);
    w = buffer_load_u32(s_expected_ram,
                        first_root + CTX_RECORD2 + CTX_W);
    expected_access(slot + SLOT_X2);
    expected_access(slot + SLOT_Y2);
    expected_access(slot + SLOT_Z2);
    expected_access(slot + SLOT_W2);
    buffer_store_u16(s_expected_ram, slot + SLOT_CONTROL, 0u);
    buffer_store_u32(s_expected_ram, slot + SLOT_X2, x);
    buffer_store_u32(s_expected_ram, slot + SLOT_Y2, y);
    buffer_store_u32(s_expected_ram, slot + SLOT_Z2, z);
    buffer_store_u32(s_expected_ram, slot + SLOT_W2, w);

    /* The fourth C620 translation is the fresh raw LW at 0x80090748. */
    expected_access(TEST_C620);
    if (fixture->repoint_on_fresh_load)
        buffer_store_u32(s_expected_ram, TEST_C620, TEST_CONTEXT_ALT);
    expected_access(second_root + CTX_RECORD3 + CTX_X);
    expected_access(second_root + CTX_RECORD3 + CTX_Y);
    expected_access(second_root + CTX_RECORD3 + CTX_Z);
    expected_access(second_root + CTX_RECORD3 + CTX_W);
    x = buffer_load_u32(s_expected_ram,
                        second_root + CTX_RECORD3 + CTX_X);
    y = buffer_load_u32(s_expected_ram,
                        second_root + CTX_RECORD3 + CTX_Y);
    z = buffer_load_u32(s_expected_ram,
                        second_root + CTX_RECORD3 + CTX_Z);
    w = buffer_load_u32(s_expected_ram,
                        second_root + CTX_RECORD3 + CTX_W);
    expected_access(slot + SLOT_X3);
    expected_access(slot + SLOT_Y3);
    expected_access(slot + SLOT_Z3);
    expected_access(slot + SLOT_W3);
    buffer_store_u32(s_expected_ram, slot + SLOT_X3, x);
    buffer_store_u32(s_expected_ram, slot + SLOT_Y3, y);
    buffer_store_u32(s_expected_ram, slot + SLOT_Z3, z);
    buffer_store_u32(s_expected_ram, slot + SLOT_W3, w);

    expected_access(TEST_MODE);
    expected_access(slot + SLOT_CLEAR50);
    expected_access(slot + SLOT_CONST54);
    buffer_store_u32(s_expected_ram, slot + SLOT_CLEAR50, 0u);
    buffer_store_u32(s_expected_ram, slot + SLOT_CONST54, 0x40u);

    if (mode_is_armed(fixture->mode)) {
        expected_access(slot + SLOT_CONTROL);
        expected_access(slot + SLOT_CONST5C); /* PC 0x800907A4 */
        expected_access(slot + SLOT_CONST58); /* PC 0x800907A8 */
        buffer_store_u16(s_expected_ram, slot + SLOT_CONTROL, 3u);
        buffer_store_u32(s_expected_ram, slot + SLOT_CONST5C, 0x80u);
        buffer_store_u32(s_expected_ram, slot + SLOT_CONST58, 0x80u);
    }
}

static void run_helper_fixture(const HelperFixture* fixture, u32 salt)
{
    u32 source;
    u32 destination;
    bool trace_ok;

    seed_ram(salt);
    raw_store_u32(TEST_C620, fixture->root);
    memcpy(s_expected_ram, g_PsxRam, sizeof(g_PsxRam));
    memcpy(s_expected_scratch, g_PsxScratchpad, sizeof(g_PsxScratchpad));
    reset_trace();

    source = fixture->root + oracle_record_offset(fixture->source_record);
    destination = fixture->root +
                  oracle_record_offset(fixture->destination_record) +
                  CTX_LINK;
    oracle_helper(s_expected_ram, fixture->root, fixture->source_record,
                  fixture->destination_record);

    s_repoint_on_fresh_load = false;
    s_trace_armed = true;
    wm_800848B4(fixture->source_record, fixture->destination_record);
    s_trace_armed = false;

    trace_ok = trace_matches(fixture->name);
    check_case(fixture->name, "helper exact two-access order", trace_ok);
    check_case(fixture->name, "helper exact access count",
               s_actual_trace_count == 2u);
    check_case(fixture->name, "helper wrapping source publication",
               raw_load_u32(destination) == source);
    check_case(fixture->name, "helper whole 3 MiB RAM footprint",
               memcmp(g_PsxRam, s_expected_ram, sizeof(g_PsxRam)) == 0);
    check_case(fixture->name, "helper 1 MiB guard canary",
               memcmp(g_PsxRam + TEST_MAIN_RAM_BYTES,
                      s_expected_ram + TEST_MAIN_RAM_BYTES,
                      sizeof(g_PsxRam) - TEST_MAIN_RAM_BYTES) == 0);
    check_case(fixture->name, "helper scratchpad untouched",
               memcmp(g_PsxScratchpad, s_expected_scratch,
                      sizeof(g_PsxScratchpad)) == 0);
    check_case(fixture->name, "helper reads C620 exactly once",
               s_c620_translation_count == 1u);
}

static bool common_slot_values_match(u32 slot, u32 second_root)
{
    return raw_load_u32(slot + SLOT_X2) ==
               raw_load_u32(TEST_CONTEXT_ROOT + CTX_RECORD2 + CTX_X) &&
           raw_load_u32(slot + SLOT_Y2) ==
               raw_load_u32(TEST_CONTEXT_ROOT + CTX_RECORD2 + CTX_Y) &&
           raw_load_u32(slot + SLOT_Z2) ==
               raw_load_u32(TEST_CONTEXT_ROOT + CTX_RECORD2 + CTX_Z) &&
           raw_load_u32(slot + SLOT_W2) ==
               raw_load_u32(TEST_CONTEXT_ROOT + CTX_RECORD2 + CTX_W) &&
           raw_load_u32(slot + SLOT_X3) ==
               raw_load_u32(second_root + CTX_RECORD3 + CTX_X) &&
           raw_load_u32(slot + SLOT_Y3) ==
               raw_load_u32(second_root + CTX_RECORD3 + CTX_Y) &&
           raw_load_u32(slot + SLOT_Z3) ==
               raw_load_u32(second_root + CTX_RECORD3 + CTX_Z) &&
           raw_load_u32(slot + SLOT_W3) ==
               raw_load_u32(second_root + CTX_RECORD3 + CTX_W);
}

static void run_callback_fixture(const CallbackFixture* fixture, u32 salt)
{
    u32 slot = fixture->pool + (s32_bits(fixture->slot_index) << 7);
    u32 second_root = fixture->repoint_on_fresh_load
                          ? TEST_CONTEXT_ALT
                          : TEST_CONTEXT_ROOT;
    u32 original_state;
    u32 original_58;
    u32 original_5c;
    u32 original_before;
    u32 original_after;
    s32 result;
    bool trace_ok;

    seed_ram(salt);
    raw_store_u32(TEST_POOL_PTR, fixture->pool);
    raw_store_u32(TEST_MODE, s32_bits(fixture->mode));
    raw_store_u32(TEST_C620, TEST_CONTEXT_ROOT);
    seed_context(TEST_CONTEXT_ROOT, 0x13579BDFu, salt);
    seed_context(TEST_CONTEXT_ALT, 0xECA86420u, salt + 1u);

    original_state = raw_load_u32(slot + SLOT_STATE);
    original_58 = raw_load_u32(slot + SLOT_CONST58);
    original_5c = raw_load_u32(slot + SLOT_CONST5C);
    original_before = raw_load_u32(slot - 4u);
    original_after = raw_load_u32(slot + TEST_SLOT_STRIDE);

    memcpy(s_expected_ram, g_PsxRam, sizeof(g_PsxRam));
    memcpy(s_expected_scratch, g_PsxScratchpad, sizeof(g_PsxScratchpad));
    reset_trace();
    oracle_callback(fixture, slot);

    s_repoint_on_fresh_load = fixture->repoint_on_fresh_load;
    s_trace_armed = true;
    result = wm_800906E0(fixture->slot_index);
    s_trace_armed = false;

    trace_ok = trace_matches(fixture->name);
    check_case(fixture->name, "return is raw scheduler state 1", result == 1);
    check_case(fixture->name, "exact helper/callback access order", trace_ok);
    check_case(fixture->name, "exact path access count",
               s_actual_trace_count ==
                   (mode_is_armed(fixture->mode) ? 30u : 27u));
    check_case(fixture->name, "two helpers precede every slot access",
               s_actual_trace_count >= 5u &&
                   s_actual_trace[0].address == TEST_C620 &&
                   s_actual_trace[1].address == TEST_CONTEXT_ROOT + 0xF8u &&
                   s_actual_trace[2].address == TEST_C620 &&
                   s_actual_trace[3].address == TEST_CONTEXT_ROOT + 0x14Cu &&
                   s_actual_trace[4].address == TEST_POOL_PTR);
    check_case(fixture->name, "C620 loaded twice by helpers and twice by body",
               s_c620_translation_count == 4u);
    check_case(fixture->name, "helper record-two link publication",
               raw_load_u32(TEST_CONTEXT_ROOT + 0xF8u) ==
                   TEST_CONTEXT_ROOT);
    check_case(fixture->name, "helper record-three link publication",
               raw_load_u32(TEST_CONTEXT_ROOT + 0x14Cu) ==
                   TEST_CONTEXT_ROOT);
    check_case(fixture->name, "record-two and fresh record-three copies",
               common_slot_values_match(slot, second_root));
    check_case(fixture->name, "slot +50/+54 common constants",
               raw_load_u32(slot + SLOT_CLEAR50) == 0u &&
                   raw_load_u32(slot + SLOT_CONST54) == 0x40u);
    check_case(fixture->name, "signed mode partition and +5C/+58 values",
               mode_is_armed(fixture->mode)
                   ? (buffer_load_u16(g_PsxRam, slot + SLOT_CONTROL) == 3u &&
                      raw_load_u32(slot + SLOT_CONST5C) == 0x80u &&
                      raw_load_u32(slot + SLOT_CONST58) == 0x80u)
                   : (buffer_load_u16(g_PsxRam, slot + SLOT_CONTROL) == 0u &&
                      raw_load_u32(slot + SLOT_CONST5C) == original_5c &&
                      raw_load_u32(slot + SLOT_CONST58) == original_58));
    check_case(fixture->name, "scheduler slot state +00 untouched",
               raw_load_u32(slot + SLOT_STATE) == original_state);
    check_case(fixture->name, "adjacent slot/subrecord canaries untouched",
               raw_load_u32(slot - 4u) == original_before &&
                   raw_load_u32(slot + TEST_SLOT_STRIDE) == original_after);
    check_case(fixture->name, "fresh C620 root is observed, not cached",
               raw_load_u32(TEST_C620) ==
                   (fixture->repoint_on_fresh_load
                        ? TEST_CONTEXT_ALT
                        : TEST_CONTEXT_ROOT));
    check_case(fixture->name, "whole 3 MiB RAM exact footprint",
               memcmp(g_PsxRam, s_expected_ram, sizeof(g_PsxRam)) == 0);
    check_case(fixture->name, "1 MiB guard canary",
               memcmp(g_PsxRam + TEST_MAIN_RAM_BYTES,
                      s_expected_ram + TEST_MAIN_RAM_BYTES,
                      sizeof(g_PsxRam) - TEST_MAIN_RAM_BYTES) == 0);
    check_case(fixture->name, "scratchpad untouched",
               memcmp(g_PsxScratchpad, s_expected_scratch,
                      sizeof(g_PsxScratchpad)) == 0);
}

static void run_declared_mutation_sentinels(void)
{
    /* These are oracle self-checks: each predicate describes a high-risk
     * wrong model without reusing production expressions. Copied-source
     * M1-M9 builds exercise the real variants separately. */
    check_case("oracle", "signed-mode mutant differs at INT32_MIN",
               mode_is_armed(INT32_MIN) == false);
    check_case("oracle", "mode-3 lower-bound mutant differs",
               mode_is_armed(3) == false);
    check_case("oracle", "mode-4 arm declared",
               mode_is_armed(4));
    check_case("oracle", "mode-7 arm declared",
               mode_is_armed(7));
    check_case("oracle", "mode-8 upper-bound mutant differs",
               mode_is_armed(8) == false);
    check_case("oracle", "record stride is declarative 84",
               oracle_record_offset(1) == 84u);
    check_case("oracle", "negative helper index wraps",
               oracle_record_offset(-1) == 0xFFFFFFACu);
    check_case("oracle", "INT32_MIN helper index wraps to zero",
               oracle_record_offset(INT32_MIN) == 0u);
    check_case("oracle", "slot shift wraps INT32_MIN to zero",
               (s32_bits(INT32_MIN) << 7) == 0u);
}

int main(void)
{
    static const HelperFixture helper_fixtures[] = {
        {"helper-natural-0-to-2", TEST_CONTEXT_ROOT, 0, 2},
        {"helper-negative-indices", 0x80012000u, -1, -1},
        {"helper-int-min-max", 0xFFFFFFF0u, INT32_MIN, INT32_MAX},
        {"helper-int-max-min", 0x7FFFF000u, INT32_MAX, INT32_MIN},
        {"helper-large-wrapping", 0x801FF000u, 0x40000001, 0x20000003}
    };
    static const CallbackFixture callback_fixtures[] = {
        {"mode-int-min", 8, TEST_POOL_BASE, INT32_MIN, false},
        {"mode-minus-one", 8, TEST_POOL_BASE, -1, false},
        {"mode-zero", 8, TEST_POOL_BASE, 0, false},
        {"mode-one-natural-slot8", 8, TEST_POOL_BASE, 1, false},
        {"mode-two", 8, TEST_POOL_BASE, 2, false},
        {"mode-three", 8, TEST_POOL_BASE, 3, false},
        {"mode-four", 8, TEST_POOL_BASE, 4, false},
        {"mode-five-fresh-c620", 8, TEST_POOL_BASE, 5, true},
        {"mode-six", 8, TEST_POOL_BASE, 6, false},
        {"mode-seven", 8, TEST_POOL_BASE, 7, false},
        {"mode-eight", 8, TEST_POOL_BASE, 8, false},
        {"mode-nine", 8, TEST_POOL_BASE, 9, false},
        {"mode-int-max", 8, TEST_POOL_BASE, INT32_MAX, false},
        {"slot-index-minus-one-wrap", -1, 0x800D8000u, 4, false},
        {"slot-index-int-min-shift-wrap", INT32_MIN,
         0x800D8100u, 7, false},
        {"slot-addition-wrap", 1, 0xFFFFFFF0u, 8, false}
    };
    size_t index;

    for (index = 0u;
         index < sizeof(helper_fixtures) / sizeof(helper_fixtures[0]);
         index++)
        run_helper_fixture(&helper_fixtures[index], (u32)index + 1u);

    for (index = 0u;
         index < sizeof(callback_fixtures) / sizeof(callback_fixtures[0]);
         index++)
        run_callback_fixture(&callback_fixtures[index],
                             (u32)index + 0x31u);

    run_declared_mutation_sentinels();

    printf("=== Results: %d/%d PASS ===\n", s_pass_count, s_total_count);
    return s_failure_count == 0 ? 0 : 1;
}
