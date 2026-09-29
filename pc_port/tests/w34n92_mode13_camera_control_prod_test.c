/* Focused production certificate for retail 0x80080578/0x80080600. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_80578.h"
#include "world_map_scheduler.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

#define POOL          UINT32_C(0x800A0000)
#define POOL_PTR      UINT32_C(0x8009BE24)
#define SLOT_INDEX    6
#define SLOT          (POOL + (u32)SLOT_INDEX * UINT32_C(0x80))
#define VIEW_HEIGHT   UINT32_C(0x8009BE0C)
#define RESET_X       UINT32_C(0x8009C5AC)
#define RESET_Y       UINT32_C(0x8009C5B0)
#define RESET_Z       UINT32_C(0x8009C5B4)
#define POSITION      UINT32_C(0x8009BE28)
#define POSITION_COPY UINT32_C(0x8009D55C)
#define ANGLES        UINT32_C(0x8009BD38)
#define CAMERA_INPUT  UINT32_C(0x8009BD40)
#define CAMERA_HEIGHT UINT32_C(0x8009D3F0)
#define CAMERA_GATE   UINT32_C(0x8009D144)
#define JITTER        UINT32_C(0x1F8000A0)

static int failures;
static int random_value;
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
    write32(RESET_X, UINT32_C(0x00110000));
    write32(RESET_Y, UINT32_C(0x00220000));
    write32(RESET_Z, UINT32_C(0x00330000));
    write32(SLOT + 0x7Cu, 4096u);
    camera_calls = 0u;
    camera_output = 0u;
    camera_position = 0u;
    camera_height = 0;
    camera_angles = 0u;
    random_value = 0;
}

int rand(void)
{
    return random_value;
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

static void test_initializer(void)
{
    seed();
    write32(VIEW_HEIGHT, UINT32_C(0xDEADBEEF));
    write32(CAMERA_GATE, UINT32_C(0xDEADBEEF));
    check(wm_80080578(SLOT_INDEX) == 1, "init.return");
    check(read32(VIEW_HEIGHT) == 120u && read32(CAMERA_GATE) == 0u,
          "init.camera_constants");
    check(read32(POSITION + 0u) == UINT32_C(0x00110000) &&
          read32(POSITION + 4u) == UINT32_C(0x00220000) &&
          read32(POSITION + 8u) == UINT32_C(0x00330000) &&
          read32(POSITION_COPY + 0u) == UINT32_C(0x00110000) &&
          read32(POSITION_COPY + 4u) == UINT32_C(0x00220000) &&
          read32(POSITION_COPY + 8u) == UINT32_C(0x00330000),
          "init.positions");
    check(read32(SLOT + 0x7Cu) == 4096u &&
          read16(SLOT + 0x04u) == 1u && read16(SLOT + 0x20u) == 0u,
          "init.slot_state");
}

static void test_latches(void)
{
    seed();
    write32(CAMERA_GATE, 1u);
    write16(SLOT + 0x04u, 1u);
    check(wm_80080600(SLOT_INDEX) == 1, "latch1.return");
    check(read16(SLOT + 0x04u) == 0u &&
          read16(SLOT + 0x20u) == 1u &&
          read16(ANGLES + 0u) == 15u &&
          read16(ANGLES + 2u) == UINT16_C(0x08DF) &&
          read16(ANGLES + 4u) == 0u &&
          read32(SLOT + 0x50u) == UINT32_C(0x007F0000) &&
          read32(CAMERA_HEIGHT) == UINT32_C(0x007FF800),
          "latch1.state");

    seed();
    write32(CAMERA_GATE, 1u);
    write16(SLOT + 0x04u, 2u);
    check(wm_80080600(SLOT_INDEX) == 1 &&
          read16(SLOT + 0x04u) == 0u &&
          read16(SLOT + 0x20u) == 2u &&
          read32(SLOT + 0x7Cu) == UINT32_C(0x00008000),
          "latch2.state");

    seed();
    write32(CAMERA_GATE, 1u);
    write16(SLOT + 0x04u, 3u);
    check(wm_80080600(SLOT_INDEX) == 1 &&
          read16(SLOT + 0x04u) == 0u &&
          read16(SLOT + 0x20u) == 3u &&
          read32(SLOT + 0x7Cu) == UINT32_C(0x0007C000),
          "latch3.state");
}

static void test_camera_gate(void)
{
    seed();
    write16(SLOT + 0x04u, 0u);
    write16(SLOT + 0x20u, 0u);
    write32(CAMERA_GATE, 0u);
    write32(CAMERA_HEIGHT, UINT32_C(0x00550000));
    check(wm_80080600(SLOT_INDEX) == 1 && camera_calls == 1u &&
          camera_output == CAMERA_INPUT && camera_position == POSITION &&
          camera_height == INT32_C(0x00550000) && camera_angles == ANGLES,
          "camera.generic_call");

    camera_calls = 0u;
    write32(CAMERA_GATE, 1u);
    check(wm_80080600(SLOT_INDEX) == 1 && camera_calls == 0u,
          "camera.gate");
}

static void test_state_one(void)
{
    seed();
    write32(CAMERA_GATE, 1u);
    write16(SLOT + 0x20u, 1u);
    write32(SLOT + 0x50u, UINT32_C(0x00300000));
    write32(CAMERA_HEIGHT, UINT32_C(0x00400000));
    write32(SLOT + 0x54u, UINT32_C(0xFFF00000));
    write32(SLOT + 0x58u, 0u);
    write32(SLOT + 0x5Cu, UINT32_C(0x002E0000));
    write32(SLOT + 0x60u, UINT32_C(0x004E0000));
    write32(SLOT + 0x7Cu, UINT32_C(0x00008000));
    write16(CAMERA_INPUT + 2u, 100u);
    write16(CAMERA_INPUT + 0x0Au, 200u);
    random_value = 10;

    check(wm_80080600(SLOT_INDEX) == 1, "state1.return");
    check(read32(SLOT + 0x50u) == UINT32_C(0x00300000) &&
          read32(CAMERA_HEIGHT) == UINT32_C(0x003F8000),
          "state1.primary_clamp");
    check(read32(SLOT + 0x54u) == UINT32_C(0xFFF00000) &&
          read32(SLOT + 0x58u) == UINT32_C(0xFFFF0000) &&
          read16(ANGLES + 0u) == UINT16_C(0xFFF0),
          "state1.roll_approach");
    check(read32(SLOT + 0x5Cu) == UINT32_C(0x002E0000) &&
          read32(SLOT + 0x60u) == UINT32_C(0x004D0000) &&
          read16(ANGLES + 2u) == UINT16_C(0x04D0),
          "state1.pitch_approach");
    check(read16(JITTER + 2u) == UINT16_C(0xFFFE) &&
          read16(CAMERA_INPUT + 2u) == 98u &&
          read16(CAMERA_INPUT + 0x0Au) == 198u,
          "state1.jitter_mirror");
}

static void test_state_three(void)
{
    seed();
    write32(CAMERA_GATE, 1u);
    write16(SLOT + 0x20u, 3u);
    write32(SLOT + 0x7Cu, UINT32_C(0x00004000));
    check(wm_80080600(SLOT_INDEX) == 1 &&
          read16(SLOT + 0x20u) == 0u &&
          read32(SLOT + 0x7Cu) == 4096u,
          "state3.clamp");
}

static void test_scheduler_resolution(void)
{
    seed();
    write32(SLOT + 0x18u, UINT32_C(0x80080578));
    write32(SLOT + 0x1Cu, UINT32_C(0x80080600));
    write16(SLOT + 0u, 0u);
    wm_sched_reset();
    wm_sched_callback_registry_clear();
    wm_80097800();
    check(wm_sched_get_callbacks_executed() == 1 &&
          wm_sched_get_missing_hits() == 0 &&
          wm_sched_get_invalid_hits() == 0 && read16(SLOT + 0u) == 1u,
          "scheduler.init_resolution");

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
    test_latches();
    test_camera_gate();
    test_state_one();
    test_state_three();
    test_scheduler_resolution();
    if (failures != 0) {
        fprintf(stderr,
                "W34N92 MODE13 CAMERA CONTROL CERTIFICATE: %d failure(s)\n",
                failures);
        return 1;
    }
    puts("W34N92 MODE13 CAMERA CONTROL CERTIFICATE PASS");
    return 0;
}
