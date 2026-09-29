/* Focused production certificate for retail 0x8007D414/0x8007D4A4. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_callback_7d414.h"
#include "world_map_scheduler.h"

/* matched C (src/world_map/main.c), linked from the matched object */
s32 func_8007D414(s32 arg0);

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

#define POOL        UINT32_C(0x800A0000)
#define POOL_PTR    UINT32_C(0x8009BE24)
#define SLOT_INDEX  12
#define SLOT        (POOL + (u32)SLOT_INDEX * UINT32_C(0x80))
#define CONTEXT     UINT32_C(0x800A8000)
#define CONTEXT_PTR UINT32_C(0x8009C620)
#define SCRATCH_VEC UINT32_C(0x1F8000A0)

static int failures;
static unsigned link_count;
static s32 link_source;
static s32 link_destination;
static unsigned rotation_count;
static SVECTOR *rotation_angles;
static MATRIX *rotation_matrix;
static unsigned wrap_count;
static u32 wrap_address;
static u32 wrap_z_before;
static unsigned height_count;
static u32 height_x;
static u32 height_z;
static unsigned marker_count;
static u32 marker_id;
static u32 marker_vector;
static u32 marker_flags;
static unsigned destroy_count;
static u32 destroy_record;

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
    link_count = 0u;
    link_source = 0;
    link_destination = 0;
    rotation_count = 0u;
    rotation_angles = NULL;
    rotation_matrix = NULL;
    wrap_count = 0u;
    wrap_address = 0u;
    wrap_z_before = 0u;
    height_count = 0u;
    height_x = 0u;
    height_z = 0u;
    marker_count = 0u;
    marker_id = 0u;
    marker_vector = 0u;
    marker_flags = 0u;
    destroy_count = 0u;
    destroy_record = 0u;
}

static void seed(void)
{
    memset(g_PsxRam, 0xA5, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0x5A, sizeof(g_PsxScratchpad));
    write32(POOL_PTR, POOL);
    write32(CONTEXT_PTR, CONTEXT);
    reset_trace();
}

void wm_800848B4(s32 source_record, s32 destination_record)
{
    link_count++;
    link_source = source_record;
    link_destination = destination_record;
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
    wrap_z_before = read32(vec_addr + 8u);
    if (vec_addr == SLOT + 0x28u) {
        write32(vec_addr + 0u, UINT32_C(0xFFF00000));
        write32(vec_addr + 8u, UINT32_C(0x00400000));
    }
}

s32 wm_80093A5C(u32 x_bits, u32 z_bits)
{
    height_count++;
    height_x = x_bits;
    height_z = z_bits;
    return (s32)UINT32_C(0x0000A000);
}

void wm_80089160(u32 id, u32 vector, u32 flags)
{
    marker_count++;
    marker_id = id;
    marker_vector = vector;
    marker_flags = flags;
}

void wm_800894C8(u32 record_index)
{
    destroy_count++;
    destroy_record = record_index;
}

static void test_initializer(void)
{
    seed();
    write16(CONTEXT + 0x3F0u, UINT16_C(0x7777));
    write16(CONTEXT + 0x408u, UINT16_C(0x1111));
    write16(CONTEXT + 0x40Au, UINT16_C(0x2222));
    write16(CONTEXT + 0x40Cu, UINT16_C(0x3333));

    check(func_8007D414(SLOT_INDEX) == 1, "init.return");
    /* func_800848B4 stores record src's guest address at record dst +0x50 */
    check(read32(CONTEXT + 15u * 0x54u + 0x50u) == CONTEXT + 12u * 0x54u,
          "init.link");
    check(read16(CONTEXT + 0x3F0u) == 0u &&
          read16(CONTEXT + 0x408u) == 0u &&
          read16(CONTEXT + 0x40Au) == 0u &&
          read16(CONTEXT + 0x40Cu) == 0u,
          "init.context_zeroes");
    check(rotation_count == 1u &&
          rotation_angles == (SVECTOR *)PSX_ADDR(CONTEXT + 0x408u) &&
          rotation_matrix == (MATRIX *)PSX_ADDR(CONTEXT + 0x410u),
          "init.rotation");
    check(read32(SLOT + 0x40u) == UINT32_C(0xFFFFC000) &&
          read32(SLOT + 0x28u) == UINT32_C(0x00E80000) &&
          read32(SLOT + 0x2Cu) == 0u &&
          read32(SLOT + 0x30u) == UINT32_C(0x00280000) &&
          read32(SLOT + 0x38u) == 0u &&
          read32(SLOT + 0x3Cu) == 0u,
          "init.slot_seed");
}

static void test_moving_branch(void)
{
    seed();
    write16(SLOT + 4u, 0u);
    write32(SLOT + 0x28u, UINT32_C(0x00E80000));
    write32(SLOT + 0x2Cu, 0u);
    write32(SLOT + 0x30u, UINT32_C(0x00280000));
    write32(SLOT + 0x40u, UINT32_C(0xFFFFC000));

    check(wm_8007D4A4(SLOT_INDEX) == 1, "move.return");
    check(wrap_count == 1u && wrap_address == SLOT + 0x28u &&
          wrap_z_before == UINT32_C(0x0027C000),
          "move.wrap");
    check(height_count == 1u &&
          height_x == UINT32_C(0xFFF00000) &&
          height_z == UINT32_C(0x00400000),
          "move.height_input");
    check(read32(SLOT + 0x2Cu) == UINT32_C(0x00006000),
          "move.height_bias");
    check(read32(CONTEXT + 0x3F8u) == UINT32_C(0xFFFFFF00) &&
          read32(CONTEXT + 0x3FCu) == 6u &&
          read32(CONTEXT + 0x400u) == UINT32_C(0x00000400),
          "move.context_position");
    check(marker_count == 1u && marker_id == 24u &&
          marker_vector == SCRATCH_VEC && marker_flags == 0u,
          "move.marker");
    check(destroy_count == 0u, "move.no_destroy");
}

static void test_completion_branch(void)
{
    seed();
    write16(SLOT + 4u, 1u);
    write32(SLOT + 0x28u, UINT32_C(0xFFF00000));
    write32(SLOT + 0x2Cu, UINT32_C(0x00006000));
    write32(SLOT + 0x30u, UINT32_C(0x00400000));
    write16(CONTEXT + 0x3F0u, 0u);
    write16(CONTEXT + 0x4ECu, 0u);

    check(wm_8007D4A4(SLOT_INDEX) == 3, "complete.return");
    check(read16(SLOT + 4u) == 0u, "complete.latch_clear");
    check(read16(CONTEXT + 0x3F0u) == 1u &&
          read16(CONTEXT + 0x4ECu) == 1u,
          "complete.flags");
    check(destroy_count == 1u && destroy_record == 24u,
          "complete.destroy");
    check(wrap_count == 0u && height_count == 0u,
          "complete.skips_movement");
    check(read16(SCRATCH_VEC + 0u) == UINT16_C(0xFF00) &&
          read16(SCRATCH_VEC + 2u) == 6u &&
          read16(SCRATCH_VEC + 4u) == UINT16_C(0x0400),
          "complete.scratch_vector");
    check(marker_count == 1u && marker_id == 29u &&
          marker_vector == SCRATCH_VEC && marker_flags == 0u,
          "complete.marker");
}

static void test_scheduler_resolution(void)
{
    seed();
    write32(SLOT + 0x18u, UINT32_C(0x8007D414));
    write32(SLOT + 0x1Cu, UINT32_C(0x8007D4A4));
    write16(SLOT + 0u, 0u);
    wm_sched_reset();
    wm_sched_callback_registry_clear();
    wm_80097800();
    check(wm_sched_get_callbacks_executed() == 1 &&
          wm_sched_get_missing_hits() == 0 &&
          wm_sched_get_invalid_hits() == 0,
          "scheduler.init_resolution");

    reset_trace();
    write16(SLOT + 0u, 1u);
    write16(SLOT + 4u, 1u);
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
    test_moving_branch();
    test_completion_branch();
    test_scheduler_resolution();
    if (failures != 0) {
        fprintf(stderr,
                "W34N82 MODE12 LINKED MARKER C CERTIFICATE: %d failure(s)\n",
                failures);
        return 1;
    }
    puts("W34N82 MODE12 LINKED MARKER C CERTIFICATE PASS");
    return 0;
}
