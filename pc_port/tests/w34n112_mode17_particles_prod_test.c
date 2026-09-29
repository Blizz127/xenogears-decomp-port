/* Focused production certificate for retail 0x80082F64..0x800834D0. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_83214.h"
#include "world_map_scheduler.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

#define POOL        UINT32_C(0x800B0000)
#define POOL_PTR    UINT32_C(0x8009BE24)
#define SLOT_INDEX  3
#define SLOT        (POOL + (u32)SLOT_INDEX * UINT32_C(0x80))
#define CONTEXT     UINT32_C(0x800C0000)
#define CONTEXT_PTR UINT32_C(0x8009C620)
#define OBJECT      (CONTEXT + UINT32_C(0x1A94))
#define DESCRIPTOR  UINT32_C(0x800C4000)
#define BUFFER_A    UINT32_C(0x800C5000)
#define BUFFER_B    UINT32_C(0x800C6000)
#define SIDE        UINT32_C(0x8009D7F0)
#define TABLE_1     UINT32_C(0x8009AABC)
#define TABLE_2     UINT32_C(0x8009AB48)
#define TABLE_4     UINT32_C(0x8009ABD4)
#define SC_VECTOR   UINT32_C(0x1F800000)
#define SC_ANGLES   UINT32_C(0x1F8000A0)
#define SC_MATRIX   UINT32_C(0x1F8000F0)

static int failures;
static u32 tpage_calls;
static int tpage_abr;
static int sine_argument;
static u32 rot_calls;
static u32 scale_calls;
static u32 scale_x;
static u32 scale_y;
static u32 scale_z;

static void check(int condition, const char *name)
{
    if (!condition) {
        fprintf(stderr, "ASSERTION %s FAILED\n", name);
        failures++;
    }
}

static void write8(u32 address, u8 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
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

static void seed(void)
{
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0, sizeof(g_PsxScratchpad));
    write32(POOL_PTR, POOL);
    write32(CONTEXT_PTR, CONTEXT);
    write32(OBJECT + 0x40u, DESCRIPTOR);
    write32(OBJECT + 0x48u, BUFFER_A);
    write32(OBJECT + 0x4Cu, BUFFER_B);
    tpage_calls = 0u;
    tpage_abr = -1;
    sine_argument = -1;
    rot_calls = 0u;
    scale_calls = 0u;
    scale_x = 0u;
    scale_y = 0u;
    scale_z = 0u;
}

static void fill_table(u32 table, const s16 values[14])
{
    u32 index;
    for (index = 0u; index < 14u; index++)
        write16(table + index * 2u, (u16)values[index]);
}

u16 GetTPage(int tp, int abr, int x, int y)
{
    check(tp == 0 && x == 704 && y == 256, "init.tpage_arguments");
    tpage_calls++;
    tpage_abr = abr;
    return UINT16_C(0x4321);
}

int rsin(int angle)
{
    sine_argument = angle;
    return 100;
}

void *RotMatrix(void *angles, void *matrix)
{
    u32 index;
    rot_calls++;
    check(angles == PSX_ADDR(SC_ANGLES) && matrix == PSX_ADDR(SC_MATRIX),
          "transform.rot_arguments");
    for (index = 0u; index < 32u; index++)
        write8(SC_MATRIX + index, (u8)(UINT32_C(0xA0) + index));
    return matrix;
}

void *ScaleMatrix(void *matrix, void *vector)
{
    scale_calls++;
    check(matrix == PSX_ADDR(SC_MATRIX) && vector == PSX_ADDR(SC_VECTOR),
          "transform.scale_arguments");
    scale_x = read32(SC_VECTOR + 0u);
    scale_y = read32(SC_VECTOR + 4u);
    scale_z = read32(SC_VECTOR + 8u);
    return matrix;
}

static void test_initializer(void)
{
    u32 index;
    int layout_ok = 1;

    seed();
    write16(DESCRIPTOR + 4u, 2u);
    memset(PSX_ADDR(BUFFER_A), 0xCC, 64u);
    memset(PSX_ADDR(BUFFER_B), 0x55, 64u);
    check(wm_80083214(SLOT_INDEX) == 1, "init.return");
    check(tpage_calls == 2u && tpage_abr == 3, "init.object_address");
    check(tpage_calls == 2u, "init.primitive_count");
    for (index = 0u; index < 2u; index++) {
        u32 primitive = BUFFER_A + index * 32u;
        if (read8(primitive + 3u) != 7u ||
            read8(primitive + 4u) != 0u ||
            read8(primitive + 5u) != 0u ||
            read8(primitive + 6u) != 0u ||
            read8(primitive + 7u) != 38u ||
            read16(primitive + 22u) != UINT16_C(0x4321))
            layout_ok = 0;
    }
    check(layout_ok != 0, "init.primitive_layout");
    check(memcmp(PSX_ADDR(BUFFER_A), PSX_ADDR(BUFFER_B), 64u) == 0,
          "init.buffer_mirror");
}

static void test_state_table_selection(void)
{
    static const s16 state1[14] = {
        101, 102, 103, 104, 105, 106, 1, 1, 0, 1, 2, 3, 4, 105
    };
    static const s16 state4[14] = {
        401, 402, 403, 104, 105, 106, 1, 1, 0, 1, 2, 3, 4, 405
    };

    seed();
    fill_table(TABLE_1, state1);
    write16(SLOT + 0x04u, 1u);
    (void)wm_80083264(SLOT_INDEX);
    check(read32(OBJECT + 0x08u) == 101u &&
          read32(SLOT + 0x78u) == 105u,
          "state1.table");

    seed();
    fill_table(TABLE_4, state4);
    write16(SLOT + 0x04u, 4u);
    (void)wm_80083264(SLOT_INDEX);
    check(read32(OBJECT + 0x08u) == 401u &&
          read32(SLOT + 0x78u) == 405u,
          "state4.table");
}

static void test_state2_transform_and_color(void)
{
    static const s16 state1[14] = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14
    };
    static const s16 state2[14] = {
        -11, 22, -33, 100, 150, 200, 10, -20, 77,
        0x123, 0x8FC, 0x300, 0x20, 0x444
    };
    u32 index;
    int matrix_ok = 1;
    int colors_ok = 1;

    seed();
    fill_table(TABLE_1, state1);
    fill_table(TABLE_2, state2);
    write16(DESCRIPTOR + 4u, 2u);
    memset(PSX_ADDR(BUFFER_A), 0x11, 64u);
    memset(PSX_ADDR(BUFFER_B), 0x22, 64u);
    write32(SIDE, 1u);
    write16(SLOT + 0x04u, 2u);
    check(wm_80083264(SLOT_INDEX) == 1, "update.return");

    check(read32(OBJECT + 0x08u) == UINT32_C(0xFFFFFFF5) &&
          read32(OBJECT + 0x0Cu) == 22u &&
          read32(OBJECT + 0x10u) == UINT32_C(0xFFFFFFDF),
          "state.table_selection");
    check(read16(SLOT + 0x20u) == 1u && read16(SLOT + 0x04u) == 0u,
          "state.active");
    check(read32(SLOT + 0x78u) == UINT32_C(0x444),
          "state.parameter_copy");
    check(read32(SLOT + 0x50u) == 110u &&
          read32(SLOT + 0x54u) == 130u,
          "motion.channel_step");
    check(read32(SLOT + 0x6Cu) == UINT32_C(0x904),
          "motion.angle");
    check(read32(SLOT + 0x70u) == UINT32_C(0x320),
          "motion.scale");
    check(sine_argument == 0x8FC && rot_calls == 1u && scale_calls == 1u &&
          scale_x == UINT32_C(0x300) && scale_y == 4096u && scale_z == 600u,
          "transform.inputs");
    for (index = 0u; index < 32u; index++)
        if (read8(OBJECT + 0x20u + index) !=
            (u8)(UINT32_C(0xA0) + index))
            matrix_ok = 0;
    check(matrix_ok != 0, "transform.matrix_copy");
    for (index = 0u; index < 2u; index++) {
        u32 primitive = BUFFER_B + index * 32u;
        if (read8(primitive + 4u) != 110u ||
            read8(primitive + 5u) != 130u ||
            read8(primitive + 6u) != 200u)
            colors_ok = 0;
    }
    if (read8(BUFFER_A + 4u) != 0x11u)
        colors_ok = 0;
    check(colors_ok != 0, "color.buffer_side");
}

static void test_oscillator_clamps_and_clear(void)
{
    seed();
    write16(DESCRIPTOR + 4u, 0u);
    write16(SLOT + 0x20u, 1u);
    write32(SLOT + 0x50u, 60u);
    write32(SLOT + 0x54u, 250u);
    write32(SLOT + 0x5Cu, UINT32_C(0xFFFFFFF6));
    write32(SLOT + 0x60u, 10u);
    write32(SLOT + 0x6Cu, UINT32_C(0x100));
    write32(SLOT + 0x70u, UINT32_C(0x200));
    write32(SLOT + 0x74u, UINT32_C(0x10));
    (void)wm_80083264(SLOT_INDEX);
    check(read32(SLOT + 0x50u) == 64u &&
          read32(SLOT + 0x5Cu) == 10u &&
          read32(SLOT + 0x54u) == 255u &&
          read32(SLOT + 0x60u) == UINT32_C(0xFFFFFFF6),
          "oscillator.clamp");

    rot_calls = 0u;
    write16(SLOT + 0x04u, 3u);
    write16(SLOT + 0x20u, 1u);
    (void)wm_80083264(SLOT_INDEX);
    check(read16(SLOT + 0x04u) == 0u &&
          read16(SLOT + 0x20u) == 0u && rot_calls == 0u,
          "state3.clear");
}

static void test_scheduler_resolution(void)
{
    seed();
    write16(DESCRIPTOR + 4u, 0u);
    write32(SLOT + 0x18u, UINT32_C(0x80083214));
    write32(SLOT + 0x1Cu, UINT32_C(0x80083264));
    write16(SLOT + 0x00u, 0u);
    wm_sched_reset();
    wm_sched_callback_registry_clear();
    wm_80097800();
    check(wm_sched_get_callbacks_executed() == 1 &&
          wm_sched_get_missing_hits() == 0 &&
          wm_sched_get_invalid_hits() == 0,
          "scheduler.init_resolution");

    write16(SLOT + 0x04u, 3u);
    wm_sched_reset();
    wm_80097800();
    check(wm_sched_get_callbacks_executed() == 1 &&
          wm_sched_get_missing_hits() == 0 &&
          wm_sched_get_invalid_hits() == 0 &&
          read16(SLOT + 0x04u) == 0u,
          "scheduler.update_resolution");
}

int main(void)
{
    test_initializer();
    test_state_table_selection();
    test_state2_transform_and_color();
    test_oscillator_clamps_and_clear();
    test_scheduler_resolution();
    if (failures != 0) {
        fprintf(stderr, "W34N112 MODE17 PARTICLE CERTIFICATE: %d failure(s)\n",
                failures);
        return 1;
    }
    puts("W34N112 MODE17 PARTICLE CERTIFICATE PASS");
    return 0;
}
