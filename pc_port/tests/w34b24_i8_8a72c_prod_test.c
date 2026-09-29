/*
 * Focused production-linked oracle for retail world callback 0x8008A72C.
 *
 * Expected values are hand-derived from the retail disassembly
 * (docs/evidence/w34b24-pre3-8a72c + this rung's re-derivation).  All
 * callees are routed through recording seams; callee results are forced
 * per case.  Store events are captured through the production trace
 * seam and compared against per-case expected event predicates.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_8a72c.h"

#define POOL_GUEST   0x800D7538u
#define SLOT1        (POOL_GUEST + 0x80u)
#define SC           0x1F800000u
#define CANARY       0xB6B6B6B6u

#define G_POOL_PTR   0x8009BE24u
#define G_MODE_WORD  0x8009C170u
#define G_MODE_FLAG  0x8009BE10u
#define G_BD04       0x8009BD04u
#define G_AREA       0x8009BD60u
#define G_BOUND      0x8009D738u
#define G_CRUMB      0x8009B180u
#define G_RING_IDX   0x8009D154u
#define G_RING       0x8009CEC4u
#define G_POSE       0x8009D55Cu
#define G_HEAD       0x8009D52Cu
#define G_D554       0x8009D554u
#define G_D7CC       0x8009D7CCu
#define G_RESYNC     0x8006F8E5u
#define G_BUSY2      0x8006F8E6u
#define G_BUSY3      0x8006F8E7u
#define G_S4PRES     0x8006F368u
#define G_S2PRES     0x8006F369u
#define G_S3PRES     0x8006F36Au
#define G_WARPX      0x8006EF90u
#define G_WARPZ      0x8006EF92u
#define G_WARPH      0x8006EE5Au
#define G_MIRX       0x8006EE54u
#define G_MIRZ       0x8006EE56u
#define G_MIRH       0x8006EE58u

static int s_failures;
static u8 s_native_sprite[0xC0];

static void chk(const char* n, u32 got, u32 want)
{
    if (got != want) {
        fprintf(stderr, "ASSERTION %s: got=0x%08x expected=0x%08x\n", n, got,
                want);
        if (++s_failures > 30)
            exit(1);
    }
}

static u32 rd32(u32 a) { u32 v; memcpy(&v, PSX_ADDR(a), 4); return v; }
static void wr32(u32 a, u32 v) { memcpy(PSX_ADDR(a), &v, 4); }
static u16 rd16(u32 a) { u16 v; memcpy(&v, PSX_ADDR(a), 2); return v; }
static void wr16(u32 a, u16 v) { memcpy(PSX_ADDR(a), &v, 2); }
static void wr8(u32 a, u8 v) { memcpy(PSX_ADDR(a), &v, 1); }
static u8 rd8(u32 a) { u8 v; memcpy(&v, PSX_ADDR(a), 1); return v; }

/* --------------------------- call log ------------------------------- */

typedef struct {
    char tag[8];
    u32 a[5];
} Call;

static Call s_calls[64];
static u32 s_ncalls;

static void log_call(const char* tag, u32 a0, u32 a1, u32 a2, u32 a3, u32 a4)
{
    if (s_ncalls < 64u) {
        snprintf(s_calls[s_ncalls].tag, sizeof(s_calls[s_ncalls].tag), "%s",
                 tag);
        s_calls[s_ncalls].a[0] = a0;
        s_calls[s_ncalls].a[1] = a1;
        s_calls[s_ncalls].a[2] = a2;
        s_calls[s_ncalls].a[3] = a3;
        s_calls[s_ncalls].a[4] = a4;
    }
    s_ncalls++;
}

static const Call* find_call(const char* tag, u32 nth)
{
    u32 i, seen = 0;

    for (i = 0; i < s_ncalls && i < 64u; i++)
        if (strcmp(s_calls[i].tag, tag) == 0) {
            if (seen == nth)
                return &s_calls[i];
            seen++;
        }
    return 0;
}

static u32 count_calls(const char* tag)
{
    u32 i, n = 0;

    for (i = 0; i < s_ncalls && i < 64u; i++)
        if (strcmp(s_calls[i].tag, tag) == 0)
            n++;
    return n;
}

void wm_8a72c_test_store(u32 address, u32 width, u32 value)
{
    (void)address;
    (void)width;
    (void)value;
}

/* Forced results */
static s32 s_f_90a84;
static s32 s_f_95414[2];
static u32 s_n_95414;
static s32 s_f_97770;
static s32 s_f_8bec8;
static s32 s_f_93978 = 0x777;
static long s_f_rcos = 100;
static long s_f_rsin = 60;

s32 wm_8a72c_test_90a84(u32 a) { log_call("90a84", a, 0, 0, 0, 0); return s_f_90a84; }
void wm_8a72c_test_245d8(u32 o, s16 n) { log_call("245d8", o, (u32)(s32)n, 0, 0, 0); }
void wm_8a72c_test_894c8(u32 i) { log_call("894c8", i, 0, 0, 0, 0); }
void wm_8a72c_test_8c1dc(u32 c, u32 o, u32 u) { log_call("8c1dc", c, o, u, 0, 0); }
s32 wm_8a72c_test_95414(u32 p, u32 d, u32 o, s32 s, s32 m)
{
    s32 r = s_f_95414[(s_n_95414 < 2u) ? s_n_95414 : 1u];

    log_call("95414", p, d, o, (u32)s, (u32)m);
    s_n_95414++;
    return r;
}
void wm_8a72c_test_8c040(u32 v, s32 a1, s32 a2, u32 o1, u32 o2)
{
    log_call("8c040", v, (u32)a1, (u32)a2, o1, o2);
}
void wm_8a72c_test_7528c(void) { log_call("7528c", 0, 0, 0, 0, 0); }
s32 wm_8a72c_test_94238(u32 p, u32 i) { log_call("94238", p, i, 0, 0, 0); return 0; }
s32 wm_8a72c_test_97770(u32 s, s32 v) { log_call("97770", s, (u32)v, 0, 0, 0); return s_f_97770; }
s32 wm_8a72c_test_941c4(u32 a, u32 b, u32 o, u32 g)
{
    log_call("941c4", a, b, o, g, 0);
    return 0;
}
s32 wm_8a72c_test_8bec8(u32 o) { log_call("8bec8", o, 0, 0, 0, 0); return s_f_8bec8; }
s32 wm_8a72c_test_93978(s32 x, s32 z) { log_call("93978", (u32)x, (u32)z, 0, 0, 0); return s_f_93978; }
long wm_8a72c_test_rcos(long a) { log_call("rcos", (u32)a, 0, 0, 0, 0); return s_f_rcos; }
long wm_8a72c_test_rsin(long a) { log_call("rsin", (u32)a, 0, 0, 0, 0); return s_f_rsin; }
void wm_8a72c_test_74794(s32 t, u32 p) { log_call("74794", (u32)t, p, 0, 0, 0); }

/* --------------------------- fixtures ------------------------------- */

static void reset(void)
{
    u32 i;

    s_ncalls = 0;
    s_n_95414 = 0;
    s_f_90a84 = 0;
    s_f_95414[0] = 1;
    s_f_95414[1] = 1;
    s_f_97770 = 1;
    s_f_8bec8 = 0;
    wr32(G_POOL_PTR, POOL_GUEST);
    for (i = 0; i < 0x400u; i += 4u)
        wr32(POOL_GUEST + i, 0u);
    wr32(G_MODE_WORD, 0u);
    wr32(G_MODE_FLAG, 0x1234u);
    wr16(G_BD04, 0x7777u);
    wr8(G_AREA, 0u);
    wr8(G_BOUND, 0u);
    wr16(G_RING_IDX, 0u);
    wr8(G_RESYNC, 0u);
    wr8(G_BUSY2, 0u);
    wr8(G_BUSY3, 0u);
    wr8(G_S4PRES, 0u);
    wr8(G_S2PRES, 0u);
    wr8(G_S3PRES, 0u);
    wr16(G_WARPX, 0u);
    wr16(G_WARPZ, 0u);
    wr16(G_WARPH, 0u);
    /* breadcrumb gate row 0 nonzero by default */
    wr16(G_CRUMB, 1u);
    /* slot record defaults */
    wr16(SLOT1 + 4u, 0u);
    wr16(SLOT1 + 0x20u, 0u);
    wr16(SLOT1 + 0x24u, 0u);
    wr32(SLOT1 + 0x28u, 0x5000u << 12);
    wr32(SLOT1 + 0x2Cu, 0x100u);
    wr32(SLOT1 + 0x30u, 0x3000u << 12);
    wr32(SLOT1 + 0x34u, 0u);
    wr32(SLOT1 + 0x38u, 0u);
    wr32(SLOT1 + 0x3Cu, 0u);
    wr32(SLOT1 + 0x40u, 0u);
    wr32(SLOT1 + 0x44u, 0u);
    wr16(SLOT1 + 0x48u, 0x400u);
    wr16(SLOT1 + 0x4Au, 6u);
    memset(s_native_sprite, 0, sizeof(s_native_sprite));
    wr32(SLOT1 + 0x4Cu, (u32)(uintptr_t)s_native_sprite);
    /* Keep the incorrectly remapped guest byte distinct from the native
     * sprite byte. The retail guard must ignore this alias. */
    *(s8*)PSX_ADDR((u32)(uintptr_t)s_native_sprite + 0xAFu) = 7;
}

static void set_anim(u8 v)
{
    s_native_sprite[0xAF] = v;
    *(s8*)PSX_ADDR((u32)(uintptr_t)s_native_sprite + 0xAFu) =
        (s8)(v == 0u ? 1 : 0);
}

static s32 run(void)
{
    return wm_8008A72C(1);
}

static void expect_tail(const char* tag, u32 want_74794)
{
    char n[64];

    snprintf(n, sizeof(n), "%s.mirx", tag);
    chk(n, rd16(G_MIRX), (u16)(rd32(SLOT1 + 0x28u) >> 12));
    snprintf(n, sizeof(n), "%s.mirz", tag);
    chk(n, rd16(G_MIRZ), (u16)(rd32(SLOT1 + 0x30u) >> 12));
    snprintf(n, sizeof(n), "%s.mirh", tag);
    chk(n, rd16(G_MIRH), rd16(SLOT1 + 0x48u));
    snprintf(n, sizeof(n), "%s.n74794", tag);
    chk(n, count_calls("74794"), want_74794);
}

int main(void)
{
    s32 r;

    PsxMemory_Init();

    /* ---- substate 3 -> state 1 ---- */
    reset();
    wr16(SLOT1 + 4u, 3u);
    wr16(SLOT1 + 0x20u, 0x13u); /* parked afterwards? no: substate sets 1 */
    s_f_90a84 = 1;              /* JT2 class 1 */
    r = run();
    chk("sub3.ret", (u32)r, 1u);
    chk("sub3.sub", rd16(SLOT1 + 4u), 0u);
    /* state was set to 1 by the substate, then JT1 slot1 ran (class1 ->
     * state 0x40) */
    chk("sub3.state", rd16(SLOT1 + 0x20u), 0x40u);
    chk("sub3.d554", rd32(G_D554), 0u);
    chk("sub3.d7cc", rd32(G_D7CC), 0u);
    chk("sub3.bd04", rd16(G_BD04), 0u);
    chk("sub3.flag24", rd16(SLOT1 + 0x24u), 0u);
    expect_tail("sub3", 1u);

    /* ---- substate 2 -> state 0x28 (warp-in runs) ---- */
    reset();
    wr16(SLOT1 + 4u, 2u);
    wr16(G_WARPX, 0x123u);
    wr16(G_WARPZ, 0x456u);
    wr16(G_WARPH, 0x800u);
    r = run();
    chk("sub2.ret", (u32)r, 1u);
    chk("sub2.state", rd16(SLOT1 + 0x20u), 0x29u); /* 0x28 -> ++ */
    chk("sub2.posx", rd32(SLOT1 + 0x28u), 0x123u << 12);
    chk("sub2.posz", rd32(SLOT1 + 0x30u), 0x456u << 12);
    chk("sub2.posy", rd32(SLOT1 + 0x2Cu), (u32)s_f_93978);
    chk("sub2.5c", rd32(SLOT1 + 0x5Cu), 0x800u);
    chk("sub2.head", rd16(SLOT1 + 0x48u), 0x800u);
    /* facing point: x + rcos*48, z + (-rsin)*48 */
    chk("sub2.fx", rd32(SC + 0u), (0x123u << 12) + (u32)s_f_rcos * 48u);
    chk("sub2.fz", rd32(SC + 8u),
        (0x456u << 12) + (0u - (u32)s_f_rsin) * 48u);
    chk("sub2.t50", rd32(SLOT1 + 0x50u), rd32(SC + 0u) >> 12);
    chk("sub2.t54", rd32(SLOT1 + 0x54u), rd32(SC + 8u) >> 12);
    {
        const Call* c = find_call("941c4", 0);

        chk("sub2.941c4", (u32)(c != 0), 1u);
        if (c) {
            chk("sub2.941c4.a", c->a[0], SLOT1 + 0x28u);
            chk("sub2.941c4.b", c->a[1], SC);
            chk("sub2.941c4.o", c->a[2], SLOT1 + 0x38u);
            chk("sub2.941c4.g", c->a[3], SLOT1 + 0x48u);
        }
        c = find_call("245d8", 0);
        chk("sub2.245d8", (u32)(c != 0), 1u);
        if (c)
            chk("sub2.245d8.anim", c->a[1], 1u);
    }

    /* ---- substate 6: lap gate closed / open ---- */
    reset();
    wr16(SLOT1 + 4u, 6u);
    wr32(SLOT1 + 0x58u, 4u);
    wr32(G_MODE_WORD, 9u); /* != 5 -> no commit */
    wr16(SLOT1 + 0x20u, 0x13u); /* parked */
    r = run();
    chk("sub6c.ret", (u32)r, 1u);
    chk("sub6c.lap", rd32(SLOT1 + 0x58u), 5u);
    chk("sub6c.state", rd16(SLOT1 + 0x20u), 0x13u); /* unchanged */
    chk("sub6c.flag", rd32(G_MODE_FLAG), 0x1234u);  /* untouched */

    reset();
    wr16(SLOT1 + 4u, 6u);
    wr32(SLOT1 + 0x58u, 4u);
    wr32(G_MODE_WORD, 5u); /* == lap+1 -> commit */
    wr16(SLOT1 + 0x20u, 0x13u);
    s_f_90a84 = 1; /* state forced 0 -> JT1 slot 0 -> class1 */
    r = run();
    chk("sub6o.lap", rd32(SLOT1 + 0x58u), 5u);
    chk("sub6o.flag", rd32(G_MODE_FLAG), 1u);
    chk("sub6o.state", rd16(SLOT1 + 0x20u), 0x40u); /* 0 -> class1 -> 0x40 */

    /* ---- JT1 guard: state 0x41 parks (OOR) ---- */
    reset();
    wr16(SLOT1 + 0x20u, 0x41u);
    r = run();
    chk("oor.ret", (u32)r, 1u);
    chk("oor.calls", s_ncalls, 1u); /* only 74794 */
    expect_tail("oor", 1u);

    /* ---- parked state 0x13 ---- */
    reset();
    wr16(SLOT1 + 0x20u, 0x13u);
    wr16(SLOT1 + 0x24u, 1u); /* suppress 74794 */
    r = run();
    chk("park.ret", (u32)r, 1u);
    chk("park.calls", s_ncalls, 0u);
    expect_tail("park", 0u);

    /* ---- state 0: resync path ---- */
    reset();
    wr8(G_RESYNC, 1u);
    wr32(POOL_GUEST + 0x228u, 0xAAA000u);
    wr32(POOL_GUEST + 0x22Cu, 0xBBB000u);
    wr32(POOL_GUEST + 0x230u, 0xCCC000u);
    wr16(POOL_GUEST + 0x248u, 0x777u);
    r = run();
    chk("resync.ret", (u32)r, 1u);
    chk("resync.x", rd32(SLOT1 + 0x28u), 0xAAA000u);
    chk("resync.y", rd32(SLOT1 + 0x2Cu), 0xBBB000u);
    chk("resync.z", rd32(SLOT1 + 0x30u), 0xCCC000u);
    chk("resync.h", rd16(SLOT1 + 0x48u), 0x777u);
    chk("resync.flag24", rd16(SLOT1 + 0x24u), 1u);
    chk("resync.894c8", count_calls("894c8"), 1u);
    expect_tail("resync", 0u); /* +0x24 == 1 suppresses 74794 */

    /* ---- state 0 -> JT2 class 3, area==7 and area!=7 ---- */
    reset();
    s_f_90a84 = 3;
    wr8(G_AREA, 7u);
    r = run();
    chk("cls3a.state", rd16(SLOT1 + 0x20u), 8u);
    reset();
    s_f_90a84 = 3;
    wr8(G_AREA, 5u);
    r = run();
    chk("cls3b.state", rd16(SLOT1 + 0x20u), 0x10u);
    chk("cls3b.area", rd8(G_AREA), 4u);

    /* ---- state 0 -> JT2 classes 2/4/5 -> pre-tail only ---- */
    reset();
    s_f_90a84 = 4;
    wr16(G_BD04, 0x5555u);
    r = run();
    chk("cls4.state", rd16(SLOT1 + 0x20u), 0u);
    chk("cls4.bd04", rd16(G_BD04), 0u);

    /* ---- movement: r=1 straight, breadcrumb open, vel nonzero ---- */
    reset();
    s_f_90a84 = 0; /* OOR -> movement */
    s_f_95414[0] = 1;
    wr32(SLOT1 + 0x38u, 0x1000u);
    set_anim(1); /* already walking: no 245d8 */
    wr32(SC + 0x90u, 0x11111000u);
    wr32(SC + 0x94u, 0x22222000u);
    wr32(SC + 0x98u, 0x33333000u);
    wr32(SC + 0x9Cu, 0x44444000u);
    wr8(G_BOUND, 0u);
    wr16(G_CRUMB, 1u);
    r = run();
    chk("mv1.ret", (u32)r, 1u);
    chk("mv1.n95414", count_calls("95414"), 1u);
    chk("mv1.already-walk-call-count", count_calls("245d8"), 0u);
    {
        const Call* c = find_call("95414", 0);

        if (c) {
            chk("mv1.p", c->a[0], SLOT1 + 0x28u);
            chk("mv1.d", c->a[1], SLOT1 + 0x38u);
            chk("mv1.o", c->a[2], SC + 0x90u);
            chk("mv1.s", c->a[3], 6u << 12);
            chk("mv1.m", c->a[4], 0x1234u);
        }
    }
    chk("mv1.posx", rd32(SLOT1 + 0x28u), 0x11111000u); /* committed */
    chk("mv1.posw", rd32(SLOT1 + 0x34u), 0x44444000u);
    chk("mv1.ring", rd16(G_RING_IDX), 1u);
    chk("mv1.ring.x", rd32(G_RING + 0x14u), 0x11111000u);
    chk("mv1.ring.h", rd16(G_RING + 0x14u + 0x10u), 0x400u);
    chk("mv1.7528c", count_calls("7528c"), 1u);
    chk("mv1.8c1dc", count_calls("8c1dc"), 1u);
    {
        const Call* c = find_call("8c1dc", 0);

        if (c)
            chk("mv1.8c1dc.ctx", c->a[0], 0x2Fu);
        c = find_call("8c040", 0);
        chk("mv1.8c040", (u32)(c != 0), 1u);
        if (c) {
            chk("mv1.8c040.v", c->a[0], SC + 0x90u);
            chk("mv1.8c040.o1", c->a[3], G_BOUND);
            chk("mv1.8c040.o2", c->a[4], G_AREA);
        }
    }
    chk("mv1.velz", rd32(SLOT1 + 0x38u), 0u); /* zeroed at block end */
    chk("mv1.94238", count_calls("94238"), 1u);
    chk("mv1.pose", rd32(G_POSE), 0x11111000u);
    chk("mv1.bd04", rd16(G_BD04), 0u);

    /* ---- movement: breadcrumb gate closed -> no commit, no ring ---- */
    reset();
    s_f_90a84 = 0;
    s_f_95414[0] = 1;
    wr32(SLOT1 + 0x38u, 0x1000u);
    set_anim(1);
    wr32(SC + 0x90u, 0x9000000u);
    wr8(G_BOUND, 2u);   /* crumb row 2 = 0 */
    wr16(G_CRUMB + 4u, 0u);
    r = run();
    chk("mv2.posx", rd32(SLOT1 + 0x28u), 0x5000u << 12); /* NOT committed */
    chk("mv2.ring", rd16(G_RING_IDX), 0u);
    chk("mv2.7528c", count_calls("7528c"), 0u);

    /* ---- movement: area==7 bias (+3) ---- */
    reset();
    s_f_90a84 = 0;
    s_f_95414[0] = 1;
    wr32(SLOT1 + 0x38u, 0x1000u);
    set_anim(1);
    wr8(G_AREA, 7u);
    wr8(G_BOUND, 1u);            /* -> row 4 */
    wr16(G_CRUMB + 8u, 0u);      /* row 4 closed */
    wr16(G_CRUMB + 2u, 0xFFFFu); /* row 1 open (would commit if unbiased) */
    wr16(G_CRUMB + 6u, 0xFFFFu); /* row 3 open (kills a +2 bias mutant) */
    r = run();
    chk("mv7.posx", rd32(SLOT1 + 0x28u), 0x5000u << 12); /* no commit */

    /* ---- movement: retry protocol r=0 then r=1 ---- */
    reset();
    s_f_90a84 = 0;
    s_f_95414[0] = 0;
    s_f_95414[1] = 1;
    wr32(SLOT1 + 0x38u, 0x1000u);
    set_anim(1);
    wr32(SC + 0x90u, 0x1000u);
    wr32(SC + 0x94u, 0x2000u);
    wr32(SC + 0x98u, 0x3000u);
    wr32(SC + 0x9Cu, 0x4000u);
    r = run();
    chk("mvr.n95414", count_calls("95414"), 2u);
    /* velocity got the 4-word redirect before the second call; end of
     * block zeroes +0x38/3C/40; +0x44 keeps the redirect value */
    chk("mvr.v44", rd32(SLOT1 + 0x44u), 0x4000u);

    /* ---- movement: double failure zeroes ONLY +0x40/+0x38 ---- */
    reset();
    s_f_90a84 = 0;
    s_f_95414[0] = 0;
    s_f_95414[1] = 0;
    wr32(SLOT1 + 0x38u, 0x1000u);
    set_anim(1);
    wr32(SC + 0x90u, 0xA000u);
    wr32(SC + 0x94u, 0xB000u);
    wr32(SC + 0x98u, 0xC000u);
    wr32(SC + 0x9Cu, 0xD000u);
    r = run();
    chk("mvz.n95414", count_calls("95414"), 2u);
    /* after redirect copy: 38=A000 3C=B000 40=C000 44=D000; fallback
     * zeroes 40,38; r!=1 -> second 8c040 with slot pos; block-end zero
     * clears 40/3C/38; 44 remains D000 */
    chk("mvz.v44", rd32(SLOT1 + 0x44u), 0xD000u);
    {
        const Call* c = find_call("8c040", 0);

        chk("mvz.8c040", (u32)(c != 0), 1u);
        if (c)
            chk("mvz.8c040.v", c->a[0], SLOT1 + 0x28u); /* else-branch */
    }
    /* mid-state probe: the s16-r==0 test width — force r=0x10000
     * (nonzero 32-bit, zero low half) -> must RETRY */
    reset();
    s_f_90a84 = 0;
    s_f_95414[0] = 0x10000;
    s_f_95414[1] = 1;
    wr32(SLOT1 + 0x38u, 0x1000u);
    set_anim(1);
    r = run();
    chk("mvw.n95414", count_calls("95414"), 2u);

    /* ---- movement: vel zero -> idle anim arm ---- */
    reset();
    s_f_90a84 = 0;
    s_f_95414[0] = 1;
    set_anim(1); /* walking flag set while idle -> reset to 0 + 894c8 */
    wr16(G_CRUMB, 0u); /* gate closed: no commit */
    r = run();
    {
        const Call* c = find_call("245d8", 0);

        chk("mvi.245d8", (u32)(c != 0), 1u);
        if (c)
            chk("mvi.245d8.anim", c->a[1], 0u);
    }
    chk("mvi.894c8", count_calls("894c8"), 1u);
    chk("mvi.8c1dc", count_calls("8c1dc"), 0u);

    /* ---- ring wrap: index 0x1F -> 0 ---- */
    reset();
    s_f_90a84 = 0;
    s_f_95414[0] = 1;
    wr32(SLOT1 + 0x38u, 0x1000u);
    set_anim(1);
    wr16(G_RING_IDX, 0x1Fu);
    wr32(SC + 0x90u, 0x1000u);
    r = run();
    chk("wrap.ring", rd16(G_RING_IDX), 0u);
    chk("wrap.entry", rd32(G_RING), 0x1000u);

    /* ---- state 8: slot-2 handshake arms ---- */
    reset();
    wr16(SLOT1 + 0x20u, 8u);
    wr8(G_S2PRES, 0xFFu);
    r = run();
    chk("s8a.state", rd16(SLOT1 + 0x20u), 9u);
    chk("s8a.97770", count_calls("97770"), 0u);

    reset();
    wr16(SLOT1 + 0x20u, 8u);
    wr8(G_S2PRES, 0u);
    wr8(G_BUSY2, 0u);
    wr8(G_AREA, 0x21u);
    r = run();
    chk("s8b.state", rd16(SLOT1 + 0x20u), 9u);
    chk("s8b.n97770", count_calls("97770"), 2u);
    {
        const Call* c = find_call("97770", 0);

        if (c) {
            chk("s8b.c0.slot", c->a[0], 2u);
            chk("s8b.c0.val", c->a[1], 1u);
        }
        c = find_call("97770", 1);
        if (c) {
            chk("s8b.c1.slot", c->a[0], 5u);
            chk("s8b.c1.val", c->a[1], 8u);
        }
    }
    chk("s8b.own86", rd16(SLOT1 + 0x86u), 0x21u);

    reset();
    wr16(SLOT1 + 0x20u, 8u);
    wr8(G_S2PRES, 0u);
    wr8(G_BUSY2, 1u);
    wr8(G_AREA, 0x22u);
    r = run();
    chk("s8c.state", rd16(SLOT1 + 0x20u), 9u);
    chk("s8c.n97770", count_calls("97770"), 1u);
    {
        const Call* c = find_call("97770", 0);

        if (c) {
            chk("s8c.c0.slot", c->a[0], 5u);
            chk("s8c.c0.val", c->a[1], 1u);
        }
    }
    /* pool-relative neighbor write, NOT own-record */
    chk("s8c.pool286", rd16(POOL_GUEST + 0x286u), 0x22u);
    chk("s8c.own86", rd16(SLOT1 + 0x86u), 0u);

    /* ---- state 0xA: slot-4 claim hold vs go ---- */
    reset();
    wr16(SLOT1 + 0x20u, 0xAu);
    wr8(G_S4PRES, 0u);
    s_f_97770 = 0;
    r = run();
    chk("sa_hold.state", rd16(SLOT1 + 0x20u), 0xAu);
    reset();
    wr16(SLOT1 + 0x20u, 0xAu);
    wr8(G_S4PRES, 0u);
    s_f_97770 = 1;
    r = run();
    chk("sa_go.state", rd16(SLOT1 + 0x20u), 0xDu);

    /* ---- state 0xD: approach setup ---- */
    reset();
    wr16(SLOT1 + 0x20u, 0xDu);
    wr8(G_AREA, 3u);
    wr32(POOL_GUEST + 3u * 128u + 0x28u, 0xABC000u);
    wr32(POOL_GUEST + 3u * 128u + 0x30u, 0xDEF000u);
    r = run();
    chk("sd.state", rd16(SLOT1 + 0x20u), 0xEu);
    chk("sd.t50", rd32(SLOT1 + 0x50u), 0xABCu);
    chk("sd.t54", rd32(SLOT1 + 0x54u), 0xDEFu);
    {
        const Call* c = find_call("941c4", 0);

        if (c)
            chk("sd.941c4.b", c->a[1], POOL_GUEST + 3u * 128u + 0x28u);
    }

    /* ---- state 0xE: approach step (8bec8==3 advances) ---- */
    reset();
    wr16(SLOT1 + 0x20u, 0xEu);
    s_f_8bec8 = 3;
    r = run();
    chk("se.state", rd16(SLOT1 + 0x20u), 0xFu);
    {
        const Call* c = find_call("8c1dc", 0);

        if (c)
            chk("se.8c1dc.ctx", c->a[0], 0x2Fu); /* slot_idx 1 + 0x2E */
    }
    reset();
    wr16(SLOT1 + 0x20u, 0xEu);
    s_f_8bec8 = 2;
    r = run();
    chk("se_hold.state", rd16(SLOT1 + 0x20u), 0xEu);

    /* ---- state 0x12: teleport + full ring refill ---- */
    reset();
    wr16(SLOT1 + 0x20u, 0x12u);
    wr16(G_WARPX, 0x321u);
    wr16(G_WARPZ, 0x654u);
    wr16(G_WARPH, 0x900u);
    wr16(G_RING_IDX, 0x15u);
    wr32(SC + 0x3Cu, 0x5A5A5A5Au); /* stale word copied faithfully */
    r = run();
    chk("s12.state", rd16(SLOT1 + 0x20u), 0xDu);
    chk("s12.idx", rd16(G_RING_IDX), 0u);
    chk("s12.e0x", rd32(G_RING + 0u), 0x321u << 12);
    chk("s12.e0y", rd32(G_RING + 4u), (u32)s_f_93978);
    chk("s12.e0z", rd32(G_RING + 8u), 0x654u << 12);
    chk("s12.e0w", rd32(G_RING + 12u), 0x5A5A5A5Au);
    chk("s12.e0h", rd16(G_RING + 0x10u), 0x900u);
    chk("s12.e31x", rd32(G_RING + 31u * 0x14u), 0x321u << 12);
    chk("s12.e31h", rd16(G_RING + 31u * 0x14u + 0x10u), 0x900u);

    /* ---- state 0x2A: ring reset from slot pos ---- */
    reset();
    wr16(SLOT1 + 0x20u, 0x2Au);
    wr8(G_RESYNC, 0x55u);
    wr16(G_RING_IDX, 7u);
    r = run();
    chk("s2a.state", rd16(SLOT1 + 0x20u), 0x2Bu);
    chk("s2a.resync", rd8(G_RESYNC), 0u);
    chk("s2a.lap", rd32(SLOT1 + 0x58u), 1u);
    chk("s2a.idx", rd16(G_RING_IDX), 0u);
    chk("s2a.e0", rd32(G_RING), 0x5000u << 12);
    chk("s2a.e31", rd32(G_RING + 31u * 0x14u), 0x5000u << 12);

    /* ---- state 0x2B: mode-wait arms ---- */
    reset();
    wr16(SLOT1 + 0x20u, 0x2Bu);
    wr32(G_MODE_WORD, 1u);
    r = run();
    chk("s2b1.state", rd16(SLOT1 + 0x20u), 1u);
    reset();
    wr16(SLOT1 + 0x20u, 0x2Bu);
    wr32(G_MODE_WORD, 2u);
    wr8(G_BUSY2, 1u);
    r = run();
    chk("s2b2h.state", rd16(SLOT1 + 0x20u), 0x2Bu);
    reset();
    wr16(SLOT1 + 0x20u, 0x2Bu);
    wr32(G_MODE_WORD, 3u);
    wr8(G_BUSY2, 0u);
    wr8(G_BUSY3, 0u);
    r = run();
    chk("s2b3.state", rd16(SLOT1 + 0x20u), 0x2Cu);
    reset();
    wr16(SLOT1 + 0x20u, 0x2Bu);
    wr32(G_MODE_WORD, 0u);
    r = run();
    chk("s2b0.state", rd16(SLOT1 + 0x20u), 0x2Bu);

    /* ---- state 0x2D: -> 0x40 both arms ---- */
    reset();
    wr16(SLOT1 + 0x20u, 0x2Du);
    wr8(G_S3PRES, 0xFFu);
    r = run();
    chk("s2da.state", rd16(SLOT1 + 0x20u), 0x40u);
    reset();
    wr16(SLOT1 + 0x20u, 0x2Du);
    wr8(G_S3PRES, 0u);
    s_f_97770 = 1;
    r = run();
    chk("s2db.state", rd16(SLOT1 + 0x20u), 0x40u);
    reset();
    wr16(SLOT1 + 0x20u, 0x2Du);
    wr8(G_S3PRES, 0u);
    s_f_97770 = 0;
    r = run();
    chk("s2dh.state", rd16(SLOT1 + 0x20u), 0x2Du);

    /* ---- state 2: follow slot-7 record ---- */
    reset();
    wr16(SLOT1 + 0x20u, 2u);
    wr32(POOL_GUEST + 0x3A8u, 0x777000u);
    wr16(POOL_GUEST + 0x3C8u, 0x123u);
    r = run();
    chk("s2.x", rd32(SLOT1 + 0x28u), 0x777000u);
    chk("s2.h", rd16(SLOT1 + 0x48u), 0x123u);

    if (s_failures != 0) {
        fprintf(stderr, "FAILURES=%d\n", s_failures);
        return 1;
    }
    printf("W34B24-I8 0x8008A72C focused oracle PASS\n");
    return 0;
}
