/* Focused production-linked certificate for retail mode-17 slot-2 control. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_827ec.h"
#include "world_map_scheduler.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

#define POOL          UINT32_C(0x800A0000)
#define POOL_PTR      UINT32_C(0x8009BE24)
#define SLOT_INDEX    2
#define SLOT          (POOL + (u32)SLOT_INDEX * UINT32_C(0x80))
#define RESET_X       UINT32_C(0x8009C5AC)
#define RESET_Y       UINT32_C(0x8009C5B0)
#define RESET_Z       UINT32_C(0x8009C5B4)
#define POSITION      UINT32_C(0x8009BE28)
#define TARGET        UINT32_C(0x8009D55C)
#define ANGLES        UINT32_C(0x8009BD38)
#define CAMERA_INPUT  UINT32_C(0x8009BD40)
#define CAMERA_HEIGHT UINT32_C(0x8009D3F0)
#define CAMERA_GATE   UINT32_C(0x8009D144)
#define VIEW_HEIGHT   UINT32_C(0x8009BE0C)
#define WORLD_X       UINT32_C(0x8009BBB4)
#define WORLD_Z       UINT32_C(0x8009BBBC)
#define JITTER_X      UINT32_C(0x8009BD42)
#define JITTER_Z      UINT32_C(0x8009BD4A)
#define SCRATCH       UINT32_C(0x1F800000)

static int failures;
static int random_value;
static unsigned wrap_delta_calls;
static unsigned wrap_target_calls;
static u32 last_wrap_delta;
static u32 last_wrap_target;
static unsigned camera_calls;
static u32 camera_output;
static u32 camera_position;
static s32 camera_height;
static u32 camera_angles;

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
    write32(RESET_X, UINT32_C(0x01100000));
    write32(RESET_Y, UINT32_C(0x02200000));
    write32(RESET_Z, UINT32_C(0x03300000));
    write32(SLOT + 0x7Cu, 4096u);
    random_value = 0;
    wrap_delta_calls = 0u;
    wrap_target_calls = 0u;
    last_wrap_delta = 0u;
    last_wrap_target = 0u;
    camera_calls = 0u;
    camera_output = 0u;
    camera_position = 0u;
    camera_height = 0;
    camera_angles = 0u;
}

int rand(void)
{
    return random_value;
}

void wm_80093484(u32 vector)
{
    wrap_delta_calls++;
    last_wrap_delta = vector;
}

void wm_80093354(u32 vector)
{
    wrap_target_calls++;
    last_wrap_target = vector;
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

static void initialize(void)
{
    check(wm_800827EC(SLOT_INDEX) == 1, "init.return");
}

static void test_initializer(void)
{
    seed();
    write32(SLOT - 4u, UINT32_C(0x13579BDF));
    write32(SLOT + 0x80u, UINT32_C(0x2468ACE0));
    initialize();
    check(read32(SLOT + 0x7Cu) == 4096u &&
          read16(SLOT + 0x20u) == 0u && read16(SLOT + 0x04u) == 0u &&
          read32(SLOT + 0x5Cu) == UINT32_C(0x00500000) &&
          read32(VIEW_HEIGHT) == 120u,
          "init.constants");
    check(read32(POSITION + 0u) == UINT32_C(0x01100000) &&
          read32(POSITION + 4u) == UINT32_C(0x02200000) &&
          read32(POSITION + 8u) == UINT32_C(0x03300000) &&
          read32(TARGET + 0u) == UINT32_C(0x01100000) &&
          read32(TARGET + 4u) == UINT32_C(0x02200000) &&
          read32(TARGET + 8u) == UINT32_C(0x03300000) &&
          read32(SLOT + 0x28u) == UINT32_C(0x01100000) &&
          read32(SLOT + 0x2Cu) == UINT32_C(0x02200000) &&
          read32(SLOT + 0x30u) == UINT32_C(0x03300000),
          "init.position_fanout");
    check(read16(ANGLES + 0u) == UINT16_C(0xFDE0) &&
          read16(ANGLES + 2u) == 0u && read16(ANGLES + 4u) == 0u &&
          read32(SLOT + 0x50u) == UINT32_C(0xFFDE0000) &&
          read32(SLOT + 0x38u) == UINT32_C(0xFFDE0000) &&
          read32(SLOT + 0x54u) == 0u && read32(SLOT + 0x3Cu) == 0u &&
          read32(SLOT + 0x58u) == 0u && read32(SLOT + 0x40u) == 0u &&
          read32(CAMERA_HEIGHT) == UINT32_C(0x00500000) &&
          read32(CAMERA_GATE) == 0u,
          "init.camera_state");
    check(read32(SLOT - 4u) == UINT32_C(0x13579BDF) &&
          read32(SLOT + 0x80u) == UINT32_C(0x2468ACE0),
          "init.write_bounds");
}

static void test_interpolator(void)
{
    check(wm_800771D8(100, 105, 10) == 105 &&
          wm_800771D8(100, 130, 10) == 110 &&
          wm_800771D8(100, 100, 10) == 100,
          "approach.clamp_and_step");
    check(wm_800771D8(100, 110, -10) == 90,
          "approach.strict_clamp");
}

static void test_latch_mapping(void)
{
    static const u16 expected[12] = {
        1u, 0u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u
    };
    unsigned command;

    for (command = 1u; command <= 12u; command++) {
        seed();
        initialize();
        write32(CAMERA_GATE, 1u);
        write16(SLOT + 0x04u, (u16)command);
        check(wm_800828DC(SLOT_INDEX) == 1, "latch.return");
        check(read16(SLOT + 0x04u) == 0u &&
              read16(SLOT + 0x20u) == expected[command - 1u],
              "latch.mapping");
    }
}

static void test_latch_presets(void)
{
    seed();
    initialize();
    write32(CAMERA_GATE, 1u);
    write16(SLOT + 0x04u, 2u);
    (void)wm_800828DC(SLOT_INDEX);
    check(read32(SLOT + 0x5Cu) == UINT32_C(0x003E0000) &&
          read16(ANGLES + 0u) == UINT16_C(0xFFE0) &&
          read16(ANGLES + 2u) == UINT16_C(0x0D70) &&
          read32(SLOT + 0x50u) == UINT32_C(0xFFFE0000) &&
          read32(SLOT + 0x54u) == UINT32_C(0x00D70000) &&
          read32(CAMERA_HEIGHT) == UINT32_C(0x003E0000),
          "latch2.camera_preset");

    seed();
    initialize();
    write32(CAMERA_GATE, 1u);
    write16(SLOT + 0x04u, 6u);
    (void)wm_800828DC(SLOT_INDEX);
    check(read32(SLOT + 0x7Cu) == 4096u &&
          read32(SLOT + 0x5Cu) == UINT32_C(0x00320000) &&
          read16(ANGLES + 0u) == 64u &&
          read16(ANGLES + 2u) == 3392u &&
          read32(TARGET + 0u) == UINT32_C(0x01379000) &&
          read32(TARGET + 4u) == UINT32_C(0xFFEE0000) &&
          read32(TARGET + 8u) == UINT32_C(0x04B2A000),
          "latch6.full_preset");
}

static void test_camera_gate(void)
{
    seed();
    initialize();
    write32(CAMERA_GATE, 0u);
    check(wm_800828DC(SLOT_INDEX) == 1 && camera_calls == 1u &&
          camera_output == CAMERA_INPUT && camera_position == POSITION &&
          camera_height == INT32_C(0x00500000) &&
          camera_angles == ANGLES,
          "camera.call_contract");
    camera_calls = 0u;
    write32(CAMERA_GATE, 1u);
    (void)wm_800828DC(SLOT_INDEX);
    check(camera_calls == 0u, "camera.gate");
}

static void test_rotation_helper(void)
{
    seed();
    write32(SLOT + 0x38u, 0u);
    write32(SLOT + 0x3Cu, 0u);
    write32(SLOT + 0x40u, 0u);
    write32(SLOT + 0x50u, 800u);
    write32(SLOT + 0x54u, 400u);
    write32(SLOT + 0x58u, UINT32_C(0xFFFFFCE0));
    wm_80076DA4(SLOT, SCRATCH);
    check(wrap_delta_calls == 1u && last_wrap_delta == SCRATCH &&
          read32(SCRATCH + 0u) == 800u &&
          read32(SCRATCH + 4u) == 400u &&
          read32(SCRATCH + 8u) == UINT32_C(0xFFFFFCE0) &&
          read32(SCRATCH + 0x10u) == 100u &&
          read32(SCRATCH + 0x14u) == 50u &&
          read32(SCRATCH + 0x18u) == UINT32_C(0xFFFFFF9C),
          "motion.rotation_work");
    check(read32(SLOT + 0x38u) == 100u &&
          read32(SLOT + 0x3Cu) == 400u &&
          read32(SLOT + 0x40u) == UINT32_C(0xFFFFFF9C),
          "motion.rotation_step");
}

static void test_height_helper(void)
{
    seed();
    write32(CAMERA_HEIGHT, 0u);
    write32(SLOT + 0x5Cu, 800u);
    wm_80076F54(SLOT, SCRATCH);
    check(read32(CAMERA_HEIGHT) == 100u, "motion.height_step");
    write32(CAMERA_HEIGHT, 0u);
    write32(SLOT + 0x5Cu, 400u);
    wm_80076F54(SLOT, SCRATCH);
    check(read32(CAMERA_HEIGHT) == 400u, "motion.height_clamp");
}

static void test_position_helper(void)
{
    seed();
    write32(SLOT + 0x28u, 0u);
    write32(SLOT + 0x2Cu, 0u);
    write32(SLOT + 0x30u, 0u);
    write32(SLOT + 0x34u, UINT32_C(0xABCDEF01));
    write32(TARGET + 0u, 4000u);
    write32(TARGET + 4u, 8000u);
    write32(TARGET + 8u, UINT32_C(0xFFFFE0C0));
    write32(WORLD_X, 100u);
    write32(WORLD_Z, 200u);
    wm_80076FA8(SLOT, SCRATCH);
    check(wrap_delta_calls == 1u && last_wrap_delta == SCRATCH &&
          read32(SCRATCH + 0u) == 4000u &&
          read32(SCRATCH + 4u) == 8000u &&
          read32(SCRATCH + 8u) == UINT32_C(0xFFFFE0C0),
          "motion.position_work");
    check(read32(SLOT + 0x28u) == 4000u &&
          read32(SLOT + 0x2Cu) == 1000u &&
          read32(SLOT + 0x30u) == UINT32_C(0xFFFFFC18) &&
          read32(WORLD_X) == 4100u &&
          read32(WORLD_Z) == UINT32_C(0xFFFFFCE0),
          "motion.position_step");
    check(read32(POSITION + 0u) == read32(SLOT + 0x28u) &&
          read32(POSITION + 4u) == read32(SLOT + 0x2Cu) &&
          read32(POSITION + 8u) == read32(SLOT + 0x30u) &&
          read32(POSITION + 0x0Cu) == UINT32_C(0xABCDEF01),
          "motion.position_publish");
}

static void test_state_machine(void)
{
    seed();
    initialize();
    write32(CAMERA_GATE, 1u);
    write16(SLOT + 0x20u, 2u);
    write32(SLOT + 0x50u, 0u);
    (void)wm_800828DC(SLOT_INDEX);
    check(read32(SLOT + 0x50u) == UINT32_C(0xFFFF0000),
          "state2.step_direction");

    seed();
    initialize();
    write32(CAMERA_GATE, 1u);
    write16(SLOT + 0x20u, 4u);
    write32(SLOT + 0x7Cu, UINT32_C(0x7E00));
    (void)wm_800828DC(SLOT_INDEX);
    check(read32(SLOT + 0x7Cu) == UINT32_C(0x8000) &&
          read16(SLOT + 0x20u) == 4u,
          "state4.strict_ceiling");
    (void)wm_800828DC(SLOT_INDEX);
    check(read32(SLOT + 0x7Cu) == UINT32_C(0x8000) &&
          read16(SLOT + 0x20u) == 0u,
          "state4.clamp_and_exit");

    seed();
    initialize();
    write32(CAMERA_GATE, 1u);
    write16(SLOT + 0x20u, 11u);
    write32(SLOT + 0x50u, UINT32_C(0xFFF80000));
    write32(SLOT + 0x54u, UINT32_C(0x002C0000));
    write32(SLOT + 0x5Cu, UINT32_C(0x00100000));
    write32(TARGET + 0u, UINT32_C(0x04CD9000));
    write32(TARGET + 4u, UINT32_C(0xFFEA4000));
    write32(TARGET + 8u, UINT32_C(0x01C8A000));
    write32(SLOT + 0x28u, UINT32_C(0x04CD9000));
    write32(SLOT + 0x2Cu, UINT32_C(0xFFEA4000));
    write32(SLOT + 0x30u, UINT32_C(0x01C8A000));
    (void)wm_800828DC(SLOT_INDEX);
    check(wrap_target_calls == 1u && last_wrap_target == TARGET,
          "state11.wrap");
}

static void test_pipeline_and_jitter(void)
{
    seed();
    initialize();
    write32(CAMERA_GATE, 1u);
    write32(SLOT + 0x38u, 0u);
    write32(SLOT + 0x3Cu, 0u);
    write32(SLOT + 0x40u, 0u);
    write32(SLOT + 0x50u, 800u);
    write32(SLOT + 0x54u, 400u);
    write32(SLOT + 0x58u, 0u);
    write32(CAMERA_HEIGHT, 0u);
    write32(SLOT + 0x5Cu, 800u);
    write32(SLOT + 0x28u, 0u);
    write32(SLOT + 0x2Cu, 0u);
    write32(SLOT + 0x30u, 0u);
    write32(TARGET + 0u, 4000u);
    write32(TARGET + 4u, 0u);
    write32(TARGET + 8u, 0u);
    (void)wm_800828DC(SLOT_INDEX);
    check(read32(SLOT + 0x38u) == 100u &&
          read32(CAMERA_HEIGHT) == 100u &&
          read32(SLOT + 0x28u) == 4000u,
          "motion.pipeline");

    seed();
    initialize();
    write32(CAMERA_GATE, 1u);
    write32(SLOT + 0x7Cu, UINT32_C(0x8000));
    write16(JITTER_X, 100u);
    write16(JITTER_Z, 200u);
    random_value = 10;
    (void)wm_800828DC(SLOT_INDEX);
    check(read16(SCRATCH + 0xA2u) == UINT16_C(0xFFFE) &&
          read16(JITTER_X) == 98u && read16(JITTER_Z) == 198u,
          "jitter.center");
}

static void test_scheduler_resolution(void)
{
    seed();
    write32(SLOT + 0x18u, UINT32_C(0x800827EC));
    write32(SLOT + 0x1Cu, UINT32_C(0x800828DC));
    write16(SLOT + 0x00u, 0u);
    wm_sched_reset();
    wm_sched_callback_registry_clear();
    wm_80097800();
    check(wm_sched_get_callbacks_executed() == 1 &&
          wm_sched_get_missing_hits() == 0 &&
          wm_sched_get_invalid_hits() == 0 &&
          read16(SLOT + 0x00u) == 1u,
          "scheduler.init_resolution");
    wm_sched_reset();
    wm_80097800();
    check(wm_sched_get_callbacks_executed() == 1 &&
          wm_sched_get_missing_hits() == 0 &&
          wm_sched_get_invalid_hits() == 0 &&
          read16(SLOT + 0x00u) == 1u,
          "scheduler.update_resolution");
}

int main(void)
{
    test_initializer();
    test_interpolator();
    test_latch_mapping();
    test_latch_presets();
    test_camera_gate();
    test_rotation_helper();
    test_height_helper();
    test_position_helper();
    test_state_machine();
    test_pipeline_and_jitter();
    test_scheduler_resolution();
    if (failures != 0) {
        fprintf(stderr,
                "W34N113 MODE17 CONTROLLER CERTIFICATE: %d failure(s)\n",
                failures);
        return 1;
    }
    puts("W34N113 MODE17 CONTROLLER CERTIFICATE PASS");
    return 0;
}
