/*
 * Integration / certification test for world-map dispatcher 0x80094A5C.
 *
 * Builds with -DWM_94A5C_TEST_TRACE -DWM_94A5C_TEST_HOOK so the unit under
 * test routes every helper call (tail helpers + four private probes) and the
 * external GTE probe 0x8004A70C through controllable test symbols.  We verify:
 *   - t0 = (Xcell, Zcell) classification drives exactly the documented helper
 *     selection (single-helper cases and complex-case probe-sign routing),
 *   - call order and early-return semantics of the two-helper pairs,
 *   - the per-case base/workspace mask writes (s1[0], s1[2]) in the no-copy path,
 *   - the common tail (return 1 + copy when 94060 passes, return 0 when blocked).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_func_94a5c.h"

#define BASE_ADDR UINT32_C(0x800A1000)
#define DIR_ADDR  UINT32_C(0x800A1100)
#define WS        0x1F800000u

/* Helper ids (mirror world_map_func_94a5c.c HID_*). */
enum { HID_9443C = 1, HID_945C8 = 2, HID_94750 = 3, HID_948D8 = 4 };

typedef struct { int stage; int hid; int v; } Ev;
static Ev    s_ev[16];   /* trace events only (unused for ordering) */
static int   s_evn;
static Ev    s_hev[16];  /* helper-call events only */
static int   s_hevn;
static s32   s_probe_result;   /* value returned by the four private probes */
static int   s_4a70c_calls;
static s32   s_4a70c_ret;      /* value returned by the GTE probe mock */
static int   s_tail_93354;
static int   s_tail_93f18;
static s32   s_93f18_code;
static s32   s_94060_ret;
static int   s_failures;

static void fail(const char *name, const char *what, s64 got, s64 exp)
{
    if (got == exp)
        return;
    fprintf(stderr, "ASSERTION %s %s: got=%lld expected=%lld\n",
            name, what, (long long)got, (long long)exp);
    s_failures++;
}

static u32 load_u32(u32 addr)
{
    u32 v;
    memcpy(&v, PSX_ADDR(addr), sizeof(v));
    return v;
}

static void store_u32(u32 addr, u32 v)
{
    memcpy(PSX_ADDR(addr), &v, sizeof(v));
}

static s32 as_s32(u32 b)
{
    s32 v;
    memcpy(&v, &b, sizeof(v));
    return v;
}

static void record_helper(int hid)
{
    if (s_hevn < 16) {
        s_hev[s_hevn].stage = 0;
        s_hev[s_hevn].hid = hid;
        s_hev[s_hevn].v = 0;
    }
    s_hevn++;
}

static void trace_cb(int stage, int hid, int v)
{
    if (s_evn < 16) {
        s_ev[s_evn].stage = stage;
        s_ev[s_evn].hid = hid;
        s_ev[s_evn].v = v;
    }
    s_evn++;
}

static s32 probe_mock(s32 a0, s32 a1, s32 a2, s32 a3)
{
    (void)a0; (void)a1; (void)a2; (void)a3;
    s_4a70c_calls++;
    return s_4a70c_ret;
}

s32 func_8004A70C(s32 a0, s32 a1, s32 a2, s32 a3)
{
    return probe_mock(a0, a1, a2, a3);
}

void wm_private_test_93354(u32 addr) { (void)addr; s_tail_93354++; }
s32 wm_private_test_93f18(u32 addr) { (void)addr; s_tail_93f18++; return s_93f18_code; }
s32 wm_private_test_94060(s32 mode, s32 code) { (void)mode; (void)code; return s_94060_ret; }
s32 wm_private_test_9443C(u32 b, u32 d, u32 w, s32 m) { (void)b;(void)d;(void)w;(void)m; record_helper(HID_9443C); return s_probe_result; }
s32 wm_private_test_945C8(u32 b, u32 d, u32 w, s32 m) { (void)b;(void)d;(void)w;(void)m; record_helper(HID_945C8); return s_probe_result; }
s32 wm_private_test_94750(u32 b, u32 d, u32 w, s32 m) { (void)b;(void)d;(void)w;(void)m; record_helper(HID_94750); return s_probe_result; }
s32 wm_private_test_948D8(u32 b, u32 d, u32 w, s32 m) { (void)b;(void)d;(void)w;(void)m; record_helper(HID_948D8); return s_probe_result; }

static void reset(void)
{
    memset(g_PsxRam, 0xA5, PSX_RAM_SIZE);
    memset(g_PsxScratchpad, 0x5A, 4096u);
    memset(s_ev, 0, sizeof(s_ev));
    memset(s_hev, 0, sizeof(s_hev));
    s_evn = 0;
    s_hevn = 0;
    s_4a70c_calls = 0;
    s_probe_result = 0;
    s_4a70c_ret = 0;
    s_tail_93354 = 0;
    s_tail_93f18 = 0;
    s_93f18_code = as_s32(UINT32_C(0x00007FFF));
    s_94060_ret = 0;
}

/* Pick base/dir/scale so the new position ends in the desired X/Z >>19 cell.
 * When `near` is set, the X move sits one unit below a cell boundary with a
 * tiny delta, so >>12 keeps it in-cell while >>11 crosses (kills SRA mutants). */
static void set_move(int dx, int dz, int near)
{
    u32 bx, bz, dxv, dzv;
    if (dx == 1)      { bx = near ? 0x7FFFEu : 0x7FFFFu; dxv = near ? 0x1000u : 0x2000u; }
    else if (dx == -1){ bx = 0x80001u; dxv = 0xFFFFE000u; }
    else              { bx = 0x40000u; dxv = 0u; }
    if (dz == 1)      { bz = 0x7FFFFu; dzv = 0x2000u; }
    else if (dz == -1){ bz = 0x80001u; dzv = 0xFFFFE000u; }
    else              { bz = 0x40000u; dzv = 0u; }
    store_u32(BASE_ADDR + 0, bx);
    store_u32(BASE_ADDR + 8, bz);
    store_u32(DIR_ADDR + 0, dxv);
    store_u32(DIR_ADDR + 8, dzv);
}

static u32 exp_mask0(int case_id, u32 baseX)
{
    u32 hi = baseX & 0xFFF80000u;
    if (case_id == 5 || case_id == 9) return hi | 0x00080000u;
    return hi;
}
static u32 exp_mask2(int case_id, u32 baseZ)
{
    u32 hi = baseZ & 0xFFF80000u;
    if (case_id == 5 || case_id == 6) return hi | 0x00080000u;
    return hi;
}

/* Since the W34B24-I4 NormalClip rebinding, the production TU's default
 * probe references the real native NormalClip; this focused build links
 * without PsyCross, so provide the retail NCLIP reference (s64 cross-sum
 * of the packed s16 screen points truncated to the low word — the same
 * formula PsyCross's integer path implements).  Every test case installs
 * probe_mock, so this body also serves as a canary: it must never run. */
long NormalClip(long sxy0, long sxy1, long sxy2)
{
    s32 x0 = (s32)(s16)((u32)sxy0 & 0xFFFFu), y0 = (s32)((u32)sxy0) >> 16;
    s32 x1 = (s32)(s16)((u32)sxy1 & 0xFFFFu), y1 = (s32)((u32)sxy1) >> 16;
    s32 x2 = (s32)(s16)((u32)sxy2 & 0xFFFFu), y2 = (s32)((u32)sxy2) >> 16;
    long long sum = (long long)x0 * y1 + (long long)x1 * y2 +
                    (long long)x2 * y0 - (long long)x0 * y2 -
                    (long long)x1 * y0 - (long long)x2 * y1;

    fprintf(stderr, "ASSERTION default_probe_reached: NormalClip called "
                    "despite probe_mock installation\n");
    exit(1);
    return (long)(s32)(u32)(sum & 0xFFFFFFFFll);
}

int main(void)
{
    PsxMemory_Init();
    wm_80094A5C_set_trace(trace_cb);
    wm_80094A5C_set_probe(probe_mock);

    struct { int dx, dz, t0, near; } moves[] = {
        { 0,  0,  0, 0}, { 1,  0,  1, 0}, { -1, 0,  2, 0},
        { 0,  1,  4, 0}, { 0, -1,  8, 0}, { 1,  1,  5, 0},
        { -1, 1,  6, 0}, { 1, -1,  9, 0}, { -1,-1, 10, 0},
        { 1,  0,  0, 1},
    };

    struct { int case_id; int v; int first; int second; } route[] = {
        {5,-1, HID_9443C, HID_94750}, {5,0, HID_9443C, 0}, {5,1, HID_94750, HID_9443C},
        {6,-1, HID_94750, HID_945C8}, {6,0, HID_945C8, 0}, {6,1, HID_945C8, HID_94750},
        {9,-1, HID_948D8, HID_9443C}, {9,0, HID_9443C, 0}, {9,1, HID_9443C, HID_948D8},
        {10,-1,HID_945C8, HID_948D8}, {10,0,HID_945C8, 0}, {10,1,HID_948D8, HID_945C8},
    };

    for (size_t mi = 0; mi < sizeof(moves)/sizeof(moves[0]); mi++) {
        int dx = moves[mi].dx, dz = moves[mi].dz, t0 = moves[mi].t0;
        char nm[48];
        snprintf(nm, sizeof(nm), "t0=%d", t0);

        int single = (t0 == 1 || t0 == 2 || t0 == 4 || t0 == 8);
        int complex = (t0 == 5 || t0 == 6 || t0 == 9 || t0 == 10);
        int exp_first_single =
            (t0 == 1) ? HID_9443C : (t0 == 2) ? HID_945C8 :
            (t0 == 4) ? HID_94750 : HID_948D8;

        /* A: helper returns nonzero -> immediate return, no tail copy */
        reset(); set_move(dx, dz, moves[mi].near);
        s_probe_result = 2; s_4a70c_ret = 1; s_94060_ret = 0;
        s32 retA = wm_80094A5C(BASE_ADDR, DIR_ADDR, 1, as_s32(UINT32_C(0x00001234)));
        if (single) {
            fail(nm, "A-helper-count", s_hevn, 1);
            fail(nm, "A-helper", s_hev[0].hid, exp_first_single);
            fail(nm, "A-return", retA, 2);
            fail(nm, "A-tail-skipped", s_tail_93354, 0);
        } else if (complex) {
            fail(nm, "A-4a70c", s_4a70c_calls, 1);
            fail(nm, "A-helper-count", s_hevn, 1);
            int ef = 0;
            for (size_t r = 0; r < sizeof(route)/sizeof(route[0]); r++)
                if (route[r].case_id == t0 && route[r].v == 1) ef = route[r].first;
            fail(nm, "A-first", s_hev[0].hid, ef);
            fail(nm, "A-return", retA, 2);
            /* mask preserved (no tail copy) */
            u32 bx = load_u32(BASE_ADDR + 0);
            u32 bz = load_u32(BASE_ADDR + 8);
            fail(nm, "A-mask0", (s64)load_u32(WS + 0), (s64)exp_mask0(t0, bx));
            fail(nm, "A-mask2", (s64)load_u32(WS + 8), (s64)exp_mask2(t0, bz));
        } else {
            fail(nm, "A-4a70c-skipped", s_4a70c_calls, 0);
            fail(nm, "A-tail-93354", s_tail_93354, 1);
            fail(nm, "A-tail-93f18", s_tail_93f18, 1);
            fail(nm, "A-return", retA, 1);
            for (u32 i = 0; i < 4u; i++)
                fail(nm, "A-copy", (s64)load_u32(WS + i*4u),
                     (s64)load_u32(WS + 0x10u + i*4u));
        }

        /* B: helper returns 0 (v==0 for complex) -> tail runs, 94060 passes */
        reset(); set_move(dx, dz, moves[mi].near);
        s_probe_result = 0; s_4a70c_ret = 0; s_94060_ret = 0;
        s32 retB = wm_80094A5C(BASE_ADDR, DIR_ADDR, 1, as_s32(UINT32_C(0x00001234)));
        if (single) {
            fail(nm, "B-return", retB, 1);
            fail(nm, "B-tail-93354", s_tail_93354, 1);
            fail(nm, "B-tail-93f18", s_tail_93f18, 1);
            for (u32 i = 0; i < 4u; i++)
                fail(nm, "B-copy", (s64)load_u32(WS + i*4u),
                     (s64)load_u32(WS + 0x10u + i*4u));
        } else if (complex) {
            int ef = 0;
            for (size_t r = 0; r < sizeof(route)/sizeof(route[0]); r++)
                if (route[r].case_id == t0 && route[r].v == 0) ef = route[r].first;
            fail(nm, "B-first", s_hev[0].hid, ef);
            fail(nm, "B-helper-count", s_hevn, 1);
            fail(nm, "B-tail-93354", s_tail_93354, 1);
            fail(nm, "B-return", retB, 1);
        } else {
            fail(nm, "B-return", retB, 1);
            fail(nm, "B-tail-93354", s_tail_93354, 1);
        }

        /* C: tail blocked by 94060 -> return 0, no copy */
        reset(); set_move(dx, dz, moves[mi].near);
        s_probe_result = 0; s_4a70c_ret = 0; s_94060_ret = as_s32(UINT32_C(0x12345678));
        s32 retC = wm_80094A5C(BASE_ADDR, DIR_ADDR, 1, as_s32(UINT32_C(0x00001234)));
        if (t0 == 0 || single || complex) {
            fail(nm, "C-return", retC, 0);
            fail(nm, "C-tail-93354", s_tail_93354, 1);
            fail(nm, "C-tail-93f18", s_tail_93f18, 1);
        }

        /* D: v<0 / v>0 routing & order (both helpers return 0) */
        if (complex) {
            for (int vsign = -1; vsign <= 1; vsign += 2) {
                reset(); set_move(dx, dz, moves[mi].near);
                s_probe_result = 0; s_4a70c_ret = vsign; s_94060_ret = 0;
                wm_80094A5C(BASE_ADDR, DIR_ADDR, 1, as_s32(UINT32_C(0x00001234)));
                int ef = 0, es = 0;
                for (size_t r = 0; r < sizeof(route)/sizeof(route[0]); r++)
                    if (route[r].case_id == t0 && route[r].v == vsign) {
                        ef = route[r].first; es = route[r].second;
                    }
                char vn[16];
                snprintf(vn, sizeof(vn), "D%d", vsign);
                fail(nm, vn, s_hev[0].hid, ef);
                if (es != 0) {
                    fail(nm, vn, s_hev[1].hid, es);
                    fail(nm, vn, s_hevn, 2);
                } else {
                    fail(nm, vn, s_hevn, 1);
                }
            }
        }
    }

    if (s_failures != 0) {
        fprintf(stderr, "ASSERTION failures=%d\n", s_failures);
        return 1;
    }
    printf("W34B22-I5 0x80094A5C PASS cases=9 routing+tail verified\n");
    return 0;
}
