/* Focused production certificate for retail 0x8007D600/0x8007D690. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_scheduler.h"

/* matched C (src/world_map/main.c), linked from the matched object */
s32 func_8007D690(s32 arg0);

/* matched C (src/world_map/main.c), linked from the matched object */
s32 func_8007D600(s32 arg0);

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

#define POOL          UINT32_C(0x800A0000)
#define POOL_PTR      UINT32_C(0x8009BE24)
#define SLOT_INDEX    5
#define SLOT          (POOL + (u32)SLOT_INDEX * UINT32_C(0x80))
#define CONTEXT       UINT32_C(0x800A8000)
#define CONTEXT_PTR   UINT32_C(0x8009C620)
#define SCRATCH_VEC   UINT32_C(0x1F8000A0)

static int failures;
static unsigned link_count;
static s32 link_source;
static s32 link_destination;
static unsigned rotation_count;
static SVECTOR *rotation_angles;
static MATRIX *rotation_matrix;
static unsigned wrap_count;
static u32 wrap_address;
static u32 wrap_x_before;
static u32 wrap_z_before;
static unsigned height_count;
static u32 height_x;
static u32 height_z;
static unsigned marker_count;
static u32 marker_id;
static u32 marker_vector;
static u32 marker_flags;

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
    wrap_x_before = 0u;
    wrap_z_before = 0u;
    height_count = 0u;
    height_x = 0u;
    height_z = 0u;
    marker_count = 0u;
    marker_id = 0u;
    marker_vector = 0u;
    marker_flags = 0u;
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
    wrap_x_before = read32(vec_addr + 0u);
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

s32 func_80093A5C(s32 x, s32 z)
{
    return wm_80093A5C((u32)x, (u32)z);
}

void func_80089160(s32 id, void *vector, void *flags)
{
    wm_80089160((u32)id, PsxMemory_GuestAddr(vector),
                flags != NULL ? PsxMemory_GuestAddr(flags) : 0u);
}

void wm_800894C8(u32 record_index)
{
    (void)record_index;
}

static void test_initializer(void)
{
    seed();
    write16(CONTEXT + 0x444u, UINT16_C(0x1111));
    write16(CONTEXT + 0x45Cu, UINT16_C(0x2222));
    write16(CONTEXT + 0x45Eu, UINT16_C(0x3333));
    write16(CONTEXT + 0x460u, UINT16_C(0x4444));

    check(func_8007D600(SLOT_INDEX) == 1, "init.return");
    /* func_800848B4 stores record src's guest address at record dst +0x50 */
    check(read32(CONTEXT + 14u * 0x54u + 0x50u) == CONTEXT + 13u * 0x54u,
          "init.link");
    check(read16(CONTEXT + 0x444u) == 0u &&
          read16(CONTEXT + 0x45Cu) == 0u &&
          read16(CONTEXT + 0x45Eu) == 0u &&
          read16(CONTEXT + 0x460u) == 0u,
          "init.context_zeroes");
    check(rotation_count == 1u &&
          rotation_angles == (SVECTOR *)PSX_ADDR(CONTEXT + 0x45Cu) &&
          rotation_matrix == (MATRIX *)PSX_ADDR(CONTEXT + 0x464u),
          "init.rotation");
    check(read32(SLOT + 0x40u) == UINT32_C(0xFFFFC000) &&
          read32(SLOT + 0x28u) == UINT32_C(0x00F80000) &&
          read32(SLOT + 0x2Cu) == 0u &&
          read32(SLOT + 0x30u) == UINT32_C(0x00380000) &&
          read32(SLOT + 0x38u) == 0u &&
          read32(SLOT + 0x3Cu) == 0u,
          "init.slot_seed");
}

static void test_update(void)
{
    seed();
    write32(SLOT + 0x28u, UINT32_C(0x00F80000));
    write32(SLOT + 0x2Cu, UINT32_C(0xDEADBEEF));
    write32(SLOT + 0x30u, UINT32_C(0x00380000));
    write32(SLOT + 0x40u, UINT32_C(0xFFFFC000));
    /* The wrap is the matched func_80093354 now (not a mock): a 0x800000
     * world in both axes wraps x 0xF80000 to 0x780000 and leaves z. */
    write32(UINT32_C(0x8009D160), 1u);
    write32(UINT32_C(0x8009D2B4), 1u);

    check(func_8007D690(SLOT_INDEX) == 1, "update.return");
    check(read32(SLOT + 0x28u) == UINT32_C(0x00780000) &&
          read32(SLOT + 0x30u) == UINT32_C(0x0037C000),
          "update.wrap_target");
    check(height_count == 1u &&
          height_x == UINT32_C(0x00780000) &&
          height_z == UINT32_C(0x0037C000),
          "update.reload_after_wrap");
    check(read32(SLOT + 0x2Cu) == UINT32_C(0x00006000),
          "update.height_bias");
    check(read32(CONTEXT + 0x44Cu) == UINT32_C(0x00000780) &&
          read32(CONTEXT + 0x450u) == 6u &&
          read32(CONTEXT + 0x454u) == UINT32_C(0x0000037C),
          "update.context_position");
    check(read16(SCRATCH_VEC + 0u) == UINT16_C(0x0780) &&
          read16(SCRATCH_VEC + 2u) == 6u &&
          read16(SCRATCH_VEC + 4u) == UINT16_C(0x037C),
          "update.scratch_vector");
    check(marker_count == 1u && marker_id == 25u &&
          PSX_ADDR(marker_vector) == PSX_ADDR(SCRATCH_VEC) && marker_flags == 0u,
          "update.marker");
}

static void test_scheduler_resolution(void)
{
    seed();
    write32(SLOT + 0x18u, UINT32_C(0x8007D600));
    write32(SLOT + 0x1Cu, UINT32_C(0x8007D690));
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
    write32(SLOT + 0x28u, UINT32_C(0x00F80000));
    write32(SLOT + 0x30u, UINT32_C(0x00380000));
    write32(SLOT + 0x40u, UINT32_C(0xFFFFC000));
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
    test_update();
    test_scheduler_resolution();
    if (failures != 0) {
        fprintf(stderr,
                "W34N78 MODE12 MOVING MARKER CERTIFICATE: %d failure(s)\n",
                failures);
        return 1;
    }
    puts("W34N78 MODE12 MOVING MARKER CERTIFICATE PASS");
    return 0;
}
