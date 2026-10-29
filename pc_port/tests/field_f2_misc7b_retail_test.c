/* Retail differential, src/field/main/misc7.c hand-written arms (batch b):
 * func_8009A34C (screen-scale tween), func_8009AEE0 (move one party member),
 * func_8009B398 (move the party), func_8009B7A8 (yaw nudge).  The production
 * TU is linked unmodified (same-TU FieldGetVec{1,2}Magnitude and, for
 * func_8009B398, func_8009AEE0 are spied through WEAKEN); see
 * lib/field_retail_core.h.
 * Boundary notes:
 *  - func_8007B694 gets a stack VECTOR in func_8009AEE0; the callee reads
 *    words 0..2 only, so the two event words recorded past them (retail
 *    stack slots / native frame) are zeroed on both sides.
 *  - func_8009E574 events also record g_FieldScriptVMCurActor and
 *    D_800AFD1C at call time (the context retail lends the callee).
 * Coverage limit: direction is kept to 0..7 / 0xFF (g_FieldAngleToDirectionLUT
 * has 8 entries); party actor indices to 0..7 / 0xFF. */
#define TESTNAME "FIELD F2 MISC7B"
enum { S_ARG1 = 1, S_ARG2, S_ARG3, S_ARG4, S_ARG5, S_ARG6, S_GETARG, S_VEC1, S_VEC2, S_B694, S_E574, S_AEE0 };
#include <stdint.h>
static int32_t feed_map(uint32_t id, int32_t raw);
#define FEED_MAP(id, raw) feed_map(id, raw)
#define BUF_BASE 0x80120000u
#define BUF_SIZE 0x60000u
#include "field_retail_core.h"

#define SCRIPT 0x80120000u
#define CURACT 0x80140000u
#define FACTORS 0x80150000u
#define ADATA 0x80160000u
#define SPR 0x80170000u
#define NACT 8u

#define RK_BASE 0x8005A440u
#define RK_SIZE 0x10u
#define RT_BASE 0x800AEA30u
#define RT_SIZE 0x20u
#define RS_BASE 0x800AF990u
#define RS_SIZE 0x140u
#define RC_BASE 0x800AFB10u
#define RC_SIZE 0x10u
#define RH_BASE 0x800AFD00u
#define RH_SIZE 0x40u
#define RO_BASE 0x800B0070u
#define RO_SIZE 0x60u
#define RB_BASE 0x800B21C0u
#define RB_SIZE 0x20u
#define RL_BASE 0x800B2340u
#define RL_SIZE 0x10u
REGION(RK); REGION(RT); REGION(RS); REGION(RC); REGION(RH); REGION(RO); REGION(RB); REGION(RL);
ALIAS(D_8005A444, RK, 0x8005A444);
ALIAS(g_FieldAngleToDirectionLUT, RT, 0x800AEA34);
ALIAS(g_Scene, RS, 0x800AF990);
ALIAS(D_800AFD1C, RH, 0x800AFD1C);
ALIAS(D_800B00C0, RO, 0x800B00C0);
ALIAS(D_800B21CC, RB, 0x800B21CC); ALIAS(D_800B21CE, RB, 0x800B21CE); ALIAS(D_800B21D8, RB, 0x800B21D8);
ALIAS(D_800B2348, RL, 0x800B2348);
uintptr_t g_FieldActors, g_FieldScriptVMCurActor, g_FieldScriptVMCurScriptData;
extern int32_t D_800AFD1C;
const Region regions[] = {
    { RK_BASE, RK_SIZE, RK }, { RT_BASE, RT_SIZE, RT }, { RS_BASE, RS_SIZE, RS }, { RC_BASE, RC_SIZE, RC },
    { RH_BASE, RH_SIZE, RH }, { RO_BASE, RO_SIZE, RO }, { RB_BASE, RB_SIZE, RB }, { RL_BASE, RL_SIZE, RL },
};
const unsigned NREG = sizeof(regions) / sizeof(regions[0]);
const PtrGlobal ptrglobals[] = {
    { 0x800AFB10u, &g_FieldActors }, { 0x800B0078u, &g_FieldScriptVMCurActor }, { 0x800ADC00u, &g_FieldScriptVMCurScriptData },
};
const unsigned NPTR = sizeof(ptrglobals) / sizeof(ptrglobals[0]);

static int32_t feed_map(uint32_t id, int32_t raw) {
    Event* e = &trace.ev[trace.n - 1];
    if (id == S_B694) { e->a[4] = 0; e->a[5] = 0; }
    if (id == S_E574) {
        e->a[2] = active_cpu ? r32(0x800B0078u) : (uint32_t)g_FieldScriptVMCurActor;
        e->a[3] = active_cpu ? r32(0x800AFD1Cu) : (uint32_t)D_800AFD1C;
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
SPY(FieldGetVec1Magnitude, S_VEC1)
SPY(FieldGetVec2Magnitude, S_VEC2)
SPY(func_8007B694, S_B694)
SPY(func_8009E574, S_E574)
SPY(func_8009AEE0, S_AEE0)
/* func_8009E574 is declared ret=1 only so FEED_MAP runs at the call (context
 * capture); both callers ignore v0 (retail overwrites it before any use). */
const Target targets[] = {
    { 0x8009CF78, S_ARG1, 2, 1, 0, 0 }, { 0x8009CFBC, S_ARG2, 2, 1, 0, 0 }, { 0x8009D000, S_ARG3, 2, 1, 0, 0 },
    { 0x8009D044, S_ARG4, 2, 1, 0, 0 }, { 0x8009D088, S_ARG5, 2, 1, 0, 0 }, { 0x8009D0CC, S_ARG6, 2, 1, 0, 0 },
    { 0x800ACDEC, S_GETARG, 1, 1, 0, 0 }, { 0x80099A8C, S_VEC1, 1, 1, 0, 0 }, { 0x80099A4C, S_VEC2, 2, 1, 0, 0 },
    { 0x8007B694, S_B694, 1, 1, 0, 0, 0, 0x1 }, { 0x8009E574, S_E574, 2, 1, 0, 0 },
    { 0x8009AEE0, S_AEE0, 4, 1, 0, 0 },
};
const unsigned NTARGETS = sizeof(targets) / sizeof(targets[0]);

extern void func_8009A34C(void);
extern int32_t real_func_8009AEE0(int32_t, int32_t, int32_t, int32_t);
extern void func_8009B398(void);
extern void func_8009B7A8(int32_t, int32_t);
static uint32_t n_A34C(Args a) { (void)a; func_8009A34C(); return 0; }
static uint32_t n_AEE0(Args a) { return (uint32_t)real_func_8009AEE0((int32_t)a.a0, (int32_t)a.a1, (int32_t)a.a2, (int32_t)a.a3); }
static uint32_t n_B398(Args a) { (void)a; func_8009B398(); return 0; }
static uint32_t n_B7A8(Args a) { func_8009B7A8((int32_t)a.a0, (int32_t)a.a1); return 0; }

static uint32_t cur;
static uint16_t ip;
static void setup(void) {
    w32(0x800AFB10u, FACTORS);
    for (unsigned k = 0; k < NACT; k++) {
        w32(FACTORS + k * 0x5C + 0x04, SPR + k * 0x100);
        w32(FACTORS + k * 0x5C + 0x4C, ADATA + k * 0x200);
    }
    w32(0x800AFD1C, rnd_range(NACT));
    cur = CURACT;
    w32(0x800B0078u, cur);
    w32(0x800ADC00u, SCRIPT);
    ip = (uint16_t)((rnd() & 3) ? rnd_range(0x8000) : rnd());
    w16(cur + 0xCC, ip);
    for (unsigned i = 0; i < 3; i++) w32(0x8005A444 + 4 * i, (rnd() % 6 == 0) ? 0xFF : rnd_range(NACT));
}

static void g_A34C(Args* a) {
    (void)a; setup();
    if (rnd() % 3 == 0) w8(SCRIPT + (uint16_t)(ip + 3), 0);
    if (rnd() & 1) feed[0] = small(-0x400, 0x400);
}

static void party_actor(unsigned k) {
    uint32_t f = FACTORS + k * 0x5C, ad = ADATA + k * 0x200, sp = SPR + k * 0x100;
    if (rnd() % 3) w16(f + 0x58, (uint16_t)(r32(f + 0x58) & ~0x20u));
    if (rnd() & 1) w32(sp + 0x18, 0);
    static const uint16_t speeds[] = { 0, 1, 0x40, 0x100, 0x8000, 0xFFFF };
    if (rnd() & 1) w16(ad + 0x76, speeds[rnd_range(6)]);
    for (unsigned o = 0x20; o <= 0x28; o += 4)
        if (rnd() & 1) w32(ad + o, (uint32_t)small(-0x400, 0x400) << 16 | (rnd() & 0xFFFF));
    if (rnd() & 1) { w16(ad + 0x68, r32(ad + 0x20) >> 16); w16(ad + 0x6A, r32(ad + 0x24) >> 16); w16(ad + 0x6C, r32(ad + 0x28) >> 16); }
    static const uint16_t c6e[] = { 0, 0x3F, 0x40, 0x41, 0x7FFF, 0x8000, 0xFFFF };
    if (rnd() % 4) w16(ad + 0x6E, c6e[rnd_range(7)]);
    if (rnd() & 1) w32(ad, r32(ad) & ~0x8000u);
}

static void g_AEE0(Args* a) {
    setup();
    a->a0 = rnd_range(3);
    for (unsigned k = 0; k < NACT; k++) party_actor(k);
    a->a1 = (uint32_t)((rnd() & 1) ? rnd_val() : small(-0x400, 0x400));
    a->a2 = (uint32_t)((rnd() & 1) ? rnd_val() : small(-0x400, 0x400));
    a->a3 = (rnd() & 1) ? 0xFF : rnd_range(8);
    if (rnd() & 1) w16(0x800B2348, 0);
    for (unsigned i = 0; i < 4; i++) {
        unsigned k = rnd_range(4);
        if (k == 0) feed[i] = small(-4, 0x40);
        else if (k == 1) feed[i] = small(0, 0x400);
    }
}

static void g_B398(Args* a) {
    (void)a; setup();
    if (rnd() % 3 == 0) feed[0] = 0x7FFF;
    for (unsigned k = 0; k < NACT; k++) if (rnd() & 1) w16(ADATA + k * 0x200 + 0x106, (uint16_t)rnd());
    for (unsigned i = 4; i <= 12; i += 4) feed[i] = (rnd() % 8 == 0) ? rnd_val() : (rnd() & 1) ? 0 : -1;
    if (rnd() & 1) { feed[4] = feed[8] = feed[12] = 0; }
}

static void g_B7A8(Args* a) {
    a->a0 = (rnd() & 1) ? 0 : rnd();
    static const int32_t d[] = { 0, 0, 1, -1, 2, 7, 0x100, -3 };
    a->a1 = (rnd() & 1) ? (uint32_t)d[rnd_range(8)] : (uint32_t)rnd_val();
}

Fn fns[] = {
    { "func_8009A34C", 0x8009A34C, 53, n_A34C, g_A34C, 3000, 0 },
    { "func_8009AEE0", 0x8009AEE0, 159, n_AEE0, g_AEE0, 8000, 1 },
    { "func_8009B398", 0x8009B398, 179, n_B398, g_B398, 4000, 0 },
    { "func_8009B7A8", 0x8009B7A8, 31, n_B7A8, g_B7A8, 3000, 0 },
};
const unsigned NFNS = sizeof(fns) / sizeof(fns[0]);
