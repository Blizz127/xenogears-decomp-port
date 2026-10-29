/* Retail differential, src/field/main/misc.c hand-written arms (batch misca):
 * func_800871B0 8008800C 8008A2E8 8008A7DC 8008AACC 8008F7B8.
 * The production TU is linked unmodified; see lib/field_retail_core.h.
 *
 * Extra observations (not expressible through the shared Target table) go
 * through an "xlog": FEED_MAP runs on both sides for ret=1 targets; on the
 * retail side it reads guest state via active_cpu, on the native side the
 * custom spy stashes the host pointer / 7th argument first.  The two logs are
 * compared in NATIVE_POST.  Used for: SetTransMatrix mtx.t[0..2],
 * RotTransSV input vector vx/vy/vz, FieldLoadTIMWithClut 7th argument. */
#define TESTNAME "FIELD F2 MISCA"
#include <stdint.h>
#include "battle_mips_adapter.h"
/* Copy loops etc. need more than the core's 20000-instruction budget. */
#define PcPortMipsRun(c, e, h, n) PcPortMipsRun(c, e, h, 400000)

enum { S_GETARG = 1, S_HEAPALLOC, S_HEAPFREE, S_STOREIMG, S_26F44, S_ARG1, S_ARG2, S_ARG3, S_ARG4, S_ARG5,
       S_1E72CC, S_SETROT, S_SETTRANS, S_ROTTRANS, S_GETIARG, S_WRITEU16, S_A558, S_SETIDX, S_DECODE,
       S_READF, S_CDSYNC, S_TIM, S_A915C, S_GEAR, S_WDSLOAD, S_WDSFREE, S_3BDFC, S_85EEC, S_1B66C, S_85B20 };

static int32_t feed_map(uint32_t id, int32_t raw);
static void xlog_pre(void);
static void xlog_post(void);
#define FEED_MAP(id, raw) feed_map(id, raw)
#define NATIVE_PRE() xlog_pre()
#define NATIVE_POST() xlog_post()
#include "field_retail_core.h"

#define SCRIPT 0x80120000u
#define CURACT 0x80130000u

/* globals regions (retail symbol names alias into them; every PC-separate
 * symbol keeps its own name -- D_80062524 lives at a fake address). */
#define RA_BASE 0x8004F300u
#define RA_SIZE 0x80u
#define RB_BASE 0x800595A0u
#define RB_SIZE 0x10u
#define RC_BASE 0x80062510u
#define RC_SIZE 0x20u
#define RD_BASE 0x80062600u /* fake home of the PC-only host shadow D_80062524 */
#define RD_SIZE 0x10u
#define RE_BASE 0x8006FAB0u
#define RE_SIZE 0x20u
#define RF_BASE 0x800ADB10u
#define RF_SIZE 0x100u
#define RG_BASE 0x800AF870u
#define RG_SIZE 0x10u
#define RH_BASE 0x800AFC50u
#define RH_SIZE 0x30u
#define RI_BASE 0x800AFD00u
#define RI_SIZE 0x20u
#define RJ_BASE 0x800B0070u
#define RJ_SIZE 0x60u
#define RK_BASE 0x800B1F70u
#define RK_SIZE 0x10u
#define RL_BASE 0x800C3A40u
#define RL_SIZE 0x10u
REGION(RA); REGION(RB); REGION(RC); REGION(RD); REGION(RE); REGION(RF); REGION(RG); REGION(RH);
REGION(RI); REGION(RJ); REGION(RK); REGION(RL);
ALIAS(g_GameSceneMapNum, RA, 0x8004F34C); ALIAS(D_8004F370, RA, 0x8004F370); ALIAS(D_8004F324, RA, 0x8004F324);
ALIAS(D_8004F308, RA, 0x8004F308); ALIAS(D_8004F354, RA, 0x8004F354);
ALIAS(D_80062518, RC, 0x80062518);
ALIAS(g_GamePartyMemberSkins, RE, 0x8006FABC);
ALIAS(D_800ADB1C, RF, 0x800ADB1C); ALIAS(D_800ADBB4, RF, 0x800ADBB4); ALIAS(D_800ADBC4, RF, 0x800ADBC4);
ALIAS(D_800ADBC8, RF, 0x800ADBC8); ALIAS(D_800ADBCC, RF, 0x800ADBCC); ALIAS(D_800ADBDC, RF, 0x800ADBDC);
ALIAS(D_800AF87C, RG, 0x800AF87C);
ALIAS(D_800AFC58, RH, 0x800AFC58); ALIAS(D_800AFC5A, RH, 0x800AFC5A); ALIAS(D_800AFC5C, RH, 0x800AFC5C);
ALIAS(D_800AFC5E, RH, 0x800AFC5E); ALIAS(g_FieldScriptMaxInstructionCount, RH, 0x800AFC7C);
ALIAS(D_800AFD0C, RI, 0x800AFD0C); ALIAS(D_800AFD18, RI, 0x800AFD18);
ALIAS(D_800B00C0, RJ, 0x800B00C0);
ALIAS(D_800B1F74, RK, 0x800B1F74);
ALIAS(D_800C3A48, RL, 0x800C3A48);
uintptr_t g_FieldScriptVMCurActor, g_FieldScriptVMCurScriptData, D_800ADBC0, D_800AFD08, D_800595AC, D_80062524;

const Region regions[] = {
    { RA_BASE, RA_SIZE, RA }, { RB_BASE, RB_SIZE, RB }, { RC_BASE, RC_SIZE, RC }, { RD_BASE, RD_SIZE, RD },
    { RE_BASE, RE_SIZE, RE }, { RF_BASE, RF_SIZE, RF }, { RG_BASE, RG_SIZE, RG }, { RH_BASE, RH_SIZE, RH },
    { RI_BASE, RI_SIZE, RI }, { RJ_BASE, RJ_SIZE, RJ }, { RK_BASE, RK_SIZE, RK }, { RL_BASE, RL_SIZE, RL },
};
const unsigned NREG = sizeof(regions) / sizeof(regions[0]);
const PtrGlobal ptrglobals[] = {
    { 0x800B0078u, &g_FieldScriptVMCurActor }, { 0x800ADC00u, &g_FieldScriptVMCurScriptData },
    { 0x800ADBC0u, &D_800ADBC0 }, { 0x800AFD08u, &D_800AFD08 }, { 0x800595ACu, &D_800595AC },
    { 0x80062600u, &D_80062524 },
};
const unsigned NPTR = sizeof(ptrglobals) / sizeof(ptrglobals[0]);

/* host region pointer -> guest address */
static uint32_t h2g(uintptr_t p) {
    for (unsigned r = 0; r < NREG; r++)
        if (p >= (uintptr_t)regions[r].nat && p < (uintptr_t)regions[r].nat + regions[r].size)
            return regions[r].base + (uint32_t)(p - (uintptr_t)regions[r].nat);
    return (uint32_t)p;
}

/* ---- extra observation log ------------------------------------------------ */
static uint32_t xlog[2][64];
static unsigned xn[2];
static const uint8_t* stash_p;
static uint32_t stash_arg7;
static void xrec(uint32_t v) {
    int side = active_cpu ? 0 : 1;
    require(xn[side] < 64, "xlog size");
    xlog[side][xn[side]++] = v;
}
static uint32_t xr16(uint32_t gaddr, const uint8_t* host, unsigned off) {
    uint16_t v;
    if (active_cpu) return r32(gaddr + off) & 0xFFFFu;
    memcpy(&v, host + off, 2);
    return v;
}
static uint32_t xr32(uint32_t gaddr, const uint8_t* host, unsigned off) {
    uint32_t v;
    if (active_cpu) return r32(gaddr + off);
    memcpy(&v, host + off, 4);
    return v;
}
static int32_t feed_map(uint32_t id, int32_t raw) {
    uint32_t g = active_cpu ? active_cpu->gpr[4] : 0;
    if (id == S_SETTRANS) { xrec(xr32(g, stash_p, 0x14)); xrec(xr32(g, stash_p, 0x18)); xrec(xr32(g, stash_p, 0x1C)); }
    if (id == S_ROTTRANS) { xrec(xr16(g, stash_p, 0)); xrec(xr16(g, stash_p, 2)); xrec(xr16(g, stash_p, 4)); }
    if (id == S_TIM)
        xrec((uint32_t)(int32_t)(int16_t)(active_cpu ? r32(active_cpu->gpr[29] + 0x18) : stash_arg7));
    return raw;
}
static void xlog_pre(void) { xn[1] = 0; }
static void xlog_post(void) {
    require(xn[0] == xn[1], "xlog count");
    require(memcmp(xlog[0], xlog[1], 4 * xn[0]) == 0, "xlog values (extra observations)");
}
static void xlog_reset(void) { xn[0] = xn[1] = 0; }

SPY(FieldScriptVMGetArgument, S_GETARG)
SPY(HeapAlloc, S_HEAPALLOC)
SPY(HeapFree, S_HEAPFREE)
SPY(func_80026F44, S_26F44)
SPY(FieldScriptArgument1, S_ARG1)
SPY(FieldScriptArgument2, S_ARG2)
SPY(FieldScriptArgument3, S_ARG3)
SPY(FieldScriptArgument4, S_ARG4)
SPY(FieldScriptArgument5, S_ARG5)
SPY(func_801E72CC, S_1E72CC)
SPY(SetRotMatrix, S_SETROT)
SPY(FieldScriptVMGetInstructionArgument, S_GETIARG)
SPY(FieldScriptMemoryWriteU16, S_WRITEU16)
SPY(func_8008A558, S_A558)
SPY(ArchiveSetIndex, S_SETIDX)
SPY(ArchiveDecodeAlignedSize, S_DECODE)
SPY(ArchiveReadFileToBuffer, S_READF)
SPY(ArchiveCdDataSync, S_CDSYNC)
SPY(func_800A915C, S_A915C)
SPY(GameCharacterGetGearID, S_GEAR)
SPY(SoundLoadWdsFile, S_WDSLOAD)
SPY(SoundFreeWdsEntry, S_WDSFREE)
SPY(func_8003BDFC, S_3BDFC)
SPY(func_80085EEC, S_85EEC)
SPY(func_8001B66C, S_1B66C)
SPY(func_80085B20, S_85B20)
long StoreImage(long a, long b, long c, long d, long e, long f) {
    uint64_t in[6] = { (uint64_t)a, (uint64_t)b, (uint64_t)c, (uint64_t)d, (uint64_t)e, (uint64_t)f };
    long r = spy_core(S_STOREIMG, in);
    trace.ev[trace.n - 1].a[0] = h2g((uintptr_t)a);
    return r;
}
long SetTransMatrix(long a, long b, long c, long d, long e, long f) {
    uint64_t in[6] = { (uint64_t)a, (uint64_t)b, (uint64_t)c, (uint64_t)d, (uint64_t)e, (uint64_t)f };
    stash_p = (const uint8_t*)(uintptr_t)a;
    return spy_core(S_SETTRANS, in);
}
long RotTransSV(long a, long b, long c, long d, long e, long f) {
    uint64_t in[6] = { (uint64_t)a, (uint64_t)b, (uint64_t)c, (uint64_t)d, (uint64_t)e, (uint64_t)f };
    stash_p = (const uint8_t*)(uintptr_t)a;
    return spy_core(S_ROTTRANS, in);
}
/* real prototype (misc3.c): (u_long*, short x6); compare the shorts as shorts */
long FieldLoadTIMWithClut(long a, long b, long c, long d, long e, long f, long g) {
    uint64_t in[6] = { (uint64_t)a, (uint64_t)(int64_t)(int16_t)b, (uint64_t)(int64_t)(int16_t)c,
                       (uint64_t)(int64_t)(int16_t)d, (uint64_t)(int64_t)(int16_t)e, (uint64_t)(int64_t)(int16_t)f };
    stash_arg7 = (uint32_t)g;
    return spy_core(S_TIM, in);
}

const Target targets[] = {
    { 0x800ACDEC, S_GETARG, 1, 1, 0, 0 }, { 0x80031BDC, S_HEAPALLOC, 2, 1, 0, 0 }, { 0x800320E8, S_HEAPFREE, 1, 0, 0, 0 },
    { 0x800448F8, S_STOREIMG, 2, 0, 0, 0, 0, 0x1 }, { 0x80026F44, S_26F44, 4, 0, 0, 0 },
    { 0x8009CF78, S_ARG1, 2, 1, 0, 0 }, { 0x8009CFBC, S_ARG2, 2, 1, 0, 0 }, { 0x8009D000, S_ARG3, 2, 1, 0, 0 },
    { 0x8009D044, S_ARG4, 2, 1, 0, 0 }, { 0x8009D088, S_ARG5, 2, 1, 0, 0 },
    { 0x801E72CC, S_1E72CC, 4, 0, 0x3, 0, 0xA }, { 0x80049EFC, S_SETROT, 1, 0, 0, 0, 0, 0x1 },
    { 0x80049F8C, S_SETTRANS, 1, 1, 0, 0, 0, 0x1 }, { 0x8004A54C, S_ROTTRANS, 3, 1, 0x7, 0, 0x230 },
    { 0x800ACDB8, S_GETIARG, 1, 1, 0, 0 }, { 0x800A3074, S_WRITEU16, 2, 0, 0, 0 },
    { 0x8008A558, S_A558, 0, 1, 0, 0 }, { 0x80028470, S_SETIDX, 2, 0, 0, 0 }, { 0x800288EC, S_DECODE, 1, 1, 0, 0 },
    { 0x800295D8, S_READF, 4, 0, 0, 0 }, { 0x80028A60, S_CDSYNC, 1, 0, 0, 0 }, { 0x80070340, S_TIM, 6, 1, 0, 0 },
    { 0x800A915C, S_A915C, 0, 0, 0, 0 }, { 0x8001ACF0, S_GEAR, 1, 1, 0, 0 }, { 0x80037FD8, S_WDSLOAD, 2, 1, 0, 0 },
    { 0x80038310, S_WDSFREE, 1, 0, 0, 0 }, { 0x8003BDFC, S_3BDFC, 1, 0, 0, 0 }, { 0x80085EEC, S_85EEC, 0, 0, 0, 0 },
    { 0x8001B66C, S_1B66C, 0, 0, 0, 0 }, { 0x80085B20, S_85B20, 1, 0, 0, 0 },
};
const unsigned NTARGETS = sizeof(targets) / sizeof(targets[0]);

/* ---- native entry wrappers --------------------------------------------- */
extern void func_800871B0(void);
extern void func_8008800C(void);
extern void func_8008A2E8(void);
extern void func_8008A7DC(int32_t, int32_t);
extern void func_8008AACC(void);
extern void func_8008F7B8(void);
static uint32_t n_71B0(Args a) { (void)a; func_800871B0(); return 0; }
static uint32_t n_800C(Args a) { (void)a; func_8008800C(); return 0; }
static uint32_t n_A2E8(Args a) { (void)a; func_8008A2E8(); return 0; }
static uint32_t n_A7DC(Args a) { func_8008A7DC((int32_t)a.a0, (int32_t)a.a1); return 0; }
static uint32_t n_AACC(Args a) { (void)a; func_8008AACC(); return 0; }
static uint32_t n_F7B8(Args a) { (void)a; func_8008F7B8(); return 0; }

static uint16_t ipv;
static void setup(void) {
    xlog_reset();
    w32(0x800B0078u, CURACT); w32(0x800ADC00u, SCRIPT);
    ipv = (uint16_t)rnd_range(0x7000);
    w16(CURACT + 0xCC, ipv);
}
static void sbyte(unsigned off, uint8_t v) { w8(SCRIPT + ipv + off, v); }

static void g_71B0(Args* a) {
    (void)a; setup();
    sbyte(1, (uint8_t)((rnd() % 6 == 0) ? rnd() : rnd_range(5)));
}
static void g_800C(Args* a) {
    (void)a; setup();
}
static void g_A2E8(Args* a) {
    (void)a; setup();
    sbyte(1, (uint8_t)((rnd() % 5 == 0) ? rnd() : rnd_range(3)));
    feed[0] = (rnd() % 4 == 0) ? -1 : rnd_val();
    /* sub-op 1: x/y/clutY are short parameters of FieldLoadTIMWithClut;
     * keep the 32-bit script values in s16 range (retail callee truncates). */
    feed[1] = (rnd() % 3 == 0) ? 0xFF : small(-0x8000 - 0xE8, 0x7FFF - 0xE8);
    feed[2] = (rnd() & 1) ? small(0x2B0, 0x2D0) : small(-0x8000, 0x7FFF);
    feed[3] = (rnd() & 1) ? small(0xF0, 0x110) : small(-0x8000, 0x7FFF);
}
static void g_A7DC(Args* a) {
    setup();
    a->a0 = (rnd() & 1) ? (uint32_t)small(0, 20) : (uint32_t)rnd_val();
    a->a1 = rnd_range(3);
    if (rnd() & 1) w32(0x800ADB1C, 0);
    w32(0x8004F34C, (rnd() & 1) ? (rnd() & ~0xC000u) : rnd());
    feed[1] = feed[2] = (rnd() % 3 == 0) ? 0xFF : rnd_val();
}
static void g_AACC(Args* a) {
    (void)a; setup();
    sbyte(1, (uint8_t)((rnd() % 3 == 0) ? rnd() : 1));
    feed[0] = (rnd() % 5 == 0) ? rnd_val() : 0;
    /* load path: slot 3 is the documented PC divergence (D_800595AC <- host
     * shadow D_80062524), so loads cover slots 0..2 only. */
    w32(0x800AFD18, rnd_range(3));
    feed[1] = (int32_t)rnd_range(4);
    feed[3] = (rnd() & 1) ? (int32_t)(rnd() & 0xFF) : rnd_val();
    if (rnd() & 1) w32(0x8004F370, 1);
}
static void g_F7B8(Args* a) {
    (void)a; setup();
    if (rnd() & 1) w32(0x800ADB1C, 0);
    if (rnd() & 1) feed[0] = (int32_t)r32(0x8004F324);
    feed[1] = (rnd() % 4 == 0) ? rnd_val() : 0;
    if (rnd() % 3) w32(0x800ADBDC, 1 + rnd_range(5));
    if (rnd() & 1) w32(0x8004F354, (rnd() & 1) ? 1 : rnd());
    if (rnd() & 1) w32(0x8004F308, (rnd() & 1) ? 0xFFFFFFFFu : rnd());
}

Fn fns[] = {
    { "func_800871B0", 0x800871B0, 133, n_71B0, g_71B0, 1500, 0 },
    { "func_8008800C", 0x8008800C, 99, n_800C, g_800C, 1500, 0 },
    { "func_8008A2E8", 0x8008A2E8, 126, n_A2E8, g_A2E8, 2000, 0 },
    { "func_8008A7DC", 0x8008A7DC, 88, n_A7DC, g_A7DC, 1500, 0 },
    { "func_8008AACC", 0x8008AACC, 135, n_AACC, g_AACC, 2000, 0 },
    { "func_8008F7B8", 0x8008F7B8, 85, n_F7B8, g_F7B8, 2000, 0 },
};
const unsigned NFNS = sizeof(fns) / sizeof(fns[0]);
