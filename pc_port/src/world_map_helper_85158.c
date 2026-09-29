/*
 * World-map node-plane prober 0x80085158.
 *
 * Register-width-faithful transcription of retail world_map.bin
 * [0x80085158, 0x80085418).  See world_map_helper_85158.h.
 *
 * The libgte residents are canonical externs (PsyCross at the
 * production link); certificate builds provide recording versions.
 * Guest pointers are translated through the accepted PSX_ADDR model
 * (scratchpad included), matching wm_80085760 / wm_80095324.
 *
 * Retail sequencing preserved exactly:
 *   1. coef_out[0] = (pos.X sra 12) - rec[8]
 *      coef_out[8] = rec[0x10] - (pos.Z sra 12)      (asymmetric)
 *   2. 8-word MATRIX copy rec[0x20..0x3F] -> scratch 0xF0, then
 *      t[2](0x10C)=0, t[0](0x104)=0, scale vec (0x800,0x800,0x800)
 *      stored to 0x18/0x14/0x10 in that order, t[1](0x108)=rec[0xC]
 *   3. ScaleMatrix(0xF0, 0x10); SetRotMatrix(0xF0); SetTransMatrix(0xF0)
 *   4. RotTrans(vtx[lh node+0], 0x00, &flag)
 *      RotTrans(vtx[lh node+2], 0x10, &flag)   (overwrites scale vec)
 *      RotTrans(vtx[lh node+4], 0x20, &flag)
 *   5. In-place edges: 0x10 -= 0x00 (x,y,z), 0x20 -= 0x00 (x,y,z),
 *      exact retail interleaving preserved
 *   6. OuterProduct0(0x20, 0x10, 0x20)   -- out aliases input 0
 *   7. [0x20/0x24/0x28] sra 2
 *   8. VectorNormal(0x20, normal_out)
 *   9. return wm_800935DC(coef_out, 0x00, normal_out)
 *
 * Node addressing: nodes = rec[0x44]; node = nodes + 8 + 14*node_id
 * (node_id lhu zero-extended); verts = *(nodes + 4); vertex index is
 * lh SIGN-extended s16, scaled by 8.
 */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_plane_solver.h"
#include "world_map_helper_85158.h"

typedef struct {
    s32 vx;
    s32 vy;
    s32 vz;
    s32 pad;
} Wm85158Vector;

typedef struct {
    s16 m[3][3];
    s16 pad;
    s32 t[3];
} Wm85158Matrix;

typedef struct {
    s16 vx;
    s16 vy;
    s16 vz;
    s16 pad;
} Wm85158Svector;

extern Wm85158Matrix *ScaleMatrix(Wm85158Matrix *m, Wm85158Vector *v);
extern void SetRotMatrix(Wm85158Matrix *m);
extern void SetTransMatrix(Wm85158Matrix *m);
extern void RotTrans(Wm85158Svector *v0, Wm85158Vector *v1, long *flag);
extern void OuterProduct0(Wm85158Vector *v0, Wm85158Vector *v1,
                          Wm85158Vector *v2);
extern long VectorNormal(Wm85158Vector *v0, Wm85158Vector *v1);

#if defined(WM_85158_TEST_TRACE)
extern void wm_85158_test_store(u32 address, u32 value);
#define WM_85158_TRACE_STORE(a, v) wm_85158_test_store((a), (v))
#else
#define WM_85158_TRACE_STORE(a, v) ((void)0)
#endif

/* Retail scratch layout. */
#define WM_85158_V0      0x1F800000u /* RotTrans out vtx0 / 935DC base pt */
#define WM_85158_V1      0x1F800010u /* scale vec, then RotTrans out vtx1 */
#define WM_85158_V2      0x1F800020u /* RotTrans out vtx2, then normal    */
#define WM_85158_MTX     0x1F8000F0u /* MATRIX workspace                  */

/* Region record table (84-byte stride). */
#define WM_85158_REC_TABLE_PTR 0x8009C620u

static void *wm_85158_guest(u32 address)
{
    return PSX_ADDR(address);
}

static u32 wm_85158_load_u32(u32 addr)
{
    u32 v;

    memcpy(&v, wm_85158_guest(addr), sizeof(v));
    return v;
}

static void wm_85158_store_u32(u32 addr, u32 v)
{
    memcpy(wm_85158_guest(addr), &v, sizeof(v));
    WM_85158_TRACE_STORE(addr, v);
}

static s16 wm_85158_load_s16(u32 addr) __attribute__((unused));
static s16 wm_85158_load_s16(u32 addr)
{
    s16 v;

    memcpy(&v, wm_85158_guest(addr), sizeof(v));
    return v;
}

static u32 wm_85158_as_u32(s32 v) __attribute__((unused));
static u32 wm_85158_as_u32(s32 v)
{
    u32 b;

    memcpy(&b, &v, sizeof(b));
    return b;
}

/* Exact MIPS SRA of a 32-bit register. */
static u32 wm_85158_sra(u32 bits, u32 amount)
{
    u32 value = bits >> amount;

    if ((bits & 0x80000000u) != 0u)
        value |= UINT32_MAX << (32u - amount);
    return value;
}

u32 wm_80085158(u32 pos_vec, u32 coef_out, u32 normal_out, u32 attr,
                u32 node_id)
{
    u32 rec;
    u32 nodes;
    u32 node;
    u32 verts;
    long rt_flag = 0;

    /* andi $a3, 0xFFFF then rec = *(0x8009C620) + 84*attr
     * (5x -> 21x -> 84x address chain). */
#if defined(WM_85158_MUTANT_MISSING_ATTR_MASK)
    /* keep attr */
#else
    attr &= 0xFFFFu;
#endif
#if defined(WM_85158_MUTANT_WRONG_ATTR_STRIDE)
    rec = wm_85158_load_u32(WM_85158_REC_TABLE_PTR) + attr * 80u;
#else
    rec = wm_85158_load_u32(WM_85158_REC_TABLE_PTR) + attr * 84u;
#endif

    /* coef_out[0] = (pos.X sra 12) - rec[8]  (SUBU wrap) */
#if defined(WM_85158_MUTANT_WRONG_SRA)
    wm_85158_store_u32(coef_out + 0u,
                       wm_85158_sra(wm_85158_load_u32(pos_vec + 0u), 11u) -
                           wm_85158_load_u32(rec + 8u));
#else
    wm_85158_store_u32(coef_out + 0u,
                       wm_85158_sra(wm_85158_load_u32(pos_vec + 0u), 12u) -
                           wm_85158_load_u32(rec + 8u));
#endif
    /* coef_out[8] = rec[0x10] - (pos.Z sra 12)  (asymmetric direction) */
#if defined(WM_85158_MUTANT_WRONG_XZ_ASYMMETRY)
    wm_85158_store_u32(coef_out + 8u,
                       wm_85158_sra(wm_85158_load_u32(pos_vec + 8u), 12u) -
                           wm_85158_load_u32(rec + 0x10u));
#else
    wm_85158_store_u32(coef_out + 8u,
                       wm_85158_load_u32(rec + 0x10u) -
                           wm_85158_sra(wm_85158_load_u32(pos_vec + 8u), 12u));
#endif

    /* lhu 0x48($sp): node id is a zero-extended halfword. */
#if defined(WM_85158_MUTANT_NODE_ID_SIGN)
    node_id = (u32)(s32)(s16)(u16)node_id;
#else
    node_id = (u32)(u16)node_id;
#endif

    /* 8-word MATRIX copy rec[0x20..0x3F] -> 0xF0 (retail store order),
     * then trans overwrites. */
    {
        u32 i;

        for (i = 0u; i < 8u; i++)
            wm_85158_store_u32(WM_85158_MTX + 4u * i,
                               wm_85158_load_u32(rec + 0x20u + 4u * i));
    }
    wm_85158_store_u32(WM_85158_MTX + 0x1Cu, 0u);          /* t[2] */
#if defined(WM_85158_MUTANT_WRONG_TRANS_SLOT)
    wm_85158_store_u32(WM_85158_MTX + 0x18u, 0u);
    wm_85158_store_u32(WM_85158_V1 + 8u, 0x800u);
    wm_85158_store_u32(WM_85158_V1 + 4u, 0x800u);
    wm_85158_store_u32(WM_85158_V1 + 0u, 0x800u);
    wm_85158_store_u32(WM_85158_MTX + 0x14u, wm_85158_load_u32(rec + 0xCu));
#else
    wm_85158_store_u32(WM_85158_MTX + 0x14u, 0u);          /* t[0] */
    /* scale vector (0x800, 0x800, 0x800), retail order +8, +4, +0 */
#if defined(WM_85158_MUTANT_WRONG_SCALE_VALUE)
    wm_85158_store_u32(WM_85158_V1 + 8u, 0x1000u);
    wm_85158_store_u32(WM_85158_V1 + 4u, 0x1000u);
    wm_85158_store_u32(WM_85158_V1 + 0u, 0x1000u);
#else
    wm_85158_store_u32(WM_85158_V1 + 8u, 0x800u);
    wm_85158_store_u32(WM_85158_V1 + 4u, 0x800u);
    wm_85158_store_u32(WM_85158_V1 + 0u, 0x800u);
#endif
    wm_85158_store_u32(WM_85158_MTX + 0x18u,               /* t[1] */
                       wm_85158_load_u32(rec + 0xCu));
#endif

    /* GTE setup: ScaleMatrix then SetRotMatrix then SetTransMatrix. */
#if defined(WM_85158_MUTANT_WRONG_CALL_ORDER)
    (void)ScaleMatrix((Wm85158Matrix *)wm_85158_guest(WM_85158_MTX),
                      (Wm85158Vector *)wm_85158_guest(WM_85158_V1));
    SetTransMatrix((Wm85158Matrix *)wm_85158_guest(WM_85158_MTX));
    SetRotMatrix((Wm85158Matrix *)wm_85158_guest(WM_85158_MTX));
#else
    (void)ScaleMatrix((Wm85158Matrix *)wm_85158_guest(WM_85158_MTX),
                      (Wm85158Vector *)wm_85158_guest(WM_85158_V1));
    SetRotMatrix((Wm85158Matrix *)wm_85158_guest(WM_85158_MTX));
    SetTransMatrix((Wm85158Matrix *)wm_85158_guest(WM_85158_MTX));
#endif

    /* Node triangle: nodes = rec[0x44]; node = nodes + 8 + 14*id;
     * verts = *(nodes + 4); vertex index lh SIGN-extended, *8. */
    nodes = wm_85158_load_u32(rec + 0x44u);
    node = nodes + 8u + 14u * node_id;
    verts = wm_85158_load_u32(nodes + 4u);
    {
        static const u32 out_slots[3] = {
            WM_85158_V0, WM_85158_V1, WM_85158_V2
        };
        u32 i;

        for (i = 0u; i < 3u; i++) {
#if defined(WM_85158_MUTANT_VTX_INDEX_UNSIGNED)
            u32 idx;
            u16 raw;

            memcpy(&raw, wm_85158_guest(node + 2u * i), sizeof(raw));
            idx = (u32)raw;
            RotTrans((Wm85158Svector *)wm_85158_guest(verts + (idx << 3)),
                     (Wm85158Vector *)wm_85158_guest(out_slots[i]),
                     &rt_flag);
#else
            s32 idx = (s32)wm_85158_load_s16(node + 2u * i);

            RotTrans((Wm85158Svector *)wm_85158_guest(
                         verts + (wm_85158_as_u32(idx) << 3)),
                     (Wm85158Vector *)wm_85158_guest(out_slots[i]),
                     &rt_flag);
#endif
        }
    }

    /* In-place edge vectors, exact retail interleaving:
     *   v0=[0x10]; t0=[0x00]; v1=[0x14]; t1=[0x04];
     *   [0x10]=v0-t0; v0=[0x18]; a3=[0x08]; [0x14]=v1-t1;
     *   v1=[0x20]; [0x18]=v0-a3; [0x20]=v1-t0;
     *   v0=[0x24]; v1=[0x28]; [0x24]=v0-t1; [0x28]=v1-a3; */
    {
        u32 v0 = wm_85158_load_u32(WM_85158_V1 + 0u);
        u32 t0 = wm_85158_load_u32(WM_85158_V0 + 0u);
        u32 v1 = wm_85158_load_u32(WM_85158_V1 + 4u);
        u32 t1 = wm_85158_load_u32(WM_85158_V0 + 4u);
        u32 a3;

        wm_85158_store_u32(WM_85158_V1 + 0u, v0 - t0);
        v0 = wm_85158_load_u32(WM_85158_V1 + 8u);
        a3 = wm_85158_load_u32(WM_85158_V0 + 8u);
        wm_85158_store_u32(WM_85158_V1 + 4u, v1 - t1);
        v1 = wm_85158_load_u32(WM_85158_V2 + 0u);
        wm_85158_store_u32(WM_85158_V1 + 8u, v0 - a3);
        wm_85158_store_u32(WM_85158_V2 + 0u, v1 - t0);
        v0 = wm_85158_load_u32(WM_85158_V2 + 4u);
        v1 = wm_85158_load_u32(WM_85158_V2 + 8u);
        wm_85158_store_u32(WM_85158_V2 + 4u, v0 - t1);
        wm_85158_store_u32(WM_85158_V2 + 8u, v1 - a3);
    }

    /* OuterProduct0(V2-V0, V1-V0, out) with out ALIASING input 0. */
#if defined(WM_85158_MUTANT_SWAPPED_OP0_ARGS)
    OuterProduct0((Wm85158Vector *)wm_85158_guest(WM_85158_V1),
                  (Wm85158Vector *)wm_85158_guest(WM_85158_V2),
                  (Wm85158Vector *)wm_85158_guest(WM_85158_V2));
#else
    OuterProduct0((Wm85158Vector *)wm_85158_guest(WM_85158_V2),
                  (Wm85158Vector *)wm_85158_guest(WM_85158_V1),
                  (Wm85158Vector *)wm_85158_guest(WM_85158_V2));
#endif

    /* sra 2 on the cross product. */
#if !defined(WM_85158_MUTANT_MISSING_SRA2)
    wm_85158_store_u32(WM_85158_V2 + 0u,
                       wm_85158_sra(wm_85158_load_u32(WM_85158_V2 + 0u), 2u));
    wm_85158_store_u32(WM_85158_V2 + 4u,
                       wm_85158_sra(wm_85158_load_u32(WM_85158_V2 + 4u), 2u));
    wm_85158_store_u32(WM_85158_V2 + 8u,
                       wm_85158_sra(wm_85158_load_u32(WM_85158_V2 + 8u), 2u));
#endif

    (void)VectorNormal((Wm85158Vector *)wm_85158_guest(WM_85158_V2),
                       (Wm85158Vector *)wm_85158_guest(normal_out));

#if defined(WM_85158_MUTANT_WRONG_935DC_ARGS)
    return wm_800935DC(coef_out, normal_out, WM_85158_V0);
#else
    return wm_800935DC(coef_out, WM_85158_V0, normal_out);
#endif
}
