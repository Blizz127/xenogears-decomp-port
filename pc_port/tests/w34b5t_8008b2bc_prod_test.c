/*
 * W34B5-T production-linked focused test for world callback 0x8008B2BC.
 *
 * This file supplies controlled seams only for the callback's four native
 * dependencies.  Expected state is computed by the declarative retail route
 * table below; no expected value is obtained from the production callback.
 *
 * Example O0 build (run from the repository root):
 *
 *   gcc -std=c11 -O0 -g -DXENO_PC_PORT -fno-pie -no-pie \
 *     -Iinclude -Ipc_port/src \
 *     pc_port/tests/w34b5t_8008b2bc_prod_test.c \
 *     pc_port/src/world_map_callback_8b2bc.c \
 *     pc_port/src/world_map_scheduler.c pc_port/src/psx_memory.c \
 *     -o scratchpad/w34b5t_8b2bc_implementation/w34b5t_o0
 *
 * Replace -O0 with -O2 for the optimized run.  For the UB gate add:
 *
 *   -fsanitize=undefined -fno-sanitize-recover=undefined
 */
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_8b2bc.h"
#include "world_map_scheduler.h"

#define U8(a)  (*(u8*)PSX_ADDR(a))
#define U16(a) (*(u16*)PSX_ADDR(a))
#define U32(a) (*(u32*)PSX_ADDR(a))

#define POOL_ADDR       0x80020000u
#define PACKAGE_ADDR    0x80030000u

#define BE24            0x8009BE24u
#define CD38            0x8009CD38u
#define MODE            0x8009BE10u
#define SEED_X          0x8006EE54u
#define SEED_Z          0x8006EE56u
#define SEED_VALUE      0x8006EE58u
#define C584            0x8009C584u
#define RUNTIME_X       0x8009C5ACu
#define RUNTIME_Z       0x8009C5B4u
#define F8E6            0x8006F8E6u
#define F369            0x8006F369u

#define SLOT_STATE      0x00u
#define SLOT_TIMER      0x02u
#define SLOT_HEADER4    0x04u
#define SLOT_CB0        0x18u
#define SLOT_CB1        0x1Cu
#define SLOT_CONTROL    0x20u
#define SLOT_FLAG       0x24u
#define SLOT_X          0x28u
#define SLOT_Y          0x2Cu
#define SLOT_Z          0x30u
#define SLOT_AUX        0x34u
#define SLOT_CLEAR38    0x38u
#define SLOT_CLEAR3C    0x3Cu
#define SLOT_CLEAR40    0x40u
#define SLOT_VALUE      0x48u
#define SLOT_CONST      0x4Au
#define SLOT_OBJECT     0x4Cu
#define SLOT_HISTORY    0x58u
#define SLOT_SIGNED     0x5Cu

#define DIRECT_SLOT_INDEX 5
#define OBJECT_SIZE       0x200u
#define SLOT_SENTINEL_PTR 0xA1B2C3D4u
#define INITIAL_FLAGS     0xA5A5000Du
#define INITIAL_VALUE     0x8001u
#define RELOADED_C584     0x5AA57FFEu
#define RELOAD_SCALE_FLAGS 0x13570005u
#define RELOAD_FINAL_FLAGS 0x2468000Du

enum call_id {
    CALL_ALLOC = 1,
    CALL_ANIM,
    CALL_SCALE,
    CALL_TERRAIN
};

enum route_family {
    ROUTE_COMMON = 0,
    ROUTE_MODES_1_3,
    ROUTE_MODES_4_6,
    ROUTE_MODE_7
};

/* Raw retail jump table 0x80070638, expressed as independently audited
 * semantic families.  The production callback is not consulted here. */
static const enum route_family s_retail_routes[13] = {
    ROUTE_MODES_1_3, ROUTE_MODES_1_3, ROUTE_MODES_1_3,
    ROUTE_MODES_4_6, ROUTE_MODES_4_6, ROUTE_MODES_4_6,
    ROUTE_MODE_7,
    ROUTE_COMMON, ROUTE_COMMON, ROUTE_COMMON,
    ROUTE_COMMON, ROUTE_COMMON, ROUTE_COMMON
};

typedef struct test_case {
    u32 mode;
    u8 f369;
    u8 f8e6;
    u16 seed_x;
    u16 seed_z;
    u32 runtime_x;
    u32 runtime_z;
    u32 terrain0;
    u32 terrain1;
} test_case_t;

typedef struct oracle_result {
    enum route_family route;
    s32 return_state;
    u16 control;
    u16 flag;
    u32 x;
    u32 y;
    u32 z;
    u32 aux;
    u16 value;
    u32 signed_value;
    u32 history;
    u8 f8e6;
    int object_created;
    int second_terrain;
    int control_written;
} oracle_result_t;

static int s_pass_count;
static int s_total_count;
static int s_failure_count;
static char s_case_name[96];

static u8 s_object[OBJECT_SIZE];
static u8 s_reload_scale_object[OBJECT_SIZE];
static u8 s_reload_final_object[OBJECT_SIZE];
static u8 s_ram_before[PSX_RAM_SIZE];
static int s_call_log[8];
static int s_call_count;
static int s_alloc_calls;
static int s_anim_calls;
static int s_scale_calls;
static void* s_alloc_package;
static s16 s_alloc_args[5];
static void* s_anim_object;
static s16 s_anim_index;
static int s_anim_saw_owner_store;
static void* s_scale_object;
static short s_scale_value;
static s32 s_terrain_x[2];
static s32 s_terrain_z[2];
static u32 s_terrain_returns[2];
static int s_terrain_count;
static u32 s_current_slot;
static int s_retarget_object_reloads;
static int s_mutate_f369_on_first_terrain;
static u8 s_first_terrain_f369;
static int s_later_callback_calls;
static int s_later_callback_arg;

static void check_result(const char* description, int condition)
{
    s_total_count++;
    if (condition) {
        s_pass_count++;
    } else {
        s_failure_count++;
        printf("FAIL [%s]: %s\n", s_case_name, description);
    }
}

static s32 bits_to_s32(u32 bits)
{
    s32 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static u32 sign_extend16_bits(u16 bits)
{
    if (bits <= 0x7FFFu)
        return (u32)bits;
    return (u32)bits + 0xFFFF0000u;
}

static u32 object_bits(void)
{
    return (u32)(uintptr_t)(void*)s_object;
}

static void log_call(int call)
{
    if (s_call_count < (int)(sizeof(s_call_log) / sizeof(s_call_log[0])))
        s_call_log[s_call_count] = call;
    s_call_count++;
}

/* ------------------------- controlled dependency seams ------------------ */

void* func_80024524(void* package, s16 tex_x, s16 tex_y,
                    s16 clut_x, s16 clut_y, s16 arg5)
{
    log_call(CALL_ALLOC);
    s_alloc_calls++;
    s_alloc_package = package;
    s_alloc_args[0] = tex_x;
    s_alloc_args[1] = tex_y;
    s_alloc_args[2] = clut_x;
    s_alloc_args[3] = clut_y;
    s_alloc_args[4] = arg5;

    /* Dependency-owned effects are deliberately simple and do not duplicate
     * callback behavior.  They leave one flags word for the callback to edit. */
    memset(s_object, 0x6B, sizeof(s_object));
    *(u32*)(s_object + 0x3Cu) = INITIAL_FLAGS;
    return s_object;
}

void func_800245D8(void* object, s16 animation)
{
    log_call(CALL_ANIM);
    s_anim_calls++;
    s_anim_object = object;
    s_anim_index = animation;
    s_anim_saw_owner_store =
        U32(s_current_slot + SLOT_OBJECT) == (u32)(uintptr_t)object;
    if (s_retarget_object_reloads)
        U32(s_current_slot + SLOT_OBJECT) =
            (u32)(uintptr_t)(void*)s_reload_scale_object;
}

void SpriteSetScale(void* object, short scale)
{
    log_call(CALL_SCALE);
    s_scale_calls++;
    s_scale_object = object;
    s_scale_value = scale;
    if (s_retarget_object_reloads)
        U32(s_current_slot + SLOT_OBJECT) =
            (u32)(uintptr_t)(void*)s_reload_final_object;
}

s32 wm_80093978(s32 x, s32 z)
{
    int index = s_terrain_count;
    log_call(CALL_TERRAIN);
    if (index < 2) {
        s_terrain_x[index] = x;
        s_terrain_z[index] = z;
    }
    s_terrain_count++;

    if (index == 0 && s_mutate_f369_on_first_terrain)
        U8(F369) = s_first_terrain_f369;

    /* A second-call mutation proves that cb0 reloads C584 after the terrain
     * dependency returns, while slot+5C remains derived from initial EE58. */
    if (index == 1)
        U32(C584) = RELOADED_C584;

    return bits_to_s32(s_terrain_returns[index < 2 ? index : 1]);
}

static s16 later_callback(int slot_index)
{
    s_later_callback_calls++;
    s_later_callback_arg = slot_index;
    return 1;
}

/* ------------------------------ oracle ---------------------------------- */

static enum route_family oracle_route(u32 mode)
{
    u32 index = mode - 1u;
    if (index >= 13u)
        return ROUTE_COMMON;
    return s_retail_routes[index];
}

static oracle_result_t oracle_for(const test_case_t* tc)
{
    oracle_result_t out;

    out.route = oracle_route(tc->mode);
    out.return_state = tc->f369 == 0xFFu ? 3 : 1;
    out.control = 0xBEEFu;
    out.flag = tc->f369 == 0xFFu ? 1u : 0u;
    out.x = (u32)tc->seed_x << 12;
    out.y = tc->terrain0;
    out.z = (u32)tc->seed_z << 12;
    out.aux = 0x11223344u;
    out.value = INITIAL_VALUE;
    out.signed_value = sign_extend16_bits(INITIAL_VALUE);
    out.history = 15u;
    out.f8e6 = tc->f8e6;
    out.object_created = tc->f369 != 0xFFu;
    out.second_terrain = 0;
    out.control_written = 0;

    switch (out.route) {
    case ROUTE_MODES_1_3:
        if (tc->f8e6 == 0u) {
            out.x = tc->runtime_x;
            out.y = tc->terrain1;
            out.z = tc->runtime_z;
            out.value = (u16)RELOADED_C584;
            out.second_terrain = 1;
        } else {
            out.control = 1u;
            out.flag = 1u;
            out.control_written = 1;
        }
        break;
    case ROUTE_MODES_4_6:
        out.control = 3u;
        out.flag = 1u;
        out.control_written = 1;
        if (tc->f369 != 0xFFu)
            out.f8e6 = 1u;
        break;
    case ROUTE_MODE_7:
        out.control = 2u;
        out.flag = 1u;
        out.control_written = 1;
        if (tc->f369 != 0xFFu)
            out.f8e6 = 1u;
        break;
    case ROUTE_COMMON:
        break;
    }

    return out;
}

static void reset_instrumentation(void)
{
    memset(s_call_log, 0, sizeof(s_call_log));
    s_call_count = 0;
    s_alloc_calls = 0;
    s_anim_calls = 0;
    s_scale_calls = 0;
    s_alloc_package = NULL;
    memset(s_alloc_args, 0, sizeof(s_alloc_args));
    s_anim_object = NULL;
    s_anim_index = (s16)-1;
    s_anim_saw_owner_store = 0;
    s_scale_object = NULL;
    s_scale_value = 0;
    memset(s_terrain_x, 0, sizeof(s_terrain_x));
    memset(s_terrain_z, 0, sizeof(s_terrain_z));
    s_terrain_count = 0;
    s_retarget_object_reloads = 0;
    s_mutate_f369_on_first_terrain = 0;
    s_first_terrain_f369 = 0u;
    s_later_callback_calls = 0;
    s_later_callback_arg = -1;
}

static void prepare_case(const test_case_t* tc, int slot_index)
{
    u32 slot = POOL_ADDR + ((u32)slot_index << 7);

    memset(g_PsxRam, 0xA5, PSX_RAM_SIZE);
    memset(s_object, 0xD6, sizeof(s_object));
    reset_instrumentation();

    s_current_slot = slot;
    s_terrain_returns[0] = tc->terrain0;
    s_terrain_returns[1] = tc->terrain1;

    U32(BE24) = POOL_ADDR;
    U32(CD38) = PACKAGE_ADDR;
    U32(MODE) = tc->mode;
    U16(SEED_X) = tc->seed_x;
    U16(SEED_Z) = tc->seed_z;
    U16(SEED_VALUE) = INITIAL_VALUE;
    U32(C584) = 0xA55A2468u;
    U32(RUNTIME_X) = tc->runtime_x;
    U32(RUNTIME_Z) = tc->runtime_z;
    U8(F8E6) = tc->f8e6;
    U8(F369) = tc->f369;

    memset(PSX_ADDR(slot), 0xCD, 0x80u);
    U16(slot + SLOT_STATE) = 0x5A5Au;
    U16(slot + SLOT_TIMER) = 0x1357u;
    U16(slot + SLOT_HEADER4) = 0x2468u;
    U32(slot + SLOT_CB0) = 0x8008B2BCu;
    U32(slot + SLOT_CB1) = 0x8008B644u;
    U16(slot + SLOT_CONTROL) = 0xBEEFu;
    U16(slot + SLOT_FLAG) = 0xCAFEu;
    U32(slot + SLOT_AUX) = 0x11223344u;
    U32(slot + SLOT_OBJECT) = SLOT_SENTINEL_PTR;

    memcpy(s_ram_before, g_PsxRam, PSX_RAM_SIZE);
}

static int byte_in_range(size_t offset, u32 guest_addr, size_t width)
{
    size_t start = (size_t)(guest_addr & 0x1FFFFFu);
    return offset >= start && offset < start + width;
}

static int ram_byte_authorized(size_t offset, const oracle_result_t* expected)
{
    u32 slot = s_current_slot;

    if (byte_in_range(offset, slot + SLOT_FLAG, 2u) ||
        byte_in_range(offset, slot + SLOT_X, 12u) ||
        byte_in_range(offset, slot + SLOT_CLEAR38, 12u) ||
        byte_in_range(offset, slot + SLOT_VALUE, 4u) ||
        byte_in_range(offset, slot + SLOT_HISTORY, 4u) ||
        byte_in_range(offset, slot + SLOT_SIGNED, 4u))
        return 1;

    if (expected->control_written &&
        byte_in_range(offset, slot + SLOT_CONTROL, 2u))
        return 1;
    if (expected->object_created &&
        byte_in_range(offset, slot + SLOT_OBJECT, 4u))
        return 1;
    if (expected->second_terrain && byte_in_range(offset, C584, 4u))
        return 1; /* controlled terrain-seam effect */
    if (s_mutate_f369_on_first_terrain && byte_in_range(offset, F369, 1u))
        return 1; /* controlled terrain-seam effect */
    if ((expected->route == ROUTE_MODES_4_6 ||
         expected->route == ROUTE_MODE_7) &&
        (expected->object_created ||
         (s_mutate_f369_on_first_terrain &&
          s_first_terrain_f369 != 0xFFu)) &&
        byte_in_range(offset, F8E6, 1u))
        return 1;
    return 0;
}

static int only_authorized_ram_changed(const oracle_result_t* expected)
{
    size_t i;
    for (i = 0; i < (size_t)PSX_RAM_SIZE; i++) {
        if (g_PsxRam[i] != s_ram_before[i] &&
            !ram_byte_authorized(i, expected))
            return 0;
    }
    return 1;
}

static int object_matches_expected(const oracle_result_t* expected)
{
    size_t i;

    if (!expected->object_created) {
        for (i = 0; i < sizeof(s_object); i++)
            if (s_object[i] != 0xD6u)
                return 0;
        return 1;
    }

    for (i = 0; i < sizeof(s_object); i++) {
        u8 expected_byte = 0x6Bu;
        if (i >= 0x3Cu && i < 0x40u) {
            u32 flags = INITIAL_FLAGS & 0xFFFFFFFBu;
            expected_byte = (u8)(flags >> ((u32)(i - 0x3Cu) * 8u));
        }
        if (s_object[i] != expected_byte)
            return 0;
    }
    return 1;
}

static int object_has_fill_and_flags(const u8* object, u8 fill, u32 flags)
{
    size_t i;
    for (i = 0; i < OBJECT_SIZE; i++) {
        u8 expected_byte = fill;
        if (i >= 0x3Cu && i < 0x40u)
            expected_byte = (u8)(flags >> ((u32)(i - 0x3Cu) * 8u));
        if (object[i] != expected_byte)
            return 0;
    }
    return 1;
}

static int call_order_matches(const oracle_result_t* expected)
{
    int wanted[5];
    int count = 0;
    int i;

    if (expected->object_created) {
        wanted[count++] = CALL_ALLOC;
        wanted[count++] = CALL_ANIM;
        wanted[count++] = CALL_SCALE;
    }
    wanted[count++] = CALL_TERRAIN;
    if (expected->second_terrain)
        wanted[count++] = CALL_TERRAIN;

    if (s_call_count != count)
        return 0;
    for (i = 0; i < count; i++)
        if (s_call_log[i] != wanted[i])
            return 0;
    return 1;
}

static void verify_case(const test_case_t* tc, int slot_index)
{
    u32 slot = POOL_ADDR + ((u32)slot_index << 7);
    oracle_result_t expected;
    s32 result;

    prepare_case(tc, slot_index);
    expected = oracle_for(tc);
    result = wm_8008B2BC((s32)slot_index);

    check_result("return state", result == expected.return_state);
    check_result("direct callback does not own scheduler state",
                 U16(slot + SLOT_STATE) == 0x5A5Au);
    check_result("scheduler timer untouched", U16(slot + SLOT_TIMER) == 0x1357u);
    check_result("scheduler header +4 untouched",
                 U16(slot + SLOT_HEADER4) == 0x2468u);
    check_result("cb0 header untouched", U32(slot + SLOT_CB0) == 0x8008B2BCu);
    check_result("cb1 header untouched", U32(slot + SLOT_CB1) == 0x8008B644u);

    check_result("exact dependency call order", call_order_matches(&expected));
    check_result("first terrain executes", s_terrain_count >= 1);
    check_result("exact terrain call count",
                 s_terrain_count == (expected.second_terrain ? 2 : 1));
    check_result("seed X is zero-extended LHU then shifted",
                 (u32)s_terrain_x[0] == ((u32)tc->seed_x << 12));
    check_result("seed Z is zero-extended LHU then shifted",
                 (u32)s_terrain_z[0] == ((u32)tc->seed_z << 12));
    if (expected.second_terrain) {
        check_result("second terrain signed X bits",
                     (u32)s_terrain_x[1] == tc->runtime_x);
        check_result("second terrain signed Z bits",
                     (u32)s_terrain_z[1] == tc->runtime_z);
        check_result("C584 dependency mutation observed",
                     U32(C584) == RELOADED_C584);
    } else {
        check_result("C584 remains read-only without second terrain",
                     U32(C584) == 0xA55A2468u);
    }

    if (expected.object_created) {
        check_result("allocation called once", s_alloc_calls == 1);
        check_result("package guest pointer resolved to host",
                     s_alloc_package == PSX_ADDR(PACKAGE_ADDR));
        check_result("allocation tex X", s_alloc_args[0] == (s16)0x110);
        check_result("allocation tex Y", s_alloc_args[1] == (s16)0x1E0);
        check_result("allocation clut X", s_alloc_args[2] == (s16)0x150);
        check_result("allocation clut Y", s_alloc_args[3] == (s16)0x100);
        check_result("allocation arg5", s_alloc_args[4] == (s16)0x40);
        check_result("object stored before animation dependency",
                     s_anim_saw_owner_store);
        check_result("animation called once", s_anim_calls == 1);
        check_result("animation object", s_anim_object == (void*)s_object);
        check_result("animation zero", s_anim_index == 0);
        check_result("scale called once", s_scale_calls == 1);
        check_result("scale object", s_scale_object == (void*)s_object);
        check_result("scale 0x1800", s_scale_value == (short)0x1800);
        check_result("slot object exact packed pointer",
                     U32(slot + SLOT_OBJECT) == object_bits());
        check_result("packed object pointer round trip",
                     (void*)(uintptr_t)U32(slot + SLOT_OBJECT) ==
                     (void*)s_object);
        check_result("object bit2 cleared and unrelated flags preserved",
                     *(u32*)(s_object + 0x3Cu) ==
                     (INITIAL_FLAGS & 0xFFFFFFFBu));
    } else {
        check_result("suppression skips allocation", s_alloc_calls == 0);
        check_result("suppression skips animation", s_anim_calls == 0);
        check_result("suppression skips scale", s_scale_calls == 0);
        check_result("suppression preserves object pointer sentinel",
                     U32(slot + SLOT_OBJECT) == SLOT_SENTINEL_PTR);
    }
    check_result("object-region dependency/direct-write canary",
                 object_matches_expected(&expected));

    check_result("slot +20 mode control", U16(slot + SLOT_CONTROL) == expected.control);
    check_result("slot +24 path flag", U16(slot + SLOT_FLAG) == expected.flag);
    check_result("slot +28 X", U32(slot + SLOT_X) == expected.x);
    check_result("slot +2C terrain Y", U32(slot + SLOT_Y) == expected.y);
    check_result("slot +30 Z", U32(slot + SLOT_Z) == expected.z);
    check_result("slot +34 preserved", U32(slot + SLOT_AUX) == expected.aux);
    check_result("slot +38 zero", U32(slot + SLOT_CLEAR38) == 0u);
    check_result("slot +3C zero", U32(slot + SLOT_CLEAR3C) == 0u);
    check_result("slot +40 zero", U32(slot + SLOT_CLEAR40) == 0u);
    check_result("slot +48 exact value", U16(slot + SLOT_VALUE) == expected.value);
    check_result("slot +4A literal 8", U16(slot + SLOT_CONST) == 8u);
    check_result("slot +58 literal 15", U32(slot + SLOT_HISTORY) == expected.history);
    check_result("slot +5C sign-extends initial EE58",
                 U32(slot + SLOT_SIGNED) == expected.signed_value);
    if (expected.second_terrain) {
        check_result("+48 reload diverges from initial +5C",
                     U16(slot + SLOT_VALUE) == 0x7FFEu &&
                     U32(slot + SLOT_SIGNED) == 0xFFFF8001u);
    }

    check_result("F369 is read-only", U8(F369) == tc->f369);
    check_result("F8E6 exact result", U8(F8E6) == expected.f8e6);
    check_result("mode global read-only", U32(MODE) == tc->mode);
    check_result("seed globals read-only",
                 U16(SEED_X) == tc->seed_x &&
                 U16(SEED_Z) == tc->seed_z &&
                 U16(SEED_VALUE) == INITIAL_VALUE);
    check_result("runtime coordinate globals read-only",
                 U32(RUNTIME_X) == tc->runtime_x &&
                 U32(RUNTIME_Z) == tc->runtime_z);

    check_result("full PSX-RAM write-set canary",
                 only_authorized_ram_changed(&expected));
    check_result("preceding slot guard preserved",
                 memcmp(PSX_ADDR(slot - 0x80u),
                        s_ram_before + ((slot - 0x80u) & 0x1FFFFFu),
                        0x80u) == 0);
    check_result("following slot guard preserved",
                 memcmp(PSX_ADDR(slot + 0x80u),
                        s_ram_before + ((slot + 0x80u) & 0x1FFFFFu),
                        0x80u) == 0);
}

static void run_direct_matrix(void)
{
    static const u32 modes[] = {
        1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u, 12u, 13u,
        0u, 14u, UINT32_MAX, 0x80000000u
    };
    static const u8 f369_values[] = { 0x00u, 0x0Au, 0xFFu };
    static const u8 f8e6_values[] = { 0x00u, 0x6Du };
    unsigned mode_i;
    unsigned f369_i;
    unsigned f8e6_i;

    for (mode_i = 0;
         mode_i < (unsigned)(sizeof(modes) / sizeof(modes[0]));
         mode_i++) {
        for (f369_i = 0;
             f369_i < (unsigned)(sizeof(f369_values) / sizeof(f369_values[0]));
             f369_i++) {
            for (f8e6_i = 0;
                 f8e6_i < (unsigned)(sizeof(f8e6_values) /
                                     sizeof(f8e6_values[0]));
                 f8e6_i++) {
                test_case_t tc;
                tc.mode = modes[mode_i];
                tc.f369 = f369_values[f369_i];
                tc.f8e6 = f8e6_values[f8e6_i];
                tc.seed_x = 0x8001u;
                tc.seed_z = 0xFFFFu;
                tc.runtime_x = 0xFFF00000u;
                tc.runtime_z = 0x80000000u;
                tc.terrain0 = 0x11223344u;
                tc.terrain1 = 0xF2345678u;

                snprintf(s_case_name, sizeof(s_case_name),
                         "mode=%08x F369=%02x F8E6=%02x",
                         (unsigned)tc.mode, (unsigned)tc.f369,
                         (unsigned)tc.f8e6);
                verify_case(&tc, DIRECT_SLOT_INDEX);
            }
        }
    }
}

static void run_object_reload_observability(void)
{
    test_case_t tc;
    oracle_result_t expected;
    u32 slot = POOL_ADDR + ((u32)DIRECT_SLOT_INDEX << 7);
    s32 result;

    tc.mode = 8u;
    tc.f369 = 0x0Au;
    tc.f8e6 = 0u;
    tc.seed_x = 0x8001u;
    tc.seed_z = 0xFFFFu;
    tc.runtime_x = 0xFFF00000u;
    tc.runtime_z = 0x80000000u;
    tc.terrain0 = 0x11223344u;
    tc.terrain1 = 0xF2345678u;

    snprintf(s_case_name, sizeof(s_case_name), "slot+4C-reload-observability");
    prepare_case(&tc, DIRECT_SLOT_INDEX);
    expected = oracle_for(&tc);
    memset(s_reload_scale_object, 0xB2, sizeof(s_reload_scale_object));
    memset(s_reload_final_object, 0xC3, sizeof(s_reload_final_object));
    *(u32*)(s_reload_scale_object + 0x3Cu) = RELOAD_SCALE_FLAGS;
    *(u32*)(s_reload_final_object + 0x3Cu) = RELOAD_FINAL_FLAGS;
    s_retarget_object_reloads = 1;

    result = wm_8008B2BC((s32)DIRECT_SLOT_INDEX);

    check_result("reload test returns present state", result == 1);
    check_result("allocation result stored before animation",
                 s_anim_saw_owner_store && s_anim_object == (void*)s_object);
    check_result("animation seam retarget is reloaded for scale",
                 s_scale_object == (void*)s_reload_scale_object);
    check_result("scale seam retarget remains in slot",
                 U32(slot + SLOT_OBJECT) ==
                 (u32)(uintptr_t)(void*)s_reload_final_object);
    check_result("final packed pointer round trip",
                 (void*)(uintptr_t)U32(slot + SLOT_OBJECT) ==
                 (void*)s_reload_final_object);
    check_result("allocation object flags were not stale-cleared",
                 object_has_fill_and_flags(s_object, 0x6Bu, INITIAL_FLAGS));
    check_result("scale-target flags were not stale-cleared",
                 object_has_fill_and_flags(s_reload_scale_object, 0xB2u,
                                           RELOAD_SCALE_FLAGS));
    check_result("second +4C reload directs bit clear to final object",
                 object_has_fill_and_flags(s_reload_final_object, 0xC3u,
                                           RELOAD_FINAL_FLAGS & 0xFFFFFFFBu));
    check_result("reload test exact dependency order",
                 call_order_matches(&expected));
    check_result("reload test callback still does not own state",
                 U16(slot + SLOT_STATE) == 0x5A5Au);
    check_result("reload seam changes only authorized guest field",
                 only_authorized_ram_changed(&expected));
}

static void run_f369_reread_observability(void)
{
    test_case_t tc;
    oracle_result_t expected;
    u32 slot = POOL_ADDR + ((u32)DIRECT_SLOT_INDEX << 7);
    s32 result;

    tc.mode = 4u;
    tc.f369 = 0x0Au;
    tc.f8e6 = 0x6Du;
    tc.seed_x = 0x8001u;
    tc.seed_z = 0xFFFFu;
    tc.runtime_x = 0xFFF00000u;
    tc.runtime_z = 0x80000000u;
    tc.terrain0 = 0x11223344u;
    tc.terrain1 = 0xF2345678u;

    snprintf(s_case_name, sizeof(s_case_name), "F369-reread-present-to-FF");
    prepare_case(&tc, DIRECT_SLOT_INDEX);
    expected = oracle_for(&tc);
    s_mutate_f369_on_first_terrain = 1;
    s_first_terrain_f369 = 0xFFu;
    result = wm_8008B2BC((s32)DIRECT_SLOT_INDEX);

    check_result("entry F369 controls return before seam mutation", result == 1);
    check_result("entry F369 controls allocation before seam mutation",
                 s_alloc_calls == 1);
    check_result("terrain seam changed F369 to FF", U8(F369) == 0xFFu);
    check_result("mode4 fresh F369 read suppresses F8E6 store",
                 U8(F8E6) == 0x6Du);
    check_result("mode4 fields still execute",
                 U16(slot + SLOT_CONTROL) == 3u &&
                 U16(slot + SLOT_FLAG) == 1u);
    check_result("present-to-FF exact dependency order",
                 call_order_matches(&expected));
    check_result("present-to-FF write-set canary",
                 only_authorized_ram_changed(&expected));

    tc.mode = 7u;
    tc.f369 = 0xFFu;
    tc.f8e6 = 0x6Du;
    snprintf(s_case_name, sizeof(s_case_name), "F369-reread-FF-to-present");
    prepare_case(&tc, DIRECT_SLOT_INDEX);
    expected = oracle_for(&tc);
    s_mutate_f369_on_first_terrain = 1;
    s_first_terrain_f369 = 0x0Au;
    result = wm_8008B2BC((s32)DIRECT_SLOT_INDEX);

    check_result("entry FF controls return before seam mutation", result == 3);
    check_result("entry FF suppresses allocation before seam mutation",
                 s_alloc_calls == 0);
    check_result("terrain seam changed F369 to present", U8(F369) == 0x0Au);
    check_result("mode7 fresh F369 read enables F8E6 store", U8(F8E6) == 1u);
    check_result("mode7 fields still execute",
                 U16(slot + SLOT_CONTROL) == 2u &&
                 U16(slot + SLOT_FLAG) == 1u);
    check_result("suppressed object pointer remains sentinel",
                 U32(slot + SLOT_OBJECT) == SLOT_SENTINEL_PTR);
    check_result("FF-to-present exact dependency order",
                 call_order_matches(&expected));
    check_result("FF-to-present write-set canary",
                 only_authorized_ram_changed(&expected));
}

/* -------------------------- scheduler integration ----------------------- */

static void prepare_scheduler_globals(u8 f369)
{
    memset(g_PsxRam, 0, PSX_RAM_SIZE);
    memset(s_object, 0xD6, sizeof(s_object));
    reset_instrumentation();
    U32(BE24) = POOL_ADDR;
    U32(CD38) = PACKAGE_ADDR;
    U32(MODE) = 8u;
    U16(SEED_X) = 0x8001u;
    U16(SEED_Z) = 0xFFFFu;
    U16(SEED_VALUE) = INITIAL_VALUE;
    U32(C584) = 0xA55A2468u;
    U32(RUNTIME_X) = 0xFFF00000u;
    U32(RUNTIME_Z) = 0x80000000u;
    U8(F8E6) = 0u;
    U8(F369) = f369;
    s_terrain_returns[0] = 0x11223344u;
    s_terrain_returns[1] = 0xF2345678u;
}

static void run_scheduler_integration(void)
{
    u32 slot2 = POOL_ADDR + 0x100u;
    u32 slot3 = POOL_ADDR + 0x180u;

    snprintf(s_case_name, sizeof(s_case_name), "scheduler-present");
    prepare_scheduler_globals(0x0Au);
    s_current_slot = slot2;
    U16(slot2 + SLOT_STATE) = 0u;
    U32(slot2 + SLOT_CB0) = 0x8008B2BCu;
    U32(slot2 + SLOT_CB1) = 0x8008B644u;
    U16(slot2 + SLOT_CONTROL) = 0xBEEFu;
    U32(slot2 + SLOT_AUX) = 0x11223344u;
    U32(slot2 + SLOT_OBJECT) = SLOT_SENTINEL_PTR;
    U16(slot3 + SLOT_STATE) = 0u;
    U32(slot3 + SLOT_CB0) = 0x81234567u;
    U32(slot3 + SLOT_CB1) = 0x81234567u;

    wm_sched_callback_registry_clear();
    wm_sched_callback_register(0x81234567u, later_callback);
    wm_sched_reset();
    wm_80097800();

    check_result("scheduler resolves production B2BC",
                 wm_sched_get_callbacks_executed() == 2);
    check_result("scheduler stores cb0 return state 1",
                 U16(slot2 + SLOT_STATE) == 1u);
    check_result("scheduler continues to slot3 in same pass",
                 s_later_callback_calls == 1 && s_later_callback_arg == 3);
    check_result("scheduler pass completes",
                 wm_sched_get_outcome() == WM_SCHED_PASS_COMPLETE);

    wm_sched_reset();
    wm_80097800();
    check_result("B644 remains recognized unresolved",
                 wm_sched_get_outcome() == WM_SCHED_STOP_MISSING_CALLBACK &&
                 wm_sched_get_frontier_pc() == 0x8008B644u);
    check_result("missing cb1 cannot fabricate state",
                 U16(slot2 + SLOT_STATE) == 1u);

    snprintf(s_case_name, sizeof(s_case_name), "scheduler-suppressed");
    prepare_scheduler_globals(0xFFu);
    s_current_slot = slot2;
    U16(slot2 + SLOT_STATE) = 0u;
    U32(slot2 + SLOT_CB0) = 0x8008B2BCu;
    U32(slot2 + SLOT_CB1) = 0x8008B644u;
    U16(slot2 + SLOT_CONTROL) = 0xBEEFu;
    U32(slot2 + SLOT_AUX) = 0x11223344u;
    U32(slot2 + SLOT_OBJECT) = SLOT_SENTINEL_PTR;
    U16(slot3 + SLOT_STATE) = 0u;
    U32(slot3 + SLOT_CB0) = 0x81234567u;
    U32(slot3 + SLOT_CB1) = 0x81234567u;
    wm_sched_callback_registry_clear();
    wm_sched_callback_register(0x81234567u, later_callback);
    wm_sched_reset();
    wm_80097800();
    check_result("scheduler stores suppressed return state 3",
                 U16(slot2 + SLOT_STATE) == 3u);
    check_result("suppressed cb0 still continues same pass",
                 s_later_callback_calls == 1 && s_later_callback_arg == 3);
    check_result("suppressed path has no object allocation",
                 s_alloc_calls == 0 && U32(slot2 + SLOT_OBJECT) == SLOT_SENTINEL_PTR);

    snprintf(s_case_name, sizeof(s_case_name), "resolver-boundaries");
    prepare_scheduler_globals(0u);
    U16(slot3 + SLOT_STATE) = 0u;
    U32(slot3 + SLOT_CB0) = 0x8008BB40u;
    U32(slot3 + SLOT_CB1) = 0x8008B644u;
    wm_sched_callback_registry_clear();
    wm_sched_reset();
    wm_80097800();
    check_result("BB40 remains next same-pass missing frontier",
                 wm_sched_get_outcome() == WM_SCHED_STOP_MISSING_CALLBACK &&
                 wm_sched_get_frontier_pc() == 0x8008BB40u);

    prepare_scheduler_globals(0u);
    U16(POOL_ADDR + SLOT_STATE) = 0u;
    U32(POOL_ADDR + SLOT_CB0) = 0x8008B498u;
    U32(POOL_ADDR + SLOT_CB1) = 0x8008B498u;
    wm_sched_callback_registry_clear();
    wm_sched_reset();
    wm_80097800();
    check_result("B498 remains separate invalid boundary",
                 wm_sched_get_outcome() == WM_SCHED_STOP_INVALID_CALLBACK &&
                 wm_sched_get_frontier_pc() == 0x8008B498u);

    prepare_scheduler_globals(0u);
    U16(POOL_ADDR + SLOT_STATE) = 0u;
    U32(POOL_ADDR + SLOT_CB0) = 0x8008B54Cu;
    U32(POOL_ADDR + SLOT_CB1) = 0x8008B54Cu;
    wm_sched_callback_registry_clear();
    wm_sched_reset();
    wm_80097800();
    check_result("B54C remains separate invalid boundary",
                 wm_sched_get_outcome() == WM_SCHED_STOP_INVALID_CALLBACK &&
                 wm_sched_get_frontier_pc() == 0x8008B54Cu);

    prepare_scheduler_globals(0u);
    U16(POOL_ADDR + SLOT_STATE) = 0u;
    U32(POOL_ADDR + SLOT_CB0) = 0x8008A72Cu;
    U32(POOL_ADDR + SLOT_CB1) = 0x8008A72Cu;
    wm_sched_callback_registry_clear();
    wm_sched_reset();
    wm_80097800();
    check_result("A72C remains recognized missing",
                 wm_sched_get_outcome() == WM_SCHED_STOP_MISSING_CALLBACK &&
                 wm_sched_get_frontier_pc() == 0x8008A72Cu);
}

int main(void)
{
    printf("W34B5-T wm_8008B2BC production-linked test\n");
    PsxMemory_Init();

    snprintf(s_case_name, sizeof(s_case_name), "pointer-preflight");
    check_result("controlled object pointer fits u32",
                 (uintptr_t)(void*)s_object <= (uintptr_t)UINT32_MAX);
    check_result("controlled object u32 round trip",
                 (void*)(uintptr_t)object_bits() == (void*)s_object);
    check_result("reload scale target fits u32",
                 (uintptr_t)(void*)s_reload_scale_object <=
                 (uintptr_t)UINT32_MAX);
    check_result("reload final target fits u32",
                 (uintptr_t)(void*)s_reload_final_object <=
                 (uintptr_t)UINT32_MAX);
    check_result("reload targets are three distinct objects",
                 &s_object[0] != &s_reload_scale_object[0] &&
                 &s_object[0] != &s_reload_final_object[0] &&
                 &s_reload_scale_object[0] != &s_reload_final_object[0]);
    check_result("reload scale target u32 round trip",
                 (void*)(uintptr_t)(u32)(uintptr_t)(void*)s_reload_scale_object ==
                 (void*)s_reload_scale_object);
    check_result("reload final target u32 round trip",
                 (void*)(uintptr_t)(u32)(uintptr_t)(void*)s_reload_final_object ==
                 (void*)s_reload_final_object);

    run_direct_matrix();
    run_object_reload_observability();
    run_f369_reread_observability();
    run_scheduler_integration();

    printf("RESULT: %d/%d PASS", s_pass_count, s_total_count);
    if (s_failure_count != 0)
        printf(" (%d FAIL)", s_failure_count);
    printf("\n");
    return s_failure_count == 0 ? 0 : 1;
}
