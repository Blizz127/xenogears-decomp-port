/*
 * W34B14-B production-linked retail certificate for callback 0x80092BE4.
 *
 * Expected PCs, call targets, arguments, typed global accesses, order, and
 * return semantics below are independently transcribed from the verified
 * retail slice. The actual production callback source is included unchanged.
 * Accepted helper bodies are replaced only by observation seams: the first
 * seam injects an asymmetric post-constructor D4A8 value, and neither seam
 * reimplements helper behavior.
 */
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"

enum {
    EVENT_LHU = 1,
    EVENT_SB = 2,
    EVENT_SH = 3,
    EVENT_CALL_32F54 = 4,
    EVENT_RETURN_32F54 = 5,
    EVENT_CALL_34614 = 6,
    EVENT_CALLBACK_RETURN = 7,
    EVENT_UNKNOWN = 255
};

typedef struct Event {
    u32 pc;
    u32 kind;
    u32 address;
    u32 width;
    u32 value;
    s32 args[7];
} Event;

typedef struct Fixture {
    const char* name;
    s32 slot_index;
    u16 pre_constructor_flags;
    u16 post_constructor_flags;
    u16 initial_selection;
    u8 initial_state_byte;
} Fixture;

#define EVENT_CAPACITY 16u
#define RAM_MASK 0x001FFFFFu
#define MAIN_RAM_BYTES 0x00200000u
#define SLOT_BYTES 0x80u

/* Independent retail constants. */
#define RETAIL_TEXT_OBJECT 0x8009D498u
#define RETAIL_TEXT_FLAGS  0x8009D4A8u
#define RETAIL_TEXT_STATE  0x8009D500u
#define RETAIL_SELECTION   0x8009BD24u
#define UNRELATED_POOL     0x8009BE24u
#define TEST_POOL          0x800D7000u

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

static Event s_actual[EVENT_CAPACITY];
static Event s_expected[EVENT_CAPACITY];
static size_t s_actual_count;
static size_t s_expected_count;
static bool s_actual_overflow;
static u16 s_constructor_post_flags;
static u16 s_constructor_entry_selection;
static u16 s_constructor_entry_flags;
static u8 s_constructor_entry_state;
static u16 s_reset_entry_selection;
static u16 s_reset_entry_flags;
static u8 s_reset_entry_state;
static int s_constructor_calls;
static int s_reset_calls;

static u8 s_before_ram[PSX_RAM_SIZE];
static u8 s_expected_ram[PSX_RAM_SIZE];
static u8 s_before_scratch[sizeof(g_PsxScratchpad)];
static u8 s_before_slot[SLOT_BYTES];
static int s_pass_count;
static int s_total_count;
static int s_failure_count;

static size_t ram_index(u32 address)
{
    return (size_t)(address & RAM_MASK);
}

static u8 memory_load_u8(const u8* memory, u32 address)
{
    return memory[ram_index(address)];
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

static void memory_store_u8(u8* memory, u32 address, u8 value)
{
    memory[ram_index(address)] = value;
}

static void memory_store_u16(u8* memory, u32 address, u16 value)
{
    memcpy(memory + ram_index(address), &value, sizeof(value));
}

static void memory_store_u32(u8* memory, u32 address, u32 value)
{
    memcpy(memory + ram_index(address), &value, sizeof(value));
}

static u8 raw_load_u8(u32 address)
{
    return memory_load_u8(g_PsxRam, address);
}

static u16 raw_load_u16(u32 address)
{
    return memory_load_u16(g_PsxRam, address);
}

static u32 raw_load_u32(u32 address)
{
    return memory_load_u32(g_PsxRam, address);
}

static void raw_store_u8(u32 address, u8 value)
{
    memory_store_u8(g_PsxRam, address, value);
}

static void raw_store_u16(u32 address, u16 value)
{
    memory_store_u16(g_PsxRam, address, value);
}

static void raw_store_u32(u32 address, u32 value)
{
    memory_store_u32(g_PsxRam, address, value);
}

static void clear_args(s32 args[7])
{
    size_t index;
    for (index = 0u; index < 7u; index++)
        args[index] = 0;
}

static void append_event(Event* events, size_t* count,
                         u32 pc, u32 kind, u32 address,
                         u32 width, u32 value)
{
    Event* event;

    if (*count >= (size_t)EVENT_CAPACITY) {
        if (events == s_actual)
            s_actual_overflow = true;
        (*count)++;
        return;
    }
    event = &events[*count];
    event->pc = pc;
    event->kind = kind;
    event->address = address;
    event->width = width;
    event->value = value;
    clear_args(event->args);
    (*count)++;
}

static u32 host_pointer_as_guest(const void* pointer)
{
    uintptr_t base = (uintptr_t)(const void*)g_PsxRam;
    uintptr_t current = (uintptr_t)pointer;
    uintptr_t delta;

    if (current < base)
        return 0xFFFFFFFFu;
    delta = current - base;
    if (delta >= (uintptr_t)MAIN_RAM_BYTES)
        return 0xFFFFFFFFu;
    return 0x80000000u | (u32)delta;
}

static void append_constructor_call(void* object,
                                    s32 tpage_x, s32 tpage_y,
                                    s32 x, s32 y, s32 width,
                                    s32 host_dead_mode, s32 height)
{
    Event* event;

    append_event(s_actual, &s_actual_count, 0x80092C28u,
                 EVENT_CALL_32F54, 0x80032F54u, 0u,
                 host_pointer_as_guest(object));
    if (s_actual_count <= (size_t)EVENT_CAPACITY) {
        event = &s_actual[s_actual_count - 1u];
        event->args[0] = tpage_x;
        event->args[1] = tpage_y;
        event->args[2] = x;
        event->args[3] = y;
        event->args[4] = width;
        event->args[5] = height;
        event->args[6] = host_dead_mode;
    }
}

/* Accepted helper observation seam. This records the call boundary and models
 * only the constructor's externally injected D4A8 poststate needed to prove
 * the callback's mandatory fresh reload. */
void func_80032F54(void* object, s32 tpage_x, s32 tpage_y,
                   s32 x, s32 y, s32 width, s32 host_dead_mode,
                   s32 height)
{
    s_constructor_calls++;
    s_constructor_entry_selection = raw_load_u16(RETAIL_SELECTION);
    s_constructor_entry_flags = raw_load_u16(RETAIL_TEXT_FLAGS);
    s_constructor_entry_state = raw_load_u8(RETAIL_TEXT_STATE);
    append_constructor_call(object, tpage_x, tpage_y, x, y, width,
                            host_dead_mode, height);

    /* Untraced helper-side publication: the callback must freshly observe it. */
    raw_store_u16(RETAIL_TEXT_FLAGS, s_constructor_post_flags);
    append_event(s_actual, &s_actual_count, 0x80092C30u,
                 EVENT_RETURN_32F54, 0x80032F54u, 0u,
                 (u32)s_constructor_post_flags);
}

void func_80034614(void* object)
{
    s_reset_calls++;
    s_reset_entry_selection = raw_load_u16(RETAIL_SELECTION);
    s_reset_entry_flags = raw_load_u16(RETAIL_TEXT_FLAGS);
    s_reset_entry_state = raw_load_u8(RETAIL_TEXT_STATE);
    append_event(s_actual, &s_actual_count, 0x80092C50u,
                 EVENT_CALL_34614, 0x80034614u, 0u,
                 host_pointer_as_guest(object));
}

void wm_92be4_test_trace(u32 pc, u32 kind, u32 address,
                         u32 width, u32 value)
{
    u32 event_kind = EVENT_UNKNOWN;

    if (kind == 1u)
        event_kind = EVENT_LHU;
    else if (kind == 2u)
        event_kind = EVENT_SB;
    else if (kind == 3u)
        event_kind = EVENT_SH;
    append_event(s_actual, &s_actual_count, pc, event_kind,
                 address, width, value);
}

#define WM_92BE4_TEST_TRACE 1
#ifndef WM_92BE4_PRODUCTION_SOURCE
#define WM_92BE4_PRODUCTION_SOURCE "../src/world_map_callback_92be4.c"
#endif
#include WM_92BE4_PRODUCTION_SOURCE

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
    case EVENT_LHU: return "LHU";
    case EVENT_SB: return "SB";
    case EVENT_SH: return "SH";
    case EVENT_CALL_32F54: return "CALL_32F54";
    case EVENT_RETURN_32F54: return "RETURN_32F54";
    case EVENT_CALL_34614: return "CALL_34614";
    case EVENT_CALLBACK_RETURN: return "CALLBACK_RETURN";
    default: return "UNKNOWN";
    }
}

static void expected_constructor_call(void)
{
    Event* event;

    append_event(s_expected, &s_expected_count, 0x80092C28u,
                 EVENT_CALL_32F54, 0x80032F54u, 0u,
                 RETAIL_TEXT_OBJECT);
    event = &s_expected[s_expected_count - 1u];
    event->args[0] = 960;
    event->args[1] = 384;
    event->args[2] = 160;
    event->args[3] = 120;
    event->args[4] = 32;
    event->args[5] = 1;
    event->args[6] = 0;
}

static void build_expected(const Fixture* fixture, s32 callback_return)
{
    u16 published = (u16)(fixture->post_constructor_flags | 2u);

    memcpy(s_expected_ram, s_before_ram, sizeof(s_expected_ram));
    memory_store_u16(s_expected_ram, RETAIL_SELECTION, 0xFFFFu);
    memory_store_u16(s_expected_ram, RETAIL_TEXT_FLAGS,
                     fixture->post_constructor_flags);
    memory_store_u8(s_expected_ram, RETAIL_TEXT_STATE, 8u);
    memory_store_u16(s_expected_ram, RETAIL_TEXT_FLAGS, published);

    memset(s_expected, 0, sizeof(s_expected));
    s_expected_count = 0u;
    append_event(s_expected, &s_expected_count, 0x80092C00u,
                 EVENT_SH, RETAIL_SELECTION, 2u, 0xFFFFu);
    expected_constructor_call();
    append_event(s_expected, &s_expected_count, 0x80092C30u,
                 EVENT_RETURN_32F54, 0x80032F54u, 0u,
                 (u32)fixture->post_constructor_flags);
    append_event(s_expected, &s_expected_count, 0x80092C34u,
                 EVENT_LHU, RETAIL_TEXT_FLAGS, 2u,
                 (u32)fixture->post_constructor_flags);
    append_event(s_expected, &s_expected_count, 0x80092C40u,
                 EVENT_SB, RETAIL_TEXT_STATE, 1u, 8u);
    append_event(s_expected, &s_expected_count, 0x80092C4Cu,
                 EVENT_SH, RETAIL_TEXT_FLAGS, 2u, (u32)published);
    append_event(s_expected, &s_expected_count, 0x80092C50u,
                 EVENT_CALL_34614, 0x80034614u, 0u,
                 RETAIL_TEXT_OBJECT);
    append_event(s_expected, &s_expected_count, 0x80092C68u,
                 EVENT_CALLBACK_RETURN, 0u, 0u,
                 (u32)callback_return);
}

static bool trace_matches(const char* fixture)
{
    size_t event_index;

    if (s_actual_overflow || s_actual_count != s_expected_count) {
        printf("TRACE [%s]: actual=%zu expected=%zu overflow=%d\n",
               fixture, s_actual_count, s_expected_count,
               s_actual_overflow ? 1 : 0);
        return false;
    }
    for (event_index = 0u; event_index < s_expected_count; event_index++) {
        const Event* actual = &s_actual[event_index];
        const Event* expected = &s_expected[event_index];
        if (memcmp(actual, expected, sizeof(*actual)) != 0) {
            printf("TRACE [%s] #%zu: actual %08x %s %08x/%u=%08x; "
                   "expected %08x %s %08x/%u=%08x\n",
                   fixture, event_index,
                   (unsigned int)actual->pc, kind_name(actual->kind),
                   (unsigned int)actual->address,
                   (unsigned int)actual->width,
                   (unsigned int)actual->value,
                   (unsigned int)expected->pc, kind_name(expected->kind),
                   (unsigned int)expected->address,
                   (unsigned int)expected->width,
                   (unsigned int)expected->value);
            return false;
        }
    }
    return true;
}

static size_t exact_event_count(u32 pc, u32 kind, u32 address, u32 width)
{
    size_t count = 0u;
    size_t index;

    for (index = 0u; index < s_actual_count &&
                     index < (size_t)EVENT_CAPACITY; index++) {
        const Event* event = &s_actual[index];
        if (event->pc == pc && event->kind == kind &&
            event->address == address && event->width == width)
            count++;
    }
    return count;
}

static u32 exact_event_value(u32 pc, u32 kind, u32 address)
{
    size_t index;

    for (index = 0u; index < s_actual_count &&
                     index < (size_t)EVENT_CAPACITY; index++) {
        const Event* event = &s_actual[index];
        if (event->pc == pc && event->kind == kind &&
            event->address == address)
            return event->value;
    }
    return 0xDEADDEADu;
}

static bool direct_access_addresses_are_exact(void)
{
    size_t index;

    for (index = 0u; index < s_actual_count &&
                     index < (size_t)EVENT_CAPACITY; index++) {
        const Event* event = &s_actual[index];
        if (event->kind == EVENT_LHU) {
            if (event->address != RETAIL_TEXT_FLAGS || event->width != 2u)
                return false;
        } else if (event->kind == EVENT_SB) {
            if (event->address != RETAIL_TEXT_STATE || event->width != 1u)
                return false;
        } else if (event->kind == EVENT_SH) {
            if ((event->address != RETAIL_SELECTION &&
                 event->address != RETAIL_TEXT_FLAGS) ||
                event->width != 2u)
                return false;
        } else if (event->kind == EVENT_UNKNOWN) {
            return false;
        }
    }
    return true;
}

static bool only_expected_direct_bytes_differ(void)
{
    size_t index;

    for (index = 0u; index < sizeof(g_PsxRam); index++) {
        bool allowed = index == ram_index(RETAIL_SELECTION) ||
                       index == ram_index(RETAIL_SELECTION) + 1u ||
                       index == ram_index(RETAIL_TEXT_FLAGS) ||
                       index == ram_index(RETAIL_TEXT_FLAGS) + 1u ||
                       index == ram_index(RETAIL_TEXT_STATE);
        if (!allowed && g_PsxRam[index] != s_before_ram[index])
            return false;
    }
    return true;
}

static void seed_memory(u32 salt)
{
    size_t index;

    for (index = 0u; index < sizeof(g_PsxRam); index++) {
        u32 low = (u32)(index & 0xFFFFFFFFu);
        g_PsxRam[index] =
            (u8)((low * 37u + (low >> 7) + salt * 61u + 0x53u) & 0xFFu);
    }
    for (index = 0u; index < sizeof(g_PsxScratchpad); index++) {
        u32 low = (u32)index;
        g_PsxScratchpad[index] =
            (u8)((low * 31u + salt * 19u + 0xA7u) & 0xFFu);
    }
}

static void reset_observation(const Fixture* fixture)
{
    memset(s_actual, 0, sizeof(s_actual));
    s_actual_count = 0u;
    s_actual_overflow = false;
    s_constructor_post_flags = fixture->post_constructor_flags;
    s_constructor_entry_selection = 0u;
    s_constructor_entry_flags = 0u;
    s_constructor_entry_state = 0u;
    s_reset_entry_selection = 0u;
    s_reset_entry_flags = 0u;
    s_reset_entry_state = 0u;
    s_constructor_calls = 0;
    s_reset_calls = 0;
}

static void run_fixture(const Fixture* fixture, u32 salt)
{
    u32 slot = TEST_POOL + ((u32)fixture->slot_index << 7);
    u16 expected_flags = (u16)(fixture->post_constructor_flags | 2u);
    s32 result;

    seed_memory(salt);
    raw_store_u32(UNRELATED_POOL, TEST_POOL);
    raw_store_u16(RETAIL_SELECTION, fixture->initial_selection);
    raw_store_u16(RETAIL_TEXT_FLAGS, fixture->pre_constructor_flags);
    raw_store_u8(RETAIL_TEXT_STATE, fixture->initial_state_byte);
    memcpy(s_before_ram, g_PsxRam, sizeof(g_PsxRam));
    memcpy(s_before_scratch, g_PsxScratchpad, sizeof(g_PsxScratchpad));
    memcpy(s_before_slot, g_PsxRam + ram_index(slot), sizeof(s_before_slot));
    reset_observation(fixture);

    result = wm_80092BE4(fixture->slot_index);
    append_event(s_actual, &s_actual_count, 0x80092C68u,
                 EVENT_CALLBACK_RETURN, 0u, 0u, (u32)result);
    build_expected(fixture, 1);

    check_case(fixture->name, "return is scheduler state 1", result == 1);
    check_case(fixture->name, "exact retail call/access/return event trace",
               trace_matches(fixture->name));
    check_case(fixture->name, "event trace stays within capacity",
               !s_actual_overflow);
    check_case(fixture->name, "both accepted helpers called exactly once",
               s_constructor_calls == 1 && s_reset_calls == 1);
    check_case(fixture->name, "constructor sees prior BD24 SH publication",
               s_constructor_entry_selection == 0xFFFFu);
    check_case(fixture->name, "constructor sees exact D4A8 prestate",
               s_constructor_entry_flags == fixture->pre_constructor_flags);
    check_case(fixture->name, "constructor sees D500 prestate",
               s_constructor_entry_state == fixture->initial_state_byte);
    check_case(fixture->name, "one mandatory fresh post-constructor D4A8 LHU",
               exact_event_count(0x80092C34u, EVENT_LHU,
                                 RETAIL_TEXT_FLAGS, 2u) == 1u);
    check_case(fixture->name, "fresh D4A8 LHU sees helper poststate",
               exact_event_value(0x80092C34u, EVENT_LHU,
                                 RETAIL_TEXT_FLAGS) ==
                   (u32)fixture->post_constructor_flags);
    check_case(fixture->name, "D500 uses exact byte publication",
               exact_event_count(0x80092C40u, EVENT_SB,
                                 RETAIL_TEXT_STATE, 1u) == 1u &&
                   raw_load_u8(RETAIL_TEXT_STATE) == 8u);
    check_case(fixture->name, "D4A8 publishes unsigned fresh OR 2",
               exact_event_count(0x80092C4Cu, EVENT_SH,
                                 RETAIL_TEXT_FLAGS, 2u) == 1u &&
                   raw_load_u16(RETAIL_TEXT_FLAGS) == expected_flags);
    check_case(fixture->name, "second helper sees BD24 sentinel",
               s_reset_entry_selection == 0xFFFFu);
    check_case(fixture->name, "second helper sees D500 after byte store",
               s_reset_entry_state == 8u);
    check_case(fixture->name, "second helper sees published D4A8",
               s_reset_entry_flags == expected_flags);
    check_case(fixture->name, "direct access addresses and widths exact",
               direct_access_addresses_are_exact());
    check_case(fixture->name, "whole guest RAM matches independent oracle",
               memcmp(g_PsxRam, s_expected_ram, sizeof(g_PsxRam)) == 0);
    check_case(fixture->name, "direct callback write footprint is exact",
               only_expected_direct_bytes_differ());
    check_case(fixture->name, "pool global is not read or modified",
               raw_load_u32(UNRELATED_POOL) == TEST_POOL);
    check_case(fixture->name, "selected slot 128-byte canary untouched",
               memcmp(g_PsxRam + ram_index(slot), s_before_slot,
                      sizeof(s_before_slot)) == 0);
    check_case(fixture->name, "main-RAM guard canary untouched",
               memcmp(g_PsxRam + MAIN_RAM_BYTES,
                      s_before_ram + MAIN_RAM_BYTES,
                      sizeof(g_PsxRam) - MAIN_RAM_BYTES) == 0);
    check_case(fixture->name, "scratchpad canary untouched",
               memcmp(g_PsxScratchpad, s_before_scratch,
                      sizeof(g_PsxScratchpad)) == 0);

    if (strcmp(fixture->name, "fresh-zero-equal-postimage") == 0) {
        printf("EQUAL_POSTIMAGE fresh-zero actual=%04x expected=%04x "
               "ram_equal=%d fresh_lhu_count=%zu\n",
               (unsigned int)raw_load_u16(RETAIL_TEXT_FLAGS),
               (unsigned int)expected_flags,
               memcmp(g_PsxRam, s_expected_ram, sizeof(g_PsxRam)) == 0 ?
                   1 : 0,
               exact_event_count(0x80092C34u, EVENT_LHU,
                                 RETAIL_TEXT_FLAGS, 2u));
    }
}

static void run_oracle_sentinels(void)
{
    check_case("oracle", "0x0000 OR 2 is 0x0002",
               (u16)(0x0000u | 2u) == 0x0002u);
    check_case("oracle", "0x0001 OR 2 is 0x0003",
               (u16)(0x0001u | 2u) == 0x0003u);
    check_case("oracle", "0x1234 OR 2 is 0x1236",
               (u16)(0x1234u | 2u) == 0x1236u);
    check_case("oracle", "0x8000 OR 2 is 0x8002",
               (u16)(0x8000u | 2u) == 0x8002u);
    check_case("oracle", "0xFFFC OR 2 is 0xFFFE",
               (u16)(0xFFFCu | 2u) == 0xFFFEu);
    check_case("oracle", "0xFFFF OR 2 remains 0xFFFF",
               (u16)(0xFFFFu | 2u) == 0xFFFFu);
    check_case("oracle", "slot index INT32_MIN shift is irrelevant",
               ((u32)INT32_MIN << 7) == 0u);
    check_case("oracle", "slot index minus one shift wraps",
               ((u32)-1 << 7) == 0xFFFFFF80u);
    check_case("oracle", "retail first target independently fixed",
               0x80032F54u != 0x80034614u);
    check_case("oracle", "zero fixture postimage hides missing read",
               (u16)(0u | 2u) == 2u);
}

int main(void)
{
    static const Fixture fixtures[] = {
        {"fresh-zero-equal-postimage", 12, 0x0000u, 0x0000u,
         0x1357u, 0xA5u},
        {"fresh-one", -1, 0x8000u, 0x0001u,
         0x2468u, 0x5Au},
        {"fresh-1234-from-0040", 0, 0x0040u, 0x1234u,
         0xABCDu, 0xC3u},
        {"fresh-high-bit", INT32_MIN, 0x1234u, 0x8000u,
         0x7E81u, 0x3Cu},
        {"fresh-fffc", INT32_MAX, 0x0000u, 0xFFFCu,
         0x55AAu, 0x96u},
        {"fresh-ffff", 63, 0xFFFDu, 0xFFFFu,
         0x0F0Fu, 0x69u}
    };
    size_t index;

    for (index = 0u; index < sizeof(fixtures) / sizeof(fixtures[0]); index++)
        run_fixture(&fixtures[index], (u32)index + 0x71u);
    run_oracle_sentinels();

    printf("=== Results: %d/%d PASS ===\n", s_pass_count, s_total_count);
    return s_failure_count == 0 ? 0 : 1;
}
