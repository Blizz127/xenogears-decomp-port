/*
 * World-map helper 0x80096F18 (GTE view matrix builder).
 */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_helper_96f18.h"

#define SCRATCH 0x1F800000u

static u16 f18_lhu(u32 a) { u16 v; memcpy(&v, PSX_ADDR(a), 2); return v; }
static s32 f18_lw(u32 a) { s32 v; memcpy(&v, PSX_ADDR(a), 4); return v; }
static void f18_sh(u32 a, u16 v) { memcpy(PSX_ADDR(a), &v, 2); }
static void f18_sw(u32 a, s32 v) { memcpy(PSX_ADDR(a), &v, 4); }

void wm_80096F18(u32 out_matrix, u32 pos_vec, s32 height, u32 rot_svec)
{
    SVECTOR rot;

    /* Set output matrix column 0: (0, pos.Y>>12, 0) */
    f18_sh(out_matrix + 8, 0);
    f18_sh(out_matrix + 0xC, 0);
    f18_sh(out_matrix + 0xA, (u16)((s32)f18_lw(pos_vec + 4) >> 12));

    /* Build rotation SVECTOR from rot_svec */
    rot.vx = (s16)f18_lhu(rot_svec);
#if defined(WM_F18_MUTANT_WRONG_ANGLE_SOURCE)
    rot.vy = (s16)f18_lhu(rot_svec + 4);
#else
    rot.vy = (s16)f18_lhu(rot_svec + 2);
#endif
    rot.vz = 0;

    /* Retail stages the angles at +0xA0 and publishes the matrix at +0xF0. */
    f18_sh(SCRATCH + 0xA0, (u16)rot.vx);
    f18_sh(SCRATCH + 0xA2, (u16)rot.vy);
    f18_sh(SCRATCH + 0xA4, (u16)rot.vz);
#if defined(WM_F18_MUTANT_WRONG_MATRIX_DESTINATION)
    RotMatrixYXZ((SVECTOR*)PSX_ADDR(SCRATCH + 0xA0),
                 (MATRIX*)PSX_ADDR(SCRATCH + 0xA0));
#else
    RotMatrixYXZ((SVECTOR*)PSX_ADDR(SCRATCH + 0xA0),
                 (MATRIX*)PSX_ADDR(SCRATCH + 0xF0));
#endif

    /* ApplyMatrixLV: transform (0, 0, -height>>12) through matrix */
    {
#if defined(WM_F18_MUTANT_WRONG_HEIGHT_SIGN)
        s32 h = height >> 12;
#else
        s32 h = -(height >> 12);
#endif
        f18_sw(SCRATCH, 0);
        f18_sw(SCRATCH + 4, 0);
        f18_sw(SCRATCH + 8, h);
    }
    ApplyMatrixLV((MATRIX*)PSX_ADDR(SCRATCH + 0xF0),
                  (VECTOR*)PSX_ADDR(SCRATCH),
                  (VECTOR*)PSX_ADDR(SCRATCH + 0x10));

    /* Store first column result */
    f18_sh(out_matrix + 0, (u16)f18_lw(SCRATCH + 0x10));
#if defined(WM_F18_MUTANT_MISSING_POSITION_Y)
    f18_sh(out_matrix + 2, (u16)f18_lw(SCRATCH + 0x14));
#else
    f18_sh(out_matrix + 2,
           (u16)(f18_lw(SCRATCH + 0x14) + f18_lw(out_matrix + 0xA)));
#endif
    f18_sh(out_matrix + 4, (u16)f18_lw(SCRATCH + 0x18));

    /* Build second rotation */
    f18_sh(SCRATCH + 0xA0, 0);
#if defined(WM_F18_MUTANT_WRONG_ROTATION_ORDER)
    f18_sh(SCRATCH + 0xA2, f18_lhu(rot_svec + 4));
    f18_sh(SCRATCH + 0xA4, f18_lhu(rot_svec + 2));
#else
    f18_sh(SCRATCH + 0xA2, f18_lhu(rot_svec + 2));
    f18_sh(SCRATCH + 0xA4, f18_lhu(rot_svec + 4));
#endif
    RotMatrixYXZ((SVECTOR*)PSX_ADDR(SCRATCH + 0xA0),
                 (MATRIX*)PSX_ADDR(SCRATCH + 0xF0));

    /* ApplyMatrix: transform (0, -0x1000, 0) through matrix */
    f18_sh(SCRATCH + 0xA0, 0);
    f18_sh(SCRATCH + 0xA2, (u16)(s16)(-0x1000));
    f18_sh(SCRATCH + 0xA4, 0);
    ApplyMatrix((MATRIX*)PSX_ADDR(SCRATCH + 0xF0),
                (SVECTOR*)PSX_ADDR(SCRATCH + 0xA0),
                (VECTOR*)PSX_ADDR(out_matrix + 0x10));
}
