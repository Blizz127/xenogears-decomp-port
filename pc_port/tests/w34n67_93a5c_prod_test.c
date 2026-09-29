/* Production-linked certificate for retail world helper 0x80093A5C. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include <libgte.h>
#include "psx_memory.h"
#include "world_map_helper_93a5c.h"

#define CELL_ADDR       0x800A0000u
#define SCRATCH_ADDR    0x1F800000u
#define SCRATCH_OFFSET  0x00000000u
#define PHASE_X_ADDR    0x8009C5BCu
#define PHASE_Z_ADDR    0x8009C618u
#define COEFF_0_ADDR    0x8009B244u
#define COEFF_1_ADDR    0x8009B24Cu
#define COEFF_2_ADDR    0x8009B254u
#define COEFF_3_ADDR    0x8009B25Cu

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

static uint8_t before_ram[PSX_RAM_SIZE];
static int failures;
static s32 lookup_x;
static s32 lookup_z;
static int lookup_calls;
static int rcos_calls;
static int rcos_args[8];
static VECTOR outer_lhs;
static VECTOR outer_rhs;
static VECTOR outer_cross;
static VECTOR normal_input;
static int outer_calls;
static int normal_calls;
static u32 plane_a0;
static u32 plane_a1;
static u32 plane_a2;
static s32 plane_query[3];
static s32 plane_base[3];
static s32 plane_normal[3];
static u32 plane_result;
static int plane_calls;

static void check(int condition, const char *name)
{
    if (condition == 0) {
        fprintf(stderr, "ASSERTION %s FAILED\n", name);
        failures++;
    }
}

static s32 bits_to_s32(u32 bits)
{
    s32 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static void write_u32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u32 read_u32(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static s16 read_s16(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static s32 fake_cosine(s32 angle)
{
    u32 mixed = ((u32)angle * UINT32_C(13) + UINT32_C(0x345)) &
                UINT32_C(0x1FFF);
    return (s32)mixed - 4096;
}

int rsin(int angle)
{
    if (rcos_calls < 8)
        rcos_args[rcos_calls] = angle;
    rcos_calls++;
    return (int)fake_cosine((s32)angle);
}

u32 wm_80093660(s32 x, s32 z)
{
    lookup_x = x;
    lookup_z = z;
    lookup_calls++;
    return CELL_ADDR;
}

void OuterProduct0(VECTOR *lhs, VECTOR *rhs, VECTOR *out)
{
    int64_t x = (int64_t)lhs->vy * (int64_t)rhs->vz -
                (int64_t)lhs->vz * (int64_t)rhs->vy;
    int64_t y = (int64_t)lhs->vz * (int64_t)rhs->vx -
                (int64_t)lhs->vx * (int64_t)rhs->vz;
    int64_t z = (int64_t)lhs->vx * (int64_t)rhs->vy -
                (int64_t)lhs->vy * (int64_t)rhs->vx;

    outer_lhs = *lhs;
    outer_rhs = *rhs;
    out->vx = (int)(uint32_t)x;
    out->vy = (int)(uint32_t)y;
    out->vz = (int)(uint32_t)z;
    outer_cross = *out;
    outer_calls++;
}

long VectorNormal(VECTOR *input, VECTOR *output)
{
    normal_input = *input;
    output->vx = 101;
    output->vy = 4096;
    output->vz = -211;
    normal_calls++;
    return 0x1234L;
}

u32 wm_800935DC(u32 a0, u32 a1, u32 a2)
{
    plane_a0 = a0;
    plane_a1 = a1;
    plane_a2 = a2;
    memcpy(plane_query, PSX_ADDR(a0), sizeof(plane_query));
    memcpy(plane_base, PSX_ADDR(a1), sizeof(plane_base));
    memcpy(plane_normal, PSX_ADDR(a2), sizeof(plane_normal));
    write_u32(a0 + 4u, plane_result);
    plane_calls++;
    return plane_result;
}

static u32 mult_lo(u32 lhs_bits, u32 rhs_bits)
{
    int64_t product = (int64_t)bits_to_s32(lhs_bits) *
                      (int64_t)bits_to_s32(rhs_bits);
    return (u32)(uint64_t)product;
}

static s16 oracle_height(u32 phase_x, u32 phase_z, s8 byte_height)
{
    s32 cosine_z = fake_cosine(bits_to_s32(phase_z));
    s32 doubled_z = bits_to_s32((u32)cosine_z << 4u) >> 3;
    s32 cosine_x = fake_cosine(bits_to_s32(phase_x));
    s32 wave = bits_to_s32(mult_lo((u32)cosine_x,
                                   (u32)doubled_z)) >> 20;
    u16 result = (u16)((u32)wave + (u32)((s32)byte_height * 8));
    s16 signed_result;

    memcpy(&signed_result, &result, sizeof(signed_result));
    return signed_result;
}

static int vector3_equal(const VECTOR *actual, const s32 expected[3])
{
    return (s32)actual->vx == expected[0] &&
           (s32)actual->vy == expected[1] &&
           (s32)actual->vz == expected[2];
}

static int record3_equal(const s32 actual[3], const s32 expected[3])
{
    return actual[0] == expected[0] && actual[1] == expected[1] &&
           actual[2] == expected[2];
}

static int byte_is_authorized(size_t index)
{
    size_t relative;

    if (index < (size_t)SCRATCH_OFFSET ||
        index >= (size_t)SCRATCH_OFFSET + 0xBCu)
        return 0;
    relative = index - (size_t)SCRATCH_OFFSET;
    if (relative < 0x0Cu || (relative >= 0x10u && relative < 0x1Cu) ||
        (relative >= 0x20u && relative < 0x2Cu))
        return 1;
    return relative == 0xA2u || relative == 0xA3u ||
           relative == 0xAAu || relative == 0xABu ||
           relative == 0xB2u || relative == 0xB3u ||
           relative == 0xBAu || relative == 0xBBu;
}

static void check_write_set(void)
{
    size_t index;
    int clean = 1;

    for (index = 0; index < (size_t)PSX_RAM_SIZE; index++) {
        if (g_PsxRam[index] != before_ram[index] &&
            byte_is_authorized(index) == 0) {
            fprintf(stderr,
                    "WRITE_SET offset=0x%zx before=%02x after=%02x\n",
                    index, before_ram[index], g_PsxRam[index]);
            clean = 0;
            break;
        }
    }
    check(clean, "memory.write_set");
}

static void run_case(int flag_set, int negative_selection, int case_index)
{
    const u32 x_bits = UINT32_C(0x00123000);
    const u32 z_bits = UINT32_C(0x00234000);
    const u32 phase_x = UINT32_C(0x111);
    const u32 phase_z = UINT32_C(0x222);
    const u32 x_index = (x_bits >> 19u) & 7u;
    const u32 z_index = (z_bits >> 19u) & 7u;
    const s8 bytes[4] = { 3, -5, 7, -9 };
    s16 heights[4];
    int expected_angles[8];
    s32 expected_lhs[3];
    s32 expected_rhs[3];
    s32 expected_query[3];
    s32 expected_base[3];
    s32 expected_normal[3] = { 101, 4096, -211 };
    s32 result;
    int index;

    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(PSX_ADDR(SCRATCH_ADDR), 0xCC, 0xC0u);
    memset(PSX_ADDR(CELL_ADDR), 0, 0x40u);
    *(s8 *)PSX_ADDR(CELL_ADDR + 0x00u) = bytes[0];
    *(s8 *)PSX_ADDR(CELL_ADDR + 0x04u) = bytes[1];
    *(s8 *)PSX_ADDR(CELL_ADDR + 0x24u) = bytes[2];
    *(s8 *)PSX_ADDR(CELL_ADDR + 0x28u) = bytes[3];
    *(u8 *)PSX_ADDR(CELL_ADDR + 0x01u) = flag_set != 0 ? 0x80u : 0u;
    write_u32(PHASE_X_ADDR, phase_x);
    write_u32(PHASE_Z_ADDR, phase_z);
    write_u32(COEFF_0_ADDR,
              flag_set == 0 ? (negative_selection != 0 ? 1u : UINT32_MAX) : 0u);
    write_u32(COEFF_1_ADDR, 0u);
    write_u32(COEFF_2_ADDR,
              flag_set != 0 ? (negative_selection != 0 ? UINT32_MAX : 1u) : 0u);
    write_u32(COEFF_3_ADDR, 0u);

    memcpy(before_ram, g_PsxRam, sizeof(before_ram));
    lookup_calls = 0;
    rcos_calls = 0;
    memset(rcos_args, 0, sizeof(rcos_args));
    outer_calls = 0;
    normal_calls = 0;
    plane_calls = 0;
    plane_result = UINT32_C(0x1234) + (u32)case_index;

    heights[0] = oracle_height(phase_x + (x_index << 9u),
                               phase_z + (z_index << 9u), bytes[0]);
    heights[1] = oracle_height(phase_x + ((x_index + 1u) << 9u),
                               phase_z + (z_index << 9u), bytes[1]);
    heights[2] = oracle_height(phase_x + (x_index << 9u),
                               phase_z + ((z_index + 1u) << 9u), bytes[2]);
    heights[3] = oracle_height(phase_x + ((x_index + 1u) << 9u),
                               phase_z + ((z_index + 1u) << 9u), bytes[3]);

    expected_angles[0] = bits_to_s32(phase_z + (z_index << 9u));
    expected_angles[1] = bits_to_s32(phase_x + (x_index << 9u));
    expected_angles[2] = expected_angles[0];
    expected_angles[3] = bits_to_s32(phase_x + ((x_index + 1u) << 9u));
    expected_angles[4] = bits_to_s32(phase_z + ((z_index + 1u) << 9u));
    expected_angles[5] = expected_angles[1];
    expected_angles[6] = expected_angles[4];
    expected_angles[7] = expected_angles[3];

    if (flag_set != 0 && negative_selection != 0) {
        expected_rhs[0] = 128;
        expected_rhs[1] = (s32)heights[3] - (s32)heights[0];
        expected_rhs[2] = -128;
        expected_lhs[0] = 0;
        expected_lhs[1] = (s32)heights[2] - (s32)heights[0];
        expected_lhs[2] = -128;
    } else if (flag_set != 0) {
        expected_rhs[0] = 128;
        expected_rhs[1] = (s32)heights[1] - (s32)heights[0];
        expected_rhs[2] = 0;
        expected_lhs[0] = 128;
        expected_lhs[1] = (s32)heights[3] - (s32)heights[0];
        expected_lhs[2] = -128;
    } else if (negative_selection != 0) {
        expected_rhs[0] = -128;
        expected_rhs[1] = (s32)heights[2] - (s32)heights[1];
        expected_rhs[2] = -128;
        expected_lhs[0] = -128;
        expected_lhs[1] = (s32)heights[0] - (s32)heights[1];
        expected_lhs[2] = 0;
    } else {
        expected_rhs[0] = 0;
        expected_rhs[1] = (s32)heights[2] - (s32)heights[1];
        expected_rhs[2] = -128;
        expected_lhs[0] = -128;
        expected_lhs[1] = (s32)heights[3] - (s32)heights[1];
        expected_lhs[2] = -128;
    }

    result = wm_80093A5C(x_bits, z_bits);

    check(lookup_calls == 1 && lookup_x == bits_to_s32(x_bits) &&
          lookup_z == bits_to_s32(z_bits), "cell_lookup.coordinates");
    check(rcos_calls == 8, "height.rcos_count");
    for (index = 0; index < 8; index++)
        check(rcos_args[index] == expected_angles[index], "height.rcos_order");
    check(read_s16(SCRATCH_ADDR + 0xA2u) == heights[0] &&
          read_s16(SCRATCH_ADDR + 0xAAu) == heights[1] &&
          read_s16(SCRATCH_ADDR + 0xB2u) == heights[2] &&
          read_s16(SCRATCH_ADDR + 0xBAu) == heights[3],
          "height.corner_values");
    check(vector3_equal(&outer_lhs, expected_lhs) &&
          vector3_equal(&outer_rhs, expected_rhs), "selection.edges");
    check(outer_calls == 1 && vector3_equal(&outer_lhs, expected_lhs) &&
          vector3_equal(&outer_rhs, expected_rhs), "cross.operands");
    check(normal_calls == 1 &&
          (s32)normal_input.vx == (s32)outer_cross.vx &&
          (s32)normal_input.vy == (s32)outer_cross.vy &&
          (s32)normal_input.vz == (s32)outer_cross.vz,
          "normalize.input");

    expected_query[0] = (s32)((x_bits >> 12u) & 0x7Fu);
    expected_query[1] = bits_to_s32(plane_result);
    expected_query[2] = bits_to_s32(0u - ((z_bits >> 12u) & 0x7Fu));
    expected_base[0] = flag_set != 0 ? 0 : 128;
    expected_base[1] = flag_set != 0 ? (s32)heights[0] : (s32)heights[1];
    expected_base[2] = 0;
    check(plane_calls == 1 && plane_a0 == SCRATCH_ADDR + 0x10u &&
          plane_a1 == SCRATCH_ADDR + 0x20u && plane_a2 == SCRATCH_ADDR,
          "plane.arguments");
    check(plane_query[0] == expected_query[0] &&
          plane_query[2] == expected_query[2], "plane.query");
    check(record3_equal(plane_base, expected_base), "plane.base");
    check(record3_equal(plane_normal, expected_normal), "plane.normal");
    check(result == bits_to_s32(plane_result << 12u), "return.shift");
    check(read_u32(SCRATCH_ADDR + 0x14u) == plane_result,
          "plane.result_slot");
    check_write_set();
}

int main(void)
{
    run_case(1, 1, 0);
    run_case(1, 0, 1);
    run_case(0, 1, 2);
    run_case(0, 0, 3);
    if (failures != 0) {
        fprintf(stderr, "W34N67 93A5C CERTIFICATE: %d failure(s)\n", failures);
        return 1;
    }
    puts("W34N67 93A5C CERTIFICATE PASS");
    return 0;
}
