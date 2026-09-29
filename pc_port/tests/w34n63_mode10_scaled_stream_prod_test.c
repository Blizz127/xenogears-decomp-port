/* Focused production certificate for retail 0x8007A06C..0x8007A410. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_callback_7a144.h"
#include "world_map_scheduler.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

#define POOL        0x800A0000u
#define POOL_PTR    0x8009BE24u
#define SLOT_INDEX  2
#define SLOT        (POOL + (u32)SLOT_INDEX * 0x80u)
#define CONTEXT_PTR 0x8009C620u
#define CONTEXT     0x800A8000u
#define BASE_MATRIX 0x8009A180u
#define MATRIX_A    0x1F8000F0u
#define MATRIX_B    0x1F800110u
#define SCALE_A     0x1F800000u
#define SCALE_B     0x1F800010u
#define DESC_A      0x800B0000u
#define DESC_B      0x800B0100u
#define SOURCE_A    0x800B1000u
#define DEST_A      0x800B2000u
#define SOURCE_B    0x800B3000u
#define DEST_B      0x800B4000u

static int failures;
static int tpage_calls;
static int clut_calls;
static int tpage_args[4];
static int clut_args[2];
static int scale_calls;
static u32 scale_matrix[4];
static u32 scale_vector[4];
static s32 scale_values[4][3];

static void check(int condition, const char* name)
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

static u32 guest_of_scratch(const void* pointer)
{
    return 0x1F800000u +
        (u32)((const uint8_t*)pointer - g_PsxRam);
}

static void seed(void)
{
    u32 index;
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0xA5, sizeof(g_PsxScratchpad));
    write32(POOL_PTR, POOL);
    write32(CONTEXT_PTR, CONTEXT);
    for (index = 0u; index < 8u; index++)
        write32(BASE_MATRIX + index * 4u, 0xB0000000u + index);
    tpage_calls = 0;
    clut_calls = 0;
    memset(tpage_args, 0, sizeof(tpage_args));
    memset(clut_args, 0, sizeof(clut_args));
    scale_calls = 0;
    memset(scale_matrix, 0, sizeof(scale_matrix));
    memset(scale_vector, 0, sizeof(scale_vector));
    memset(scale_values, 0, sizeof(scale_values));
}

u16 GetTPage(int tp, int abr, int x, int y)
{
    tpage_calls++;
    tpage_args[0] = tp;
    tpage_args[1] = abr;
    tpage_args[2] = x;
    tpage_args[3] = y;
    return 0x1357u;
}

u16 GetClut(int x, int y)
{
    clut_calls++;
    clut_args[0] = x;
    clut_args[1] = y;
    return 0x2468u;
}

MATRIX* ScaleMatrix(MATRIX* matrix, VECTOR* scale)
{
    int call = scale_calls;
    u32 index;
    if (call < 4) {
        scale_matrix[call] = guest_of_scratch(matrix);
        scale_vector[call] = guest_of_scratch(scale);
        scale_values[call][0] = scale->vx;
        scale_values[call][1] = scale->vy;
        scale_values[call][2] = scale->vz;
    }
    for (index = 0u; index < 8u; index++) {
        u32 word = 0xC1000000u + (u32)call * 0x01000000u + index;
        memcpy((uint8_t*)matrix + index * 4u, &word, sizeof(word));
    }
    scale_calls++;
    return matrix;
}

static void seed_streams(u16 count_a, u16 count_b)
{
    write32(CONTEXT + 0x4D8u, DESC_A);
    write32(CONTEXT + 0x4E0u, SOURCE_A);
    write32(CONTEXT + 0x4E4u, DEST_A);
    write16(DESC_A + 4u, count_a);
    write32(CONTEXT + 0x52Cu, DESC_B);
    write32(CONTEXT + 0x534u, SOURCE_B);
    write32(CONTEXT + 0x538u, DEST_B);
    write16(DESC_B + 4u, count_b);
    memset(PSX_ADDR(SOURCE_A), 0x11, 120u);
    memset(PSX_ADDR(DEST_A), 0xCC, 120u);
    memset(PSX_ADDR(SOURCE_B), 0x22, 120u);
    memset(PSX_ADDR(DEST_B), 0xDD, 120u);
}

static void check_formatted_record(u32 record, const char* name)
{
    check(read8(record + 3u) == 9u &&
          read8(record + 4u) == 128u &&
          read8(record + 5u) == 128u &&
          read8(record + 6u) == 128u &&
          read8(record + 7u) == 46u &&
          read16(record + 14u) == 0x2468u &&
          read16(record + 22u) == 0x1357u,
          name);
}

static void test_stream_helper(void)
{
    seed();
    seed_streams(2u, 0u);
    wm_8007A06C(CONTEXT + 0x498u, SOURCE_A, 2u);
    check(tpage_calls == 2 && clut_calls == 2 &&
          tpage_args[0] == 1 && tpage_args[1] == 3 &&
          tpage_args[2] == 832 && tpage_args[3] == 256 &&
          clut_args[0] == 256 && clut_args[1] == 511,
          "stream.psyq_arguments");
    check_formatted_record(SOURCE_A, "stream.primitive_code");
    check_formatted_record(SOURCE_A + 40u, "stream.second_record");
    check_formatted_record(DEST_A, "stream.copy_direction");
    check(memcmp(PSX_ADDR(SOURCE_A), PSX_ADDR(DEST_A), 80u) == 0,
          "stream.copy_bytes");
}

static void test_pair_init(void)
{
    seed();
    seed_streams(2u, 1u);
    write16(SLOT + 0x20u, 7u);
    write32(SLOT + 0x6Cu, 0x11111111u);
    write32(SLOT + 0x70u, 0x22222222u);
    check(wm_8007A144(SLOT_INDEX) == 3, "init.return");
    check(read16(SLOT + 0x20u) == 0u &&
          read32(SLOT + 0x6Cu) == 0u && read32(SLOT + 0x70u) == 0u,
          "init.slot_state");
    check(tpage_calls == 3 && clut_calls == 3 &&
          memcmp(PSX_ADDR(SOURCE_A), PSX_ADDR(DEST_A), 80u) == 0,
          "init.first_stream");
    check(memcmp(PSX_ADDR(SOURCE_B), PSX_ADDR(DEST_B), 40u) == 0 &&
          read8(DEST_B + 7u) == 46u,
          "init.second_stream");
}

static void seed_update(void)
{
    u32 index;
    for (index = 0u; index < 4u; index++)
        write32(CONTEXT + 8u + index * 4u, 0x10101010u + index);
    memset(PSX_ADDR(CONTEXT + 0x4A0u), 0xCC, 16u);
    memset(PSX_ADDR(CONTEXT + 0x4F4u), 0xDD, 16u);
    memset(PSX_ADDR(CONTEXT + 0x4B8u), 0xEE, 32u);
    memset(PSX_ADDR(CONTEXT + 0x50Cu), 0xFF, 32u);
}

static void check_update_matrices(u32 primary, u32 secondary)
{
    u32 index;
    check(scale_calls == 2 && scale_matrix[0] == MATRIX_A &&
          scale_matrix[1] == MATRIX_B && scale_vector[0] == SCALE_A &&
          scale_vector[1] == SCALE_B &&
          (u32)scale_values[0][0] == primary &&
          scale_values[0][1] == 4096 &&
          (u32)scale_values[0][2] == primary &&
          (u32)scale_values[1][0] == secondary &&
          scale_values[1][1] == 4096 &&
          (u32)scale_values[1][2] == secondary,
          "update.scale_order");
    for (index = 0u; index < 8u; index++) {
        check(read32(CONTEXT + 0x4B8u + index * 4u) ==
              0xC1000000u + index, "update.matrix_copy");
        check(read32(CONTEXT + 0x50Cu + index * 4u) ==
              0xC2000000u + index, "update.second_matrix_copy");
    }
}

static void test_pair_update(void)
{
    u32 index;
    seed();
    seed_update();
    write32(SLOT + 0x6Cu, 0u);
    write32(SLOT + 0x70u, 0u);
    check(wm_8007A1B4(SLOT_INDEX) == 1, "update.return_active");
    check(read32(SLOT + 0x6Cu) == 192u &&
          read32(SLOT + 0x70u) == 0u,
          "update.primary_phase");
    for (index = 0u; index < 4u; index++)
        check(read32(CONTEXT + 0x4A0u + index * 4u) ==
              0x10101010u + index &&
              read32(CONTEXT + 0x4F4u + index * 4u) ==
              0x10101010u + index,
              "update.position_copies");
    check_update_matrices(192u, 0u);

    seed();
    seed_update();
    write32(SLOT + 0x6Cu, 2000u);
    write32(SLOT + 0x70u, 0x7F80u);
    (void)wm_8007A1B4(SLOT_INDEX);
    check(read32(SLOT + 0x6Cu) == 2192u &&
          read32(SLOT + 0x70u) == 0x0080u,
          "update.secondary_wrap");

    seed();
    seed_update();
    write16(SLOT + 4u, 7u);
    write32(SLOT + 0x6Cu, 2048u);
    write32(SLOT + 0x70u, 0x5F00u);
    check(wm_8007A1B4(SLOT_INDEX) == 3 &&
          read16(SLOT + 4u) == 0u &&
          read32(SLOT + 0x6Cu) == 0u &&
          read32(SLOT + 0x70u) == 0u,
          "update.terminal");
    check_update_matrices(0u, 0u);
}

static void test_scheduler_resolution(void)
{
    seed();
    seed_streams(0u, 0u);
    memset(PSX_ADDR(POOL), 0, 64u * 0x80u);
    write32(SLOT + 0x18u, 0x8007A144u);
    write32(SLOT + 0x1Cu, 0x8007A1B4u);
    wm_sched_reset();
    wm_sched_callback_registry_clear();
    wm_80097800();
    check(wm_sched_get_callbacks_executed() == 1 &&
          wm_sched_get_missing_hits() == 0 &&
          wm_sched_get_invalid_hits() == 0 &&
          read16(SLOT) == 3u,
          "scheduler.init_resolution");

    write16(SLOT, 1u);
    wm_sched_reset();
    wm_80097800();
    check(wm_sched_get_callbacks_executed() == 1 &&
          wm_sched_get_missing_hits() == 0 &&
          wm_sched_get_invalid_hits() == 0 &&
          read16(SLOT) == 1u,
          "scheduler.update_resolution");
}

int main(void)
{
    test_stream_helper();
    test_pair_init();
    test_pair_update();
    test_scheduler_resolution();
    if (failures != 0) {
        fprintf(stderr, "W34N63 MODE10 SCALED-STREAM CERTIFICATE: %d failure(s)\n",
                failures);
        return 1;
    }
    puts("W34N63 MODE10 SCALED-STREAM CERTIFICATE PASS");
    return 0;
}
