/*
 * W34B5-W production-linked focused test for world callback 0x8008BB40.
 *
 * The expected-state model below is derived from the retail callback CFG,
 * jump table, and write inventory.  It never calls a production replica as
 * an oracle.  Only the callback's four native dependencies are replaced by
 * controlled observation seams.
 *
 * Example build (run from the repository root):
 *
 *   gcc -std=c11 -O0 -g -DXENO_PC_PORT -fno-pie -no-pie \
 *     -Iinclude -Ipc_port/src \
 *     pc_port/tests/w34b5w_8008bb40_prod_test.c \
 *     pc_port/src/world_map_callback_8bb40.c \
 *     pc_port/src/world_map_scheduler.c pc_port/src/psx_memory.c \
 *     -o scratchpad/w34b5w_8bb40_implementation/w34b5w_o0
 *
 * Replace -O0 with -O2 for the optimized run.  For the UB gate add:
 *
 *   -fsanitize=undefined -fno-sanitize-recover=undefined
 */
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_8bb40.h"
#include "world_map_scheduler.h"

#define U8(a)  (*(u8*)PSX_ADDR(a))
#define U16(a) (*(u16*)PSX_ADDR(a))
#define U32(a) (*(u32*)PSX_ADDR(a))

#define POOL_ADDR       0x80020000u
#define PACKAGE_ADDR    0x80030000u

#define BE24            0x8009BE24u
#define CD3C            0x8009CD3Cu
#define MODE            0x8009BE10u
#define SEED_X          0x8006EE54u
#define SEED_Z          0x8006EE56u
#define SEED_VALUE      0x8006EE58u
#define C584            0x8009C584u
#define RUNTIME_X       0x8009C5ACu
#define RUNTIME_Z       0x8009C5B4u
#define F8E7            0x8006F8E7u
#define F36A            0x8006F36Au

#define SLOT_STATE      0x00u
#define SLOT_TIMER      0x02u
#define SLOT_HEADER4    0x04u
#define SLOT_HEADER6    0x06u
#define SLOT_CB0        0x18u
#define SLOT_CB1        0x1Cu
#define SLOT_CONTROL    0x20u
#define SLOT_HEADER22   0x22u
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
#define INITIAL_C584      0xA55A2468u
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

/* Retail jump table at 0x80070778, expressed as independently decoded
 * semantic route families rather than production control flow. */
static const enum route_family s_retail_routes[13] = {
    ROUTE_MODES_1_3, ROUTE_MODES_1_3, ROUTE_MODES_1_3,
    ROUTE_MODES_4_6, ROUTE_MODES_4_6, ROUTE_MODES_4_6,
    ROUTE_MODE_7,
    ROUTE_COMMON, ROUTE_COMMON, ROUTE_COMMON,
    ROUTE_COMMON, ROUTE_COMMON, ROUTE_COMMON
};

typedef struct test_case {
    u32 mode;
    u8 f36a;
    u8 f8e7;
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
    u8 f8e7;
    int object_created;
    int second_terrain;
    int control_written;
} oracle_result_t;

static int s_pass_count;
static int s_total_count;
static int s_failure_count;
static char s_case_name[112];

/* Word arrays give the object flag access its required native alignment. */
static u32 s_object_words[OBJECT_SIZE / sizeof(u32)];
static u32 s_reload_scale_words[OBJECT_SIZE / sizeof(u32)];
static u32 s_reload_final_words[OBJECT_SIZE / sizeof(u32)];
static u8 s_ram_before[PSX_RAM_SIZE];
static int s_call_log[8];
static int s_call_count;
static int s_alloc_calls;
static int s_anim_calls;
static int s_scale_calls;
static void* s_alloc_package;
static s16 s_alloc_args[5];
static void* s_alloc_return_object;
static u32 s_object_initial_flags;
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
static int s_mutate_f36a_on_first_terrain;
static u8 s_first_terrain_f36a;
static int s_later_callback_calls;
static int s_later_callback_arg;

static u8* object_data(void)
{
    return (u8*)(void*)s_object_words;
}

static u8* reload_scale_data(void)
{
    return (u8*)(void*)s_reload_scale_words;
}

static u8* reload_final_data(void)
{
    return (u8*)(void*)s_reload_final_words;
}

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

static u32 pointer_bits(void* pointer)
{
    return (u32)(uintptr_t)pointer;
}

static void log_call(int call)
{
    if (s_call_count < (int)(sizeof(s_call_log) / sizeof(s_call_log[0])))
        s_call_log[s_call_count] = call;
    s_call_count++;
}

/* ------------------------- controlled dependency seams ---------------- */

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

    if (s_alloc_return_object == (void*)object_data()) {
        memset(object_data(), 0x6B, OBJECT_SIZE);
        *(u32*)(void*)(object_data() + 0x3Cu) = s_object_initial_flags;
    }
    return s_alloc_return_object;
}

void func_800245D8(void* object, s16 animation)
{
    log_call(CALL_ANIM);
    s_anim_calls++;
    s_anim_object = object;
    s_anim_index = animation;
    s_anim_saw_owner_store =
        U32(s_current_slot + SLOT_OBJECT) == pointer_bits(object);
    if (s_retarget_object_reloads)
        U32(s_current_slot + SLOT_OBJECT) =
            pointer_bits((void*)reload_scale_data());
}

void SpriteSetScale(void* object, short scale)
{
    log_call(CALL_SCALE);
    s_scale_calls++;
    s_scale_object = object;
    s_scale_value = scale;
    if (s_retarget_object_reloads)
        U32(s_current_slot + SLOT_OBJECT) =
            pointer_bits((void*)reload_final_data());
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

    if (index == 0 && s_mutate_f36a_on_first_terrain)
        U8(F36A) = s_first_terrain_f36a;

    /* The retail callback loads C584 after the second terrain return. */
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

/* ----------------------------- retail oracle -------------------------- */

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
    out.return_state = tc->f36a == 0xFFu ? 3 : 1;
    out.control = 0xBEEFu;
    out.flag = tc->f36a == 0xFFu ? 1u : 0u;
    out.x = (u32)tc->seed_x << 12;
    out.y = tc->terrain0;
    out.z = (u32)tc->seed_z << 12;
    out.aux = 0x11223344u;
    out.value = INITIAL_VALUE;
    out.signed_value = sign_extend16_bits(INITIAL_VALUE);
    out.history = 30u;
    out.f8e7 = tc->f8e7;
    out.object_created = tc->f36a != 0xFFu;
    out.second_terrain = 0;
    out.control_written = 0;

    switch (out.route) {
    case ROUTE_MODES_1_3:
        if (tc->f8e7 == 0u) {
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
        if (tc->f36a != 0xFFu)
            out.f8e7 = 1u;
        break;
    case ROUTE_MODE_7:
        out.control = 2u;
        out.flag = 1u;
        out.control_written = 1;
        if (tc->f36a != 0xFFu)
            out.f8e7 = 1u;
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
    s_alloc_return_object = (void*)object_data();
    s_object_initial_flags = INITIAL_FLAGS;
    s_anim_object = NULL;
    s_anim_index = (s16)-1;
    s_anim_saw_owner_store = 0;
    s_scale_object = NULL;
    s_scale_value = 0;
    memset(s_terrain_x, 0, sizeof(s_terrain_x));
    memset(s_terrain_z, 0, sizeof(s_terrain_z));
    s_terrain_count = 0;
    s_retarget_object_reloads = 0;
    s_mutate_f36a_on_first_terrain = 0;
    s_first_terrain_f36a = 0u;
    s_later_callback_calls = 0;
    s_later_callback_arg = -1;
}

static void prepare_case(const test_case_t* tc, int slot_index)
{
    u32 slot = POOL_ADDR + ((u32)slot_index << 7);

    memset(g_PsxRam, 0xA5, PSX_RAM_SIZE);
    memset(object_data(), 0xD6, OBJECT_SIZE);
    memset(reload_scale_data(), 0xE7, OBJECT_SIZE);
    memset(reload_final_data(), 0xF8, OBJECT_SIZE);
    reset_instrumentation();

    s_current_slot = slot;
    s_terrain_returns[0] = tc->terrain0;
    s_terrain_returns[1] = tc->terrain1;

    U32(BE24) = POOL_ADDR;
    U32(CD3C) = PACKAGE_ADDR;
    U32(MODE) = tc->mode;
    U16(SEED_X) = tc->seed_x;
    U16(SEED_Z) = tc->seed_z;
    U16(SEED_VALUE) = INITIAL_VALUE;
    U32(C584) = INITIAL_C584;
    U32(RUNTIME_X) = tc->runtime_x;
    U32(RUNTIME_Z) = tc->runtime_z;
    U8(F8E7) = tc->f8e7;
    U8(F36A) = tc->f36a;

    memset(PSX_ADDR(slot), 0xCD, 0x80u);
    U16(slot + SLOT_STATE) = 0x5A5Au;
    U16(slot + SLOT_TIMER) = 0x1357u;
    U16(slot + SLOT_HEADER4) = 0x2468u;
    U16(slot + SLOT_HEADER6) = 0x369Cu;
    U32(slot + SLOT_CB0) = 0x8008BB40u;
    U32(slot + SLOT_CB1) = 0x8008B644u;
    U16(slot + SLOT_CONTROL) = 0xBEEFu;
    U16(slot + SLOT_HEADER22) = 0xD00Du;
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
    if (s_mutate_f36a_on_first_terrain && byte_in_range(offset, F36A, 1u))
        return 1; /* controlled terrain-seam effect */
    if ((expected->route == ROUTE_MODES_4_6 ||
         expected->route == ROUTE_MODE_7) &&
        (expected->object_created ||
         (s_mutate_f36a_on_first_terrain &&
          s_first_terrain_f36a != 0xFFu)) &&
        byte_in_range(offset, F8E7, 1u))
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

static int object_has_fill_and_flags(const u8* object, u8 fill, u32 flags)
{
    size_t i;
    for (i = 0; i < OBJECT_SIZE; i++) {
        u8 expected_byte = fill;
        if (i >= 0x3Cu && i < 0x40u)
            expected_byte =
                (u8)(flags >> ((u32)(i - 0x3Cu) * 8u));
        if (object[i] != expected_byte)
            return 0;
    }
    return 1;
}

static int object_matches_expected(const oracle_result_t* expected)
{
    if (!expected->object_created) {
        size_t i;
        for (i = 0; i < OBJECT_SIZE; i++)
            if (object_data()[i] != 0xD6u)
                return 0;
        return 1;
    }
    return object_has_fill_and_flags(object_data(), 0x6Bu,
                                     INITIAL_FLAGS & 0xFFFFFFFBu);
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
    result = wm_8008BB40((s32)slot_index);

    check_result("return state", result == expected.return_state);
    check_result("callback does not own scheduler state",
                 U16(slot + SLOT_STATE) == 0x5A5Au);
    check_result("scheduler timer untouched",
                 U16(slot + SLOT_TIMER) == 0x1357u);
    check_result("scheduler header +4 untouched",
                 U16(slot + SLOT_HEADER4) == 0x2468u);
    check_result("slot +6 untouched", U16(slot + SLOT_HEADER6) == 0x369Cu);
    check_result("cb0 header untouched",
                 U32(slot + SLOT_CB0) == 0x8008BB40u);
    check_result("cb1 header untouched",
                 U32(slot + SLOT_CB1) == 0x8008B644u);
    check_result("slot +22 untouched",
                 U16(slot + SLOT_HEADER22) == 0xD00Du);

    check_result("exact dependency call order", call_order_matches(&expected));
    check_result("first terrain executes", s_terrain_count >= 1);
    check_result("exact terrain call count",
                 s_terrain_count == (expected.second_terrain ? 2 : 1));
    check_result("seed X zero-extends LHU before shift",
                 (u32)s_terrain_x[0] == ((u32)tc->seed_x << 12));
    check_result("seed Z zero-extends LHU before shift",
                 (u32)s_terrain_z[0] == ((u32)tc->seed_z << 12));
    if (expected.second_terrain) {
        check_result("second terrain X bits",
                     (u32)s_terrain_x[1] == tc->runtime_x);
        check_result("second terrain Z bits",
                     (u32)s_terrain_z[1] == tc->runtime_z);
        check_result("post-terrain C584 reload observed",
                     U32(C584) == RELOADED_C584);
    } else {
        check_result("C584 read-only without second terrain",
                     U32(C584) == INITIAL_C584);
    }

    if (expected.object_created) {
        check_result("allocation called once", s_alloc_calls == 1);
        check_result("package KSEG word resolved to native host",
                     s_alloc_package == PSX_ADDR(PACKAGE_ADDR));
        check_result("allocation arg 1 tex X", s_alloc_args[0] == (s16)0x120);
        check_result("allocation arg 2 tex Y", s_alloc_args[1] == (s16)0x1E0);
        check_result("allocation arg 3 clut X", s_alloc_args[2] == (s16)0x160);
        check_result("allocation arg 4 clut Y", s_alloc_args[3] == (s16)0x100);
        check_result("allocation arg 5", s_alloc_args[4] == (s16)0x40);
        check_result("object published before animation dependency",
                     s_anim_saw_owner_store);
        check_result("animation called once", s_anim_calls == 1);
        check_result("animation receives allocation object",
                     s_anim_object == (void*)object_data());
        check_result("animation index zero", s_anim_index == 0);
        check_result("scale called once", s_scale_calls == 1);
        check_result("scale receives reloaded object",
                     s_scale_object == (void*)object_data());
        check_result("scale is 0x1800", s_scale_value == (short)0x1800);
        check_result("slot stores exact low-native pointer",
                     U32(slot + SLOT_OBJECT) ==
                     pointer_bits((void*)object_data()));
        check_result("low-native pointer round trip",
                     (void*)(uintptr_t)U32(slot + SLOT_OBJECT) ==
                     (void*)object_data());
        check_result("only object flag bit 2 cleared",
                     *(u32*)(void*)(object_data() + 0x3Cu) ==
                     (INITIAL_FLAGS & 0xFFFFFFFBu));
    } else {
        check_result("suppression skips allocation", s_alloc_calls == 0);
        check_result("suppression skips animation", s_anim_calls == 0);
        check_result("suppression skips scale", s_scale_calls == 0);
        check_result("suppression preserves slot object sentinel",
                     U32(slot + SLOT_OBJECT) == SLOT_SENTINEL_PTR);
    }
    check_result("object region canary", object_matches_expected(&expected));

    check_result("slot +20 mode control",
                 U16(slot + SLOT_CONTROL) == expected.control);
    check_result("slot +24 path flag", U16(slot + SLOT_FLAG) == expected.flag);
    check_result("slot +28 X", U32(slot + SLOT_X) == expected.x);
    check_result("slot +2C terrain Y", U32(slot + SLOT_Y) == expected.y);
    check_result("slot +30 Z", U32(slot + SLOT_Z) == expected.z);
    check_result("slot +34 canary preserved", U32(slot + SLOT_AUX) == expected.aux);
    check_result("slot +38 zero", U32(slot + SLOT_CLEAR38) == 0u);
    check_result("slot +3C zero", U32(slot + SLOT_CLEAR3C) == 0u);
    check_result("slot +40 zero", U32(slot + SLOT_CLEAR40) == 0u);
    check_result("slot +48 exact value", U16(slot + SLOT_VALUE) == expected.value);
    check_result("slot +4A literal 8", U16(slot + SLOT_CONST) == 8u);
    check_result("slot +58 literal 30", U32(slot + SLOT_HISTORY) == expected.history);
    check_result("slot +5C signed initial EE58",
                 U32(slot + SLOT_SIGNED) == expected.signed_value);
    if (expected.second_terrain) {
        check_result("+48/+5C intentional divergence",
                     U16(slot + SLOT_VALUE) == 0x7FFEu &&
                     U32(slot + SLOT_SIGNED) == 0xFFFF8001u);
    }

    check_result("F36A read-only", U8(F36A) == tc->f36a);
    check_result("F8E7 exact result", U8(F8E7) == expected.f8e7);
    check_result("mode global read-only", U32(MODE) == tc->mode);
    check_result("seed globals read-only",
                 U16(SEED_X) == tc->seed_x &&
                 U16(SEED_Z) == tc->seed_z &&
                 U16(SEED_VALUE) == INITIAL_VALUE);
    check_result("runtime coordinates read-only",
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

static test_case_t base_case(void)
{
    test_case_t tc;
    tc.mode = 8u;
    tc.f36a = 0x05u;
    tc.f8e7 = 0u;
    tc.seed_x = 0x8001u;
    tc.seed_z = 0xFFFFu;
    tc.runtime_x = 0xFFF00000u;
    tc.runtime_z = 0x80000000u;
    tc.terrain0 = 0x11223344u;
    tc.terrain1 = 0xF2345678u;
    return tc;
}

static void run_direct_matrix(void)
{
    static const u32 modes[] = {
        1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u, 12u, 13u,
        0u, 14u, UINT32_MAX, 0x80000000u
    };
    static const u8 f36a_values[] = {
        0x00u, 0x01u, 0x05u, 0x7Fu, 0xFEu, 0xFFu
    };
    static const u8 f8e7_values[] = { 0x00u, 0x6Du };
    unsigned mode_i;
    unsigned f36a_i;
    unsigned f8e7_i;

    for (mode_i = 0;
         mode_i < (unsigned)(sizeof(modes) / sizeof(modes[0]));
         mode_i++) {
        for (f36a_i = 0;
             f36a_i < (unsigned)(sizeof(f36a_values) /
                                  sizeof(f36a_values[0]));
             f36a_i++) {
            for (f8e7_i = 0;
                 f8e7_i < (unsigned)(sizeof(f8e7_values) /
                                      sizeof(f8e7_values[0]));
                 f8e7_i++) {
                test_case_t tc = base_case();
                tc.mode = modes[mode_i];
                tc.f36a = f36a_values[f36a_i];
                tc.f8e7 = f8e7_values[f8e7_i];

                snprintf(s_case_name, sizeof(s_case_name),
                         "mode=%08x F36A=%02x F8E7=%02x",
                         (unsigned)tc.mode, (unsigned)tc.f36a,
                         (unsigned)tc.f8e7);
                verify_case(&tc, DIRECT_SLOT_INDEX);
            }
        }
    }
}

static void run_unsigned_slot_arithmetic(void)
{
    test_case_t tc = base_case();

    /* Retail shifts the low 32 bits of a0.  For -1 this wraps the slot offset
     * to 0xFFFFFF80 and selects the record immediately before POOL_ADDR.
     * UBSan also makes this a direct tripwire against signed-left-shift C. */
    snprintf(s_case_name, sizeof(s_case_name), "slot-index-u32-wrap-minus-one");
    verify_case(&tc, -1);
}

static void run_null_package_no_recovery(void)
{
    test_case_t tc = base_case();
    u32 slot = POOL_ADDR + ((u32)DIRECT_SLOT_INDEX << 7);
    s32 result;

    /* Retail has no callback-side NULL recovery: a present resource byte
     * still invokes func_80024524 with native NULL when the package word is
     * zero.  The controlled allocation seam makes that observation safe. */
    snprintf(s_case_name, sizeof(s_case_name), "present-with-null-package");
    prepare_case(&tc, DIRECT_SLOT_INDEX);
    U32(CD3C) = 0u;
    result = wm_8008BB40((s32)DIRECT_SLOT_INDEX);

    check_result("null-package case retains present return", result == 1);
    check_result("null-package case still calls allocation once",
                 s_alloc_calls == 1);
    check_result("guest zero is passed as native NULL", s_alloc_package == NULL);
    check_result("null-package case does not invent suppression",
                 U16(slot + SLOT_FLAG) == 0u);
    check_result("null-package case publishes returned object",
                 U32(slot + SLOT_OBJECT) ==
                 pointer_bits((void*)object_data()));
}

static void run_object_reload_observability(void)
{
    test_case_t tc = base_case();
    oracle_result_t expected;
    u32 slot = POOL_ADDR + ((u32)DIRECT_SLOT_INDEX << 7);
    s32 result;

    snprintf(s_case_name, sizeof(s_case_name), "slot+4C-reload-observability");
    prepare_case(&tc, DIRECT_SLOT_INDEX);
    expected = oracle_for(&tc);
    memset(reload_scale_data(), 0xB2, OBJECT_SIZE);
    memset(reload_final_data(), 0xC3, OBJECT_SIZE);
    *(u32*)(void*)(reload_scale_data() + 0x3Cu) = RELOAD_SCALE_FLAGS;
    *(u32*)(void*)(reload_final_data() + 0x3Cu) = RELOAD_FINAL_FLAGS;
    s_retarget_object_reloads = 1;

    result = wm_8008BB40((s32)DIRECT_SLOT_INDEX);

    check_result("reload test returns present state", result == 1);
    check_result("allocation published before animation",
                 s_anim_saw_owner_store &&
                 s_anim_object == (void*)object_data());
    check_result("animation retarget is reloaded for scale",
                 s_scale_object == (void*)reload_scale_data());
    check_result("scale retarget remains published",
                 U32(slot + SLOT_OBJECT) ==
                 pointer_bits((void*)reload_final_data()));
    check_result("final pointer exact round trip",
                 (void*)(uintptr_t)U32(slot + SLOT_OBJECT) ==
                 (void*)reload_final_data());
    check_result("allocation object not stale-cleared",
                 object_has_fill_and_flags(object_data(), 0x6Bu,
                                           INITIAL_FLAGS));
    check_result("scale target not stale-cleared",
                 object_has_fill_and_flags(reload_scale_data(), 0xB2u,
                                           RELOAD_SCALE_FLAGS));
    check_result("second reload directs bit clear to final object",
                 object_has_fill_and_flags(reload_final_data(), 0xC3u,
                                           RELOAD_FINAL_FLAGS & 0xFFFFFFFBu));
    check_result("reload test exact call order", call_order_matches(&expected));
    check_result("reload test does not own scheduler state",
                 U16(slot + SLOT_STATE) == 0x5A5Au);
    check_result("reload seam only changes authorized RAM",
                 only_authorized_ram_changed(&expected));
}

static void run_object_flag_patterns(void)
{
    static const u32 patterns[] = { 0xFFFFFFFFu, 0x50000005u };
    static const u32 expected_flags[] = { 0xFFFFFFFBu, 0x50000001u };
    unsigned i;

    for (i = 0; i < (unsigned)(sizeof(patterns) / sizeof(patterns[0])); i++) {
        test_case_t tc = base_case();
        u32 slot = POOL_ADDR + ((u32)DIRECT_SLOT_INDEX << 7);
        s32 result;

        snprintf(s_case_name, sizeof(s_case_name), "object-flags=%08x",
                 (unsigned)patterns[i]);
        prepare_case(&tc, DIRECT_SLOT_INDEX);
        s_object_initial_flags = patterns[i];
        result = wm_8008BB40((s32)DIRECT_SLOT_INDEX);

        check_result("flag-pattern return", result == 1);
        check_result("flag-pattern only clears bit 2",
                     *(u32*)(void*)(object_data() + 0x3Cu) ==
                     expected_flags[i]);
        check_result("flag-pattern preserves every unrelated object byte",
                     object_has_fill_and_flags(object_data(), 0x6Bu,
                                               expected_flags[i]));
        check_result("flag-pattern slot pointer exact",
                     U32(slot + SLOT_OBJECT) ==
                     pointer_bits((void*)object_data()));
    }
}

static void run_f36a_reread_observability(void)
{
    test_case_t tc = base_case();
    oracle_result_t expected;
    u32 slot = POOL_ADDR + ((u32)DIRECT_SLOT_INDEX << 7);
    s32 result;

    tc.mode = 4u;
    tc.f36a = 0x05u;
    tc.f8e7 = 0x6Du;
    snprintf(s_case_name, sizeof(s_case_name), "F36A-present-to-FF-reread");
    prepare_case(&tc, DIRECT_SLOT_INDEX);
    expected = oracle_for(&tc);
    s_mutate_f36a_on_first_terrain = 1;
    s_first_terrain_f36a = 0xFFu;
    result = wm_8008BB40((s32)DIRECT_SLOT_INDEX);

    check_result("entry present byte fixes return before mutation", result == 1);
    check_result("entry present byte allocates before mutation", s_alloc_calls == 1);
    check_result("terrain seam changed F36A to FF", U8(F36A) == 0xFFu);
    check_result("mode4 fresh FF suppresses F8E7 store", U8(F8E7) == 0x6Du);
    check_result("mode4 writes control and flag",
                 U16(slot + SLOT_CONTROL) == 3u &&
                 U16(slot + SLOT_FLAG) == 1u);
    check_result("present-to-FF exact call order", call_order_matches(&expected));
    check_result("present-to-FF write set",
                 only_authorized_ram_changed(&expected));

    tc.mode = 7u;
    tc.f36a = 0xFFu;
    tc.f8e7 = 0x6Du;
    snprintf(s_case_name, sizeof(s_case_name), "F36A-FF-to-present-reread");
    prepare_case(&tc, DIRECT_SLOT_INDEX);
    expected = oracle_for(&tc);
    s_mutate_f36a_on_first_terrain = 1;
    s_first_terrain_f36a = 0x05u;
    result = wm_8008BB40((s32)DIRECT_SLOT_INDEX);

    check_result("entry FF fixes return before mutation", result == 3);
    check_result("entry FF suppresses allocation before mutation",
                 s_alloc_calls == 0 && s_anim_calls == 0 && s_scale_calls == 0);
    check_result("terrain seam changed F36A to present", U8(F36A) == 0x05u);
    check_result("mode7 fresh present byte sets F8E7", U8(F8E7) == 1u);
    check_result("mode7 writes control and flag",
                 U16(slot + SLOT_CONTROL) == 2u &&
                 U16(slot + SLOT_FLAG) == 1u);
    check_result("suppression preserves object sentinel",
                 U32(slot + SLOT_OBJECT) == SLOT_SENTINEL_PTR);
    check_result("FF-to-present exact call order", call_order_matches(&expected));
    check_result("FF-to-present write set",
                 only_authorized_ram_changed(&expected));
}

static void run_c584_before_y_store_observability(void)
{
    test_case_t tc = base_case();
    oracle_result_t expected;
    u32 alias_pool = C584 - ((u32)DIRECT_SLOT_INDEX << 7) - SLOT_Y;
    u32 alias_slot = alias_pool + ((u32)DIRECT_SLOT_INDEX << 7);
    s32 result;

    /* Force slot+0x2C to alias the fixed C584 source word.  The terrain seam
     * changes C584 to 0x5AA57FFE immediately before returning 0xF2345678.
     * Retail loads C584 first, then stores returned Y to slot+0x2C.  Thus
     * +0x48 must receive 0x7FFE even though C584/Y finishes as 0xF2345678.
     * Reversing those two retail operations would incorrectly yield 0x5678. */
    tc.mode = 1u;
    tc.f8e7 = 0u;
    snprintf(s_case_name, sizeof(s_case_name), "C584-before-Y-store-alias");
    prepare_case(&tc, DIRECT_SLOT_INDEX);
    expected = oracle_for(&tc);

    memset(PSX_ADDR(alias_slot), 0xCD, 0x80u);
    U32(BE24) = alias_pool;
    s_current_slot = alias_slot;
    U16(alias_slot + SLOT_STATE) = 0x5A5Au;
    U32(alias_slot + SLOT_CB0) = 0x8008BB40u;
    U32(alias_slot + SLOT_CB1) = 0x8008B644u;
    U16(alias_slot + SLOT_CONTROL) = 0xBEEFu;
    U32(alias_slot + SLOT_AUX) = 0x11223344u;
    U32(alias_slot + SLOT_OBJECT) = SLOT_SENTINEL_PTR;
    U32(C584) = INITIAL_C584;
    U32(RUNTIME_X) = tc.runtime_x;
    U32(RUNTIME_Z) = tc.runtime_z;

    result = wm_8008BB40((s32)DIRECT_SLOT_INDEX);

    check_result("alias construction maps slot Y exactly onto C584",
                 alias_slot + SLOT_Y == C584);
    check_result("alias order test returns present state", result == 1);
    check_result("alias order test takes second terrain", s_terrain_count == 2);
    check_result("alias order test exact call order", call_order_matches(&expected));
    check_result("second terrain uses replacement X",
                 (u32)s_terrain_x[1] == tc.runtime_x);
    check_result("overlapped runtime Z observes earlier signed-state store",
                 (u32)s_terrain_z[1] == 0xFFFF8001u);
    check_result("returned Y overwrites aliased C584 only after its load",
                 U32(alias_slot + SLOT_Y) == tc.terrain1 &&
                 U32(C584) == tc.terrain1);
    check_result("+48 retains pre-Y C584 low16",
                 U16(alias_slot + SLOT_VALUE) == 0x7FFEu &&
                 U16(alias_slot + SLOT_VALUE) != (u16)tc.terrain1);
    check_result("+5C still derives from initial EE58",
                 U32(alias_slot + SLOT_SIGNED) == 0xFFFF8001u);
}

static void run_high_pointer_policy(void)
{
#if UINTPTR_MAX > UINT32_MAX
    test_case_t tc = base_case();
    pid_t child;
    int status = 0;

    snprintf(s_case_name, sizeof(s_case_name), "unrepresentable-pointer-policy");
    prepare_case(&tc, DIRECT_SLOT_INDEX);
    child = fork();
    if (child == (pid_t)0) {
        struct rlimit no_core;
        no_core.rlim_cur = 0;
        no_core.rlim_max = 0;
        (void)setrlimit(RLIMIT_CORE, &no_core);
        s_alloc_return_object =
            (void*)((uintptr_t)UINT32_MAX + (uintptr_t)1u);
        (void)wm_8008BB40((s32)DIRECT_SLOT_INDEX);
        _exit(99);
    }
    check_result("fork for high-pointer probe succeeds", child > (pid_t)0);
    if (child > (pid_t)0) {
        pid_t waited = waitpid(child, &status, 0);
        check_result("high-pointer probe child reaped", waited == child);
        check_result("unrepresentable object pointer aborts",
                     WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
    }
#else
    snprintf(s_case_name, sizeof(s_case_name), "unrepresentable-pointer-policy");
    check_result("32-bit host has no representable high-pointer probe", 1);
#endif
}

/* -------------------------- scheduler integration --------------------- */

static void prepare_scheduler_globals(u8 f36a, u32 mode)
{
    memset(g_PsxRam, 0, PSX_RAM_SIZE);
    memset(object_data(), 0xD6, OBJECT_SIZE);
    reset_instrumentation();
    U32(BE24) = POOL_ADDR;
    U32(CD3C) = f36a == 0xFFu ? 0u : PACKAGE_ADDR;
    U32(MODE) = mode;
    U16(SEED_X) = 0u;
    U16(SEED_Z) = 0u;
    U16(SEED_VALUE) = INITIAL_VALUE;
    U32(C584) = INITIAL_C584;
    U32(RUNTIME_X) = 0u;
    U32(RUNTIME_Z) = 0u;
    U8(F8E7) = 0u;
    U8(F36A) = f36a;
    s_terrain_returns[0] = 0x11223344u;
    s_terrain_returns[1] = 0xF2345678u;
}

static void seed_scheduler_slot(u32 slot, u16 state, u32 cb0, u32 cb1)
{
    U16(slot + SLOT_STATE) = state;
    U32(slot + SLOT_CB0) = cb0;
    U32(slot + SLOT_CB1) = cb1;
}

static void run_scheduler_integration(void)
{
    u32 slot3 = POOL_ADDR + ((u32)3u << 7);
    u32 slot4 = POOL_ADDR + ((u32)4u << 7);

    snprintf(s_case_name, sizeof(s_case_name), "scheduler-present");
    prepare_scheduler_globals(0x05u, 8u);
    s_current_slot = slot3;
    seed_scheduler_slot(slot3, 0u, 0x8008BB40u, 0x8008B644u);
    U16(slot3 + SLOT_CONTROL) = 0xBEEFu;
    U32(slot3 + SLOT_AUX) = 0x11223344u;
    U32(slot3 + SLOT_OBJECT) = SLOT_SENTINEL_PTR;
    seed_scheduler_slot(slot4, 0u, 0x81234567u, 0x81234567u);

    wm_sched_callback_registry_clear();
    wm_sched_callback_register(0x81234567u, later_callback);
    wm_sched_reset();
    wm_80097800();

    check_result("scheduler resolves only production BB40 mapping",
                 wm_sched_get_callbacks_executed() == 2);
    check_result("scheduler stores present return state 1",
                 U16(slot3 + SLOT_STATE) == 1u);
    check_result("scheduler continues to slot4 in same pass",
                 s_later_callback_calls == 1 && s_later_callback_arg == 4);
    check_result("present scheduler pass completes",
                 wm_sched_get_outcome() == WM_SCHED_PASS_COMPLETE);

    wm_sched_reset();
    wm_80097800();
    check_result("shared B644 remains unresolved",
                 wm_sched_get_outcome() == WM_SCHED_STOP_MISSING_CALLBACK &&
                 wm_sched_get_frontier_pc() == 0x8008B644u);
    check_result("missing B644 cannot fabricate state",
                 U16(slot3 + SLOT_STATE) == 1u);

    /* Accepted natural slot-3 prestate: mode 1, F36A FF, package zero.
     * Retail suppression returns 3.  The production scheduler owns that
     * state store and must then encounter slot-4 C530 in the same pass. */
    snprintf(s_case_name, sizeof(s_case_name), "natural-suppressed-frontier");
    prepare_scheduler_globals(0xFFu, 1u);
    s_current_slot = slot3;
    seed_scheduler_slot(slot3, 0u, 0x8008BB40u, 0x8008B644u);
    U32(slot3 + SLOT_OBJECT) = SLOT_SENTINEL_PTR;
    seed_scheduler_slot(slot4, 0u, 0x8008C530u, 0x8008C844u);
    wm_sched_callback_registry_clear();
    wm_sched_reset();
    wm_80097800();

    check_result("natural BB40 executes exactly once",
                 wm_sched_get_callbacks_executed() == 1);
    check_result("natural suppressed return stored as slot3 state 3",
                 U16(slot3 + SLOT_STATE) == 3u);
    check_result("natural suppression has zero object helper calls",
                 s_alloc_calls == 0 && s_anim_calls == 0 && s_scale_calls == 0);
    check_result("natural suppression preserves slot object word",
                 U32(slot3 + SLOT_OBJECT) == SLOT_SENTINEL_PTR);
    check_result("natural mode1 still takes both terrain samples",
                 s_terrain_count == 2);
    check_result("natural state remains coherent FF/null-package",
                 U8(F36A) == 0xFFu && U32(CD3C) == 0u);
    check_result("scheduler naturally advances to slot4 C530 frontier",
                 wm_sched_get_outcome() == WM_SCHED_STOP_MISSING_CALLBACK &&
                 wm_sched_get_frontier_pc() == 0x8008C530u &&
                 wm_sched_get_last_slot() == 4);
    check_result("unresolved C530 cannot mutate slot4 state",
                 U16(slot4 + SLOT_STATE) == 0u);
    check_result("natural pass made two dispatch attempts",
                 wm_sched_get_dispatch_attempts() == 2);

    snprintf(s_case_name, sizeof(s_case_name), "resolver-exclusions");
    prepare_scheduler_globals(0xFFu, 8u);
    seed_scheduler_slot(slot4, 1u, 0x8008C530u, 0x8008C844u);
    wm_sched_callback_registry_clear();
    wm_sched_reset();
    wm_80097800();
    check_result("C844 remains recognized unresolved",
                 wm_sched_get_outcome() == WM_SCHED_STOP_MISSING_CALLBACK &&
                 wm_sched_get_frontier_pc() == 0x8008C844u);

    prepare_scheduler_globals(0xFFu, 8u);
    seed_scheduler_slot(slot3, 0u, 0x8008BD1Cu, 0x8008BD1Cu);
    wm_sched_callback_registry_clear();
    wm_sched_reset();
    wm_80097800();
    check_result("next physical BD1C is not absorbed or resolved",
                 wm_sched_get_outcome() == WM_SCHED_STOP_INVALID_CALLBACK &&
                 wm_sched_get_frontier_pc() == 0x8008BD1Cu);
}

int main(void)
{
    printf("W34B5-W wm_8008BB40 production-linked test\n");
    PsxMemory_Init();

    snprintf(s_case_name, sizeof(s_case_name), "pointer-preflight");
    check_result("controlled object pointer fits u32",
                 (uintptr_t)(void*)object_data() <= (uintptr_t)UINT32_MAX);
    check_result("controlled object exact u32 round trip",
                 (void*)(uintptr_t)pointer_bits((void*)object_data()) ==
                 (void*)object_data());
    check_result("scale-reload target fits u32",
                 (uintptr_t)(void*)reload_scale_data() <=
                 (uintptr_t)UINT32_MAX);
    check_result("final-reload target fits u32",
                 (uintptr_t)(void*)reload_final_data() <=
                 (uintptr_t)UINT32_MAX);
    check_result("three controlled objects are distinct",
                 object_data() != reload_scale_data() &&
                 object_data() != reload_final_data() &&
                 reload_scale_data() != reload_final_data());
    check_result("scale-reload target exact round trip",
                 (void*)(uintptr_t)pointer_bits((void*)reload_scale_data()) ==
                 (void*)reload_scale_data());
    check_result("final-reload target exact round trip",
                 (void*)(uintptr_t)pointer_bits((void*)reload_final_data()) ==
                 (void*)reload_final_data());

    run_direct_matrix();
    run_unsigned_slot_arithmetic();
    run_null_package_no_recovery();
    run_object_reload_observability();
    run_object_flag_patterns();
    run_f36a_reread_observability();
    run_c584_before_y_store_observability();
    run_high_pointer_policy();
    run_scheduler_integration();

    printf("RESULT: %d/%d PASS", s_pass_count, s_total_count);
    if (s_failure_count != 0)
        printf(" (%d FAIL)", s_failure_count);
    printf("\n");
    return s_failure_count == 0 ? 0 : 1;
}
