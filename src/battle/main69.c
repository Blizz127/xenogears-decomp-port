#include "common.h"
#ifndef XENO_PC_PORT
#include "psyq/libgte.h"
#endif


#ifndef XENO_PC_PORT
void func_800A5BE8(s32 a0, s32 a1, s32 a2, s32 a3, VECTOR* normal);
extern s32 D_800D3344;
extern s32 D_800D39CC;
s32 func_800A5870(s32 arg0, s32 arg1, s32 arg2);
#endif

#ifndef XENO_PC_PORT
#endif
#ifndef XENO_PC_PORT
extern s32 D_800D3348; /* triangle count of the D_800D39CC table */
extern s32 func_800A5A48(s32 v0, s32 v1, s32 v2, s32 arg);

/* First triangle of the D_800D39CC table (14-byte records of three vertex
 * indices into the 8-byte D_800D3344 table) for which func_800A5A48 returns
 * -1; -1 when there is none or the tables are not loaded. */
s32 func_800A579C(s32 arg0) {
    s32 i;

    if (D_800D3344 == 0 || D_800D39CC == 0) {
        return -1;
    }
    for (i = 0; i < D_800D3348; i++) {
        s16* tri = (s16*)(i * 14 + D_800D39CC);

        if (func_800A5A48(tri[0] * 8 + D_800D3344, tri[1] * 8 + D_800D3344,
                          tri[2] * 8 + D_800D3344, arg0) == -1) {
            return i;
        }
    }
    return -1;
}
#endif
#ifndef XENO_PC_PORT

#ifndef XENO_PC_PORT
s32 func_800A5870(s32 arg0, s32 arg1, s32 arg2) {
    s32 temp_s0;
    void *temp_v0;

    if ((D_800D3344 == 0) || (D_800D39CC == 0) || (arg1 < 0)) {
        return -1;
    }
    temp_s0 = arg1 * 0xE;
    temp_v0 = temp_s0 + D_800D39CC;
    func_800A5BE8((*(s16 *)((s8*)(temp_v0) + 0) * 8) + D_800D3344, (*(s16 *)((s8*)(temp_v0) + 2) * 8) + D_800D3344, (*(s16 *)((s8*)(temp_v0) + 4) * 8) + D_800D3344, arg0, arg2);
    return *(u8 *)((s8*)((temp_s0 + D_800D39CC)) + 0xC);
}
#endif

#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2F64; /* pass counter; its wrap clears every triangle's +0xD flag */
extern s32 func_800A5D54(s32 arg0, s32 tri, s32 depth);

/* Up to `tries` attempts of func_800A5D54 on triangle `tri`, stopping at the
 * first non-negative result, which is returned (-1 when the tables are not
 * loaded or `tri` is out of range). */
s32 func_800A5914(s32 arg0, s32 tri, s32 tries, s32 result) {
    s32 i;

    if (D_800D3344 == 0) {
        return -1;
    }
    if (D_800D39CC == 0) {
        return -1;
    }
    if (tri >= D_800D3348) {
        return -1;
    }
    for (i = 0; i < tries; i++) {
        result = func_800A5D54(arg0, tri, tries);
        if (result >= 0) {
            break;
        }
    }
    D_800D2F64++;
    if (D_800D2F64 == 0) {
        D_800D2F64 = 1;
        for (i = 0; i < D_800D3348; i++) {
            *(u8*)(i * 14 + D_800D39CC + 0xD) = 0;
        }
    }
    return result;
}
#endif
#ifndef XENO_PC_PORT
#endif
#ifndef XENO_PC_PORT
extern void OuterProduct0(VECTOR* v0, VECTOR* v1, VECTOR* v2);

/* Point-in-triangle test on the XZ plane: -1 when point a3 lies on the inner
 * side of all three edges of (a0, a1, a2) (SVECTOR addresses), else 0. */
s32 func_800A5A48(s32 a0, s32 a1, s32 a2, s32 a3) {
    SVECTOR* v0 = (SVECTOR*)a0;
    SVECTOR* v1 = (SVECTOR*)a1;
    SVECTOR* v2 = (SVECTOR*)a2;
    SVECTOR* p = (SVECTOR*)a3;
    VECTOR edge;
    VECTOR toP;
    VECTOR cross;

    edge.vx = v1->vx - v0->vx;
    edge.vy = 0;
    edge.vz = v1->vz - v0->vz;
    toP.vx = p->vx - v0->vx;
    toP.vy = 0;
    toP.vz = p->vz - v0->vz;
    OuterProduct0(&edge, &toP, &cross);
    if (cross.vy < 0) {
        return 0;
    }
    edge.vx = v2->vx - v1->vx;
    edge.vy = 0;
    edge.vz = v2->vz - v1->vz;
    toP.vx = p->vx - v1->vx;
    toP.vy = 0;
    toP.vz = p->vz - v1->vz;
    OuterProduct0(&edge, &toP, &cross);
    if (cross.vy >= 0) {
        edge.vx = v0->vx - v2->vx;
        edge.vy = 0;
        edge.vz = v0->vz - v2->vz;
        toP.vx = p->vx - v2->vx;
        toP.vy = 0;
        toP.vz = p->vz - v2->vz;
        OuterProduct0(&edge, &toP, &cross);
        return ~cross.vy >> 31;
    }
    return 0;
}
#endif
#ifndef XENO_PC_PORT
#endif
#ifndef XENO_PC_PORT
/* Height of triangle (a0, a1, a2) (SVECTOR addresses) at point a3's x/z:
 * the plane normal goes to *normal and the height into the point's vy
 * (0 for a vertical plane). */
void func_800A5BE8(s32 a0, s32 a1, s32 a2, s32 a3, VECTOR* normal) {
    SVECTOR* v0 = (SVECTOR*)a0;
    SVECTOR* v1 = (SVECTOR*)a1;
    SVECTOR* v2 = (SVECTOR*)a2;
    SVECTOR* p = (SVECTOR*)a3;
    VECTOR n1;
    VECTOR n2;
    VECTOR d;

    d.vx = v1->vx - v0->vx;
    d.vy = v1->vy - v0->vy;
    d.vz = v1->vz - v0->vz;
    VectorNormal(&d, &n1);
    d.vx = v2->vx - v0->vx;
    d.vy = v2->vy - v0->vy;
    d.vz = v2->vz - v0->vz;
    VectorNormal(&d, &n2);
    OuterProduct12(&n1, &n2, normal);
    if (normal->vy == 0) {
        p->vy = 0;
        return;
    }
    p->vy = v0->vy + (-((p->vx - v0->vx) * normal->vx) - (p->vz - v0->vz) * normal->vz) / normal->vy;
}
#endif
#ifndef XENO_PC_PORT
#endif
#ifndef XENO_PC_PORT
/* Depth-limited search from triangle `tri` through its three neighbours
 * (+6/+8/+A): the first triangle not yet visited this pass (+0xD != the pass
 * counter) for which func_800A5A48 returns -1; -1 when none is found. */
s32 func_800A5D54(s32 arg0, s32 tri, s32 depth) {
    s32 r;

    if (tri < 0) {
        return -1;
    }
    if (*(u8*)(tri * 14 + D_800D39CC + 0xD) != D_800D2F64) {
        s16* t;

        *(u8*)(tri * 14 + D_800D39CC + 0xD) = D_800D2F64;
        t = (s16*)(tri * 14 + D_800D39CC);
        if (func_800A5A48(t[0] * 8 + D_800D3344, t[1] * 8 + D_800D3344, t[2] * 8 + D_800D3344, arg0) == -1) {
            return tri;
        }
    }
    if (depth <= 0) {
        return -1;
    }
    if ((r = func_800A5D54(arg0, ((s16*)(tri * 14 + D_800D39CC))[3], depth - 1)) < 0
        && (r = func_800A5D54(arg0, ((s16*)(tri * 14 + D_800D39CC))[4], depth - 1)) < 0
        && (r = func_800A5D54(arg0, ((s16*)(tri * 14 + D_800D39CC))[5], depth - 1)) < 0) {
        r = -1;
    }
    return r;
}
#endif
#ifndef XENO_PC_PORT
#endif


#ifndef XENO_PC_PORT
extern u32 D_800D2D40;
#endif
#ifndef XENO_PC_PORT
extern u32 D_800D2D48;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C3BCC[];
#endif
extern u16 D_8005A3A0[];
#ifndef XENO_PC_PORT
extern u8 D_800D2E62[];
#endif
#ifndef XENO_PC_PORT
extern u16 D_800D39E0;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800D3278;
#endif
extern void LoadImage(void* pRect, void* pData);
extern void DrawSync(s32 mode);


/* func_800A5E9C.s */
void func_800A5E9C(u32 a, u32 b) {
    D_800D2D40 = a;
    D_800D2D48 = b;
}
