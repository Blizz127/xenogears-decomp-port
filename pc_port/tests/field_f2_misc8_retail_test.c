/* Retail differential, src/field/main/misc8.c hand-written arms (first batch):
 * func_8008492C 80086078 800860F0 800863E8 80086590 80086C34 800859DC 80085B20.
 * The production TU is linked unmodified; see lib/field_retail_core.h. */
#define TESTNAME "FIELD F2 MISC8"
enum { S_SOUNDVOL = 1, S_W2S, S_SPU344, S_SPU55C, S_SPU20C, S_VEC3, S_SLOT, S_SND860F0, S_SND862CC,
       S_GETARG, S_PBANK, S_PBANKS, S_2945C, S_380D0, S_3BDFC, S_WDSPART, S_CDSYNC, S_1B66C, S_SETIDX,
       S_86024, S_85560, S_HEAPALLOC };
#include <stdint.h>
static int32_t feed_map(uint32_t id, int32_t raw) {
    if (id == S_SLOT) return (int32_t)((uint32_t)raw % 4u) - 1; /* -1..2 */
    return raw;
}
#define FEED_MAP(id, raw) feed_map(id, raw)

/* BUF window must cover ADATA (0x80160000): sibling misc-ops test uses 0x60000
 * for the same layout; the core.h 0x40000 default leaves ADATA unmapped and
 * the native body segfaults on the host pointer. */
#define BUF_BASE 0x80120000u
#define BUF_SIZE 0x60000u
#include "field_retail_core.h"

#define SCRIPT 0x80120000u
#define CURACT 0x80130000u
#define FACTORS 0x80150000u
#define ADATA 0x80160000u
#define NACT 16u

/* globals regions (retail symbol names alias into them) */
#define RA_BASE 0x800AFE80u
#define RA_SIZE 0x40u
#define RB_BASE 0x800B21A0u
#define RB_SIZE 0x20u
#define RC_BASE 0x800ADB90u
#define RC_SIZE 0x10u
#define RD_BASE 0x800ADBF0u
#define RD_SIZE 0x10u
#define RE_BASE 0x800ADC00u
#define RE_SIZE 0x10u
#define RF_BASE 0x800ADFC0u
#define RF_SIZE 0x220u
#define RG_BASE 0x800AFC40u
#define RG_SIZE 0x40u
#define RH_BASE 0x800AFD00u
#define RH_SIZE 0x40u
#define RI_BASE 0x800B02CCu
#define RI_SIZE 0x3C0u
#define RJ_BASE 0x8004F300u
#define RJ_SIZE 0x60u
#define RK_BASE 0x80062580u
#define RK_SIZE 0x20u
#define RL_BASE 0x800B2370u
#define RL_SIZE 0x10u
#define RM_BASE 0x800C3A10u
#define RM_SIZE 0x20u
#define RN_BASE 0x800AFB10u
#define RN_SIZE 0x10u
#define RO_BASE 0x800B0070u
#define RO_SIZE 0x10u
REGION(RA); REGION(RB); REGION(RC); REGION(RD); REGION(RE); REGION(RF); REGION(RG); REGION(RH);
REGION(RI); REGION(RJ); REGION(RK); REGION(RL); REGION(RM); REGION(RN); REGION(RO);
ALIAS(D_800AFE88, RA, 0x800AFE88); ALIAS(D_800AFE8A, RA, 0x800AFE8A);
ALIAS(D_800B21AC, RB, 0x800B21AC);
ALIAS(D_800ADB98, RC, 0x800ADB98);
ALIAS(D_800ADBFC, RD, 0x800ADBFC);
ALIAS(D_800ADC0C, RE, 0x800ADC0C);
ALIAS(D_800ADFCC, RF, 0x800ADFCC);
ALIAS(D_800AFC54, RG, 0x800AFC54);
ALIAS(D_800AFD1C, RH, 0x800AFD1C);
ALIAS(g_FieldDefaultParticleBanks, RI, 0x800B02CC);
ALIAS(D_8004F308, RJ, 0x8004F308); ALIAS(D_8004F33C, RJ, 0x8004F33C); ALIAS(D_8004F354, RJ, 0x8004F354);
ALIAS(D_800B2370, RL, 0x800B2370);
uintptr_t g_FieldActors, g_FieldScriptVMCurActor, g_FieldScriptVMCurScriptData, D_800C3A1C, g_GameCurLoadedWDS;

const Region regions[] = {
    { RA_BASE, RA_SIZE, RA }, { RB_BASE, RB_SIZE, RB }, { RC_BASE, RC_SIZE, RC }, { RD_BASE, RD_SIZE, RD },
    { RE_BASE, RE_SIZE, RE }, { RF_BASE, RF_SIZE, RF }, { RG_BASE, RG_SIZE, RG }, { RH_BASE, RH_SIZE, RH },
    { RI_BASE, RI_SIZE, RI }, { RJ_BASE, RJ_SIZE, RJ }, { RK_BASE, RK_SIZE, RK }, { RL_BASE, RL_SIZE, RL },
    { RM_BASE, RM_SIZE, RM }, { RN_BASE, RN_SIZE, RN }, { RO_BASE, RO_SIZE, RO },
};
const unsigned NREG = sizeof(regions) / sizeof(regions[0]);
const PtrGlobal ptrglobals[] = {
    { 0x800AFB10u, &g_FieldActors }, { 0x800B0078u, &g_FieldScriptVMCurActor },
    { 0x800ADC00u, &g_FieldScriptVMCurScriptData }, { 0x800C3A1Cu, &D_800C3A1C }, { 0x8006258Cu, &g_GameCurLoadedWDS },
};
const unsigned NPTR = sizeof(ptrglobals) / sizeof(ptrglobals[0]);

extern void func_800859DC(void);
SPY(func_80086078, S_SOUNDVOL)
SPY(FieldActorWorldToScreenPosition, S_W2S)
SPY(func_8003A344, S_SPU344)
SPY(func_8003A55C, S_SPU55C)
SPY(func_8003A20C, S_SPU20C)
SPY(FieldGetVec3Magnitude, S_VEC3)
SPY(func_80086470, S_SLOT)
SPY(func_800860F0, S_SND860F0)
SPY(func_800862CC, S_SND862CC)
SPY(FieldScriptVMGetArgument, S_GETARG)
SPY(FieldInitializeDefaultParticleBanks, S_PBANK)
SPY(FieldInitializeParticleBanks, S_PBANKS)
SPY(func_8002945C, S_2945C)
SPY(func_800380D0, S_380D0)
SPY(func_8003BDFC, S_3BDFC)
SPY(SoundTransferWdsPart, S_WDSPART)
SPY(ArchiveCdDataSync, S_CDSYNC)
SPY(func_8001B66C, S_1B66C)
SPY(ArchiveSetIndex, S_SETIDX)
SPY(func_80086024, S_86024)
SPY(HeapAlloc, S_HEAPALLOC)
long func_80085560(long a, long b, long c, long d, long e, long f) {
    /* third argument is the host address of func_800859DC natively, 0x800859DC in the guest */
    uint64_t in[6] = { (uint64_t)a, (uint64_t)b, (c == (long)(uintptr_t)func_800859DC) ? 0x800859DCu : (uint64_t)c,
                       (uint64_t)d, (uint64_t)e, (uint64_t)f };
    return spy_core(S_85560, in);
}

const Target targets[] = {
    { 0x80086078, S_SOUNDVOL, 3, 0, 0x2, 1 }, { 0x80086200, S_W2S, 3, 0, 0x6, 1 },
    { 0x8003A344, S_SPU344, 2, 0, 0, 0 }, { 0x8003A55C, S_SPU55C, 2, 0, 0, 0 }, { 0x8003A20C, S_SPU20C, 1, 0, 0, 0 },
    { 0x80099A04, S_VEC3, 3, 1, 0, 0 }, { 0x80086470, S_SLOT, 2, 1, 0, 0 },
    { 0x800860F0, S_SND860F0, 5, 0, 0, 0 }, { 0x800862CC, S_SND862CC, 5, 0, 0, 0 },
    { 0x800ACDEC, S_GETARG, 1, 1, 0, 0 }, { 0x800A94A4, S_PBANK, 1, 0, 0, 0 }, { 0x800A99A8, S_PBANKS, 1, 0, 0, 0 },
    { 0x8002945C, S_2945C, 1, 1, 0, 0 }, { 0x800380D0, S_380D0, 3, 1, 0, 0 }, { 0x8003BDFC, S_3BDFC, 1, 0, 0, 0 },
    { 0x8003827C, S_WDSPART, 2, 0, 0, 0 }, { 0x80028A60, S_CDSYNC, 1, 0, 0, 0 }, { 0x8001B66C, S_1B66C, 0, 0, 0, 0 },
    { 0x80028470, S_SETIDX, 2, 0, 0, 0 }, { 0x80086024, S_86024, 0, 0, 0, 0 }, { 0x80085560, S_85560, 3, 0, 0, 0 },
    { 0x80031BDC, S_HEAPALLOC, 2, 1, 0, 0 },
};
const unsigned NTARGETS = sizeof(targets) / sizeof(targets[0]);

/* ---- native entry wrappers --------------------------------------------- */
extern int32_t func_8008492C(uint8_t*);
extern void real_func_80086078(int32_t, int32_t*, int32_t);
extern void real_func_800860F0(int32_t, int32_t, int16_t, int32_t, int32_t);
extern void func_800863E8(int32_t);
extern void func_80086590(void*);
extern void func_80086C34(void);
extern void real_func_800859DC(uint8_t*);
extern void func_80085B20(int32_t);
static uint32_t n_492C(Args a) { return (uint32_t)func_8008492C((uint8_t*)(uintptr_t)a.a0); }
static uint32_t n_6078(Args a) { real_func_80086078((int32_t)a.a0, (int32_t*)(uintptr_t)a.a1, (int32_t)a.a2); return 0; }
static uint32_t n_60F0(Args a) { real_func_800860F0((int32_t)a.a0, (int32_t)a.a1, (int16_t)a.a2, (int32_t)a.a3, (int32_t)a.a4); return 0; }
static uint32_t n_63E8(Args a) { func_800863E8((int32_t)a.a0); return 0; }
static uint32_t n_6590(Args a) { func_80086590((void*)(uintptr_t)a.a0); return 0; }
static uint32_t n_6C34(Args a) { (void)a; func_80086C34(); return 0; }
static uint32_t n_59DC(Args a) { real_func_800859DC((uint8_t*)(uintptr_t)a.a0); return 0; }
static uint32_t n_5B20(Args a) { func_80085B20((int32_t)a.a0); return 0; }

static void setup_ptrs(void) {
    w32(0x800AFB10u, FACTORS); w32(0x800B0078u, CURACT); w32(0x800ADC00u, SCRIPT);
    for (unsigned k = 0; k < NACT; k++) w32(FACTORS + k * 0x5C + 0x4C, ADATA + k * 0x200);
}
static void set_ip(void) { w16(CURACT + 0xCC, (uint16_t)rnd_range(0x7000)); }

static void g_492C(Args* a) {
    setup_ptrs();
    uint32_t ad = ADATA + rnd_range(NACT) * 0x200; a->a0 = ad;
    unsigned depth = rnd_range(12);
    if (depth > 0) w32(ad + 0x14, r32(ad + 0x14) & ~0x420000u);
    if (depth > 1) w32(0x800ADB98, 0);
    if (depth > 2) w32(ad + 0x30, 0);
    if (depth > 3) w32(ad + 0x34, 0);
    if (depth > 4) w32(ad + 0x38, 0);
    if (depth > 5) w32(0x800ADC0C, 1);
    if (depth > 6) w8(ad + 0x74, 0xFF);
    if (depth > 7) w32(ad + 0x00, r32(ad) & ~0x401800u);
    if (depth > 8) { w32(ad + 0x04, r32(ad + 4) & ~7u); w32(ad + 0x04, r32(ad + 4) | (rnd() & 7)); w16(ad + 0x10, (uint16_t)rnd_range(4)); }
}
static void g_6078(Args* a) {
    a->a0 = (uint32_t)rnd_val();
    if (rnd() & 1) a->a0 = (uint32_t)small(-5, 0x300);
    a->a1 = BUF_BASE + 0x1000 + 4 * rnd_range(100);
    a->a2 = (uint32_t)rnd_val();
    if (rnd() % 3) w16(0x800B21AC, (uint16_t)small(0, 0x400));
    if (rnd() % 10 == 0) w16(0x800B21AC, 1);
}
static void g_60F0(Args* a) {
    a->a0 = rnd(); a->a1 = (uint32_t)rnd_val(); a->a2 = (uint32_t)(int32_t)(int16_t)rnd(); a->a3 = (uint32_t)rnd_val();
    a->a4 = (rnd() % 3) ? rnd_range(6) : rnd();
    for (unsigned i = 0; i < 3; i++) if (rnd() % 3 == 0) w16(0x800AFE88 + i * 6, (uint16_t)a->a4);
    if (rnd() % 3 == 0) w16(0x800AFE88 + rnd_range(3) * 6, (uint16_t)a->a4);
}
static void g_63E8(Args* a) {
    a->a0 = (rnd() % 4) ? rnd_range(6) : rnd();
    for (unsigned i = 0; i < 3; i++) if (rnd() % 3 == 0) w16(0x800AFE88 + i * 6, (uint16_t)a->a0);
}
static void g_6590(Args* a) {
    setup_ptrs();
    a->a0 = BUF_BASE + 0x2000 + 16 * rnd_range(100);
    w32(0x800ADBFC, rnd_range(NACT + 1));
    for (unsigned i = 0; i < NACT; i++) {
        uint32_t ad = ADATA + i * 0x200;
        if (rnd() % 3) w8(ad + 0x10D, (uint8_t)rnd_range(2) * 0xFF ? 0xFF : (uint8_t)rnd());
        else w8(ad + 0x10D, 0);
        for (unsigned o = 0x22; o <= 0x2A; o += 4) w16(ad + o, (uint16_t)small(-300, 300));
    }
    for (unsigned o = 2; o <= 0xA; o += 4) w16(a->a0 + o, (uint16_t)small(-300, 300));
    for (unsigned i = 0; i < 3; i++) if (rnd() % 2) w16(0x800AFE88 + i * 6, (uint16_t)rnd_range(NACT)); else w16(0x800AFE8A + i * 6, 0xFFFF);
    for (unsigned i = 0; i < 3; i++) if (rnd() % 2) w16(0x800AFE8A + i * 6, 0xFFFF);
}
static void g_6C34(Args* a) {
    (void)a; set_ip(); setup_ptrs();
    uint32_t ip = r32(CURACT) ; (void)ip;
    uint16_t ipv = (uint16_t)rnd_range(0x7000);
    w16(CURACT + 0xCC, ipv);
    w8(SCRIPT + ipv + 1, (uint8_t)(rnd() % 3));
    w32(0x800AFD1C, (uint32_t)rnd_val());
    for (unsigned i = 0; i < 3; i++) feed[i] = (rnd() % 3 == 0) ? 0x27 : rnd_val();
}
static void g_59DC(Args* a) {
    a->a0 = BUF_BASE + 0x8000 + 4 * rnd_range(100);
    w32(0x800B2370, (uint32_t)small(-2, 6));
    w32(0x800C3A1C, BUF_BASE + 0x20000);
    feed[1] = rnd_val();
}
static void g_5B20(Args* a) {
    a->a0 = (rnd() % 5 == 0) ? 0xFF : rnd_range(0x100);
    if (a->a0 != 0xFF) {
        if (rnd() & 1) w8(0x800ADFCC + a->a0 * 2 + 1, 1);
        if (rnd() & 1) w8(0x800ADFCC + a->a0 * 2, 0xFF);
        else if (rnd() & 1) w32(0x8004F33C, r32(0x800ADFCC + a->a0 * 2) & 0xFF);
    }
    feed[5] = (int32_t)(BUF_BASE + 0x30000);
}

Fn fns[] = {
    { "func_8008492C", 0x8008492C, 69, n_492C, g_492C, 1500, 1 },
    { "func_80086078", 0x80086078, 30, n_6078, g_6078, 1500, 0 },
    { "func_800860F0", 0x800860F0, 68, n_60F0, g_60F0, 1500, 0 },
    { "func_800863E8", 0x800863E8, 34, n_63E8, g_63E8, 800, 0 },
    { "func_80086590", 0x80086590, 222, n_6590, g_6590, 1500, 0 },
    { "func_80086C34", 0x80086C34, 70, n_6C34, g_6C34, 800, 0 },
    { "func_800859DC", 0x800859DC, 81, n_59DC, g_59DC, 800, 0 },
    { "func_80085B20", 0x80085B20, 71, n_5B20, g_5B20, 1500, 0 },
};
const unsigned NFNS = sizeof(fns) / sizeof(fns[0]);
