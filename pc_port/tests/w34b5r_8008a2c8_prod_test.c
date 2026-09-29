/* Production-linked focused test for world callback 0x8008A2C8. */
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_8a2c8.h"
#include "world_map_scheduler.h"

#define U8(a)  (*(u8*)PSX_ADDR(a))
#define U16(a) (*(u16*)PSX_ADDR(a))
#define U32(a) (*(u32*)PSX_ADDR(a))

#define POOL_ADDR       0x80020000u
#define PACKAGE_ADDR    0x80030000u
#define BE24            0x8009BE24u
#define CD34            0x8009CD34u
#define MODE            0x8009BE10u
#define SEED_X          0x8006EF64u
#define SEED_Z          0x8006EF66u
#define C584            0x8009C584u
#define RUNTIME_X       0x8009C5ACu
#define RUNTIME_Z       0x8009C5B4u
#define F8E5            0x8006F8E5u
#define F368            0x8006F368u
#define D55C            0x8009D55Cu
#define D52C            0x8009D52Cu
#define D154            0x8009D154u
#define POSE            0x8009CEC4u
#define EE54            0x8006EE54u
#define EE56            0x8006EE56u
#define EE58            0x8006EE58u

#define SLOT_STATE      0x00u
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
#define SLOT_SIGNED     0x5Cu

enum {
    CALL_ALLOC = 1,
    CALL_ANIM,
    CALL_SCALE,
    CALL_TERRAIN
};

typedef struct test_case {
    const char* name;
    u32 mode;
    u8 flag;
    u8 control;
    u16 seed_x;
    u16 seed_z;
    u32 runtime_x;
    u32 runtime_z;
    u32 state_initial;
    u32 state_after_second;
    int change_state_on_second;
    u32 terrain0;
    u32 terrain1;
    u16 expected_control;
    int expect_control_write;
    u16 expected_flag;
    u8 expected_f8e5;
    int expect_second;
    int expect_publication;
} test_case_t;

typedef struct oracle_result {
    u32 x;
    u32 y;
    u32 z;
    u32 aux;
    u16 value;
    u32 signed_value;
    u16 control;
    u16 flag;
    u8 f8e5;
    int terrain_calls;
    int publication;
} oracle_result_t;

static int pass_count;
static int total_count;
static int failure_count;

static u32 s_object_words[0x80];
static u32 s_object_initial_flags;
static int s_call_log[8];
static int s_call_count;
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
static int s_change_state_on_second;
static u32 s_state_after_second;
static u32 s_current_slot;
static int s_later_callback_calls;
static int s_later_callback_arg;

static void check_result(const char* name, int condition)
{
    total_count++;
    if (condition) {
        pass_count++;
    } else {
        failure_count++;
        printf("FAIL: %s\n", name);
    }
}

static s32 bits_to_s32(u32 bits)
{
    s32 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static u32 sra12(u32 bits)
{
    u32 value = bits >> 12;
    if ((bits & 0x80000000u) != 0u)
        value |= 0xFFF00000u;
    return value;
}

static u32 object_bits(void)
{
    return (u32)(uintptr_t)(void*)s_object_words;
}

void* func_80024524(void* package, s16 tex_x, s16 tex_y,
                    s16 clut_x, s16 clut_y, s16 arg5)
{
    u8* object = (u8*)s_object_words;
    s_call_log[s_call_count++] = CALL_ALLOC;
    s_alloc_package = package;
    s_alloc_args[0] = tex_x;
    s_alloc_args[1] = tex_y;
    s_alloc_args[2] = clut_x;
    s_alloc_args[3] = clut_y;
    s_alloc_args[4] = arg5;
    memset(object, 0x6B, sizeof(s_object_words));
    *(u32*)(object + 0x3Cu) = s_object_initial_flags;
    return object;
}

void func_800245D8(void* object, s16 animation)
{
    s_call_log[s_call_count++] = CALL_ANIM;
    s_anim_object = object;
    s_anim_index = animation;
    s_anim_saw_owner_store =
        U32(s_current_slot + SLOT_OBJECT) == (u32)(uintptr_t)object;
}

void SpriteSetScale(void* object, short scale)
{
    s_call_log[s_call_count++] = CALL_SCALE;
    s_scale_object = object;
    s_scale_value = scale;
    *(u32*)((u8*)object + 0x3Cu) |= 0x10000000u;
}

s32 wm_80093978(s32 x, s32 z)
{
    int index = s_terrain_count;
    s_call_log[s_call_count++] = CALL_TERRAIN;
    if (index < 2) {
        s_terrain_x[index] = x;
        s_terrain_z[index] = z;
    }
    s_terrain_count++;
    if (index == 1 && s_change_state_on_second)
        U32(C584) = s_state_after_second;
    return bits_to_s32(s_terrain_returns[index < 2 ? index : 1]);
}

static s16 later_callback(int slot_index)
{
    s_later_callback_calls++;
    s_later_callback_arg = slot_index;
    return 1;
}

static s32 sign_extend16(u16 bits)
{
    return bits <= 0x7FFFu ? (s32)bits : (s32)(u32)bits - 0x10000;
}

static oracle_result_t oracle_for(const test_case_t* tc, u32 aux)
{
    oracle_result_t out;
    u32 index = tc->mode - 1u;

    out.x = (u32)tc->seed_x << 12;
    out.y = tc->terrain0;
    out.z = (u32)tc->seed_z << 12;
    out.aux = aux;
    out.value = (u16)tc->state_initial;
    out.signed_value = (u32)sign_extend16(out.value);
    out.control = 0xBEEFu;
    out.flag = 0u;
    out.f8e5 = tc->flag;
    out.terrain_calls = 1;
    out.publication = 0;

    if (index < 13u) {
        if (index <= 2u) {
            if (tc->flag == 0u) {
                out.x = tc->runtime_x;
                out.y = tc->terrain1;
                out.z = tc->runtime_z;
                out.value = (u16)(tc->change_state_on_second
                    ? tc->state_after_second : tc->state_initial);
                out.terrain_calls = 2;
                out.publication = 1;
            } else {
                out.control = 1u;
                out.flag = 1u;
            }
        } else if (index <= 6u) {
            out.control = index == 6u ? 2u : 3u;
            out.flag = 1u;
            if (tc->control != 0xFFu)
                out.f8e5 = 1u;
        }
    }
    return out;
}

static int slot_offset_authorized(u32 offset)
{
    return (offset >= SLOT_CONTROL && offset < SLOT_CONTROL + 2u) ||
           (offset >= SLOT_FLAG && offset < SLOT_FLAG + 2u) ||
           (offset >= SLOT_X && offset < SLOT_X + 16u) ||
           (offset >= SLOT_CLEAR38 && offset < SLOT_CLEAR40 + 4u) ||
           (offset >= SLOT_VALUE && offset < SLOT_OBJECT + 4u) ||
           (offset >= SLOT_SIGNED && offset < SLOT_SIGNED + 4u);
}

static void prepare_case(const test_case_t* tc, int slot_index)
{
    u32 slot = POOL_ADDR + ((u32)slot_index << 7);
    u8* slot_bytes;

    memset(g_PsxRam, 0xA5, PSX_RAM_SIZE);
    memset(s_call_log, 0, sizeof(s_call_log));
    s_call_count = 0;
    s_terrain_count = 0;
    s_change_state_on_second = tc->change_state_on_second;
    s_state_after_second = tc->state_after_second;
    s_object_initial_flags = 0x02004005u;
    s_anim_saw_owner_store = 0;
    s_current_slot = slot;
    s_terrain_returns[0] = tc->terrain0;
    s_terrain_returns[1] = tc->terrain1;

    U32(BE24) = POOL_ADDR;
    U32(CD34) = PACKAGE_ADDR;
    U32(MODE) = tc->mode;
    U16(SEED_X) = tc->seed_x;
    U16(SEED_Z) = tc->seed_z;
    U32(C584) = tc->state_initial;
    U32(RUNTIME_X) = tc->runtime_x;
    U32(RUNTIME_Z) = tc->runtime_z;
    U8(F8E5) = tc->flag;
    U8(F368) = tc->control;

    slot_bytes = (u8*)PSX_ADDR(slot);
    memset(slot_bytes, 0xCD, 0x80u);
    U16(slot + SLOT_STATE) = 0x5A5Au;
    U16(slot + SLOT_CONTROL) = 0xBEEFu;
    U16(slot + SLOT_FLAG) = 0xCAFEu;
    U32(slot + SLOT_AUX) = 0x11223344u;
    memset(PSX_ADDR(POSE - 16u), 0xCC, 0x2A0u);
    memset(PSX_ADDR(D55C), 0xD7, 16u);
    U16(D52C) = 0xD7D7u;
    U16(D154) = 0xD1D1u;
}

static void verify_case(const test_case_t* tc, int slot_index)
{
    u32 slot = POOL_ADDR + ((u32)slot_index << 7);
    u8 before_slot[0x80];
    oracle_result_t expected;
    s32 result;
    int untouched = 1;
    int object_untouched = 1;
    int pose_ok = 1;
    int padding_ok = 1;
    int i;

    prepare_case(tc, slot_index);
    memcpy(before_slot, PSX_ADDR(slot), sizeof(before_slot));
    expected = oracle_for(tc, 0x11223344u);
    result = wm_8008A2C8((s32)slot_index);

    check_result("return=1", result == 1);
    check_result("callback leaves scheduler state", U16(slot + SLOT_STATE) == 0x5A5Au);
    check_result("slot object pointer", U32(slot + SLOT_OBJECT) == object_bits());
    check_result("pointer round trip",
                 (void*)(uintptr_t)U32(slot + SLOT_OBJECT) == (void*)s_object_words);
    check_result("package resolved to host", s_alloc_package == PSX_ADDR(PACKAGE_ADDR));
    check_result("allocation tex x", s_alloc_args[0] == (s16)0x100);
    check_result("allocation tex y", s_alloc_args[1] == (s16)0x1E0);
    check_result("allocation clut x", s_alloc_args[2] == (s16)0x140);
    check_result("allocation clut y", s_alloc_args[3] == (s16)0x100);
    check_result("allocation arg5", s_alloc_args[4] == (s16)0x40);
    check_result("owner stored before animation", s_anim_saw_owner_store);
    check_result("animation object", s_anim_object == (void*)s_object_words);
    check_result("animation zero", s_anim_index == 0);
    check_result("scale object", s_scale_object == (void*)s_object_words);
    check_result("scale 0x1800", s_scale_value == (short)0x1800);
    check_result("object flags",
                 *(u32*)((u8*)s_object_words + 0x3Cu) ==
                 ((s_object_initial_flags | 0x10000000u) & 0xFFFFFFFBu));

    for (i = 0; i < (int)sizeof(s_object_words); i++) {
        if (i >= 0x3C && i < 0x40)
            continue;
        if (((u8*)s_object_words)[i] != 0x6Bu)
            object_untouched = 0;
    }
    check_result("no unrelated direct object writes", object_untouched);

    check_result("terrain call count", s_terrain_count == expected.terrain_calls);
    check_result("seed terrain x", (u32)s_terrain_x[0] == ((u32)tc->seed_x << 12));
    check_result("seed terrain z", (u32)s_terrain_z[0] == ((u32)tc->seed_z << 12));
    if (expected.terrain_calls == 2) {
        check_result("replacement terrain x", (u32)s_terrain_x[1] == tc->runtime_x);
        check_result("replacement terrain z", (u32)s_terrain_z[1] == tc->runtime_z);
    }

    check_result("slot x", U32(slot + SLOT_X) == expected.x);
    check_result("slot y", U32(slot + SLOT_Y) == expected.y);
    check_result("slot z", U32(slot + SLOT_Z) == expected.z);
    check_result("slot aux preserved", U32(slot + SLOT_AUX) == expected.aux);
    check_result("slot clear38", U32(slot + SLOT_CLEAR38) == 0u);
    check_result("slot clear3c", U32(slot + SLOT_CLEAR3C) == 0u);
    check_result("slot clear40", U32(slot + SLOT_CLEAR40) == 0u);
    check_result("slot value", U16(slot + SLOT_VALUE) == expected.value);
    check_result("slot constant", U16(slot + SLOT_CONST) == 8u);
    check_result("slot signed first value", U32(slot + SLOT_SIGNED) == expected.signed_value);
    check_result("slot control", U16(slot + SLOT_CONTROL) == expected.control);
    check_result("slot flag", U16(slot + SLOT_FLAG) == expected.flag);
    check_result("F8E5 result", U8(F8E5) == expected.f8e5);

    for (i = 0; i < 0x80; i++) {
        if (!slot_offset_authorized((u32)i) &&
            ((u8*)PSX_ADDR(slot))[i] != before_slot[i])
            untouched = 0;
    }
    check_result("slot unauthorized bytes untouched", untouched);

    if (expected.publication) {
        check_result("D55C x", U32(D55C + 0u) == expected.x);
        check_result("D560 y", U32(D55C + 4u) == expected.y);
        check_result("D564 z", U32(D55C + 8u) == expected.z);
        check_result("D568 aux", U32(D55C + 12u) == expected.aux);
        check_result("D52C state", U16(D52C) == expected.value);
    } else {
        check_result("publication untouched",
                     U32(D55C) == 0xD7D7D7D7u && U16(D52C) == 0xD7D7u);
    }

    check_result("D154 clear", U16(D154) == 0u);
    check_result("EE54 arithmetic shift", U16(EE54) == (u16)sra12(expected.x));
    check_result("EE56 arithmetic shift", U16(EE56) == (u16)sra12(expected.z));
    check_result("EE58 value", U16(EE58) == expected.value);

    for (i = 0; i < 32; i++) {
        u32 entry = POSE + (u32)i * 0x14u;
        if (U32(entry + 0u) != expected.x ||
            U32(entry + 4u) != expected.y ||
            U32(entry + 8u) != expected.z ||
            U32(entry + 12u) != expected.aux ||
            U16(entry + 16u) != expected.value)
            pose_ok = 0;
        if (U16(entry + 18u) != 0xCCCCu)
            padding_ok = 0;
    }
    check_result("pose table 32 records", pose_ok);
    check_result("pose table +0x12 padding", padding_ok);
    check_result("pose guards/no record33",
                 U32(POSE - 4u) == 0xCCCCCCCCu &&
                 U32(POSE + 32u * 0x14u) == 0xCCCCCCCCu);

    check_result("call prefix allocate/anim/scale/terrain",
                 s_call_count >= 4 && s_call_log[0] == CALL_ALLOC &&
                 s_call_log[1] == CALL_ANIM && s_call_log[2] == CALL_SCALE &&
                 s_call_log[3] == CALL_TERRAIN);
    if (expected.terrain_calls == 2)
        check_result("second terrain order",
                     s_call_count == 5 && s_call_log[4] == CALL_TERRAIN);
    else
        check_result("no second terrain", s_call_count == 4);

    printf("PASS CASE: %s\n", tc->name);
}

static void run_direct_cases(void)
{
    static const test_case_t cases[] = {
        {"mode1-natural",1u,0u,0x55u,0x8001u,0xFFFFu,0xFFF00000u,0x80000000u,
         0x00018001u,0x00027FFEu,1,0x11112222u,0xF2345678u,0,0,0,0,1,1},
        {"mode1-flag",1u,7u,0x55u,1u,2u,3u,4u,0x8123u,0,0,
         0x101u,0x202u,1,1,1,7u,0,0},
        {"mode2-flag",2u,1u,0x55u,3u,4u,5u,6u,0x7FFFu,0,0,
         0x303u,0x404u,1,1,1,1u,0,0},
        {"mode3-flag",3u,0x80u,0x55u,5u,6u,7u,8u,0x8000u,0,0,
         0x505u,0x606u,1,1,1,0x80u,0,0},
        {"mode4-ff",4u,0x22u,0xFFu,7u,8u,9u,10u,1u,0,0,
         0x707u,0x808u,3,1,1,0x22u,0,0},
        {"mode5-set",5u,0x22u,0u,9u,10u,11u,12u,2u,0,0,
         0x909u,0xA0Au,3,1,1,1u,0,0},
        {"mode6-set",6u,0u,0x7Fu,11u,12u,13u,14u,3u,0,0,
         0xB0Bu,0xC0Cu,3,1,1,1u,0,0},
        {"mode7-ff",7u,0x44u,0xFFu,13u,14u,15u,16u,4u,0,0,
         0xD0Du,0xE0Eu,2,1,1,0x44u,0,0},
        {"mode7-set",7u,0u,1u,15u,16u,17u,18u,5u,0,0,
         0xF0Fu,0x1010u,2,1,1,1u,0,0},
        {"mode8",8u,0x21u,0x55u,17u,18u,19u,20u,6u,0,0,0x1111u,0,0,0,0x21u,0,0},
        {"mode9",9u,0x22u,0x55u,19u,20u,21u,22u,7u,0,0,0x1212u,0,0,0,0x22u,0,0},
        {"mode10",10u,0x23u,0x55u,21u,22u,23u,24u,8u,0,0,0x1313u,0,0,0,0x23u,0,0},
        {"mode11",11u,0x24u,0x55u,23u,24u,25u,26u,9u,0,0,0x1414u,0,0,0,0x24u,0,0},
        {"mode12",12u,0x25u,0x55u,25u,26u,27u,28u,10u,0,0,0x1515u,0,0,0,0x25u,0,0},
        {"mode13",13u,0x26u,0x55u,27u,28u,29u,30u,11u,0,0,0x1616u,0,0,0,0x26u,0,0},
        {"invalid0",0u,0x31u,0x55u,29u,30u,31u,32u,12u,0,0,0x1717u,0,0,0,0x31u,0,0},
        {"invalid14",14u,0x32u,0x55u,31u,32u,33u,34u,13u,0,0,0x1818u,0,0,0,0x32u,0,0},
        {"invalid-max",UINT32_MAX,0x33u,0x55u,33u,34u,35u,36u,14u,0,0,0x1919u,0,0,0,0x33u,0,0},
        {"invalid-high",0x80000000u,0x34u,0x55u,35u,36u,37u,38u,15u,0,0,0x1A1Au,0,0,0,0x34u,0,0}
    };
    unsigned i;
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
        verify_case(&cases[i], 5);
}

static void run_scheduler_integration(void)
{
    test_case_t tc = {"scheduler",8u,0u,0xFFu,1u,2u,3u,4u,
                      0x1234u,0,0,0xABCDEF01u,0,0,0,0,0,0,0};
    u32 slot1 = POOL_ADDR + 0x80u;
    u32 slot2 = POOL_ADDR + 0x100u;

    prepare_case(&tc, 1);
    memset(PSX_ADDR(POOL_ADDR), 0, 0x2000u);
    U32(BE24) = POOL_ADDR;
    U32(CD34) = PACKAGE_ADDR;
    U32(MODE) = 8u;
    U16(SEED_X) = 1u;
    U16(SEED_Z) = 2u;
    U32(C584) = 0x1234u;
    s_terrain_returns[0] = 0xABCDEF01u;
    s_terrain_returns[1] = 0u;
    s_current_slot = slot1;

    U16(slot1 + SLOT_STATE) = 0u;
    U32(slot1 + SLOT_CB0) = 0x8008A2C8u;
    U32(slot1 + SLOT_CB1) = 0x8008A72Cu;
    U16(slot2 + SLOT_STATE) = 0u;
    U32(slot2 + SLOT_CB0) = 0x81234567u;
    U32(slot2 + SLOT_CB1) = 0x81234567u;

    wm_sched_callback_registry_clear();
    wm_sched_callback_register(0x81234567u, later_callback);
    wm_sched_reset();
    s_later_callback_calls = 0;
    s_later_callback_arg = -1;
    wm_80097800();

    check_result("scheduler resolves real 8A2C8", wm_sched_get_callbacks_executed() == 2);
    check_result("scheduler stores slot1 state 1", U16(slot1 + SLOT_STATE) == 1u);
    check_result("scheduler same-pass slot2", s_later_callback_calls == 1);
    check_result("scheduler slot2 argument", s_later_callback_arg == 2);
    check_result("scheduler pass completes", wm_sched_get_outcome() == WM_SCHED_PASS_COMPLETE);

    wm_sched_reset();
    wm_80097800();
    check_result("8A72C remains missing",
                 wm_sched_get_outcome() == WM_SCHED_STOP_MISSING_CALLBACK &&
                 wm_sched_get_frontier_pc() == 0x8008A72Cu);
    check_result("state1 unchanged at missing cb1", U16(slot1 + SLOT_STATE) == 1u);

    memset(PSX_ADDR(POOL_ADDR), 0, 0x2000u);
    U16(POOL_ADDR + SLOT_STATE) = 0u;
    U32(POOL_ADDR + SLOT_CB0) = 0x8008A52Cu;
    U32(POOL_ADDR + SLOT_CB1) = 0x8008A52Cu;
    wm_sched_reset();
    wm_80097800();
    check_result("8A52C remains separate invalid boundary",
                 wm_sched_get_outcome() == WM_SCHED_STOP_INVALID_CALLBACK &&
                 wm_sched_get_frontier_pc() == 0x8008A52Cu);
}

int main(void)
{
    printf("W34B5-R wm_8008A2C8 production-linked test\n");
    PsxMemory_Init();
    check_result("test object is representable in u32",
                 (uintptr_t)(void*)s_object_words <= (uintptr_t)UINT32_MAX);
    check_result("u32 object round trip",
                 (void*)(uintptr_t)object_bits() == (void*)s_object_words);
    run_direct_cases();
    run_scheduler_integration();
    printf("RESULT: %d/%d PASS", pass_count, total_count);
    if (failure_count != 0)
        printf(" (%d FAIL)", failure_count);
    printf("\n");
    return failure_count == 0 ? 0 : 1;
}
