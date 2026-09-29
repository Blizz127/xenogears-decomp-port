/* Focused production certificate for retail 0x8007F8AC/0x8007F968. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_callback_7f8ac.h"
#include "world_map_scheduler.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

#define POOL        UINT32_C(0x800A0000)
#define POOL_PTR    UINT32_C(0x8009BE24)
#define CONTEXT     UINT32_C(0x800B0000)
#define CONTEXT_PTR UINT32_C(0x8009C620)
#define SCALE_TABLE UINT32_C(0x8009A684)
#define DESC_BASE   UINT32_C(0x800B1000)
#define SRC_BASE    UINT32_C(0x800B2000)
#define SLOT(n)     (POOL + (u32)(n) * UINT32_C(0x80))
#define OWNER(n)    (CONTEXT + (u32)(n) * UINT32_C(0x54))

static int failures;
static unsigned stream_calls;
static u32 stream_owner;
static u32 stream_source;
static u16 stream_count;
static u32 stream_abr;
static unsigned rot_calls;
static unsigned mul_calls;
static unsigned scale_calls;
static u16 mul_left_m00;
static s32 scale_x;
static s32 scale_y;
static s32 scale_z;
static unsigned marker_count;
static u32 marker_id[8];
static u32 marker_position[8];
static u32 marker_rotation[8];
static unsigned release_count;
static u32 release_id[8];

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
    stream_calls = 0u;
    stream_owner = 0u;
    stream_source = 0u;
    stream_count = 0u;
    stream_abr = 0u;
    rot_calls = 0u;
    mul_calls = 0u;
    scale_calls = 0u;
    mul_left_m00 = 0u;
    scale_x = 0;
    scale_y = 0;
    scale_z = 0;
    marker_count = 0u;
    release_count = 0u;
}

static void seed(void)
{
    u32 slot;

    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0, sizeof(g_PsxScratchpad));
    write32(POOL_PTR, POOL);
    write32(CONTEXT_PTR, CONTEXT);
    for (slot = 4u; slot <= 8u; slot++) {
        u32 descriptor = DESC_BASE + slot * 0x20u;
        u32 source = SRC_BASE + slot * 0x100u;
        write32(OWNER(slot) + 0x40u, descriptor);
        write32(OWNER(slot) + 0x48u, source);
        write16(descriptor + 4u, (u16)(slot + 1u));
        write16(SCALE_TABLE + slot * 2u, (u16)(0x200u + slot * 0x20u));
    }
    write32(SLOT(3) + 0x28u, UINT32_C(0x01000000));
    write32(SLOT(3) + 0x2Cu, UINT32_C(0xFFF00000));
    write32(SLOT(3) + 0x30u, UINT32_C(0x02000000));
    reset_trace();
}

void wm_8007EBBC(u32 owner, u32 primitive_source, u16 count, u32 abr)
{
    stream_calls++;
    stream_owner = owner;
    stream_source = primitive_source;
    stream_count = count;
    stream_abr = abr;
}

MATRIX *RotMatrixYXZ(SVECTOR *rotation, MATRIX *matrix)
{
    (void)rotation;
    memset(matrix, 0, sizeof(*matrix));
    matrix->m[0][0] = 4096;
    matrix->m[1][1] = 4096;
    matrix->m[2][2] = 4096;
    rot_calls++;
    return matrix;
}

MATRIX *MulMatrix0(MATRIX *left, MATRIX *right, MATRIX *output)
{
    (void)right;
    mul_left_m00 = (u16)left->m[0][0];
    memcpy(output, left, sizeof(*output));
    mul_calls++;
    return output;
}

MATRIX *ScaleMatrix(MATRIX *matrix, VECTOR *scale)
{
    scale_x = scale->vx;
    scale_y = scale->vy;
    scale_z = scale->vz;
    scale_calls++;
    return matrix;
}

int ratan2(int y, int x)
{
    check(y == 2138 && x == 3494, "follow.ratan2_args");
    return 0x321;
}

void wm_80089160(u32 id, u32 position, u32 rotation)
{
    marker_id[marker_count] = id;
    marker_position[marker_count] = position;
    marker_rotation[marker_count] = rotation;
    marker_count++;
}

void wm_800894C8(u32 id)
{
    release_id[release_count] = id;
    release_count++;
}

static void test_initializer(void)
{
    seed();
    check(wm_8007F8AC(4) == 1, "init.return");
    check(stream_calls == 1u && stream_owner == OWNER(4) &&
          stream_source == SRC_BASE + 4u * 0x100u &&
          stream_count == 5u, "init.stream_owner");
    check(stream_abr == 3u, "init.normal_abr");
    check(read32(SLOT(4) + 0x38u) == UINT32_C(0xFFFFF7A6) &&
          read32(SLOT(4) + 0x3Cu) == 0u &&
          read32(SLOT(4) + 0x40u) == UINT32_C(0x00000DA6) &&
          read16(SLOT(4) + 0x20u) == 0u, "init.velocity_state");
    check(read16(SLOT(4) + 0x22u) == 60u, "init.timer");
    check(read32(SLOT(4) + 0x5Cu) == 0x280u, "init.scale_table");

    reset_trace();
    check(wm_8007F8AC(8) == 1 && stream_owner == OWNER(8) &&
          stream_abr == 1u, "init.slot8_abr");
}

static void prepare_slot(void)
{
    seed();
    (void)wm_8007F8AC(4);
    reset_trace();
}

static void test_follow_and_render(void)
{
    prepare_slot();
    check(wm_8007F968(4) == 1, "follow.return");
    check(read32(SLOT(4) + 0x28u) == read32(SLOT(3) + 0x28u) &&
          read32(SLOT(4) + 0x2Cu) == read32(SLOT(3) + 0x2Cu) &&
          read32(SLOT(4) + 0x30u) == read32(SLOT(3) + 0x30u),
          "follow.primary_position");
    check(read16(UINT32_C(0x1F8000A0)) == 0x142Du &&
          read16(UINT32_C(0x1F8000A2)) == 0xFF00u &&
          read16(UINT32_C(0x1F8000A4)) == 0x192Du,
          "follow.marker_position");
    check(marker_count == 2u && marker_id[0] == 35u && marker_id[1] == 36u &&
          marker_position[0] == UINT32_C(0x1F8000A0) &&
          marker_rotation[0] == UINT32_C(0x1F8000A8),
          "follow.marker_calls");
    check(read32(OWNER(4) + 0x08u) == 0x1000u &&
          read32(OWNER(4) + 0x0Cu) == UINT32_C(0xFFFFFF00) &&
          read32(OWNER(4) + 0x10u) == 0x2000u,
          "render.position_publish");
    check(rot_calls == 1u && mul_calls == 1u && scale_calls == 1u &&
          mul_left_m00 == 3494u, "render.fixed_matrix");
    check(scale_x == 640 && scale_y == 640 && scale_z == 8192,
          "render.scale_vector");
}

static void test_latches(void)
{
    prepare_slot();
    write16(SLOT(4) + 0x04u, 2u);
    (void)wm_8007F968(4);
    check(read16(SLOT(4) + 0x04u) == 0u &&
          read16(SLOT(4) + 0x20u) == 1u &&
          read16(SLOT(4) + 0x22u) == 59u, "latch2.state");

    prepare_slot();
    write16(SLOT(4) + 0x04u, 3u);
    write16(OWNER(4) + 0x18u, 9u);
    write16(OWNER(4) + 0x1Au, 9u);
    write16(OWNER(4) + 0x1Cu, 9u);
    (void)wm_8007F968(4);
    check(read16(SLOT(4) + 0x20u) == 0u &&
          read16(SLOT(4) + 0x22u) == 60u &&
          read16(OWNER(4) + 0x18u) == 0u &&
          read16(OWNER(4) + 0x1Au) == 0u &&
          read16(OWNER(4) + 0x1Cu) == 256u,
          "latch3.reset_state");
    check(release_count == 2u && release_id[0] == 35u &&
          release_id[1] == 36u && rot_calls == 2u,
          "latch3.effects_matrix");

    prepare_slot();
    write16(SLOT(4) + 0x04u, 4u);
    (void)wm_8007F968(4);
    check(read16(SLOT(4) + 0x20u) == 1u &&
          read16(SLOT(4) + 0x22u) == 3u, "latch4.fade_timer");

    prepare_slot();
    write16(SLOT(4) + 0x04u, 5u);
    (void)wm_8007F968(4);
    check(read16(SLOT(4) + 0x20u) == 1u &&
          read16(SLOT(4) + 0x22u) == 29u, "latch5.fade_timer");
}

static void test_fade_terminal(void)
{
    prepare_slot();
    write16(SLOT(4) + 0x20u, 1u);
    write16(SLOT(4) + 0x22u, 0u);
    write32(SLOT(4) + 0x5Cu, 128u);
    check(wm_8007F968(4) == 1 && read32(SLOT(4) + 0x5Cu) == 0u,
          "fade.scale_step");
    check(wm_8007F968(4) == 3 && read32(SLOT(4) + 0x5Cu) == 0u &&
          read16(OWNER(4) + 0x00u) == 1u, "fade.terminal_release");
}

static void test_scheduler_resolution(void)
{
    seed();
    write32(SLOT(4) + 0x18u, UINT32_C(0x8007F8AC));
    write32(SLOT(4) + 0x1Cu, UINT32_C(0x8007F968));
    write16(SLOT(4) + 0x00u, 0u);
    wm_sched_reset();
    wm_sched_callback_registry_clear();
    wm_80097800();
    check(wm_sched_get_callbacks_executed() == 1 &&
          wm_sched_get_missing_hits() == 0 && read16(SLOT(4)) == 1u,
          "scheduler.init_resolution");
    wm_sched_reset();
    wm_80097800();
    check(wm_sched_get_callbacks_executed() == 1 &&
          wm_sched_get_missing_hits() == 0,
          "scheduler.update_resolution");
}

int main(void)
{
    test_initializer();
    test_follow_and_render();
    test_latches();
    test_fade_terminal();
    test_scheduler_resolution();
    if (failures != 0) {
        fprintf(stderr, "W34N101 MODE15 TRAIL CERTIFICATE: %d failure(s)\n",
                failures);
        return 1;
    }
    puts("W34N101 MODE15 TRAIL CERTIFICATE PASS");
    return 0;
}
