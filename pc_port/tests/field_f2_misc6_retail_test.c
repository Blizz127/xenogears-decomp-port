/* Retail differential, src/field/main/misc6.c port arms (batch a):
 * func_8009E574 (halfword-range positions) 8009E810 8009F5F4 8009FA54.
 * The production TU is linked unmodified; see lib/field_retail_core.h.
 *
 * func_8009E810 keeps a deliberate port-only block (MAP16 follow-cam fix):
 * NATIVE_POST verifies that the native body made EXACTLY those four extra
 * stores (FieldActor +0x24/+0x44 = arg, sprite +0x84 = (u16)arg, sprite +0x04
 * = arg<<16) and only then restores those 14 bytes, so every other byte, the
 * call trace and the retail stores are still compared 1:1.  It is reported
 * as DIVERGES, not proven.
 *
 * Coverage limits (retail-inherent, not generated):
 *  - func_8009E574: D_800AFB54-1 > 4 overflows retail's 4-row stack arrays and
 *    a walkmeshId outside the filled rows reads uninitialised stack; both are
 *    kept inside the rows func_8007B1C4 filled.  x/z are halfword-range here
 *    (func_8007B1C4 still has an s16 port prototype); full-word x/z are
 *    covered by field_f2_misc6c_retail_test.c.
 *  - func_8009F5F4: XENO_TEST_INPUT is unset (test-input prologue inactive). */
#define TESTNAME "FIELD F2 MISC6A"
enum { S_7B1C4 = 1, S_80968, S_79288, S_81F5C, S_9E574, S_GETVAR };
#include <stdint.h>
static int32_t feed_map(uint32_t id, int32_t raw);
#define FEED_MAP(id, raw) feed_map(id, raw)
static void native_pre(void);
static void native_post(void);
#define NATIVE_PRE() native_pre()
#define NATIVE_POST() native_post()
#define BUF_BASE 0x80120000u
#define BUF_SIZE 0x60000u
#include "field_retail_core.h"

#define SCRIPT 0x80120000u
#define CURACT 0x80130000u
#define FACTORS 0x80131000u
#define SPR 0x80132000u
#define NACT 16u

#define RA_BASE 0x800ADB00u
#define RA_SIZE 0x110u
#define RB_BASE 0x800ADF60u
#define RB_SIZE 0x50u
#define RD_BASE 0x800AF980u
#define RD_SIZE 0x150u
#define RE_BASE 0x800AFB10u
#define RE_SIZE 0x50u
#define RF_BASE 0x800AFD10u
#define RF_SIZE 0x20u
#define RG_BASE 0x800AFE90u
#define RG_SIZE 0x10u
#define RI_BASE 0x800B0070u
#define RI_SIZE 0x60u
#define RJ_BASE 0x800B2170u
#define RJ_SIZE 0x240u
#define RK_BASE 0x800C2690u
#define RK_SIZE 0x1270u
REGION(RA); REGION(RB); REGION(RD); REGION(RE); REGION(RF); REGION(RG); REGION(RI); REGION(RJ); REGION(RK);
ALIAS(D_800ADB02, RA, 0x800ADB02); ALIAS(D_800ADB28, RA, 0x800ADB28); ALIAS(D_800ADB64, RA, 0x800ADB64);
ALIAS(D_800ADB68, RA, 0x800ADB68);
ALIAS(D_800ADF68, RB, 0x800ADF68); ALIAS(D_800ADF88, RB, 0x800ADF88);
ALIAS(g_CamInterpolation, RD, 0x800AF984); ALIAS(g_Scene, RD, 0x800AF990);
ALIAS(D_800AFB54, RE, 0x800AFB54);
ALIAS(D_800AFD1C, RF, 0x800AFD1C);
ALIAS(D_800AFE9C, RG, 0x800AFE9C);
ALIAS(g_FieldControl, RJ, 0x800B2176); ALIAS(D_800B21CE, RJ, 0x800B21CE); ALIAS(D_800B2340, RJ, 0x800B2340);
ALIAS(D_800B2342, RJ, 0x800B2342); ALIAS(D_800B2344, RJ, 0x800B2344); ALIAS(D_800B2354, RJ, 0x800B2354);
ALIAS(D_800B2360, RJ, 0x800B2360);
ALIAS(D_800C2694, RK, 0x800C2694); ALIAS(g_FieldTextBoxes, RK, 0x800C2698);
uintptr_t g_FieldActors, g_FieldScriptVMCurActor, g_FieldScriptVMCurScriptData;

const Region regions[] = {
    { RA_BASE, RA_SIZE, RA }, { RB_BASE, RB_SIZE, RB }, { RD_BASE, RD_SIZE, RD }, { RE_BASE, RE_SIZE, RE },
    { RF_BASE, RF_SIZE, RF }, { RG_BASE, RG_SIZE, RG }, { RI_BASE, RI_SIZE, RI }, { RJ_BASE, RJ_SIZE, RJ },
    { RK_BASE, RK_SIZE, RK },
};
const unsigned NREG = sizeof(regions) / sizeof(regions[0]);
const PtrGlobal ptrglobals[] = {
    { 0x800AFB10u, &g_FieldActors }, { 0x800B0078u, &g_FieldScriptVMCurActor },
    { 0x800ADC00u, &g_FieldScriptVMCurScriptData },
};
const unsigned NPTR = sizeof(ptrglobals) / sizeof(ptrglobals[0]);

/* func_8007B1C4 has an s16 port prototype: take the halfwords explicitly. */
long func_8007B1C4(long a, long b, long c, long d, long e, long f) {
    uint64_t in[6] = { (uint64_t)(int64_t)(int16_t)a, (uint64_t)(int64_t)(int16_t)b, (uint64_t)c,
                       (uint64_t)d, (uint64_t)e, (uint64_t)f };
    return spy_core(S_7B1C4, in);
}
SPY(func_80080968, S_80968)
SPY(func_80079288, S_79288)
SPY(func_80081F5C, S_81F5C)
SPY(func_8009E574, S_9E574)
SPY(FieldScriptVMGetVariableValue, S_GETVAR)
/* XENO_TEST_INPUT tooling hook (inactive here; retail has no such call). */
void PcPort_TestInputInject(uint16_t* held) { (void)held; fail("test-input inject called with XENO_TEST_INPUT unset"); }

/* func_8007B1C4(x, z, i, out s16[4], state s32[4]): halfwords 4 + 8 */
const Target targets[] = {
    { 0x8007B1C4, S_7B1C4, 3, 1, 0x18, 0, 0x84000u, 0 },
    { 0x80080968, S_80968, 1, 1, 0, 0 },
    { 0x80079288, S_79288, 0, 0, 0, 0 },
    { 0x80081F5C, S_81F5C, 1, 1, 0, 0 },
    { 0x8009E574, S_9E574, 2, 0, 0, 0 },
    { 0x800A3018, S_GETVAR, 1, 1, 0, 0 },
};
const unsigned NTARGETS = sizeof(targets) / sizeof(targets[0]);

static int32_t feed_map(uint32_t id, int32_t raw) {
    if (id == S_81F5C) return ((uint32_t)raw & 1u) ? 0 : raw;
    return raw;
}

/* ---- func_8009E810 port-only stores ------------------------------------ */
static int chk810;
static uint32_t c810_arg, c810_fa, c810_spr;
static uint8_t c810_save[4][4];
static void native_pre(void) {
    chk810 = strcmp(cur_name, "func_8009E810") == 0;
    if (!chk810) return;
    int32_t idx; memcpy(&idx, RF + (0x800AFD1C - RF_BASE), 4);
    c810_fa = FACTORS + (uint32_t)idx * 0x5Cu;
    memcpy(&c810_spr, (uint8_t*)(uintptr_t)(c810_fa + 4), 4);
    memcpy(c810_save[0], (uint8_t*)(uintptr_t)(c810_fa + 0x24), 4);
    memcpy(c810_save[1], (uint8_t*)(uintptr_t)(c810_fa + 0x44), 4);
    if (c810_spr) {
        memcpy(c810_save[2], (uint8_t*)(uintptr_t)(c810_spr + 0x84), 2);
        memcpy(c810_save[3], (uint8_t*)(uintptr_t)(c810_spr + 0x04), 4);
    }
}
static void native_post(void) {
    if (!chk810) return;
    int32_t s = (int16_t)c810_arg, v;
    memcpy(&v, (uint8_t*)(uintptr_t)(c810_fa + 0x24), 4); require(v == s, "9E810 port store FA+0x24");
    memcpy(&v, (uint8_t*)(uintptr_t)(c810_fa + 0x44), 4); require(v == s, "9E810 port store FA+0x44");
    memcpy((uint8_t*)(uintptr_t)(c810_fa + 0x24), c810_save[0], 4);
    memcpy((uint8_t*)(uintptr_t)(c810_fa + 0x44), c810_save[1], 4);
    if (c810_spr) {
        uint16_t h; memcpy(&h, (uint8_t*)(uintptr_t)(c810_spr + 0x84), 2);
        require(h == (uint16_t)c810_arg, "9E810 port store sprite+0x84");
        uint32_t w; memcpy(&w, (uint8_t*)(uintptr_t)(c810_spr + 0x04), 4);
        require(w == c810_arg << 16, "9E810 port store sprite+0x04");
        memcpy((uint8_t*)(uintptr_t)(c810_spr + 0x84), c810_save[2], 2);
        memcpy((uint8_t*)(uintptr_t)(c810_spr + 0x04), c810_save[3], 4);
    }
}

/* ---- native entry wrappers --------------------------------------------- */
extern void real_func_8009E574(int32_t, int32_t);
extern void func_8009E810(int16_t);
extern void func_8009F5F4(void);
extern void func_8009FA54(int32_t);
static uint32_t n_E574(Args a) { real_func_8009E574((int32_t)a.a0, (int32_t)a.a1); return 0; }
static uint32_t n_E810(Args a) { func_8009E810((int16_t)a.a0); return 0; }
static uint32_t n_F5F4(Args a) { (void)a; func_8009F5F4(); return 0; }
static uint32_t n_FA54(Args a) { func_8009FA54((int32_t)a.a0); return 0; }

static void setup_ptrs(void) {
    w32(0x800AFB10u, FACTORS); w32(0x800B0078u, CURACT); w32(0x800ADC00u, SCRIPT);
    for (unsigned k = 0; k < NACT; k++) w32(FACTORS + k * 0x5C + 0x04, SPR + k * 0x200);
    w32(0x800AFD1C, rnd_range(NACT));
}

static void g_E574(Args* a) {
    setup_ptrs();
    int16_t n = (int16_t)small(2, 5);
    w16(0x800AFB54, (uint16_t)n);
    w16(CURACT + 0x10, (uint16_t)rnd_range((uint32_t)(n - 1)));
    a->a0 = (uint32_t)(int32_t)(int16_t)rnd();
    a->a1 = (uint32_t)(int32_t)(int16_t)rnd();
    if (rnd() % 4 == 0) { a->a0 = (uint32_t)small(-0x800, 0x800); a->a1 = (uint32_t)small(-0x800, 0x800); }
}
static void g_E810(Args* a) {
    setup_ptrs();
    a->a0 = (rnd() & 1) ? rnd() : (uint32_t)(int32_t)(int16_t)rnd();
    c810_arg = a->a0;
    if (rnd() % 6 == 0) { uint32_t fa = FACTORS + r32(0x800AFD1C) * 0x5C; w32(fa + 4, 0); }
}
static void g_F5F4(Args* a) {
    (void)a;
    unsetenv("XENO_TEST_INPUT");
    setup_ptrs();
    w16(CURACT + 0xCC, (uint16_t)rnd());
    uint32_t f0 = r32(CURACT);
    if (rnd() % 4) f0 |= 0x4000; else f0 &= ~0x4000u;
    if (rnd() & 1) f0 &= ~0x1800u;
    w32(CURACT, f0);
    for (unsigned i = 0; i < 4; i++) w16(0x800C2698 + 0x37C + i * 0x498, (rnd() % 6) ? (uint16_t)(rnd() | 1) : 0);
    if (rnd() % 4) w16(0x800B2176, 0);
    if (rnd() & 1) w32(CURACT + 0x14, r32(CURACT + 0x14) & ~0x400000u);
    if (rnd() & 1) {   /* prevPosition == position (stuck frame); occasionally only partly */
        for (unsigned o = 0; o < 3; o++) memcpy(gp(CURACT + 0x68 + 2 * o), gp(CURACT + 0x22 + 4 * o), 2);
        if (rnd() % 4 == 0) w16(CURACT + 0x68 + 2 * rnd_range(3), (uint16_t)rnd());
    }
    if (rnd() & 1) w16(0x800ADB02, (uint16_t)small(0x1E, 0x22));
    if (rnd() % 3) w32(0x800ADB64, 0xFF);
    if (rnd() & 1) w16(0x800B2344, 0);
    if (rnd() & 1) w16(0x800B2342, (rnd() & 1) ? 0 : (uint16_t)small(1, 3));
    if (rnd() & 1) w8(0x800B2354, 0);
    if (rnd() & 1) w8(0x800B21CE, 0);
    if (rnd() % 3 == 0) w16(0x800AFE9C, (uint16_t)(rnd() & 0x0FFF));
}
static void g_FA54(Args* a) {
    setup_ptrs();
    if (rnd() % 6) w8(SCRIPT, 0xFF);
    a->a0 = rnd_range(0x800);
    uint32_t e = SCRIPT + a->a0 * 7;
    if (rnd() % 3 == 0) w8(e + 6, 0xFF);
    if (rnd() % 3 == 0) w8(e + 7, 0xFF);
}

Fn fns[] = {
    { "func_8009E574", 0x8009E574, 167, n_E574, g_E574, 2500, 0 },
    { "func_8009E810", 0x8009E810, 11, n_E810, g_E810, 1000, 0 },
    { "func_8009F5F4", 0x8009F5F4, 259, n_F5F4, g_F5F4, 6000, 0 },
    { "func_8009FA54", 0x8009FA54, 81, n_FA54, g_FA54, 2000, 0 },
};
const unsigned NFNS = sizeof(fns) / sizeof(fns[0]);
