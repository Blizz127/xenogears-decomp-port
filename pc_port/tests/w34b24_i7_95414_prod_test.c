/*
 * Focused production-linked oracle for retail world keystone 0x80095414.
 *
 * All nine callees are routed through scripted, recording seams;
 * expectations are hand-derived from the retail disassembly (entry
 * projection via an s64-truncation reference; case-1 exits incl. exact
 * proximity boundaries; every jump-table slot, pair mask, and tie-break
 * arm; cache-global transition orders).  Seam script overruns are hard
 * failures, so a mutant that loops or re-dispatches unexpectedly dies
 * deterministically.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_func_95414.h"

#define POS_VEC   0x800C8000u
#define DIR_VEC   0x800C8100u
#define OUT_VEC   0x800C8200u
#define RECTAB    0x800C0000u   /* region record table base */
#define NODESBLK  0x800C1000u   /* rec[0x44] target */
#define CANARY    0x77777777u

#define SC_PROJ   0x1F800060u
#define SC_COEF   0x1F800070u
#define SC_NORM   0x1F800080u
#define G_ATTR    0x8009C840u
#define G_NODE    0x8009C16Cu
#define G_RECTAB  0x8009C620u
#define G_CAND    0x8009D718u
#define FRAME_ATTR 0x801FFE18u

static int s_failures;

static void check(const char *name, u32 got, u32 want)
{
    if (got != want) {
        fprintf(stderr, "ASSERTION %s: got=0x%08x expected=0x%08x\n",
                name, got, want);
        if (++s_failures > 30) {
            fprintf(stderr, "too many failures\n");
            exit(1);
        }
    }
}

static u32 rd(u32 a) { u32 v; memcpy(&v, PSX_ADDR(a), 4); return v; }
static void wr(u32 a, u32 v) { memcpy(PSX_ADDR(a), &v, 4); }
static void wr16(u32 a, u16 v) { memcpy(PSX_ADDR(a), &v, 2); }
static u16 rd16(u32 a) { u16 v; memcpy(&v, PSX_ADDR(a), 2); return v; }

/* ------------------------- call/event log --------------------------- */

typedef struct {
    char kind[8];
    u32 a, b, c, d, e;
} Ev;

static Ev s_log[64];
static u32 s_nlog;

static void logev(const char *k, u32 a, u32 b, u32 c, u32 d, u32 e)
{
    if (s_nlog < 64u) {
        snprintf(s_log[s_nlog].kind, sizeof(s_log[s_nlog].kind), "%s", k);
        s_log[s_nlog].a = a; s_log[s_nlog].b = b; s_log[s_nlog].c = c;
        s_log[s_nlog].d = d; s_log[s_nlog].e = e;
    }
    s_nlog++;
}

static const Ev *ev(u32 i)
{
    if (i >= s_nlog || i >= 64u) {
        fprintf(stderr, "ASSERTION eventlog: index %u >= count %u\n",
                i, s_nlog);
        exit(1);
    }
    return &s_log[i];
}

static void expect_ev(const char *name, u32 i, const char *kind,
                      u32 a, u32 b, u32 c, u32 d, u32 e)
{
    const Ev *v = ev(i);
    char nm[96];

    snprintf(nm, sizeof(nm), "%s.kind", name);
    if (strcmp(v->kind, kind) != 0) {
        fprintf(stderr, "ASSERTION %s: got=%s expected=%s\n", nm, v->kind,
                kind);
        s_failures++;
        return;
    }
    snprintf(nm, sizeof(nm), "%s.a", name); check(nm, v->a, a);
    snprintf(nm, sizeof(nm), "%s.b", name); check(nm, v->b, b);
    snprintf(nm, sizeof(nm), "%s.c", name); check(nm, v->c, c);
    snprintf(nm, sizeof(nm), "%s.d", name); check(nm, v->d, d);
    snprintf(nm, sizeof(nm), "%s.e", name); check(nm, v->e, e);
}

/* --------------------------- seam scripts --------------------------- */

static s32 s_93978_script[8];
static u32 s_93978_n, s_93978_i;
static s32 s_84d00_ret;
static u16 s_84d00_attr;
static s32 s_85418_script[8];
static u32 s_85418_n, s_85418_i;
static u32 s_85158_heights[8];
static u32 s_85158_n, s_85158_i;
static s32 s_85760_script[8];
static u32 s_85760_n, s_85760_i;
static s32 s_951a8_ret;

static void overrun(const char *who)
{
    fprintf(stderr, "ASSERTION %s: seam script overrun\n", who);
    exit(1);
}

void wm_95414_test_93354(u32 vec_addr)
{
    logev("93354", vec_addr, 0, 0, 0, 0);
}

s32 wm_95414_test_93978(s32 x, s32 z)
{
    logev("93978", (u32)x, (u32)z, 0, 0, 0);
    if (s_93978_i >= s_93978_n)
        overrun("93978");
    return s_93978_script[s_93978_i++];
}

s32 wm_95414_test_951a8(u32 b, u32 d, u32 o, s32 sc, s32 m)
{
    logev("951a8", b, d, o, (u32)sc, (u32)m);
    return s_951a8_ret;
}

s32 wm_95414_test_85760(u32 pos, u32 ws, s32 attr, s32 node)
{
    logev("85760", pos, ws, (u32)attr, (u32)node, 0);
    if (s_85760_i >= s_85760_n)
        overrun("85760");
    return s_85760_script[s_85760_i++];
}

s32 wm_95414_test_84d00(u32 pos, u32 out_attr)
{
    logev("84d00", pos, out_attr, 0, 0, 0);
    wr16(out_attr, s_84d00_attr);
    return s_84d00_ret;
}

u32 wm_95414_test_85158(u32 a, u32 b, u32 c, u32 attr, u32 node)
{
    logev("85158", a, b, c, attr, node);
    if (s_85158_i >= s_85158_n)
        overrun("85158");
    wr(SC_COEF + 4u, s_85158_heights[s_85158_i++]);
    return 0u;
}

s32 wm_95414_test_85418(u32 a, s32 yoff, u32 attr, u32 node)
{
    logev("85418", a, (u32)yoff, attr, node, 0);
    if (s_85418_i >= s_85418_n)
        overrun("85418");
    return s_85418_script[s_85418_i++];
}

void wm_95414_test_952b0(u32 mov, u32 out, u32 ref)
{
    logev("952b0", mov, out, ref, 0, 0);
}

void wm_95414_test_95324(u32 nrm, u32 mov, u32 out)
{
    logev("95324", nrm, mov, out, 0, 0);
}

/* store trace: only globals + out vec, for ordering checks. */
void wm_95414_test_store(u32 address, u32 value)
{
    if (address == G_ATTR || address == G_NODE ||
        (address >= OUT_VEC && address < OUT_VEC + 0x10u))
        logev("st", address, value, 0, 0, 0);
}

static void reset(void)
{
    s_nlog = 0;
    s_93978_i = 0; s_93978_n = 0;
    s_85418_i = 0; s_85418_n = 0;
    s_85158_i = 0; s_85158_n = 0;
    s_85760_i = 0; s_85760_n = 0;
    s_84d00_ret = 0;
    s_84d00_attr = 0;
    s_951a8_ret = 0;
}

/* Standard input state; entry projections consume these. */
static void setup_entry(void)
{
    wr(POS_VEC + 0u, 0x10000u);
    wr(POS_VEC + 4u, 0x5000u);   /* pos.y: >>12 = 5 */
    wr(POS_VEC + 8u, 0x20000u);
    wr(DIR_VEC + 0u, 0x3000u);
    wr(DIR_VEC + 8u, (u32)-0x2000);
    wr(OUT_VEC + 0u, CANARY);
    wr(OUT_VEC + 4u, CANARY);
    wr(OUT_VEC + 8u, CANARY);
    s_93978_script[0] = 0x123;
    s_93978_script[1] = 0x456;
    s_93978_n = 2;
}

/* Expected entry-projection values (independent s64 reference). */
#define EXP_PX 0x11800u  /* 0x10000 + lo32(0x3000*0x800)>>12 */
#define EXP_PZ 0x1F000u  /* 0x20000 + sra(lo32(-0x2000*0x800),12) */
#define SCALE  0x800

static void check_entry_events(const char *tag)
{
    char nm[64];

    snprintf(nm, sizeof(nm), "%s.e0", tag);
    expect_ev(nm, 0, "93354", SC_PROJ, 0, 0, 0, 0);
    snprintf(nm, sizeof(nm), "%s.e1", tag);
    expect_ev(nm, 1, "93978", EXP_PX, EXP_PZ, 0, 0, 0);
    /* events 2..4: out stores +0, +8 then 93354(out) */
    snprintf(nm, sizeof(nm), "%s.sc60", tag);
    check(nm, rd(SC_PROJ + 0u), EXP_PX);
    snprintf(nm, sizeof(nm), "%s.sc68", tag);
    check(nm, rd(SC_PROJ + 8u), EXP_PZ);
    snprintf(nm, sizeof(nm), "%s.sc64", tag);
    check(nm, rd(SC_PROJ + 4u), 0x123u - 0x4000u);
    snprintf(nm, sizeof(nm), "%s.e4", tag);
    expect_ev(nm, 4, "93354", OUT_VEC, 0, 0, 0, 0);
    snprintf(nm, sizeof(nm), "%s.e5", tag);
    expect_ev(nm, 5, "93978", EXP_PX, EXP_PZ, 0, 0, 0);
}

/* Node record helpers: node k at NODESBLK+8+14k. */
static void set_node(int k, s32 l6, s32 l8, s32 lA, u16 type)
{
    u32 n = NODESBLK + 8u + 14u * (u32)k;

    wr16(n + 6u, (u16)(s16)l6);
    wr16(n + 8u, (u16)(s16)l8);
    wr16(n + 0xAu, (u16)(s16)lA);
    wr16(n + 0xCu, type);
}

static void setup_case3(u32 attr, u16 node_id)
{
    wr(G_ATTR, attr);
    wr(G_NODE, (u32)node_id);
    wr(G_RECTAB, RECTAB);
    /* rec = RECTAB + 84*sign16(attr); rec[0x44] = NODESBLK */
    wr(RECTAB + 84u * (attr & 0xFFFFu) + 0x44u, NODESBLK);
    /* deterministic zeros around node[-1] for the PAIR_MASK mutant */
    {
        u32 i;

        for (i = 0; i < 0x40u; i += 4u)
            wr(NODESBLK - 0x20u + i, 0u);
    }
}

int main(void)
{
    s32 r;

    PsxMemory_Init();

    /* ============ A: entry + case-1 s4==0 fallback ============ */
    reset();
    setup_entry();
    wr(G_ATTR, 0xFFFFFFFFu);
    wr(G_NODE, 5u);                 /* stale; must be reset to -1 */
    s_84d00_ret = 0;
    s_951a8_ret = 1;
    r = wm_80095414(POS_VEC, DIR_VEC, OUT_VEC, SCALE, 0x7ABC1234);
    check("A.ret", (u32)r, 1u);
    check_entry_events("A");
    /* find the 951a8 event: verbatim scale + mode passthrough */
    {
        u32 i, found = 0;

        for (i = 0; i < s_nlog && i < 64u; i++)
            if (strcmp(s_log[i].kind, "951a8") == 0) {
                expect_ev("A.951a8", i, "951a8", POS_VEC, DIR_VEC, OUT_VEC,
                          SCALE, 0x7ABC1234u);
                found = 1;
                break;
            }
        check("A.951a8.found", found, 1u);
    }
    check("A.cache.node", rd(G_NODE), 0xFFFFFFFFu);
    check("A.cache.attr", rd(G_ATTR), 0xFFFFFFFFu);
    /* reset order: C16C before C840 */
    {
        u32 i;
        int saw_node = 0, order_ok = 0;

        for (i = 0; i < s_nlog && i < 64u; i++)
            if (strcmp(s_log[i].kind, "st") == 0) {
                if (s_log[i].a == G_NODE && s_log[i].b == 0xFFFFFFFFu)
                    saw_node = 1;
                if (s_log[i].a == G_ATTR && s_log[i].b == 0xFFFFFFFFu &&
                    saw_node)
                    order_ok = 1;
            }
        check("A.reset.order", (u32)order_ok, 1u);
    }
    printf("A entry + s4==0 fallback ok\n");

    /* ============ B: case-1 all-filtered fallback ============ */
    reset();
    setup_entry();
    wr(G_ATTR, 0xFFFFFFFFu);
    s_84d00_ret = 4;
    s_84d00_attr = 7;
    wr16(G_CAND + 0u, 5u); wr16(G_CAND + 2u, 0u);
    wr16(G_CAND + 4u, 9u); wr16(G_CAND + 6u, 0u);
    s_85418_script[0] = 0; s_85418_script[1] = 0; s_85418_n = 2;
    s_951a8_ret = 0;
    r = wm_80095414(POS_VEC, DIR_VEC, OUT_VEC, SCALE, 3);
    check("B.ret", (u32)r, 0u);
    check("B.cand0.marked", (u32)rd16(G_CAND), 0xFFFFu);
    check("B.cand1.marked", (u32)rd16(G_CAND + 4u), 0xFFFFu);
    {
        u32 i, n418 = 0, n951 = 0;

        for (i = 0; i < s_nlog && i < 64u; i++) {
            if (strcmp(s_log[i].kind, "85418") == 0) {
                if (n418 == 0)
                    expect_ev("B.418a", i, "85418", SC_PROJ, 0x70u, 7u, 5u, 0);
                if (n418 == 1)
                    expect_ev("B.418b", i, "85418", SC_PROJ, 0x70u, 7u, 9u, 0);
                n418++;
            }
            if (strcmp(s_log[i].kind, "951a8") == 0)
                n951++;
        }
        check("B.418.count", n418, 2u);
        check("B.951a8.called", n951, 1u);
    }
    printf("B all-filtered fallback ok\n");

    /* ============ C: case-1 proximity boundaries + accept ====== */
    reset();
    setup_entry();
    wr(G_ATTR, 0xFFFFFFFFu);
    s_84d00_ret = 6;
    s_84d00_attr = 7;
    wr16(G_CAND + 0u, 5u);  wr16(G_CAND + 2u, 0u);   /* diff 11: reject */
    wr16(G_CAND + 4u, 8u);  wr16(G_CAND + 6u, 1u);   /* flag 1: skip   */
    wr16(G_CAND + 8u, 9u);  wr16(G_CAND + 10u, 0u);  /* diff 10: accept*/
    s_85418_script[0] = 1; s_85418_script[1] = 1; s_85418_script[2] = 1;
    s_85418_n = 3;
    /* pos.y>>12 = 5; heights: 16 -> diff 11 reject; 15 -> diff 10 ok  */
    s_85158_heights[0] = 16u; s_85158_heights[1] = 15u; s_85158_n = 2;
    r = wm_80095414(POS_VEC, DIR_VEC, OUT_VEC, SCALE, 3);
    check("C.ret", (u32)r, 1u);
    check("C.out4", rd(OUT_VEC + 4u), 15u << 12);
    check("C.attr", rd(G_ATTR), 7u);
    check("C.node", rd(G_NODE), 9u);
    {
        /* accept store order: out4, C840, C16C */
        u32 i;
        int st = 0;

        for (i = 0; i < s_nlog && i < 64u; i++)
            if (strcmp(s_log[i].kind, "st") == 0) {
                if (st == 0 && s_log[i].a == OUT_VEC + 4u &&
                    s_log[i].b == (15u << 12))
                    st = 1;
                else if (st == 1 && s_log[i].a == G_ATTR)
                    st = 2;
                else if (st == 2 && s_log[i].a == G_NODE)
                    st = 3;
            }
        check("C.accept.order", (u32)st, 3u);
        /* the flagged candidate never reached 85158 */
        for (i = 0; i < s_nlog && i < 64u; i++)
            if (strcmp(s_log[i].kind, "85158") == 0)
                check("C.85158.not8", (u32)(s_log[i].e != 8u), 1u);
    }
    printf("C proximity accept ok\n");

    /* negative diff boundary: height -5 -> |−10| accept */
    reset();
    setup_entry();
    wr(G_ATTR, 0xFFFFFFFFu);
    s_84d00_ret = 2; s_84d00_attr = 7;
    wr16(G_CAND + 0u, 5u); wr16(G_CAND + 2u, 0u);
    s_85418_script[0] = 1; s_85418_n = 1;
    s_85158_heights[0] = (u32)-5; s_85158_n = 1;
    r = wm_80095414(POS_VEC, DIR_VEC, OUT_VEC, SCALE, 3);
    check("C2.ret", (u32)r, 1u);
    check("C2.out4", rd(OUT_VEC + 4u), ((u32)-5) << 12);
    printf("C2 negative-diff accept ok\n");

    /* no proximity hit -> 95324, cache untouched */
    reset();
    setup_entry();
    wr(G_ATTR, 0xFFFFFFFFu);
    s_84d00_ret = 2; s_84d00_attr = 7;
    wr16(G_CAND + 0u, 5u); wr16(G_CAND + 2u, 0u);
    s_85418_script[0] = 1; s_85418_n = 1;
    s_85158_heights[0] = 17u; s_85158_n = 1;   /* diff 12 reject */
    r = wm_80095414(POS_VEC, DIR_VEC, OUT_VEC, SCALE, 3);
    check("D.ret", (u32)r, 0u);
    check("D.attr.untouched", rd(G_ATTR), 0xFFFFFFFFu);
    {
        u32 i, n = 0;

        for (i = 0; i < s_nlog && i < 64u; i++)
            if (strcmp(s_log[i].kind, "95324") == 0) {
                expect_ev("D.95324", i, "95324", SC_NORM, DIR_VEC, OUT_VEC,
                          0, 0);
                n++;
            }
        check("D.95324.count", n, 1u);
    }
    printf("D no-hit 95324 ok\n");

    /* high-bit attr: lhu args vs lh cache store */
    reset();
    setup_entry();
    wr(G_ATTR, 0xFFFFFFFFu);
    s_84d00_ret = 2; s_84d00_attr = 0x8001u;
    wr16(G_CAND + 0u, 5u); wr16(G_CAND + 2u, 0u);
    s_85418_script[0] = 1; s_85418_n = 1;
    s_85158_heights[0] = 15u; s_85158_n = 1;
    r = wm_80095414(POS_VEC, DIR_VEC, OUT_VEC, SCALE, 3);
    check("E.ret", (u32)r, 1u);
    check("E.attr.signext", rd(G_ATTR), 0xFFFF8001u);
    {
        u32 i;

        for (i = 0; i < s_nlog && i < 64u; i++) {
            if (strcmp(s_log[i].kind, "85418") == 0)
                check("E.418.attr.lhu", s_log[i].c, 0x8001u);
            if (strcmp(s_log[i].kind, "85158") == 0)
                check("E.158.attr.lhu", s_log[i].d, 0x8001u);
        }
    }
    printf("E attr width ok\n");

    /* ============ F: case-3 slot 0 land ============ */
    reset();
    setup_entry();
    setup_case3(2u, 1u);
    s_85760_script[0] = 0; s_85760_n = 1;
    s_85158_heights[0] = 0x21u; s_85158_n = 1;
    r = wm_80095414(POS_VEC, DIR_VEC, OUT_VEC, SCALE, 3);
    check("F.ret", (u32)r, 1u);
    check("F.out4", rd(OUT_VEC + 4u), 0x21u << 12);
    check("F.node", rd(G_NODE), 1u);
    check("F.attr", rd(G_ATTR), 2u);
    {
        u32 i, n760 = 0;
        int st = 0;

        for (i = 0; i < s_nlog && i < 64u; i++) {
            if (strcmp(s_log[i].kind, "85760") == 0) {
                expect_ev("F.760", i, "85760", POS_VEC, SC_PROJ, 2u, 1u, 0);
                n760++;
            }
            if (strcmp(s_log[i].kind, "85158") == 0)
                expect_ev("F.158", i, "85158", SC_PROJ, SC_COEF, SC_NORM,
                          2u, 1u);
            if (strcmp(s_log[i].kind, "st") == 0) {
                /* land order: C16C, out4, C840 */
                if (st == 0 && s_log[i].a == G_NODE) st = 1;
                else if (st == 1 && s_log[i].a == OUT_VEC + 4u) st = 2;
                else if (st == 2 && s_log[i].a == G_ATTR) st = 3;
            }
        }
        check("F.760.count", n760, 1u);       /* kills S4_NOT_CLEARED */
        check("F.land.order", (u32)st, 3u);
    }
    printf("F slot0 land ok\n");

    /* ============ G: slot1 arms ============ */
    /* G1: link -1 -> fallback */
    reset(); setup_entry(); setup_case3(2u, 1u);
    set_node(1, -1, -1, -1, 0u);
    s_85760_script[0] = 1; s_85760_n = 1;
    s_951a8_ret = 1;
    r = wm_80095414(POS_VEC, DIR_VEC, OUT_VEC, SCALE, 3);
    check("G1.ret", (u32)r, 1u);
    check("G1.cache", rd(G_ATTR), 0xFFFFFFFFu);
    /* G2: continue (type!=1) then land at neighbor */
    reset(); setup_entry(); setup_case3(2u, 1u);
    set_node(1, 3, -1, -1, 0u);
    set_node(3, -1, -1, -1, 0u);       /* type 0 -> walk to 3 */
    s_85760_script[0] = 1; s_85760_script[1] = 0; s_85760_n = 2;
    s_85158_heights[0] = 7u; s_85158_n = 1;
    r = wm_80095414(POS_VEC, DIR_VEC, OUT_VEC, SCALE, 3);
    check("G2.ret", (u32)r, 1u);
    check("G2.node", rd(G_NODE), 3u);
    {
        u32 i, n760 = 0;

        for (i = 0; i < s_nlog && i < 64u; i++)
            if (strcmp(s_log[i].kind, "85760") == 0) {
                if (n760 == 1)
                    check("G2.760b.node", s_log[i].d, 3u);
                n760++;
            }
        check("G2.760.count", n760, 2u);
    }
    /* G3: redirect (type==1) -> 952B0 fp+0x30; +8 set to -1 so the
     * JT_SLOT_SWAP mutant diverges into a fallback. */
    reset(); setup_entry(); setup_case3(2u, 1u);
    set_node(1, 3, -1, -1, 0u);
    set_node(3, -1, -1, -1, 1u);
    s_85760_script[0] = 1; s_85760_n = 1;
    r = wm_80095414(POS_VEC, DIR_VEC, OUT_VEC, SCALE, 3);
    check("G3.ret", (u32)r, 0u);
    {
        u32 i, n = 0;

        for (i = 0; i < s_nlog && i < 64u; i++)
            if (strcmp(s_log[i].kind, "952b0") == 0) {
                expect_ev("G3.952b0", i, "952b0", DIR_VEC, OUT_VEC,
                          0x1F800030u, 0, 0);
                n++;
            }
        check("G3.952b0.count", n, 1u);
    }
    printf("G slot1 arms ok\n");

    /* ============ H: slot2 / slot4 redirects + slot4 continue ==== */
    reset(); setup_entry(); setup_case3(2u, 1u);
    set_node(1, -1, 4, -1, 0u);
    set_node(4, -1, -1, -1, 1u);
    s_85760_script[0] = 2; s_85760_n = 1;
    r = wm_80095414(POS_VEC, DIR_VEC, OUT_VEC, SCALE, 3);
    check("H1.ret", (u32)r, 0u);
    {
        u32 i;

        for (i = 0; i < s_nlog && i < 64u; i++)
            if (strcmp(s_log[i].kind, "952b0") == 0)
                check("H1.ref", s_log[i].c, 0x1F800040u);
    }
    /* slot4: +8 = -1, +A = 5 (LINK_OFFSET mutant diverges) */
    reset(); setup_entry(); setup_case3(2u, 1u);
    set_node(1, -1, -1, 5, 0u);
    set_node(5, -1, -1, -1, 1u);
    s_85760_script[0] = 4; s_85760_n = 1;
    r = wm_80095414(POS_VEC, DIR_VEC, OUT_VEC, SCALE, 3);
    check("H2.ret", (u32)r, 0u);
    {
        u32 i, n = 0;

        for (i = 0; i < s_nlog && i < 64u; i++)
            if (strcmp(s_log[i].kind, "952b0") == 0) {
                check("H2.ref", s_log[i].c, 0x1F800050u);
                n++;
            }
        check("H2.952b0.count", n, 1u);
    }
    /* slot4 continue: type != 1 walks */
    reset(); setup_entry(); setup_case3(2u, 1u);
    set_node(1, -1, -1, 5, 0u);
    set_node(5, -1, -1, -1, 2u);
    s_85760_script[0] = 4; s_85760_script[1] = 0; s_85760_n = 2;
    s_85158_heights[0] = 9u; s_85158_n = 1;
    r = wm_80095414(POS_VEC, DIR_VEC, OUT_VEC, SCALE, 3);
    check("H3.ret", (u32)r, 1u);
    check("H3.node", rd(G_NODE), 5u);
    printf("H slot2/slot4 ok\n");

    /* ============ I: slot3 pair resolver ============ */
    /* I1 mask 0 -> fallback */
    reset(); setup_entry(); setup_case3(2u, 1u);
    set_node(1, -1, -1, -1, 0u);
    s_85760_script[0] = 3; s_85760_n = 1;
    s_951a8_ret = 0;
    r = wm_80095414(POS_VEC, DIR_VEC, OUT_VEC, SCALE, 3);
    check("I1.ret", (u32)r, 0u);
    check("I1.cache", rd(G_NODE), 0xFFFFFFFFu);
    /* I2 mask 1 type==1 -> 952B0 0x30 (PAIR_MASK mutant sees mask 3) */
    reset(); setup_entry(); setup_case3(2u, 1u);
    set_node(1, 3, -1, -1, 0u);
    set_node(3, -1, -1, -1, 1u);
    s_85760_script[0] = 3; s_85760_n = 1;
    r = wm_80095414(POS_VEC, DIR_VEC, OUT_VEC, SCALE, 3);
    check("I2.ret", (u32)r, 0u);
    {
        u32 i, n = 0;

        for (i = 0; i < s_nlog && i < 64u; i++)
            if (strcmp(s_log[i].kind, "952b0") == 0) {
                check("I2.ref", s_log[i].c, 0x1F800030u);
                n++;
            }
        check("I2.952b0.count", n, 1u);
    }
    /* I3 mask 2 type==1 -> 952B0 0x40 */
    reset(); setup_entry(); setup_case3(2u, 1u);
    set_node(1, -1, 4, -1, 0u);
    set_node(4, -1, -1, -1, 1u);
    s_85760_script[0] = 3; s_85760_n = 1;
    r = wm_80095414(POS_VEC, DIR_VEC, OUT_VEC, SCALE, 3);
    check("I3.ret", (u32)r, 0u);
    {
        u32 i;

        for (i = 0; i < s_nlog && i < 64u; i++)
            if (strcmp(s_log[i].kind, "952b0") == 0)
                check("I3.ref", s_log[i].c, 0x1F800040u);
    }
    /* I4 mask 3, v==0 (both types nonzero): zero-out order +8,+4,+0 */
    reset(); setup_entry(); setup_case3(2u, 1u);
    set_node(1, 3, 4, -1, 0u);
    set_node(3, -1, -1, -1, 2u);
    set_node(4, -1, -1, -1, 2u);
    s_85760_script[0] = 3; s_85760_n = 1;
    r = wm_80095414(POS_VEC, DIR_VEC, OUT_VEC, SCALE, 3);
    check("I4.ret", (u32)r, 0u);
    check("I4.out0", rd(OUT_VEC + 0u), 0u);
    check("I4.out8", rd(OUT_VEC + 8u), 0u);
    {
        u32 i;
        int st = 0;

        for (i = 0; i < s_nlog && i < 64u; i++)
            if (strcmp(s_log[i].kind, "st") == 0 && s_log[i].b == 0u) {
                if (st == 0 && s_log[i].a == OUT_VEC + 8u) st = 1;
                else if (st == 1 && s_log[i].a == OUT_VEC + 4u) st = 2;
                else if (st == 2 && s_log[i].a == OUT_VEC + 0u) st = 3;
            }
        check("I4.zero.order", (u32)st, 3u);
    }
    /* I5 mask 3, v==2 (typeA!=0, typeB==0): continue with lb
     * (TIEBREAK_TYPEB_POLARITY mutant computes v=0 -> zero-out).     */
    reset(); setup_entry(); setup_case3(2u, 1u);
    set_node(1, 3, 4, -1, 0u);
    set_node(3, -1, -1, -1, 2u);
    set_node(4, -1, -1, -1, 0u);
    s_85760_script[0] = 3; s_85760_script[1] = 0; s_85760_n = 2;
    s_85158_heights[0] = 4u; s_85158_n = 1;
    r = wm_80095414(POS_VEC, DIR_VEC, OUT_VEC, SCALE, 3);
    check("I5.ret", (u32)r, 1u);
    check("I5.node", rd(G_NODE), 4u);
    /* I6 mask 3, v==1 (typeA==0, typeB!=0): follow +6 */
    reset(); setup_entry(); setup_case3(2u, 1u);
    set_node(1, 3, 4, -1, 0u);
    set_node(3, -1, -1, -1, 0u);
    set_node(4, -1, -1, -1, 2u);
    s_85760_script[0] = 3; s_85760_script[1] = 0; s_85760_n = 2;
    s_85158_heights[0] = 4u; s_85158_n = 1;
    r = wm_80095414(POS_VEC, DIR_VEC, OUT_VEC, SCALE, 3);
    check("I6.ret", (u32)r, 1u);
    check("I6.node", rd(G_NODE), 3u);
    printf("I slot3 arms ok\n");

    /* ============ J: slot5 tie-break follows +8 ============ */
    reset(); setup_entry(); setup_case3(2u, 1u);
    set_node(1, 3, 4, 6, 0u);     /* +6=3 (A), +8=4, +A=6 (B) */
    set_node(3, -1, -1, -1, 0u);  /* typeA(+6) == 0 */
    set_node(6, -1, -1, -1, 2u);  /* typeB(+A) != 0 -> v=1 */
    set_node(4, -1, -1, -1, 0u);  /* follow target via +8 */
    s_85760_script[0] = 5; s_85760_script[1] = 0; s_85760_n = 2;
    s_85158_heights[0] = 4u; s_85158_n = 1;
    r = wm_80095414(POS_VEC, DIR_VEC, OUT_VEC, SCALE, 3);
    check("J.ret", (u32)r, 1u);
    check("J.node.follow8", rd(G_NODE), 4u);
    printf("J slot5 tie-break +8 ok\n");

    /* ============ K: slot6 mask1 redirect 0x40 / mask2 0x50 ====== */
    reset(); setup_entry(); setup_case3(2u, 1u);
    set_node(1, -1, 4, -1, 0u);   /* A = +8 link */
    set_node(4, -1, -1, -1, 1u);
    s_85760_script[0] = 6; s_85760_n = 1;
    r = wm_80095414(POS_VEC, DIR_VEC, OUT_VEC, SCALE, 3);
    check("K1.ret", (u32)r, 0u);
    {
        u32 i;

        for (i = 0; i < s_nlog && i < 64u; i++)
            if (strcmp(s_log[i].kind, "952b0") == 0)
                check("K1.ref", s_log[i].c, 0x1F800040u);
    }
    reset(); setup_entry(); setup_case3(2u, 1u);
    set_node(1, -1, -1, 6, 0u);   /* B = +A link */
    set_node(6, -1, -1, -1, 1u);
    s_85760_script[0] = 6; s_85760_n = 1;
    r = wm_80095414(POS_VEC, DIR_VEC, OUT_VEC, SCALE, 3);
    check("K2.ret", (u32)r, 0u);
    {
        u32 i;

        for (i = 0; i < s_nlog && i < 64u; i++)
            if (strcmp(s_log[i].kind, "952b0") == 0)
                check("K2.ref", s_log[i].c, 0x1F800050u);
    }
    printf("K slot6 ok\n");

    /* ============ L: >=8 guard and slot 7 re-dispatch ============ */
    reset(); setup_entry(); setup_case3(2u, 1u);
    s_85760_script[0] = 8; s_85760_script[1] = 0; s_85760_n = 2;
    s_85158_heights[0] = 2u; s_85158_n = 1;
    r = wm_80095414(POS_VEC, DIR_VEC, OUT_VEC, SCALE, 3);
    check("L1.ret", (u32)r, 1u);
    reset(); setup_entry(); setup_case3(2u, 1u);
    s_85760_script[0] = 7; s_85760_script[1] = 0; s_85760_n = 2;
    s_85158_heights[0] = 2u; s_85158_n = 1;
    r = wm_80095414(POS_VEC, DIR_VEC, OUT_VEC, SCALE, 3);
    check("L2.ret", (u32)r, 1u);
    printf("L guard/slot7 ok\n");

    if (s_failures != 0) {
        fprintf(stderr, "FAILURES=%d\n", s_failures);
        return 1;
    }
    printf("W34B24-I7 0x80095414 focused oracle PASS\n");
    return 0;
}
