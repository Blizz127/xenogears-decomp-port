/* Focused production certificate for retail 0x8007CE84/0x8007CF18. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_callback_7ce84.h"
#include "world_map_scheduler.h"

/* matched C (src/world_map/main.c), linked from the matched object */
s32 func_8007CE84(s32 arg0);

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

#define POOL        UINT32_C(0x800A0000)
#define POOL_PTR    UINT32_C(0x8009BE24)
#define SLOT_INDEX  5
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
static u32 marker_ids[4];
static u32 marker_vectors[4];
static u32 marker_flags[4];

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
    memset(marker_ids, 0, sizeof(marker_ids));
    memset(marker_vectors, 0, sizeof(marker_vectors));
    memset(marker_flags, 0, sizeof(marker_flags));
}

static void seed(void)
{
    memset(g_PsxRam, 0xA5, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0x5A, sizeof(g_PsxScratchpad));
    write32(POOL_PTR, POOL);
    write32(CONTEXT_PTR, CONTEXT);
    reset_trace();
}

static void seed_motion(void)
{
    write32(SLOT + 0x28u, UINT32_C(0x00B00000));
    write32(SLOT + 0x2Cu, 0u);
    write32(SLOT + 0x30u, UINT32_C(0x00200000));
    write32(SLOT + 0x40u, UINT32_C(0xFFFFC000));
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
    check(marker_count < 4u, "trace.marker_capacity");
    if (marker_count < 4u) {
        marker_ids[marker_count] = id;
        marker_vectors[marker_count] = vector;
        marker_flags[marker_count] = flags;
        marker_count++;
    }
}

void wm_800894C8(u32 record_index)
{
    (void)record_index;
}

static void test_initializer(void)
{
    seed();
    write16(CONTEXT + 0x1A4u, UINT16_C(0x7777));
    write16(CONTEXT + 0x1BCu, UINT16_C(0x1111));
    write16(CONTEXT + 0x1BEu, UINT16_C(0x2222));
    write16(CONTEXT + 0x1C0u, UINT16_C(0x3333));

    check(func_8007CE84(SLOT_INDEX) == 1, "init.return");
    /* func_800848B4 stores record src's guest address at record dst +0x50 */
    check(read32(CONTEXT + 6u * 0x54u + 0x50u) == CONTEXT + 5u * 0x54u,
          "init.link");
    check(read16(CONTEXT + 0x1A4u) == 0u &&
          read16(CONTEXT + 0x1BCu) == 0u &&
          read16(CONTEXT + 0x1BEu) == 0u &&
          read16(CONTEXT + 0x1C0u) == 0u,
          "init.context_zeroes");
    check(rotation_count == 1u &&
          rotation_angles == (SVECTOR *)PSX_ADDR(CONTEXT + 0x1BCu) &&
          rotation_matrix == (MATRIX *)PSX_ADDR(CONTEXT + 0x1C4u),
          "init.rotation");
    check(read32(SLOT + 0x40u) == UINT32_C(0xFFFFC000) &&
          read32(SLOT + 0x28u) == UINT32_C(0x00B00000) &&
          read32(SLOT + 0x2Cu) == 0u &&
          read32(SLOT + 0x30u) == UINT32_C(0x00200000) &&
          read32(SLOT + 0x38u) == 0u &&
          read32(SLOT + 0x3Cu) == 0u &&
          read16(SLOT + 0x20u) == 0u,
          "init.slot_seed");
}

static void test_unarmed_update(void)
{
    seed();
    seed_motion();
    write16(SLOT + 0x20u, 0u);

    check(wm_8007CF18(SLOT_INDEX) == 1, "move.return");
    check(wrap_count == 1u && wrap_address == SLOT + 0x28u &&
          wrap_z_before == UINT32_C(0x001FC000),
          "move.wrap");
    check(height_count == 1u &&
          height_x == UINT32_C(0xFFF00000) &&
          height_z == UINT32_C(0x00400000),
          "move.height_input");
    check(read32(SLOT + 0x2Cu) == UINT32_C(0x00006000),
          "move.height_bias");
    check(read32(CONTEXT + 0x1ACu) == UINT32_C(0xFFFFFF00) &&
          read32(CONTEXT + 0x1B0u) == 6u &&
          read32(CONTEXT + 0x1B4u) == UINT32_C(0x00000400),
          "move.context_position");
    check(marker_count == 1u && marker_ids[0] == 21u &&
          marker_vectors[0] == SCRATCH_VEC && marker_flags[0] == 0u,
          "move.primary_marker");
}

static void test_latch_and_persistent_dual_marker(void)
{
    seed();
    seed_motion();
    write16(SLOT + 4u, 1u);
    write16(SLOT + 0x20u, 0u);

    check(wm_8007CF18(SLOT_INDEX) == 1, "latch.return");
    check(read16(SLOT + 4u) == 0u && read16(SLOT + 0x20u) == 1u,
          "latch.arm");
    check(marker_count == 2u && marker_ids[0] == 21u &&
          marker_ids[1] == 30u && marker_vectors[0] == SCRATCH_VEC &&
          marker_vectors[1] == SCRATCH_VEC && marker_flags[0] == 0u &&
          marker_flags[1] == 0u,
          "latch.dual_markers");

    reset_trace();
    write32(SLOT + 0x30u, UINT32_C(0x00200000));
    check(wm_8007CF18(SLOT_INDEX) == 1, "persistent.return");
    check(read16(SLOT + 0x20u) == 1u && marker_count == 2u &&
          marker_ids[0] == 21u && marker_ids[1] == 30u,
          "persistent.dual_markers");
}

static void test_scheduler_resolution(void)
{
    seed();
    write32(SLOT + 0x18u, UINT32_C(0x8007CE84));
    write32(SLOT + 0x1Cu, UINT32_C(0x8007CF18));
    write16(SLOT + 0u, 0u);
    wm_sched_reset();
    wm_sched_callback_registry_clear();
    wm_80097800();
    check(wm_sched_get_callbacks_executed() == 1 &&
          wm_sched_get_missing_hits() == 0 &&
          wm_sched_get_invalid_hits() == 0,
          "scheduler.init_resolution");

    reset_trace();
    seed_motion();
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
    test_unarmed_update();
    test_latch_and_persistent_dual_marker();
    test_scheduler_resolution();
    if (failures != 0) {
        fprintf(stderr,
                "W34N83 MODE12 DUAL MARKER CERTIFICATE: %d failure(s)\n",
                failures);
        return 1;
    }
    puts("W34N83 MODE12 DUAL MARKER CERTIFICATE PASS");
    return 0;
}
