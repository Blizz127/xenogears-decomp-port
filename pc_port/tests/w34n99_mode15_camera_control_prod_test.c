/* Focused production certificate for retail 0x8007E450/0x8007E4E4. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_7e450.h"
#include "world_map_scheduler.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

#define POOL          UINT32_C(0x800A0000)
#define POOL_PTR      UINT32_C(0x8009BE24)
#define SLOT_INDEX    2
#define SLOT          (POOL + (u32)SLOT_INDEX * UINT32_C(0x80))
#define RESET_POS     UINT32_C(0x8009C5AC)
#define LIVE_POS      UINT32_C(0x8009BE28)
#define SHADOW_POS    UINT32_C(0x8009D55C)
#define VIEW_Y        UINT32_C(0x8009BE0C)
#define CAMERA_HEIGHT UINT32_C(0x8009D3F0)
#define CAMERA_GATE   UINT32_C(0x8009D144)
#define ANGLES        UINT32_C(0x8009BD38)
#define CAMERA_INPUT  UINT32_C(0x8009BD40)
#define WORLD_X_ACCUM UINT32_C(0x8009BBB4)
#define WORLD_Z_ACCUM UINT32_C(0x8009BBBC)

static int failures;
static int random_value = 6;
static unsigned wrap_calls;
static unsigned camera_calls;
static u32 camera_out;
static u32 camera_pos;
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

static void reset_trace(void)
{
    wrap_calls = 0u;
    camera_calls = 0u;
    camera_out = 0u;
    camera_pos = 0u;
    camera_height = 0;
    camera_angles = 0u;
}

static void seed(void)
{
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0, sizeof(g_PsxScratchpad));
    write32(POOL_PTR, POOL);
    write32(RESET_POS + 0u, UINT32_C(0x02000000));
    write32(RESET_POS + 4u, UINT32_C(0xFFF80000));
    write32(RESET_POS + 8u, UINT32_C(0x05000000));
    reset_trace();
}

int rand(void)
{
    return random_value;
}

void wm_80093354(u32 vec_addr)
{
    check(vec_addr == SLOT + 0x28u, "camera.wrap_arg");
    wrap_calls++;
}

void wm_80096F18(u32 out_matrix, u32 pos_vec, s32 height, u32 rot_svec)
{
    camera_calls++;
    camera_out = out_matrix;
    camera_pos = pos_vec;
    camera_height = height;
    camera_angles = rot_svec;
}

static void init_slot(void)
{
    check(wm_8007E450(SLOT_INDEX) == 1, "init.return");
}

static void run_latch(u16 latch)
{
    write16(SLOT + 4u, latch);
    write32(CAMERA_GATE, 1u);
    write16(SLOT + 0x20u, 0u);
    write32(SLOT + 0x7Cu, 4096u);
    (void)wm_8007E4E4(SLOT_INDEX);
}

static void run_state(u16 state)
{
    write16(SLOT + 4u, 0u);
    write16(SLOT + 0x20u, state);
    write32(CAMERA_GATE, 1u);
    reset_trace();
    (void)wm_8007E4E4(SLOT_INDEX);
}

static void test_initializer(void)
{
    seed();
    init_slot();
    check(read32(CAMERA_GATE) == 0u && read32(VIEW_Y) == 120u &&
          read32(SLOT + 0x7Cu) == 4096u && read16(SLOT + 4u) == 1u &&
          read32(SLOT + 0x58u) == 64u &&
          read16(SLOT + 0x20u) == 0u && read32(SLOT + 0x50u) == 0u,
          "init.controls");
    check(read32(LIVE_POS + 0u) == UINT32_C(0x02000000) &&
          read32(LIVE_POS + 4u) == UINT32_C(0xFFF80000) &&
          read32(LIVE_POS + 8u) == UINT32_C(0x05000000) &&
          read32(SHADOW_POS + 0u) == read32(LIVE_POS + 0u) &&
          read32(SHADOW_POS + 4u) == read32(LIVE_POS + 4u) &&
          read32(SHADOW_POS + 8u) == read32(LIVE_POS + 8u),
          "init.positions");
}

static void test_static_latches_and_camera(void)
{
    seed();
    init_slot();
    run_latch(1u);
    check(read16(SLOT + 4u) == 0u &&
          read32(CAMERA_HEIGHT) == UINT32_C(0x00500000) &&
          read16(ANGLES + 0u) == 336u && read16(ANGLES + 2u) == 3920u &&
          read16(ANGLES + 4u) == 0u, "latch1.camera_state");
    run_latch(2u);
    check(read32(CAMERA_HEIGHT) == UINT32_C(0x004D0000) &&
          read16(ANGLES + 0u) == 3168u && read16(ANGLES + 2u) == 896u,
          "latch2.camera_state");
    run_latch(3u);
    check(read32(CAMERA_HEIGHT) == UINT32_C(0x00400000) &&
          read16(ANGLES + 0u) == 48u && read16(ANGLES + 2u) == 352u,
          "latch3.camera_state");
    run_latch(4u);
    check(read16(ANGLES + 0u) == 48u && read16(ANGLES + 2u) == 2400u,
          "latch4.camera_state");

    write16(SLOT + 4u, 1u);
    write16(SLOT + 0x20u, 0u);
    write32(SLOT + 0x7Cu, 4096u);
    write32(CAMERA_GATE, 0u);
    reset_trace();
    (void)wm_8007E4E4(SLOT_INDEX);
    check(wrap_calls == 1u && camera_calls == 1u &&
          camera_out == CAMERA_INPUT && camera_pos == LIVE_POS &&
          camera_height == INT32_C(0x00500000) && camera_angles == ANGLES,
          "camera.generic_call");
}

static void test_dynamic_latches(void)
{
    seed();
    init_slot();
    write32(LIVE_POS + 0u, UINT32_C(0x03000000));
    write32(LIVE_POS + 8u, UINT32_C(0x06000000));
    write16(ANGLES + 0u, 100u);
    write16(ANGLES + 2u, 1000u);
    run_latch(5u);
    check(read16(SLOT + 0x20u) == 1u &&
          read32(SLOT + 0x5Cu) == UINT32_C(0xFFFF8300),
          "latch5.state_speed");

    write32(LIVE_POS + 0u, UINT32_C(0x03000000));
    write32(LIVE_POS + 4u, UINT32_C(0x00010000));
    write32(LIVE_POS + 8u, UINT32_C(0x06000000));
    run_latch(6u);
    check(read16(SLOT + 0x20u) == 3u &&
          read32(CAMERA_HEIGHT) == UINT32_C(0x00220000) &&
          read16(ANGLES + 0u) == (u16)(s16)-92 &&
          read16(ANGLES + 2u) == 3704u, "latch6.state_camera");

    write16(SLOT + 4u, 7u);
    write16(SLOT + 0x20u, 0u);
    write32(SLOT + 0x7Cu, UINT32_C(0x00010000));
    write32(CAMERA_GATE, 1u);
    (void)wm_8007E4E4(SLOT_INDEX);
    check(read16(SLOT + 0x20u) == 5u &&
          read32(CAMERA_HEIGHT) == UINT32_C(0x00040000) &&
          read32(SLOT + 0x7Cu) == UINT32_C(0x0000E000),
          "latch7.state_amplitude");
}

static void test_path_states(void)
{
    seed();
    init_slot();
    write32(LIVE_POS + 0u, UINT32_C(0x03000000));
    write32(LIVE_POS + 8u, UINT32_C(0x06000000));
    write32(WORLD_X_ACCUM, UINT32_C(0x01000000));
    write32(WORLD_Z_ACCUM, UINT32_C(0x02000000));
    write32(SLOT + 0x5Cu, 0u);
    write16(ANGLES + 0u, 32u);
    write16(ANGLES + 2u, 64u);
    write32(CAMERA_HEIGHT, UINT32_C(0x00400000));
    run_state(1u);
    check(read32(LIVE_POS + 0u) == UINT32_C(0x02FBD300) &&
          read32(LIVE_POS + 8u) == UINT32_C(0x0606D300) &&
          read32(WORLD_X_ACCUM) == UINT32_C(0x00FBD300) &&
          read32(WORLD_Z_ACCUM) == UINT32_C(0x0206D300),
          "state1.path_delta");

    write32(LIVE_POS + 0u, UINT32_C(0x01FE0CFF));
    write32(LIVE_POS + 8u, UINT32_C(0x0592AD01));
    write32(SLOT + 0x5Cu, 0u);
    run_state(1u);
    check(read16(SLOT + 0x20u) == 2u &&
          read32(LIVE_POS + 0u) == UINT32_C(0x01F9E000) &&
          read32(LIVE_POS + 8u) == UINT32_C(0x05998000) &&
          read32(SLOT + 0x7Cu) == UINT32_C(0x00010000),
          "state1.transition");

    write32(LIVE_POS + 0u, UINT32_C(0x02194CFF));
    write32(LIVE_POS + 8u, UINT32_C(0x05A35D01));
    write32(SLOT + 0x5Cu, 0u);
    run_state(24u);
    check(read16(SLOT + 0x20u) == 25u &&
          read32(LIVE_POS + 0u) == UINT32_C(0x02152000) &&
          read32(LIVE_POS + 8u) == UINT32_C(0x05AA3000) &&
          read32(SLOT + 0x7Cu) == UINT32_C(0x00010000) &&
          read32(SLOT + 0x60u) == UINT32_C(0x00010000),
          "state24.transition");
}

static void test_state_three_and_decays(void)
{
    seed();
    init_slot();
    write32(LIVE_POS + 0u, UINT32_C(0x03000000));
    write32(LIVE_POS + 4u, UINT32_C(0x00100000));
    write32(LIVE_POS + 8u, UINT32_C(0x06000000));
    write32(CAMERA_HEIGHT, UINT32_C(0x00200000));
    write16(ANGLES + 0u, 0u);
    write16(ANGLES + 2u, 0u);
    run_state(3u);
    check(read32(LIVE_POS + 0u) == UINT32_C(0x02FB3C00) &&
          read32(LIVE_POS + 4u) == UINT32_C(0x0010FF80) &&
          read32(LIVE_POS + 8u) == UINT32_C(0x05F9AA80),
          "state3.position_delta");

    write32(SLOT + 0x7Cu, 4096u);
    run_state(5u);
    check(read16(SLOT + 0x20u) == 0u &&
          read32(SLOT + 0x7Cu) == 4096u, "state5.clamp");

    write32(SLOT + 0x7Cu, 4096u);
    run_state(17u);
    check(read16(SLOT + 0x20u) == 17u &&
          read32(SLOT + 0x7Cu) == 4096u, "state17.clamp_keeps_state");

    write16(ANGLES + 2u, 100u);
    write32(SLOT + 0x60u, 512u);
    run_state(25u);
    check(read16(ANGLES + 2u) == 100u && read32(SLOT + 0x60u) == 0u,
          "state25.speed_decay");
}

static void test_jitter(void)
{
    seed();
    init_slot();
    write16(SLOT + 4u, 0u);
    write16(SLOT + 0x20u, 0u);
    write32(SLOT + 0x7Cu, 32768u);
    write32(CAMERA_GATE, 1u);
    write16(CAMERA_INPUT + 2u, 100u);
    write16(CAMERA_INPUT + 0x0Au, 200u);
    (void)wm_8007E4E4(SLOT_INDEX);
    check(read16(CAMERA_INPUT + 2u) == 102u &&
          read16(CAMERA_INPUT + 0x0Au) == 202u,
          "jitter.mirrored_offset");
}

static void test_scheduler_resolution(void)
{
    seed();
    write32(SLOT + 0x18u, UINT32_C(0x8007E450));
    write32(SLOT + 0x1Cu, UINT32_C(0x8007E4E4));
    write16(SLOT + 0u, 0u);
    wm_sched_reset();
    wm_sched_callback_registry_clear();
    wm_80097800();
    check(wm_sched_get_callbacks_executed() == 1 &&
          wm_sched_get_missing_hits() == 0 &&
          wm_sched_get_invalid_hits() == 0 && read16(SLOT + 0u) == 1u,
          "scheduler.init_resolution");
    write32(CAMERA_GATE, 1u);
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
    test_static_latches_and_camera();
    test_dynamic_latches();
    test_path_states();
    test_state_three_and_decays();
    test_jitter();
    test_scheduler_resolution();
    if (failures != 0) {
        fprintf(stderr,
                "W34N99 MODE15 CAMERA CONTROL CERTIFICATE: %d failure(s)\n",
                failures);
        return 1;
    }
    puts("W34N99 MODE15 CAMERA CONTROL CERTIFICATE PASS");
    return 0;
}
