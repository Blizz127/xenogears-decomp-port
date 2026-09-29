/*
 * Focused production-linked oracle for the W34B24-I6 8A72C dependency
 * pack: wm_80097770, wm_800941C4, wm_80094238, wm_8008BEC8, wm_8008C1DC.
 *
 * Expected values are hand-derived from the retail disassembly (slice
 * SHAs gated by the run script).  Callee seams are recording,
 * test-controlled implementations following the accepted pattern; the
 * production link uses the real canonical helpers.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_97770.h"
#include "world_map_helper_941c4.h"
#include "world_map_helper_94238.h"
#include "world_map_helper_8bec8.h"
#include "world_map_helper_8c1dc.h"

#define POOL_BASE  0x800C0000u
#define TAB_BASE   0x800C1000u
#define REC_BASE   0x800C2000u
#define VEC_A      0x800C3000u
#define VEC_B      0x800C3100u
#define OUT_VEC    0x800C3200u
#define ANG_OUT    0x800C3300u
#define OBJ_BASE   0x800C4000u
#define CTX_OUT    0x800C5000u
#define POS_VEC    0x800C7000u
#define CANARY     0xC5C5C5C5u

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

static u32 rd32(u32 a) { u32 v; memcpy(&v, PSX_ADDR(a), 4); return v; }
static u16 rd16(u32 a) { u16 v; memcpy(&v, PSX_ADDR(a), 2); return v; }
static void wr32(u32 a, u32 v) { memcpy(PSX_ADDR(a), &v, 4); }
static void wr16(u32 a, u16 v) { memcpy(PSX_ADDR(a), &v, 2); }
static void wrs32(u32 a, s32 v) { u32 b; memcpy(&b, &v, 4); wr32(a, b); }
static void wrs16(u32 a, s32 v) { wr16(a, (u16)(s16)v); }
static s32 S(u32 b) { s32 v; memcpy(&v, &b, 4); return v; }

/* ------------------------- seams / traces ---------------------------- */

typedef struct { u32 a; u32 v; u32 w; } Ev;
static Ev s_ev[16];
static u32 s_nev;
static void ev_reset(void) { s_nev = 0; }
static void ev_push(u32 a, u32 v, u32 w)
{
    if (s_nev < 16u) { s_ev[s_nev].a = a; s_ev[s_nev].v = v; s_ev[s_nev].w = w; }
    s_nev++;
}
void wm_97770_test_store(u32 a, u32 v, u32 w) { ev_push(a, v, w); }
void wm_94238_test_store(u32 a, u32 v, u32 w) { ev_push(a, v, w); }
void wm_8c1dc_test_store(u32 a, u32 v) { ev_push(a, v, 2u); }

/* 941C4 trig seams. */
static long s_ratan2_ret, s_rcos_ret, s_rsin_ret;
static long s_ratan2_y, s_ratan2_x, s_rcos_arg, s_rsin_arg;
static u32 s_seq, s_ratan2_seq, s_rcos_seq, s_rsin_seq;
long wm_941c4_test_ratan2(long y, long x)
{
    s_ratan2_seq = ++s_seq; s_ratan2_y = y; s_ratan2_x = x;
    return s_ratan2_ret;
}
long wm_941c4_test_rcos(long a)
{
    s_rcos_seq = ++s_seq; s_rcos_arg = a; return s_rcos_ret;
}
long wm_941c4_test_rsin(long a)
{
    s_rsin_seq = ++s_seq; s_rsin_arg = a; return s_rsin_ret;
}

/* 8BEC8 seams: 93354 records only (wrap isolated by its own accepted
 * certificate); 93978 records args and returns a forced angle. */
static u32 s_93354_calls, s_93354_arg;
static s32 s_93978_ret, s_93978_x, s_93978_z;
static u32 s_93978_calls;
void wm_8bec8_test_93354(u32 a) { s_93354_calls++; s_93354_arg = a; }
s32 wm_8bec8_test_93978(s32 x, s32 z)
{
    s_93978_calls++; s_93978_x = x; s_93978_z = z; return s_93978_ret;
}

/* 8C1DC seams. */
static s32 s_93f18_ret;
static u32 s_93f18_arg;
s32 wm_8c1dc_test_93f18(u32 a) { s_93f18_arg = a; return s_93f18_ret; }
static u32 s_89160_calls, s_89160_a0, s_89160_a1, s_89160_a2;
void wm_8c1dc_test_89160(u32 a0, u32 a1, u32 a2)
{
    s_89160_calls++; s_89160_a0 = a0; s_89160_a1 = a1; s_89160_a2 = a2;
}
static u32 s_894c8_calls, s_894c8_a0;
void wm_8c1dc_test_894c8(u32 a0) { s_894c8_calls++; s_894c8_a0 = a0; }

/* ------------------------------------------------------------------ */
/* Section A: 97770 pool claim                                         */
/* ------------------------------------------------------------------ */

static void section_a(void)
{
    s32 r;

    wr32(0x8009BE24u, POOL_BASE);
    /* slot 0: free (occ +4 == 0, +2 nonzero garbage, +6 canary). */
    wr16(POOL_BASE + 0u, 0x7777u);
    wr16(POOL_BASE + 2u, 0x0BADu);
    wr16(POOL_BASE + 4u, 0u);
    wr16(POOL_BASE + 6u, 0xBEEFu);
    ev_reset();
    r = wm_80097770(0u, 0x12345);
    check_s32("A.claim.ret", r, 1);
    check_u32("A.claim.mark", (u32)rd16(POOL_BASE + 0u), 1u);
    check_u32("A.claim.value", (u32)rd16(POOL_BASE + 4u), 0x2345u);
    check_u32("A.claim.canary6", (u32)rd16(POOL_BASE + 6u), 0xBEEFu);
    check_u32("A.claim.stores", s_nev, 2u);
    check_u32("A.claim.store0", s_ev[0].a, POOL_BASE + 0u);
    check_u32("A.claim.store1", s_ev[1].a, POOL_BASE + 4u);

    /* slot 3 (stride 128): free; negative claim value. */
    wr16(POOL_BASE + 0x180u + 0u, 0u);
    wr16(POOL_BASE + 0x180u + 4u, 0u);
    ev_reset();
    r = wm_80097770(3u, -2);
    check_s32("A.stride.ret", r, 1);
    check_u32("A.stride.addr", s_ev[0].a, POOL_BASE + 0x180u);
    check_u32("A.stride.value", (u32)rd16(POOL_BASE + 0x180u + 4u), 0xFFFEu);

    /* busy: occ == 5. */
    wr16(POOL_BASE + 0x80u + 0u, 0x1111u);
    wr16(POOL_BASE + 0x80u + 4u, 5u);
    ev_reset();
    r = wm_80097770(1u, 9);
    check_s32("A.busy.ret", r, 0);
    check_u32("A.busy.stores", s_nev, 0u);
    check_u32("A.busy.mark_untouched", (u32)rd16(POOL_BASE + 0x80u), 0x1111u);

    /* busy: occ == -1 (negative halfword). */
    wrs16(POOL_BASE + 0x100u + 4u, -1);
    ev_reset();
    r = wm_80097770(2u, 9);
    check_s32("A.busyneg.ret", r, 0);
    printf("A 97770 ok\n");
}

/* ------------------------------------------------------------------ */
/* Section B: 941C4 perpendicular heading                              */
/* ------------------------------------------------------------------ */

static void run_941c4(s32 ax, s32 az, s32 bx, s32 bz, long t_ret,
                      u32 want_ang, long c_ret, long s_ret,
                      const char *tag)
{
    char name[96];
    s32 r;

    s_seq = 0;
    s_ratan2_ret = t_ret;
    s_rcos_ret = c_ret;
    s_rsin_ret = s_ret;
    wrs32(VEC_A + 0u, ax);
    wrs32(VEC_A + 8u, az);
    wrs32(VEC_B + 0u, bx);
    wrs32(VEC_B + 8u, bz);
    wr32(OUT_VEC + 0u, CANARY);
    wr32(OUT_VEC + 8u, CANARY);
    wr16(ANG_OUT, 0xAAAAu);

    r = wm_800941C4(VEC_A, VEC_B, OUT_VEC, ANG_OUT);

    snprintf(name, sizeof(name), "B.%s.ratan2_y", tag);
    check_s32(name, (s32)s_ratan2_y, S((u32)bz - (u32)az));
    snprintf(name, sizeof(name), "B.%s.ratan2_x", tag);
    check_s32(name, (s32)s_ratan2_x, S((u32)bx - (u32)ax));
    snprintf(name, sizeof(name), "B.%s.angle", tag);
    check_u32(name, (u32)rd16(ANG_OUT), want_ang);
    snprintf(name, sizeof(name), "B.%s.rcos_arg", tag);
    check_s32(name, (s32)s_rcos_arg, S(want_ang));
    snprintf(name, sizeof(name), "B.%s.rsin_arg", tag);
    check_s32(name, (s32)s_rsin_arg, S(want_ang));
    snprintf(name, sizeof(name), "B.%s.order", tag);
    check_u32(name,
              (u32)(s_ratan2_seq == 1u && s_rcos_seq == 2u && s_rsin_seq == 3u),
              1u);
    snprintf(name, sizeof(name), "B.%s.outx", tag);
    check_s32(name, S(rd32(OUT_VEC + 0u)), (s32)c_ret);
    snprintf(name, sizeof(name), "B.%s.outz", tag);
    check_u32(name, rd32(OUT_VEC + 8u), 0u - (u32)(s32)s_ret);
    snprintf(name, sizeof(name), "B.%s.ret", tag);
    check_u32(name, (u32)r, 0u - (u32)(s32)s_ret);
    printf("B %s ok\n", tag);
}

static void section_b(void)
{
    run_941c4(1000, 2000, 1300, 2400, 0, 0x400u, 0x123, 0x456, "basic");
    /* bias wrap: (0xD00 + 0x400) & 0xFFF = 0x100. */
    run_941c4(0, 0, 5, 5, 0xD00, 0x100u, -7, 9, "bias_wrap");
    /* negative ratan2: (-1 + 0x400) & 0xFFF = 0x3FF. */
    run_941c4(0, 0, -5, -5, -1, 0x3FFu, 0, (long)(s32)0x80000000, "neg_atan");
    /* subtraction wrap: B - A wraps in 32 bits. */
    run_941c4((s32)0x80000000, 0, 0x7FFFFFFF, 0, 0x800, 0xC00u, 1, 1, "wrap_sub");
}

/* ------------------------------------------------------------------ */
/* Section C: 94238 rect lookup                                        */
/* ------------------------------------------------------------------ */

static void build_rects(void)
{
    /* rec0: x0=10 z0=20 w=5 h=6 marker=1 id=0x1234 type=4 */
    wrs16(REC_BASE + 0x00u, 10);
    wrs16(REC_BASE + 0x02u, 20);
    wrs16(REC_BASE + 0x04u, 5);
    wrs16(REC_BASE + 0x06u, 6);
    wrs16(REC_BASE + 0x08u, 1);
    wr16(REC_BASE + 0x0Cu, 0x1234u);
    wrs16(REC_BASE + 0x0Eu, 4);
    /* rec1: x0=200 z0=300 w=1 h=1 marker=1 id=0x00FF type=2 */
    wrs16(REC_BASE + 0x10u, 200);
    wrs16(REC_BASE + 0x12u, 300);
    wrs16(REC_BASE + 0x14u, 1);
    wrs16(REC_BASE + 0x16u, 1);
    wrs16(REC_BASE + 0x18u, 1);
    wr16(REC_BASE + 0x1Cu, 0x00FFu);
    wrs16(REC_BASE + 0x1Eu, 2);
    /* terminator */
    wrs16(REC_BASE + 0x28u, -1);
    /* zero-origin record list (for the srl test), separate list. */
    wrs16(REC_BASE + 0x100u + 0x00u, 0);
    wrs16(REC_BASE + 0x100u + 0x02u, 0);
    wrs16(REC_BASE + 0x100u + 0x04u, 0);
    wrs16(REC_BASE + 0x100u + 0x06u, 0);
    wrs16(REC_BASE + 0x100u + 0x08u, 1);
    wr16(REC_BASE + 0x100u + 0x0Cu, 0x0077u);
    wrs16(REC_BASE + 0x100u + 0x0Eu, 1);
    wrs16(REC_BASE + 0x100u + 0x18u, -1);
    /* empty list */
    wrs16(REC_BASE + 0x200u + 8u, -1);
    /* pointer table */
    wr32(0x8009BD00u, TAB_BASE);
    wr32(TAB_BASE + 0u, REC_BASE);
    wr32(TAB_BASE + 4u, REC_BASE + 0x100u);
    wr32(TAB_BASE + 8u, REC_BASE + 0x200u);
}

static void run_94238(s32 px, s32 pz, u32 idx, s32 want_ret, u32 want_ptr,
                      u32 want_ida, u32 want_idb, const char *tag)
{
    char name[96];
    s32 r;

    wrs32(POS_VEC + 0u, px);
    wrs32(POS_VEC + 8u, pz);
    wr32(0x8009D7D8u, CANARY);
    wr16(0x8009BD24u, 0x5A5Au);
    wr16(0x8009CE68u, 0x5A5Au);
    ev_reset();
    r = wm_80094238(POS_VEC, idx);
    snprintf(name, sizeof(name), "C.%s.ret", tag);
    check_s32(name, r, want_ret);
    snprintf(name, sizeof(name), "C.%s.ptr", tag);
    check_u32(name, rd32(0x8009D7D8u), want_ptr);
    snprintf(name, sizeof(name), "C.%s.ida", tag);
    check_u32(name, (u32)rd16(0x8009BD24u), want_ida);
    snprintf(name, sizeof(name), "C.%s.idb", tag);
    check_u32(name, (u32)rd16(0x8009CE68u), want_idb);
    printf("C %s ok\n", tag);
}

static void section_c(void)
{
    build_rects();
    /* type-4 hit at exact lower bound (inclusive). */
    run_94238(10 << 12, 20 << 12, 0u, 1, 0xFFFFFFFFu, 0xFFFFu, 0x1234u,
              "hit4_low");
    /* inclusive upper bound: (x0+w, z0+h). */
    run_94238(15 << 12, 26 << 12, 0u, 1, 0xFFFFFFFFu, 0xFFFFu, 0x1234u,
              "hit4_high");
    /* second record, type != 4: ptr = record guest address. */
    run_94238(200 << 12, 300 << 12, 0u, 1, REC_BASE + 0x10u, 0x00FFu,
              0xFFFFu, "hit2_second");
    /* miss after scanning both records. */
    run_94238(16 << 12, 20 << 12, 0u, 0, 0xFFFFFFFFu, 0xFFFFu, 0xFFFFu,
              "miss_scan");
    /* empty list: first marker == -1. */
    run_94238(10 << 12, 20 << 12, 2u, 0, 0xFFFFFFFFu, 0xFFFFu, 0xFFFFu,
              "miss_empty");
    /* Wrapped-negative position: pos.x = 0x80000000 -> cx =
     * (0x80000000>>12)&0xFFFF = 0 -> hits the zero-origin record. */
    run_94238((s32)0x80000000, 0, 1u, 1, REC_BASE + 0x100u, 0x0077u,
              0xFFFFu, "srl_neg");
    /* andi 0xFFFF is load-bearing: pos.x = 0x12345678 -> masked cell
     * 0x2345 hits; the unmasked shifted value 0x12345 would miss. */
    wrs16(REC_BASE + 0x100u + 0x00u, 0x2345);
    wrs16(REC_BASE + 0x100u + 0x02u, 0);
    run_94238(0x12345678, 0, 1u, 1, REC_BASE + 0x100u, 0x0077u,
              0xFFFFu, "coord_mask");
    wrs16(REC_BASE + 0x100u + 0x00u, 0);
}

/* ------------------------------------------------------------------ */
/* Section D: 8BEC8 approach stepper                                   */
/* ------------------------------------------------------------------ */

static void obj_setup(s32 px, s32 pz, s32 tx, s32 tz, s32 vx, s32 vz,
                      u16 step)
{
    wrs32(OBJ_BASE + 0x28u, px);
    wr32(OBJ_BASE + 0x2Cu, CANARY);
    wrs32(OBJ_BASE + 0x30u, pz);
    wrs32(OBJ_BASE + 0x38u, vx);
    wrs32(OBJ_BASE + 0x40u, vz);
    wr16(OBJ_BASE + 0x4Au, step);
    wrs32(OBJ_BASE + 0x50u, tx);
    wrs32(OBJ_BASE + 0x54u, tz);
    s_93354_calls = 0;
    s_93978_calls = 0;
    s_93978_ret = 0x777;
}

static void section_d(void)
{
    s32 r;

    /* both close (d == 0 / d == 4): no movement, mask 3. */
    obj_setup(100 << 12, (50 << 12) + 7, 100, 54, 0x9999, 0x9999, 8u);
    r = wm_8008BEC8(OBJ_BASE);
    check_s32("D.close.ret", r, 3);
    check_s32("D.close.posx", S(rd32(OBJ_BASE + 0x28u)), 100 << 12);
    check_u32("D.close.93354calls", s_93354_calls, 1u);
    check_u32("D.close.93354arg", s_93354_arg, OBJ_BASE + 0x28u);
    check_u32("D.close.angle", rd32(OBJ_BASE + 0x2Cu), 0x777u);
    check_s32("D.close.978x", s_93978_x, 100 << 12);
    check_s32("D.close.978z", s_93978_z, (50 << 12) + 7);

    /* X far (d == 10), Z close: step 5 -> half 2 -> +vel*2; mask 2. */
    obj_setup(10 << 12, 50 << 12, 20, 50, 0x1000, 0x2000, 5u);
    r = wm_8008BEC8(OBJ_BASE);
    check_s32("D.xfar.ret", r, 2);
    check_s32("D.xfar.posx", S(rd32(OBJ_BASE + 0x28u)), (10 << 12) + 0x2000);
    check_s32("D.xfar.posz", S(rd32(OBJ_BASE + 0x30u)), 50 << 12);
    check_s32("D.xfar.978x", s_93978_x, (10 << 12) + 0x2000);

    /* negative distance -> abs; d == 10 far. */
    obj_setup(30 << 12, 50 << 12, 20, 50, 0x1000, 0, 4u);
    r = wm_8008BEC8(OBJ_BASE);
    check_s32("D.negd.ret", r, 2);
    check_s32("D.negd.posx", S(rd32(OBJ_BASE + 0x28u)), (30 << 12) + 0x2000);

    /* threshold: d == 5 is FAR (slti d,5), d == 4 is close. */
    obj_setup(0, 0, 5, 4, 0x1000, 0x1000, 2u);
    r = wm_8008BEC8(OBJ_BASE);
    check_s32("D.thresh.ret", r, 2); /* X d=5 far, Z d=4 close */
    check_s32("D.thresh.posx", S(rd32(OBJ_BASE + 0x28u)), 0x1000);

    /* odd negative step: -5 -> half -2 (toward zero). */
    obj_setup(0, 0, 100, 0, 0x1000, 0, 0xFFFBu);
    r = wm_8008BEC8(OBJ_BASE);
    check_s32("D.negstep.ret", r, 2);
    check_s32("D.negstep.posx", S(rd32(OBJ_BASE + 0x28u)),
              S(0u - 0x2000u));

    /* low-word wrap: vel 0x40000000 * half 4 -> low32 0. */
    obj_setup(0, 0, 100, 0, 0x40000000, 0, 8u);
    r = wm_8008BEC8(OBJ_BASE);
    check_s32("D.wrap.posx", S(rd32(OBJ_BASE + 0x28u)), 0);
    printf("D 8BEC8 ok\n");
}

/* ------------------------------------------------------------------ */
/* Section E: 8C1DC class dispatch                                     */
/* ------------------------------------------------------------------ */

static void section_e(void)
{
    u32 i;

    wrs32(OBJ_BASE + 0x28u, 0x12345678);
    wrs32(OBJ_BASE + 0x2Cu, -0x1000);
    wrs32(OBJ_BASE + 0x30u, 0x3000);
    wr32(OBJ_BASE + 0x5Cu, 0xABCD5678u);
    for (i = 0u; i <= 0xEu; i += 2u)
        wr16(CTX_OUT + 0xA0u + i, 0x7C7Cu);

    /* class 3: full store set, order, and 89160 dispatch. */
    s_93f18_ret = 3;
    s_89160_calls = 0;
    s_894c8_calls = 0;
    ev_reset();
    wm_8008C1DC(0x77u, OBJ_BASE, CTX_OUT);
    check_u32("E.c3.f18arg", s_93f18_arg, OBJ_BASE + 0x28u);
    check_u32("E.c3.stores", s_nev, 6u);
    check_u32("E.c3.o0", s_ev[0].a, CTX_OUT + 0xA0u);
    check_u32("E.c3.o1", s_ev[1].a, CTX_OUT + 0xA2u);
    check_u32("E.c3.o2", s_ev[2].a, CTX_OUT + 0xACu);
    check_u32("E.c3.o3", s_ev[3].a, CTX_OUT + 0xA8u);
    check_u32("E.c3.o4", s_ev[4].a, CTX_OUT + 0xA4u);
    check_u32("E.c3.o5", s_ev[5].a, CTX_OUT + 0xAAu);
    check_u32("E.c3.a0", (u32)rd16(CTX_OUT + 0xA0u), 0x2345u);
    check_u32("E.c3.a2", (u32)rd16(CTX_OUT + 0xA2u), 0xFFFFu);
    check_u32("E.c3.a4", (u32)rd16(CTX_OUT + 0xA4u), 3u);
    check_u32("E.c3.a8", (u32)rd16(CTX_OUT + 0xA8u), 0u);
    check_u32("E.c3.aa", (u32)rd16(CTX_OUT + 0xAAu), 0x5678u);
    check_u32("E.c3.ac", (u32)rd16(CTX_OUT + 0xACu), 0u);
    check_u32("E.c3.calls160", s_89160_calls, 1u);
    check_u32("E.c3.160a0", s_89160_a0, 0x77u);
    check_u32("E.c3.160a1", s_89160_a1, CTX_OUT + 0xA0u);
    check_u32("E.c3.160a2", s_89160_a2, CTX_OUT + 0xA8u);
    check_u32("E.c3.no894c8", s_894c8_calls, 0u);

    /* sign16 of the 93F18 result: 0x10003 -> 3. */
    s_93f18_ret = 0x10003;
    s_89160_calls = 0;
    ev_reset();
    wm_8008C1DC(0x11u, OBJ_BASE, CTX_OUT);
    check_u32("E.sign16.calls160", s_89160_calls, 1u);

    /* class 2: only 894C8(ctx), no stores. */
    s_93f18_ret = 2;
    s_89160_calls = 0;
    s_894c8_calls = 0;
    ev_reset();
    wm_8008C1DC(0x55u, OBJ_BASE, CTX_OUT);
    check_u32("E.c2.stores", s_nev, 0u);
    check_u32("E.c2.calls894c8", s_894c8_calls, 1u);
    check_u32("E.c2.894a0", s_894c8_a0, 0x55u);
    check_u32("E.c2.no160", s_89160_calls, 0u);
    printf("E 8C1DC ok\n");
}

int main(void)
{
    PsxMemory_Init();

    section_a();
    section_b();
    section_c();
    section_d();
    section_e();

    if (s_failures != 0) {
        fprintf(stderr, "FAILURES=%d\n", s_failures);
        return 1;
    }
    printf("W34B24-I6 8A72C dependency pack focused oracle PASS\n");
    return 0;
}
