/* Focused production certificate for retail callbacks 0x8007756C/0x800776E0. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_7756c.h"
#include "world_map_scheduler.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

#define POOL       0x800A0000u
#define POOL_PTR   0x8009BE24u
#define SLOT_INDEX 3
#define SLOT       (POOL + (u32)SLOT_INDEX * 0x80u)
#define ANGLES     0x8009BD38u
#define MATRIX     0x8009BD40u
#define MATRIX_B   0x8009BD48u
#define POSITION   0x8009BE28u
#define CAMERA     0x8009C5ACu
#define HEIGHT     0x8009D3F0u
#define STATE      0x8009D144u
#define GEOM_Y     0x8009BE0Cu
#define HELD       0x8009CD4Cu
#define RELEASED   0x8009BD10u
#define D554       0x8009D554u
#define D7CC       0x8009D7CCu
#define GEOM_H     0x8009BCDCu

static uint8_t ram_before[PSX_RAM_SIZE];
static uint8_t scratch_before[4096];
static int failures;
static int matrix_calls;
static u32 matrix_args[4];
static s16 matrix_angles[3];
static int geom_calls;
static int geom_h;

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

void wm_80096F18(u32 out_matrix, u32 pos_vec, s32 height, u32 rot_svec)
{
    matrix_calls++;
    matrix_args[0] = out_matrix;
    matrix_args[1] = pos_vec;
    matrix_args[2] = (u32)height;
    matrix_args[3] = rot_svec;
    memcpy(matrix_angles, PSX_ADDR(rot_svec), sizeof(matrix_angles));
    write32(out_matrix + 0u, 0xA1A2A3A4u);
    write32(out_matrix + 4u, 0xB1B2B3B4u);
}

void SetGeomScreen(int h)
{
    geom_calls++;
    geom_h = h;
}

static int allowed_ram_byte(size_t offset, int update)
{
    u32 address = 0x80000000u + (u32)offset;
    if (offset >= 0xA0u && offset < 0xA8u)
        return 1;
    if (address >= SLOT + 0x28u && address < SLOT + 0x48u)
        return 1;
    if (address >= ANGLES && address < ANGLES + 6u)
        return 1;
    if (address >= MATRIX && address < MATRIX + 16u)
        return 1;
    if (address >= POSITION && address < POSITION + 12u)
        return 1;
    if (address >= HEIGHT && address < HEIGHT + 4u)
        return 1;
    if (address >= STATE && address < STATE + 4u)
        return 1;
    if (address >= GEOM_Y && address < GEOM_Y + 4u)
        return 1;
    if (update && address >= D554 && address < D554 + 4u)
        return 1;
    if (update && address >= D7CC && address < D7CC + 4u)
        return 1;
    return 0;
}

static void snapshot(void)
{
    memcpy(ram_before, g_PsxRam, sizeof(ram_before));
    memcpy(scratch_before, g_PsxScratchpad, sizeof(scratch_before));
}

static void check_write_set(int update, const char* name)
{
    size_t i;
    for (i = 0u; i < sizeof(g_PsxRam); i++) {
        if (ram_before[i] != g_PsxRam[i] && !allowed_ram_byte(i, update)) {
            fprintf(stderr, "ASSERTION %s FAILED ram+0x%zx\n", name, i);
            failures++;
            break;
        }
    }
    for (i = 0u; i < sizeof(g_PsxScratchpad); i++) {
        if (scratch_before[i] != g_PsxScratchpad[i] &&
                !(i >= 0xA0u && i < 0xA8u)) {
            fprintf(stderr, "ASSERTION %s FAILED scratch+0x%zx\n", name, i);
            failures++;
            break;
        }
    }
}

static void seed(void)
{
    memset(g_PsxRam, 0x5Au, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0xC3, sizeof(g_PsxScratchpad));
    matrix_calls = 0;
    geom_calls = 0;
    geom_h = 0;
    write32(POOL_PTR, POOL);
    memset(PSX_ADDR(POOL), 0, 64u * 0x80u);
    write32(CAMERA + 0u, 0x11111111u);
    write32(CAMERA + 4u, 0x22222222u);
    write32(CAMERA + 8u, 0x33333333u);
    write32(SLOT + 0x30u, 0x44444444u);
    write32(SLOT + 0x34u, 0x55555555u);
    write32(MATRIX_B + 0u, 0x61626364u);
    write32(MATRIX_B + 4u, 0x71727374u);
}

static void test_init(void)
{
    s32 result;

    seed();
    snapshot();
    result = wm_8007756C(SLOT_INDEX);
    check(result == 1, "init.return_one");
    check(read16(ANGLES + 0u) == 0xFF80u &&
          read16(ANGLES + 2u) == 0x0200u &&
          read16(ANGLES + 4u) == 0u, "init.angle_values");
    check(read32(HEIGHT) == 0x00400000u, "init.height_constant");
    check(read32(GEOM_Y) == 120u, "init.geometry_offset_y");
    check(read32(SLOT + 0x28u) == 0xFFF80000u &&
          read32(SLOT + 0x2Cu) == 0x00200000u,
          "init.slot_target");
    check(read32(SLOT + 0x38u) == read32(SLOT + 0x28u) &&
          read32(SLOT + 0x3Cu) == read32(SLOT + 0x2Cu) &&
          read32(SLOT + 0x40u) == 0x44444444u &&
          read32(SLOT + 0x44u) == 0x55555555u,
          "init.slot_snapshot");
    check(read32(POSITION + 0u) == 0x11111111u &&
          read32(POSITION + 4u) == 0x22222222u &&
          read32(POSITION + 8u) == 0x33333333u,
          "init.camera_position_copy");
    check(matrix_calls == 1 && matrix_args[0] == MATRIX &&
          matrix_args[1] == POSITION && matrix_args[2] == 0x00400000u &&
          matrix_args[3] == ANGLES, "init.matrix_arguments");
    check(read32(MATRIX + 0u) == 0x61626364u &&
          read32(MATRIX + 4u) == 0x71727374u &&
          read32(MATRIX_B + 0u) == 0xA1A2A3A4u &&
          read32(MATRIX_B + 4u) == 0xB1B2B3B4u,
          "init.matrix_block_swap");
    check(read32(0x1F8000A0u) == 0xA1A2A3A4u &&
          read32(0x1F8000A4u) == 0xB1B2B3B4u,
          "init.guest_scratch_save");
    check_write_set(0, "init.write_set");
}

static void test_update(void)
{
    s32 result;

    seed();
    write32(SLOT + 0x28u, 0u);
    write32(SLOT + 0x2Cu, 0x00020000u);
    write32(SLOT + 0x38u, 0u);
    write32(SLOT + 0x3Cu, 0u);
    write16(HELD, 0x9004u);
    write16(RELEASED, 0x0040u);
    write32(D554, 1u);
    write32(D7CC, 2u);
    write32(HEIGHT, 0x00400000u);
    write32(GEOM_H, 0x123u);
    snapshot();
    result = wm_800776E0(SLOT_INDEX);
    check(result == 1, "update.return_one");
    check(read32(D554) == 0u && read32(D7CC) == 0u,
          "update.release_exit");
    check(read32(SLOT + 0x28u) == 0x00008000u &&
          read32(SLOT + 0x2Cu) == 0x00010000u,
          "update.input_masks");
    check(read32(SLOT + 0x38u) == 0x00001000u &&
          read32(SLOT + 0x3Cu) == 0x00002000u,
          "update.eighth_smoothing");
    check(matrix_angles[0] == 1 && matrix_angles[1] == 2,
          "update.matrix_angles_before_half_turn");
    check(read16(ANGLES + 2u) == 0x0802u,
          "update.half_turn_wrap");
    check(geom_calls == 1 && geom_h == 0x123,
          "update.geometry_screen");
    check_write_set(1, "update.write_set");
}

static void test_lower_limit_is_inclusive(void)
{
    seed();
    write32(SLOT + 0x28u, 0xFFE88000u);
    write32(SLOT + 0x2Cu, 0u);
    write32(SLOT + 0x38u, 0xFFE80000u);
    write32(SLOT + 0x3Cu, 0u);
    write16(HELD, 0x4000u);
    write16(RELEASED, 0u);
    write32(HEIGHT, 0u);
    write32(GEOM_H, 256u);
    (void)wm_800776E0(SLOT_INDEX);
    check(read32(SLOT + 0x28u) == 0xFFE80000u,
          "update.lower_limit_inclusive");
}

static void test_scheduler_guest_resolution(void)
{
    seed();
    write16(SLOT + 0x00u, 0u);
    write32(SLOT + 0x18u, 0x8007756Cu);
    write32(SLOT + 0x1Cu, 0x800776E0u);
    wm_sched_reset();
    wm_sched_callback_registry_clear();
    wm_80097800();
    check(read16(SLOT + 0x00u) == 1u && matrix_calls == 1,
          "scheduler.cb0_guest_resolution");

    write16(HELD, 0u);
    write16(RELEASED, 0u);
    write32(GEOM_H, 256u);
    wm_80097800();
    check(read16(SLOT + 0x00u) == 1u && matrix_calls == 2 &&
          geom_calls == 1,
          "scheduler.cb1_guest_resolution");
}

int main(void)
{
    test_init();
    test_update();
    test_lower_limit_is_inclusive();
    test_scheduler_guest_resolution();
    if (failures != 0) {
        fprintf(stderr, "W34N56 MODE8/11 CALLBACK CERTIFICATE: %d failure(s)\n",
                failures);
        return 1;
    }
    puts("W34N56 MODE8/11 CALLBACK CERTIFICATE PASS");
    return 0;
}
