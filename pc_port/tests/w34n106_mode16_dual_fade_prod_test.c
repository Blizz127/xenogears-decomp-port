/* Focused production certificate for retail 0x800819C8..0x80081C3C. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_callback_819c8.h"
#include "world_map_scheduler.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

#define POOL        UINT32_C(0x800A0000)
#define POOL_PTR    UINT32_C(0x8009BE24)
#define SLOT_INDEX  4
#define SLOT        (POOL + (u32)SLOT_INDEX * UINT32_C(0x80))
#define CONTEXT     UINT32_C(0x800C0000)
#define CONTEXT_PTR UINT32_C(0x8009C620)
#define OWNER_A     CONTEXT
#define OWNER_B     (CONTEXT + UINT32_C(0x54))
#define DESC_A      UINT32_C(0x800C1000)
#define DESC_B      UINT32_C(0x800C1100)
#define STREAM_A0   UINT32_C(0x800C2000)
#define STREAM_A1   UINT32_C(0x800C2100)
#define STREAM_B0   UINT32_C(0x800C2200)
#define STREAM_B1   UINT32_C(0x800C2300)
#define RESET_X     UINT32_C(0x8009C5AC)
#define RESET_Y     UINT32_C(0x8009C5B0)
#define RESET_Z     UINT32_C(0x8009C5B4)
#define BASE_MATRIX UINT32_C(0x8009A180)
#define STREAM_SIDE UINT32_C(0x8009D7F0)
#define SCALE       UINT32_C(0x1F800000)

typedef struct InitCall {
    u32 owner;
    u32 source;
    u16 count;
    s32 abr;
} InitCall;

typedef struct ColorCall {
    u32 stream;
    s32 count;
    u8 red;
    u8 green;
    u8 blue;
} ColorCall;

static int failures;
static InitCall init_calls[4];
static unsigned init_call_count;
static ColorCall color_calls[4];
static unsigned color_call_count;
static unsigned scale_call_count;
static u32 scale_matrix_address;
static u32 scale_vector_address;
static s32 scale_x;
static s32 scale_y;
static s32 scale_z;
static u32 scale_input_word0;

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
    memset(init_calls, 0, sizeof(init_calls));
    memset(color_calls, 0, sizeof(color_calls));
    init_call_count = 0u;
    color_call_count = 0u;
    scale_call_count = 0u;
    scale_matrix_address = 0u;
    scale_vector_address = 0u;
    scale_x = 0;
    scale_y = 0;
    scale_z = 0;
    scale_input_word0 = 0u;
}

static void seed(void)
{
    u32 index;

    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0, sizeof(g_PsxScratchpad));
    write32(POOL_PTR, POOL);
    write32(CONTEXT_PTR, CONTEXT);
    write32(RESET_X, UINT32_C(0x01234000));
    write32(RESET_Y, UINT32_C(0xFFE00000));
    write32(RESET_Z, UINT32_C(0x04567000));
    write32(OWNER_A + 0x40u, DESC_A);
    write32(OWNER_A + 0x48u, STREAM_A0);
    write32(OWNER_A + 0x4Cu, STREAM_A1);
    write16(DESC_A + 4u, 2u);
    write32(OWNER_B + 0x40u, DESC_B);
    write32(OWNER_B + 0x48u, STREAM_B0);
    write32(OWNER_B + 0x4Cu, STREAM_B1);
    write16(DESC_B + 4u, 3u);
    for (index = 0u; index < 8u; index++)
        write32(BASE_MATRIX + index * 4u,
                UINT32_C(0xB0000000) + index);
    reset_trace();
}

void wm_800816DC(u32 owner, u32 source, u16 count, s32 abr)
{
    unsigned call = init_call_count;

    check(call < 4u, "trace.init_capacity");
    if (call < 4u) {
        init_calls[call].owner = owner;
        init_calls[call].source = source;
        init_calls[call].count = count;
        init_calls[call].abr = abr;
    }
    init_call_count++;
}

void wm_800809EC(u32 stream, s32 count, u8 red, u8 green, u8 blue)
{
    unsigned call = color_call_count;

    check(call < 4u, "trace.color_capacity");
    if (call < 4u) {
        color_calls[call].stream = stream;
        color_calls[call].count = count;
        color_calls[call].red = red;
        color_calls[call].green = green;
        color_calls[call].blue = blue;
    }
    color_call_count++;
}

MATRIX *ScaleMatrix(MATRIX *matrix, VECTOR *scale)
{
    u32 index;

    scale_call_count++;
    scale_matrix_address = CONTEXT + 0x20u;
    scale_vector_address = SCALE;
    check(matrix == (MATRIX *)PSX_ADDR(scale_matrix_address),
          "trace.scale_matrix_pointer");
    check(scale == (VECTOR *)PSX_ADDR(scale_vector_address),
          "trace.scale_vector_pointer");
    scale_x = scale->vx;
    scale_y = scale->vy;
    scale_z = scale->vz;
    memcpy(&scale_input_word0, matrix, sizeof(scale_input_word0));
    for (index = 0u; index < 8u; index++) {
        u32 value = UINT32_C(0xC0000000) + index;
        memcpy((uint8_t *)matrix + index * 4u, &value, sizeof(value));
    }
    return matrix;
}

static void test_initializer(void)
{
    u32 index;

    seed();
    check(wm_800819C8(SLOT_INDEX) == 1, "init.return");
    check(read16(SLOT + 0x20u) == 0u &&
          read32(SLOT + 0x28u) == UINT32_C(0x01234000) &&
          read32(SLOT + 0x2Cu) == UINT32_C(0xFFE00000) &&
          read32(SLOT + 0x30u) == UINT32_C(0x04567000),
          "init.position");
    check(read32(SLOT + 0x50u) == 0u &&
          read32(SLOT + 0x54u) == 0u &&
          read32(SLOT + 0x58u) == 0u,
          "init.fade");
    check(scale_call_count == 1u &&
          scale_input_word0 == UINT32_C(0xB0000000) &&
          scale_x == 4096 && scale_y == 28672 && scale_z == 4096,
          "init.scale");
    for (index = 0u; index < 8u; index++) {
        check(read32(CONTEXT + 0x20u + index * 4u) ==
                  UINT32_C(0xC0000000) + index &&
              read32(CONTEXT + 0x74u + index * 4u) ==
                  UINT32_C(0xC0000000) + index,
              "init.matrix_mirror");
    }
    check(init_call_count == 2u &&
          init_calls[0].owner == OWNER_A &&
          init_calls[0].source == STREAM_A0 &&
          init_calls[0].count == 2u && init_calls[0].abr == 3 &&
          init_calls[1].owner == OWNER_B &&
          init_calls[1].source == STREAM_B0 &&
          init_calls[1].count == 3u && init_calls[1].abr == 3,
          "init.streams");
}

static void test_latch_and_fade(void)
{
    seed();
    write16(SLOT + 0x04u, 1u);
    write16(SLOT + 0x20u, 0u);
    write32(SLOT + 0x50u, 10u);
    write32(SLOT + 0x54u, 20u);
    write32(SLOT + 0x58u, 30u);
    write32(STREAM_SIDE, 1u);
    check(wm_80081B24(SLOT_INDEX) == 1 &&
          read16(SLOT + 0x04u) == 0u &&
          read16(SLOT + 0x20u) == 1u,
          "update.latch");
    check(read32(SLOT + 0x50u) == 14u &&
          read32(SLOT + 0x54u) == 22u &&
          read32(SLOT + 0x58u) == 31u,
          "update.fade_steps");
    check(color_call_count == 2u &&
          color_calls[0].stream == STREAM_A1 &&
          color_calls[0].count == 2 && color_calls[0].red == 14u &&
          color_calls[0].green == 22u && color_calls[0].blue == 31u &&
          color_calls[1].stream == STREAM_B1 &&
          color_calls[1].count == 3 && color_calls[1].red == 14u &&
          color_calls[1].green == 22u && color_calls[1].blue == 31u,
          "update.streams");
}

static void test_fade_saturation(void)
{
    seed();
    write16(SLOT + 0x20u, 1u);
    write32(SLOT + 0x50u, 248u);
    write32(SLOT + 0x54u, 10u);
    write32(SLOT + 0x58u, 20u);
    check(wm_80081B24(SLOT_INDEX) == 1 &&
          read16(SLOT + 0x20u) == 0u &&
          read32(SLOT + 0x50u) == 252u &&
          read32(SLOT + 0x54u) == 12u &&
          read32(SLOT + 0x58u) == 21u &&
          color_call_count == 2u && color_calls[0].red == 252u &&
          color_calls[0].green == 12u && color_calls[0].blue == 21u,
          "update.fade_saturation");
}

static void test_scheduler_resolution(void)
{
    seed();
    write32(SLOT + 0x18u, UINT32_C(0x800819C8));
    write32(SLOT + 0x1Cu, UINT32_C(0x80081B24));
    write16(SLOT + 0u, 0u);
    wm_sched_reset();
    wm_sched_callback_registry_clear();
    wm_80097800();
    check(wm_sched_get_callbacks_executed() == 1 &&
          wm_sched_get_missing_hits() == 0 &&
          wm_sched_get_invalid_hits() == 0 && read16(SLOT + 0u) == 1u,
          "scheduler.init_resolution");

    reset_trace();
    wm_sched_reset();
    wm_80097800();
    check(wm_sched_get_callbacks_executed() == 1 &&
          wm_sched_get_missing_hits() == 0 &&
          wm_sched_get_invalid_hits() == 0 && color_call_count == 2u,
          "scheduler.update_resolution");
}

int main(void)
{
    test_initializer();
    test_latch_and_fade();
    test_fade_saturation();
    test_scheduler_resolution();
    if (failures != 0) {
        fprintf(stderr,
                "W34N106 MODE16 DUAL FADE CERTIFICATE: %d failure(s)\n",
                failures);
        return 1;
    }
    puts("W34N106 MODE16 DUAL FADE CERTIFICATE PASS");
    return 0;
}
