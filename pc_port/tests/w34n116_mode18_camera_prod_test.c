/* Focused production-linked certificate for retail mode-18 camera control. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_83a00.h"
#include "world_map_scheduler.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

#define POOL          UINT32_C(0x800A0000)
#define POOL_PTR      UINT32_C(0x8009BE24)
#define SLOT_INDEX    2
#define SLOT          (POOL + (u32)SLOT_INDEX * UINT32_C(0x80))
#define RESET_X       UINT32_C(0x8009C5AC)
#define POSITION      UINT32_C(0x8009BE28)
#define TARGET        UINT32_C(0x8009D55C)
#define ANGLES        UINT32_C(0x8009BD38)
#define CAMERA_INPUT  UINT32_C(0x8009BD40)
#define CAMERA_HEIGHT UINT32_C(0x8009D3F0)
#define CAMERA_GATE   UINT32_C(0x8009D144)
#define WORLD_X       UINT32_C(0x8009BBB4)
#define WORLD_Z       UINT32_C(0x8009BBBC)
#define JITTER_X      UINT32_C(0x8009BD42)
#define JITTER_Z      UINT32_C(0x8009BD4A)
#define SCRATCH       UINT32_C(0x1F800000)

static int failures;
static int random_value;
static unsigned camera_calls;
static u32 camera_output;
static u32 camera_position;
static s32 camera_height;
static u32 camera_angles;
static unsigned motion_calls;
static unsigned motion_order[3];
static unsigned approach_calls;
static s32 approach_targets[4];
static s32 approach_steps[4];

static void check(int condition, const char *name)
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

static u32 read32(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void seed(void)
{
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0, sizeof(g_PsxScratchpad));
    write32(POOL_PTR, POOL);
    write32(RESET_X, UINT32_C(0x01000000));
    write32(SLOT + 0x7Cu, UINT32_C(0x1000));
    write32(CAMERA_GATE, 1u);
    random_value = 0;
    camera_calls = 0u;
    camera_output = 0u;
    camera_position = 0u;
    camera_height = 0;
    camera_angles = 0u;
    motion_calls = 0u;
    memset(motion_order, 0, sizeof(motion_order));
    approach_calls = 0u;
    memset(approach_targets, 0, sizeof(approach_targets));
    memset(approach_steps, 0, sizeof(approach_steps));
}

int rand(void)
{
    return random_value;
}

s32 wm_800771D8(s32 current, s32 target, s32 step)
{
    int64_t distance;
    int64_t magnitude;

    if (approach_calls < 4u) {
        approach_targets[approach_calls] = target;
        approach_steps[approach_calls] = step;
    }
    approach_calls++;
    if (current == target)
        return current;
    distance = (int64_t)target - (int64_t)current;
    if (distance < 0)
        distance = -distance;
    magnitude = (int64_t)step;
    if (magnitude < 0)
        magnitude = -magnitude;
    if (distance < magnitude)
        return target;
    return current + step;
}

void wm_80076DA4(u32 slot, u32 work)
{
    check(slot == SLOT && work == SCRATCH, "motion.arguments");
    if (motion_calls < 3u)
        motion_order[motion_calls] = 1u;
    motion_calls++;
}

void wm_80076F54(u32 slot, u32 work)
{
    check(slot == SLOT && work == SCRATCH, "motion.arguments");
    if (motion_calls < 3u)
        motion_order[motion_calls] = 2u;
    motion_calls++;
}

void wm_80076FA8(u32 slot, u32 work)
{
    check(slot == SLOT && work == SCRATCH, "motion.arguments");
    if (motion_calls < 3u)
        motion_order[motion_calls] = 3u;
    motion_calls++;
}

void wm_80096F18(u32 output_matrix, u32 position, s32 height,
                 u32 rotation_angles)
{
    camera_calls++;
    camera_output = output_matrix;
    camera_position = position;
    camera_height = height;
    camera_angles = rotation_angles;
}

static void test_latch_mapping(void)
{
    unsigned command;

    for (command = 1u; command <= 6u; command++) {
        seed();
        write16(SLOT + 0x04u, (u16)command);
        check(wm_80083A00(SLOT_INDEX) == 1, "latch.return");
        check(read16(SLOT + 0x04u) == 0u &&
              read16(SLOT + 0x20u) == (u16)command,
              "latch.mapping");
    }
}

static void test_latch_presets(void)
{
    seed();
    write16(SLOT + 0x04u, 2u);
    (void)wm_80083A00(SLOT_INDEX);
    check(read32(SLOT + 0x5Cu) == UINT32_C(0x00180000) &&
          read16(ANGLES + 0u) == 64u && read16(ANGLES + 2u) == 416u &&
          read32(SLOT + 0x50u) == UINT32_C(0x00040000) &&
          read32(SLOT + 0x54u) == UINT32_C(0x001A0000) &&
          read32(POSITION + 4u) == UINT32_C(0xFFF60000) &&
          read32(TARGET + 4u) == UINT32_C(0xFFF60000) &&
          read32(SLOT + 0x2Cu) == UINT32_C(0xFFF60000),
          "latch2.position");

    seed();
    write32(WORLD_X, UINT32_C(0x00002000));
    write16(SLOT + 0x04u, 3u);
    (void)wm_80083A00(SLOT_INDEX);
    check(read32(POSITION) == UINT32_C(0x01700000) &&
          read32(TARGET) == UINT32_C(0x01700000) &&
          read32(SLOT + 0x28u) == UINT32_C(0x01700000) &&
          read32(WORLD_X) == UINT32_C(0x00702000),
          "latch3.world_x");

    seed();
    write32(WORLD_X, UINT32_C(0x00001000));
    write32(WORLD_Z, UINT32_C(0x00002000));
    write16(SLOT + 0x04u, 4u);
    (void)wm_80083A00(SLOT_INDEX);
    check(read32(POSITION + 0u) == UINT32_C(0x01800000) &&
          read32(POSITION + 4u) == UINT32_C(0xFFF80000) &&
          read32(POSITION + 8u) == UINT32_C(0x01900000) &&
          read32(WORLD_X) == UINT32_C(0x00001100) &&
          read32(WORLD_Z) == UINT32_C(0x00001F00),
          "latch4.world_drift");

    seed();
    write32(WORLD_Z, UINT32_C(0x00002000));
    write16(SLOT + 0x04u, 6u);
    (void)wm_80083A00(SLOT_INDEX);
    check(read32(SLOT + 0x5Cu) == UINT32_C(0x00620000) &&
          read16(ANGLES + 0u) == UINT16_C(0xFF80) &&
          read16(ANGLES + 2u) == UINT16_C(0x0A70) &&
          read32(POSITION + 4u) == UINT32_C(0xFFEE0000) &&
          read32(POSITION + 8u) == UINT32_C(0x01980000) &&
          read32(WORLD_Z) == UINT32_C(0x00001F00),
          "latch6.preset");
}

static void test_camera_gate(void)
{
    seed();
    write32(CAMERA_GATE, 0u);
    write32(CAMERA_HEIGHT, UINT32_C(0x00123456));
    (void)wm_80083A00(SLOT_INDEX);
    check(camera_calls == 1u && camera_output == CAMERA_INPUT &&
          camera_position == POSITION && camera_height == INT32_C(0x00123456) &&
          camera_angles == ANGLES,
          "camera.call_contract");
    camera_calls = 0u;
    write32(CAMERA_GATE, 1u);
    (void)wm_80083A00(SLOT_INDEX);
    check(camera_calls == 0u, "camera.gate");
}

static void test_state_interpolation(void)
{
    seed();
    write16(SLOT + 0x20u, 1u);
    write32(SLOT + 0x50u, UINT32_C(0xFFFE0000));
    write32(SLOT + 0x54u, UINT32_C(0x00400000));
    write32(SLOT + 0x5Cu, UINT32_C(0x00960000));
    (void)wm_80083A00(SLOT_INDEX);
    check(approach_calls == 3u &&
          approach_targets[0] == INT32_C(-0x00400000) &&
          approach_steps[0] == INT32_C(-0x00008000) &&
          approach_targets[1] == 0 &&
          approach_steps[1] == INT32_C(-0x00008000) &&
          approach_targets[2] == INT32_C(0x00380000) &&
          approach_steps[2] == INT32_C(-0x0000D000) &&
          read32(SLOT + 0x50u) == UINT32_C(0xFFFD8000) &&
          read32(SLOT + 0x54u) == UINT32_C(0x003F8000) &&
          read32(SLOT + 0x5Cu) == UINT32_C(0x00953000),
          "state1.interpolation");

    seed();
    write16(SLOT + 0x20u, 5u);
    write32(SLOT + 0x50u, UINT32_C(0xFFF00000));
    (void)wm_80083A00(SLOT_INDEX);
    check(approach_calls == 1u &&
          approach_targets[0] == INT32_C(0x00200000) &&
          approach_steps[0] == INT32_C(0x00002000) &&
          read32(SLOT + 0x50u) == UINT32_C(0xFFF02000),
          "state5.interpolation");
}

static void test_pipeline_and_jitter(void)
{
    seed();
    (void)wm_80083A00(SLOT_INDEX);
    check(motion_calls == 3u && motion_order[0] == 1u &&
          motion_order[1] == 2u && motion_order[2] == 3u,
          "motion.pipeline");

    seed();
    write32(SLOT + 0x7Cu, UINT32_C(0x8000));
    write16(JITTER_X, 100u);
    write16(JITTER_Z, 200u);
    random_value = 10;
    (void)wm_80083A00(SLOT_INDEX);
    check(read16(SCRATCH + 0xA2u) == UINT16_C(0xFFFE) &&
          read16(JITTER_X) == 98u && read16(JITTER_Z) == 198u,
          "jitter.center");
}

static void test_scheduler_resolution(void)
{
    seed();
    write32(SLOT + 0x1Cu, UINT32_C(0x80083A00));
    write16(SLOT + 0x00u, 1u);
    wm_sched_reset();
    wm_sched_callback_registry_clear();
    wm_80097800();
    check(wm_sched_get_callbacks_executed() == 1 &&
          wm_sched_get_missing_hits() == 0 &&
          wm_sched_get_invalid_hits() == 0,
          "scheduler.update_resolution");
}

int main(void)
{
    test_latch_mapping();
    test_latch_presets();
    test_camera_gate();
    test_state_interpolation();
    test_pipeline_and_jitter();
    test_scheduler_resolution();
    if (failures != 0) {
        fprintf(stderr,
                "W34N116 MODE18 CAMERA CERTIFICATE: %d failure(s)\n",
                failures);
        return 1;
    }
    puts("W34N116 MODE18 CAMERA CERTIFICATE PASS");
    return 0;
}
