/*
 * World-map nav-mesh point-in-triangle probe 0x80084DB8.
 *
 * Register-width-faithful transcription of retail world_map.bin
 * [0x80084DB8, 0x80085158).  See world_map_helper_84db8.h.
 *
 * Retail flow:
 *   rec = *(0x8009C620) + 84 * region_idx
 *   dx  = (pos.X sra 12) - rec[8]         -> scratch 0x40 (stored first)
 *   dz  = rec[0x10] - (pos.Z sra 12)      -> scratch 0x48 (OPPOSITE order)
 *   reject unless |dx| < 0x800 && |dz| < 0x800  (negu-wrap abs; INT_MIN
 *   wraps to itself and PASSES the signed slti gate -- faithful)
 *   MATRIX copy rec[0x20..0x3F] -> 0xF0; trans=(0, rec[0xC], 0);
 *   scale (0x800,0x800,0x800) at scratch 0 (stores +8, +4, +0);
 *   ScaleMatrix(0xF0, 0x1F800000); SetRotMatrix; SetTransMatrix
 *   hdr = rec[0x44]; count = hdr[0] (lw, cached in a stack slot --
 *   NOT reloaded); verts = hdr[4]
 *   point = (dz reload << 16) | (lhu dx reload)  -> scratch 0x38
 *   per 14-byte node (hdr+8):
 *     vtx k: idx = lh node[2k] (sign-extended); RotTrans(verts + idx*8,
 *            scratch 0x00/0x10/0x20, &flag)   [inline MVMVA 0x4A480012
 *            + swc2 MAC1..3 + dead cfc2 FLAG in retail -- RotTrans is
 *            the identical opcode + store sequence]
 *     packs: pV0 = (sc[8]<<16)  | lhu sc[0]
 *            pV1 = (sc[0x18]<<16) | (sc[0x10] & 0xFFFF)
 *            pV2 = (sc[0x28]<<16) | (sc[0x20] & 0xFFFF)
 *     edge tests (staged through sc[0x30]/sc[0x34]):
 *            NormalClip(pV0, pV1, point) > 0 -> skip
 *            NormalClip(pV1, pV2, point) > 0 -> skip
 *            NormalClip(pV2, pV0, point) > 0 -> skip   (pV0 re-packed
 *                                                       with lhu again)
 *     accept: sh n -> [0x8009D718 + 4k]; sh node[0xC] -> [0x8009D71A+4k];
 *             result += 2
 *   return result
 */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_84db8.h"

typedef struct {
    s16 vx;
    s16 vy;
    s16 vz;
    s16 pad;
} Wm84db8Svector;

typedef struct {
    s32 vx;
    s32 vy;
    s32 vz;
    s32 pad;
} Wm84db8Vector;

typedef struct {
    s16 m[3][3];
    s16 pad;
    s32 t[3];
} Wm84db8Matrix;

extern Wm84db8Matrix *ScaleMatrix(Wm84db8Matrix *m, Wm84db8Vector *v);
extern void SetRotMatrix(Wm84db8Matrix *m);
extern void SetTransMatrix(Wm84db8Matrix *m);
extern void RotTrans(Wm84db8Svector *v0, Wm84db8Vector *v1, long *flag);
extern long NormalClip(long sxy0, long sxy1, long sxy2);

#if defined(WM_84DB8_TEST_TRACE)
extern void wm_84db8_test_scale_matrix(u32 m_addr, u32 v_addr);
extern void wm_84db8_test_set_rot(u32 m_addr);
extern void wm_84db8_test_set_trans(u32 m_addr);
extern void wm_84db8_test_rot_trans(u32 sv_addr, u32 out_addr);
extern s32 wm_84db8_test_normal_clip(u32 sxy0, u32 sxy1, u32 sxy2);
extern void wm_84db8_test_store(u32 address, u32 value);
#define WM_84DB8_SCALE_MATRIX(m, v)  wm_84db8_test_scale_matrix((m), (v))
#define WM_84DB8_SET_ROT(m)          wm_84db8_test_set_rot(m)
#define WM_84DB8_SET_TRANS(m)        wm_84db8_test_set_trans(m)
#define WM_84DB8_ROT_TRANS(sv, out)  wm_84db8_test_rot_trans((sv), (out))
#define WM_84DB8_NCLIP(p0, p1, p2)   wm_84db8_test_normal_clip((p0), (p1), (p2))
#define WM_84DB8_TRACE_STORE(a, v)   wm_84db8_test_store((a), (v))
#else
#define WM_84DB8_SCALE_MATRIX(m, v) \
    ((void)ScaleMatrix((Wm84db8Matrix *)PSX_ADDR(m), \
                       (Wm84db8Vector *)PSX_ADDR(v)))
#define WM_84DB8_SET_ROT(m)   SetRotMatrix((Wm84db8Matrix *)PSX_ADDR(m))
#define WM_84DB8_SET_TRANS(m) SetTransMatrix((Wm84db8Matrix *)PSX_ADDR(m))
static void wm_84db8_rot_trans(u32 sv_addr, u32 out_addr)
{
    long dead_flag; /* retail: cfc2 FLAG stored to sp[0x10], never read */

    RotTrans((Wm84db8Svector *)PSX_ADDR(sv_addr),
             (Wm84db8Vector *)PSX_ADDR(out_addr), &dead_flag);
}
#define WM_84DB8_ROT_TRANS(sv, out)  wm_84db8_rot_trans((sv), (out))
#define WM_84DB8_NCLIP(p0, p1, p2) \
    ((s32)NormalClip((long)(s32)(p0), (long)(s32)(p1), (long)(s32)(p2)))
#define WM_84DB8_TRACE_STORE(a, v)   ((void)0)
#endif

/* Retail scratch / global layout. */
#define WM_84DB8_SC_V0        0x1F800000u /* MAC out vtx0; also scale vec */
#define WM_84DB8_SC_V1        0x1F800010u
#define WM_84DB8_SC_V2        0x1F800020u
#define WM_84DB8_SC_PACK_A    0x1F800030u
#define WM_84DB8_SC_PACK_B    0x1F800034u
#define WM_84DB8_SC_POINT     0x1F800038u
#define WM_84DB8_SC_DX        0x1F800040u
#define WM_84DB8_SC_DZ        0x1F800048u
#define WM_84DB8_SC_MTX       0x1F8000F0u
#define WM_84DB8_REC_PTR      0x8009C620u
#define WM_84DB8_CAND_ID      0x8009D718u
#define WM_84DB8_CAND_TYPE    0x8009D71Au

static u32 wm_84db8_load_u32(u32 addr)
{
    u32 v;

    memcpy(&v, PSX_ADDR(addr), sizeof(v));
    return v;
}

static u16 wm_84db8_load_u16(u32 addr)
{
    u16 v;

    memcpy(&v, PSX_ADDR(addr), sizeof(v));
    return v;
}

static s16 wm_84db8_load_s16(u32 addr) __attribute__((unused));
static s16 wm_84db8_load_s16(u32 addr)
{
    s16 v;

    memcpy(&v, PSX_ADDR(addr), sizeof(v));
    return v;
}

static void wm_84db8_store_u32(u32 addr, u32 v)
{
    memcpy(PSX_ADDR(addr), &v, sizeof(v));
    WM_84DB8_TRACE_STORE(addr, v);
}

static void wm_84db8_store_u16(u32 addr, u16 v)
{
    memcpy(PSX_ADDR(addr), &v, sizeof(v));
    WM_84DB8_TRACE_STORE(addr, (u32)v);
}

static s32 wm_84db8_as_s32(u32 b)
{
    s32 v;

    memcpy(&v, &b, sizeof(v));
    return v;
}

static u32 wm_84db8_as_u32(s32 v)
{
    u32 b;

    memcpy(&b, &v, sizeof(b));
    return b;
}

/* Exact MIPS SRA of a 32-bit register. */
static u32 wm_84db8_sra(u32 bits, u32 amount)
{
    u32 value = bits >> amount;

    if ((bits & 0x80000000u) != 0u)
        value |= UINT32_MAX << (32u - amount);
    return value;
}

/* bgez/negu absolute value with 32-bit wrap (INT_MIN stays INT_MIN). */
static u32 wm_84db8_abs_wrap(u32 bits)
{
    if (wm_84db8_as_s32(bits) < 0)
        return 0u - bits;
    return bits;
}

s32 wm_80084DB8(u32 pos_vec, s32 region_idx)
{
    u32 rec;
    u32 dx;
    u32 dz;
    u32 reject;
    u32 hdr;
    u32 verts;
    s32 count;
    s32 result;
    s32 n;
    u32 node;
    u32 out_id;
    u32 out_type;

    /* rec = base + 84 * idx (sll/addu chain, 32-bit wrap). */
    rec = wm_84db8_load_u32(WM_84DB8_REC_PTR) +
          wm_84db8_as_u32(region_idx) * 84u;

    /* dx = (pos.X sra 12) - rec[8]; stored BEFORE the gate test. */
#if defined(WM_84DB8_MUTANT_DELTA_DIR)
    dx = wm_84db8_load_u32(rec + 8u) -
         wm_84db8_sra(wm_84db8_load_u32(pos_vec + 0u), 12u);
#else
    dx = wm_84db8_sra(wm_84db8_load_u32(pos_vec + 0u), 12u) -
         wm_84db8_load_u32(rec + 8u);
#endif
    wm_84db8_store_u32(WM_84DB8_SC_DX, dx);
#if defined(WM_84DB8_MUTANT_GATE_OFF_BY_ONE)
    reject = (wm_84db8_as_s32(wm_84db8_abs_wrap(dx)) < 0x801) ? 0u : 1u;
#else
    reject = (wm_84db8_as_s32(wm_84db8_abs_wrap(dx)) < 0x800) ? 0u : 1u;
#endif

    /* dz = rec[0x10] - (pos.Z sra 12): operand order OPPOSITE of dx. */
#if defined(WM_84DB8_MUTANT_DELTA_DIR)
    dz = wm_84db8_sra(wm_84db8_load_u32(pos_vec + 8u), 12u) -
         wm_84db8_load_u32(rec + 0x10u);
#else
    dz = wm_84db8_load_u32(rec + 0x10u) -
         wm_84db8_sra(wm_84db8_load_u32(pos_vec + 8u), 12u);
#endif
    wm_84db8_store_u32(WM_84DB8_SC_DZ, dz);
#if defined(WM_84DB8_MUTANT_GATE_OFF_BY_ONE)
    reject |= (wm_84db8_as_s32(wm_84db8_abs_wrap(dz)) < 0x801) ? 0u : 1u;
#else
    reject |= (wm_84db8_as_s32(wm_84db8_abs_wrap(dz)) < 0x800) ? 0u : 1u;
#endif
    if (reject != 0u)
        return 0;

    /* 8-word MATRIX copy rec[0x20..0x3F] -> 0xF0 (retail order). */
    {
        u32 i;

        for (i = 0u; i < 8u; i++)
#if defined(WM_84DB8_MUTANT_MATRIX_OFFSET)
            wm_84db8_store_u32(WM_84DB8_SC_MTX + 4u * i,
                               wm_84db8_load_u32(rec + 0x24u + 4u * i));
#else
            wm_84db8_store_u32(WM_84DB8_SC_MTX + 4u * i,
                               wm_84db8_load_u32(rec + 0x20u + 4u * i));
#endif
    }
    /* trans.z = 0; trans.x = 0; scale (0x800,0x800,0x800) at scratch 0
     * (stores +8, +4, +0); trans.y = rec[0xC].  Retail store order. */
    wm_84db8_store_u32(WM_84DB8_SC_MTX + 0x1Cu, 0u);
    wm_84db8_store_u32(WM_84DB8_SC_MTX + 0x14u, 0u);
    {
        u32 trans_y = wm_84db8_load_u32(rec + 0xCu);
#if defined(WM_84DB8_MUTANT_SCALE_VALUE)
        u32 sc = 0x1000u;
#else
        u32 sc = 0x800u;
#endif

        wm_84db8_store_u32(WM_84DB8_SC_V0 + 8u, sc);
        wm_84db8_store_u32(WM_84DB8_SC_V0 + 4u, sc);
        wm_84db8_store_u32(WM_84DB8_SC_V0 + 0u, sc);
        wm_84db8_store_u32(WM_84DB8_SC_MTX + 0x18u, trans_y);
    }
    WM_84DB8_SCALE_MATRIX(WM_84DB8_SC_MTX, WM_84DB8_SC_V0);
    WM_84DB8_SET_ROT(WM_84DB8_SC_MTX);
    WM_84DB8_SET_TRANS(WM_84DB8_SC_MTX);

    /* Mesh header; count is CACHED (retail spills it to the stack). */
    hdr = wm_84db8_load_u32(rec + 0x44u);
    {
        u32 point =
#if defined(WM_84DB8_MUTANT_PACK_ORDER)
            (wm_84db8_load_u32(WM_84DB8_SC_DX) << 16) |
            (u32)wm_84db8_load_u16(WM_84DB8_SC_DZ);
#else
            (wm_84db8_load_u32(WM_84DB8_SC_DZ) << 16) |
            (u32)wm_84db8_load_u16(WM_84DB8_SC_DX);
#endif

        count = wm_84db8_as_s32(wm_84db8_load_u32(hdr + 0u));
        verts = wm_84db8_load_u32(hdr + 4u);
        wm_84db8_store_u32(WM_84DB8_SC_POINT, point);
    }
    result = 0;
    if (count <= 0)
        return result;

    node = hdr + 8u;
    out_id = WM_84DB8_CAND_ID;
    out_type = WM_84DB8_CAND_TYPE;
    n = 0;
    do {
        u32 point;
        u32 pack_a;
        u32 pack_b;
        u32 k;

        /* Transform the triangle's three vertices. */
        for (k = 0u; k < 3u; k++) {
            static const u32 out_slots[3] = {
                WM_84DB8_SC_V0, WM_84DB8_SC_V1, WM_84DB8_SC_V2
            };
#if defined(WM_84DB8_MUTANT_IDX_UNSIGNED)
            u32 idx = (u32)wm_84db8_load_u16(node + 2u * k);
#else
            u32 idx =
                wm_84db8_as_u32((s32)wm_84db8_load_s16(node + 2u * k));
#endif

            WM_84DB8_ROT_TRANS(verts + (idx << 3), out_slots[k]);
        }

        point = wm_84db8_load_u32(WM_84DB8_SC_POINT);

        /* Edge 1: NormalClip(pack(V0), pack(V1), point). */
        pack_a = (wm_84db8_load_u32(WM_84DB8_SC_V0 + 8u) << 16) |
                 (u32)wm_84db8_load_u16(WM_84DB8_SC_V0 + 0u);
        wm_84db8_store_u32(WM_84DB8_SC_PACK_A, pack_a);
        pack_b = (wm_84db8_load_u32(WM_84DB8_SC_V1 + 8u) << 16) |
                 (wm_84db8_load_u32(WM_84DB8_SC_V1 + 0u) & 0xFFFFu);
        wm_84db8_store_u32(WM_84DB8_SC_PACK_B, pack_b);
#if defined(WM_84DB8_MUTANT_NCLIP_ARG_ORDER)
        if (WM_84DB8_NCLIP(pack_b, pack_a, point) > 0)
            goto next_node;
#elif defined(WM_84DB8_MUTANT_ACCEPT_GEZ)
        if (WM_84DB8_NCLIP(pack_a, pack_b, point) >= 0)
            goto next_node;
#else
        if (WM_84DB8_NCLIP(pack_a, pack_b, point) > 0)
            goto next_node;
#endif

        /* Edge 2: NormalClip(pack(V1), pack(V2), point). */
        pack_a = (wm_84db8_load_u32(WM_84DB8_SC_V1 + 8u) << 16) |
                 (wm_84db8_load_u32(WM_84DB8_SC_V1 + 0u) & 0xFFFFu);
        wm_84db8_store_u32(WM_84DB8_SC_PACK_A, pack_a);
        pack_b = (wm_84db8_load_u32(WM_84DB8_SC_V2 + 8u) << 16) |
                 (wm_84db8_load_u32(WM_84DB8_SC_V2 + 0u) & 0xFFFFu);
        wm_84db8_store_u32(WM_84DB8_SC_PACK_B, pack_b);
#if defined(WM_84DB8_MUTANT_ACCEPT_GEZ)
        if (WM_84DB8_NCLIP(pack_a, pack_b, point) >= 0)
            goto next_node;
#else
        if (WM_84DB8_NCLIP(pack_a, pack_b, point) > 0)
            goto next_node;
#endif

        /* Edge 3: NormalClip(pack(V2), pack(V0), point); V0 re-packed
         * with lhu again. */
        pack_a = (wm_84db8_load_u32(WM_84DB8_SC_V2 + 8u) << 16) |
                 (wm_84db8_load_u32(WM_84DB8_SC_V2 + 0u) & 0xFFFFu);
        wm_84db8_store_u32(WM_84DB8_SC_PACK_A, pack_a);
        pack_b = (wm_84db8_load_u32(WM_84DB8_SC_V0 + 8u) << 16) |
                 (u32)wm_84db8_load_u16(WM_84DB8_SC_V0 + 0u);
        wm_84db8_store_u32(WM_84DB8_SC_PACK_B, pack_b);
#if defined(WM_84DB8_MUTANT_ACCEPT_GEZ)
        if (WM_84DB8_NCLIP(pack_a, pack_b, point) >= 0)
            goto next_node;
#else
        if (WM_84DB8_NCLIP(pack_a, pack_b, point) > 0)
            goto next_node;
#endif

        /* Accept: append (id, type) halfwords, stride 4; result += 2. */
        wm_84db8_store_u16(out_id, (u16)(u32)n);
#if defined(WM_84DB8_MUTANT_APPEND_STRIDE)
        out_id += 2u;
#else
        out_id += 4u;
#endif
        result += 2;
        wm_84db8_store_u16(out_type, wm_84db8_load_u16(node + 0xCu));
#if defined(WM_84DB8_MUTANT_APPEND_STRIDE)
        out_type += 2u;
#else
        out_type += 4u;
#endif

    next_node:
        n++;
        node += 0xEu;
    } while (n < count);

    return result;
}
