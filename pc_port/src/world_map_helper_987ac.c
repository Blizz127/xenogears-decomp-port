/*
 * Retail world-map four-corner classifier 0x800987AC.
 *
 * The helper transforms four SVECTOR corners through the live GTE matrix,
 * then classifies the resulting quadrilateral against four X/Z and Y/Z
 * clipping planes. Return values are retail's tri-state result:
 *   -1: every corner lies outside at least one plane
 *    0: the quadrilateral intersects one or more planes
 *    1: no adjacent corner pair lies outside any plane
 */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_helper_987ac.h"

#define WM_987AC_SC_OUT0 0x1F800000u
#define WM_987AC_SC_OUT1 0x1F800010u
#define WM_987AC_SC_OUT2 0x1F800020u
#define WM_987AC_SC_OUT3 0x1F800030u

#define WM_987AC_PLANE_XZ0_X 0x8009C828u
#define WM_987AC_PLANE_XZ0_Z 0x8009C830u
#define WM_987AC_PLANE_XZ1_X 0x8009C844u
#define WM_987AC_PLANE_XZ1_Z 0x8009C84Cu
#define WM_987AC_PLANE_YZ0_Y 0x8009C878u
#define WM_987AC_PLANE_YZ0_Z 0x8009C87Cu
#define WM_987AC_PLANE_YZ1_Y 0x8009C7F4u
#define WM_987AC_PLANE_YZ1_Z 0x8009C7F8u

static u32 wm_987ac_lw(u32 addr)
{
    u32 value;

    memcpy(&value, PSX_ADDR(addr), sizeof(value));
    return value;
}

static s32 wm_987ac_as_s32(u32 bits)
{
    s32 value;

    memcpy(&value, &bits, sizeof(value));
    return value;
}

/* MIPS mult/mflo retains the low 32 product bits before the addu. */
static u32 wm_987ac_mul_lo(u32 lhs, u32 rhs)
{
    int64_t product = (int64_t)wm_987ac_as_s32(lhs) *
                      (int64_t)wm_987ac_as_s32(rhs);

    return (u32)(uint64_t)product;
}

static u32 wm_987ac_sra12(u32 bits)
{
    u32 shifted = bits >> 12;

    if ((bits & UINT32_C(0x80000000)) != 0u)
        shifted |= UINT32_C(0xFFF00000);
    return shifted;
}

#if defined(WM_987AC_TEST_TRACE)
extern void wm_987ac_test_rot_trans(u32 input_addr, u32 output_addr);
#define WM_987AC_ROT_TRANS(input_addr, output_addr) \
    wm_987ac_test_rot_trans((input_addr), (output_addr))
#else
static void wm_987ac_rot_trans(u32 input_addr, u32 output_addr)
{
    long dead_flag;

    RotTrans((SVECTOR *)PSX_ADDR(input_addr),
             (VECTOR *)PSX_ADDR(output_addr), &dead_flag);
}
#define WM_987AC_ROT_TRANS(input_addr, output_addr) \
    wm_987ac_rot_trans((input_addr), (output_addr))
#endif

static s32 wm_987ac_dot2(u32 output_addr, u32 component_offset,
                         u32 first_factor_addr, u32 second_factor_addr)
{
    u32 first = wm_987ac_mul_lo(
        wm_987ac_lw(output_addr + component_offset),
        wm_987ac_lw(first_factor_addr));
    u32 second = wm_987ac_mul_lo(
        wm_987ac_lw(output_addr + 8u),
        wm_987ac_lw(second_factor_addr));

    return wm_987ac_as_s32(wm_987ac_sra12(first + second));
}

/* Retail visits corners in 0,1,3,2 order around the quad and counts the
 * four adjacent pairs for which both plane distances are negative. */
static s32 wm_987ac_negative_pair_count(s32 q0, s32 q1, s32 q2, s32 q3)
{
    s32 count = 0;

#if defined(WM_987AC_MUTANT_CORNER_ORDER)
    if (q0 < 0 && q1 < 0) count++;
    if (q1 < 0 && q2 < 0) count++;
    if (q2 < 0 && q3 < 0) count++;
    if (q3 < 0 && q0 < 0) count++;
#else
    if (q0 < 0 && q1 < 0) count++;
    if (q1 < 0 && q3 < 0) count++;
    if (q3 < 0 && q2 < 0) count++;
    if (q2 < 0 && q0 < 0) count++;
#endif
    return count;
}

static s32 wm_987ac_plane_count(u32 component_offset,
                                u32 first_factor_addr,
                                u32 second_factor_addr)
{
    s32 q0 = wm_987ac_dot2(WM_987AC_SC_OUT0, component_offset,
                           first_factor_addr, second_factor_addr);
    s32 q1 = wm_987ac_dot2(WM_987AC_SC_OUT1, component_offset,
                           first_factor_addr, second_factor_addr);
    s32 q2 = wm_987ac_dot2(WM_987AC_SC_OUT2, component_offset,
                           first_factor_addr, second_factor_addr);
    s32 q3 = wm_987ac_dot2(WM_987AC_SC_OUT3, component_offset,
                           first_factor_addr, second_factor_addr);

    return wm_987ac_negative_pair_count(q0, q1, q2, q3);
}

s32 wm_800987AC(u32 corner0, u32 corner1, u32 corner2, u32 corner3)
{
    s32 xz0;
    s32 xz1;
    s32 yz0;
    s32 yz1;

    WM_987AC_ROT_TRANS(corner0, WM_987AC_SC_OUT0);
    WM_987AC_ROT_TRANS(corner1, WM_987AC_SC_OUT1);
    WM_987AC_ROT_TRANS(corner2, WM_987AC_SC_OUT2);
#if !defined(WM_987AC_MUTANT_SKIP_CORNER3)
    WM_987AC_ROT_TRANS(corner3, WM_987AC_SC_OUT3);
#else
    (void)corner3;
#endif

    xz0 = wm_987ac_plane_count(0u, WM_987AC_PLANE_XZ0_X,
                               WM_987AC_PLANE_XZ0_Z);
    if (xz0 == 4)
#if defined(WM_987AC_MUTANT_OUTSIDE_ZERO)
        return 0;
#else
        return -1;
#endif

    xz1 = wm_987ac_plane_count(0u, WM_987AC_PLANE_XZ1_X,
                               WM_987AC_PLANE_XZ1_Z);
    if (xz1 == 4)
        return -1;

#if defined(WM_987AC_MUTANT_WRONG_YZ_COMPONENT)
    yz0 = wm_987ac_plane_count(0u, WM_987AC_PLANE_YZ0_Y,
                               WM_987AC_PLANE_YZ0_Z);
#else
    yz0 = wm_987ac_plane_count(4u, WM_987AC_PLANE_YZ0_Y,
                               WM_987AC_PLANE_YZ0_Z);
#endif
    if (yz0 == 4)
        return -1;

#if defined(WM_987AC_MUTANT_WRONG_YZ_COMPONENT)
    yz1 = wm_987ac_plane_count(0u, WM_987AC_PLANE_YZ1_Y,
                               WM_987AC_PLANE_YZ1_Z);
#else
    yz1 = wm_987ac_plane_count(4u, WM_987AC_PLANE_YZ1_Y,
                               WM_987AC_PLANE_YZ1_Z);
#endif
    if (yz1 == 4)
        return -1;

#if defined(WM_987AC_MUTANT_INSIDE_ZERO)
    return 0;
#else
    return ((xz0 | xz1 | yz0 | yz1) == 0) ? 1 : 0;
#endif
}
