/*
 * World-map path-node movement router 0x80095414 (keystone).
 *
 * Register-width-faithful transcription of retail world_map.bin
 * [0x80095414, 0x80095CD4).  See world_map_func_95414.h and the audit
 * pack docs/evidence/w34b24-pre-95414/ (JUMP_TABLE.csv / CFG.csv).
 *
 * AUDIT CORRECTION carried into this implementation: the pair-resolver
 * tie-break (mask == 3) computes  v = (typeA==0 ? 1:0) | (typeB==0 ? 2:0)
 * -- bit 1 is set when typeB == 0 (`bnez typeB` SKIPS the ori at
 * 0x80095A60/0x80095C10) -- the PRE JUMP_TABLE.csv prose said
 * "typeB != 0", which contradicts the instructions.  The instructions
 * win; the certificate exercises both tie-break arms.
 *
 * Dead code (compiler switch skeleton, documented not transcribed):
 * the mode-select value is strictly (D_8009C840 == -1) ? 1 : 3, so the
 * v1==0 arm at 0x80095598 (a fourth wm_800951A8 site) and the
 * return-uninitialized-$s5 arms are retail-unreachable.  v1==2 shares
 * the v1==3 branch target and needs no separate arm.  The case-1
 * preset s5 = -1 at 0x80095610 is dead (always overwritten at
 * 0x8009565C before any read).
 *
 * HOST ADAPTATIONS (each provably invisible to valid retail behavior):
 *  1. Frame slot: retail keeps a halfword attr slot at 0x18($sp) and
 *     passes its address to wm_80084D00.  The port's C frame is host
 *     memory, which guest-u32 helpers cannot address, so the slot is
 *     materialized at WM_95414_FRAME_ATTR inside the retail stack
 *     region of emulated RAM (above the heap ceiling 0x801FC000; the
 *     port never emulates the MIPS stack, so the region is otherwise
 *     dead).  Every retail 0x18($sp) access maps to this address with
 *     its exact width (sh / lh / lhu).
 *  2. Dispatch bound: retail's re-dispatch loop has no iteration
 *     limit; valid retail behavior always terminates through a
 *     terminal arm.  A generous cap (4096) that aborts loudly converts
 *     a would-be hang into a diagnosable stop without altering any
 *     terminating execution (accepted scheduler convention).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_93354.h"
#include "world_map_terrain_sampler.h"
#include "world_map_helper_951a8.h"
#include "world_map_helper_85760.h"
#include "world_map_helper_84d00.h"
#include "world_map_helper_85158.h"
#include "world_map_helper_85418.h"
#include "world_map_helper_952b0.h"
#include "world_map_helper_95324.h"
#include "world_map_func_95414.h"

/* ------------------------------------------------------------------ */
/* Test seams: route the nine callees + memory events.                 */
/* ------------------------------------------------------------------ */

#if defined(WM_95414_TEST_TRACE)
extern void wm_95414_test_93354(u32 vec_addr);
extern s32 wm_95414_test_93978(s32 x, s32 z);
extern s32 wm_95414_test_951a8(u32 b, u32 d, u32 o, s32 sc, s32 m);
extern s32 wm_95414_test_85760(u32 pos, u32 ws, s32 attr, s32 node);
extern s32 wm_95414_test_84d00(u32 pos, u32 out_attr);
extern u32 wm_95414_test_85158(u32 a, u32 b, u32 c, u32 attr, u32 node);
extern s32 wm_95414_test_85418(u32 a, s32 yoff, u32 attr, u32 node);
extern void wm_95414_test_952b0(u32 mov, u32 out, u32 ref);
extern void wm_95414_test_95324(u32 nrm, u32 mov, u32 out);
extern void wm_95414_test_store(u32 address, u32 value);
#define WM_95414_CALL_93354(a)              wm_95414_test_93354(a)
#define WM_95414_CALL_93978(x, z)           wm_95414_test_93978((x), (z))
#define WM_95414_CALL_951A8(b, d, o, s, m)  wm_95414_test_951a8((b), (d), (o), (s), (m))
#define WM_95414_CALL_85760(p, w, a, n)     wm_95414_test_85760((p), (w), (a), (n))
#define WM_95414_CALL_84D00(p, o)           wm_95414_test_84d00((p), (o))
#define WM_95414_CALL_85158(a, b, c, t, n)  wm_95414_test_85158((a), (b), (c), (t), (n))
#define WM_95414_CALL_85418(a, y, t, n)     wm_95414_test_85418((a), (y), (t), (n))
#define WM_95414_CALL_952B0(m, o, r)        wm_95414_test_952b0((m), (o), (r))
#define WM_95414_CALL_95324(nv, m, o)       wm_95414_test_95324((nv), (m), (o))
#define WM_95414_TRACE_STORE(a, v)          wm_95414_test_store((a), (v))
#else
#define WM_95414_CALL_93354(a)              wm_80093354(a)
#define WM_95414_CALL_93978(x, z)           wm_80093978((x), (z))
#define WM_95414_CALL_951A8(b, d, o, s, m)  wm_800951A8((b), (d), (o), (s), (m))
#define WM_95414_CALL_85760(p, w, a, n)     wm_80085760((p), (w), (a), (n))
#define WM_95414_CALL_84D00(p, o)           wm_80084D00((p), (o))
#define WM_95414_CALL_85158(a, b, c, t, n)  wm_80085158((a), (b), (c), (t), (n))
#define WM_95414_CALL_85418(a, y, t, n)     wm_80085418((a), (y), (t), (n))
#define WM_95414_CALL_952B0(m, o, r)        wm_800952B0((m), (o), (r))
#define WM_95414_CALL_95324(nv, m, o)       wm_80095324((nv), (m), (o))
#define WM_95414_TRACE_STORE(a, v)          ((void)0)
#endif

/* Retail layout. */
#define WM_95414_SC_PROJ     0x1F800060u  /* projected X / heading / Z */
#define WM_95414_SC_COEF     0x1F800070u  /* 85158 out block; +4 = height */
#define WM_95414_SC_NORM     0x1F800080u
#define WM_95414_SC_R30      0x1F800030u  /* 952B0 refs per redirect arm */
#define WM_95414_SC_R40      0x1F800040u
#define WM_95414_SC_R50      0x1F800050u
#define WM_95414_G_ATTR      0x8009C840u  /* cached attr (s32; -1 = none) */
#define WM_95414_G_NODE      0x8009C16Cu  /* cached node id */
#define WM_95414_G_RECTAB    0x8009C620u  /* region record table ptr */
#define WM_95414_G_CAND      0x8009D718u  /* candidate list, stride 4 */

/* Host adaptation 1 (header comment): the retail 0x18($sp) attr slot,
 * materialized in the dead retail-stack region of emulated RAM. */
#define WM_95414_FRAME_ATTR  0x801FFE18u

/* Host adaptation 2 (header comment): dispatch bound. */
#define WM_95414_DISPATCH_CAP 4096u

static u32 wm_95414_load_u32(u32 addr)
{
    u32 v;

    memcpy(&v, PSX_ADDR(addr), sizeof(v));
    return v;
}

static void wm_95414_store_u32(u32 addr, u32 v)
{
    memcpy(PSX_ADDR(addr), &v, sizeof(v));
    WM_95414_TRACE_STORE(addr, v);
}

static u16 wm_95414_load_u16(u32 addr)
{
    u16 v;

    memcpy(&v, PSX_ADDR(addr), sizeof(v));
    return v;
}

static s32 wm_95414_load_s16(u32 addr)
{
    return (s32)(s16)wm_95414_load_u16(addr);
}

static void wm_95414_store_u16(u32 addr, u16 v)
{
    memcpy(PSX_ADDR(addr), &v, sizeof(v));
    WM_95414_TRACE_STORE(addr, (u32)v);
}

static s32 wm_95414_as_s32(u32 b)
{
    s32 v;

    memcpy(&v, &b, sizeof(v));
    return v;
}

static u32 wm_95414_as_u32(s32 v)
{
    u32 b;

    memcpy(&b, &v, sizeof(b));
    return b;
}

/* Exact MIPS SRA on the 32-bit register. */
static u32 wm_95414_sra(u32 bits, u32 amount)
{
    u32 value = bits >> amount;

    if ((bits & 0x80000000u) != 0u)
        value |= UINT32_MAX << (32u - amount);
    return value;
}

static s32 wm_95414_sign16(u32 v)
{
    return (s32)(s16)(u16)(v & 0xFFFFu);
}

/* pos/dir projection term: mult + mflo low word, then sra 12. */
static u32 wm_95414_proj_term(u32 dir_word, s32 scale)
{
#if defined(WM_95414_MUTANT_WRONG_PROJECTION_SRA)
    return wm_95414_sra(dir_word * wm_95414_as_u32(scale), 11u);
#else
    return wm_95414_sra(dir_word * wm_95414_as_u32(scale), 12u);
#endif
}

/* Node record addressing: 14-byte records at nodes_plus8. */
static u32 wm_95414_node(u32 nodes_plus8, s32 id_s16)
{
    return nodes_plus8 + 14u * wm_95414_as_u32(id_s16);
}

/* Shared 951A8 fallback tail: call, then cache reset C16C, C840. */
static s32 wm_95414_fallback(u32 pos_vec, u32 dir_vec, u32 out_vec,
                             s32 scale, s32 mode)
{
    s32 r;

#if defined(WM_95414_MUTANT_SCALE_MODE_SWAP)
    r = WM_95414_CALL_951A8(pos_vec, dir_vec, out_vec, mode, scale);
#elif defined(WM_95414_MUTANT_MODE_MASKED)
    r = WM_95414_CALL_951A8(pos_vec, dir_vec, out_vec, scale,
                            wm_95414_as_s32((u32)mode & 0xFFFFu));
#else
    r = WM_95414_CALL_951A8(pos_vec, dir_vec, out_vec, scale, mode);
#endif
#if !defined(WM_95414_MUTANT_CACHE_NO_RESET)
    wm_95414_store_u32(WM_95414_G_NODE, 0xFFFFFFFFu);
    wm_95414_store_u32(WM_95414_G_ATTR, 0xFFFFFFFFu);
#endif
    return r;
}

s32 wm_80095414(u32 pos_vec, u32 dir_vec, u32 out_vec, s32 scale, s32 mode)
{
    /* ---------------- entry: projected position + headings -------- */
    {
        u32 px = wm_95414_load_u32(pos_vec + 0u) +
                 wm_95414_proj_term(wm_95414_load_u32(dir_vec + 0u), scale);
        u32 pz;
        s32 h;

        wm_95414_store_u32(WM_95414_SC_PROJ + 0u, px);
        pz = wm_95414_load_u32(pos_vec + 8u) +
             wm_95414_proj_term(wm_95414_load_u32(dir_vec + 8u), scale);
        wm_95414_store_u32(WM_95414_SC_PROJ + 8u, pz);
        WM_95414_CALL_93354(WM_95414_SC_PROJ);
        h = WM_95414_CALL_93978(
                wm_95414_as_s32(wm_95414_load_u32(WM_95414_SC_PROJ + 0u)),
                wm_95414_as_s32(wm_95414_load_u32(WM_95414_SC_PROJ + 8u)));
        wm_95414_store_u32(WM_95414_SC_PROJ + 4u,
                           wm_95414_as_u32(h) - 0x4000u);

        px = wm_95414_load_u32(pos_vec + 0u) +
             wm_95414_proj_term(wm_95414_load_u32(dir_vec + 0u), scale);
        wm_95414_store_u32(out_vec + 0u, px);
        pz = wm_95414_load_u32(pos_vec + 8u) +
             wm_95414_proj_term(wm_95414_load_u32(dir_vec + 8u), scale);
        /* retail: this store sits in the 93354 jal delay slot. */
        wm_95414_store_u32(out_vec + 8u, pz);
        WM_95414_CALL_93354(out_vec);
        h = WM_95414_CALL_93978(
                wm_95414_as_s32(wm_95414_load_u32(out_vec + 0u)),
                wm_95414_as_s32(wm_95414_load_u32(out_vec + 8u)));
        wm_95414_store_u32(out_vec + 4u, wm_95414_as_u32(h) - 0x4000u);
    }

    /* ---------------- mode select: (C840 == -1) ? case1 : case3 --- */
    if (wm_95414_load_u32(WM_95414_G_ATTR) == 0xFFFFFFFFu) {
        /* -------------- case 1: candidate search ------------------ */
        s32 s4;
        s32 s2;

        s4 = WM_95414_CALL_84D00(WM_95414_SC_PROJ, WM_95414_FRAME_ATTR);
        if (s4 == 0)
            return wm_95414_fallback(pos_vec, dir_vec, out_vec, scale, mode);

        /* loop 1: filter candidates via 85418 (retail presets s5=-1
         * here; dead, always overwritten before any read). */
        s2 = 0;
        if (s4 > 0) {
            s32 s1;
            u32 cand = WM_95414_G_CAND;

            for (s1 = 0; s1 < s4; s1 += 2) {
                s32 r = WM_95414_CALL_85418(
                            WM_95414_SC_PROJ, 0x70,
                            (u32)wm_95414_load_u16(WM_95414_FRAME_ATTR),
                            (u32)wm_95414_load_u16(cand));

                if (r == 0) {
                    wm_95414_store_u16(cand, 0xFFFFu);
#if defined(WM_95414_MUTANT_FILTER_STEP)
                    s2 += 1;
#else
                    s2 += 2;
#endif
                }
                cand += 4u;
            }
        }
        if (s2 == s4)
            return wm_95414_fallback(pos_vec, dir_vec, out_vec, scale, mode);

        /* loop 2: proximity scan (s5 = 0 entering; retail delay slot).
         * Retail bounds this loop by s4 too (blez guard + slt). */
        if (s4 > 0) {
            s32 s1;
            u32 idp = WM_95414_G_CAND;

            for (s1 = 0; s1 < s4; s1 += 2, idp += 4u) {
                s32 id = wm_95414_load_s16(idp);
                s32 fl;
                u32 height;
                u32 diff;

                if (id == -1)
                    continue;
                fl = wm_95414_load_s16(idp + 2u);
                if (fl == 1)
                    continue;
#if defined(WM_95414_MUTANT_ATTR_WIDTH)
                (void)WM_95414_CALL_85158(
                    WM_95414_SC_PROJ, WM_95414_SC_COEF, WM_95414_SC_NORM,
                    wm_95414_as_u32(wm_95414_load_s16(WM_95414_FRAME_ATTR)),
                    (u32)(u16)id);
#else
                (void)WM_95414_CALL_85158(
                    WM_95414_SC_PROJ, WM_95414_SC_COEF, WM_95414_SC_NORM,
                    (u32)wm_95414_load_u16(WM_95414_FRAME_ATTR),
                    (u32)(u16)id);
#endif
                height = wm_95414_load_u32(WM_95414_SC_COEF + 4u);
                /* diff = height - (pos[4] sra 12); bgez/negu abs;
                 * slti 0xB acceptance. */
                diff = height -
                       wm_95414_sra(wm_95414_load_u32(pos_vec + 4u), 12u);
                if (wm_95414_as_s32(diff) < 0)
                    diff = 0u - diff;
#if defined(WM_95414_MUTANT_PROX_BOUNDARY)
                if (wm_95414_as_s32(diff) < 12) {
#else
                if (wm_95414_as_s32(diff) < 11) {
#endif
                    /* accept: out[4] snap, then C840, then C16C
                     * (retail store order). */
#if defined(WM_95414_MUTANT_HEIGHT_SHIFT)
                    wm_95414_store_u32(out_vec + 4u, height << 11);
#else
                    wm_95414_store_u32(out_vec + 4u, height << 12);
#endif
#if defined(WM_95414_MUTANT_CACHE_SWAPPED)
                    wm_95414_store_u32(
                        WM_95414_G_ATTR,
                        wm_95414_as_u32(wm_95414_load_s16(idp)));
                    wm_95414_store_u32(
                        WM_95414_G_NODE,
                        wm_95414_as_u32(
                            wm_95414_load_s16(WM_95414_FRAME_ATTR)));
#else
                    wm_95414_store_u32(
                        WM_95414_G_ATTR,
                        wm_95414_as_u32(
                            wm_95414_load_s16(WM_95414_FRAME_ATTR)));
                    wm_95414_store_u32(
                        WM_95414_G_NODE,
                        wm_95414_as_u32(wm_95414_load_s16(idp)));
#endif
                    return 1;
                }
            }
        }

        /* no proximity hit: wall-tangent projection, return 0. */
        WM_95414_CALL_95324(WM_95414_SC_NORM, dir_vec, out_vec);
        return 0;
    }

    /* ------------------ case 3: cached-node graph walk ------------ */
    {
        u32 attr_raw = wm_95414_load_u32(WM_95414_G_ATTR);
        u32 nodes;      /* rec[0x44] + 8 (14-byte node records)       */
        u32 s0;         /* current node id (register-width u32)       */
        s32 s5 = 0;     /* retail: caller garbage until a terminal arm
                         * assigns it; every reachable return passes a
                         * terminal arm, so this init is unobservable */
        int s4 = 1;
        u32 guard = 0u;

        nodes = wm_95414_load_u32(
                    wm_95414_load_u32(WM_95414_G_RECTAB) +
                    84u * wm_95414_as_u32(wm_95414_sign16(attr_raw)) +
                    0x44u) + 8u;
        s0 = (u32)wm_95414_load_u16(WM_95414_G_NODE);
        /* retail: sh a0, 0x18(sp) — low half of the raw C840 word. */
        wm_95414_store_u16(WM_95414_FRAME_ATTR, (u16)attr_raw);

        while (s4) {
            u32 cls;

            /* Host adaptation 2 (header): bounded dispatch. */
            if (++guard > WM_95414_DISPATCH_CAP) {
                fprintf(stderr,
                        "[wm_80095414] dispatch cap exceeded (s0=0x%x)\n",
                        s0);
                abort();
            }

            cls = wm_95414_as_u32(WM_95414_CALL_85760(
                      pos_vec, WM_95414_SC_PROJ,
                      wm_95414_load_s16(WM_95414_FRAME_ATTR),
                      wm_95414_sign16(s0)));
            /* sltiu guard: unsigned compare of the raw class. */
            if (cls >= 8u)
                continue;

#if defined(WM_95414_MUTANT_JT_SLOT_SWAP)
            if (cls == 1u)
                cls = 2u;
            else if (cls == 2u)
                cls = 1u;
#endif
            switch (cls) {
            case 0u: { /* land on node */
                u32 height;

                s5 = 1;
#if !defined(WM_95414_MUTANT_S4_NOT_CLEARED)
                s4 = 0;
#endif
                (void)WM_95414_CALL_85158(
                    WM_95414_SC_PROJ, WM_95414_SC_COEF, WM_95414_SC_NORM,
                    (u32)wm_95414_load_u16(WM_95414_FRAME_ATTR),
                    s0 & 0xFFFFu);
                /* retail store order: C16C, out[4], C840. */
                wm_95414_store_u32(WM_95414_G_NODE,
                                   wm_95414_as_u32(wm_95414_sign16(s0)));
                height = wm_95414_load_u32(WM_95414_SC_COEF + 4u);
#if defined(WM_95414_MUTANT_HEIGHT_SHIFT)
                wm_95414_store_u32(out_vec + 4u, height << 11);
#else
                wm_95414_store_u32(out_vec + 4u, height << 12);
#endif
                wm_95414_store_u32(
                    WM_95414_G_ATTR,
                    wm_95414_as_u32(
                        wm_95414_load_s16(WM_95414_FRAME_ATTR)));
                break;
            }
            case 1u:
            case 2u:
            case 4u: { /* single-link followers */
#if defined(WM_95414_MUTANT_LINK_OFFSET)
                static const u32 link_off[3] = { 6u, 8u, 8u };
#else
                static const u32 link_off[3] = { 6u, 8u, 0xAu };
#endif
                static const u32 ref_off[3] = {
                    WM_95414_SC_R30, WM_95414_SC_R40, WM_95414_SC_R50
                };
                u32 k = (cls == 1u) ? 0u : ((cls == 2u) ? 1u : 2u);
                u32 node = wm_95414_node(nodes, wm_95414_sign16(s0));
                s32 link = wm_95414_load_s16(node + link_off[k]);
                u32 ntype;

                if (link == -1) {
                    s5 = wm_95414_fallback(pos_vec, dir_vec, out_vec,
                                           scale, mode);
                    s4 = 0;
                    break;
                }
                ntype = (u32)wm_95414_load_u16(
                            wm_95414_node(nodes, link) + 0xCu);
                /* retail delay slot: s0 = link on BOTH outcomes. */
                s0 = wm_95414_as_u32(link);
#if defined(WM_95414_MUTANT_TYPE_POLARITY)
                if (ntype == 1u)
                    break;              /* keep walking */
#else
                if (ntype != 1u)
                    break;              /* keep walking */
#endif
                s5 = 0;
                s4 = 0;
                WM_95414_CALL_952B0(dir_vec, out_vec, ref_off[k]);
                break;
            }
            case 3u:
            case 5u:
            case 6u: { /* pair resolvers */
                static const u32 offA[3] = { 6u, 6u, 8u };
                static const u32 offB[3] = { 8u, 0xAu, 0xAu };
                u32 k = (cls == 3u) ? 0u : ((cls == 5u) ? 1u : 2u);
                /* redirect scratch by link position:
                 * +6 -> 0x30, +8 -> 0x40, +0xA -> 0x50 */
                u32 refA = (offA[k] == 6u) ? WM_95414_SC_R30
                                           : WM_95414_SC_R40;
                u32 refB = (offB[k] == 8u) ? WM_95414_SC_R40
                                           : WM_95414_SC_R50;
                u32 node = wm_95414_node(nodes, wm_95414_sign16(s0));
                s32 la = wm_95414_load_s16(node + offA[k]);
                s32 lb = wm_95414_load_s16(node + offB[k]);
#if defined(WM_95414_MUTANT_PAIR_MASK)
                u32 mask = ((la != -1) ? 1u : 0u) |
                           ((lb == -1) ? 2u : 0u);
#else
                u32 mask = ((la != -1) ? 1u : 0u) |
                           ((lb != -1) ? 2u : 0u);
#endif

                if (mask == 0u) {
                    s5 = wm_95414_fallback(pos_vec, dir_vec, out_vec,
                                           scale, mode);
                    s4 = 0;
                    break;
                }
                if (mask == 1u) {
                    u32 ta = (u32)wm_95414_load_u16(
                                 wm_95414_node(nodes, la) + 0xCu);

                    s0 = wm_95414_as_u32(la);   /* delay slot */
                    if (ta != 1u)
                        break;                  /* keep walking */
                    s5 = 0;
                    s4 = 0;
                    WM_95414_CALL_952B0(dir_vec, out_vec, refA);
                    break;
                }
                if (mask == 2u) {
                    u32 tb = (u32)wm_95414_load_u16(
                                 wm_95414_node(nodes, lb) + 0xCu);

                    s0 = wm_95414_as_u32(lb);   /* delay slot */
                    if (tb != 1u)
                        break;                  /* keep walking */
                    s5 = 0;
                    s4 = 0;
                    WM_95414_CALL_952B0(dir_vec, out_vec, refB);
                    break;
                }
                /* mask == 3: type tie-break.
                 * v = (typeA==0 ? 1:0) | (typeB==0 ? 2:0). */
                {
                    u32 ta = (u32)wm_95414_load_u16(
                                 wm_95414_node(nodes, la) + 0xCu);
                    u32 tb = (u32)wm_95414_load_u16(
                                 wm_95414_node(nodes, lb) + 0xCu);
#if defined(WM_95414_MUTANT_TIEBREAK_TYPEB_POLARITY)
                    u32 v = ((ta == 0u) ? 1u : 0u) |
                            ((tb != 0u) ? 2u : 0u);
#else
                    u32 v = ((ta == 0u) ? 1u : 0u) |
                            ((tb == 0u) ? 2u : 0u);
#endif

                    if (v == 0u) {
                        /* dead-end: zero out vec, order +8,+4,+0. */
                        s5 = 0;
                        s4 = 0;
#if defined(WM_95414_MUTANT_ZERO_ORDER)
                        wm_95414_store_u32(out_vec + 0u, 0u);
                        wm_95414_store_u32(out_vec + 4u, 0u);
                        wm_95414_store_u32(out_vec + 8u, 0u);
#else
                        wm_95414_store_u32(out_vec + 8u, 0u);
                        wm_95414_store_u32(out_vec + 4u, 0u);
                        wm_95414_store_u32(out_vec + 0u, 0u);
#endif
                        break;
                    }
                    if (v == 2u) {
                        s0 = wm_95414_as_u32(lb);   /* continue */
                        break;
                    }
                    /* v == 1 or v == 3: re-read the follow link
                     * through the CURRENT node (lhu, zero-extended):
                     * slot 3 follows +6; slots 5/6 follow +8. */
#if defined(WM_95414_MUTANT_TIEBREAK_FOLLOW_SWAP)
                    s0 = (u32)wm_95414_load_u16(
                             wm_95414_node(nodes, wm_95414_sign16(s0)) +
                             ((cls == 3u) ? 8u : 6u));
#else
                    s0 = (u32)wm_95414_load_u16(
                             wm_95414_node(nodes, wm_95414_sign16(s0)) +
                             ((cls == 3u) ? 6u : 8u));
#endif
                    break;
                }
            }
            default:   /* slot 7: no-op, re-dispatch */
                break;
            }
        }
        return s5;
    }
}
