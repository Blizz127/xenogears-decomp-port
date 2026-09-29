/*
 * Focused production-linked oracle for retail world helper 0x80085418
 * (node-triangle plane-straddle filter).
 *
 * Expected values are hand-derived from the retail disassembly; the
 * libgte residents are provided by THIS file as recording,
 * test-controlled implementations (accepted w34b21_c1b_85760 pattern),
 * modeling the SetRotMatrix/SetTransMatrix -> RotTrans state hand-off
 * at the contract level: every call records sequence, exact pointers,
 * and input snapshots; outputs are test-chosen to drive each downstream
 * stage independently of GTE internals (which are canonical PsyCross at
 * the production link and not under test here).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_85418.h"

#define REC_TABLE_PTR 0x8009C620u
#define REC_BASE      0x800C0000u
#define NODE_ARR      0x800C8000u
#define VTX_BASE      0x800CC000u
#define POS_VEC       0x800B0000u

#define SC_V0     0x1F800000u
#define SC_V1     0x1F800010u
#define SC_V2     0x1F800020u
#define SC_NORM   0x1F800030u
#define SC_DOTS   0x1F800040u
#define SC_MATRIX 0x1F8000F0u
#define SC_PROBES 0x1F800110u

typedef struct {
    s16 m[3][3];
    s16 pad;
    s32 t[3];
} TMat;

typedef struct {
    s32 vx;
    s32 vy;
    s32 vz;
    s32 pad;
} TVec;

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

static void check_s32(const char *name, s32 got, s32 expected)
{
    if (got != expected) {
        fprintf(stderr,
                "ASSERTION %s: got=%d (0x%08x) expected=%d (0x%08x)\n",
                name, got, (u32)got, expected, (u32)expected);
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

static void wr(u32 a, u32 v)
{
    memcpy(PSX_ADDR(a), &v, 4);
}

static void wr16(u32 a, u32 v)
{
    u16 h = (u16)v;

    memcpy(PSX_ADDR(a), &h, 2);
}

static void wr_s32(u32 a, s32 v)
{
    u32 b;

    memcpy(&b, &v, 4);
    wr(a, b);
}

/* ------------------------- trace seams ------------------------------- */

typedef struct {
    u32 a;
    u32 w;
    u32 v;
} StEv;

static StEv s_stores[128];
static u32 s_nstores;

void wm_85418_test_load(u32 a, u32 w, u32 v)
{
    (void)a;
    (void)w;
    (void)v;
}

void wm_85418_test_store(u32 a, u32 w, u32 v)
{
    if (s_nstores < 128u) {
        s_stores[s_nstores].a = a;
        s_stores[s_nstores].w = w;
        s_stores[s_nstores].v = v;
    }
    s_nstores++;
}

/* ------------------- recording libgte residents ---------------------- */

static u32 s_seq;
static u32 q_scale, q_setrot, q_settrans, q_rt[3], q_op0, q_vn, q_amlv;
static u32 s_rt_n;
static const void *p_scale_m, *p_scale_v;
static TMat snap_scale_m;
static TVec snap_scale_v;
static TMat snap_setrot, snap_settrans;
static const void *p_rt_in[3], *p_rt_out[3];
static TSvec snap_rt_in[3];
static TVec ctl_rt_out[3];
static const void *p_op0_a, *p_op0_b, *p_op0_o;
static TVec snap_op0_a, snap_op0_b;
static TVec ctl_op0_out;
static const void *p_vn_in, *p_vn_out;
static TVec snap_vn_in;
static TVec ctl_vn_out;
static const void *p_amlv_m, *p_amlv_v, *p_amlv_o;
static s16 snap_amlv_rows[6]; /* rows 0 and 1 only; row2 is dead scratch */
static TVec snap_amlv_n;
static s32 ctl_dotA, ctl_dotB;

TMat *ScaleMatrix(TMat *m, TVec *v)
{
    q_scale = ++s_seq;
    p_scale_m = m;
    p_scale_v = v;
    snap_scale_m = *m;
    snap_scale_v = *v;
    return m;
}

void SetRotMatrix(TMat *m)
{
    q_setrot = ++s_seq;
    snap_setrot = *m;
}

void SetTransMatrix(TMat *m)
{
    q_settrans = ++s_seq;
    snap_settrans = *m;
}

void RotTrans(TSvec *v0, TVec *v1, long *flag)
{
    u32 n = s_rt_n < 3u ? s_rt_n : 2u;

    q_rt[n] = ++s_seq;
    p_rt_in[n] = v0;
    p_rt_out[n] = v1;
    snap_rt_in[n] = *v0;
    *v1 = ctl_rt_out[n];
    *flag = 0;
    s_rt_n++;
}

void OuterProduct0(TVec *v0, TVec *v1, TVec *v2)
{
    q_op0 = ++s_seq;
    p_op0_a = v0;
    p_op0_b = v1;
    p_op0_o = v2;
    snap_op0_a = *v0;
    snap_op0_b = *v1;
    *v2 = ctl_op0_out;
}

long VectorNormal(TVec *v0, TVec *v1)
{
    q_vn = ++s_seq;
    p_vn_in = v0;
    p_vn_out = v1;
    snap_vn_in = *v0;
    *v1 = ctl_vn_out;
    return 1;
}

void ApplyMatrixLV(TMat *m, TVec *v0, TVec *v1)
{
    q_amlv = ++s_seq;
    p_amlv_m = m;
    p_amlv_v = v0;
    p_amlv_o = v1;
    memcpy(snap_amlv_rows, m, sizeof(snap_amlv_rows));
    snap_amlv_n = *v0;
    v1->vx = ctl_dotA;
    v1->vy = ctl_dotB;
    /* v1->vz deliberately untouched: the retail dead-row2 dot is never
     * read; leaving it stale proves production doesn't consume it. */
}

/* --------------------------- fixture --------------------------------- */

#define ATTR    3u
#define NODE_ID 2u

static void build_fixture(u32 attr_slot)
{
    u32 rec = REC_BASE + 84u * attr_slot;
    u32 i;

    wr(REC_TABLE_PTR, REC_BASE);
    wr_s32(rec + 0x08u, 100);         /* rx */
    wr_s32(rec + 0x0Cu, 0x123456);    /* trans y */
    wr_s32(rec + 0x10u, 900);         /* rz */
    /* matrix copy source: distinctive halfwords 0xA100.. */
    for (i = 0; i < 16u; i++)
        wr16(rec + 0x20u + 2u * i, 0xA100u + i);
    wr(rec + 0x44u, NODE_ARR);
    wr(NODE_ARR + 4u, VTX_BASE);
    /* node NODE_ID at NODE_ARR + 8 + 14*id: vertex indices 5, -2, 9 */
    {
        u32 node = NODE_ARR + 8u + 14u * NODE_ID;

        wr16(node + 0u, 5u);
        wr16(node + 2u, (u32)(u16)(s16)-2);
        wr16(node + 4u, 9u);
    }
    /* distinct halfwords at the WRONG-stride node location so the
     * stride mutant reads different indices */
    {
        u32 wrongnode = NODE_ARR + 8u + 12u * NODE_ID;

        if (wrongnode != NODE_ARR + 8u + 14u * NODE_ID) {
            wr16(wrongnode + 0u, 77u);
            wr16(wrongnode + 2u, 78u);
        }
    }
}

static void seed_scratch_canary(void)
{
    u32 a;

    for (a = 0x1F800000u; a < 0x1F800130u; a += 4u)
        wr(a, 0xC5C5C5C5u);
    /* scratch 0x70: value read by the y_offset-as-pointer mutant */
    wr(0x1F800070u, 0x00500000u);
}

/* One full run with controlled stage outputs; returns production result. */
static s32 run_case(s32 pos_x, s32 pos_y, s32 pos_z, s32 y_off,
                    s32 dotA, s32 dotB)
{
    s_seq = 0;
    s_rt_n = 0;
    s_nstores = 0;
    /* Controlled RotTrans outputs (transformed vertices). */
    ctl_rt_out[0].vx = 10;   ctl_rt_out[0].vy = 20;   ctl_rt_out[0].vz = 30;
    ctl_rt_out[0].pad = 0;
    ctl_rt_out[1].vx = 110;  ctl_rt_out[1].vy = 250;  ctl_rt_out[1].vz = -70;
    ctl_rt_out[1].pad = 0;
    ctl_rt_out[2].vx = -40;  ctl_rt_out[2].vy = 21;   ctl_rt_out[2].vz = 5030;
    ctl_rt_out[2].pad = 0;
    /* Controlled cross product: choose values where >>1 differs from >>2
     * even after sign, and negative components exercise sra. */
    ctl_op0_out.vx = 0x1001;
    ctl_op0_out.vy = -0x2003;
    ctl_op0_out.vz = 7;
    ctl_op0_out.pad = 0;
    ctl_vn_out.vx = 3;
    ctl_vn_out.vy = -4;
    ctl_vn_out.vz = 5;
    ctl_vn_out.pad = 0;
    ctl_dotA = dotA;
    ctl_dotB = dotB;

    build_fixture(ATTR);
    seed_scratch_canary();
    wr_s32(POS_VEC + 0u, pos_x);
    wr_s32(POS_VEC + 4u, pos_y);
    wr_s32(POS_VEC + 8u, pos_z);

    return wm_80085418(POS_VEC, y_off, ATTR, NODE_ID);
}

static void verify_pipeline(const char *tag, s32 pos_x, s32 pos_y,
                            s32 pos_z, s32 y_off)
{
    char n[96];
    u32 i;

    /* Call order. */
    snprintf(n, sizeof(n), "%s.order", tag);
    if (!(q_scale == 1u && q_setrot == 2u && q_settrans == 3u &&
          q_rt[0] == 4u && q_rt[1] == 5u && q_rt[2] == 6u &&
          q_op0 == 7u && q_vn == 8u && q_amlv == 9u)) {
        fprintf(stderr,
                "ASSERTION %s: got=%u,%u,%u,%u,%u,%u,%u,%u,%u expected=1..9\n",
                n, q_scale, q_setrot, q_settrans, q_rt[0], q_rt[1],
                q_rt[2], q_op0, q_vn, q_amlv);
        s_failures++;
    }

    /* ScaleMatrix args + content. */
    snprintf(n, sizeof(n), "%s.scale_m", tag);
    check_ptr(n, p_scale_m, PSX_ADDR(SC_MATRIX));
    snprintf(n, sizeof(n), "%s.scale_v", tag);
    check_ptr(n, p_scale_v, PSX_ADDR(SC_V1));
    for (i = 0; i < 9u; i++) {
        snprintf(n, sizeof(n), "%s.m[%u]", tag, i);
        check_u32(n, (u32)(u16)snap_scale_m.m[i / 3][i % 3], 0xA100u + i);
    }
    snprintf(n, sizeof(n), "%s.t0", tag);
    check_s32(n, snap_scale_m.t[0], 0);
    snprintf(n, sizeof(n), "%s.t1", tag);
    check_s32(n, snap_scale_m.t[1], 0x123456);
    snprintf(n, sizeof(n), "%s.t2", tag);
    check_s32(n, snap_scale_m.t[2], 0);
    snprintf(n, sizeof(n), "%s.scalevec", tag);
    if (snap_scale_v.vx != 0x800 || snap_scale_v.vy != 0x800 ||
        snap_scale_v.vz != 0x800) {
        fprintf(stderr, "ASSERTION %s: got=(%d,%d,%d) expected 0x800^3\n",
                n, snap_scale_v.vx, snap_scale_v.vy, snap_scale_v.vz);
        s_failures++;
    }
    /* SetRot/SetTrans see the same matrix object. */
    snprintf(n, sizeof(n), "%s.setrot_t1", tag);
    check_s32(n, snap_setrot.t[1], 0x123456);
    snprintf(n, sizeof(n), "%s.settrans_t1", tag);
    check_s32(n, snap_settrans.t[1], 0x123456);

    /* RotTrans inputs: vtx = VTX_BASE + idx*8 for idx 5, -2, 9. */
    snprintf(n, sizeof(n), "%s.rt0_in", tag);
    check_ptr(n, p_rt_in[0], PSX_ADDR(VTX_BASE + 5u * 8u));
    snprintf(n, sizeof(n), "%s.rt1_in", tag);
    check_ptr(n, p_rt_in[1], PSX_ADDR(VTX_BASE - 16u));
    snprintf(n, sizeof(n), "%s.rt2_in", tag);
    check_ptr(n, p_rt_in[2], PSX_ADDR(VTX_BASE + 9u * 8u));
    snprintf(n, sizeof(n), "%s.rt0_out", tag);
    check_ptr(n, p_rt_out[0], PSX_ADDR(SC_V0));
    snprintf(n, sizeof(n), "%s.rt1_out", tag);
    check_ptr(n, p_rt_out[1], PSX_ADDR(SC_V1));
    snprintf(n, sizeof(n), "%s.rt2_out", tag);
    check_ptr(n, p_rt_out[2], PSX_ADDR(SC_V2));

    /* Edge math at OuterProduct0: a = V2-V0 (in place at SC_V2),
     * b = V1-V0 (at SC_V1), from the controlled RotTrans outputs. */
    snprintf(n, sizeof(n), "%s.op0_ptrs", tag);
    if (p_op0_a != PSX_ADDR(SC_V2) || p_op0_b != PSX_ADDR(SC_V1) ||
        p_op0_o != PSX_ADDR(SC_V2)) {
        fprintf(stderr, "ASSERTION %s: got=(%p,%p,%p)\n", n, p_op0_a,
                p_op0_b, p_op0_o);
        s_failures++;
    }
    snprintf(n, sizeof(n), "%s.edgeA", tag);
    if (snap_op0_a.vx != -40 - 10 || snap_op0_a.vy != 21 - 20 ||
        snap_op0_a.vz != 5030 - 30) {
        fprintf(stderr, "ASSERTION %s: got=(%d,%d,%d)\n", n,
                snap_op0_a.vx, snap_op0_a.vy, snap_op0_a.vz);
        s_failures++;
    }
    snprintf(n, sizeof(n), "%s.edgeB", tag);
    if (snap_op0_b.vx != 110 - 10 || snap_op0_b.vy != 250 - 20 ||
        snap_op0_b.vz != -70 - 30) {
        fprintf(stderr, "ASSERTION %s: got=(%d,%d,%d)\n", n,
                snap_op0_b.vx, snap_op0_b.vy, snap_op0_b.vz);
        s_failures++;
    }

    /* VectorNormal input = controlled OP0 output >> 2 (arithmetic). */
    snprintf(n, sizeof(n), "%s.vn_in", tag);
    if (snap_vn_in.vx != (0x1001 >> 2) || snap_vn_in.vy != (-0x2003 >> 2) ||
        snap_vn_in.vz != (7 >> 2)) {
        fprintf(stderr, "ASSERTION %s: got=(%d,%d,%d) expected=(%d,%d,%d)\n",
                n, snap_vn_in.vx, snap_vn_in.vy, snap_vn_in.vz,
                0x1001 >> 2, -0x2003 >> 2, 7 >> 2);
        s_failures++;
    }
    snprintf(n, sizeof(n), "%s.vn_ptrs", tag);
    if (p_vn_in != PSX_ADDR(SC_V2) || p_vn_out != PSX_ADDR(SC_NORM)) {
        fprintf(stderr, "ASSERTION %s\n", n);
        s_failures++;
    }

    /* Probe rows at ApplyMatrixLV: hand-derived, s16 truncation. */
    {
        s32 dx = (pos_x >> 12) - 100 - 10;
        s32 dy1 = (pos_y >> 12) - 20;
        s32 dy2 = dy1 - y_off;
        s32 dz = 900 - (pos_z >> 12) - 30;

        snprintf(n, sizeof(n), "%s.row0", tag);
        if (snap_amlv_rows[0] != (s16)dx || snap_amlv_rows[1] != (s16)dy1 ||
            snap_amlv_rows[2] != (s16)dz) {
            fprintf(stderr,
                    "ASSERTION %s: got=(%d,%d,%d) expected=(%d,%d,%d)\n", n,
                    snap_amlv_rows[0], snap_amlv_rows[1], snap_amlv_rows[2],
                    (s16)dx, (s16)dy1, (s16)dz);
            s_failures++;
        }
        snprintf(n, sizeof(n), "%s.row1", tag);
        if (snap_amlv_rows[3] != (s16)dx || snap_amlv_rows[4] != (s16)dy2 ||
            snap_amlv_rows[5] != (s16)dz) {
            fprintf(stderr,
                    "ASSERTION %s: got=(%d,%d,%d) expected=(%d,%d,%d)\n", n,
                    snap_amlv_rows[3], snap_amlv_rows[4], snap_amlv_rows[5],
                    (s16)dx, (s16)dy2, (s16)dz);
            s_failures++;
        }
    }
    snprintf(n, sizeof(n), "%s.amlv_ptrs", tag);
    if (p_amlv_m != PSX_ADDR(SC_PROBES) || p_amlv_v != PSX_ADDR(SC_NORM) ||
        p_amlv_o != PSX_ADDR(SC_DOTS)) {
        fprintf(stderr, "ASSERTION %s\n", n);
        s_failures++;
    }
    snprintf(n, sizeof(n), "%s.amlv_n", tag);
    if (snap_amlv_n.vx != 3 || snap_amlv_n.vy != -4 || snap_amlv_n.vz != 5) {
        fprintf(stderr, "ASSERTION %s\n", n);
        s_failures++;
    }

    /* Production never writes the dead row2 scratch (0x11C..0x121). */
    for (i = 0; i < s_nstores && i < 128u; i++) {
        if (s_stores[i].a >= 0x1F80011Cu && s_stores[i].a < 0x1F800122u) {
            snprintf(n, sizeof(n), "%s.deadrow2_store", tag);
            fprintf(stderr, "ASSERTION %s: store at 0x%08x\n", n,
                    s_stores[i].a);
            s_failures++;
        }
    }
}

int main(void)
{
    s32 r;

    PsxMemory_Init();

    /* Sign matrix for the return value: (dotA, dotB) -> {0,-1}. */
    r = run_case(0x400000, 0x100000, 0x200000, 0x70, 500, 700);
    check_s32("ret.pp", r, 0);
    verify_pipeline("pp", 0x400000, 0x100000, 0x200000, 0x70);
    r = run_case(0x400000, 0x100000, 0x200000, 0x70, -500, -700);
    check_s32("ret.nn", r, 0);
    r = run_case(0x400000, 0x100000, 0x200000, 0x70, 500, -700);
    check_s32("ret.pn", r, -1);
    r = run_case(0x400000, 0x100000, 0x200000, 0x70, -500, 700);
    check_s32("ret.np", r, -1);
    r = run_case(0x400000, 0x100000, 0x200000, 0x70, 0, -1);
    check_s32("ret.zn", r, -1);
    r = run_case(0x400000, 0x100000, 0x200000, 0x70, 0, 1);
    check_s32("ret.zp", r, 0);
    r = run_case(0x400000, 0x100000, 0x200000, 0x70, (s32)0x80000000, 0);
    check_s32("ret.minz", r, -1);
    r = run_case(0x400000, 0x100000, 0x200000, 0x70, 0, 0);
    check_s32("ret.zz", r, 0);
    printf("sign matrix ok\n");

    /* s16 wrap in the probe stores: dx = 0x12345 - 110 -> wraps. */
    r = run_case(0x12345000 + (110 << 12), 0x100000, 0x200000, 0x70,
                 1, 2);
    verify_pipeline("wrap", 0x12345000 + (110 << 12), 0x100000, 0x200000,
                    0x70);
    printf("s16 wrap ok\n");

    /* Negative position components (sra 12) + dy2 sign crossing:
     * pos_y >> 12 = 0x70 + 20 - 0x20 makes dy1 = 0x50, dy2 = -0x20. */
    r = run_case(-20480000, (0x50 + 20) << 12, -81920000, 0x70, -3, 3);
    check_s32("ret.negpos", r, -1);
    verify_pipeline("negpos", -20480000, (0x50 + 20) << 12, -81920000,
                    0x70);
    printf("negative/sra12/dy2 ok\n");

    /* attr masking (andi 0xFFFF): attr | 0x10000 must behave as attr. */
    s_seq = 0;
    s_rt_n = 0;
    s_nstores = 0;
    ctl_dotA = 5;
    ctl_dotB = 7;
    build_fixture(ATTR);
    seed_scratch_canary();
    wr_s32(POS_VEC + 0u, 0x400000);
    wr_s32(POS_VEC + 4u, 0x100000);
    wr_s32(POS_VEC + 8u, 0x200000);
    r = wm_80085418(POS_VEC, 0x70, 0x10000u | ATTR, NODE_ID);
    check_s32("mask.ret", r, 0);
    check_u32("mask.m0", (u32)(u16)snap_scale_m.m[0][0], 0xA100u);
    check_s32("mask.t1", snap_scale_m.t[1], 0x123456);
    printf("attr mask ok\n");

    if (s_failures != 0) {
        fprintf(stderr, "FAILURES=%d\n", s_failures);
        return 1;
    }
    printf("W34B24-I3 0x80085418 focused oracle PASS\n");
    return 0;
}
