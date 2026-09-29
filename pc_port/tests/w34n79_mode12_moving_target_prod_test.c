/* Focused production certificate for retail 0x8007D774/0x8007D7FC. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_callback_7d774.h"
#include "world_map_scheduler.h"

/* matched C (src/world_map/main.c), linked from the matched object */
s32 func_8007D774(s32 arg0);

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

#define POOL          UINT32_C(0x800A0000)
#define POOL_PTR      UINT32_C(0x8009BE24)
#define SLOT_INDEX    7
#define SLOT          (POOL + (u32)SLOT_INDEX * UINT32_C(0x80))
#define CONTEXT       UINT32_C(0x800A8000)
#define CONTEXT_PTR   UINT32_C(0x8009C620)
#define WORLD_X       UINT32_C(0x8009BE28)
#define WORLD_Z       UINT32_C(0x8009BE30)
#define POSITION_COPY UINT32_C(0x8009D55C)

static int failures;
static unsigned rotation_count;
static SVECTOR *rotation_angles;
static MATRIX *rotation_matrix;
static unsigned wrap_count;
static u32 wrap_address;
static u32 wrap_x_before;
static u32 wrap_z_before;

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
    rotation_count = 0u;
    rotation_angles = NULL;
    rotation_matrix = NULL;
    wrap_count = 0u;
    wrap_address = 0u;
    wrap_x_before = 0u;
    wrap_z_before = 0u;
}

static void seed(void)
{
    memset(g_PsxRam, 0xA5, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0x5A, sizeof(g_PsxScratchpad));
    write32(POOL_PTR, POOL);
    write32(CONTEXT_PTR, CONTEXT);
    reset_trace();
}

MATRIX *RotMatrixYXZ(SVECTOR *angles, MATRIX *matrix)
{
    rotation_count++;
    rotation_angles = angles;
    rotation_matrix = matrix;
    return matrix;
}

void wm_80093354(u32 vec_addr)
{
    wrap_count++;
    wrap_address = vec_addr;
    wrap_x_before = read32(vec_addr + 0u);
    wrap_z_before = read32(vec_addr + 8u);
    if (vec_addr == SLOT + 0x28u) {
        write32(vec_addr + 0u, UINT32_C(0x00200000));
        write32(vec_addr + 8u, UINT32_C(0xFFE00000));
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

static void seed_update_position(void)
{
    write32(SLOT + 0x28u, UINT32_C(0xFFF00000));
    write32(SLOT + 0x2Cu, UINT32_C(0xFFD80000));
    write32(SLOT + 0x30u, UINT32_C(0x00400000));
    write32(SLOT + 0x40u, UINT32_C(0xFFFFA000));
}

static void test_initializer(void)
{
    seed();
    write16(CONTEXT + 0x540u, UINT16_C(0x7777));
    write16(CONTEXT + 0x558u, UINT16_C(0x1111));
    write16(CONTEXT + 0x55Au, UINT16_C(0x2222));
    write16(CONTEXT + 0x55Cu, UINT16_C(0x3333));

    check(func_8007D774(SLOT_INDEX) == 3, "init.return");
    check(read16(CONTEXT + 0x540u) == 1u,
          "init.context_state");
    check(read16(CONTEXT + 0x558u) == 0u &&
          read16(CONTEXT + 0x55Au) == 0u &&
          read16(CONTEXT + 0x55Cu) == 0u,
          "init.angles");
    check(rotation_count == 1u &&
          rotation_angles == (SVECTOR *)PSX_ADDR(CONTEXT + 0x558u) &&
          rotation_matrix == (MATRIX *)PSX_ADDR(CONTEXT + 0x560u),
          "init.rotation");
    check(read32(SLOT + 0x28u) == UINT32_C(0x00D00000) &&
          read32(SLOT + 0x2Cu) == UINT32_C(0xFFD80000) &&
          read32(SLOT + 0x30u) == UINT32_C(0x00400000) &&
          read32(SLOT + 0x38u) == 0u &&
          read32(SLOT + 0x3Cu) == 0u &&
          read32(SLOT + 0x40u) == UINT32_C(0xFFFFA000),
          "init.slot_seed");
}

static void test_update_without_latch(void)
{
    seed();
    seed_update_position();
    write16(SLOT + 0x20u, 0u);
    write32(POSITION_COPY + 0u, UINT32_C(0x11111111));
    write32(POSITION_COPY + 4u, UINT32_C(0x22222222));
    write32(POSITION_COPY + 8u, UINT32_C(0x33333333));

    check(wm_8007D7FC(SLOT_INDEX) == 1, "update.return");
    check(wrap_count == 1u && wrap_address == SLOT + 0x28u &&
          wrap_x_before == UINT32_C(0xFFF00000) &&
          wrap_z_before == UINT32_C(0x003FA000),
          "update.wrap_target");
    check(read32(CONTEXT + 0x548u) == UINT32_C(0x00000200) &&
          read32(CONTEXT + 0x54Cu) == UINT32_C(0xFFFFFD80) &&
          read32(CONTEXT + 0x550u) == UINT32_C(0xFFFFFE00),
          "update.context_position");
    check(read32(POSITION_COPY + 0u) == UINT32_C(0x11111111) &&
          read32(POSITION_COPY + 4u) == UINT32_C(0x22222222) &&
          read32(POSITION_COPY + 8u) == UINT32_C(0x33333333),
          "update.copy_gate");
}

static void test_latch_one(void)
{
    seed();
    seed_update_position();
    write16(SLOT + 4u, 1u);
    write16(CONTEXT + 0x540u, 1u);
    write32(WORLD_X, UINT32_C(0x00100000));
    write32(WORLD_Z, UINT32_C(0x00200000));

    check(wm_8007D7FC(SLOT_INDEX) == 1, "latch1.return");
    check(read16(SLOT + 4u) == 0u &&
          read16(CONTEXT + 0x540u) == 0u,
          "latch1.state_clear");
    check(wrap_x_before == UINT32_C(0x00100000) &&
          wrap_z_before == UINT32_C(0x005FA000),
          "latch1.position_reset");
}

static void test_latch_two(void)
{
    seed();
    seed_update_position();
    write16(SLOT + 4u, 2u);
    write16(SLOT + 0x20u, 0u);
    write32(POSITION_COPY + 0u, 0u);
    write32(POSITION_COPY + 4u, 0u);
    write32(POSITION_COPY + 8u, 0u);

    check(wm_8007D7FC(SLOT_INDEX) == 1, "latch2.return");
    check(read16(SLOT + 4u) == 0u && read16(SLOT + 0x20u) == 2u,
          "latch2.state");
    check(read32(POSITION_COPY + 0u) == UINT32_C(0x00200000) &&
          read32(POSITION_COPY + 4u) == UINT32_C(0xFFD80000) &&
          read32(POSITION_COPY + 8u) == UINT32_C(0xFFE00000),
          "latch2.position_copy");
}

static void test_scheduler_resolution(void)
{
    seed();
    write32(SLOT + 0x18u, UINT32_C(0x8007D774));
    write32(SLOT + 0x1Cu, UINT32_C(0x8007D7FC));
    write16(SLOT + 0u, 0u);
    wm_sched_reset();
    wm_sched_callback_registry_clear();
    wm_80097800();
    check(wm_sched_get_callbacks_executed() == 1 &&
          wm_sched_get_missing_hits() == 0 &&
          wm_sched_get_invalid_hits() == 0,
          "scheduler.init_resolution");

    reset_trace();
    seed_update_position();
    write16(SLOT + 0u, 1u);
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
    test_update_without_latch();
    test_latch_one();
    test_latch_two();
    test_scheduler_resolution();
    if (failures != 0) {
        fprintf(stderr,
                "W34N79 MODE12 MOVING TARGET CERTIFICATE: %d failure(s)\n",
                failures);
        return 1;
    }
    puts("W34N79 MODE12 MOVING TARGET CERTIFICATE PASS");
    return 0;
}
