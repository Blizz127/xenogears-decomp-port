/*
 * World-map warped-terrain height sampler 0x80093A5C.
 *
 * Exact retail boundary: [0x80093A5C, 0x80093E8C).
 */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psyq/libgte.h"
#include "psx_memory.h"
#include "world_map_helper_93a5c.h"
#include "world_map_plane_solver.h"
#include "world_map_terrain_cell.h"

#define WM_A5C_SCRATCH       0x1F800000u
#define WM_A5C_PHASE_X       0x8009C5BCu
#define WM_A5C_PHASE_Z       0x8009C618u
#define WM_A5C_COEFF_0       0x8009B244u
#define WM_A5C_COEFF_1       0x8009B24Cu
#define WM_A5C_COEFF_2       0x8009B254u
#define WM_A5C_COEFF_3       0x8009B25Cu

#define WM_A5C_EDGE_0        (WM_A5C_SCRATCH + 0x00u)
#define WM_A5C_EDGE_1        (WM_A5C_SCRATCH + 0x10u)
#define WM_A5C_CROSS         (WM_A5C_SCRATCH + 0x20u)
#define WM_A5C_H00           (WM_A5C_SCRATCH + 0xA2u)
#define WM_A5C_H10           (WM_A5C_SCRATCH + 0xAAu)
#define WM_A5C_H01           (WM_A5C_SCRATCH + 0xB2u)
#define WM_A5C_H11           (WM_A5C_SCRATCH + 0xBAu)

extern void OuterProduct0(VECTOR *v0, VECTOR *v1, VECTOR *out);
extern long VectorNormal(VECTOR *input, VECTOR *output);

static s32 a5c_bits_to_s32(u32 bits)
{
    s32 value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static u32 a5c_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u16 a5c_lhu(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static s8 a5c_lb(u32 address)
{
    s8 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u8 a5c_lbu(u32 address)
{
    u8 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void a5c_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void a5c_sh(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

/* Signed MULT followed by MFLO. */
static u32 a5c_mult_lo(u32 lhs_bits, u32 rhs_bits)
{
    int64_t product = (int64_t)a5c_bits_to_s32(lhs_bits) *
                      (int64_t)a5c_bits_to_s32(rhs_bits);
    return (u32)(uint64_t)product;
}

/* Retail's SLL 4 / SRA 3 sequence, including its 32-bit boundary. */
static s32 a5c_double_cosine(s32 cosine)
{
    return a5c_bits_to_s32((u32)cosine << 4u) >> 3;
}

static s16 a5c_corner_height(u32 phase_x, u32 phase_z, u32 cell_byte)
{
    s32 cosine_z = (s32)rcos(a5c_bits_to_s32(phase_z));
    s32 doubled_z = a5c_double_cosine(cosine_z);
    s32 cosine_x = (s32)rcos(a5c_bits_to_s32(phase_x));
    u32 product = a5c_mult_lo((u32)cosine_x, (u32)doubled_z);
    s32 wave = a5c_bits_to_s32(product) >> 20;
    s32 byte_height = (s32)a5c_lb(cell_byte) * 8;
    u16 result = (u16)((u32)wave + (u32)byte_height);
    s16 signed_result;

    memcpy(&signed_result, &result, sizeof(signed_result));
    return signed_result;
}

static u32 a5c_local_eighth(s32 coordinate)
{
    return (u32)(coordinate / 8) & UINT32_C(0xFFFF);
}

static int a5c_flag_is_set(u32 cell)
{
#if defined(W34N67_MUTANT_INVERT_FLAG)
    return (a5c_lbu(cell + 1u) & 0x80u) == 0u;
#else
    return (a5c_lbu(cell + 1u) & 0x80u) != 0u;
#endif
}

s32 wm_80093A5C(u32 x_bits, u32 z_bits)
{
    s32 x = a5c_bits_to_s32(x_bits);
    s32 z = a5c_bits_to_s32(z_bits);
#if defined(W34N67_MUTANT_CELL_Z_FROM_X)
    u32 cell = wm_80093660(x, x);
#else
    u32 cell = wm_80093660(x, z);
#endif
    u32 x_index = (x_bits >> 19u) & 7u;
    u32 z_index = (z_bits >> 19u) & 7u;
    u32 phase_x = a5c_lw(WM_A5C_PHASE_X);
    u32 phase_z = a5c_lw(WM_A5C_PHASE_Z);
    s16 h00;
    s16 h10;
    s16 h01;
    s16 h11;
    u32 local_x;
    u32 local_z;
    u32 selection;

    h00 = a5c_corner_height(phase_x + (x_index << 9u),
                            phase_z + (z_index << 9u), cell + 0x00u);
#if defined(W34N67_MUTANT_NO_X_NEIGHBOR)
    h10 = a5c_corner_height(phase_x + (x_index << 9u),
#else
    h10 = a5c_corner_height(phase_x + ((x_index + 1u) << 9u),
#endif
                            phase_z + (z_index << 9u), cell + 0x04u);
#if defined(W34N67_MUTANT_WRONG_H01_BYTE)
    h01 = a5c_corner_height(phase_x + (x_index << 9u),
                            phase_z + ((z_index + 1u) << 9u), cell + 0x08u);
#else
    h01 = a5c_corner_height(phase_x + (x_index << 9u),
                            phase_z + ((z_index + 1u) << 9u), cell + 0x24u);
#endif
    h11 = a5c_corner_height(phase_x + ((x_index + 1u) << 9u),
                            phase_z + ((z_index + 1u) << 9u), cell + 0x28u);

    a5c_sh(WM_A5C_H00, (u16)h00);
    a5c_sh(WM_A5C_H10, (u16)h10);
    a5c_sh(WM_A5C_H01, (u16)h01);
    a5c_sh(WM_A5C_H11, (u16)h11);

    local_x = a5c_local_eighth(x);
    local_z = a5c_local_eighth(z);

    if (a5c_flag_is_set(cell)) {
        selection = a5c_mult_lo(local_x, a5c_lw(WM_A5C_COEFF_2)) +
                    a5c_mult_lo(0u - local_z,
                                 a5c_lw(WM_A5C_COEFF_3));
        if (a5c_bits_to_s32(selection) < 0) {
            a5c_sw(WM_A5C_EDGE_0 + 0u, 128u);
            a5c_sw(WM_A5C_EDGE_0 + 4u, (u32)((s32)h11 - (s32)h00));
            a5c_sw(WM_A5C_EDGE_0 + 8u, (u32)-128);
            a5c_sw(WM_A5C_EDGE_1 + 0u, 0u);
            a5c_sw(WM_A5C_EDGE_1 + 4u, (u32)((s32)h01 - (s32)h00));
            a5c_sw(WM_A5C_EDGE_1 + 8u, (u32)-128);
        } else {
            a5c_sw(WM_A5C_EDGE_0 + 0u, 128u);
            a5c_sw(WM_A5C_EDGE_0 + 4u, (u32)((s32)h10 - (s32)h00));
            a5c_sw(WM_A5C_EDGE_0 + 8u, 0u);
            a5c_sw(WM_A5C_EDGE_1 + 0u, 128u);
            a5c_sw(WM_A5C_EDGE_1 + 4u, (u32)((s32)h11 - (s32)h00));
            a5c_sw(WM_A5C_EDGE_1 + 8u, (u32)-128);
        }
    } else {
        selection = a5c_mult_lo(local_x + UINT32_C(0xFFFF0000),
                                 a5c_lw(WM_A5C_COEFF_0)) +
                    a5c_mult_lo(0u - local_z,
                                 a5c_lw(WM_A5C_COEFF_1));
        if (a5c_bits_to_s32(selection) < 0) {
            a5c_sw(WM_A5C_EDGE_0 + 0u, (u32)-128);
            a5c_sw(WM_A5C_EDGE_0 + 4u, (u32)((s32)h01 - (s32)h10));
            a5c_sw(WM_A5C_EDGE_0 + 8u, (u32)-128);
            a5c_sw(WM_A5C_EDGE_1 + 0u, (u32)-128);
            a5c_sw(WM_A5C_EDGE_1 + 4u, (u32)((s32)h00 - (s32)h10));
            a5c_sw(WM_A5C_EDGE_1 + 8u, 0u);
        } else {
            a5c_sw(WM_A5C_EDGE_0 + 0u, 0u);
            a5c_sw(WM_A5C_EDGE_0 + 4u, (u32)((s32)h01 - (s32)h10));
            a5c_sw(WM_A5C_EDGE_0 + 8u, (u32)-128);
            a5c_sw(WM_A5C_EDGE_1 + 0u, (u32)-128);
            a5c_sw(WM_A5C_EDGE_1 + 4u, (u32)((s32)h11 - (s32)h10));
            a5c_sw(WM_A5C_EDGE_1 + 8u, (u32)-128);
        }
    }

#if defined(W34N67_MUTANT_SWAP_CROSS)
    OuterProduct0((VECTOR *)PSX_ADDR(WM_A5C_EDGE_0),
                  (VECTOR *)PSX_ADDR(WM_A5C_EDGE_1),
                  (VECTOR *)PSX_ADDR(WM_A5C_CROSS));
#else
    OuterProduct0((VECTOR *)PSX_ADDR(WM_A5C_EDGE_1),
                  (VECTOR *)PSX_ADDR(WM_A5C_EDGE_0),
                  (VECTOR *)PSX_ADDR(WM_A5C_CROSS));
#endif
    (void)VectorNormal((VECTOR *)PSX_ADDR(WM_A5C_CROSS),
                       (VECTOR *)PSX_ADDR(WM_A5C_EDGE_0));

    a5c_sw(WM_A5C_EDGE_1 + 0u, (x_bits >> 12u) & 0x7Fu);
#if defined(W34N67_MUTANT_POSITIVE_QUERY_Z)
    a5c_sw(WM_A5C_EDGE_1 + 8u, (z_bits >> 12u) & 0x7Fu);
#else
    a5c_sw(WM_A5C_EDGE_1 + 8u,
           0u - ((z_bits >> 12u) & 0x7Fu));
#endif
    if (a5c_flag_is_set(cell)) {
        a5c_sw(WM_A5C_CROSS + 0u, 0u);
        a5c_sw(WM_A5C_CROSS + 4u,
               (u32)(s32)(s16)a5c_lhu(WM_A5C_H00));
    } else {
#if defined(W34N67_MUTANT_BASE_ALWAYS_H00)
        a5c_sw(WM_A5C_CROSS + 0u, 0u);
        a5c_sw(WM_A5C_CROSS + 4u,
               (u32)(s32)(s16)a5c_lhu(WM_A5C_H00));
#else
        a5c_sw(WM_A5C_CROSS + 0u, 128u);
        a5c_sw(WM_A5C_CROSS + 4u,
               (u32)(s32)(s16)a5c_lhu(WM_A5C_H10));
#endif
    }
    a5c_sw(WM_A5C_CROSS + 8u, 0u);

    (void)wm_800935DC(WM_A5C_EDGE_1, WM_A5C_CROSS, WM_A5C_EDGE_0);
#if defined(W34N67_MUTANT_RETURN_SHIFT_3)
    return a5c_bits_to_s32(a5c_lw(WM_A5C_EDGE_1 + 4u) << 3u);
#else
    return a5c_bits_to_s32(a5c_lw(WM_A5C_EDGE_1 + 4u) << 12u);
#endif
}

/* Entry for the matched C in src/world_map/main.c (0x80093A5C is not
 * matched yet); both arguments are scalar fixed-point coordinates. */
s32 func_80093A5C(s32 x, s32 z)
{
    return wm_80093A5C((u32)x, (u32)z);
}
