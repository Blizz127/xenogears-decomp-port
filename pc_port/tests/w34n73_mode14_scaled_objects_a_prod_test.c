/* Focused production certificate for retail 0x8007B200/0x8007B394. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_callback_7b200.h"
#include "world_map_scheduler.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

#define POOL        UINT32_C(0x800A0000)
#define POOL_PTR    UINT32_C(0x8009BE24)
#define SLOT_INDEX  3
#define SLOT        (POOL + (u32)SLOT_INDEX * UINT32_C(0x80))
#define CONTEXT     UINT32_C(0x800A8000)
#define CONTEXT_PTR UINT32_C(0x8009C620)
#define RESET_POS   UINT32_C(0x8009C5AC)
#define BASE_MATRIX UINT32_C(0x8009A180)
#define DESCRIPTOR_A UINT32_C(0x800B0000)
#define DESCRIPTOR_B UINT32_C(0x800B0100)
#define SOURCE_A    UINT32_C(0x800B1000)
#define SOURCE_B    UINT32_C(0x800B2000)
#define SC_VECTOR_A UINT32_C(0x1F800000)
#define SC_VECTOR_B UINT32_C(0x1F800010)
#define SC_MATRIX_A UINT32_C(0x1F8000F0)
#define SC_MATRIX_B UINT32_C(0x1F800110)

typedef struct PrimitiveCall {
    u32 owner;
    u32 source;
    u16 count;
} PrimitiveCall;

static int failures;
static PrimitiveCall primitive_calls[4];
static unsigned primitive_count;
static unsigned scale_count;
static MATRIX *scale_matrix[4];
static VECTOR *scale_vector[4];
static u32 scale_input[4][8];
static s32 scale_xyz[4][3];

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

static u32 matrix_pattern(unsigned call_index, unsigned word_index)
{
    return UINT32_C(0xA0000000) + (u32)call_index * UINT32_C(0x01000000) +
           (u32)word_index;
}

static int region_is_pattern(u32 address, unsigned call_index)
{
    unsigned i;

    for (i = 0u; i < 8u; i++)
        if (read32(address + (u32)i * 4u) != matrix_pattern(call_index, i))
            return 0;
    return 1;
}

static void seed(void)
{
    unsigned i;

    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0xA5, sizeof(g_PsxScratchpad));
    write32(POOL_PTR, POOL);
    write32(CONTEXT_PTR, CONTEXT);
    for (i = 0u; i < 8u; i++)
        write32(BASE_MATRIX + (u32)i * 4u,
                UINT32_C(0x11000000) + (u32)i);
    write32(CONTEXT + 0x190u, DESCRIPTOR_A);
    write32(CONTEXT + 0x198u, SOURCE_A);
    write32(CONTEXT + 0x1E4u, DESCRIPTOR_B);
    write32(CONTEXT + 0x1ECu, SOURCE_B);
    write16(DESCRIPTOR_A + 4u, 3u);
    write16(DESCRIPTOR_B + 4u, 5u);
    write16(SOURCE_A + 4u, 33u);
    write16(SOURCE_B + 4u, 55u);
    primitive_count = 0u;
    scale_count = 0u;
}

void wm_8007A06C(u32 owner, u32 primitive_source, u16 count)
{
    check(primitive_count < 4u, "trace.primitive_capacity");
    if (primitive_count < 4u) {
        primitive_calls[primitive_count].owner = owner;
        primitive_calls[primitive_count].source = primitive_source;
        primitive_calls[primitive_count].count = count;
        primitive_count++;
    }
}

MATRIX *ScaleMatrix(MATRIX *matrix, VECTOR *vector)
{
    unsigned i;
    unsigned index = scale_count;

    check(index < 4u, "trace.scale_capacity");
    if (index >= 4u)
        return matrix;
    scale_matrix[index] = matrix;
    scale_vector[index] = vector;
    memcpy(scale_input[index], matrix, sizeof(scale_input[index]));
    scale_xyz[index][0] = vector->vx;
    scale_xyz[index][1] = vector->vy;
    scale_xyz[index][2] = vector->vz;
    scale_count++;
    for (i = 0u; i < 8u; i++) {
        u32 value = matrix_pattern(index + 1u, i);
        memcpy((uint8_t *)(void *)matrix + (size_t)i * 4u,
               &value, sizeof(value));
    }
    return matrix;
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

static void check_base_input(unsigned call_index, const char *name)
{
    unsigned i;

    for (i = 0u; i < 8u; i++)
        if (scale_input[call_index][i] != UINT32_C(0x11000000) + (u32)i) {
            check(0, name);
            return;
        }
    check(1, name);
}

static void test_initializer(void)
{
    seed();
    write32(SLOT + 0x50u, UINT32_C(0xAAAAAAAA));
    write32(SLOT + 0x54u, UINT32_C(0xBBBBBBBB));

    check(wm_8007B200(SLOT_INDEX) == 3, "init.return");
    check(primitive_count == 2u &&
          primitive_calls[0].owner == CONTEXT + 0x150u &&
          primitive_calls[0].source == SOURCE_A &&
          primitive_calls[0].count == 3u &&
          primitive_calls[1].owner == CONTEXT + 0x1A4u &&
          primitive_calls[1].source == SOURCE_B &&
          primitive_calls[1].count == 5u,
          "init.primitive_calls");
    check(primitive_count == 2u && primitive_calls[0].count == 3u &&
          primitive_calls[1].count == 5u,
          "init.primitive_arguments");
    check(read32(SLOT + 0x50u) == 0u && read32(SLOT + 0x54u) == 0u,
          "init.slot_seeds");
    check(scale_count == 1u &&
          scale_matrix[0] == (MATRIX *)PSX_ADDR(SC_MATRIX_A) &&
          scale_vector[0] == (VECTOR *)PSX_ADDR(SC_VECTOR_A) &&
          scale_xyz[0][0] == 0 && scale_xyz[0][1] == 4096 &&
          scale_xyz[0][2] == 0,
          "init.scale_arguments");
    check_base_input(0u, "init.base_matrix");
    check(region_is_pattern(CONTEXT + 0x218u, 1u) &&
          region_is_pattern(CONTEXT + 0x26Cu, 1u),
          "init.matrix_destinations");
    check(read16(CONTEXT + 0x1F8u) == 1u &&
          read16(CONTEXT + 0x24Cu) == 1u,
          "init.owner_states");
}

static void test_latch_stagger_and_matrices(void)
{
    seed();
    write16(SLOT + 4u, 1u);
    write16(CONTEXT + 0x150u, UINT16_C(0x7777));
    write16(CONTEXT + 0x1A4u, UINT16_C(0x8888));
    write32(RESET_POS + 0u, UINT32_C(0xFFF00000));
    write32(RESET_POS + 8u, UINT32_C(0x00200000));
    write32(SLOT + 0x50u, 1664u);
    write32(SLOT + 0x54u, 77u);

    check(wm_8007B394(SLOT_INDEX) == 1, "update.return");
    check(read16(SLOT + 4u) == 0u &&
          read16(CONTEXT + 0x150u) == 0u &&
          read16(CONTEXT + 0x1A4u) == 0u,
          "update.latch_clear");
    check(read32(CONTEXT + 0x158u) == UINT32_C(0xFFFFFF00) &&
          read32(CONTEXT + 0x1ACu) == UINT32_C(0xFFFFFF00) &&
          read32(CONTEXT + 0x15Cu) == UINT32_C(0xFFFFFFC0) &&
          read32(CONTEXT + 0x1B0u) == UINT32_C(0xFFFFFFC0) &&
          read32(CONTEXT + 0x160u) == 512u &&
          read32(CONTEXT + 0x1B4u) == 512u,
          "update.positions");
    check(read32(SLOT + 0x50u) == 2048u &&
          read32(SLOT + 0x54u) == 77u,
          "update.stagger_threshold");
    check(scale_count == 2u &&
          scale_xyz[0][0] == 2048 && scale_xyz[0][1] == 4096 &&
          scale_xyz[0][2] == 2048 &&
          scale_xyz[1][0] == 77 && scale_xyz[1][1] == 4096 &&
          scale_xyz[1][2] == 77,
          "update.scale_vectors");
    check_base_input(0u, "update.primary_base_matrix");
    check_base_input(1u, "update.secondary_base_matrix");
    check(region_is_pattern(CONTEXT + 0x170u, 1u) &&
          region_is_pattern(CONTEXT + 0x1C4u, 2u),
          "update.matrix_destinations");
}

static void test_phase_clamps(void)
{
    seed();
    write32(SLOT + 0x50u, 32400u);
    write32(SLOT + 0x54u, 32400u);
    check(wm_8007B394(SLOT_INDEX) == 1 &&
          read32(SLOT + 0x50u) == 32512u &&
          read32(SLOT + 0x54u) == 32512u,
          "update.phase_clamps");
}

static void test_scheduler_resolution(void)
{
    seed();
    write32(SLOT + 0x18u, UINT32_C(0x8007B200));
    write32(SLOT + 0x1Cu, UINT32_C(0x8007B394));
    write16(SLOT + 0u, 0u);
    wm_sched_reset();
    wm_sched_callback_registry_clear();
    wm_80097800();
    check(wm_sched_get_callbacks_executed() == 1 &&
          wm_sched_get_missing_hits() == 0 &&
          wm_sched_get_invalid_hits() == 0,
          "scheduler.init_resolution");

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
    test_latch_stagger_and_matrices();
    test_phase_clamps();
    test_scheduler_resolution();
    if (failures != 0) {
        fprintf(stderr,
                "W34N73 MODE14 SCALED OBJECTS A CERTIFICATE: %d failure(s)\n",
                failures);
        return 1;
    }
    puts("W34N73 MODE14 SCALED OBJECTS A CERTIFICATE PASS");
    return 0;
}
