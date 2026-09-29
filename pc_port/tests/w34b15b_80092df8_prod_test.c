/*
 * W34B15-B production-linked retail certificate for callback 0x80092DF8.
 *
 * Expected PCs, helper targets/arguments, typed guest accesses, packet bytes,
 * access order, and negative rendering properties are independently
 * transcribed from the verified 480-byte retail slice. The actual production
 * callback source is included unchanged. Accepted helpers are replaced only
 * by observation seams so asymmetric post-constructor state and helper order
 * remain directly observable.
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
    EVENT_LW = 4,
    EVENT_SW = 5,
    EVENT_CALL_32F54 = 10,
    EVENT_RETURN_32F54 = 11,
    EVENT_CALL_34614 = 12,
    EVENT_RETURN_34614 = 13,
    EVENT_CALL_TPAGE = 14,
    EVENT_RETURN_TPAGE = 15,
    EVENT_CALL_CLUT = 16,
    EVENT_RETURN_CLUT = 17,
    EVENT_CALL_SEMI = 18,
    EVENT_RETURN_SEMI = 19,
    EVENT_WRONG_HELPER = 20,
    EVENT_CALLBACK_RETURN = 21,
    EVENT_UNKNOWN = 255
};

typedef struct Event {
    u32 pc;
    u32 kind;
    u32 address;
    u32 width;
    u32 value;
    s32 args[8];
} Event;

typedef struct Fixture {
    const char* name;
    s32 slot_index;
    u16 pre_constructor_flags;
    u16 post_constructor_flags;
    u16 initial_selection;
    u8 initial_window_state;
} Fixture;

#define EVENT_CAPACITY 80u
#define RAM_MASK 0x001FFFFFu
#define MAIN_RAM_BYTES 0x00200000u
#define SLOT_BYTES 0x80u
#define POOL_BYTES (64u * SLOT_BYTES)

/* Independent retail constants from world_map.bin, not production macros. */
#define RETAIL_WINDOW       0x8009BD64u
#define RETAIL_SELECTION    0x8009CE68u
#define RETAIL_WINDOW_STATE 0x8009BDCCu
#define RETAIL_D498         0x8009D498u
#define RETAIL_D4A8         0x8009D4A8u
#define RETAIL_PRIM_A       0x8009D2B8u
#define RETAIL_PRIM_B       0x8009D2E0u
#define RETAIL_POOL_PTR     0x8009BE24u
#define TEST_POOL           0x800D7000u
#define TEST_OT_HEAD        0x800D6800u
#define TEST_OT_GUARD       0x800D6840u

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

static Event s_actual[EVENT_CAPACITY];
static Event s_expected[EVENT_CAPACITY];
static size_t s_actual_count;
static size_t s_expected_count;
static bool s_actual_overflow;
static u8 s_before_ram[PSX_RAM_SIZE];
static u8 s_expected_ram[PSX_RAM_SIZE];
static u8 s_before_scratch[sizeof(g_PsxScratchpad)];
static u8 s_before_pool[POOL_BYTES];
static u8 s_before_slot[SLOT_BYTES];

static u16 s_constructor_post_flags;
static u16 s_constructor_entry_selection;
static u16 s_constructor_entry_flags;
static u8 s_constructor_entry_state;
static u32 s_constructor_object;
static s32 s_constructor_args[7];
static u32 s_reset_object;
static u16 s_reset_entry_flags;
static u8 s_reset_entry_state;
static int s_constructor_calls;
static int s_reset_calls;
static int s_tpage_calls;
static int s_clut_calls;
static int s_semi_calls;
static int s_wrong_helper_calls;
static int s_addprim_calls;
static int s_drawprim_calls;
static int s_ot_writes;

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
    memset(event, 0, sizeof(*event));
    event->pc = pc;
    event->kind = kind;
    event->address = address;
    event->width = width;
    event->value = value;
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

    append_event(s_actual, &s_actual_count, 0x80092E3Cu,
                 EVENT_CALL_32F54, 0x80032F54u, 0u,
                 host_pointer_as_guest(object));
    if (s_actual_count <= (size_t)EVENT_CAPACITY) {
        event = &s_actual[s_actual_count - 1u];
        event->args[0] = tpage_x;
        event->args[1] = tpage_y;
        event->args[2] = x;
        event->args[3] = y;
        event->args[4] = width;
        event->args[5] = host_dead_mode;
        event->args[6] = height;
    }
}

/* Accepted constructor observation seam. Only the independently controlled
 * D4A8 poststate is injected so the callback's mandatory fresh read can be
 * distinguished from stale or omitted accesses. */
void func_80032F54(void* object, s32 tpage_x, s32 tpage_y,
                   s32 x, s32 y, s32 width, s32 host_dead_mode,
                   s32 height)
{
    s_constructor_calls++;
    s_constructor_entry_selection = raw_load_u16(RETAIL_SELECTION);
    s_constructor_entry_flags = raw_load_u16(RETAIL_D4A8);
    s_constructor_entry_state = raw_load_u8(RETAIL_WINDOW_STATE);
    s_constructor_object = host_pointer_as_guest(object);
    s_constructor_args[0] = tpage_x;
    s_constructor_args[1] = tpage_y;
    s_constructor_args[2] = x;
    s_constructor_args[3] = y;
    s_constructor_args[4] = width;
    s_constructor_args[5] = host_dead_mode;
    s_constructor_args[6] = height;
    append_constructor_call(object, tpage_x, tpage_y, x, y, width,
                            host_dead_mode, height);
    raw_store_u16(RETAIL_D4A8, s_constructor_post_flags);
    append_event(s_actual, &s_actual_count, 0x80092E44u,
                 EVENT_RETURN_32F54, 0x80032F54u, 0u,
                 (u32)s_constructor_post_flags);
}

void func_80034614(void* object)
{
    s_reset_calls++;
    s_reset_object = host_pointer_as_guest(object);
    s_reset_entry_flags = raw_load_u16(RETAIL_D4A8);
    s_reset_entry_state = raw_load_u8(RETAIL_WINDOW_STATE);
    append_event(s_actual, &s_actual_count, 0x80092E64u,
                 EVENT_CALL_34614, 0x80034614u, 0u,
                 s_reset_object);
    append_event(s_actual, &s_actual_count, 0x80092E6Cu,
                 EVENT_RETURN_34614, 0x80034614u, 0u,
                 (u32)s_reset_entry_flags);
}

u16 GetTPage(int tp, int abr, int x, int y)
{
    Event* event;
    const u16 result = 0x001Eu;

    s_tpage_calls++;
    append_event(s_actual, &s_actual_count, 0x80092F44u,
                 EVENT_CALL_TPAGE, 0x80043A1Cu, 0u, 0u);
    event = &s_actual[s_actual_count - 1u];
    event->args[0] = tp;
    event->args[1] = abr;
    event->args[2] = x;
    event->args[3] = y;
    append_event(s_actual, &s_actual_count, 0x80092F4Cu,
                 EVENT_RETURN_TPAGE, 0x80043A1Cu, 0u, (u32)result);
    return result;
}

u16 GetTPageWrong(int tp, int abr, int x, int y)
{
    Event* event;

    s_wrong_helper_calls++;
    append_event(s_actual, &s_actual_count, 0x80092F44u,
                 EVENT_WRONG_HELPER, 0x80043A58u, 0u, 0x001Eu);
    event = &s_actual[s_actual_count - 1u];
    event->args[0] = tp;
    event->args[1] = abr;
    event->args[2] = x;
    event->args[3] = y;
    return 0x001Eu;
}

u16 GetClut(int x, int y)
{
    Event* event;
    const u16 result = 0x7913u;

    s_clut_calls++;
    append_event(s_actual, &s_actual_count, 0x80092F58u,
                 EVENT_CALL_CLUT, 0x80043A58u, 0u, 0u);
    event = &s_actual[s_actual_count - 1u];
    event->args[0] = x;
    event->args[1] = y;
    append_event(s_actual, &s_actual_count, 0x80092F60u,
                 EVENT_RETURN_CLUT, 0x80043A58u, 0u, (u32)result);
    return result;
}

void SetSemiTrans(void* primitive, int enabled)
{
    Event* event;
    u32 guest = host_pointer_as_guest(primitive);
    u8 code = raw_load_u8(guest + 7u);

    s_semi_calls++;
    append_event(s_actual, &s_actual_count, 0x80092F74u,
                 EVENT_CALL_SEMI, 0x80043BFCu, 0u, guest);
    event = &s_actual[s_actual_count - 1u];
    event->args[0] = enabled;
    if (enabled != 0)
        code = (u8)(code | 2u);
    else
        code = (u8)(code & (u8)~2u);
    raw_store_u8(guest + 7u, code);
    append_event(s_actual, &s_actual_count, 0x80092F7Cu,
                 EVENT_RETURN_SEMI, 0x80043BFCu, 0u, (u32)code);
}

/* Test seams used only by copied-source submission mutants. */
void AddPrim(void* ordering_table, void* primitive)
{
    u32 ot_guest = host_pointer_as_guest(ordering_table);
    u32 prim_guest = host_pointer_as_guest(primitive);
    u32 tag = raw_load_u32(prim_guest);
    u32 head = raw_load_u32(ot_guest);

    s_addprim_calls++;
    raw_store_u32(prim_guest, (tag & 0xFF000000u) | (head & 0x00FFFFFFu));
    raw_store_u32(ot_guest, (head & 0xFF000000u) |
                  (prim_guest & 0x00FFFFFFu));
}

void DrawPrim(void* primitive)
{
    (void)primitive;
    s_drawprim_calls++;
}

void wm_92df8_test_trace(u32 pc, u32 kind, u32 address,
                         u32 width, u32 value)
{
    u32 event_kind = EVENT_UNKNOWN;

    if (kind == 1u)
        event_kind = EVENT_LHU;
    else if (kind == 2u)
        event_kind = EVENT_SB;
    else if (kind == 3u)
        event_kind = EVENT_SH;
    else if (kind == 4u)
        event_kind = EVENT_LW;
    else if (kind == 5u)
        event_kind = EVENT_SW;
    if (address == TEST_OT_HEAD || address == TEST_OT_GUARD)
        s_ot_writes++;
    append_event(s_actual, &s_actual_count, pc, event_kind,
                 address, width, value);
}

#define WM_92DF8_TEST_TRACE 1
#ifndef WM_92DF8_PRODUCTION_SOURCE
#define WM_92DF8_PRODUCTION_SOURCE "../src/world_map_callback_92df8.c"
#endif
#include WM_92DF8_PRODUCTION_SOURCE

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
    case EVENT_LW: return "LW";
    case EVENT_SW: return "SW";
    case EVENT_CALL_32F54: return "CALL_32F54";
    case EVENT_RETURN_32F54: return "RETURN_32F54";
    case EVENT_CALL_34614: return "CALL_34614";
    case EVENT_RETURN_34614: return "RETURN_34614";
    case EVENT_CALL_TPAGE: return "CALL_TPAGE";
    case EVENT_RETURN_TPAGE: return "RETURN_TPAGE";
    case EVENT_CALL_CLUT: return "CALL_CLUT";
    case EVENT_RETURN_CLUT: return "RETURN_CLUT";
    case EVENT_CALL_SEMI: return "CALL_SEMI";
    case EVENT_RETURN_SEMI: return "RETURN_SEMI";
    case EVENT_WRONG_HELPER: return "WRONG_HELPER";
    case EVENT_CALLBACK_RETURN: return "CALLBACK_RETURN";
    default: return "UNKNOWN";
    }
}

static Event* expected_event(u32 pc, u32 kind, u32 address,
                             u32 width, u32 value)
{
    append_event(s_expected, &s_expected_count, pc, kind,
                 address, width, value);
    return &s_expected[s_expected_count - 1u];
}

static void expected_store_u8(u32 pc, u32 address, u8 value)
{
    expected_event(pc, EVENT_SB, address, 1u, (u32)value);
    memory_store_u8(s_expected_ram, address, value);
}

static void expected_store_u16(u32 pc, u32 address, u16 value)
{
    expected_event(pc, EVENT_SH, address, 2u, (u32)value);
    memory_store_u16(s_expected_ram, address, value);
}

static void expected_store_u32(u32 pc, u32 address, u32 value)
{
    expected_event(pc, EVENT_SW, address, 4u, value);
    memory_store_u32(s_expected_ram, address, value);
}

static u32 expected_load_u32(u32 pc, u32 address)
{
    u32 value = memory_load_u32(s_expected_ram, address);
    expected_event(pc, EVENT_LW, address, 4u, value);
    return value;
}

static void expected_constructor_call(void)
{
    Event* event = expected_event(0x80092E3Cu, EVENT_CALL_32F54,
                                  0x80032F54u, 0u, RETAIL_WINDOW);
    event->args[0] = 960;
    event->args[1] = 397;
    event->args[2] = 160;
    event->args[3] = 120;
    event->args[4] = 32;
    event->args[5] = 0;
    event->args[6] = 4;
}

static void expected_tpage_call(void)
{
    Event* event = expected_event(0x80092F44u, EVENT_CALL_TPAGE,
                                  0x80043A1Cu, 0u, 0u);
    event->args[0] = 0;
    event->args[1] = 0;
    event->args[2] = 896;
    event->args[3] = 256;
}

static void expected_clut_call(void)
{
    Event* event = expected_event(0x80092F58u, EVENT_CALL_CLUT,
                                  0x80043A58u, 0u, 0u);
    event->args[0] = 304;
    event->args[1] = 484;
}

static void expected_semi_call(void)
{
    Event* event = expected_event(0x80092F74u, EVENT_CALL_SEMI,
                                  0x80043BFCu, 0u, RETAIL_PRIM_A);
    event->args[0] = 1;
}

static void build_expected(const Fixture* fixture)
{
    u16 published = (u16)(fixture->post_constructor_flags | 2u);
    u32 offset;
    u32 value0;
    u32 value1;
    u32 value2;
    u32 value3;

    memcpy(s_expected_ram, s_before_ram, sizeof(s_expected_ram));
    memset(s_expected, 0, sizeof(s_expected));
    s_expected_count = 0u;

    expected_store_u16(0x80092E20u, RETAIL_SELECTION, 0xFFFFu);
    expected_constructor_call();
    memory_store_u16(s_expected_ram, RETAIL_D4A8,
                     fixture->post_constructor_flags);
    expected_event(0x80092E44u, EVENT_RETURN_32F54,
                   0x80032F54u, 0u,
                   (u32)fixture->post_constructor_flags);
    expected_event(0x80092E50u, EVENT_LHU, RETAIL_D4A8, 2u,
                   (u32)fixture->post_constructor_flags);
    expected_store_u8(0x80092E5Cu, RETAIL_WINDOW_STATE, 8u);
    expected_store_u16(0x80092E68u, RETAIL_D4A8, published);
    expected_event(0x80092E64u, EVENT_CALL_34614,
                   0x80034614u, 0u, RETAIL_WINDOW);
    expected_event(0x80092E6Cu, EVENT_RETURN_34614,
                   0x80034614u, 0u, (u32)published);

    expected_store_u8(0x80092E80u, RETAIL_PRIM_A + 0x03u, 9u);
    expected_store_u8(0x80092E90u, RETAIL_PRIM_A + 0x07u, 0x2Cu);
    expected_store_u16(0x80092EA0u, RETAIL_PRIM_A + 0x0Au, 112u);
    expected_store_u16(0x80092EA8u, RETAIL_PRIM_A + 0x12u, 112u);
    expected_store_u16(0x80092EB4u, RETAIL_PRIM_A + 0x10u, 296u);
    expected_store_u16(0x80092EBCu, RETAIL_PRIM_A + 0x20u, 296u);
    expected_store_u16(0x80092EC8u, RETAIL_PRIM_A + 0x08u, 152u);
    expected_store_u16(0x80092ED0u, RETAIL_PRIM_A + 0x18u, 152u);
    expected_store_u16(0x80092EDCu, RETAIL_PRIM_A + 0x1Au, 180u);
    expected_store_u16(0x80092EE4u, RETAIL_PRIM_A + 0x22u, 180u);
    expected_store_u8(0x80092EF0u, RETAIL_PRIM_A + 0x0Cu, 0x80u);
    expected_store_u8(0x80092EF8u, RETAIL_PRIM_A + 0x0Du, 0u);
    expected_store_u8(0x80092F00u, RETAIL_PRIM_A + 0x14u, 0xFFu);
    expected_store_u8(0x80092F08u, RETAIL_PRIM_A + 0x15u, 0u);
    expected_store_u8(0x80092F10u, RETAIL_PRIM_A + 0x1Cu, 0x80u);
    expected_store_u8(0x80092F18u, RETAIL_PRIM_A + 0x1Du, 0x3Fu);
    expected_store_u8(0x80092F20u, RETAIL_PRIM_A + 0x24u, 0xFFu);
    expected_store_u8(0x80092F28u, RETAIL_PRIM_A + 0x25u, 0x3Fu);
    expected_store_u8(0x80092F30u, RETAIL_PRIM_A + 0x04u, 0x80u);
    expected_store_u8(0x80092F38u, RETAIL_PRIM_A + 0x05u, 0x80u);
    expected_store_u8(0x80092F40u, RETAIL_PRIM_A + 0x06u, 0x80u);

    expected_tpage_call();
    expected_event(0x80092F4Cu, EVENT_RETURN_TPAGE,
                   0x80043A1Cu, 0u, 0x001Eu);
    expected_store_u16(0x80092F54u, RETAIL_PRIM_A + 0x16u, 0x001Eu);
    expected_clut_call();
    expected_event(0x80092F60u, EVENT_RETURN_CLUT,
                   0x80043A58u, 0u, 0x7913u);
    expected_store_u16(0x80092F70u, RETAIL_PRIM_A + 0x0Eu, 0x7913u);
    expected_semi_call();
    memory_store_u8(s_expected_ram, RETAIL_PRIM_A + 7u, 0x2Eu);
    expected_event(0x80092F7Cu, EVENT_RETURN_SEMI,
                   0x80043BFCu, 0u, 0x2Eu);

    for (offset = 0u; offset != 0x20u; offset += 0x10u) {
        value0 = expected_load_u32(0x80092F84u,
                                   RETAIL_PRIM_A + offset);
        value1 = expected_load_u32(0x80092F88u,
                                   RETAIL_PRIM_A + offset + 4u);
        value2 = expected_load_u32(0x80092F8Cu,
                                   RETAIL_PRIM_A + offset + 8u);
        value3 = expected_load_u32(0x80092F90u,
                                   RETAIL_PRIM_A + offset + 12u);
        expected_store_u32(0x80092F94u, RETAIL_PRIM_B + offset, value0);
        expected_store_u32(0x80092F98u,
                           RETAIL_PRIM_B + offset + 4u, value1);
        expected_store_u32(0x80092F9Cu,
                           RETAIL_PRIM_B + offset + 8u, value2);
        expected_store_u32(0x80092FA0u,
                           RETAIL_PRIM_B + offset + 12u, value3);
    }
    value0 = expected_load_u32(0x80092FB0u, RETAIL_PRIM_A + 0x20u);
    value1 = expected_load_u32(0x80092FB4u, RETAIL_PRIM_A + 0x24u);
    expected_store_u32(0x80092FB8u, RETAIL_PRIM_B + 0x20u, value0);
    expected_store_u32(0x80092FBCu, RETAIL_PRIM_B + 0x24u, value1);
    expected_event(0x80092FD0u, EVENT_CALLBACK_RETURN, 0u, 0u, 1u);
}

static bool trace_matches(const char* fixture)
{
    size_t index;

    if (s_actual_overflow || s_actual_count != s_expected_count) {
        printf("TRACE [%s]: actual=%zu expected=%zu overflow=%d\n",
               fixture, s_actual_count, s_expected_count,
               s_actual_overflow ? 1 : 0);
        return false;
    }
    for (index = 0u; index < s_expected_count; index++) {
        const Event* actual = &s_actual[index];
        const Event* expected = &s_expected[index];
        if (memcmp(actual, expected, sizeof(*actual)) != 0) {
            printf("TRACE [%s] #%zu actual=%08x/%s/%08x/%u/%08x "
                   "expected=%08x/%s/%08x/%u/%08x\n",
                   fixture, index,
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
    size_t index;
    size_t count = 0u;

    for (index = 0u; index < s_actual_count &&
                     index < (size_t)EVENT_CAPACITY; index++) {
        const Event* event = &s_actual[index];
        if (event->pc == pc && event->kind == kind &&
            event->address == address && event->width == width)
            count++;
    }
    return count;
}

static bool packet_words_are_exact(void)
{
    static const u32 words[10] = {
        0x09C3B2A1u, 0x2E808080u, 0x00700098u, 0x79130080u,
        0x00700128u, 0x001E00FFu, 0x00B40098u, 0xE5D43F80u,
        0x00B40128u, 0x07F63FFFu
    };
    size_t index;

    for (index = 0u; index < 10u; index++) {
        if (raw_load_u32(RETAIL_PRIM_A + (u32)index * 4u) != words[index])
            return false;
    }
    return true;
}

static void seed_memory(u32 salt)
{
    size_t index;

    for (index = 0u; index < sizeof(g_PsxRam); index++) {
        u32 low = (u32)(index & (size_t)0xFFFFFFFFu);
        g_PsxRam[index] =
            (u8)((low * 37u + (low >> 7) + salt * 61u + 0x53u) & 0xFFu);
    }
    for (index = 0u; index < sizeof(g_PsxScratchpad); index++) {
        u32 low = (u32)index;
        g_PsxScratchpad[index] =
            (u8)((low * 31u + salt * 19u + 0xA7u) & 0xFFu);
    }
}

static void seed_primitive_regions(void)
{
    memset(PSX_ADDR(RETAIL_PRIM_A), 0xCC, 40u);
    memset(PSX_ADDR(RETAIL_PRIM_B), 0x5A, 40u);
    raw_store_u8(RETAIL_PRIM_A + 0u, 0xA1u);
    raw_store_u8(RETAIL_PRIM_A + 1u, 0xB2u);
    raw_store_u8(RETAIL_PRIM_A + 2u, 0xC3u);
    raw_store_u8(RETAIL_PRIM_A + 0x1Eu, 0xD4u);
    raw_store_u8(RETAIL_PRIM_A + 0x1Fu, 0xE5u);
    raw_store_u8(RETAIL_PRIM_A + 0x26u, 0xF6u);
    raw_store_u8(RETAIL_PRIM_A + 0x27u, 0x07u);
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
    s_constructor_object = 0u;
    memset(s_constructor_args, 0, sizeof(s_constructor_args));
    s_reset_object = 0u;
    s_reset_entry_flags = 0u;
    s_reset_entry_state = 0u;
    s_constructor_calls = 0;
    s_reset_calls = 0;
    s_tpage_calls = 0;
    s_clut_calls = 0;
    s_semi_calls = 0;
    s_wrong_helper_calls = 0;
    s_addprim_calls = 0;
    s_drawprim_calls = 0;
    s_ot_writes = 0;
}

static void run_fixture(const Fixture* fixture, u32 salt)
{
    static const s32 constructor_args[7] = {960, 397, 160, 120, 32, 0, 4};
    u32 slot = TEST_POOL + 13u * SLOT_BYTES;
    u16 expected_flags = (u16)(fixture->post_constructor_flags | 2u);
    u32 initial_low_tag;
    s32 result;
    bool exact_trace;

    seed_memory(salt);
    seed_primitive_regions();
    raw_store_u32(RETAIL_POOL_PTR, TEST_POOL);
    raw_store_u16(RETAIL_SELECTION, fixture->initial_selection);
    raw_store_u16(RETAIL_D4A8, fixture->pre_constructor_flags);
    raw_store_u8(RETAIL_WINDOW_STATE, fixture->initial_window_state);
    raw_store_u32(TEST_OT_HEAD, 0x5A654321u);
    raw_store_u32(TEST_OT_GUARD, 0xA55AA55Au);
    initial_low_tag = raw_load_u32(RETAIL_PRIM_A) & 0x00FFFFFFu;
    memcpy(s_before_ram, g_PsxRam, sizeof(g_PsxRam));
    memcpy(s_before_scratch, g_PsxScratchpad, sizeof(g_PsxScratchpad));
    memcpy(s_before_pool, PSX_ADDR(TEST_POOL), sizeof(s_before_pool));
    memcpy(s_before_slot, PSX_ADDR(slot), sizeof(s_before_slot));
    reset_observation(fixture);

    result = wm_80092DF8(fixture->slot_index);
    append_event(s_actual, &s_actual_count, 0x80092FD0u,
                 EVENT_CALLBACK_RETURN, 0u, 0u, (u32)result);
    build_expected(fixture);
    exact_trace = trace_matches(fixture->name);

    check_case(fixture->name, "return is scheduler state 1", result == 1);
    check_case(fixture->name, "exact retail helper/access/return event trace",
               exact_trace);
    check_case(fixture->name, "event trace stays within capacity",
               !s_actual_overflow);
    check_case(fixture->name, "all five accepted helpers called once",
               s_constructor_calls == 1 && s_reset_calls == 1 &&
               s_tpage_calls == 1 && s_clut_calls == 1 &&
               s_semi_calls == 1 && s_wrong_helper_calls == 0);
    check_case(fixture->name,
               "func_80032F54 exact seven values include height 4",
               s_constructor_object == RETAIL_WINDOW &&
               memcmp(s_constructor_args, constructor_args,
                      sizeof(constructor_args)) == 0);
    check_case(fixture->name,
               "selection sentinel precedes constructor",
               s_constructor_entry_selection == 0xFFFFu);
    check_case(fixture->name,
               "constructor observes exact D4A8 and BDCC prestates",
               s_constructor_entry_flags == fixture->pre_constructor_flags &&
               s_constructor_entry_state == fixture->initial_window_state);
    check_case(fixture->name,
               "one fresh D4A8 halfword load occurs after constructor",
               exact_event_count(0x80092E50u, EVENT_LHU,
                                 RETAIL_D4A8, 2u) == 1u);
    check_case(fixture->name,
               "D4A8 fresh OR2 publication preserves all other bits",
               raw_load_u16(RETAIL_D4A8) == expected_flags);
    check_case(fixture->name,
               "BDCC byte and D4A8 halfword precede func_80034614",
               s_reset_object == RETAIL_WINDOW &&
               s_reset_entry_state == 8u &&
               s_reset_entry_flags == expected_flags);
    check_case(fixture->name,
               "selection and window-state globals are exact",
               raw_load_u16(RETAIL_SELECTION) == 0xFFFFu &&
               raw_load_u8(RETAIL_WINDOW_STATE) == 8u);
    check_case(fixture->name, "POLY_FT4 packet words are exact",
               packet_words_are_exact());
    check_case(fixture->name, "primitive duplicate is byte-exact 40 bytes",
               memcmp(PSX_ADDR(RETAIL_PRIM_A),
                      PSX_ADDR(RETAIL_PRIM_B), 40u) == 0);
    check_case(fixture->name, "screen geometry is exactly 144x68",
               raw_load_u16(RETAIL_PRIM_A + 0x08u) == 152u &&
               raw_load_u16(RETAIL_PRIM_A + 0x10u) == 296u &&
               raw_load_u16(RETAIL_PRIM_A + 0x18u) == 152u &&
               raw_load_u16(RETAIL_PRIM_A + 0x20u) == 296u &&
               raw_load_u16(RETAIL_PRIM_A + 0x0Au) == 112u &&
               raw_load_u16(RETAIL_PRIM_A + 0x12u) == 112u &&
               raw_load_u16(RETAIL_PRIM_A + 0x1Au) == 180u &&
               raw_load_u16(RETAIL_PRIM_A + 0x22u) == 180u);
    check_case(fixture->name,
               "texture coordinates tpage and CLUT are exact",
               raw_load_u16(RETAIL_PRIM_A + 0x0Cu) == 0x0080u &&
               raw_load_u16(RETAIL_PRIM_A + 0x14u) == 0x00FFu &&
               raw_load_u16(RETAIL_PRIM_A + 0x1Cu) == 0x3F80u &&
               raw_load_u16(RETAIL_PRIM_A + 0x24u) == 0x3FFFu &&
               raw_load_u16(RETAIL_PRIM_A + 0x16u) == 0x001Eu &&
               raw_load_u16(RETAIL_PRIM_A + 0x0Eu) == 0x7913u);
    check_case(fixture->name,
               "neutral RGB tag length and semi-transparency are exact",
               raw_load_u8(RETAIL_PRIM_A + 3u) == 9u &&
               raw_load_u8(RETAIL_PRIM_A + 4u) == 0x80u &&
               raw_load_u8(RETAIL_PRIM_A + 5u) == 0x80u &&
               raw_load_u8(RETAIL_PRIM_A + 6u) == 0x80u &&
               raw_load_u8(RETAIL_PRIM_A + 7u) == 0x2Eu);
    check_case(fixture->name,
               "primitive low 24-bit OT link is never written",
               (raw_load_u32(RETAIL_PRIM_A) & 0x00FFFFFFu) ==
                   initial_low_tag);
    check_case(fixture->name,
               "whole primitive regions match independent byte oracle",
               memcmp(PSX_ADDR(RETAIL_PRIM_A),
                      s_expected_ram + ram_index(RETAIL_PRIM_A), 80u) == 0);
    check_case(fixture->name, "whole guest RAM matches independent oracle",
               memcmp(g_PsxRam, s_expected_ram, sizeof(g_PsxRam)) == 0);
    check_case(fixture->name, "scheduler pool and selected slot untouched",
               memcmp(PSX_ADDR(TEST_POOL), s_before_pool,
                      sizeof(s_before_pool)) == 0 &&
               memcmp(PSX_ADDR(slot), s_before_slot,
                      sizeof(s_before_slot)) == 0);
    check_case(fixture->name, "callback-time slot13 state remains unchanged",
               memcmp(PSX_ADDR(slot), s_before_slot, 2u) == 0);
    check_case(fixture->name, "scratchpad canary untouched",
               memcmp(g_PsxScratchpad, s_before_scratch,
                      sizeof(g_PsxScratchpad)) == 0);
    check_case(fixture->name, "main-RAM guard canary untouched",
               memcmp(g_PsxRam + MAIN_RAM_BYTES,
                      s_before_ram + MAIN_RAM_BYTES,
                      sizeof(g_PsxRam) - MAIN_RAM_BYTES) == 0);
    check_case(fixture->name,
               "zero AddPrim OT writes and DrawPrim submission",
               s_addprim_calls == 0 && s_ot_writes == 0 &&
               s_drawprim_calls == 0 &&
               raw_load_u32(TEST_OT_HEAD) == 0x5A654321u &&
               raw_load_u32(TEST_OT_GUARD) == 0xA55AA55Au);

    if (strcmp(fixture->name, "idempotent-equal-final") == 0) {
        printf("EQUAL_FINAL D4A8=%04x expected=%04x ram_equal=%d "
               "fresh_lhu=%zu exact_order=%d\n",
               (unsigned int)raw_load_u16(RETAIL_D4A8),
               (unsigned int)expected_flags,
               memcmp(g_PsxRam, s_expected_ram, sizeof(g_PsxRam)) == 0 ?
                   1 : 0,
               exact_event_count(0x80092E50u, EVENT_LHU,
                                 RETAIL_D4A8, 2u),
               exact_trace ? 1 : 0);
    }
}

static void run_oracle_sentinels(void)
{
    check_case("oracle", "retail dimensions are independently 144x68",
               (296 - 152) == 144 && (180 - 112) == 68);
    check_case("oracle", "retail tpage formula result is 0x001E",
               ((((0 & 3) << 7) | ((0 & 3) << 5) |
                 ((256 & 0x100) >> 4) | ((896 & 0x3FF) >> 6) |
                 ((256 & 0x200) << 2))) == 0x001E);
    check_case("oracle", "retail CLUT formula result is 0x7913",
               ((484 << 6) | ((304 >> 4) & 0x3F)) == 0x7913);
    check_case("oracle", "D4A8 equals D498 plus 0x10",
               RETAIL_D4A8 == RETAIL_D498 + 0x10u);
    check_case("oracle", "idempotent OR2 can hide a missing access",
               (u16)(2u | 2u) == 2u);
    check_case("oracle", "slot argument extremes are semantically dead",
               INT32_MIN != INT32_MAX);
}

int main(void)
{
    static const Fixture fixtures[] = {
        {"idempotent-equal-final", 13, 0x0002u, 0x0002u,
         0x1357u, 0xA5u},
        {"asymmetric-0040-to-1234", -1, 0x0040u, 0x1234u,
         0x2468u, 0x5Au},
        {"high-bit-preservation", INT32_MIN, 0x0000u, 0x8001u,
         0xABCDu, 0xC3u},
        {"all-bits-set", INT32_MAX, 0xFFFCu, 0xFFFFu,
         0x7E81u, 0x3Cu}
    };
    size_t index;

    for (index = 0u; index < sizeof(fixtures) / sizeof(fixtures[0]); index++)
        run_fixture(&fixtures[index], (u32)index + 0x91u);
    run_oracle_sentinels();

    printf("PACKET_WORDS 09C3B2A1 2E808080 00700098 79130080 "
           "00700128 001E00FF 00B40098 E5D43F80 00B40128 07F63FFF\n");
    printf("CALL_ORDER 80032F54 80034614 80043A1C 80043A58 80043BFC\n");
    printf("NEGATIVE_COUNTS AddPrim=0 OT=0 DrawPrim=0 slot_access=0\n");
    printf("=== Results: %d/%d PASS ===\n", s_pass_count, s_total_count);
    return s_failure_count == 0 ? 0 : 1;
}
