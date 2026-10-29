/* Retail differential, src/field/main/misc6.c port arms (batch c):
 * func_800A0EE8 800A0FD8 800A14F0 800A1624, and func_8009E574 with FULL-WORD
 * x/z (the halfword-range run is in field_f2_misc6_retail_test.c).
 * The production TU is linked unmodified; see lib/field_retail_core.h.
 * Stack arguments beyond the core's six (func_80076AC0 #7, func_801E742C
 * #7..#9) are captured on both sides and compared in NATIVE_POST; host
 * pointers into the global regions are translated back to guest addresses.
 *
 * DIVERGENCE NOT COMPARED HERE (reported): func_8009E574 still calls
 * func_8007B1C4 through its s16 port prototype, so that callee sees only the
 * low halfwords of x/z where retail passes the full words.  In this full-word
 * variant the func_8007B1C4 events therefore record no arguments (nargs 0;
 * the out-pointer feed is still applied); the halfword-range variant in
 * batch a compares them fully.  Every store of func_8009E574 is compared.
 * Coverage limits: D_801E8670 entries are never 0 (the port keeps a host
 * null guard in func_800A0FD8 sub-ops 0/2 where retail would store through
 * address 0); skin indices are kept small (retail reads g_FieldSpriteData +
 * 4*skin + 4). */
#define TESTNAME "FIELD F2 MISC6C"
enum { S_E8030 = 1, S_8A558, S_SETIDX, S_GETARG, S_DECODE, S_HEAPALLOC, S_29AFC, S_CDSYNC, S_E742C,
       S_HEAPFREE, S_76AC0, S_A0C94, S_7B1C4, S_80968 };
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
#define OBJ 0x80140000u
#define OBJ2 0x80148000u
#define SKIN 0x80150000u
#define NACT 16u

#define RA_BASE 0x800ADB00u
#define RA_SIZE 0x110u
#define RE_BASE 0x800AFB10u
#define RE_SIZE 0x50u
#define RF_BASE 0x800AFD10u
#define RF_SIZE 0x20u
#define RI_BASE 0x800B0070u
#define RI_SIZE 0x60u
#define RJ_BASE 0x800B2170u
#define RJ_SIZE 0x240u
#define RM_BASE 0x8005A390u
#define RM_SIZE 0xF0u
#define RP_BASE 0x801E8660u
#define RP_SIZE 0x30u
REGION(RA); REGION(RE); REGION(RF); REGION(RI); REGION(RJ); REGION(RM); REGION(RP);
ALIAS(D_800ADB2C, RA, 0x800ADB2C);
ALIAS(D_800AFB54, RE, 0x800AFB54);
ALIAS(D_800AFD1C, RF, 0x800AFD1C);
ALIAS(D_800B00C0, RI, 0x800B00C0);
ALIAS(D_800B21DC, RJ, 0x800B21DC); ALIAS(D_800B21EC, RJ, 0x800B21EC); ALIAS(D_800B220C, RJ, 0x800B220C);
ALIAS(D_800B225F, RJ, 0x800B225F); ALIAS(D_800B2264, RJ, 0x800B2264); ALIAS(D_800B2394, RJ, 0x800B2394);
ALIAS(D_801E8670, RP, 0x801E8670);
uintptr_t g_FieldActors, g_FieldScriptVMCurActor, g_FieldScriptVMCurScriptData, g_FieldSpriteData;
uintptr_t D_8005A420[8], D_8005A450[8];

const Region regions[] = {
    { RA_BASE, RA_SIZE, RA }, { RE_BASE, RE_SIZE, RE }, { RF_BASE, RF_SIZE, RF }, { RI_BASE, RI_SIZE, RI },
    { RJ_BASE, RJ_SIZE, RJ }, { RM_BASE, RM_SIZE, RM }, { RP_BASE, RP_SIZE, RP },
};
const unsigned NREG = sizeof(regions) / sizeof(regions[0]);
#define PG8(base, arr) { base + 0, &arr[0] }, { base + 4, &arr[1] }, { base + 8, &arr[2] }, { base + 12, &arr[3] }, \
    { base + 16, &arr[4] }, { base + 20, &arr[5] }, { base + 24, &arr[6] }, { base + 28, &arr[7] }
const PtrGlobal ptrglobals[] = {
    { 0x800AFB10u, &g_FieldActors }, { 0x800B0078u, &g_FieldScriptVMCurActor },
    { 0x800ADC00u, &g_FieldScriptVMCurScriptData }, { 0x800AFB1Cu, &g_FieldSpriteData },
    PG8(0x8005A420u, D_8005A420), PG8(0x8005A450u, D_8005A450),
};
const unsigned NPTR = sizeof(ptrglobals) / sizeof(ptrglobals[0]);

/* host region pointer -> guest address (identity window passes through) */
static uint32_t h2g(long p) {
    for (unsigned r = 0; r < NREG; r++)
        if ((uintptr_t)p >= (uintptr_t)regions[r].nat && (uintptr_t)p < (uintptr_t)regions[r].nat + regions[r].size)
            return regions[r].base + (uint32_t)((uintptr_t)p - (uintptr_t)regions[r].nat);
    return (uint32_t)p;
}

/* extra (stack) arguments beyond the core's six, per call, both sides */
static uint32_t xr[3 * MAXEV], xn[3 * MAXEV];
static unsigned xr_n, xn_n;
static void xnat(uint32_t v) { require(xn_n < 3 * MAXEV, "extra count"); xn[xn_n++] = v; }

SPY(func_801E8030, S_E8030)
SPY(func_8008A558, S_8A558)
SPY(ArchiveSetIndex, S_SETIDX)
SPY(FieldScriptVMGetArgument, S_GETARG)
SPY(ArchiveDecodeAlignedSize, S_DECODE)
SPY(HeapAlloc, S_HEAPALLOC)
SPY(ArchiveCdDataSync, S_CDSYNC)
SPY(HeapFree, S_HEAPFREE)
SPY(func_800A0C94, S_A0C94)
SPY(func_8007B1C4, S_7B1C4)
SPY(func_80080968, S_80968)
long func_80029AFC(long a, long b, long c, long d, long e, long f) {
    uint64_t in[6] = { h2g(a), (uint64_t)b, (uint64_t)c, (uint64_t)d, (uint64_t)e, (uint64_t)f };
    return spy_core(S_29AFC, in);
}
long func_80076AC0(long a, long b, long c, long d, long e, long f, long g) {
    xnat((uint32_t)g);
    uint64_t in[6] = { (uint64_t)a, (uint64_t)b, (uint64_t)c, (uint64_t)d, (uint64_t)e, (uint64_t)f };
    return spy_core(S_76AC0, in);
}
long func_801E742C(long a, long b, long c, long d, long e, long f, long g, long h, long i) {
    xnat((uint32_t)g); xnat((uint32_t)h); xnat(h2g(i));
    uint64_t in[6] = { (uint64_t)a, (uint64_t)b, (uint64_t)c, (uint64_t)d, (uint64_t)e, (uint64_t)f };
    return spy_core(S_E742C, in);
}

const Target targets[] = {
    { 0x801E8030, S_E8030, 1, 0, 0, 0 },
    { 0x8008A558, S_8A558, 0, 1, 0, 0 },
    { 0x80028470, S_SETIDX, 2, 1, 0, 0 },
    { 0x800ACDEC, S_GETARG, 1, 1, 0, 0 },
    { 0x800288EC, S_DECODE, 1, 1, 0, 0 },
    { 0x80031BDC, S_HEAPALLOC, 2, 1, 0, 0 },
    { 0x80029AFC, S_29AFC, 3, 1, 0, 0 },
    { 0x80028A60, S_CDSYNC, 1, 1, 0, 0 },
    { 0x801E742C, S_E742C, 6, 1, 0, 0 },
    { 0x800320E8, S_HEAPFREE, 1, 0, 0, 0 },
    { 0x80076AC0, S_76AC0, 6, 1, 0, 0 },
    { 0x800A0C94, S_A0C94, 0, 0, 0, 0 },
    { 0x8007B1C4, S_7B1C4, 0, 1, 0x18, 0, 0x84000u, 0 },
    { 0x80080968, S_80968, 1, 1, 0, 0 },
};
const unsigned NTARGETS = sizeof(targets) / sizeof(targets[0]);

static int32_t feed_map(uint32_t id, int32_t raw) {
    if (id == S_8A558) return ((uint32_t)raw % 4u) ? 0 : raw;
    if (id == S_CDSYNC) return ((uint32_t)raw & 1u) ? 0 : raw;
    if (id == S_76AC0 || id == S_E742C) {   /* void callees: v0 is dead after the call */
        if (active_cpu) {
            uint32_t sp = active_cpu->gpr[29];
            unsigned k = id == S_76AC0 ? 1 : 3;
            for (unsigned j = 0; j < k; j++) { require(xr_n < 3 * MAXEV, "extra count"); xr[xr_n++] = r32(sp + 24 + 4 * j); }
        }
        return 0;
    }
    return raw;
}
static void native_pre(void) { xn_n = 0; }
static void native_post(void) {
    require(xn_n == xr_n, "stack-argument count");
    require(memcmp(xn, xr, 4 * xn_n) == 0, "stack arguments 7..9");
}

extern void func_800A0EE8(void);
extern void func_800A0FD8(void);
extern void func_800A14F0(void);
extern void func_800A1624(void);
extern void func_8009E574(int32_t, int32_t);
static uint32_t n_0EE8(Args a) { (void)a; func_800A0EE8(); return 0; }
static uint32_t n_0FD8(Args a) { (void)a; func_800A0FD8(); return 0; }
static uint32_t n_14F0(Args a) { (void)a; func_800A14F0(); return 0; }
static uint32_t n_1624(Args a) { (void)a; func_800A1624(); return 0; }
static uint32_t n_E574(Args a) { func_8009E574((int32_t)a.a0, (int32_t)a.a1); return 0; }

static void setup_ptrs(void) {
    xr_n = 0;
    w32(0x800AFB10u, FACTORS); w32(0x800B0078u, CURACT); w32(0x800ADC00u, SCRIPT); w32(0x800AFB1Cu, SKIN);
    for (unsigned k = 0; k < NACT; k++) w32(FACTORS + k * 0x5C + 0x04, SPR + k * 0x200);
    w32(0x800AFD1C, rnd_range(NACT));
    for (unsigned k = 0; k < 8; k++) {
        w32(0x801E8670 + 4 * k, OBJ + k * 0x100);
        w32(OBJ + k * 0x100 + 4, OBJ2 + k * 0x100);
    }
}
static void set_subop(unsigned pick) {
    uint16_t ip = (uint16_t)rnd_range(0x8000);
    w16(CURACT + 0xCC, ip);
    w8(SCRIPT + ip + 1, (uint8_t)(pick < 4 ? pick : rnd()));
}
static void g_0EE8(Args* a) { (void)a; setup_ptrs(); set_subop(rnd_range(4)); }
static void g_0FD8(Args* a) {
    (void)a; setup_ptrs(); set_subop(rnd_range(5));
    if (rnd() % 4) w32(0x800ADB2C, 0);
}
static void g_14F0(Args* a) { (void)a; setup_ptrs(); feed[0] = small(0, 40); }
static void g_E574(Args* a) {
    setup_ptrs();
    int16_t n = (int16_t)small(2, 5);
    w16(0x800AFB54, (uint16_t)n);
    w16(CURACT + 0x10, (uint16_t)rnd_range((uint32_t)(n - 1)));
    a->a0 = (uint32_t)rnd_val();
    a->a1 = (uint32_t)rnd_val();
}

Fn fns[] = {
    { "func_800A0EE8", 0x800A0EE8, 60, n_0EE8, g_0EE8, 1500, 0 },
    { "func_800A0FD8", 0x800A0FD8, 227, n_0FD8, g_0FD8, 5000, 0 },
    { "func_800A14F0", 0x800A14F0, 77, n_14F0, g_14F0, 1500, 0 },
    { "func_800A1624", 0x800A1624, 67, n_1624, g_14F0, 1500, 0 },
    { "func_8009E574", 0x8009E574, 167, n_E574, g_E574, 2500, 0 },
};
const unsigned NFNS = sizeof(fns) / sizeof(fns[0]);
