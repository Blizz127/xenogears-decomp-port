/* Focused production certificate for retail 0x8007AD34/0x8007ADD4. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_7ad34.h"
#include "world_map_scheduler.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

#define POOL          UINT32_C(0x800A0000)
#define POOL_PTR      UINT32_C(0x8009BE24)
#define SLOT_INDEX    6
#define SLOT          (POOL + (u32)SLOT_INDEX * UINT32_C(0x80))
#define RESET_POS     UINT32_C(0x8009C5AC)
#define ANGLES        UINT32_C(0x8009BD38)
#define CAMERA_INPUT  UINT32_C(0x8009BD40)
#define GEOM_Y        UINT32_C(0x8009BE0C)
#define POSITION      UINT32_C(0x8009BE28)
#define CAMERA_MATRIX UINT32_C(0x8009C808)
#define EVENT_FLAG    UINT32_C(0x8009D144)
#define VIEW_HEIGHT   UINT32_C(0x8009D3F0)
#define POSITION_COPY UINT32_C(0x8009D55C)
#define SC_JITTER     UINT32_C(0x1F8000A0)

typedef struct HelperCall {
    u32 a0;
    u32 a1;
    s32 a2;
    u32 a3;
} HelperCall;

static int failures;
static HelperCall generic_calls[16];
static unsigned generic_count;
static u32 look_inputs[16];
static unsigned look_count;
static u32 angle_matrix[16];
static u32 angle_input[16];
static unsigned angle_count;
static int random_values[32];
static unsigned random_count;
static unsigned random_index;

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

static void set_random_pair(int first, int second)
{
    random_values[0] = first;
    random_values[1] = second;
    random_count = 2u;
    random_index = 0u;
}

int rand(void)
{
    int value;

    check(random_index < random_count, "trace.random_capacity");
    if (random_index >= random_count)
        return 0;
    value = random_values[random_index];
    random_index++;
    return value;
}

void wm_80096F18(u32 output_matrix, u32 position, s32 height,
                  u32 rotation)
{
    check(generic_count < 16u, "trace.generic_capacity");
    if (generic_count < 16u) {
        generic_calls[generic_count].a0 = output_matrix;
        generic_calls[generic_count].a1 = position;
        generic_calls[generic_count].a2 = height;
        generic_calls[generic_count].a3 = rotation;
        generic_count++;
    }
}

void wm_80097244(u32 input_data)
{
    check(look_count < 16u, "trace.look_capacity");
    if (look_count < 16u) {
        look_inputs[look_count] = input_data;
        look_count++;
    }
}

void wm_80097070(u32 matrix, u32 angles)
{
    check(angle_count < 16u, "trace.angle_capacity");
    if (angle_count < 16u) {
        angle_matrix[angle_count] = matrix;
        angle_input[angle_count] = angles;
        angle_count++;
    }
}

void wm_80089160(u32 a0, u32 a1, u32 a2)
{
    (void)a0;
    (void)a1;
    (void)a2;
}

void wm_800894C8(u32 record_index)
{
    (void)record_index;
}

static void seed(void)
{
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0, sizeof(g_PsxScratchpad));
    write32(POOL_PTR, POOL);
    write32(RESET_POS + 0u, UINT32_C(0x00100000));
    write32(RESET_POS + 4u, UINT32_C(0x00080000));
    write32(RESET_POS + 8u, UINT32_C(0x00200000));
    write32(POSITION + 0u, UINT32_C(0x00100000));
    write32(POSITION + 4u, UINT32_C(0x00080000));
    write32(POSITION + 8u, UINT32_C(0x00200000));
    write32(VIEW_HEIGHT, UINT32_C(0x001C0000));
    write16(ANGLES + 0u, 64u);
    write16(ANGLES + 2u, 1000u);
    write16(ANGLES + 4u, 0u);
    write32(SLOT + 0x50u, 8192u);
    generic_count = 0u;
    look_count = 0u;
    angle_count = 0u;
    set_random_pair(1, 1);
}

static void test_initializer(void)
{
    seed();
    write32(GEOM_Y, UINT32_C(0xAAAAAAAA));
    write32(VIEW_HEIGHT, UINT32_C(0xBBBBBBBB));
    write32(POSITION_COPY + 0u, UINT32_C(0xCCCCCCCC));
    write32(POSITION_COPY + 4u, UINT32_C(0xDDDDDDDD));
    write32(POSITION_COPY + 8u, UINT32_C(0xEEEEEEEE));
    write32(SLOT + 0x50u, UINT32_C(0xFFFFFFFF));

    check(wm_8007AD34(SLOT_INDEX) == 1, "init.return");
    check(read32(GEOM_Y) == 120u, "init.geom_y");
    check(read32(VIEW_HEIGHT) == UINT32_C(0x001C0000),
          "init.view_height");
    check(read16(ANGLES + 0u) == 64u &&
          read16(ANGLES + 2u) == 1152u &&
          read16(ANGLES + 4u) == 0u,
          "init.angles");
    check(read32(POSITION + 0u) == UINT32_C(0x00100000) &&
          read32(POSITION + 4u) == UINT32_C(0x00080000) &&
          read32(POSITION + 8u) == UINT32_C(0x00200000) &&
          read32(POSITION_COPY + 0u) == UINT32_C(0x00100000) &&
          read32(POSITION_COPY + 4u) == UINT32_C(0x00080000) &&
          read32(POSITION_COPY + 8u) == UINT32_C(0x00200000),
          "init.position_copies");
    check(read32(SLOT + 0x50u) == 4096u, "init.scale");
}

static void run_latch(u16 latch)
{
    seed();
    write16(SLOT + 4u, latch);
    check(wm_8007ADD4(SLOT_INDEX) == 1, "latch.return");
    check(read16(SLOT + 4u) == 0u, "latch.consumed");
}

static void test_latch_table(void)
{
    run_latch(1u);
    check(read_s16(SLOT + 0x20u) == 1 &&
          read32(SLOT + 0x50u) == 8704u,
          "latch1.scale_up");

    run_latch(2u);
    check(read_s16(SLOT + 0x20u) == 2 &&
          read32(SLOT + 0x50u) == UINT32_C(0x0003FE00),
          "latch2.scale");

    run_latch(3u);
    check(read_s16(SLOT + 0x20u) == 0 &&
          read32(VIEW_HEIGHT) == UINT32_C(0x00640000) &&
          read_s16(ANGLES + 0u) == -480 &&
          read16(ANGLES + 2u) == 1152u &&
          read16(ANGLES + 4u) == 0u,
          "latch3.high_camera");

    run_latch(4u);
    check(read_s16(SLOT + 0x20u) == 0 &&
          read32(VIEW_HEIGHT) == UINT32_C(0x001E0000) &&
          read16(ANGLES + 0u) == 64u &&
          read16(ANGLES + 2u) == 1048u &&
          read32(EVENT_FLAG) == 0u &&
          read32(POSITION + 8u) == read32(RESET_POS + 8u),
          "latch4.reset_camera");

    run_latch(5u);
    check(read_s16(SLOT + 0x20u) == 3 &&
          read32(SLOT + 0x50u) == 7936u,
          "latch5.scale_down");

    run_latch(6u);
    check(read_s16(SLOT + 0x20u) == 4 &&
          read32(SLOT + 0x58u) == UINT32_C(0x001C7C00) &&
          read32(SLOT + 0x5Cu) == UINT32_C(0x005EC400) &&
          read32(EVENT_FLAG) == 1u,
          "latch6.tracking_targets");

    run_latch(7u);
    check(read_s16(SLOT + 0x20u) == 5 &&
          read16(ANGLES + 2u) == 1004u,
          "latch7.angle_ramp");
}

static void test_state_boundaries(void)
{
    seed();
    write16(SLOT + 0x20u, 1u);
    write32(SLOT + 0x50u, 32512u);
    set_random_pair(4, 4);
    (void)wm_8007ADD4(SLOT_INDEX);
    check(read_s16(SLOT + 0x20u) == 0 &&
          read32(SLOT + 0x50u) == 32768u,
          "state.scale_up_clamp");

    seed();
    write16(SLOT + 0x20u, 2u);
    write32(SLOT + 0x50u, 33000u);
    set_random_pair(4, 4);
    (void)wm_8007ADD4(SLOT_INDEX);
    check(read_s16(SLOT + 0x20u) == 0 &&
          read32(SLOT + 0x50u) == 32768u,
          "state.scale_down_clamp");

    seed();
    write16(SLOT + 0x20u, 3u);
    write32(SLOT + 0x50u, 4200u);
    set_random_pair(0, 0);
    (void)wm_8007ADD4(SLOT_INDEX);
    check(read_s16(SLOT + 0x20u) == 0 &&
          read32(SLOT + 0x50u) == 4096u,
          "state.minimum_scale");

    seed();
    write16(SLOT + 0x20u, 5u);
    write16(ANGLES + 2u, 1532u);
    (void)wm_8007ADD4(SLOT_INDEX);
    check(read_s16(SLOT + 0x20u) == 5 &&
          read16(ANGLES + 2u) == 1536u,
          "state.angle_boundary");

    seed();
    write16(SLOT + 0x20u, 5u);
    write16(ANGLES + 2u, 1533u);
    (void)wm_8007ADD4(SLOT_INDEX);
    check(read_s16(SLOT + 0x20u) == 0 &&
          read16(ANGLES + 2u) == 1537u,
          "state.angle_completion");
}

static void test_tracking_camera(void)
{
    seed();
    write16(SLOT + 0x20u, 4u);
    write32(SLOT + 0x58u, UINT32_C(0x001C7C00));
    write32(SLOT + 0x5Cu, UINT32_C(0x005EC400));
    (void)wm_8007ADD4(SLOT_INDEX);

    check(read32(POSITION + 8u) == UINT32_C(0x0023A000),
          "tracking.position_advance");
    check(read_s16(CAMERA_INPUT + 0u) == 199 &&
          read_s16(CAMERA_INPUT + 2u) == 64 &&
          read_s16(CAMERA_INPUT + 4u) == -947 &&
          read_s16(CAMERA_INPUT + 8u) == 0 &&
          read_s16(CAMERA_INPUT + 0x0Au) == 128 &&
          read_s16(CAMERA_INPUT + 0x0Cu) == 0,
          "tracking.camera_input");
    check(look_count == 1u && look_inputs[0] == CAMERA_INPUT &&
          angle_count == 1u && angle_matrix[0] == CAMERA_MATRIX &&
          angle_input[0] == ANGLES && generic_count == 0u,
          "tracking.helper_order");
}

static void test_generic_helper_and_jitter(void)
{
    seed();
    write16(SLOT + 0x20u, 0u);
    (void)wm_8007ADD4(SLOT_INDEX);
    check(generic_count == 1u &&
          generic_calls[0].a0 == CAMERA_INPUT &&
          generic_calls[0].a1 == POSITION &&
          generic_calls[0].a2 == INT32_C(0x001C0000) &&
          generic_calls[0].a3 == ANGLES,
          "generic.arguments");

    seed();
    write16(SLOT + 0x20u, 99u);
    write32(SLOT + 0x50u, 16384u);
    write16(CAMERA_INPUT + 0u, 10u);
    write16(CAMERA_INPUT + 2u, 30u);
    write16(CAMERA_INPUT + 8u, 20u);
    write16(CAMERA_INPUT + 0x0Au, 40u);
    set_random_pair(3, 3);
    (void)wm_8007ADD4(SLOT_INDEX);
    check(read_s16(SC_JITTER + 0u) == 1 &&
          read_s16(SC_JITTER + 2u) == 1 &&
          read16(CAMERA_INPUT + 0u) == 11u &&
          read16(CAMERA_INPUT + 2u) == 31u &&
          read16(CAMERA_INPUT + 8u) == 21u &&
          read16(CAMERA_INPUT + 0x0Au) == 41u,
          "jitter.mirrors");
}

static void test_scheduler_resolution(void)
{
    seed();
    write32(SLOT + 0x18u, UINT32_C(0x8007AD34));
    write32(SLOT + 0x1Cu, UINT32_C(0x8007ADD4));
    write16(SLOT + 0u, 0u);
    wm_sched_reset();
    wm_sched_callback_registry_clear();
    wm_80097800();
    check(wm_sched_get_callbacks_executed() == 1 &&
          wm_sched_get_missing_hits() == 0 &&
          wm_sched_get_invalid_hits() == 0,
          "scheduler.init_resolution");

    seed();
    write32(SLOT + 0x18u, UINT32_C(0x8007AD34));
    write32(SLOT + 0x1Cu, UINT32_C(0x8007ADD4));
    write16(SLOT + 0u, 1u);
    write16(SLOT + 0x20u, 99u);
    wm_sched_reset();
    wm_80097800();
    check(wm_sched_get_callbacks_executed() == 1 &&
          wm_sched_get_missing_hits() == 0 &&
          wm_sched_get_invalid_hits() == 0,
          "scheduler.update_resolution");
}

int main(void)
{
    test_initializer();
    test_latch_table();
    test_state_boundaries();
    test_tracking_camera();
    test_generic_helper_and_jitter();
    test_scheduler_resolution();
    if (failures != 0) {
        fprintf(stderr,
                "W34N75 MODE14 CAMERA CONTROL CERTIFICATE: %d failure(s)\n",
                failures);
        return 1;
    }
    puts("W34N75 MODE14 CAMERA CONTROL CERTIFICATE PASS");
    return 0;
}
