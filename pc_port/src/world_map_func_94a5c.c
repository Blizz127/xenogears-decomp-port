/*
 * World-map collision dispatcher 0x80094A5C.
 *
 * Register-width-faithful transcription of retail world_map.bin
 * [0x80094A5C, 0x800951A8).  See world_map_func_94a5c.h.
 */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_93354.h"
#include "world_map_helper_93f18.h"
#include "world_map_helper_94060.h"
#include "world_map_private_collision.h"
#include "world_map_func_94a5c.h"

/* Retail probe 0x8004A70C IS PsyQ NormalClip (named at that address in
 * config/symbol_addrs.slus_006.64.txt; SLUS body = mtc2 SXY0/SXY2/SXY1,
 * NCLIP, mfc2 MAC0 — see docs/evidence/w34b24-pre2-84d00-84db8/
 * A70C_CONTRACT.md).  ABI at the retail call sites here:
 *   $a0 = (Z_boundary<<16) | (X_boundary & 0xFFFF)   -> sxy0
 *   $a1 = 0                                          -> sxy1
 *   $a2 = (Z_new<<16) | (X_new & 0xFFFF)             -> sxy2
 *   $a3 = mode; NormalClip never reads $a3 (provably unused)
 *   $v0 = MAC0 = low 32 bits of the 2x signed 2D triangle area; its
 *         sign selects the X-probe vs Z-probe order.
 * The port binds the real native NormalClip (PsyCross GTE: integer
 * path, s64 cross-sum truncated to the low word by F() — verified
 * wrap, not saturate).  The 4-arg probe shape is kept so the
 * WM_94A5C_TEST_HOOK seam stays stable. */
extern long NormalClip(long sxy0, long sxy1, long sxy2);

static s32 wm_94a5c_normal_clip_probe(s32 a0, s32 a1, s32 a2, s32 a3)
{
    (void)a3; /* retail $a3 is dead in NormalClip */
    return (s32)NormalClip(a0, a1, a2);
}

/* ------------------------------------------------------------------ */
/* Test seam: when WM_94A5C_TEST_TRACE is defined, route every helper   */
/* call (tail helpers + the four private probes) through controllable   */
/* test symbols.  Retail builds use the real world-map helpers.          */
/* ------------------------------------------------------------------ */

#if defined(WM_94A5C_TEST_TRACE)
extern void wm_private_test_93354(u32 addr);
extern s32 wm_private_test_93f18(u32 addr);
extern s32 wm_private_test_94060(s32 mode, s32 code);
extern s32 wm_private_test_9443C(u32 base_vec, u32 direction_vec, u32 workspace, s32 mode);
extern s32 wm_private_test_945C8(u32 base_vec, u32 direction_vec, u32 workspace, s32 mode);
extern s32 wm_private_test_94750(u32 base_vec, u32 direction_vec, u32 workspace, s32 mode);
extern s32 wm_private_test_948D8(u32 base_vec, u32 direction_vec, u32 workspace, s32 mode);
#define WM_TAIL_93354(a)   wm_private_test_93354(a)
#define WM_TAIL_93F18(a)   wm_private_test_93f18(a)
#define WM_TAIL_94060(m,c) wm_private_test_94060((m), (c))
#define WM_PROBE_9443C     wm_private_test_9443C
#define WM_PROBE_945C8     wm_private_test_945C8
#define WM_PROBE_94750     wm_private_test_94750
#define WM_PROBE_948D8     wm_private_test_948D8
#else
#define WM_TAIL_93354(a)   wm_80093354(a)
#define WM_TAIL_93F18(a)   wm_80093F18(a)
#define WM_TAIL_94060(m,c) wm_80094060((m), (c))
#define WM_PROBE_9443C     wm_8009443C
#define WM_PROBE_945C8     wm_800945C8
#define WM_PROBE_94750     wm_80094750
#define WM_PROBE_948D8     wm_800948D8
#endif

/* ------------------------------------------------------------------ */
/* Bit-width-faithful helpers (mirror the private-collision helpers).  */
/* ------------------------------------------------------------------ */

static u32 load_u32(u32 addr)
{
    u32 v;
    memcpy(&v, PSX_ADDR(addr), 4);
    return v;
}

static void store_u32(u32 addr, u32 v)
{
    memcpy(PSX_ADDR(addr), &v, 4);
}

static void store_u16(u32 addr, u16 v)
{
    memcpy(PSX_ADDR(addr), &v, 2);
}

static void store_s16(u32 addr, s32 v)
{
    store_u16(addr, (u16)(s16)v);
}

static s32 as_s32(u32 b)
{
    s32 v;
    memcpy(&v, &b, sizeof(v));
    return v;
}

static u32 as_u32(s32 v)
{
    u32 b;
    memcpy(&b, &v, sizeof(b));
    return b;
}

/* Arithmetic shift right on the 32-bit value (matches MIPS sra). */
static u32 sra(u32 b, u32 amt)
{
    u32 v = b >> amt;
    if (b & 0x80000000u) {
        v |= ~((1u << (32u - amt)) - 1u);
    }
    return v;
}

static s32 s32_sra(s32 v, u32 amt)
{
    return as_s32(sra(as_u32(v), amt));
}

/* Sign-extend the low 16 bits (matches lhu + sll/sra). */
static s32 sign16(s32 v)
{
    return (s32)(s16)(u16)((u32)v & 0xFFFFu);
}

#define WS 0x1F800000u

/* Private-probe helper identifiers, used only by the test trace seam. */
#define HID_9443C 1
#define HID_945C8 2
#define HID_94750 3
#define HID_948D8 4

#if defined(WM_94A5C_TEST_HOOK)
static wm_94a5c_probe_fn wm_94a5c_probe = wm_94a5c_normal_clip_probe;
static wm_94a5c_trace_fn wm_94a5c_trace = 0;

#define WM_94A5C_PROBE(v40, a1, v48, mode) \
    wm_94a5c_probe((v40), (a1), (v48), (mode))
#define WM_94A5C_TRACE(stage, hid, v) \
    do { if (wm_94a5c_trace) wm_94a5c_trace((stage), (hid), (v)); } while (0)

void wm_80094A5C_set_probe(wm_94a5c_probe_fn fn)
{
    wm_94a5c_probe = fn ? fn : wm_94a5c_normal_clip_probe;
}

void wm_80094A5C_set_trace(wm_94a5c_trace_fn fn)
{
    wm_94a5c_trace = fn;
}
#else
#define WM_94A5C_PROBE(v40, a1, v48, mode) \
    wm_94a5c_normal_clip_probe((v40), (a1), (v48), (mode))
#define WM_94A5C_TRACE(stage, hid, v) ((void)0)
#endif

/* ------------------------------------------------------------------ */
/* Per-case base/workspace mask (retail 0x80094BF0/0x80094D4C/         */
/* 0x80094E84/0x80094FD8).                                            */
/* ------------------------------------------------------------------ */

#if defined(WM_94A5C_MUTANT_WRONG_MASK)
#define MASK_AND 0xFFF00000u
#else
#define MASK_AND 0xFFF80000u
#endif

static void set_case5_mask(u32 base)
{
    u32 b0 = load_u32(base + 0);
    u32 b2 = load_u32(base + 8);
    store_u32(WS + 0, (b0 & MASK_AND) | 0x00080000u);
    store_u32(WS + 8, (b2 & MASK_AND) | 0x00080000u);
}

static void set_case6_mask(u32 base)
{
    u32 b0 = load_u32(base + 0);
    u32 b2 = load_u32(base + 8);
    store_u32(WS + 0, b0 & MASK_AND);
    store_u32(WS + 8, (b2 & MASK_AND) | 0x00080000u);
}

static void set_case9_mask(u32 base)
{
    u32 b0 = load_u32(base + 0);
    u32 b2 = load_u32(base + 8);
    store_u32(WS + 0, (b0 & MASK_AND) | 0x00080000u);
    store_u32(WS + 8, b2 & MASK_AND);
}

static void set_case10_mask(u32 base)
{
    u32 b0 = load_u32(base + 0);
    u32 b2 = load_u32(base + 8);
    store_u32(WS + 0, b0 & MASK_AND);
    store_u32(WS + 8, b2 & MASK_AND);
}

/* ------------------------------------------------------------------ */
/* Delta + GTE probe block (retail 0x80094C1C..0x80094CA4 and its      */
/* three identical twins).  Reads WS[0], WS[2], WS[0x10], WS[0x18],     */
/* base[0], base[8]; writes WS[0x20..0x48]; returns the signed probe   */
/* result used to pick the X-probe vs Z-probe pair order.              */
/* ------------------------------------------------------------------ */

static s32 probe_v(u32 base, s32 mode)
{
    s32 d20 = s32_sra(as_s32((u32)(load_u32(base + 0) - load_u32(WS + 0))), 12);
    store_u32(WS + 0x20, as_u32(d20));

    s32 d30 = s32_sra(as_s32((u32)(load_u32(WS + 0x10) - load_u32(WS + 0))), 12);
    store_u32(WS + 0x30, as_u32(d30));

    s32 d38 = s32_sra(as_s32((u32)(load_u32(WS + 0x18) - load_u32(WS + 2))), 12);
    store_u32(WS + 0x38, as_u32(d38));

    s32 d28 = s32_sra(as_s32((u32)(load_u32(base + 8) - load_u32(WS + 2))), 12);
    store_u32(WS + 0x28, as_u32(d28));

    store_u32(WS + 0x44, 0u);

    u32 v40 = ((u32)as_u32(d28) << 16) | (as_u32(d20) & 0xFFFFu);
    u32 v48 = ((u32)as_u32(d38) << 16) | (as_u32(d30) & 0xFFFFu);
    store_u32(WS + 0x40, v40);
    store_u32(WS + 0x48, v48);

    return WM_94A5C_PROBE((s32)v40, 0, (s32)v48, mode);
}

/* ------------------------------------------------------------------ */
/* Common tail (retail 0x80095124).                                   */
/* ------------------------------------------------------------------ */

static s32 tail(s32 a2, s32 mode)
{
    if (a2 != 0) {
        return a2;
    }

    u32 s = WS + 0x10;
    WM_TAIL_93354(s);
    s32 c = WM_TAIL_93F18(s);
    s32 res = WM_TAIL_94060(sign16(mode), sign16(c));
    if (res != 0) {
        return 0;
    }

#if !defined(WM_94A5C_MUTANT_TAIL_NOCOPY)
    store_u32(WS + 0, load_u32(WS + 0x10));
    store_u32(WS + 4, load_u32(WS + 0x14));
    store_u32(WS + 8, load_u32(WS + 0x18));
    store_u32(WS + 0xc, load_u32(WS + 0x1c));
#endif
    return 1;
}

/* ------------------------------------------------------------------ */
/* Complex-case helper-pair dispatch.  `a`/`b` are the two helper ids  */
/* (HID_*) and the probe result `v` selects the order.                 */
/* ------------------------------------------------------------------ */

/* Two probe-sign routing patterns recovered from the retail dispatcher:
 *   pattern 1 (cases 5/10): v<0 -> A then B; v==0 -> A; v>0 -> B then A
 *   pattern 2 (cases 6/9):  v<0 -> B then A; v==0 -> A; v>0 -> A then B */
static s32 complex_dispatch(u32 base, u32 dir, s32 m, s32 v, int pat1,
                            s32 (*a)(u32, u32, u32, s32), s32 hid_a,
                            s32 (*b)(u32, u32, u32, s32), s32 hid_b)
{
    (void)hid_a;
    (void)hid_b;
    s32 r;
    if (v == 0) {
        WM_94A5C_TRACE(0, hid_a, v);
        r = a(base, dir, WS, m);
    } else {
        int first_a = pat1 ? (v < 0) : (v > 0);
        if (first_a) {
            WM_94A5C_TRACE(0, hid_a, v);
            r = a(base, dir, WS, m);
            if (r != 0) {
                return r;
            }
            WM_94A5C_TRACE(1, hid_b, v);
            r = b(base, dir, WS, m);
        } else {
            WM_94A5C_TRACE(0, hid_b, v);
            r = b(base, dir, WS, m);
            if (r != 0) {
                return r;
            }
            WM_94A5C_TRACE(1, hid_a, v);
            r = a(base, dir, WS, m);
        }
    }
    return r;
}

s32 wm_80094A5C(u32 base_vec, u32 direction_vec, s32 scale, s32 mode)
{
    u32 base = base_vec;
    u32 dir = direction_vec;

    s32 dirX = as_s32(load_u32(dir + 0));
    s32 baseX = as_s32(load_u32(base + 0));
    s32 dirZ = as_s32(load_u32(dir + 8));
    s32 baseZ = as_s32(load_u32(base + 8));

    u32 prodX = (u32)((u32)as_u32(dirX) * (u32)as_u32(scale));
#if defined(WM_94A5C_MUTANT_SRA11)
    s32 newX = baseX + as_s32(sra(prodX, 11));
#else
    s32 newX = baseX + as_s32(sra(prodX, 12));
#endif
    u32 prodZ = (u32)((u32)as_u32(dirZ) * (u32)as_u32(scale));
#if defined(WM_94A5C_MUTANT_SRA11)
    s32 newZ = baseZ + as_s32(sra(prodZ, 11));
#else
    s32 newZ = baseZ + as_s32(sra(prodZ, 12));
#endif

    store_u32(WS + 0x10, as_u32(newX));
    store_u32(WS + 0x18, as_u32(newZ));

    s32 newXhi = s32_sra(newX, 19);
    s32 baseXhi = s32_sra(baseX, 19);
    s32 newZhi = s32_sra(newZ, 19);
    s32 baseZhi = s32_sra(baseZ, 19);

    store_s16(WS + 0xa0, sign16(baseXhi));
    store_s16(WS + 0xa8, sign16(newXhi));
    store_s16(WS + 0xa4, sign16(baseZhi));
    store_s16(WS + 0xac, sign16(newZhi));

    s32 t0 = 0;
    if (newXhi < baseXhi) {
        t0 |= 2;
    } else if (baseXhi < newXhi) {
        t0 |= 1;
    }
    if (newZhi < baseZhi) {
        t0 |= 8;
    } else if (baseZhi < newZhi) {
        t0 |= 4;
    }

    s32 m = sign16(mode);

#if defined(WM_94A5C_MUTANT_SWAP_PATTERN)
#define PAT5_10 0
#define PAT6_9  1
#else
#define PAT5_10 1
#define PAT6_9  0
#endif

#if defined(WM_94A5C_MUTANT_THRESHOLD10)
    if (t0 >= 10) {
#else
    if (t0 >= 11) {
#endif
        return tail(0, mode);
    }

    s32 a2 = 0;
    switch (t0) {
    case 0:
    case 3:
    case 7:
        a2 = 0;
        break;
    case 1:
        WM_94A5C_TRACE(0, HID_9443C, 0);
        a2 = WM_PROBE_9443C(base, dir, WS, m);
        break;
    case 2:
        WM_94A5C_TRACE(0, HID_945C8, 0);
        a2 = WM_PROBE_945C8(base, dir, WS, m);
        break;
    case 4:
        WM_94A5C_TRACE(0, HID_94750, 0);
        a2 = WM_PROBE_94750(base, dir, WS, m);
        break;
    case 8:
        WM_94A5C_TRACE(0, HID_948D8, 0);
        a2 = WM_PROBE_948D8(base, dir, WS, m);
        break;
    case 5:
        set_case5_mask(base);
        a2 = complex_dispatch(base, dir, m, probe_v(base, mode), PAT5_10,
                              WM_PROBE_9443C, HID_9443C,
                              WM_PROBE_94750, HID_94750);
        break;
    case 6:
        set_case6_mask(base);
        a2 = complex_dispatch(base, dir, m, probe_v(base, mode), PAT6_9,
                              WM_PROBE_945C8, HID_945C8,
                              WM_PROBE_94750, HID_94750);
        break;
    case 9:
        set_case9_mask(base);
        a2 = complex_dispatch(base, dir, m, probe_v(base, mode), PAT6_9,
                              WM_PROBE_9443C, HID_9443C,
                              WM_PROBE_948D8, HID_948D8);
        break;
    case 10:
        set_case10_mask(base);
        a2 = complex_dispatch(base, dir, m, probe_v(base, mode), PAT5_10,
                              WM_PROBE_945C8, HID_945C8,
                              WM_PROBE_948D8, HID_948D8);
        break;
    default:
        a2 = 0;
        break;
    }

    return tail(a2, mode);
}
