/* Focused production-linked oracle for retail world helper 0x80093E8C. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_93e8c.h"

#define STRIDE_ADDR  0x8009D160u
#define TABLE_ADDR   0x8009C184u
#define VEC_ADDR     0x800A2000u
#define CELL_ADDR    0x800A4000u
#define RECORD_OFF   0x510u

typedef struct LoadEvent {
    u32 address;
    u32 width;
    u32 value;
} LoadEvent;

static LoadEvent s_loads[8];
static u32 s_load_count;
static int s_failures;

void wm_93e8c_test_load(u32 address, u32 width, u32 value)
{
    if (s_load_count < 8u) {
        s_loads[s_load_count].address = address;
        s_loads[s_load_count].width = width;
        s_loads[s_load_count].value = value;
    }
    s_load_count++;
}

static void check_s32(const char *name, s32 got, s32 expected)
{
    if (got != expected) {
        fprintf(stderr, "ASSERTION %s: got=%d (0x%08x) expected=%d (0x%08x)\n",
                name, got, (u32)got, expected, (u32)expected);
        s_failures++;
    }
}

static void check_u32(const char *name, u32 got, u32 expected)
{
    if (got != expected) {
        fprintf(stderr, "ASSERTION %s: got=0x%08x expected=0x%08x\n",
                name, got, expected);
        s_failures++;
    }
}

static void poke_u32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void poke_s16(u32 address, s16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u32 peek_u32(u32 address)
{
    u32 value;

    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 bits_from_s32(s32 value)
{
    u32 bits;

    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

/* Independent toward-zero /8 of a signed 32-bit value. */
static s32 spec_div8(s32 value)
{
    return value / 8;
}

/* Independent s16 restore: mask + subtract, not a cast chain. */
static s32 spec_s16(u32 bits)
{
    s32 v16 = (s32)(bits & 0xFFFFu);

    if (v16 >= 0x8000)
        v16 -= 0x10000;
    return v16;
}

static s32 oracle_return_from_bits(u16 half)
{
    return spec_s16(half);
}

static u32 packed_of(u32 x_bits, u32 z_bits)
{
    return ((z_bits >> 19) & 0xFu) * 16u + ((x_bits >> 19) & 0xFu);
}

static u32 table_index_of(u32 x_bits, u32 z_bits, u32 stride)
{
    s32 coarse_z = spec_div8((s32)(z_bits >> 20) -
                             (((z_bits & 0x80000000u) != 0u) ? (s32)0x1000 : 0));
    s32 coarse_x = spec_div8((s32)(x_bits >> 20) -
                             (((x_bits & 0x80000000u) != 0u) ? (s32)0x1000 : 0));
    return (u32)spec_s16((u32)coarse_z * stride + (u32)coarse_x);
}

static void reset_ram(void)
{
    memset(g_PsxRam, 0xA5, PSX_RAM_SIZE);
    memset(s_loads, 0, sizeof(s_loads));
    s_load_count = 0u;
}

static void plant_case(u32 x_bits, u32 y_bits, u32 z_bits, u32 stride,
                       u32 cell, s16 stored)
{
    u32 idx = table_index_of(x_bits, z_bits, stride);
    u32 packed = packed_of(x_bits, z_bits);
    u32 half_addr = cell + RECORD_OFF + packed * 2u;

    reset_ram();
    poke_u32(VEC_ADDR, x_bits);
    poke_u32(VEC_ADDR + 4u, y_bits);
    poke_u32(VEC_ADDR + 8u, z_bits);
    poke_u32(STRIDE_ADDR, stride);
    poke_u32(TABLE_ADDR + idx * 4u, cell);
    poke_s16(half_addr - 2u, (s16)0xCCDDu);
    poke_s16(half_addr + 2u, (s16)0xAABBu);
    poke_s16(half_addr, stored);
    poke_u32(VEC_ADDR - 4u, 0x11111111u);
    poke_u32(VEC_ADDR + 12u, 0x22222222u);
    poke_u32(STRIDE_ADDR - 4u, 0x33333333u);
    poke_u32(STRIDE_ADDR + 4u, 0x44444444u);
}

static s32 run_case(const char *name, u32 x_bits, u32 y_bits, u32 z_bits,
                    u32 stride, u32 cell, s16 stored)
{
    s32 got;
    s32 expect = oracle_return_from_bits((u16)stored);
    u32 idx = table_index_of(x_bits, z_bits, stride);
    u32 packed = packed_of(x_bits, z_bits);
    u32 half_addr = cell + RECORD_OFF + packed * 2u;
    char label[96];

    plant_case(x_bits, y_bits, z_bits, stride, cell, stored);
    got = wm_80093E8C(VEC_ADDR);

    snprintf(label, sizeof(label), "%s return", name);
    check_s32(label, got, expect);
    snprintf(label, sizeof(label), "%s Y unread", name);
    check_u32(label, peek_u32(VEC_ADDR + 4u), y_bits);
    snprintf(label, sizeof(label), "%s stride unread-write", name);
    check_u32(label, peek_u32(STRIDE_ADDR), stride);
    snprintf(label, sizeof(label), "%s pre-vector canary", name);
    check_u32(label, peek_u32(VEC_ADDR - 4u), 0x11111111u);
    snprintf(label, sizeof(label), "%s post-vector canary", name);
    check_u32(label, peek_u32(VEC_ADDR + 12u), 0x22222222u);
    snprintf(label, sizeof(label), "%s load count", name);
    check_u32(label, s_load_count, 5u);
    snprintf(label, sizeof(label), "%s load0 Z width", name);
    check_u32(label, s_loads[0].width, 4u);
    snprintf(label, sizeof(label), "%s load0 Z addr", name);
    check_u32(label, s_loads[0].address, VEC_ADDR + 8u);
    snprintf(label, sizeof(label), "%s load1 X addr", name);
    check_u32(label, s_loads[1].address, VEC_ADDR);
    snprintf(label, sizeof(label), "%s load2 stride addr", name);
    check_u32(label, s_loads[2].address, STRIDE_ADDR);
    snprintf(label, sizeof(label), "%s load3 table addr", name);
    check_u32(label, s_loads[3].address, TABLE_ADDR + idx * 4u);
    snprintf(label, sizeof(label), "%s load3 table width", name);
    check_u32(label, s_loads[3].width, 4u);
    snprintf(label, sizeof(label), "%s load4 lh addr", name);
    check_u32(label, s_loads[4].address, half_addr);
    snprintf(label, sizeof(label), "%s load4 lh width", name);
    check_u32(label, s_loads[4].width, 2u);
    return got;
}

static void test_load_count_and_halfword(void)
{
    /* Re-run a simple case and assert the lh itself. */
    u32 x = 0u;
    u32 z = 0u;
    s16 stored = 0x1234;
    u32 packed = packed_of(x, z);
    u32 half_addr = CELL_ADDR + RECORD_OFF + packed * 2u;

    plant_case(x, 0x00C0FFEEu, z, 4u, CELL_ADDR, stored);
    check_s32("zero-cell return", wm_80093E8C(VEC_ADDR), 0x1234);
    check_u32("zero-cell load count", s_load_count, 5u);
    check_u32("zero-cell lh addr", s_loads[4].address, half_addr);
    check_u32("zero-cell lh width", s_loads[4].width, 2u);
    check_u32("zero-cell lh value", s_loads[4].value, 0x1234u);
}

static void test_positive_and_pack(void)
{
    /* x nibble 3, z nibble 5 → packed 0x53. */
    u32 x = 3u << 19;
    u32 z = 5u << 19;
    s16 stored = 0x00AB;

    check_s32("pack return",
              run_case("pack", x, 0x13579BDFu, z, 4u, CELL_ADDR, stored),
              0x00AB);

    /* Bit 23 is inside 0x00FFF000 but outside 0x007FF000. */
    check_s32("mask-bit23 return",
              run_case("mask-bit23", 0u, 0x12121212u, 8u << 20, 4u,
                       CELL_ADDR, (s16)0x0044),
              0x0044);
}

static void test_negative_coarse(void)
{
    u32 x = bits_from_s32(-1);
    u32 z = bits_from_s32(-1);
    s16 stored = 7;

    check_s32("neg coarse return",
              run_case("neg-coarse", x, 0x2468ACE0u, z, 3u, CELL_ADDR, stored),
              7);
}

static void test_signed_return_not_boolean(void)
{
    s32 got;

    got = run_case("neg-half", 0u, 0x11111111u, 0u, 1u, CELL_ADDR, (s16)0x8001);
    check_s32("neg-half is -32767", got, -32767);
    check_s32("neg-half is not 0x8001u", got == (s32)0x8001u ? 1 : 0, 0);
    check_s32("neg-half is not boolean 1", got == 1 ? 1 : 0, 0);

    got = run_case("wide-half", 1u << 19, 0x22222222u, 2u << 19, 1u,
                   CELL_ADDR, (s16)0x6C0F);
    check_s32("wide-half full", got, 0x6C0F);
    check_s32("93FE4 low nibble", got & 0xF, 0xF);
    check_s32("94004 bits 10-15", (got >> 10) & 0x3F, 0x1B);
}

static void test_s16_index_truncation(void)
{
    /* stride=0x4000, coarse_z=2, coarse_x=0 → tile 0x8000 → s16 -32768. */
    u32 z = 16u << 20;
    s16 stored = -3;

    check_s32("s16-index return",
              run_case("s16-index", 0u, 0x33333333u, z, 0x4000u, CELL_ADDR,
                       stored),
              -3);
}

int main(void)
{
    PsxMemory_Init();

    test_load_count_and_halfword();
    test_positive_and_pack();
    test_negative_coarse();
    test_signed_return_not_boolean();
    test_s16_index_truncation();

    if (s_failures != 0) {
        fprintf(stderr, "ASSERTION failures=%d\n", s_failures);
        return 1;
    }
    printf("W34B22-I2 0x80093E8C focused oracle PASS\n");
    return 0;
}
