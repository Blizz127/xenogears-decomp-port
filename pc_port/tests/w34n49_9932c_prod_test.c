/* W34N49 production-linked certificate for retail wm_8009932C. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_helper_9932c.h"

#define SCRATCH UINT32_C(0x1F800000)

static int s_failures;
static int s_dispatch_calls;
static u32 s_comp_args[3];
static u32 s_rot_arg;
static u32 s_trans_arg;

static void check(const char *name, int condition)
{
    if (!condition) {
        fprintf(stderr, "ASSERTION %s\n", name);
        s_failures++;
    }
}

static u16 ld16(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 ld32(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void st16(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void st32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u32 guest_of(const void *pointer)
{
    uintptr_t base = (uintptr_t)g_PsxRam;
    uintptr_t value = (uintptr_t)pointer;
    uintptr_t offset = value - base;

    if (value < base || offset >= PSX_RAM_SIZE)
        return UINT32_MAX;
    if (offset < UINT32_C(0x400))
        return UINT32_C(0x1F800000) + (u32)offset;
    return UINT32_C(0x80000000) | (u32)offset;
}

MATRIX *CompMatrix(MATRIX *left, MATRIX *right, MATRIX *output)
{
    s_comp_args[0] = guest_of(left);
    s_comp_args[1] = guest_of(right);
    s_comp_args[2] = guest_of(output);
    memcpy(output, right, sizeof(*output));
    return output;
}

void SetRotMatrix(MATRIX *matrix)
{
    s_rot_arg = guest_of(matrix);
}

void SetTransMatrix(MATRIX *matrix)
{
    s_trans_arg = guest_of(matrix);
}

void wm_80099708(u32 tile_data, u32 ot_base, u32 packet_base, u32 origin)
{
    (void)tile_data;
    (void)ot_base;
    (void)packet_base;
    (void)origin;
    s_dispatch_calls++;
}

static void test_skipped_cells_preserve_origins(void)
{
    const u32 position = UINT32_C(0x80040000);
    unsigned char origins_before[0x18];
    u32 i;

    PsxMemory_Init();
    memset(PSX_ADDR(SCRATCH + 0x330u), 0xA6, sizeof(origins_before));
    memcpy(origins_before, PSX_ADDR(SCRATCH + 0x330u),
           sizeof(origins_before));
    for (i = 0u; i < 25u; i++)
        st16(UINT32_C(0x8009D618) + i * 2u, UINT16_MAX);
    for (i = 0u; i < 32u; i++)
        ((u8 *)PSX_ADDR(UINT32_C(0x8009D534)))[i] = (u8)(0x80u + i);
    st32(position, UINT32_C(0x12345000));
    st32(position + 8u, UINT32_C(0x0FEDC000));
    st32(UINT32_C(0x8009D7DC), UINT32_C(0xDEADBEEF));

    wm_8009932C(UINT32_C(0x80020000), UINT32_C(0x80030000), position);

    check("skipped-cells-preserve-origin-scratch",
          memcmp(origins_before, PSX_ADDR(SCRATCH + 0x330u),
                 sizeof(origins_before)) == 0);
    check("skipped-cells-do-not-dispatch", s_dispatch_calls == 0);
    check("packet-count-reset", ld32(UINT32_C(0x8009D7DC)) == 0u);
    check("x-cursor-still-advances",
          ld16(SCRATCH + 0x328u) == UINT16_C(0x14BB));
    check("z-cursor-still-advances",
          ld16(SCRATCH + 0x32Cu) == UINT16_C(0xEEDC));
    check("matrix-compose-addresses",
          s_comp_args[0] == UINT32_C(0x8009C808) &&
          s_comp_args[1] == SCRATCH + 0x350u &&
          s_comp_args[2] == SCRATCH + 0x370u);
    check("matrix-publish-addresses",
          s_rot_arg == SCRATCH + 0x370u &&
          s_trans_arg == SCRATCH + 0x370u);
}

int main(void)
{
    test_skipped_cells_preserve_origins();
    if (s_failures != 0)
        return 1;
    puts("W34N49 0x8009932C exact-side-effect certificate PASS");
    return 0;
}
