/*
 * Retail world-map visibility-mask producer 0x800983A0.
 *
 * A 5x5 grid of 0x800-unit cells is classified against the live camera.
 * Intersecting cells are subdivided into four 0x400-unit quadrants.  The
 * resulting tri-state masks are written to D_8009D618/D_8009D650 and then
 * merged with one of four retail boundary tables selected by input X/Z.
 */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_helper_983a0.h"
#include "world_map_helper_987ac.h"

#define WM_983A0_MATRIX_SOURCE  0x8009D534u
#define WM_983A0_CAMERA_MATRIX  0x8009C808u
#define WM_983A0_COARSE_MASK    0x8009D618u
#define WM_983A0_FINE_MASK      0x8009D650u
#define WM_983A0_BOUNDARY_TABLE 0x8009B7A8u

#define WM_983A0_SC_MATRIX      0x1F8000F0u
#define WM_983A0_SC_COMPOSED    0x1F800110u
#define WM_983A0_SC_BASE_X      0x1F800040u
#define WM_983A0_SC_BASE_Z      0x1F800048u

#define WM_983A0_SC_00 0x1F8000A0u
#define WM_983A0_SC_01 0x1F8000A8u
#define WM_983A0_SC_10 0x1F8000B0u
#define WM_983A0_SC_11 0x1F8000B8u
#define WM_983A0_SC_MT 0x1F8000C0u
#define WM_983A0_SC_ML 0x1F8000C8u
#define WM_983A0_SC_MR 0x1F8000D0u
#define WM_983A0_SC_MB 0x1F8000D8u
#define WM_983A0_SC_MC 0x1F8000E0u

static u32 wm_983a0_lw(u32 addr)
{
    u32 value;

    memcpy(&value, PSX_ADDR(addr), sizeof(value));
    return value;
}

static void wm_983a0_sw(u32 addr, u32 value)
{
    memcpy(PSX_ADDR(addr), &value, sizeof(value));
}

static void wm_983a0_sh(u32 addr, u16 value)
{
    memcpy(PSX_ADDR(addr), &value, sizeof(value));
}

static u32 wm_983a0_sra12(u32 bits)
{
    u32 shifted = bits >> 12;

    if ((bits & UINT32_C(0x80000000)) != 0u)
        shifted |= UINT32_C(0xFFF00000);
    return shifted;
}

#if defined(WM_983A0_TEST_TRACE)
extern void wm_983a0_test_comp_matrix(u32 left, u32 right, u32 output);
extern void wm_983a0_test_set_rot_matrix(u32 matrix);
extern void wm_983a0_test_set_trans_matrix(u32 matrix);
extern s32 wm_983a0_test_classify(u32 p0, u32 p1, u32 p2, u32 p3);
#define WM_983A0_COMP(left, right, output) \
    wm_983a0_test_comp_matrix((left), (right), (output))
#define WM_983A0_SET_ROT(matrix) wm_983a0_test_set_rot_matrix(matrix)
#define WM_983A0_SET_TRANS(matrix) wm_983a0_test_set_trans_matrix(matrix)
#define WM_983A0_CLASSIFY(p0, p1, p2, p3) \
    wm_983a0_test_classify((p0), (p1), (p2), (p3))
#else
#define WM_983A0_COMP(left, right, output) \
    ((void)CompMatrix((MATRIX *)PSX_ADDR(left), (MATRIX *)PSX_ADDR(right), \
                      (MATRIX *)PSX_ADDR(output)))
#define WM_983A0_SET_ROT(matrix) \
    SetRotMatrix((MATRIX *)PSX_ADDR(matrix))
#define WM_983A0_SET_TRANS(matrix) \
    SetTransMatrix((MATRIX *)PSX_ADDR(matrix))
#define WM_983A0_CLASSIFY(p0, p1, p2, p3) \
    wm_800987AC((p0), (p1), (p2), (p3))
#endif

static void wm_983a0_set_point(u32 point, u16 x, u16 z)
{
    wm_983a0_sh(point + 0u, x);
    wm_983a0_sh(point + 4u, z);
}

static u32 wm_983a0_pack_pair(s32 low, s32 high)
{
    return (u32)(u16)low | ((u32)(u16)high << 16);
}

static void wm_983a0_build_cell(u16 x, u16 z)
{
#if defined(WM_983A0_MUTANT_GRID_STEP)
    const u16 full_step = UINT16_C(0x0400);
#else
    const u16 full_step = UINT16_C(0x0800);
#endif
    const u16 half_step = UINT16_C(0x0400);

    wm_983a0_set_point(WM_983A0_SC_00, x, z);
    wm_983a0_set_point(WM_983A0_SC_01, (u16)(x + full_step), z);
    wm_983a0_set_point(WM_983A0_SC_10, x, (u16)(z - full_step));
    wm_983a0_set_point(WM_983A0_SC_11, (u16)(x + full_step),
                       (u16)(z - full_step));

    wm_983a0_set_point(WM_983A0_SC_MT, (u16)(x + half_step), z);
    wm_983a0_set_point(WM_983A0_SC_ML, x, (u16)(z - half_step));
    wm_983a0_set_point(WM_983A0_SC_MR, (u16)(x + full_step),
                       (u16)(z - half_step));
    wm_983a0_set_point(WM_983A0_SC_MB, (u16)(x + half_step),
                       (u16)(z - full_step));
    wm_983a0_set_point(WM_983A0_SC_MC, (u16)(x + half_step),
                       (u16)(z - half_step));
}

void wm_800983A0(u32 input_addr)
{
    u32 row;
    u32 column;
    u32 coarse_out = WM_983A0_COARSE_MASK;
    u32 fine_out = WM_983A0_FINE_MASK;
    u32 input_x;
    u32 input_z;
    u16 row_z;

#if defined(WM_983A0_MUTANT_MATRIX_SOURCE)
    memcpy(PSX_ADDR(WM_983A0_SC_MATRIX),
           PSX_ADDR(WM_983A0_MATRIX_SOURCE + 4u), 32u);
#else
    memcpy(PSX_ADDR(WM_983A0_SC_MATRIX),
           PSX_ADDR(WM_983A0_MATRIX_SOURCE), 32u);
#endif
    WM_983A0_COMP(WM_983A0_CAMERA_MATRIX, WM_983A0_SC_MATRIX,
                  WM_983A0_SC_COMPOSED);
    WM_983A0_SET_ROT(WM_983A0_SC_COMPOSED);
    WM_983A0_SET_TRANS(WM_983A0_SC_COMPOSED);

    input_x = wm_983a0_lw(input_addr + 0u);
    input_z = wm_983a0_lw(input_addr + 8u);
    wm_983a0_sw(WM_983A0_SC_BASE_X,
                0u - (wm_983a0_sra12(input_x) & UINT32_C(0x7FF)) -
                UINT32_C(0x1000));
    wm_983a0_sw(WM_983A0_SC_BASE_Z,
                0u - (wm_983a0_sra12(input_z) & UINT32_C(0x7FF)) -
                UINT32_C(0x1000));

    /* Retail initializes only the Y halfword of the nine scratch vectors. */
    wm_983a0_sh(WM_983A0_SC_00 + 2u, 0u);
    wm_983a0_sh(WM_983A0_SC_01 + 2u, 0u);
    wm_983a0_sh(WM_983A0_SC_10 + 2u, 0u);
    wm_983a0_sh(WM_983A0_SC_11 + 2u, 0u);
    wm_983a0_sh(WM_983A0_SC_MT + 2u, 0u);
    wm_983a0_sh(WM_983A0_SC_ML + 2u, 0u);
    wm_983a0_sh(WM_983A0_SC_MR + 2u, 0u);
    wm_983a0_sh(WM_983A0_SC_MB + 2u, 0u);
    wm_983a0_sh(WM_983A0_SC_MC + 2u, 0u);

    row_z = (u16)(0u - wm_983a0_lw(WM_983A0_SC_BASE_Z));
    for (row = 0u; row < 5u; row++) {
        u16 cell_x = (u16)wm_983a0_lw(WM_983A0_SC_BASE_X);

        for (column = 0u; column < 5u; column++) {
            s32 coarse;
            u32 fine0;
            u32 fine1;

            wm_983a0_build_cell(cell_x, row_z);
            coarse = WM_983A0_CLASSIFY(WM_983A0_SC_00,
                                       WM_983A0_SC_01,
                                       WM_983A0_SC_10,
                                       WM_983A0_SC_11);
            wm_983a0_sh(coarse_out, (u16)coarse);

#if defined(WM_983A0_MUTANT_SKIP_SUBDIVISION)
            fine0 = wm_983a0_pack_pair(coarse, coarse);
            fine1 = fine0;
#else
            if (coarse == 0) {
                s32 top_left = WM_983A0_CLASSIFY(
                    WM_983A0_SC_00, WM_983A0_SC_MT,
                    WM_983A0_SC_ML, WM_983A0_SC_MC);
                s32 top_right = WM_983A0_CLASSIFY(
                    WM_983A0_SC_MT, WM_983A0_SC_01,
                    WM_983A0_SC_MC, WM_983A0_SC_MR);
                s32 bottom_left = WM_983A0_CLASSIFY(
                    WM_983A0_SC_ML, WM_983A0_SC_MC,
                    WM_983A0_SC_10, WM_983A0_SC_MB);
                s32 bottom_right = WM_983A0_CLASSIFY(
                    WM_983A0_SC_MC, WM_983A0_SC_MR,
                    WM_983A0_SC_MB, WM_983A0_SC_11);

                fine0 = wm_983a0_pack_pair(top_left, top_right);
                fine1 = wm_983a0_pack_pair(bottom_left, bottom_right);
            } else {
                fine0 = wm_983a0_pack_pair(coarse, coarse);
                fine1 = fine0;
            }
#endif
            wm_983a0_sw(fine_out + 0u, fine0);
            wm_983a0_sw(fine_out + 4u, fine1);

            coarse_out += 2u;
            fine_out += 8u;
#if defined(WM_983A0_MUTANT_GRID_STEP)
            cell_x = (u16)(cell_x + UINT16_C(0x0400));
#else
            cell_x = (u16)(cell_x + UINT16_C(0x0800));
#endif
        }
        row_z = (u16)(row_z - UINT16_C(0x0800));
    }

    {
        u32 quadrant = (((wm_983a0_sra12(input_x) & UINT32_C(0x7FF)) <
                         UINT32_C(0x400)) ? 0u : 1u);
        u32 table;

        if ((wm_983a0_sra12(input_z) & UINT32_C(0x7FF)) >=
            UINT32_C(0x400))
            quadrant |= 2u;
#if defined(WM_983A0_MUTANT_QUADRANT)
        quadrant ^= 1u;
#endif
        table = WM_983A0_BOUNDARY_TABLE + quadrant * 200u;
        for (row = 0u; row < 25u; row++) {
#if defined(WM_983A0_MUTANT_REPLACE_MASK)
            wm_983a0_sw(WM_983A0_FINE_MASK + row * 8u,
                        wm_983a0_lw(table + row * 8u));
            wm_983a0_sw(WM_983A0_FINE_MASK + row * 8u + 4u,
                        wm_983a0_lw(table + row * 8u + 4u));
#else
            wm_983a0_sw(WM_983A0_FINE_MASK + row * 8u,
                        wm_983a0_lw(WM_983A0_FINE_MASK + row * 8u) |
                        wm_983a0_lw(table + row * 8u));
            wm_983a0_sw(WM_983A0_FINE_MASK + row * 8u + 4u,
                        wm_983a0_lw(WM_983A0_FINE_MASK + row * 8u + 4u) |
                        wm_983a0_lw(table + row * 8u + 4u));
#endif
        }
    }
}
