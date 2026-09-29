/* Focused production certificate for retail 0x800809EC..0x80080D00. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_callback_80a28.h"
#include "world_map_scheduler.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

#define POOL        UINT32_C(0x800A0000)
#define POOL_PTR    UINT32_C(0x8009BE24)
#define SLOT_INDEX  5
#define SLOT        (POOL + (u32)SLOT_INDEX * UINT32_C(0x80))
#define CONTEXT_PTR UINT32_C(0x8009C620)
#define CONTEXT     UINT32_C(0x800A8000)
#define SECOND      (CONTEXT + UINT32_C(0x54))
#define RESET_X     UINT32_C(0x8009C5AC)
#define RESET_Z     UINT32_C(0x8009C5B4)
#define BASE_MATRIX UINT32_C(0x8009A180)
#define STREAM_SIDE UINT32_C(0x8009D7F0)
#define SCALE_A     UINT32_C(0x1F800000)
#define SCALE_B     UINT32_C(0x1F800010)
#define DESC_A      UINT32_C(0x800B0000)
#define DESC_B      UINT32_C(0x800B0100)
#define STREAM_A0   UINT32_C(0x800B1000)
#define STREAM_A1   UINT32_C(0x800B2000)
#define STREAM_B0   UINT32_C(0x800B3000)
#define STREAM_B1   UINT32_C(0x800B4000)

typedef struct StreamCall {
    u32 owner;
    u32 source;
    u16 count;
} StreamCall;

typedef struct ScaleCall {
    u32 matrix;
    u32 vector;
    s32 x;
    s32 y;
    s32 z;
} ScaleCall;

static int failures;
static StreamCall stream_calls[4];
static unsigned stream_call_count;
static ScaleCall scale_calls[4];
static unsigned scale_call_count;

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

static u8 read8(u32 address)
{
    u8 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
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
    stream_call_count = 0u;
    scale_call_count = 0u;
    memset(stream_calls, 0, sizeof(stream_calls));
    memset(scale_calls, 0, sizeof(scale_calls));
}

static void seed(void)
{
    u32 index;

    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0, sizeof(g_PsxScratchpad));
    write32(POOL_PTR, POOL);
    write32(CONTEXT_PTR, CONTEXT);
    write32(RESET_X, UINT32_C(0x00123000));
    write32(RESET_Z, UINT32_C(0x00456000));
    for (index = 0u; index < 8u; index++)
        write32(BASE_MATRIX + index * 4u,
                UINT32_C(0xB0000000) + index);
    write32(CONTEXT + 0x40u, DESC_A);
    write32(CONTEXT + 0x48u, STREAM_A0);
    write32(CONTEXT + 0x4Cu, STREAM_A1);
    write16(DESC_A + 4u, 2u);
    write32(SECOND + 0x40u, DESC_B);
    write32(SECOND + 0x48u, STREAM_B0);
    write32(SECOND + 0x4Cu, STREAM_B1);
    write16(DESC_B + 4u, 1u);
    memset(PSX_ADDR(STREAM_A0), 0x11, 80u);
    memset(PSX_ADDR(STREAM_A1), 0x22, 80u);
    memset(PSX_ADDR(STREAM_B0), 0x33, 40u);
    memset(PSX_ADDR(STREAM_B1), 0x44, 40u);
    reset_trace();
}

void wm_8007A06C(u32 owner, u32 primitive_source, u16 count)
{
    check(stream_call_count < 4u, "trace.stream_capacity");
    if (stream_call_count < 4u) {
        stream_calls[stream_call_count].owner = owner;
        stream_calls[stream_call_count].source = primitive_source;
        stream_calls[stream_call_count].count = count;
    }
    stream_call_count++;
}

MATRIX *ScaleMatrix(MATRIX *matrix, VECTOR *scale)
{
    unsigned call = scale_call_count;
    u32 index;

    check(call < 4u, "trace.scale_capacity");
    if (call < 4u) {
        scale_calls[call].matrix =
            matrix == (MATRIX *)PSX_ADDR(CONTEXT + 0x20u) ?
            CONTEXT + 0x20u : CONTEXT + 0x74u;
        scale_calls[call].vector =
            scale == (VECTOR *)PSX_ADDR(SCALE_A) ? SCALE_A : SCALE_B;
        scale_calls[call].x = scale->vx;
        scale_calls[call].y = scale->vy;
        scale_calls[call].z = scale->vz;
    }
    for (index = 0u; index < 8u; index++) {
        u32 value = UINT32_C(0xC1000000) + call * UINT32_C(0x01000000) +
                    index;
        memcpy((uint8_t *)matrix + index * 4u, &value, sizeof(value));
    }
    scale_call_count++;
    return matrix;
}

static void test_rgb_helper(void)
{
    seed();
    wm_800809EC(STREAM_A0, 2, 10u, 20u, 30u);
    check(read8(STREAM_A0 + 4u) == 10u &&
          read8(STREAM_A0 + 5u) == 20u &&
          read8(STREAM_A0 + 6u) == 30u &&
          read8(STREAM_A0 + 44u) == 10u &&
          read8(STREAM_A0 + 45u) == 20u &&
          read8(STREAM_A0 + 46u) == 30u,
          "helper.rgb_stride");
    check(read8(STREAM_A0 + 3u) == 0x11u &&
          read8(STREAM_A0 + 7u) == 0x11u &&
          read8(STREAM_A0 + 43u) == 0x11u &&
          read8(STREAM_A0 + 47u) == 0x11u,
          "helper.write_bounds");
}

static void test_initializer(void)
{
    seed();
    check(wm_80080A28(SLOT_INDEX) == 3, "init.return");
    check(read32(SLOT + 0x28u) == UINT32_C(0x00123000) &&
          read32(SLOT + 0x2Cu) == UINT32_C(0xFFF80000) &&
          read32(SLOT + 0x30u) == UINT32_C(0x00456000),
          "init.position");
    check(read32(SLOT + 0x50u) == 0u &&
          read32(SLOT + 0x54u) == UINT32_C(0xFFFFF800) &&
          read32(SLOT + 0x58u) == 128u,
          "init.phases");
    check(stream_call_count == 2u &&
          stream_calls[0].owner == CONTEXT &&
          stream_calls[0].source == STREAM_A0 &&
          stream_calls[0].count == 2u &&
          stream_calls[1].owner == SECOND &&
          stream_calls[1].source == STREAM_B0 &&
          stream_calls[1].count == 1u,
          "init.streams");
}

static void seed_update(u32 brightness)
{
    write16(SLOT + 0x04u, 1u);
    write32(SLOT + 0x28u, UINT32_C(0x00123000));
    write32(SLOT + 0x2Cu, UINT32_C(0xFFFED000));
    write32(SLOT + 0x30u, UINT32_C(0x00456000));
    write32(SLOT + 0x50u, 100u);
    write32(SLOT + 0x54u, UINT32_C(0xFFFFFF38));
    write32(SLOT + 0x58u, brightness);
    write32(STREAM_SIDE, 1u);
    reset_trace();
}

static void test_update(void)
{
    seed();
    seed_update(8u);
    check(wm_80080AC4(SLOT_INDEX) == 1, "update.return");
    check(read16(SLOT + 0x04u) == 0u, "update.latch");
    check(read32(CONTEXT + 0x08u) == UINT32_C(0x00000123) &&
          read32(CONTEXT + 0x0Cu) == UINT32_C(0xFFFFFFED) &&
          read32(CONTEXT + 0x10u) == UINT32_C(0x00000456) &&
          read32(CONTEXT + 0x5Cu) == UINT32_C(0x00000123) &&
          read32(CONTEXT + 0x60u) == UINT32_C(0xFFFFFFED) &&
          read32(CONTEXT + 0x64u) == UINT32_C(0x00000456),
          "update.position");
    check(scale_call_count == 2u &&
          scale_calls[0].matrix == CONTEXT + 0x20u &&
          scale_calls[0].vector == SCALE_A && scale_calls[0].x == 100 &&
          scale_calls[0].y == 4096 && scale_calls[0].z == 100 &&
          scale_calls[1].matrix == CONTEXT + 0x74u &&
          scale_calls[1].vector == SCALE_B && scale_calls[1].x == -200 &&
          scale_calls[1].y == 4096 && scale_calls[1].z == -200,
          "update.scale_order");
    check(read32(SLOT + 0x50u) == 484u &&
          read32(SLOT + 0x54u) == 184u,
          "update.phase_steps");
    check(read8(STREAM_A1 + 4u) == 8u &&
          read8(STREAM_A1 + 44u) == 8u &&
          read8(STREAM_B1 + 4u) == 8u &&
          read8(STREAM_A0 + 4u) == 0x11u &&
          read8(STREAM_B0 + 4u) == 0x33u,
          "update.stream_side");
    check(read32(SLOT + 0x58u) == 4u, "update.fade_step");
}

static void test_clamp_and_terminal(void)
{
    seed();
    seed_update(0u);
    write32(SLOT + 0x50u, 32700u);
    write32(SLOT + 0x54u, 32767u);
    check(wm_80080AC4(SLOT_INDEX) == 3, "terminal.return");
    check(read32(SLOT + 0x50u) == 32767u &&
          read32(SLOT + 0x54u) == 32767u,
          "terminal.phase_clamp");
    check(read32(SLOT + 0x58u) == 0u &&
          read8(STREAM_A1 + 4u) == 0u && read8(STREAM_B1 + 4u) == 0u,
          "terminal.fade_clamp");
}

static void test_scheduler_resolution(void)
{
    seed();
    write32(SLOT + 0x18u, UINT32_C(0x80080A28));
    write32(SLOT + 0x1Cu, UINT32_C(0x80080AC4));
    write16(SLOT + 0u, 0u);
    wm_sched_reset();
    wm_sched_callback_registry_clear();
    wm_80097800();
    check(wm_sched_get_callbacks_executed() == 1 &&
          wm_sched_get_missing_hits() == 0 &&
          wm_sched_get_invalid_hits() == 0 && read16(SLOT + 0u) == 3u,
          "scheduler.init_resolution");

    seed_update(8u);
    write16(SLOT + 0u, 1u);
    wm_sched_reset();
    wm_80097800();
    check(wm_sched_get_callbacks_executed() == 1 &&
          wm_sched_get_missing_hits() == 0 &&
          wm_sched_get_invalid_hits() == 0 && read16(SLOT + 0u) == 1u,
          "scheduler.update_resolution");
}

int main(void)
{
    test_rgb_helper();
    test_initializer();
    test_update();
    test_clamp_and_terminal();
    test_scheduler_resolution();
    if (failures != 0) {
        fprintf(stderr,
                "W34N91 MODE13 SCALED STREAM CERTIFICATE: %d failure(s)\n",
                failures);
        return 1;
    }
    puts("W34N91 MODE13 SCALED STREAM CERTIFICATE PASS");
    return 0;
}
