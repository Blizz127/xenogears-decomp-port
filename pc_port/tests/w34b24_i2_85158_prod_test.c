/*
 * Focused production-linked oracle for retail world helper 0x80085158
 * (node-plane prober).
 *
 * Expected values are hand-derived from the retail disassembly and the
 * accepted wm_800935DC contract; the height reference recomputes the
 * plane equation independently with s64 products truncated to the low
 * word and C99 truncating division.  The six libgte residents are
 * provided by THIS file as recording, test-controlled implementations
 * (accepted w34b21_c1b_85760 pattern); production links PsyCross.
 * wm_800935DC itself is the REAL accepted implementation.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_plane_solver.h"
#include "world_map_helper_85158.h"

#define REC_TABLE_PTR 0x8009C620u
#define REC_BASE      0x800C0000u
#define NODES_BASE    0x800D0000u
#define VERTS_BASE    0x800E0000u
#define POS_VEC       0x800B0000u
#define COEF_OUT      0x1F800070u
#define NORM_OUT      0x1F800080u
#define SC_V0         0x1F800000u
#define SC_V1         0x1F800010u
#define SC_V2         0x1F800020u
#define SC_MTX        0x1F8000F0u

typedef struct {
    s32 vx;
    s32 vy;
    s32 vz;
    s32 pad;
} TVec;

typedef struct {
    s16 m[3][3];
    s16 pad;
    s32 t[3];
} TMtx;

typedef struct {
    s16 vx;
    s16 vy;
    s16 vz;
    s16 pad;
} TSvec;

static int s_failures;

static void check_u32(const char *name, u32 got, u32 expected)
{
    if (got != expected) {
        fprintf(stderr, "ASSERTION %s: got=0x%08x expected=0x%08x\n",
                name, got, expected);
        s_failures++;
    }
}

static void check_ptr(const char *name, const void *got, const void *want)
{
    if (got != want) {
        fprintf(stderr, "ASSERTION %s: got=%p expected=%p\n", name, got,
                want);
        s_failures++;
    }
}

static u32 rd(u32 a)
{
    u32 v;

    memcpy(&v, PSX_ADDR(a), 4);
    return v;
}

static void wr(u32 a, u32 v)
{
    memcpy(PSX_ADDR(a), &v, 4);
}

static void wr_s16(u32 a, s16 v)
{
    memcpy(PSX_ADDR(a), &v, 2);
}

static s32 S(u32 b)
{
    s32 v;

    memcpy(&v, &b, 4);
    return v;
}

static u32 U(s32 v)
{
    u32 b;

    memcpy(&b, &v, 4);
    return b;
}

void wm_85158_test_store(u32 address, u32 value)
{
    (void)address;
    (void)value;
}

/* ----------------------- recording libgte seams --------------------- */

static u32 s_seq;
static u32 s_scale_seq, s_setrot_seq, s_settrans_seq;
static const void *s_scale_m, *s_scale_v;
static TVec s_scale_vec_snap;
static TMtx s_settrans_snap;
static u32 s_rt_count;
static const void *s_rt_v0[3];
static const void *s_rt_out[3];
static TVec s_rt_forced[3];
static u32 s_op0_seq;
static const void *s_op0_p0, *s_op0_p1, *s_op0_p2;
static TVec s_op0_in0_snap, s_op0_in1_snap;
static TVec s_op0_forced;
static u32 s_vn_seq;
static const void *s_vn_p0, *s_vn_p1;
static TVec s_vn_in_snap;
static TVec s_vn_forced;

TMtx *ScaleMatrix(TMtx *m, TVec *v)
{
    s_scale_seq = ++s_seq;
    s_scale_m = m;
    s_scale_v = v;
    s_scale_vec_snap = *v;
    return m;
}

void SetRotMatrix(TMtx *m)
{
    s_setrot_seq = ++s_seq;
    (void)m;
}

void SetTransMatrix(TMtx *m)
{
    s_settrans_seq = ++s_seq;
    s_settrans_snap = *m;
}

void RotTrans(TSvec *v0, TVec *v1, long *flag)
{
    if (s_rt_count < 3u) {
        s_rt_v0[s_rt_count] = v0;
        s_rt_out[s_rt_count] = v1;
        *v1 = s_rt_forced[s_rt_count];
    }
    s_rt_count++;
    (void)flag;
}

void OuterProduct0(TVec *v0, TVec *v1, TVec *v2)
{
    s_op0_seq = ++s_seq;
    s_op0_p0 = v0;
    s_op0_p1 = v1;
    s_op0_p2 = v2;
    s_op0_in0_snap = *v0;
    s_op0_in1_snap = *v1;
    *v2 = s_op0_forced;
}

long VectorNormal(TVec *v0, TVec *v1)
{
    s_vn_seq = ++s_seq;
    s_vn_p0 = v0;
    s_vn_p1 = v1;
    s_vn_in_snap = *v0;
    *v1 = s_vn_forced;
    return 1;
}

static void reset_seams(void)
{
    s_seq = 0;
    s_scale_seq = s_setrot_seq = s_settrans_seq = 0;
    s_rt_count = 0;
    s_op0_seq = s_vn_seq = 0;
}

/* Independent height reference: accepted 935DC contract with s64
 * low-word truncation and C truncating division. */
static u32 ref_height(u32 A0, u32 A2, u32 B0, u32 B1, u32 B2,
                      s32 N0, s32 N1, s32 N2)
{
    u32 dx = A0 - B0;
    u32 p0 = (u32)(((s64)N0 * (s64)S(dx)) & 0xFFFFFFFFll);
    u32 dz = A2 - B2;
    u32 p1 = (u32)(((s64)N2 * (s64)S(dz)) & 0xFFFFFFFFll);
    u32 num = 0u - p0 - p1;
    s32 q = S(num) / N1;

    return U(q) + B1;
}

/* --------------------------- fixture ------------------------------- */

#define ATTR      3u
#define NODE_ID   5u
#define REC       (REC_BASE + ATTR * 84u)

static void build_fixture(u32 rec, u32 node_id, s16 i0, s16 i1, s16 i2)
{
    u32 node = NODES_BASE + 8u + 14u * node_id;
    u32 k;

    wr(REC_TABLE_PTR, REC_BASE);
    wr(rec + 8u, 0x111u);           /* cell X */
    wr(rec + 0xCu, 0x222u);         /* y-trans */
    wr(rec + 0x10u, 0x333u);        /* cell Z */
    for (k = 0u; k < 8u; k++)
        wr(rec + 0x20u + 4u * k, 0xA0A00000u + k); /* matrix words */
    wr(rec + 0x44u, NODES_BASE);
    wr(NODES_BASE + 4u, VERTS_BASE);
    wr_s16(node + 0u, i0);
    wr_s16(node + 2u, i1);
    wr_s16(node + 4u, i2);
}

static void run_case(const char *tag, u32 attr_arg, u32 node_id,
                     s16 i0, s16 i1, s16 i2, s32 px, s32 pz,
                     TVec v0, TVec v1, TVec v2, TVec cross, TVec unit)
{
    char name[96];
    u32 got;
    u32 A0, A2, want_h;
    u32 masked_attr = attr_arg & 0xFFFFu;
    u32 rec = REC_BASE + masked_attr * 84u;
    u32 node = NODES_BASE + 8u + 14u * (node_id & 0xFFFFu);

    build_fixture(rec, node_id & 0xFFFFu, i0, i1, i2);
    wr(POS_VEC + 0u, U(px));
    wr(POS_VEC + 8u, U(pz));

    reset_seams();
    s_rt_forced[0] = v0;
    s_rt_forced[1] = v1;
    s_rt_forced[2] = v2;
    s_op0_forced = cross;
    s_vn_forced = unit;

    got = wm_80085158(POS_VEC, COEF_OUT, NORM_OUT, attr_arg, node_id);

    /* coef outputs: wrap-faithful, asymmetric */
    A0 = (U(px) >> 12 | ((U(px) & 0x80000000u) ? 0xFFF00000u : 0u)) -
         rd(rec + 8u);
    A2 = rd(rec + 0x10u) -
         (U(pz) >> 12 | ((U(pz) & 0x80000000u) ? 0xFFF00000u : 0u));
    snprintf(name, sizeof(name), "%s.coef0", tag);
    check_u32(name, rd(COEF_OUT + 0u), A0);
    snprintf(name, sizeof(name), "%s.coef8", tag);
    check_u32(name, rd(COEF_OUT + 8u), A2);

    /* GTE sequence and pointer identity */
    snprintf(name, sizeof(name), "%s.order", tag);
    check_u32(name, (s_scale_seq == 1u && s_setrot_seq == 2u &&
                     s_settrans_seq == 3u && s_op0_seq == 4u &&
                     s_vn_seq == 5u) ? 1u : 0u, 1u);
    snprintf(name, sizeof(name), "%s.scale_m", tag);
    check_ptr(name, s_scale_m, PSX_ADDR(SC_MTX));
    snprintf(name, sizeof(name), "%s.scale_v", tag);
    check_ptr(name, s_scale_v, PSX_ADDR(SC_V1));
    snprintf(name, sizeof(name), "%s.scale_vec", tag);
    check_u32(name, (s_scale_vec_snap.vx == 0x800 &&
                     s_scale_vec_snap.vy == 0x800 &&
                     s_scale_vec_snap.vz == 0x800) ? 1u : 0u, 1u);
    snprintf(name, sizeof(name), "%s.trans", tag);
    check_u32(name, (s_settrans_snap.t[0] == 0 &&
                     (u32)s_settrans_snap.t[1] == rd(rec + 0xCu) &&
                     s_settrans_snap.t[2] == 0) ? 1u : 0u, 1u);

    /* RotTrans vertex addressing (s16 index, *8) and out slots */
    snprintf(name, sizeof(name), "%s.rt_count", tag);
    check_u32(name, s_rt_count, 3u);
    snprintf(name, sizeof(name), "%s.rt0_v", tag);
    check_ptr(name, s_rt_v0[0], PSX_ADDR(VERTS_BASE + (U((s32)i0) << 3)));
    snprintf(name, sizeof(name), "%s.rt1_v", tag);
    check_ptr(name, s_rt_v0[1], PSX_ADDR(VERTS_BASE + (U((s32)i1) << 3)));
    snprintf(name, sizeof(name), "%s.rt2_v", tag);
    check_ptr(name, s_rt_v0[2], PSX_ADDR(VERTS_BASE + (U((s32)i2) << 3)));
    snprintf(name, sizeof(name), "%s.rt_outs", tag);
    check_u32(name, (s_rt_out[0] == PSX_ADDR(SC_V0) &&
                     s_rt_out[1] == PSX_ADDR(SC_V1) &&
                     s_rt_out[2] == PSX_ADDR(SC_V2)) ? 1u : 0u, 1u);
    (void)node;

    /* Edge block: OuterProduct0 inputs must be V2-V0 (in0) and V1-V0
     * (in1), computed with u32 wrap. */
    snprintf(name, sizeof(name), "%s.op0_ptrs", tag);
    check_u32(name, (s_op0_p0 == PSX_ADDR(SC_V2) &&
                     s_op0_p1 == PSX_ADDR(SC_V1) &&
                     s_op0_p2 == PSX_ADDR(SC_V2)) ? 1u : 0u, 1u);
    snprintf(name, sizeof(name), "%s.op0_in0", tag);
    check_u32(name, (U(s_op0_in0_snap.vx) == U(v2.vx) - U(v0.vx) &&
                     U(s_op0_in0_snap.vy) == U(v2.vy) - U(v0.vy) &&
                     U(s_op0_in0_snap.vz) == U(v2.vz) - U(v0.vz)) ? 1u : 0u,
              1u);
    snprintf(name, sizeof(name), "%s.op0_in1", tag);
    check_u32(name, (U(s_op0_in1_snap.vx) == U(v1.vx) - U(v0.vx) &&
                     U(s_op0_in1_snap.vy) == U(v1.vy) - U(v0.vy) &&
                     U(s_op0_in1_snap.vz) == U(v1.vz) - U(v0.vz)) ? 1u : 0u,
              1u);

    /* sra 2 between cross and VectorNormal (arithmetic, sign-correct) */
    snprintf(name, sizeof(name), "%s.vn_in", tag);
    check_u32(name, (s_vn_in_snap.vx == cross.vx >> 2 &&
                     s_vn_in_snap.vy == cross.vy >> 2 &&
                     s_vn_in_snap.vz == cross.vz >> 2) ? 1u : 0u, 1u);
    snprintf(name, sizeof(name), "%s.vn_ptrs", tag);
    check_u32(name, (s_vn_p0 == PSX_ADDR(SC_V2) &&
                     s_vn_p1 == PSX_ADDR(NORM_OUT)) ? 1u : 0u, 1u);

    /* Height: real 935DC vs independent reference.  Base point is the
     * (preserved) forced V0 at scratch 0x00; normal is forced unit. */
    want_h = ref_height(A0, A2, U(v0.vx), U(v0.vy), U(v0.vz),
                        unit.vx, unit.vy, unit.vz);
    snprintf(name, sizeof(name), "%s.height", tag);
    check_u32(name, rd(COEF_OUT + 4u), want_h);
    snprintf(name, sizeof(name), "%s.ret", tag);
    check_u32(name, got, want_h);

    printf("%s: coef=(%d,%d) h=%d\n", tag, S(A0), S(A2), S(want_h));
}

int main(void)
{
    TVec v0, v1, v2, cross, unit;

    PsxMemory_Init();

    /* Case 1: plain positive geometry. */
    v0 = (TVec){ 100, 50, 200, 0 };
    v1 = (TVec){ 300, 60, 220, 0 };
    v2 = (TVec){ 150, 40, 500, 0 };
    cross = (TVec){ 4000, -16000, 800, 0 };
    unit = (TVec){ 500, -3900, 700, 0 };
    run_case("basic", ATTR, NODE_ID, 2, 7, 4,
             0x00654000, 0x00987000, v0, v1, v2, cross, unit);

    /* Case 2: attr with high garbage bits (andi 0xFFFF) + negative
     * vertex index (lh sign extension) + negative cross components. */
    cross = (TVec){ -4001, 15999, -801, 0 };
    unit = (TVec){ -500, 3900, -700, 0 };
    run_case("mask_negidx", 0x00030000u | ATTR, NODE_ID, -2, 3, 1,
             -0x00654000, -0x00987000, v0, v1, v2, cross, unit);

    /* Case 3: high-bit node id (lhu zero extension; id 0x8001). */
    cross = (TVec){ 12, -8, 20, 0 };
    unit = (TVec){ 3, -4096, 5, 0 };
    run_case("hi_node", ATTR, 0x8001u, 1, 2, 3,
             0x7FFFF000, S(0x80000FFFu), v0, v1, v2, cross, unit);

    /* Case 4: wrap-heavy coefficients and INT_MIN-ish vertex deltas. */
    v0 = (TVec){ S(0x80000000u), 1, S(0x7FFFFFFF), 0 };
    v1 = (TVec){ 5, 6, 7, 0 };
    v2 = (TVec){ -5, -6, -7, 0 };
    cross = (TVec){ S(0x80000004u), 0x1000, -4, 0 };
    unit = (TVec){ 1, 2, 3, 0 };
    run_case("wrap", ATTR, NODE_ID, 0, 1, 2,
             S(0xFFFFF000u), 0x00001000, v0, v1, v2, cross, unit);

    if (s_failures != 0) {
        fprintf(stderr, "FAILURES=%d\n", s_failures);
        return 1;
    }
    printf("W34B24-I2 0x80085158 focused oracle PASS\n");
    return 0;
}
