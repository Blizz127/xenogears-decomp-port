/*
 * Focused production-linked oracle for retail world helpers
 *   0x80084DB8 (nav-mesh point-in-triangle probe)
 *   0x80084D00 (region scanner)
 *
 * Expected values are hand-derived from the retail disassembly.  The
 * GTE residents and NormalClip are provided as recording, scripted
 * test implementations (accepted w34b21_c1b_85760 pattern); the
 * production link binds PsyCross.  The synthetic region/mesh fixture
 * lives in emulated RAM; every pack/point expectation is computed by
 * hand in the assertions (z<<16 truncation, lhu low-u16 x, asymmetric
 * deltas, negu-wrap gates).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_84db8.h"
#include "world_map_helper_84d00.h"

#define REC_PTR_ADDR   0x8009C620u
#define COUNT_ADDR     0x8009D7E0u
#define CAND_ID        0x8009D718u
#define REGIONS        0x800C0000u  /* 84-byte records */
#define HDR            0x800C1000u
#define VERTS          0x800C2000u
#define POS_VEC        0x800C3000u
#define OUT_ATTR       0x800C3100u

#define SC_V0    0x1F800000u
#define SC_MTX   0x1F8000F0u

static int s_failures;

static void check(const char *name, u32 got, u32 want)
{
    if (got != want) {
        fprintf(stderr, "ASSERTION %s: got=0x%08x expected=0x%08x\n",
                name, got, want);
        s_failures++;
        if (s_failures > 30)
            exit(1);
    }
}

static u32 rd32(u32 a) { u32 v; memcpy(&v, PSX_ADDR(a), 4); return v; }
static u16 rd16(u32 a) { u16 v; memcpy(&v, PSX_ADDR(a), 2); return v; }
static void wr32(u32 a, u32 v) { memcpy(PSX_ADDR(a), &v, 4); }
static void wr16(u32 a, u16 v) { memcpy(PSX_ADDR(a), &v, 2); }
static void wrs32(u32 a, s32 v) { u32 b; memcpy(&b, &v, 4); wr32(a, b); }

/* ------------------------- recording seams --------------------------- */

static u32 s_seq;
static u32 s_scale_seq, s_setrot_seq, s_settrans_seq;
static u32 s_scale_m, s_scale_v;
static u32 s_setrot_m, s_settrans_m;
static u32 s_mtx_snapshot[8];
static u32 s_scale_vec_snapshot[3];

void wm_84db8_test_scale_matrix(u32 m_addr, u32 v_addr)
{
    u32 i;

    s_scale_seq = ++s_seq;
    s_scale_m = m_addr;
    s_scale_v = v_addr;
    for (i = 0; i < 8u; i++)
        s_mtx_snapshot[i] = rd32(m_addr + 4u * i);
    for (i = 0; i < 3u; i++)
        s_scale_vec_snapshot[i] = rd32(v_addr + 4u * i);
}

void wm_84db8_test_set_rot(u32 m_addr) { s_setrot_seq = ++s_seq; s_setrot_m = m_addr; }
void wm_84db8_test_set_trans(u32 m_addr) { s_settrans_seq = ++s_seq; s_settrans_m = m_addr; }

/* RotTrans: record and write scripted MAC values keyed by SVECTOR addr. */
typedef struct {
    u32 sv_addr;
    s32 mx, my, mz;
} RtScript;

static RtScript s_rt_script[16];
static u32 s_rt_script_n;
static u32 s_rt_calls;
static u32 s_rt_addrs[64];
static u32 s_rt_outs[64];

void wm_84db8_test_rot_trans(u32 sv_addr, u32 out_addr)
{
    u32 i;

    if (s_rt_calls < 64u) {
        s_rt_addrs[s_rt_calls] = sv_addr;
        s_rt_outs[s_rt_calls] = out_addr;
    }
    s_rt_calls++;
    for (i = 0; i < s_rt_script_n; i++) {
        if (s_rt_script[i].sv_addr == sv_addr) {
            wrs32(out_addr + 0u, s_rt_script[i].mx);
            wrs32(out_addr + 4u, s_rt_script[i].my);
            wrs32(out_addr + 8u, s_rt_script[i].mz);
            return;
        }
    }
    fprintf(stderr, "ASSERTION rt.unknown_vertex: 0x%08x\n", sv_addr);
    exit(1);
}

/* NormalClip: record args, return scripted sequence. */
static s32 s_nclip_script[32];
static u32 s_nclip_script_n;
static u32 s_nclip_calls;
static u32 s_nclip_args[32][3];

s32 wm_84db8_test_normal_clip(u32 a, u32 b, u32 c)
{
    if (s_nclip_calls < 32u) {
        s_nclip_args[s_nclip_calls][0] = a;
        s_nclip_args[s_nclip_calls][1] = b;
        s_nclip_args[s_nclip_calls][2] = c;
    }
    if (s_nclip_calls >= s_nclip_script_n) {
        fprintf(stderr, "ASSERTION nclip.overrun: call %u\n", s_nclip_calls);
        exit(1);
    }
    return s_nclip_script[s_nclip_calls++];
}

void wm_84db8_test_store(u32 a, u32 v) { (void)a; (void)v; }

/* 84D00 -> 84DB8 seam: scripted or forwarding. */
static int s_fwd_84db8;
static s32 s_84db8_script[8];
static u32 s_84db8_script_n;
static u32 s_84db8_calls;
static u32 s_84db8_pos[8];
static s32 s_84db8_idx[8];
static s32 s_84db8_count_side_effect; /* write count after first call */

s32 wm_84d00_test_84db8(u32 pos_vec, s32 region_idx)
{
    if (s_84db8_calls < 8u) {
        s_84db8_pos[s_84db8_calls] = pos_vec;
        s_84db8_idx[s_84db8_calls] = region_idx;
    }
    if (s_fwd_84db8) {
        s_84db8_calls++;
        return wm_80084DB8(pos_vec, region_idx);
    }
    if (s_84db8_calls == 0u && s_84db8_count_side_effect >= 0)
        wr16(COUNT_ADDR, (u16)(u32)s_84db8_count_side_effect);
    if (s_84db8_calls >= s_84db8_script_n) {
        fprintf(stderr, "ASSERTION 84db8seam.overrun\n");
        exit(1);
    }
    return s_84db8_script[s_84db8_calls++];
}

static void reset(void)
{
    s_seq = 0; s_scale_seq = 0; s_setrot_seq = 0; s_settrans_seq = 0;
    s_rt_calls = 0; s_nclip_calls = 0; s_nclip_script_n = 0;
    s_rt_script_n = 0; s_84db8_calls = 0; s_84db8_script_n = 0;
    s_fwd_84db8 = 0; s_84db8_count_side_effect = -1;
}

/* --------------------------- fixture --------------------------------- */

static void build_region(u32 rec, u16 flags, s32 cx, s32 height, s32 cz,
                         u32 hdr)
{
    u32 i;

    wr16(rec + 4u, flags);
    wrs32(rec + 8u, cx);
    wrs32(rec + 0xCu, height);
    wrs32(rec + 0x10u, cz);
    for (i = 0; i < 8u; i++)
        wr32(rec + 0x20u + 4u * i, 0x11110000u + i); /* recognizable matrix */
    wr32(rec + 0x44u, hdr);
}

/* node: 14-byte record at hdr+8+14*n */
static void build_node(u32 hdr, u32 n, s16 i0, s16 i1, s16 i2, u16 type)
{
    u32 node = hdr + 8u + 14u * n;

    wr16(node + 0u, (u16)i0);
    wr16(node + 2u, (u16)i1);
    wr16(node + 4u, (u16)i2);
    wr16(node + 0xCu, type);
}

int main(void)
{
    PsxMemory_Init();

    wr32(REC_PTR_ADDR, REGIONS);

    /* ============ Section A: range gates (84DB8 direct) ============ */
    {
        static const struct {
            s32 posx_c;  /* pos.X sra12 target */
            s32 cx;
            s32 posz_c;
            s32 cz;
            int pass;    /* 1 = gate passes (reaches setup) */
            const char *tag;
        } G[] = {
            { 100, 30, 50, 90, 1, "in_range" },       /* dx=70 dz=40 */
            { 0x7FF, 0, 0, 0, 1, "dx_7ff" },          /* |dx|=0x7FF pass */
            { 0x800, 0, 0, 0, 0, "dx_800" },          /* |dx|=0x800 reject */
            { -0x800, 0, 0, 0, 0, "dx_neg800" },
            { 0, 0, 0x7FF, 0, 0x1, "dz_7ff" },        /* dz=-0x7FF pass */
            { 0, 0, 0x800, 0, 0, "dz_800" },
            { 0, (s32)0x80000000, 0, 0, 1, "dx_intmin_wrap" },
        };
        u32 gi;

        for (gi = 0; gi < sizeof(G) / sizeof(G[0]); gi++) {
            char name[64];
            s32 r;

            reset();
            /* empty mesh so a passing gate returns 0 after setup */
            wr32(HDR + 0u, 0u);
            wr32(HDR + 4u, VERTS);
            build_region(REGIONS, 1u, G[gi].cx, 777, G[gi].cz, HDR);
            wrs32(POS_VEC + 0u, (s32)((u32)G[gi].posx_c << 12));
            wrs32(POS_VEC + 8u, (s32)((u32)G[gi].posz_c << 12));

            r = wm_80084DB8(POS_VEC, 0);
            snprintf(name, sizeof(name), "A.%s.ret", G[gi].tag);
            check(name, (u32)r, 0u);
            snprintf(name, sizeof(name), "A.%s.setup", G[gi].tag);
            check(name, s_scale_seq != 0u, (u32)G[gi].pass);
            /* stored deltas are always written, even on reject */
            snprintf(name, sizeof(name), "A.%s.dx", G[gi].tag);
            check(name, rd32(0x1F800040u),
                  (u32)G[gi].posx_c - (u32)G[gi].cx);
            snprintf(name, sizeof(name), "A.%s.dz", G[gi].tag);
            check(name, rd32(0x1F800048u),
                  (u32)G[gi].cz - (u32)G[gi].posz_c);
        }
        printf("A gates ok\n");
    }

    /* ============ Section B: matrix/scale/trans contract ============ */
    {
        u32 i;

        reset();
        wr32(HDR + 0u, 0u);
        build_region(REGIONS, 1u, 0, 0x1234, 0, HDR);
        wrs32(POS_VEC + 0u, 0);
        wrs32(POS_VEC + 8u, 0);
        (void)wm_80084DB8(POS_VEC, 0);
        check("B.scale_first", s_scale_seq, 1u);
        check("B.rot_second", s_setrot_seq, 2u);
        check("B.trans_third", s_settrans_seq, 3u);
        check("B.scale_m", s_scale_m, SC_MTX);
        check("B.scale_v", s_scale_v, SC_V0);
        check("B.rot_m", s_setrot_m, SC_MTX);
        check("B.trans_m", s_settrans_m, SC_MTX);
        for (i = 0; i < 5u; i++) {
            char name[32];

            snprintf(name, sizeof(name), "B.mtx%u", i);
            check(name, s_mtx_snapshot[i], 0x11110000u + i);
        }
        /* trans words: +0x14 (t0) = 0, +0x18 (t1) = height, +0x1C = 0 */
        check("B.t0", s_mtx_snapshot[5], 0u);
        check("B.t1", s_mtx_snapshot[6], 0x1234u);
        check("B.t2", s_mtx_snapshot[7], 0u);
        check("B.sc0", s_scale_vec_snapshot[0], 0x800u);
        check("B.sc1", s_scale_vec_snapshot[1], 0x800u);
        check("B.sc2", s_scale_vec_snapshot[2], 0x800u);
        printf("B matrix contract ok\n");
    }

    /* ============ Section C: node loop, packs, accept/skip ============ */
    {
        /* dx = 100-30 = 70 (0x46); dz = 90-50 = 40 (0x28);
         * point = (40<<16)|70 = 0x00280046. */
        u32 point = (40u << 16) | 70u;
        s32 r;

        reset();
        build_region(REGIONS, 1u, 30, 777, 90, HDR);
        wrs32(POS_VEC + 0u, 100 << 12);
        wrs32(POS_VEC + 8u, 50 << 12);
        wr32(HDR + 0u, 4u);       /* 4 nodes */
        wr32(HDR + 4u, VERTS + 0x10u); /* base bumped for negative idx */
        /* node0: verts idx -2, 1, 2 -> accept (edges -5, 0, -1)
         * node1: idx 1, 2, 3 -> skip on first edge (+7)
         * node2: idx 1, 2, 3 -> skip on third edge (+9)
         * node3: idx 1, 2, 3 -> accept (all -1) */
        build_node(HDR, 0u, -2, 1, 2, 0xBEEF);
        build_node(HDR, 1u, 1, 2, 3, 0x1111);
        build_node(HDR, 2u, 1, 2, 3, 0x2222);
        build_node(HDR, 3u, 1, 2, 3, 0xCAFE);
        /* scripted vertex transforms; x low-u16 & z<<16 truncation probed
         * with negative x and z with high bits set */
        s_rt_script[0].sv_addr = VERTS + 0x10u + ((u32)(s32)-2 << 3);
        s_rt_script[0].mx = -3;          /* low u16 = 0xFFFD */
        s_rt_script[0].my = 999;
        s_rt_script[0].mz = 0x54321;     /* <<16 truncates to 0x4321xxxx */
        s_rt_script[1].sv_addr = VERTS + 0x10u + (1u << 3);
        s_rt_script[1].mx = 0x1234;
        s_rt_script[1].my = -1;
        s_rt_script[1].mz = -2;          /* <<16 = 0xFFFE0000 */
        s_rt_script[2].sv_addr = VERTS + 0x10u + (2u << 3);
        s_rt_script[2].mx = 7;
        s_rt_script[2].my = 0;
        s_rt_script[2].mz = 5;
        s_rt_script[3].sv_addr = VERTS + 0x10u + (3u << 3);
        s_rt_script[3].mx = 11;
        s_rt_script[3].my = 0;
        s_rt_script[3].mz = 13;
        s_rt_script_n = 4;
        /* NCLIP script: node0: -5, 0, -1 (accept; 0 is NOT >0)
         * node1: +7 (skip after 1)
         * node2: -1, -1, +9 (skip after 3)
         * node3: -1, -1, -1 (accept) */
        {
            static const s32 script[] = { -5, 0, -1, 7, -1, -1, 9,
                                          -1, -1, -1 };

            memcpy(s_nclip_script, script, sizeof(script));
            s_nclip_script_n = 10;
        }
        /* canary the candidate list */
        wr32(CAND_ID + 0u, 0x5A5A5A5Au);
        wr32(CAND_ID + 4u, 0x5A5A5A5Au);
        wr32(CAND_ID + 8u, 0x5A5A5A5Au);

        r = wm_80084DB8(POS_VEC, 0);
        check("C.ret", (u32)r, 4u);
        check("C.rt_calls", s_rt_calls, 12u);
        check("C.rt0_addr", s_rt_addrs[0],
              VERTS + 0x10u + ((u32)(s32)-2 << 3)); /* negative index */
        check("C.rt0_out", s_rt_outs[0], SC_V0);
        check("C.rt1_out", s_rt_outs[1], SC_V0 + 0x10u);
        check("C.rt2_out", s_rt_outs[2], SC_V0 + 0x20u);
        check("C.nclip_calls", s_nclip_calls, 10u);
        /* node0 pack expectations:
         * pV0 = (0x54321<<16)|(u16)-3 = 0x4321FFFD
         * pV1 = ((-2)<<16)|(0x1234) = 0xFFFE1234
         * pV2 = (5<<16)|7 = 0x00050007 */
        check("C.e1.a", s_nclip_args[0][0], 0x4321FFFDu);
        check("C.e1.b", s_nclip_args[0][1], 0xFFFE1234u);
        check("C.e1.p", s_nclip_args[0][2], point);
        check("C.e2.a", s_nclip_args[1][0], 0xFFFE1234u);
        check("C.e2.b", s_nclip_args[1][1], 0x00050007u);
        check("C.e3.a", s_nclip_args[2][0], 0x00050007u);
        check("C.e3.b", s_nclip_args[2][1], 0x4321FFFDu);
        /* candidate list: (0, 0xBEEF) then (3, 0xCAFE), stride 4 */
        check("C.cand0.id", (u32)rd16(CAND_ID + 0u), 0u);
        check("C.cand0.ty", (u32)rd16(CAND_ID + 2u), 0xBEEFu);
        check("C.cand1.id", (u32)rd16(CAND_ID + 4u), 3u);
        check("C.cand1.ty", (u32)rd16(CAND_ID + 6u), 0xCAFEu);
        check("C.cand2.canary", rd32(CAND_ID + 8u), 0x5A5A5A5Au);
        printf("C node loop ok\n");
    }

    /* ============ Section D: 84D00 scanner (scripted seam) ============ */
    {
        s32 r;

        /* 3 regions, flags 0/1/1; region1 misses, region2 hits with a
         * value whose s16 truncation matters (0x10002 -> 2). */
        reset();
        wr16(COUNT_ADDR, 3u);
        build_region(REGIONS + 0u * 0x54u, 0u, 0, 0, 0, HDR);
        build_region(REGIONS + 1u * 0x54u, 1u, 0, 0, 0, HDR);
        build_region(REGIONS + 2u * 0x54u, 1u, 0, 0, 0, HDR);
        s_84db8_script[0] = 0;
        s_84db8_script[1] = 0x10002;
        s_84db8_script_n = 2;
        wr32(OUT_ATTR, 0xA5A5A5A5u);
        r = wm_80084D00(POS_VEC, OUT_ATTR);
        check("D.ret", (u32)r, 2u);            /* sign16(0x10002) */
        check("D.calls", s_84db8_calls, 2u);
        check("D.idx0", (u32)s_84db8_idx[0], 1u);
        check("D.idx1", (u32)s_84db8_idx[1], 2u);
        check("D.attr", (u32)rd16(OUT_ATTR), 2u);
        check("D.attr_width", rd16(OUT_ATTR + 2u), 0xA5A5u); /* sh only */
        printf("D scanner ok\n");

        /* sign16-miss: 0x10000 truncates to 0 -> scan continues. */
        reset();
        wr16(COUNT_ADDR, 3u);
        s_84db8_script[0] = 0x10000;
        s_84db8_script[1] = 4;
        s_84db8_script_n = 2;
        r = wm_80084D00(POS_VEC, OUT_ATTR);
        check("D2.ret", (u32)r, 4u);
        check("D2.calls", s_84db8_calls, 2u);
        printf("D2 sign16-miss ok\n");

        /* count reload: first call shrinks the count global to 1 ->
         * the loop exits before probing region 2. */
        reset();
        wr16(COUNT_ADDR, 3u);
        build_region(REGIONS + 0u * 0x54u, 1u, 0, 0, 0, HDR);
        s_84db8_script[0] = 0;
        s_84db8_script[1] = 2; /* would hit if (wrongly) reached */
        s_84db8_script_n = 2;
        s_84db8_count_side_effect = 1;
        r = wm_80084D00(POS_VEC, OUT_ATTR);
        check("E.ret", (u32)r, 0u);
        check("E.calls", s_84db8_calls, 1u);
        printf("E count reload ok\n");

        /* empty region table */
        reset();
        wr16(COUNT_ADDR, 0u);
        r = wm_80084D00(POS_VEC, OUT_ATTR);
        check("F.ret", (u32)r, 0u);
        check("F.calls", s_84db8_calls, 0u);
        printf("F empty ok\n");
    }

    /* ============ Section G: 84D00 -> real 84DB8 integration ============ */
    {
        s32 r;

        reset();
        s_fwd_84db8 = 1;
        wr16(COUNT_ADDR, 1u);
        build_region(REGIONS, 1u, 30, 777, 90, HDR);
        wrs32(POS_VEC + 0u, 100 << 12);
        wrs32(POS_VEC + 8u, 50 << 12);
        wr32(HDR + 0u, 1u);
        wr32(HDR + 4u, VERTS + 0x10u);
        build_node(HDR, 0u, 1, 2, 3, 0x7777);
        s_rt_script[0].sv_addr = VERTS + 0x10u + (1u << 3);
        s_rt_script[0].mx = 1; s_rt_script[0].my = 0; s_rt_script[0].mz = 1;
        s_rt_script[1].sv_addr = VERTS + 0x10u + (2u << 3);
        s_rt_script[1].mx = 2; s_rt_script[1].my = 0; s_rt_script[1].mz = 2;
        s_rt_script[2].sv_addr = VERTS + 0x10u + (3u << 3);
        s_rt_script[2].mx = 3; s_rt_script[2].my = 0; s_rt_script[2].mz = 3;
        s_rt_script_n = 3;
        {
            static const s32 script[] = { -1, -1, 0 };

            memcpy(s_nclip_script, script, sizeof(script));
            s_nclip_script_n = 3;
        }
        r = wm_80084D00(POS_VEC, OUT_ATTR);
        check("G.ret", (u32)r, 2u);
        check("G.attr", (u32)rd16(OUT_ATTR), 0u);
        check("G.cand.id", (u32)rd16(CAND_ID + 0u), 0u);
        check("G.cand.ty", (u32)rd16(CAND_ID + 2u), 0x7777u);
        printf("G integration ok\n");
    }

    if (s_failures != 0) {
        fprintf(stderr, "FAILURES=%d\n", s_failures);
        return 1;
    }
    printf("W34B24-I5 0x80084D00 + 0x80084DB8 focused oracle PASS\n");
    return 0;
}
