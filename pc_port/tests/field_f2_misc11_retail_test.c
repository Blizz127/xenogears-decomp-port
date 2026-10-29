/* Retail differential, src/field/main/misc11.c hand-written arms:
 * func_80091ADC 80091BBC 80092148 800921E8 80092894.
 * The production TU is linked unmodified; see lib/field_retail_core.h.
 *
 * Callee-argument normalisation (both sides identically, callee-type domain):
 *  - func_80091ADC(s16 x6, s32): args packed as 16-bit pairs + the 32-bit
 *    doMove (7th arg, retail stack 0x18) -- the core records only 6 words.
 *  - GfxLineScrollInitialize(ptr, u16,u16,u16,s16,s16,u16,u16, ptr): 9 args,
 *    the 7 16-bit ones packed in pairs (retail stack 0x18..0x20 read here).
 *  - func_8007B694(s32* v): stack pointer recorded as 0xFFFFFFFE, v[0..2]
 *    recorded (callee reads v[0], v[2]).
 *  - MoveImage/ClearImage rect: native host pointer into a mirrored global
 *    region is translated back to its guest address.
 * The oracle step budget is raised (fill loop of up to 0xFFFF bytes). */
#define TESTNAME "FIELD F2 MISC11"
enum { S_ARG1 = 1, S_ARG2, S_ARG3, S_ARG4, S_ARG5, S_ARG6, S_ADC, S_MOVE, S_CLEAR, S_GETARG, S_GETIARG,
       S_HEAP, S_GLSI, S_V1, S_V2, S_F44, S_WU16, S_RSIN, S_RCOS, S_821F4, S_7B694 };
#include <stdint.h>
static int32_t feed_map(uint32_t id, int32_t raw);
#define FEED_MAP(id, raw) feed_map(id, raw)

#define BUF_BASE 0x80120000u
#define BUF_SIZE 0x60000u
#include "battle_mips_adapter.h"
#define PcPortMipsRun(c, e, h, n) (PcPortMipsRun)(c, e, h, 2000000)
#include "field_retail_core.h"

#define SCRIPT 0x80120000u
#define CURACT 0x80128000u
#define FACTORS 0x80129000u
#define SPRITE 0x8012A000u
#define B06B8 0x8012C000u
#define ADATA 0x80140000u
#define HEAPB 0x80150000u
#define NACT 16u

/* globals regions (retail symbol names alias into them) */
#define RMI_BASE 0x800AF5E8u /* D_800AF5E8/6E8/728/768: PC also co-locates them (g_FieldMoveImageBss) */
#define RMI_SIZE 0x184u
#define RLS_BASE 0x800AFE9Cu /* D_800AFEA8 table (+0x104 size[] reached only as D_800AFEA8+0x104) */
#define RLS_SIZE 0x150u
#define RA_BASE 0x800ADB80u
#define RA_SIZE 0x90u
#define RB_BASE 0x800B0070u
#define RB_SIZE 0x60u
#define RC_BASE 0x800B06B0u
#define RC_SIZE 0x10u
#define RD_BASE 0x800B2170u
#define RD_SIZE 0x10u
#define RE_BASE 0x800B2260u
#define RE_SIZE 0x10u
#define RF_BASE 0x8004F340u
#define RF_SIZE 0x10u
#define RG_BASE 0x800AFB10u
#define RG_SIZE 0x10u
REGION(RMI); REGION(RLS); REGION(RA); REGION(RB); REGION(RC); REGION(RD); REGION(RE); REGION(RF); REGION(RG);
ALIAS(D_800AF5E8, RMI, 0x800AF5E8); ALIAS(D_800AF6E8, RMI, 0x800AF6E8);
ALIAS(D_800AF728, RMI, 0x800AF728); ALIAS(D_800AF768, RMI, 0x800AF768);
ALIAS(D_800AFEA8, RLS, 0x800AFEA8);
ALIAS(D_800ADB8C, RA, 0x800ADB8C); ALIAS(D_800ADBDC, RA, 0x800ADBDC); ALIAS(D_800ADBE4, RA, 0x800ADBE4);
ALIAS(D_800ADBEC, RA, 0x800ADBEC);
ALIAS(D_800B00C0, RB, 0x800B00C0);
ALIAS(g_FieldControl, RD, 0x800B2176);
ALIAS(g_PlayerActorIndex, RE, 0x800B226C);
ALIAS(g_GameSceneMapNum, RF, 0x8004F34C);
uintptr_t g_FieldActors, g_FieldScriptVMCurActor, g_FieldScriptVMCurScriptData, D_800B06B8;

const Region regions[] = {
    { RMI_BASE, RMI_SIZE, RMI }, { RLS_BASE, RLS_SIZE, RLS }, { RA_BASE, RA_SIZE, RA }, { RB_BASE, RB_SIZE, RB },
    { RC_BASE, RC_SIZE, RC }, { RD_BASE, RD_SIZE, RD }, { RE_BASE, RE_SIZE, RE }, { RF_BASE, RF_SIZE, RF },
    { RG_BASE, RG_SIZE, RG },
};
const unsigned NREG = sizeof(regions) / sizeof(regions[0]);
const PtrGlobal ptrglobals[] = {
    { 0x800AFB10u, &g_FieldActors }, { 0x800B0078u, &g_FieldScriptVMCurActor },
    { 0x800ADC00u, &g_FieldScriptVMCurScriptData }, { 0x800B06B8u, &D_800B06B8 },
};
const unsigned NPTR = sizeof(ptrglobals) / sizeof(ptrglobals[0]);

static uint32_t guestp(uintptr_t p) {
    for (unsigned r = 0; r < NREG; r++)
        if (p >= (uintptr_t)regions[r].nat && p < (uintptr_t)regions[r].nat + regions[r].size)
            return regions[r].base + (uint32_t)(p - (uintptr_t)regions[r].nat);
    return (uint32_t)p;
}
static uint32_t pk(uint32_t lo, uint32_t hi) { return (lo & 0xFFFFu) | (hi << 16); }

static int32_t feed_map(uint32_t id, int32_t raw) {
    if (id == S_HEAP) return (int32_t)(HEAPB + ((uint32_t)raw & 0x1FFF0u));
    if (active_cpu) { /* retail side: repack the event in the callee-type domain */
        Event* e = &trace.ev[trace.n - 1];
        uint32_t sp = active_cpu->gpr[29];
        if (id == S_ADC) {
            uint32_t a[6]; memcpy(a, e->a, sizeof(a));
            e->a[0] = pk(a[0], a[1]); e->a[1] = pk(a[2], a[3]); e->a[2] = pk(a[4], a[5]);
            e->a[3] = r32(sp + 0x18); e->a[4] = e->a[5] = 0;
        } else if (id == S_GLSI) {
            uint32_t a[6]; memcpy(a, e->a, sizeof(a));
            e->a[0] = a[0]; e->a[1] = pk(a[1], a[2]); e->a[2] = pk(a[3], a[4]);
            e->a[3] = pk(a[5], r32(sp + 0x18)); e->a[4] = r32(sp + 0x1C) & 0xFFFFu; e->a[5] = r32(sp + 0x20);
        } else if (id == S_7B694) {
            uint32_t p = e->a[0];
            e->a[0] = p >= 0x801FE000u ? 0xFFFFFFFEu : p;
            e->a[1] = r32(p); e->a[2] = r32(p + 4); e->a[3] = r32(p + 8);
        }
    }
    return raw;
}

SPY(FieldScriptArgument1, S_ARG1)
SPY(FieldScriptArgument2, S_ARG2)
SPY(FieldScriptArgument3, S_ARG3)
SPY(FieldScriptArgument4, S_ARG4)
SPY(FieldScriptArgument5, S_ARG5)
SPY(FieldScriptArgument6, S_ARG6)
SPY(FieldScriptVMGetArgument, S_GETARG)
SPY(FieldScriptVMGetInstructionArgument, S_GETIARG)
SPY(HeapAlloc, S_HEAP)
SPY(FieldGetVec1Magnitude, S_V1)
SPY(FieldGetVec2Magnitude, S_V2)
SPY(func_80092F44, S_F44)
SPY(FieldScriptMemoryWriteU16, S_WU16)
/* misc11 includes the psyq/libgte shim: game `rsin` (0x8003F8CC) is host `rcos`
 * and game `rcos` (0x8003F8B0) is host `rsin`. */
SPY(rcos, S_RSIN)
SPY(rsin, S_RCOS)
SPY(func_800821F4, S_821F4)
long func_80091ADC(long a, long b, long c, long d, long e, long f, long g) {
    uint64_t in[6] = { pk((uint32_t)a, (uint32_t)b), pk((uint32_t)c, (uint32_t)d), pk((uint32_t)e, (uint32_t)f),
                       (uint32_t)g, 0, 0 };
    return spy_core(S_ADC, in);
}
long GfxLineScrollInitialize(long ls, long x, long y, long mod, long w, long n, long dx, long dy, long pd) {
    uint64_t in[6] = { (uint32_t)ls, pk((uint32_t)x, (uint32_t)y), pk((uint32_t)mod, (uint32_t)w),
                       pk((uint32_t)n, (uint32_t)dx), (uint32_t)dy & 0xFFFFu, (uint32_t)pd };
    return spy_core(S_GLSI, in);
}
long func_8007B694(long p) {
    uint64_t in[6] = { (uint64_t)p, 0, 0, 0, 0, 0 };
    long r = spy_core(S_7B694, in);
    Event* e = &trace.ev[trace.n - 1];
    e->a[0] = on_native_stack((uintptr_t)p) ? 0xFFFFFFFEu : (uint32_t)p;
    memcpy(&e->a[1], (void*)(uintptr_t)p, 12);
    return r;
}
long MoveImage(long r, long x, long y) {
    uint64_t in[6] = { guestp((uintptr_t)r), (uint64_t)x, (uint64_t)y, 0, 0, 0 };
    return spy_core(S_MOVE, in);
}
long ClearImage(long r, long cr, long cg, long cb) {
    uint64_t in[6] = { guestp((uintptr_t)r), (uint64_t)cr, (uint64_t)cg, (uint64_t)cb, 0, 0 };
    return spy_core(S_CLEAR, in);
}

const Target targets[] = {
    { 0x8009CF78, S_ARG1, 2, 1 }, { 0x8009CFBC, S_ARG2, 2, 1 }, { 0x8009D000, S_ARG3, 2, 1 },
    { 0x8009D044, S_ARG4, 2, 1 }, { 0x8009D088, S_ARG5, 2, 1 }, { 0x8009D0CC, S_ARG6, 2, 1 },
    { 0x80091ADC, S_ADC, 6, 1 }, { 0x8004495C, S_MOVE, 3, 0 }, { 0x80044764, S_CLEAR, 4, 0 },
    { 0x800ACDEC, S_GETARG, 1, 1 }, { 0x800ACDB8, S_GETIARG, 1, 1 }, { 0x80031BDC, S_HEAP, 2, 1 },
    { 0x80027D64, S_GLSI, 6, 1 }, { 0x80099A8C, S_V1, 1, 1 }, { 0x80099A4C, S_V2, 2, 1 },
    { 0x80092F44, S_F44, 0, 0 }, { 0x800A3074, S_WU16, 2, 0 }, { 0x8003F8CC, S_RSIN, 1, 1 },
    { 0x8003F8B0, S_RCOS, 1, 1 }, { 0x800821F4, S_821F4, 3, 0 }, { 0x8007B694, S_7B694, 1, 1 },
};
const unsigned NTARGETS = sizeof(targets) / sizeof(targets[0]);

/* ---- native entry wrappers --------------------------------------------- */
extern void real_func_80091ADC(int16_t, int16_t, int16_t, int16_t, int16_t, int16_t, int32_t);
extern void func_80091BBC(void);
extern void func_80092148(void);
extern void func_800921E8(void);
extern int32_t func_80092894(int32_t, int32_t, int32_t, int32_t);
static uint32_t adc_mode, adc_do;
static uint32_t n_1ADC(Args a) {
    real_func_80091ADC((int16_t)a.a0, (int16_t)a.a1, (int16_t)a.a2, (int16_t)a.a3, (int16_t)a.a4,
                       (int16_t)adc_mode, (int32_t)adc_do);
    return 0;
}
static uint32_t n_1BBC(Args a) { (void)a; func_80091BBC(); return 0; }
static uint32_t n_2148(Args a) { (void)a; func_80092148(); return 0; }
static uint32_t n_21E8(Args a) { (void)a; func_800921E8(); return 0; }
static uint32_t n_2894(Args a) {
    return (uint32_t)func_80092894((int32_t)a.a0, (int32_t)a.a1, (int32_t)a.a2, (int32_t)a.a3);
}

static void set_cur(void) {
    w32(0x800B0078u, CURACT); w32(0x800ADC00u, SCRIPT);
    w16(CURACT + 0xCC, (uint16_t)rnd_range(0x7000));
}
static void g_1ADC(Args* a) {
    a->a0 = (uint32_t)rnd_val(); a->a1 = (uint32_t)rnd_val(); a->a2 = (uint32_t)rnd_val(); a->a3 = (uint32_t)rnd_val();
    a->a4 = (uint32_t)rnd_val();
    adc_mode = (uint32_t)rnd_val();
    adc_do = (rnd() & 1) ? 0 : (uint32_t)rnd_val();
    if (rnd() % 8 == 0) adc_do = 0x10000; /* only the full 32-bit test sees it */
    w32(STACK + 20, adc_mode); w32(STACK + 24, adc_do);
}
static void g_1BBC(Args* a) {
    (void)a; set_cur();
    w8(SCRIPT + (r32(CURACT + 0xCC) & 0xFFFF) + 0xD, (uint8_t)rnd()); /* argument mask byte */
    if (rnd() % 3) feed[0] = 0;
    if (rnd() % 3) feed[1] = 0;
}
static void g_2148(Args* a) {
    (void)a; set_cur();
    int32_t idx = (int32_t)rnd_range(32);
    feed[0] = idx;
    int32_t a3 = small(-0x100, 0x8000);
    if (rnd() % 4 == 0) a3 = small(-4, 40);
    feed[1] = a3;
    w32(RLS_BASE + 0xC + 0x84 + 4 * idx, HEAPB + rnd_range(0x10000));
    int16_t sz = (int16_t)rnd_val();
    if (rnd() % 3 == 0) sz = (int16_t)(a3 + small(-1, 1));
    w16(RLS_BASE + 0xC + 0x104 + 2 * idx, (uint16_t)sz);
}
static void g_21E8(Args* a) {
    (void)a; set_cur();
    w32(0x800ADB8C, (rnd() % 6 == 0) ? (uint32_t)rnd_val() : 0);
    int16_t cnt = (int16_t)rnd_range(32);
    unsigned k = rnd() % 16;
    if (k == 0) cnt = (int16_t)small(0x20, 0x7FFF);
    else if (k == 1) cnt = (int16_t)small(-3, -2); /* -1 would clobber the count with lineScroll[-1] */
    else if (k == 2) cnt = 0x1F;
    w16(RLS_BASE + 0xC, (uint16_t)cnt);
    uint32_t n = rnd_range(0x200);
    unsigned m = rnd() % 32;
    if (m == 0) n = 0; else if (m == 1) n = 0x8000u + rnd_range(0x8000); else if (m == 2) n = rnd_range(0x8000);
    feed[0] = (int32_t)((rnd() << 16) | n); /* GetInstructionArgument(9): upper bits are discarded */
}
static void g_2894(Args* a) {
    w32(0x800AFB10u, FACTORS); w32(0x800B0078u, CURACT); w32(0x800B06B8u, B06B8);
    w16(CURACT + 0xCC, (uint16_t)rnd_range(0x7000));
    uint32_t pi = rnd_range(NACT);
    w32(0x800B226C, pi);
    for (unsigned k = 0; k < NACT; k++) {
        w32(FACTORS + k * 0x5C + 0x4C, ADATA + k * 0x1000);
        w32(FACTORS + k * 0x5C + 0x04, SPRITE + k * 0x40);
    }
    uint32_t ad = ADATA + pi * 0x1000;
    a->a0 = (rnd() & 1) ? (uint32_t)small(-0x800, 0x800) : (uint32_t)rnd_val();
    unsigned m = rnd() % 5;
    a->a1 = m == 0 ? 1u : m == 1 ? 2u : m == 2 ? (uint32_t)rnd_val() : 0u;
    a->a2 = (rnd() & 1) ? (uint32_t)small(-2000, 2000) : (uint32_t)rnd_val();
    a->a3 = (rnd() & 1) ? (uint32_t)small(-2000, 2000) : (uint32_t)rnd_val();
    w32(0x800ADBDC, (rnd() & 1) ? 0 : (uint32_t)rnd_val());
    w32(0x800ADBE4, (rnd() & 1) ? 0 : (uint32_t)rnd_val());
    w32(0x800ADBEC, (rnd() & 1) ? 0 : (uint32_t)rnd_val());
    if (rnd() & 1) {
        for (unsigned o = 0x22; o <= 0x2A; o += 4) w16(ad + o, (uint16_t)small(-1500, 1500));
        for (unsigned o = 0x60; o <= 0x64; o += 4) w16(CURACT + o, (uint16_t)small(-500, 500));
        w16(CURACT + 0x22, (uint16_t)small(-1500, 1500)); w16(CURACT + 0x2A, (uint16_t)small(-1500, 1500));
    }
    if (rnd() % 3) { w16(ad + 0x68, r32(ad + 0x22) & 0xFFFF); w16(ad + 0x6A, r32(ad + 0x26) & 0xFFFF); }
    if (rnd() % 3) w16(ad + 0x6C, r32(ad + 0x2A) & 0xFFFF);
    if (rnd() % 2) w16(ad + 0x6E, (uint16_t)small(0x3E, 0x42));
    /* callee returns: magnitudes small so both reached/not-reached occur; trig in range */
    for (unsigned i = 0; i < MAXEV; i++) {
        if (rnd() % 4) feed[i] = small(-0x1000, 0x1000);
    }
    if (rnd() % 3) {
        feed[0] = small(0, 0x200);
        for (unsigned i = 1; i < 10; i++) feed[i] = (rnd() & 1) ? small(0, 0x800) : small(-0x1000, 0x1000);
    }
}

Fn fns[] = {
    { "func_80091ADC", 0x80091ADC, 56, n_1ADC, g_1ADC, 1500, 0 },
    { "func_80091BBC", 0x80091BBC, 145, n_1BBC, g_1BBC, 800, 0 },
    { "func_80092148", 0x80092148, 40, n_2148, g_2148, 1500, 0 },
    { "func_800921E8", 0x800921E8, 127, n_21E8, g_21E8, 800, 0 },
    { "func_80092894", 0x80092894, 227, n_2894, g_2894, 3000, 1 },
};
const unsigned NFNS = sizeof(fns) / sizeof(fns[0]);
