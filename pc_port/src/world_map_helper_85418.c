/*
 * World-map node-triangle plane-straddle filter 0x80085418.
 *
 * Register-width-faithful transcription of retail world_map.bin
 * [0x80085418, 0x80085760).  See world_map_helper_85418.h.
 *
 * Scratch layout (guest addresses, via the accepted PSX_ADDR model):
 *   0x1F800000/0x10/0x20  three RotTrans outputs (then in-place edges)
 *   0x1F800010            also the (0x800,0x800,0x800) scale VECTOR
 *                         before RotTrans #2 overwrites it (retail)
 *   0x1F800030            unit face normal (VectorNormal out)
 *   0x1F800040            ApplyMatrixLV out (dotA/+0, dotB/+4, dead/+8)
 *   0x1F8000F0            MATRIX copy (m rows +0x00..0x11, t +0x14..)
 *   0x1F800110            packed s16 probe rows (row0 +0, row1 +6;
 *                         row2 +0xC is uninitialized scratch, its dot
 *                         is computed by the GTE but never read)
 *
 * All subtraction is 32-bit wrap (subu); probe stores are sh
 * (s16 truncation of the full s32); position components are sra 12;
 * normal components sra 2.  The libgte residents are canonical
 * (PsyCross at the production link); certificate builds provide
 * recording versions that model the SetRotMatrix/SetTransMatrix ->
 * RotTrans state hand-off.
 */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_85418.h"

typedef struct {
    s16 m[3][3];
    s16 pad;
    s32 t[3];
} Wm85418Matrix;

typedef struct {
    s32 vx;
    s32 vy;
    s32 vz;
    s32 pad;
} Wm85418Vector;

typedef struct {
    s16 vx;
    s16 vy;
    s16 vz;
    s16 pad;
} Wm85418Svector;

extern Wm85418Matrix *ScaleMatrix(Wm85418Matrix *m, Wm85418Vector *v);
extern void SetRotMatrix(Wm85418Matrix *m);
extern void SetTransMatrix(Wm85418Matrix *m);
extern void RotTrans(Wm85418Svector *v0, Wm85418Vector *v1, long *flag);
extern void OuterProduct0(Wm85418Vector *v0, Wm85418Vector *v1,
                          Wm85418Vector *v2);
extern long VectorNormal(Wm85418Vector *v0, Wm85418Vector *v1);
extern void ApplyMatrixLV(Wm85418Matrix *m, Wm85418Vector *v0,
                          Wm85418Vector *v1);

#if defined(WM_85418_TEST_TRACE)
extern void wm_85418_test_load(u32 address, u32 width, u32 value);
extern void wm_85418_test_store(u32 address, u32 width, u32 value);
#define WM_85418_TRACE_LOAD(a, w, v)  wm_85418_test_load((a), (w), (v))
#define WM_85418_TRACE_STORE(a, w, v) wm_85418_test_store((a), (w), (v))
#else
#define WM_85418_TRACE_LOAD(a, w, v)  ((void)0)
#define WM_85418_TRACE_STORE(a, w, v) ((void)0)
#endif

#define WM_85418_REC_TABLE_PTR 0x8009C620u
#define WM_85418_SC_V0         0x1F800000u
#define WM_85418_SC_V1         0x1F800010u
#define WM_85418_SC_V2         0x1F800020u
#define WM_85418_SC_NORMAL     0x1F800030u
#define WM_85418_SC_DOTS       0x1F800040u
#define WM_85418_SC_MATRIX     0x1F8000F0u
#define WM_85418_SC_PROBES     0x1F800110u

static void *wm_85418_guest(u32 address)
{
    return PSX_ADDR(address);
}

static u32 wm_85418_load_u32(u32 addr)
{
    u32 v;

    memcpy(&v, wm_85418_guest(addr), sizeof(v));
    WM_85418_TRACE_LOAD(addr, 4u, v);
    return v;
}

static s32 wm_85418_load_s16(u32 addr)
{
    s16 v;

    memcpy(&v, wm_85418_guest(addr), sizeof(v));
    WM_85418_TRACE_LOAD(addr, 2u, (u32)(u16)v);
    return v;
}

static void wm_85418_store_u32(u32 addr, u32 v)
{
    memcpy(wm_85418_guest(addr), &v, sizeof(v));
    WM_85418_TRACE_STORE(addr, 4u, v);
}

static void wm_85418_store_u16(u32 addr, u32 v)
{
    u16 h = (u16)v;

    memcpy(wm_85418_guest(addr), &h, sizeof(h));
    WM_85418_TRACE_STORE(addr, 2u, (u32)h);
}

static s32 wm_85418_as_s32(u32 b)
{
    s32 v;

    memcpy(&v, &b, sizeof(v));
    return v;
}

/* Exact MIPS SRA of a 32-bit register. */
static u32 wm_85418_sra(u32 bits, u32 amount)
{
    u32 value = bits >> amount;

    if ((bits & 0x80000000u) != 0u)
        value |= UINT32_MAX << (32u - amount);
    return value;
}

s32 wm_80085418(u32 pos_vec, s32 y_offset, u32 attr, u32 node_id)
{
    u32 rec;
    u32 node_arr;
    u32 node;
    u32 vtx_base;
    u32 w0, w1, w2;
    u32 t0, t1, a3;

    /* rec = *(0x8009C620) + 84 * (attr & 0xFFFF). */
#if defined(WM_85418_MUTANT_MISSING_ATTR_MASK)
    rec = wm_85418_load_u32(WM_85418_REC_TABLE_PTR) + 84u * attr;
#else
    rec = wm_85418_load_u32(WM_85418_REC_TABLE_PTR) + 84u * (attr & 0xFFFFu);
#endif

    /* MATRIX copy: rec+0x20..0x3F -> scratch 0xF0 (retail triple order). */
#if defined(WM_85418_MUTANT_WRONG_MATRIX_SRC_OFFSET)
    w0 = wm_85418_load_u32(rec + 0x1Cu);
#else
    w0 = wm_85418_load_u32(rec + 0x20u);
#endif
    w1 = wm_85418_load_u32(rec + 0x24u);
    w2 = wm_85418_load_u32(rec + 0x28u);
    wm_85418_store_u32(WM_85418_SC_MATRIX + 0x00u, w0);
    wm_85418_store_u32(WM_85418_SC_MATRIX + 0x04u, w1);
    wm_85418_store_u32(WM_85418_SC_MATRIX + 0x08u, w2);
    w0 = wm_85418_load_u32(rec + 0x2Cu);
    w1 = wm_85418_load_u32(rec + 0x30u);
    w2 = wm_85418_load_u32(rec + 0x34u);
    wm_85418_store_u32(WM_85418_SC_MATRIX + 0x0Cu, w0);
    wm_85418_store_u32(WM_85418_SC_MATRIX + 0x10u, w1);
    wm_85418_store_u32(WM_85418_SC_MATRIX + 0x14u, w2);
    w0 = wm_85418_load_u32(rec + 0x38u);
    w1 = wm_85418_load_u32(rec + 0x3Cu);
    wm_85418_store_u32(WM_85418_SC_MATRIX + 0x18u, w0);
    wm_85418_store_u32(WM_85418_SC_MATRIX + 0x1Cu, w1);

    /* t[2] = 0; t[0] = 0; scale = (0x800,0x800,0x800) stored vz,vy,vx;
     * t[1] = rec[0xC].  Retail store order preserved. */
    wm_85418_store_u32(WM_85418_SC_MATRIX + 0x1Cu, 0u); /* t[2] @0x10C */
    wm_85418_store_u32(WM_85418_SC_MATRIX + 0x14u, 0u); /* t[0] @0x104 */
    w1 = wm_85418_load_u32(rec + 0x0Cu);
#if defined(WM_85418_MUTANT_WRONG_SCALE_VALUE)
    w0 = 0x1000u;
#else
    w0 = 0x800u;
#endif
    wm_85418_store_u32(WM_85418_SC_V1 + 8u, w0);  /* scale.vz @0x18 */
    wm_85418_store_u32(WM_85418_SC_V1 + 4u, w0);  /* scale.vy @0x14 */
    wm_85418_store_u32(WM_85418_SC_V1 + 0u, w0);  /* scale.vx @0x10 */
#if defined(WM_85418_MUTANT_WRONG_TRANS_SLOT)
    wm_85418_store_u32(WM_85418_SC_MATRIX + 0x14u, w1); /* t[0] */
#else
    wm_85418_store_u32(WM_85418_SC_MATRIX + 0x18u, w1); /* t[1] @0x108 */
#endif

    (void)ScaleMatrix((Wm85418Matrix *)wm_85418_guest(WM_85418_SC_MATRIX),
                      (Wm85418Vector *)wm_85418_guest(WM_85418_SC_V1));
#if defined(WM_85418_MUTANT_SWAPPED_GTE_ORDER)
    SetTransMatrix((Wm85418Matrix *)wm_85418_guest(WM_85418_SC_MATRIX));
    SetRotMatrix((Wm85418Matrix *)wm_85418_guest(WM_85418_SC_MATRIX));
#else
    SetRotMatrix((Wm85418Matrix *)wm_85418_guest(WM_85418_SC_MATRIX));
    SetTransMatrix((Wm85418Matrix *)wm_85418_guest(WM_85418_SC_MATRIX));
#endif

    /* node = rec[0x44] + 8 + 14 * (node_id & 0xFFFF); vertex SVECTOR
     * array base = *(rec[0x44] + 4); index halfwords at node +0/+2/+4
     * are SIGNED (lh). */
    node_arr = wm_85418_load_u32(rec + 0x44u);
#if defined(WM_85418_MUTANT_WRONG_NODE_STRIDE)
    node = node_arr + 8u + 12u * (node_id & 0xFFFFu);
#else
    node = node_arr + 8u + 14u * (node_id & 0xFFFFu);
#endif
    vtx_base = wm_85418_load_u32(node_arr + 4u);

    {
        long flag;
        u32 i;
        static const u32 out_addr[3] = {
            WM_85418_SC_V0, WM_85418_SC_V1, WM_85418_SC_V2
        };

        for (i = 0; i < 3u; i++) {
            s32 idx;
            u32 vtx;

#if defined(WM_85418_MUTANT_WRONG_VTX_INDEX_WIDTH)
            idx = (s32)(u16)wm_85418_load_s16(node + 2u * i);
#else
            idx = wm_85418_load_s16(node + 2u * i);
#endif
            vtx = vtx_base + ((u32)idx << 3);
            RotTrans((Wm85418Svector *)wm_85418_guest(vtx),
                     (Wm85418Vector *)wm_85418_guest(out_addr[i]), &flag);
        }
    }

    /* In-place edge build with the exact retail interleave:
     * V1 -= V0 (x,y,z), V2 -= V0 (x,y,z). */
    w0 = wm_85418_load_u32(WM_85418_SC_V1 + 0u);        /* V1.x */
    t0 = wm_85418_load_u32(WM_85418_SC_V0 + 0u);        /* V0.x */
    w1 = wm_85418_load_u32(WM_85418_SC_V1 + 4u);        /* V1.y */
    t1 = wm_85418_load_u32(WM_85418_SC_V0 + 4u);        /* V0.y */
    wm_85418_store_u32(WM_85418_SC_V1 + 0u, w0 - t0);
    w0 = wm_85418_load_u32(WM_85418_SC_V1 + 8u);        /* V1.z */
    a3 = wm_85418_load_u32(WM_85418_SC_V0 + 8u);        /* V0.z */
    wm_85418_store_u32(WM_85418_SC_V1 + 4u, w1 - t1);
    w1 = wm_85418_load_u32(WM_85418_SC_V2 + 0u);        /* V2.x */
    wm_85418_store_u32(WM_85418_SC_V1 + 8u, w0 - a3);
    wm_85418_store_u32(WM_85418_SC_V2 + 0u, w1 - t0);
    w0 = wm_85418_load_u32(WM_85418_SC_V2 + 4u);        /* V2.y */
    w1 = wm_85418_load_u32(WM_85418_SC_V2 + 8u);        /* V2.z */
    wm_85418_store_u32(WM_85418_SC_V2 + 4u, w0 - t1);
    wm_85418_store_u32(WM_85418_SC_V2 + 8u, w1 - a3);

#if defined(WM_85418_MUTANT_WRONG_OP0_ARGS)
    OuterProduct0((Wm85418Vector *)wm_85418_guest(WM_85418_SC_V1),
                  (Wm85418Vector *)wm_85418_guest(WM_85418_SC_V2),
                  (Wm85418Vector *)wm_85418_guest(WM_85418_SC_V2));
#else
    OuterProduct0((Wm85418Vector *)wm_85418_guest(WM_85418_SC_V2),
                  (Wm85418Vector *)wm_85418_guest(WM_85418_SC_V1),
                  (Wm85418Vector *)wm_85418_guest(WM_85418_SC_V2));
#endif

    /* Normal components sra 2 (x first, then y and z together). */
#if defined(WM_85418_MUTANT_WRONG_SRA_NORMAL)
#define WM_85418_NORM_SHIFT 1u
#else
#define WM_85418_NORM_SHIFT 2u
#endif
    w0 = wm_85418_load_u32(WM_85418_SC_V2 + 0u);
    wm_85418_store_u32(WM_85418_SC_V2 + 0u,
                       wm_85418_sra(w0, WM_85418_NORM_SHIFT));
    w0 = wm_85418_load_u32(WM_85418_SC_V2 + 4u);
    w1 = wm_85418_load_u32(WM_85418_SC_V2 + 8u);
    wm_85418_store_u32(WM_85418_SC_V2 + 4u,
                       wm_85418_sra(w0, WM_85418_NORM_SHIFT));
    wm_85418_store_u32(WM_85418_SC_V2 + 8u,
                       wm_85418_sra(w1, WM_85418_NORM_SHIFT));
    (void)VectorNormal((Wm85418Vector *)wm_85418_guest(WM_85418_SC_V2),
                       (Wm85418Vector *)wm_85418_guest(WM_85418_SC_NORMAL));

    /* Packed s16 probe rows.  Full s32 arithmetic; sh truncation at the
     * stores.  Retail load/store interleave preserved. */
    {
        u32 px = wm_85418_load_u32(pos_vec + 0u);
        u32 rx = wm_85418_load_u32(rec + 8u);
        u32 v0x = wm_85418_load_u32(WM_85418_SC_V0 + 0u);
        u32 dx, dy1, dy2, dz;
        u32 py, v0y, rz, pzraw, v0z;

        dx = wm_85418_sra(px, 12u) - rx - v0x;
        wm_85418_store_u16(WM_85418_SC_PROBES + 0x0u, dx);
#if !defined(WM_85418_MUTANT_SKIPPED_PROBE_STORE)
        wm_85418_store_u16(WM_85418_SC_PROBES + 0x6u, dx);
#endif
        py = wm_85418_load_u32(pos_vec + 4u);
        v0y = wm_85418_load_u32(WM_85418_SC_V0 + 4u);
        dy1 = wm_85418_sra(py, 12u) - v0y;
        wm_85418_store_u16(WM_85418_SC_PROBES + 0x2u, dy1);
        rz = wm_85418_load_u32(rec + 0x10u);
        pzraw = wm_85418_load_u32(pos_vec + 8u);
#if defined(WM_85418_MUTANT_Y_OFFSET_AS_POINTER)
        dy2 = dy1 - wm_85418_load_u32((u32)y_offset);
#else
        dy2 = dy1 - (u32)y_offset;
#endif
        wm_85418_store_u16(WM_85418_SC_PROBES + 0x8u, dy2);
        v0z = wm_85418_load_u32(WM_85418_SC_V0 + 8u);
#if defined(WM_85418_MUTANT_WRONG_Z_SIGN)
        dz = wm_85418_sra(pzraw, 12u) - rz - v0z;
#else
        dz = rz - wm_85418_sra(pzraw, 12u) - v0z;
#endif
        wm_85418_store_u16(WM_85418_SC_PROBES + 0x4u, dz);
        wm_85418_store_u16(WM_85418_SC_PROBES + 0xAu, dz);
    }

    ApplyMatrixLV((Wm85418Matrix *)wm_85418_guest(WM_85418_SC_PROBES),
                  (Wm85418Vector *)wm_85418_guest(WM_85418_SC_NORMAL),
                  (Wm85418Vector *)wm_85418_guest(WM_85418_SC_DOTS));

    {
        u32 dotA = wm_85418_load_u32(WM_85418_SC_DOTS + 0u);
        u32 dotB = wm_85418_load_u32(WM_85418_SC_DOTS + 4u);

#if defined(WM_85418_MUTANT_BOOLEAN_RETURN)
        return (wm_85418_as_s32(dotA ^ dotB) < 0) ? 1 : 0;
#else
        return wm_85418_as_s32(wm_85418_sra(dotA ^ dotB, 31u));
#endif
    }
}
