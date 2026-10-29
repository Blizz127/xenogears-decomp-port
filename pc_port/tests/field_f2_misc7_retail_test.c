/* Retail differential, src/field/main/misc7.c hand-written arms (batch a):
 * func_80098A7C (place actor), func_80098CAC (linear move opcode),
 * func_80099214 (jump / gravity arc opcode).  The production TU is linked
 * unmodified (same-TU FieldGetVec{2,3}Magnitude are spied through WEAKEN);
 * see lib/field_retail_core.h.
 * Coverage limits:
 *  - func_80099214: the walkmesh layer index (FieldScriptArgument3 in the
 *    0x80 paths) is kept to 0..3 and D_800AFB54 to -1..5: retail indexes its
 *    4-row stack arrays with them, larger values address unrelated stack.
 *  - func_80099214 0x80 paths: the X/Z targets handed to func_8007B1C4 are
 *    kept inside s16.  Known DIVERGES (not fixable in misc7.c): retail passes
 *    the full words and its func_8007B1C4 uses the full a1 in
 *    `addu t0, a0<<16, a1`; the port's func_8007B1C4 (misc4.c) takes s16, so
 *    a target from an unsigned script variable >= 0x8000 reaches it
 *    truncated.  Every store made by func_80099214 itself only uses the low
 *    16 bits of X/Z, so the clamp hides nothing else.
 *  - curScriptIndex kept to 0..7 (ACTOR_MAX_NUM_SCRIPTS). */
#define TESTNAME "FIELD F2 MISC7A"
enum { S_ARG1 = 1, S_ARG2, S_ARG3, S_ARG4, S_VEC3, S_VEC2, S_RATAN2, S_821F4, S_81F80, S_SQRT, S_B1C4, S_B694 };
#include <stdint.h>
static int clamp_case;
static int32_t feed_map(uint32_t id, int32_t raw) {
    if (clamp_case) {
        if (id == S_ARG3) return raw & 3;
        if (id == S_ARG1 || id == S_ARG2) return (int16_t)raw;
    }
    return raw;
}
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

#define RC_BASE 0x800AFB10u
#define RC_SIZE 0x50u
#define RH_BASE 0x800AFD00u
#define RH_SIZE 0x40u
#define RO_BASE 0x800B0070u
#define RO_SIZE 0x60u
#define RP_BASE 0x800B06B0u
#define RP_SIZE 0x10u
REGION(RC); REGION(RH); REGION(RO); REGION(RP);
ALIAS(D_800AFB54, RC, 0x800AFB54);
ALIAS(D_800AFD1C, RH, 0x800AFD1C);
ALIAS(D_800B00C0, RO, 0x800B00C0);
uintptr_t g_FieldActors, g_FieldScriptVMCurActor, g_FieldScriptVMCurScriptData, D_800B06B8;
const Region regions[] = { { RC_BASE, RC_SIZE, RC }, { RH_BASE, RH_SIZE, RH }, { RO_BASE, RO_SIZE, RO }, { RP_BASE, RP_SIZE, RP } };
const unsigned NREG = sizeof(regions) / sizeof(regions[0]);
const PtrGlobal ptrglobals[] = {
    { 0x800AFB10u, &g_FieldActors }, { 0x800B0078u, &g_FieldScriptVMCurActor },
    { 0x800ADC00u, &g_FieldScriptVMCurScriptData }, { 0x800B06B8u, &D_800B06B8 },
};
const unsigned NPTR = sizeof(ptrglobals) / sizeof(ptrglobals[0]);

SPY(FieldScriptArgument1, S_ARG1)
SPY(FieldScriptArgument2, S_ARG2)
SPY(FieldScriptArgument3, S_ARG3)
SPY(FieldScriptArgument4, S_ARG4)
SPY(FieldGetVec3Magnitude, S_VEC3)
SPY(FieldGetVec2Magnitude, S_VEC2)
SPY(ratan2, S_RATAN2)
SPY(func_800821F4, S_821F4)
SPY(func_80081F80, S_81F80)
SPY(SquareRoot0, S_SQRT)
SPY(func_8007B1C4, S_B1C4)
SPY(func_8007B694, S_B694)
/* func_8007B1C4(x, z, layer, s16 out[4], s32 state[4]): 4 + 8 halfwords out */
const Target targets[] = {
    { 0x8009CF78, S_ARG1, 2, 1, 0, 0 }, { 0x8009CFBC, S_ARG2, 2, 1, 0, 0 }, { 0x8009D000, S_ARG3, 2, 1, 0, 0 },
    { 0x8009D044, S_ARG4, 2, 1, 0, 0 }, { 0x80099A04, S_VEC3, 3, 1, 0, 0 }, { 0x80099A4C, S_VEC2, 2, 1, 0, 0 },
    { 0x8004B32C, S_RATAN2, 2, 1, 0, 0 }, { 0x800821F4, S_821F4, 3, 0, 0, 0 }, { 0x80081F80, S_81F80, 3, 0, 0, 0 },
    { 0x80048C4C, S_SQRT, 1, 1, 0, 0 }, { 0x8007B1C4, S_B1C4, 5, 1, 0x18, 0, 0x84000u, 0 },
    { 0x8007B694, S_B694, 1, 1, 0, 0, 0, 0x1 },
};
const unsigned NTARGETS = sizeof(targets) / sizeof(targets[0]);

extern void func_80098A7C(void);
extern void func_80098CAC(int32_t);
extern void func_80099214(void);
static uint32_t n_8A7C(Args a) { (void)a; func_80098A7C(); return 0; }
static uint32_t n_8CAC(Args a) { func_80098CAC((int32_t)a.a0); return 0; }
static uint32_t n_9214(Args a) { (void)a; func_80099214(); return 0; }

static uint32_t cur, spr, fa;
static uint16_t ip;
static void setup(void) {
    clamp_case = 0;
    w32(0x800AFB10u, FACTORS);
    for (unsigned k = 0; k < NACT; k++) {
        w32(FACTORS + k * 0x5C + 0x04, SPR + k * 0x100);
        w32(FACTORS + k * 0x5C + 0x4C, ADATA + k * 0x200);
    }
    unsigned idx = rnd_range(NACT);
    w32(0x800AFD1C, idx);
    fa = FACTORS + idx * 0x5C;
    spr = SPR + idx * 0x100;
    cur = (rnd() & 1) ? ADATA + idx * 0x200 : CURACT;
    w32(0x800B0078u, cur);
    w32(0x800ADC00u, SCRIPT);
    w32(0x800B06B8u, (rnd() & 1) ? FACTORS : rnd());
    ip = (uint16_t)((rnd() & 3) ? rnd_range(0x8000) : rnd());
    w16(cur + 0xCC, ip);
    w8(cur + 0xCE, (uint8_t)rnd_range(8));
}
static uint8_t sb(int off) { return *gp(SCRIPT + (uint16_t)(ip + off)); }
static void wsb(int off, uint8_t v) { w8(SCRIPT + (uint16_t)(ip + off), v); }
static void small_pos(void) {
    for (unsigned o = 0x20; o <= 0x28; o += 4)
        if (rnd() & 1) w32(cur + o, (uint32_t)small(-0x400, 0x400) << 16 | (rnd() & 0xFFFF));
}

static void g_8A7C(Args* a) { (void)a; setup(); }

static void g_8CAC(Args* a) {
    setup();
    a->a0 = (rnd() & 1) ? rnd_range(2) : rnd();
    static const uint16_t speeds[] = { 0, 1, 2, 0x40, 0x100, 0x800, 0x7FFF, 0x8000, 0xFFFF };
    if (rnd() & 1) w16(cur + 0x76, speeds[rnd_range(9)]);
    if (rnd() & 1) wsb(1, 0);
    small_pos();
    uint32_t slot = cur + 0x8C + *gp(cur + 0xCE) * 8 + 4;
    unsigned m = rnd_range(4);
    if (m == 0) w16(slot, 0); else if (m == 1) w16(slot, 0xFFFF); else if (m == 2) w16(slot, (uint16_t)small(1, 5));
    if (rnd() & 1) w16(cur + 0x102, (uint16_t)small(-3, 5));
    if (rnd() & 1) w16(cur + 0xEA, 0xFF);
    if (rnd() & 1) w16(cur + 0xE8, (rnd() & 1) ? 1 : r32(cur + 0xE6) & 0xFFFF);
    if (rnd() & 1) w32(cur, r32(cur) & ~0x2000000u);
    for (unsigned i = 0; i < 8; i++) {
        unsigned k = rnd_range(5);
        if (k == 0) feed[i] = small(-8, 8);
        else if (k == 1) feed[i] = small(-0x400, 0x400);
    }
}

static void g_9214(Args* a) {
    (void)a; setup();
    unsigned m = rnd_range(6);
    uint8_t b1 = sb(1);
    if (m < 4) b1 = (uint8_t)((b1 & ~3u) | m);
    else if (m == 4) b1 = 0x0F;
    wsb(1, b1);
    /* rewind target opcode (mode 3 tick, completed): IP - 0xB */
    if ((rnd() & 1)) wsb(-0xB + 1, (uint8_t)(sb(-0xB + 1) & 0x7F));
    int rewind80 = (sb(-0xB + 1) & 0x80) != 0;
    clamp_case = ((b1 & 3) != 3 && (b1 & 0x80)) || ((b1 & 3) == 3 && rewind80);
    static const int16_t counts[] = { -1, 0, 1, 2, 3, 4, 5 };
    w16(0x800AFB54, (uint16_t)counts[rnd_range(7)]);
    small_pos();
    if (rnd() & 1) w16(cur + 0x102, (uint16_t)small(-2, 6));
    if (rnd() & 1) w16(cur + 0xE0, (uint16_t)small(-2, 6));
    if (rnd() & 1) w32(cur + 0xD0, 0);
    if (rnd() & 1) w32(cur + 0xD8, 0);
    if (rnd() & 1) w32(cur, r32(cur) & ~0x8000u);
    if (rnd() & 1) w32(spr + 0x1C, (uint32_t)small(-0x40000, 0x40000));
    for (unsigned i = 0; i < 12; i++) {
        unsigned k = rnd_range(6);
        if (k == 0) feed[i] = small(-3, 3);
        else if (k == 1) feed[i] = small(-0x400, 0x400);
        else if (k == 2) feed[i] = -1;
    }
}

Fn fns[] = {
    { "func_80098A7C", 0x80098A7C, 97, n_8A7C, g_8A7C, 2000, 0 },
    { "func_80098CAC", 0x80098CAC, 346, n_8CAC, g_8CAC, 6000, 0 },
    { "func_80099214", 0x80099214, 475, n_9214, g_9214, 8000, 0 },
};
const unsigned NFNS = sizeof(fns) / sizeof(fns[0]);
