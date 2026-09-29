/* psyq_compat_leaf.inc -- Psy-Q compatibility: small libgpu/libgte entry
 * points the game calls by name.
 *
 * Written from the documented interface, not from the retail code:
 *   - Psy-Q Library Reference / libgpu.h: SetPolyFT3, SetPolyG3, SetPolyGT3,
 *     SetLineF2, SetLineG2 "initialise the primitive": set the packet length
 *     (words after the tag) and the GP0 command code (psx-spx "GPU Render
 *     Polygon/Line Commands").  Only len and code change.
 *   - ReadGeomScreen returns the projection distance H (GTE control
 *     register 26, psx-spx "GTE Registers").
 *   - SetMulMatrix(m0, m1) "multiplies m0 by m1 and sets the result as the
 *     GTE rotation matrix; returns m0": each column of m1 is transformed by
 *     RT = m0 with MVMVA (sf=1, V0, no translation -- PsyCross gte_rtv0) and
 *     IR1..IR3 become that column of the product, which is then packed into
 *     the RT control registers (psx-spx layout R11R12, R13R21, R22R23,
 *     R31R32, R33).  Neither input is modified.
 *   - RotMatrixZYX(r, m) builds the Q12 rotation matrix Rz * Ry * Rx for the
 *     angles in r (rotation about X first) with integer products shifted
 *     right by 12 (arithmetic, i.e. floored); it does not touch the GTE or
 *     m->t.
 *
 * Included by retail_leaf_adapters.c so every test that links that TU keeps
 * linking these symbols; tools/analysis/psyq_port_guard.py treats this file
 * as part of the compat layer.
 */

s32 ReadGeomScreen(void)
{
    return (s32)CFC2(26); /* H */
}

MATRIX* SetMulMatrix(MATRIX* m0, MATRIX* m1)
{
    s32 product[3][3];
    int i, j;

    /* RT = m0 */
    CTC2((u16)m0->m[0][0] | (u32)(u16)m0->m[0][1] << 16, 0);
    CTC2((u16)m0->m[0][2] | (u32)(u16)m0->m[1][0] << 16, 1);
    CTC2((u16)m0->m[1][1] | (u32)(u16)m0->m[1][2] << 16, 2);
    CTC2((u16)m0->m[2][0] | (u32)(u16)m0->m[2][1] << 16, 3);
    CTC2((u32)(s32)m0->m[2][2], 4);
    for (j = 0; j < 3; j++) {
        /* V0 = column j of m1 */
        MTC2((u16)m1->m[0][j] | (u32)(u16)m1->m[1][j] << 16, 0);
        MTC2((u32)(s32)m1->m[2][j], 1);
        doCOP2(0x00486012); /* MVMVA RT*V0, sf=1 (gte_rtv0) */
        for (i = 0; i < 3; i++)
            product[i][j] = (s32)MFC2(9 + i); /* IR1..IR3 */
    }
    /* RT = m0 * m1 */
    CTC2((u16)product[0][0] | (u32)(u16)product[0][1] << 16, 0);
    CTC2((u16)product[0][2] | (u32)(u16)product[1][0] << 16, 1);
    CTC2((u16)product[1][1] | (u32)(u16)product[1][2] << 16, 2);
    CTC2((u16)product[2][0] | (u32)(u16)product[2][1] << 16, 3);
    CTC2((u32)product[2][2], 4);
    return m0;
}

/* Packet length = words after the tag; code = GP0 command (psx-spx "GPU
 * Render Polygon/Line Commands").  Spelled out rather than taken from a
 * setPoly/setLine macro because PsyCross's libgpu.h gives LINE_F2 a length
 * of 2, while the packet (colour+code word, two vertex words) is 3 words. */
void SetPolyFT3(POLY_FT3* primitive) { setlen(primitive, 7); setcode(primitive, 0x24); } /* col, 3x(xy,uv/clut/tpage) */
void SetPolyG3(POLY_G3* primitive)   { setlen(primitive, 6); setcode(primitive, 0x30); } /* 3x(col, xy) */
void SetPolyGT3(POLY_GT3* primitive) { setlen(primitive, 9); setcode(primitive, 0x34); } /* 3x(col, xy, uv) */
void SetLineF2(LINE_F2* primitive)   { setlen(primitive, 3); setcode(primitive, 0x40); } /* col, 2x xy */
void SetLineG2(LINE_G2* primitive)   { setlen(primitive, 4); setcode(primitive, 0x50); } /* 2x(col, xy) */

/* Q12 product, floored (arithmetic shift right by 12). */
static s32 CompatQ12Mul(s32 a, s32 b)
{
    return (s32)(((int64_t)a * b) >> 12);
}

MATRIX* RotMatrixZYX(SVECTOR* rotation, MATRIX* matrix)
{
    /* The game's libgte shim names are swapped (include_shim/psyq/libgte.h):
     * rcos() yields the sine and rsin() the cosine. */
    s32 sin_x = rcos(rotation->vx), cos_x = rsin(rotation->vx);
    s32 sin_y = rcos(rotation->vy), cos_y = rsin(rotation->vy);
    s32 sin_z = rcos(rotation->vz), cos_z = rsin(rotation->vz);
    s32 sxsy = CompatQ12Mul(sin_x, sin_y);
    s32 cxsy = CompatQ12Mul(cos_x, sin_y);

    /* Rz * Ry * Rx */
    matrix->m[0][0] = CompatQ12Mul(cos_y, cos_z);
    matrix->m[0][1] = CompatQ12Mul(sxsy, cos_z) - CompatQ12Mul(cos_x, sin_z);
    matrix->m[0][2] = CompatQ12Mul(cxsy, cos_z) + CompatQ12Mul(sin_x, sin_z);
    matrix->m[1][0] = CompatQ12Mul(cos_y, sin_z);
    matrix->m[1][1] = CompatQ12Mul(sxsy, sin_z) + CompatQ12Mul(cos_x, cos_z);
    matrix->m[1][2] = CompatQ12Mul(cxsy, sin_z) - CompatQ12Mul(sin_x, cos_z);
    matrix->m[2][0] = -sin_y;
    matrix->m[2][1] = CompatQ12Mul(sin_x, cos_y);
    matrix->m[2][2] = CompatQ12Mul(cos_x, cos_y);
    return matrix;
}
