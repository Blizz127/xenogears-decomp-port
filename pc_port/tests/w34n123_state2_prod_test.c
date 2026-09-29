#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_state2_8eb64.h"

#define SLOT          UINT32_C(0x800A1000)
#define CONTEXT       UINT32_C(0x800A2000)
#define OUT           UINT32_C(0x1F800090)
#define ANGLES        UINT32_C(0x1F8000A0)
#define SHORT_POS     UINT32_C(0x1F8000A8)
#define MODE          UINT32_C(0x8009BE10)
#define CONTEXT_PTR   UINT32_C(0x8009C620)
#define MATRIX_Z      UINT32_C(0x8009BD3C)
#define AREA          UINT32_C(0x8009BD60)
#define BOUNDARY      UINT32_C(0x8009D738)
#define TRIGGER       UINT32_C(0x8009BD04)
#define POSITION      UINT32_C(0x8009D55C)
#define HEADING       UINT32_C(0x8009D52C)
#define STOP_FRAME    UINT32_C(0x8009D554)
#define EXIT_CODE     UINT32_C(0x8009D7CC)
#define TRIGGER_PTR   UINT32_C(0x8009D7D8)
#define TRIGGER_A     UINT32_C(0x8009BD24)
#define TRIGGER_B     UINT32_C(0x8009CE68)
#define NATIVE_HEAD   UINT32_C(0x8006EE66)

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];
void *D_80062528 = (void *)(uintptr_t)UINT32_C(0x12345678);

static int s_passes;
static int s_total;
static s32 s_control;
static s32 s_region;
static s32 s_walkable;
static s32 s_angle;
static s32 s_movement;
static s32 s_terrain;
static u8 s_boundary;
static u8 s_area;
static u32 s_movement_out[4];

static int s_control_calls;
static int s_region_calls;
static int s_walkable_calls;
static s32 s_walk_mode;
static s32 s_walk_region;
static int s_angle_calls;
static u32 s_angle_args[3];
static int s_move_calls;
static u32 s_move_args[3];
static s32 s_move_scale;
static s32 s_move_mode;
static int s_query_calls;
static u32 s_query_list;
static int s_selector_calls;
static int s_matrix_calls;
static u32 s_matrix_destinations[2];
static s16 s_matrix_angles[2][3];
static int s_presence_calls;
static u32 s_presence_args[3];
static int s_release_calls;
static u32 s_release_ids[4];
static int s_unload_calls;
static u32 s_unload_arg;
static int s_publish_calls;
static s32 s_publish_tag;
static u32 s_publish_arg;
static int s_claim_calls;
static u32 s_claim_slot;
static s32 s_claim_value;
static int s_sound_calls;
static void *s_sound_manager;
static s32 s_sound_level;
static s32 s_sound_steps;
static int s_proximity_calls;

static void check(const char *name, int condition)
{
    s_total++;
    if (condition) {
        s_passes++;
        printf("PASS [state2]: %s\n", name);
    } else {
        printf("FAIL [state2]: %s\n", name);
    }
}

static void store_u16(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u16 load_u16(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void store_u32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u32 load_u32(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 guest_address(const void *pointer)
{
    uintptr_t offset = (uintptr_t)((const uint8_t *)pointer - g_PsxRam);
    return UINT32_C(0x80000000) | (u32)offset;
}

static void reset_case(void)
{
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0, sizeof(g_PsxScratchpad));
    s_control = 0;
    s_region = 0;
    s_walkable = 0;
    s_angle = -1;
    s_movement = 0;
    s_terrain = 0;
    s_boundary = 0;
    s_area = 0;
    memset(s_movement_out, 0, sizeof(s_movement_out));
    s_control_calls = 0;
    s_region_calls = 0;
    s_walkable_calls = 0;
    s_walk_mode = 0;
    s_walk_region = 0;
    s_angle_calls = 0;
    memset(s_angle_args, 0, sizeof(s_angle_args));
    s_move_calls = 0;
    memset(s_move_args, 0, sizeof(s_move_args));
    s_move_scale = 0;
    s_move_mode = 0;
    s_query_calls = 0;
    s_query_list = 0;
    s_selector_calls = 0;
    s_matrix_calls = 0;
    memset(s_matrix_destinations, 0, sizeof(s_matrix_destinations));
    memset(s_matrix_angles, 0, sizeof(s_matrix_angles));
    s_presence_calls = 0;
    memset(s_presence_args, 0, sizeof(s_presence_args));
    s_release_calls = 0;
    memset(s_release_ids, 0, sizeof(s_release_ids));
    s_unload_calls = 0;
    s_unload_arg = 0;
    s_publish_calls = 0;
    s_publish_tag = 0;
    s_publish_arg = 0;
    s_claim_calls = 0;
    s_claim_slot = 0;
    s_claim_value = 0;
    s_sound_calls = 0;
    s_sound_manager = NULL;
    s_sound_level = 0;
    s_sound_steps = 0;
    s_proximity_calls = 0;

    store_u32(CONTEXT_PTR, CONTEXT);
    store_u16(SLOT + 0x20u, UINT16_C(2));
    store_u32(SLOT + 0x28u, UINT32_C(0x00123000));
    store_u32(SLOT + 0x2Cu, UINT32_C(0x00045000));
    store_u32(SLOT + 0x30u, UINT32_C(0xFFF67000));
    store_u32(SLOT + 0x34u, UINT32_C(0x11112222));
    store_u32(SLOT + 0x38u, UINT32_C(0x00001000));
    store_u32(SLOT + 0x3Cu, UINT32_C(0x00002000));
    store_u32(SLOT + 0x40u, UINT32_C(0xFFFFD000));
    store_u16(SLOT + 0x48u, UINT16_C(0x0456));
    store_u32(SLOT + 0x60u, UINT32_C(0x00001800));
    store_u32(SLOT + 0x70u, UINT32_C(0x00023000));
    store_u32(MODE, UINT32_C(0x00000007));
    store_u16(MATRIX_Z, UINT16_C(0x0789));
    store_u32(STOP_FRAME, UINT32_C(0xAAAAAAAA));
    store_u32(EXIT_CODE, UINT32_C(0xBBBBBBBB));
}

s32 wm_80090FB4(u32 slot)
{
    check("camera control receives slot", slot == SLOT);
    s_control_calls++;
    return s_control;
}

s32 wm_80093F18(u32 vector)
{
    check("terrain selector receives slot position", vector == SLOT + 0x28u);
    s_region_calls++;
    return s_region;
}

s32 wm_80094060(s32 mode, s32 region)
{
    s_walkable_calls++;
    s_walk_mode = mode;
    s_walk_region = region;
    return s_walkable;
}

s32 wm_8008E0F0(u32 pos, u32 unused, u32 out)
{
    s_angle_calls++;
    s_angle_args[0] = pos;
    s_angle_args[1] = unused;
    s_angle_args[2] = out;
    return s_angle;
}

s32 wm_80095CD4(u32 pos, u32 vel, u32 out, s32 scale, s32 mode)
{
    u32 index;
    s_move_calls++;
    s_move_args[0] = pos;
    s_move_args[1] = vel;
    s_move_args[2] = out;
    s_move_scale = scale;
    s_move_mode = mode;
    for (index = 0u; index < 4u; index++)
        store_u32(out + index * 4u, s_movement_out[index]);
    return s_movement;
}

void wm_8008C040(u32 vector, s32 arg1, s32 arg2, u32 out1, u32 out2)
{
    check("proximity query uses retail ABI",
          vector == OUT && arg1 == 64 && arg2 == 32 &&
          out1 == BOUNDARY && out2 == AREA);
    s_proximity_calls++;
    *(u8 *)PSX_ADDR(out1) = s_boundary;
    *(u8 *)PSX_ADDR(out2) = s_area;
}

s32 wm_80094238(u32 pos, u32 list)
{
    check("trigger query receives slot position", pos == SLOT + 0x28u);
    s_query_calls++;
    s_query_list = list;
    return 0;
}

void wm_8008E078(void)
{
    s_selector_calls++;
}

MATRIX *RotMatrixYXZ(SVECTOR *angles, MATRIX *matrix)
{
    int index = s_matrix_calls;
    if (index < 2) {
        s_matrix_destinations[index] = guest_address(matrix);
        s_matrix_angles[index][0] = angles->vx;
        s_matrix_angles[index][1] = angles->vy;
        s_matrix_angles[index][2] = angles->vz;
    }
    s_matrix_calls++;
    return matrix;
}

s32 wm_80093978(s32 x, s32 z)
{
    (void)x;
    (void)z;
    return s_terrain;
}

void wm_80089160(u32 id, u32 vector, u32 angles)
{
    s_presence_calls++;
    s_presence_args[0] = id;
    s_presence_args[1] = vector;
    s_presence_args[2] = angles;
}

void wm_800894C8(u32 record)
{
    if (s_release_calls < 4)
        s_release_ids[s_release_calls] = record;
    s_release_calls++;
}

void wm_8008E034(u32 vector)
{
    s_unload_calls++;
    s_unload_arg = vector;
}

void wm_80074794(s32 tag, u32 pos)
{
    s_publish_calls++;
    s_publish_tag = tag;
    s_publish_arg = pos;
}

s32 wm_80097770(u32 slot, s32 value)
{
    s_claim_calls++;
    s_claim_slot = slot;
    s_claim_value = value;
    return 1;
}

void func_8003A89C(void *manager, s32 level, s32 steps)
{
    s_sound_calls++;
    s_sound_manager = manager;
    s_sound_level = level;
    s_sound_steps = steps;
}

static void configure_accepted_movement(void)
{
    s_movement = 1;
    s_movement_out[0] = UINT32_C(0x00125000);
    s_movement_out[1] = UINT32_C(0x00046000);
    s_movement_out[2] = UINT32_C(0xFFF65000);
    s_movement_out[3] = UINT32_C(0x33334444);
    s_boundary = 1u;
    s_area = 9u;
    s_terrain = (s32)UINT32_C(0x00046000);
    s_region = 2;
}

static void check_shared_epilogue(const char *name, u16 expected_state)
{
    int context_ok =
        load_u32(CONTEXT + 0x5Cu) == load_u32(CONTEXT + 0x08u) &&
        load_u32(CONTEXT + 0x60u) == load_u32(CONTEXT + 0x0Cu) &&
        load_u32(CONTEXT + 0x64u) == load_u32(CONTEXT + 0x10u);
    check(name,
          context_ok && s_unload_calls == 1 &&
          s_unload_arg == SLOT + 0x28u &&
          load_u16(NATIVE_HEAD) == UINT16_C(0x0456) &&
          load_u16(SLOT + 0x20u) == expected_state);
}

static void test_accepted_movement(void)
{
    s32 result;
    reset_case();
    configure_accepted_movement();

    result = wm_8008E76C_state2(SLOT);
    check("state2 returns retail constant one", result == 1);
    check("movement helper receives retail ABI",
          s_move_calls == 1 && s_move_args[0] == SLOT + 0x28u &&
          s_move_args[1] == SLOT + 0x38u && s_move_args[2] == OUT &&
          s_move_scale == INT32_C(0x1800) && s_move_mode == 7);
    check("only result one enters proximity query", s_proximity_calls == 1);
    check("accepted output copies all four words",
          load_u32(SLOT + 0x28u) == s_movement_out[0] &&
          load_u32(SLOT + 0x2Cu) == s_movement_out[1] &&
          load_u32(SLOT + 0x30u) == s_movement_out[2] &&
          load_u32(SLOT + 0x34u) == s_movement_out[3]);
    check("trigger query uses retail list two",
          s_query_calls == 1 && s_query_list == 2u);
    check("both retail matrices are built",
          s_matrix_calls == 2 &&
          s_matrix_destinations[0] == CONTEXT + 0x20u &&
          s_matrix_destinations[1] == CONTEXT + 0x74u &&
          s_matrix_angles[0][0] == (s16)-35 &&
          s_matrix_angles[0][1] == (s16)0x0456 &&
          s_matrix_angles[0][2] == (s16)0x0789);
    check("region two updates record 63 then releases 60",
          s_presence_calls == 1 && s_presence_args[0] == 63u &&
          s_presence_args[1] == SHORT_POS &&
          s_presence_args[2] == ANGLES &&
          s_release_calls == 1 && s_release_ids[0] == 60u);
    /* Retail 0x8008EE68..0x8008EC48: the walking arm clears the velocity
     * words and the trigger halfword only; the proximity bytes written by
     * wm_8008C040 survive for later scheduler slots. */
    check("walking arm clears velocities and trigger only",
          load_u32(SLOT + 0x38u) == 0u &&
          load_u32(SLOT + 0x3Cu) == 0u &&
          load_u32(SLOT + 0x40u) == 0u &&
          load_u32(SLOT + 0x60u) == UINT32_C(0x1800) &&
          *(u8 *)PSX_ADDR(AREA) == s_area &&
          *(u8 *)PSX_ADDR(BOUNDARY) == s_boundary &&
          load_u16(TRIGGER) == 0u);
    check("position and heading publish before epilogue",
          load_u32(POSITION + 0u) == s_movement_out[0] &&
          load_u32(POSITION + 4u) == s_movement_out[1] &&
          load_u32(POSITION + 8u) == s_movement_out[2] &&
          load_u32(POSITION + 12u) == s_movement_out[3] &&
          load_u16(HEADING) == UINT16_C(0x0456));
    check("state two publishes pose ring",
          s_publish_calls == 1 && s_publish_tag == 2 &&
          s_publish_arg == SLOT + 0x28u);
    check_shared_epilogue("shared epilogue mirrors position", UINT16_C(2));
}

static void test_non_one_result_does_not_copy(void)
{
    reset_case();
    s_movement = 2;
    s_movement_out[0] = UINT32_C(0x77777777);
    s_terrain = (s32)load_u32(SLOT + 0x2Cu);

    (void)wm_8008E76C_state2(SLOT);
    check("only exact result one may copy movement",
          s_proximity_calls == 0 &&
          load_u32(SLOT + 0x28u) == UINT32_C(0x00123000));
}

static void test_zero_result_clears_speed(void)
{
    reset_case();
    s_movement = 0;
    s_terrain = (s32)load_u32(SLOT + 0x2Cu);

    (void)wm_8008E76C_state2(SLOT);
    check("zero movement clears speed and velocity",
          load_u32(SLOT + 0x60u) == 0u &&
          load_u32(SLOT + 0x38u) == 0u &&
          load_u32(SLOT + 0x40u) == 0u && s_proximity_calls == 0);
    check("inactive presence releases both records",
          s_presence_calls == 0 && s_release_calls == 2 &&
          s_release_ids[0] == 60u && s_release_ids[1] == 63u);
}

static void test_boundary_two_blocks_copy(void)
{
    reset_case();
    configure_accepted_movement();
    s_boundary = 2u;

    (void)wm_8008E76C_state2(SLOT);
    check("boundary two clears speed and blocks position copy",
          load_u32(SLOT + 0x60u) == 0u &&
          load_u32(SLOT + 0x28u) == UINT32_C(0x00123000));
}

static void test_terminal_control(void)
{
    reset_case();
    s_control = 1;

    (void)wm_8008E76C_state2(SLOT);
    check("terminal control clears frame and exit globals",
          load_u32(STOP_FRAME) == 0u && load_u32(EXIT_CODE) == 0u);
    check("terminal control skips movement", s_move_calls == 0);
    check_shared_epilogue("terminal path still runs shared epilogue",
                          UINT16_C(2));
}

static void test_transition_control(void)
{
    reset_case();
    s_control = 4;
    s_region = 5;
    s_walkable = 1;
    s_angle = INT32_C(0x0345);
    s_terrain = INT32_C(0x00123456);
    store_u32(TRIGGER_PTR, 0u);
    store_u16(TRIGGER_A, 0u);
    store_u16(TRIGGER_B, 0u);
    *(u8 *)PSX_ADDR(AREA) = 5u;
    *(u8 *)PSX_ADDR(BOUNDARY) = 6u;
    store_u16(TRIGGER, UINT16_C(7));

    (void)wm_8008E76C_state2(SLOT);
    check("walkability lookup uses retail mode and signed region",
          s_walkable_calls == 1 && s_walk_mode == 2 && s_walk_region == 5);
    check("angle search uses retail fixed output address",
          s_angle_calls == 1 && s_angle_args[0] == SLOT + 0x28u &&
          s_angle_args[1] == SLOT + 0x38u &&
          s_angle_args[2] == UINT32_C(0x00068000));
    check("successful transition selects state sixteen",
          load_u16(SLOT + 0x20u) == UINT16_C(16) &&
          load_u32(SLOT + 0x78u) == UINT32_C(0x0345) &&
          load_u32(SLOT + 0x68u) == UINT32_C(0x00123456));
    check("transition claims slot eleven and fades sound",
          s_claim_calls == 1 && s_claim_slot == 11u &&
          s_claim_value == 10 && s_sound_calls == 1 &&
          s_sound_manager == D_80062528 && s_sound_level == 0 &&
          s_sound_steps == 240);
    check("transition releases both presence records",
          s_release_calls == 2 && s_release_ids[0] == 60u &&
          s_release_ids[1] == 63u);
    check("transition resets trigger state",
          load_u32(TRIGGER_PTR) == UINT32_MAX &&
          load_u16(TRIGGER_A) == UINT16_MAX &&
          load_u16(TRIGGER_B) == UINT16_MAX &&
          *(u8 *)PSX_ADDR(AREA) == 0u &&
          *(u8 *)PSX_ADDR(BOUNDARY) == 0u && load_u16(TRIGGER) == 0u);
    /* Retail 0x8008EC34..0x8008EC50 never touches the velocity words. */
    check("transition arm keeps velocity words",
          load_u32(SLOT + 0x38u) == UINT32_C(0x00001000) &&
          load_u32(SLOT + 0x3Cu) == UINT32_C(0x00002000) &&
          load_u32(SLOT + 0x40u) == UINT32_C(0xFFFFD000));
    check("state sixteen publishes pose ring",
          s_publish_calls == 1 && s_publish_tag == 2);
    check_shared_epilogue("transition runs shared epilogue", UINT16_C(16));
}

int main(void)
{
    test_accepted_movement();
    test_non_one_result_does_not_copy();
    test_zero_result_clears_speed();
    test_boundary_two_blocks_copy();
    test_terminal_control();
    test_transition_control();
    printf("W34N123 STATE2 CERTIFICATE %s (%d/%d)\n",
           s_passes == s_total ? "PASS" : "FAIL", s_passes, s_total);
    return s_passes == s_total ? 0 : 1;
}
