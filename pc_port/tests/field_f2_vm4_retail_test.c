/* Retail differential, src/field/scripts/virtual_machine.c: func_800A28D4
 * (party skin attach / first-frame script init).  The production TU is
 * linked unmodified; see lib/field_retail_core.h.  func_80076AC0's 7th
 * (stack) argument is captured on both sides and compared in NATIVE_POST.
 * g_PartyDataBuffers is a host void*[3] on PC: mapped per element as
 * pointer globals; skinId is kept < 3 on the non-0x80 path (retail would
 * index the neighbouring words).  Port-only: a one-shot diagnostic printf
 * on the first func_80076AC0 attach (stdout only, no game state). */
#define TESTNAME "FIELD F2 VM4"
enum { S_3474 = 1, S_76AC0, S_2303C, S_WRU16, S_PARTYIDS, S_BCOFF, S_VMRUN };
#include <stdint.h>
static int32_t feed_map(uint32_t id, int32_t raw);
#define FEED_MAP(id, raw) feed_map(id, raw)
static void native_pre(void);
static void native_post(void);
#define NATIVE_PRE() native_pre()
#define NATIVE_POST() native_post()
#define MAXEV 128
#define BUF_BASE 0x80120000u
#define BUF_SIZE 0x40000u
#include "field_retail_core.h"

#define FACTORS 0x80120000u
#define ADATA 0x80121000u
#define SPR 0x80130000u
#define SDATA 0x80138000u
#define GSTATE 0x80140000u
#define SCRIPT 0x80150000u
#define PBUF 0x80158000u
#define NACT 8u

#define R4F_BASE 0x8004F300u
#define R4F_SIZE 0x10u
#define R5A_BASE 0x8005A440u
#define R5A_SIZE 0x10u
#define R6F_BASE 0x8006F990u
#define R6F_SIZE 0x10u
#define RDB_BASE 0x800ADBF0u
#define RDB_SIZE 0x10u
#define RFC_BASE 0x800AFC60u
#define RFC_SIZE 0xD0u
#define RFF_BASE 0x800AFFE0u
#define RFF_SIZE 0x10u
#define RB0_BASE 0x800B0070u   /* g_FieldScriptVMCurActor (pointer global, mirrored) */
#define RB0_SIZE 0x10u
#define RB6_BASE 0x800B06B0u   /* D_800B06B8 (pointer global, mirrored) */
#define RB6_SIZE 0x10u
#define RB2_BASE 0x800B2260u
#define RB2_SIZE 0x10u
REGION(R4F); REGION(R5A); REGION(R6F); REGION(RDB); REGION(RFC); REGION(RFF); REGION(RB0); REGION(RB6); REGION(RB2);
ALIAS(g_GamePartySkinsInitialized, R4F, 0x8004F30C);
ALIAS(D_8005A444, R5A, 0x8005A444);
ALIAS(D_8006F990, R6F, 0x8006F990);
ALIAS(D_800ADBFC, RDB, 0x800ADBFC);
ALIAS(D_800AFC74, RFC, 0x800AFC74); ALIAS(D_800AFD1C, RFC, 0x800AFD1C);
ALIAS(D_800AFFEC, RFF, 0x800AFFEC);
ALIAS(D_800B2268, RB2, 0x800B2268);
uintptr_t g_FieldActors, g_FieldSpriteData, g_pGameState, g_FieldScriptVMCurActor, g_FieldScriptVMCurScriptData,
          D_800B06B8, g_PartyDataBuffers[3];
const Region regions[] = {
    { R4F_BASE, R4F_SIZE, R4F }, { R5A_BASE, R5A_SIZE, R5A }, { R6F_BASE, R6F_SIZE, R6F }, { RDB_BASE, RDB_SIZE, RDB },
    { RFC_BASE, RFC_SIZE, RFC }, { RFF_BASE, RFF_SIZE, RFF }, { RB0_BASE, RB0_SIZE, RB0 }, { RB6_BASE, RB6_SIZE, RB6 },
    { RB2_BASE, RB2_SIZE, RB2 },
};
const unsigned NREG = sizeof(regions) / sizeof(regions[0]);
const PtrGlobal ptrglobals[] = {
    { 0x800AFB10u, &g_FieldActors }, { 0x800AFB1Cu, &g_FieldSpriteData }, { 0x8005A39Cu, &g_pGameState },
    { 0x800B0078u, &g_FieldScriptVMCurActor }, { 0x800ADC00u, &g_FieldScriptVMCurScriptData },
    { 0x800B06B8u, &D_800B06B8 },
    { 0x8005A414u, &g_PartyDataBuffers[0] }, { 0x8005A418u, &g_PartyDataBuffers[1] }, { 0x8005A41Cu, &g_PartyDataBuffers[2] },
};
const unsigned NPTR = sizeof(ptrglobals) / sizeof(ptrglobals[0]);

/* extra (stack) argument #7 of func_80076AC0, both sides */
static uint32_t xr[MAXEV], xn[MAXEV];
static unsigned xr_n, xn_n;

SPY(func_800A3474, S_3474)
SPY(func_8002303C, S_2303C)
SPY(FieldScriptMemoryWriteU16, S_WRU16)
SPY(FieldScriptWritePartyMemberIDs, S_PARTYIDS)
SPY(FieldScriptGetBytecodeOffset, S_BCOFF)
SPY(FieldScriptVMRun, S_VMRUN)
long func_80076AC0(long a, long b, long c, long d, long e, long f, long g) {
    require(xn_n < MAXEV, "extra count"); xn[xn_n++] = (uint32_t)g;
    uint64_t in[6] = { (uint64_t)a, (uint64_t)b, (uint64_t)c, (uint64_t)d, (uint64_t)e, (uint64_t)f };
    return spy_core(S_76AC0, in);
}
const Target targets[] = {
    { 0x800A3474, S_3474, 0, 0, 0, 0 },
    { 0x80076AC0, S_76AC0, 6, 1, 0, 0 },
    { 0x8002303C, S_2303C, 3, 0, 0, 0 },
    { 0x800A3074, S_WRU16, 2, 0, 0, 0 },
    { 0x800A30B4, S_PARTYIDS, 0, 0, 0, 0 },
    { 0x800A3090, S_BCOFF, 2, 1, 0, 0 },
    { 0x800A1EC8, S_VMRUN, 1, 0, 0, 0 },
};
const unsigned NTARGETS = sizeof(targets) / sizeof(targets[0]);

static int32_t feed_map(uint32_t id, int32_t raw) {
    if (id == S_BCOFF) return (int32_t)((uint32_t)raw & 0x7FFFu);
    if (id == S_76AC0) {   /* void callee: v0 dead; capture retail stack arg #7 */
        if (active_cpu) { require(xr_n < MAXEV, "extra count"); xr[xr_n++] = r32(active_cpu->gpr[29] + 0x18); }
        return 0;
    }
    return raw;
}
static void native_pre(void) { xn_n = 0; }
static void native_post(void) {
    require(xn_n == xr_n, "stack-argument count");
    require(memcmp(xn, xr, 4 * xn_n) == 0, "stack argument 7");
}

extern void func_800A28D4(void);
static uint32_t n_28D4(Args a) { (void)a; func_800A28D4(); return 0; }
static void g_28D4(Args* a) {
    (void)a;
    xr_n = 0;
    w32(0x800AFB10u, FACTORS); w32(0x800AFB1Cu, SDATA); w32(0x8005A39Cu, GSTATE);
    w32(0x800ADC00u, SCRIPT); w32(0x800B0078u, ADATA);
    for (unsigned k = 0; k < 3; k++) w32(0x8005A414u + 4 * k, PBUF + k * 0x1000);
    for (unsigned k = 0; k < 0x80; k++) w32(SDATA + 4 + 4 * k, (uint32_t)rnd_range(0x4000) & ~3u);
    for (unsigned k = 0; k < NACT; k++) {
        uint32_t ad = ADATA + k * 0x200, sp = SPR + k * 0x400;
        w32(FACTORS + k * 0x5C + 0x04, sp);
        w32(FACTORS + k * 0x5C + 0x4C, ad);
        w32(sp + 0x7C, sp + 0x100);
        w32(sp + 0x100 + 0x18, sp + 0x200);
        uint8_t skin = (rnd() & 1) ? (uint8_t)rnd_range(3) : (uint8_t)(0x80 | rnd_range(0x80));
        w8(ad + 0x126, skin);
        w16(ad + 0x12E, (uint16_t)((r32(ad + 0x12C) >> 16) & ~3u) | (uint16_t)rnd_range(4));
    }
    w32(0x8004F30C, (rnd() & 1) ? 1 : 0);
    w32(0x800ADBFC, (uint32_t)small(-1, NACT));
    w32(0x800B2268, (rnd() & 1) ? 0 : rnd());
    for (unsigned k = 0; k < 3; k++) {
        w32(0x8005A444 + 4 * k, (rnd() % 4) ? rnd_range(NACT) : 0xFF);
        w32(0x8006F990 + 4 * k, rnd_range(NACT));
    }
    w32(0x800AFC74, (rnd() & 1) ? 0 : rnd());
}
Fn fns[] = { { "func_800A28D4", 0x800A28D4, 443, n_28D4, g_28D4, 4000, 0 } };
const unsigned NFNS = sizeof(fns) / sizeof(fns[0]);
