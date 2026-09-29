/* Focused production certificate for retail mode-9 callbacks 0x80077DC8-0x80078948. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_callback_77dc8.h"
#include "world_map_scheduler.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

#define POOL          0x800A0000u
#define POOL_PTR      0x8009BE24u
#define SLOT_INDEX    2
#define SLOT          (POOL + (u32)SLOT_INDEX * 0x80u)
#define PATH_TABLE    0x8009A3F0u
#define RENDER_X      0x8009BBB4u
#define RENDER_Z      0x8009BBBCu
#define ANGLES        0x8009BD38u
#define CAMERA_INPUT  0x8009BD40u
#define CAMERA_MATRIX 0x8009C808u
#define POSITION      0x8009BE28u
#define CONTEXT_PTR   0x8009C620u
#define CAMERA_SOURCE 0x8009C5ACu
#define CONTEXT       0x800A8000u
#define GEOM_Y        0x8009BE0Cu
#define MODE_STATE    0x8009D144u
#define HEIGHT        0x8009D3F0u
#define CCA4          0x8009CCA4u
#define D3CC          0x8009D3CCu
#define D554          0x8009D554u
#define D7CC          0x8009D7CCu

#define SC_VECTOR     0x1F800000u
#define SC_VECTOR_B   0x1F800010u
#define SC_PATH_A     0x1F8000A0u

static uint8_t sound_authority[32];
void* D_8006259C = sound_authority;

static int failures;
static int sound_start_calls;
static int sound_volume_calls;
static int sound_stop_calls;
static s32 sound_start_id;
static s32 sound_volume_id;
static s32 sound_volume;
static s32 sound_stop_id;
static s32 sound_stop_pitch;
static s32 sound_stop_steps;
static int presence_calls;
static u32 presence_a0;
static u32 presence_a1;
static u32 presence_a2;
static int camera_look_calls;
static int camera_euler_calls;
static u32 camera_look_arg;
static u32 camera_euler_matrix;
static u32 camera_euler_angles;
static int claim_calls;
static u32 claim_slot;
static s32 claim_value;
static int distance_calls;
static u32 distance_left[3];
static u32 distance_right[3];
static int link_calls;
static s32 link_sources[16];
static s32 link_destinations[16];
static int wrap_calls;
static u32 wrap_arg;
static int rot_calls;
static s16 rot_inputs[4][3];

static void check(int condition, const char* name)
{
    if (!condition) {
        fprintf(stderr, "ASSERTION %s FAILED\n", name);
        failures++;
    }
}

static void write16(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void write32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u16 read16(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static s16 read_s16(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 read32(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void reset_trace(void)
{
    sound_start_calls = 0;
    sound_volume_calls = 0;
    sound_stop_calls = 0;
    presence_calls = 0;
    camera_look_calls = 0;
    camera_euler_calls = 0;
    claim_calls = 0;
    distance_calls = 0;
    link_calls = 0;
    wrap_calls = 0;
    rot_calls = 0;
    memset(distance_left, 0, sizeof(distance_left));
    memset(distance_right, 0, sizeof(distance_right));
    memset(link_sources, 0, sizeof(link_sources));
    memset(link_destinations, 0, sizeof(link_destinations));
    memset(rot_inputs, 0, sizeof(rot_inputs));
}

static void seed(void)
{
    u16 bank = 0x1234u;

    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0xA5, sizeof(g_PsxScratchpad));
    memset(sound_authority, 0, sizeof(sound_authority));
    memcpy(sound_authority + 0x14u, &bank, sizeof(bank));
    write32(POOL_PTR, POOL);
    write32(CONTEXT_PTR, CONTEXT);
    reset_trace();
}

static void seed_path_sentinel(u32 phase)
{
    u32 index = phase >> 12;
    write16(PATH_TABLE + index * 8u + 0x16u, 0xFFFFu);
}

void func_80039E60(s32 packed_id)
{
    sound_start_calls++;
    sound_start_id = packed_id;
}

void func_8003A2E4(s32 packed_id, s32 volume)
{
    sound_volume_calls++;
    sound_volume_id = packed_id;
    sound_volume = volume;
}

void func_8003A3B8(s32 packed_id, s32 pitch, s32 steps)
{
    sound_stop_calls++;
    sound_stop_id = packed_id;
    sound_stop_pitch = pitch;
    sound_stop_steps = steps;
}

void wm_80089160(u32 a0, u32 a1, u32 a2)
{
    presence_calls++;
    presence_a0 = a0;
    presence_a1 = a1;
    presence_a2 = a2;
}

void wm_80097244(u32 input_data)
{
    camera_look_calls++;
    camera_look_arg = input_data;
}

void wm_80097070(u32 matrix, u32 angles)
{
    camera_euler_calls++;
    camera_euler_matrix = matrix;
    camera_euler_angles = angles;
}

void wm_80076858(s32 parameter, u32 vector_a, u32 vector_b,
                 u32 vector_c, u32 output)
{
    (void)parameter;
    (void)vector_a;
    (void)vector_b;
    (void)vector_c;
    (void)output;
}

s32 wm_80094154(u32 left, u32 right)
{
    u32 axis;
    distance_calls++;
    for (axis = 0u; axis < 3u; axis++) {
        distance_left[axis] = read32(left + axis * 4u);
        distance_right[axis] = read32(right + axis * 4u);
    }
    return 24;
}

s32 wm_80097770(u32 slot_idx, s32 value)
{
    claim_calls++;
    claim_slot = slot_idx;
    claim_value = value;
    return 1;
}

void wm_800848B4(s32 source_record, s32 destination_record)
{
    if (link_calls < 16) {
        link_sources[link_calls] = source_record;
        link_destinations[link_calls] = destination_record;
    }
    link_calls++;
}

void wm_80093354(u32 vec_addr)
{
    wrap_calls++;
    wrap_arg = vec_addr;
}

MATRIX* RotMatrixYXZ(SVECTOR* rotation, MATRIX* matrix)
{
    int call = rot_calls;
    int axis;
    u32 word;
    u32 index;

    if (call < 4) {
        for (axis = 0; axis < 3; axis++)
            memcpy(&rot_inputs[call][axis],
                   (const uint8_t*)rotation + (size_t)axis * 2u,
                   sizeof(rot_inputs[call][axis]));
    }
    for (index = 0u; index < 8u; index++) {
        word = 0x10000000u + (u32)(call + 1) * 0x01000000u + index;
        memcpy((uint8_t*)matrix + index * 4u, &word, sizeof(word));
    }
    rot_calls++;
    return matrix;
}

static int matrix_is(u32 address, u32 prefix)
{
    u32 index;
    for (index = 0u; index < 8u; index++)
        if (read32(address + index * 4u) != prefix + index)
            return 0;
    return 1;
}

static void test_path_init(void)
{
    seed();
    check(wm_80077DC8(SLOT_INDEX) == 1, "path_init.return_one");
    check(read32(GEOM_Y) == 120u && read32(HEIGHT) == 0x00200000u,
          "path_init.world_state");
    check(read16(SLOT + 0x20u) == 0u &&
          read32(SLOT + 0x50u) == 0u &&
          read32(SLOT + 0x54u) == 0u &&
          read32(SLOT + 0x58u) == 0u &&
          read16(SLOT + 0x22u) == 24u,
          "path_init.slot_state");
    check(read16(ANGLES) == 0u && read16(ANGLES + 2u) == 0u &&
          read16(ANGLES + 4u) == 0u && read32(MODE_STATE) == 1u,
          "path_init.camera_state");
    check(sound_start_calls == 1 &&
          (u32)sound_start_id == 0x123400A4u,
          "path_init.sound_id");
}

static void test_path_state_machine(void)
{
    seed();
    seed_path_sentinel(0u);
    write16(SLOT + 0x20u, 6u);
    write16(SLOT + 0x22u, 1u);
    write32(D554, 7u);
    write32(D7CC, 9u);
    check(wm_80077E68(SLOT_INDEX) == 1, "path_update.return_one");
    check(read16(SLOT + 0x22u) == 0u && read32(D554) == 0u &&
          read32(D7CC) == 0u, "path.terminal_exit");
    check(camera_look_calls == 1 && camera_look_arg == CAMERA_INPUT &&
          camera_euler_calls == 1 &&
          camera_euler_matrix == CAMERA_MATRIX &&
          camera_euler_angles == ANGLES,
          "path.camera_chain");

    seed();
    seed_path_sentinel(0x8400u);
    write16(SLOT + 0x20u, 5u);
    write32(SLOT + 0x50u, 0x8400u);
    write32(SLOT + 0x58u, 77u);
    (void)wm_80077E68(SLOT_INDEX);
    check(read16(SLOT + 0x20u) == 6u &&
          read16(SLOT + 0x22u) == 80u &&
          read32(SLOT + 0x58u) == 0u &&
          read32(CCA4) == 2u && read32(D3CC) == 4u,
          "path.state5_transition");
    check(sound_stop_calls == 1 &&
          (u32)sound_stop_id == 0x123400A4u &&
          sound_stop_pitch == 0 && sound_stop_steps == 256 &&
          claim_calls == 1 && claim_slot == 0u && claim_value == 13,
          "path.state5_calls");

    seed();
    seed_path_sentinel(0u);
    write16(SLOT + 0x20u, 0u);
    write16(SLOT + 0x22u, 0u);
    write32(POSITION + 0u, 100u << 12);
    write32(POSITION + 8u, 200u << 12);
    (void)wm_80077E68(SLOT_INDEX);
    check(read16(SLOT + 0x20u) == 1u &&
          read32(SLOT + 0x58u) == 128u &&
          read32(SLOT + 0x50u) == 128u,
          "path.state0_transition");
    check(presence_calls == 1 && presence_a0 == 8u &&
          presence_a1 == SC_PATH_A && presence_a2 == 0u &&
          read_s16(SC_PATH_A + 0u) == 150 &&
          read_s16(SC_PATH_A + 2u) == -525 &&
          read_s16(SC_PATH_A + 4u) == 88,
          "path.state0_presence");
}

static void test_sound_vectors(void)
{
    seed();
    seed_path_sentinel(0u);
    write16(SLOT + 0x20u, 3u);
    write16(CAMERA_INPUT + 0x08u, (u16)-2);
    write16(CAMERA_INPUT + 0x0Au, 3u);
    write16(CAMERA_INPUT + 0x0Cu, (u16)-4);
    write16(CAMERA_INPUT + 0x00u, 5u);
    write16(CAMERA_INPUT + 0x02u, (u16)-6);
    write16(CAMERA_INPUT + 0x04u, 7u);
    (void)wm_80077E68(SLOT_INDEX);
    check(distance_calls == 1 &&
          distance_left[0] == 0xFFFFE000u &&
          distance_left[1] == 0x00003000u &&
          distance_left[2] == 0xFFFFC000u &&
          distance_right[0] == 0x00005000u &&
          distance_right[1] == 0xFFFFA000u &&
          distance_right[2] == 0x00007000u,
          "path.sound_vectors");
    check(read32(SC_VECTOR) == 0xFFFFE000u &&
          read32(SC_VECTOR_B) == 0x00005000u &&
          read32(SC_VECTOR + 0x20u) == 3u &&
          sound_volume_calls == 1 &&
          (u32)sound_volume_id == 0x123400A4u && sound_volume == 132,
          "path.sound_volume");
}

static void test_context_init(void)
{
    u32 index;

    seed();
    for (index = 0u; index < 4u; index++)
        write32(CAMERA_SOURCE + index * 4u, 0xABC00000u + index);
    check(wm_8007828C(SLOT_INDEX) == 1, "context_init.return_one");
    check(link_calls == 13, "context.link_count");
    for (index = 0u; index < 13u; index++)
        check(link_sources[index] == 0 &&
              link_destinations[index] == (s32)(index + 1u),
              "context.link_order");
    check(read16(SLOT + 0x20u) == 0u &&
          read32(SLOT + 0x50u) == 0u &&
          read32(SLOT + 0x54u) == 64u &&
          read32(SLOT + 0x58u) == 512u &&
          read32(SLOT + 0x5Cu) == 96u &&
          read32(SLOT + 0x60u) == 768u &&
          read32(SLOT + 0x64u) == 80u,
          "context_init.motion_state");
    for (index = 0u; index < 4u; index++)
        check(read32(SLOT + 0x28u + index * 4u) ==
              0xABC00000u + index &&
              read32(POSITION + index * 4u) == 0xABC00000u + index,
              "context_init.position_copy");
}

static void test_context_update(void)
{
    seed();
    write32(SLOT + 0x28u, 0x00123000u);
    write32(SLOT + 0x2Cu, 0x00045000u);
    write32(SLOT + 0x30u, 0x00067000u);
    write32(SLOT + 0x34u, 0xCAFEBABEu);
    write32(SLOT + 0x50u, 0x0FF0u);
    write32(SLOT + 0x54u, 0x0040u);
    write32(SLOT + 0x58u, 0x0FE0u);
    write32(SLOT + 0x5Cu, 0x0060u);
    write32(SLOT + 0x60u, 0x0FD0u);
    write32(SLOT + 0x64u, 0x0050u);
    write32(RENDER_X, 0xABCDEF01u);
    write32(RENDER_Z, 0x00100000u);
    write32(SC_VECTOR + 0x0Cu, 0xDEADBEEFu);
    check(wm_800783E8(SLOT_INDEX) == 1, "context_update.return_one");
    check(read32(SLOT + 0x30u) == 0x00063000u &&
          read32(RENDER_Z) == 0x000FC000u &&
          read32(RENDER_X) == 0xABCDEF01u,
          "context.render_z");
    check(wrap_calls == 1 && wrap_arg == SLOT + 0x28u,
          "context.wrap_call");
    check(read32(CONTEXT + 0x08u) == 0x123u,
          "context.scratch_position_x");
    check(read32(CONTEXT + 0x0Cu) == 0x45u,
          "context.scratch_position_y");
    check(read32(CONTEXT + 0x10u) == 0x63u,
          "context.scratch_position_z");
    check(read32(CONTEXT + 0x14u) == 0xDEADBEEFu,
          "context.scratch_position_pad");
    check(read32(POSITION + 0u) == 0x00123000u &&
          read32(POSITION + 4u) == 0x00045000u &&
          read32(POSITION + 8u) == 0x00063000u &&
          read32(POSITION + 0x0Cu) == 0xCAFEBABEu,
          "context.global_position");
    check(read32(SLOT + 0x50u) == 0x30u &&
          read32(SLOT + 0x58u) == 0x40u &&
          read32(SLOT + 0x60u) == 0x20u,
          "context.angle_steps");
    check(rot_calls == 4 &&
          rot_inputs[0][0] == 0 && rot_inputs[0][1] == 0x30 &&
          rot_inputs[0][2] == 0 &&
          rot_inputs[1][0] == 0 && rot_inputs[1][1] == 0x40 &&
          rot_inputs[1][2] == 0 &&
          rot_inputs[2][0] == 0 && rot_inputs[2][1] == 0x20 &&
          rot_inputs[2][2] == 0 &&
          rot_inputs[3][0] == 0 && rot_inputs[3][1] == 0 &&
          rot_inputs[3][2] == 0x20,
          "context.rotation_inputs");
    check(matrix_is(CONTEXT + 0x11Cu, 0x14000000u) &&
          matrix_is(CONTEXT + 0x0C8u, 0x14000000u) &&
          matrix_is(CONTEXT + 0x074u, 0x14000000u),
          "context.matrix_d");
    check(matrix_is(CONTEXT + 0x1C4u, 0x13000000u) &&
          matrix_is(CONTEXT + 0x170u, 0x13000000u) &&
          matrix_is(CONTEXT + 0x410u, 0x11000000u) &&
          matrix_is(CONTEXT + 0x2C0u, 0x11000000u) &&
          matrix_is(CONTEXT + 0x368u, 0x11000000u) &&
          matrix_is(CONTEXT + 0x218u, 0x11000000u) &&
          matrix_is(CONTEXT + 0x464u, 0x12000000u) &&
          matrix_is(CONTEXT + 0x314u, 0x12000000u) &&
          matrix_is(CONTEXT + 0x3BCu, 0x12000000u) &&
          matrix_is(CONTEXT + 0x26Cu, 0x12000000u),
          "context.matrix_chains");
}

static void test_scheduler_resolution(void)
{
    seed();
    memset(PSX_ADDR(POOL), 0, 64u * 0x80u);
    write32(POOL_PTR, POOL);
    write32(SLOT + 0x18u, 0x80077DC8u);
    write32(SLOT + 0x1Cu, 0x80077E68u);
    seed_path_sentinel(0u);
    wm_sched_reset();
    wm_sched_callback_registry_clear();
    wm_80097800();
    wm_80097800();
    check(sound_start_calls == 1 && camera_look_calls == 1,
          "scheduler.path_pair");
    check(wm_sched_get_missing_hits() == 0 &&
          wm_sched_get_invalid_hits() == 0,
          "scheduler.path_resolution");

    seed();
    memset(PSX_ADDR(POOL), 0, 64u * 0x80u);
    write32(POOL_PTR, POOL);
    write32(SLOT + 0x18u, 0x8007828Cu);
    write32(SLOT + 0x1Cu, 0x800783E8u);
    wm_sched_reset();
    wm_sched_callback_registry_clear();
    wm_80097800();
    wm_80097800();
    check(link_calls == 13 && wrap_calls == 1 && rot_calls == 4,
          "scheduler.context_pair");
    check(wm_sched_get_missing_hits() == 0 &&
          wm_sched_get_invalid_hits() == 0,
          "scheduler.context_resolution");
}

int main(void)
{
    test_path_init();
    test_path_state_machine();
    test_sound_vectors();
    test_context_init();
    test_context_update();
    test_scheduler_resolution();
    if (failures != 0) {
        fprintf(stderr, "W34N60 MODE9 CALLBACK CERTIFICATE: %d failure(s)\n",
                failures);
        return 1;
    }
    puts("W34N60 MODE9 CALLBACK CERTIFICATE PASS");
    return 0;
}
