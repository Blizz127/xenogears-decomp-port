/*
 * Retail model-bounds visibility helper 0x8003101C.
 *
 * The retail caller passes the model header and D_80050104.  Depending on
 * bits 0 and 1, this routine projects three or four triplets sampled from
 * the two AABB corners at header offsets 0x20 and 0x28.  It returns zero as
 * soon as a sampled point is visible and one only when every selected point
 * lies outside the screen.
 */
#include <string.h>

#include <psx/gtereg.h>

#include "common.h"
#include "psyq/libgte.h"
#include "world_map_helper_3101c.h"

extern s32 D_800500F8;
extern s32 D_800500FC;

static s16 f3101c_read_s16(const u8* address)
{
    s16 value;

    memcpy(&value, address, sizeof(value));
    return value;
}

/* Retail computes base + trunc((target - base) / 2), not (base + target)/2.
 * Those differ by one for some odd signed pairs. */
static s16 f3101c_midpoint(s16 base, s16 target)
{
#if defined(WM_3101C_MUTANT_M2)
    return (s16)(((s32)base + (s32)target) / 2);
#else
    return (s16)((s32)base + ((s32)target - (s32)base) / 2);
#endif
}

static SVECTOR f3101c_vector(s16 x, s16 y, s16 z)
{
    SVECTOR result;

    result.vx = x;
    result.vy = y;
    result.vz = z;
    result.pad = 0;
    return result;
}

static int f3101c_point_visible(u16 depth, u32 sxy)
{
#if defined(WM_3101C_MUTANT_M5)
    int depth_valid = depth != 0u;
#else
    int depth_valid = (u16)((u32)depth + 1u) >= 2u;
#endif

    return depth_valid != 0 &&
           sxy < (u32)D_800500FC &&
           (u32)(u16)sxy < (u32)D_800500F8;
}

static int f3101c_triplet_visible(SVECTOR v0, SVECTOR v1, SVECTOR v2)
{
    long xy0 = 0;
    long xy1 = 0;
    long xy2 = 0;
    long p = 0;
    long flag = 0;

    (void)RotTransPers3(&v0, &v1, &v2, &xy0, &xy1, &xy2, &p, &flag);
    return f3101c_point_visible((u16)C2_SZ1, (u32)xy0) ||
           f3101c_point_visible((u16)C2_SZ2, (u32)xy1) ||
           f3101c_point_visible((u16)C2_SZ3, (u32)xy2);
}

s32 func_8003101C(const u8* bounds, s32 mode)
{
#if defined(WM_3101C_MUTANT_M4)
    if ((mode & 3) == 0)
        return 0;
#else
    if ((mode & 3) == 0)
        return 1;
#endif

    s16 x0 = f3101c_read_s16(bounds + 0x20u);
    s16 y0 = f3101c_read_s16(bounds + 0x22u);
    s16 z0 = f3101c_read_s16(bounds + 0x24u);
    s16 x1 = f3101c_read_s16(bounds + 0x28u);
    s16 y1 = f3101c_read_s16(bounds + 0x2Au);
    s16 z1 = f3101c_read_s16(bounds + 0x2Cu);
    s16 x01 = f3101c_midpoint(x0, x1);
    s16 y01 = f3101c_midpoint(y0, y1);
    s16 z01 = f3101c_midpoint(z0, z1);
    s16 x10 = f3101c_midpoint(x1, x0);
    s16 y10 = f3101c_midpoint(y1, y0);
    s16 z10 = f3101c_midpoint(z1, z0);

#if !defined(WM_3101C_MUTANT_M1)
    if ((mode & 1) != 0) {
        if (f3101c_triplet_visible(f3101c_vector(x0, y0, z0),
                                   f3101c_vector(x1, y1, z1),
                                   f3101c_vector(x01, y01, z01)))
            return 0;
        if (f3101c_triplet_visible(f3101c_vector(x1, y0, z0),
                                   f3101c_vector(x0, y1, z0),
                                   f3101c_vector(x1, y1, z0)))
            return 0;
        if (f3101c_triplet_visible(f3101c_vector(x0, y0, z1),
                                   f3101c_vector(x1, y0, z1),
                                   f3101c_vector(x0, y1, z1)))
            return 0;
    }
#endif

    if ((mode & 2) != 0) {
        if (f3101c_triplet_visible(f3101c_vector(x01, y0, z0),
                                   f3101c_vector(x0, y0, z01),
                                   f3101c_vector(x0, y01, z0)))
            return 0;
        if (f3101c_triplet_visible(f3101c_vector(x1, y10, z0),
                                   f3101c_vector(x10, y1, z0),
                                   f3101c_vector(x1, y1, z01)))
            return 0;
        if (f3101c_triplet_visible(f3101c_vector(x0, y1, z10),
                                   f3101c_vector(x0, y10, z1),
                                   f3101c_vector(x01, y1, z1)))
            return 0;
#if !defined(WM_3101C_MUTANT_M3)
        if (f3101c_triplet_visible(f3101c_vector(x1, y0, z10),
                                   f3101c_vector(x10, y0, z1),
                                   f3101c_vector(x1, y01, z1)))
            return 0;
#endif
    }

    return 1;
}
