/* Retail differential, src/field/main/misc.c hand-written arms (batch miscb):
 * func_8008B5D4 8008B978 8008BC80 8008BF38 8008C334.
 * The production TU is linked unmodified; see lib/field_retail_core.h.
 *
 * PC layout modelled as production links it: D_8005A448 is NOT aliased (the
 * native arms must reach the retail D_8005A448 word through D_8005A444 + 1;
 * any reference to the separate PC stub would fail to link here), and
 * D_8005A418 is g_PartyDataBuffers + 8 exactly as pc_port/src/data_field.c
 * defines it (host void*[3]).  The 7th argument of func_80076AC0 is compared
 * through the xlog (see field_f2_misc_retail_test.c). */
#define TESTNAME "FIELD F2 MISCB"
#include <stdint.h>
#include "battle_mips_adapter.h"
/* 0x14000-byte party-buffer copies need ~52k interpreted steps each. */
#define PcPortMipsRun(c, e, h, n) PcPortMipsRun(c, e, h, 400000)

enum { S_GETS16 = 1, S_GETOFF, S_80A74, S_COPYPL, S_VMRUN, S_77268, S_A558, S_CDSYNC, S_GETARG, S_A790, S_A7DC,
       S_76AC0, S_DRAWSYNC, S_CF3C, S_IDTOPARTY, S_C180, S_BF38 };

static int32_t feed_map(uint32_t id, int32_t raw);
static void xlog_pre(void);
static void xlog_post(void);
#define FEED_MAP(id, raw) feed_map(id, raw)
#define NATIVE_PRE() xlog_pre()
#define NATIVE_POST() xlog_post()
#define BUF_BASE 0x80120000u
#define BUF_SIZE 0x80000u
#include "field_retail_core.h"

#define SCRIPT 0x80120000u
#define CURACT 0x80130000u
#define FACTORS 0x80131000u
#define ADATA 0x80132000u
#define GSTATE 0x80134000u
#define SPRITE 0x80137000u
#define MODEL 0x80138000u
#define HEADER 0x80138100u
#define CMD 0x80138200u
#define PAGES 0x80139000u
#define PARTY0 0x80140000u
#define NACT 16u

#define RA_BASE 0x8004F340u
#define RA_SIZE 0x20u
#define RB_BASE 0x8005A390u
#define RB_SIZE 0xC0u
#define RC_BASE 0x80062580u
#define RC_SIZE 0x20u
#define RD_BASE 0x8006F980u
#define RD_SIZE 0x20u
#define RE_BASE 0x8006FAB0u
#define RE_SIZE 0x20u
#define RF_BASE 0x800ADB00u
#define RF_SIZE 0x110u
#define RG_BASE 0x800AFB10u
#define RG_SIZE 0x10u
#define RH_BASE 0x800AFC70u
#define RH_SIZE 0x10u
#define RI_BASE 0x800AFD10u
#define RI_SIZE 0x10u
#define RJ_BASE 0x800AFFE0u
#define RJ_SIZE 0x10u
#define RK_BASE 0x800B0070u
#define RK_SIZE 0x60u
#define RL_BASE 0x800B06B0u
#define RL_SIZE 0x10u
#define RM_BASE 0x800B2260u
#define RM_SIZE 0x10u
REGION(RA); REGION(RB); REGION(RC); REGION(RD); REGION(RE); REGION(RF); REGION(RG); REGION(RH);
REGION(RI); REGION(RJ); REGION(RK); REGION(RL); REGION(RM);
ALIAS(g_GameSceneMapNum, RA, 0x8004F34C);
ALIAS(D_8005A444, RB, 0x8005A444);
ALIAS(g_GamePartyMembers, RC, 0x80062590);
ALIAS(D_8006F990, RD, 0x8006F990);
ALIAS(g_GamePartyMemberSkins, RE, 0x8006FABC);
ALIAS(g_FieldCurRenderContextIndex, RF, 0x800ADB08); ALIAS(D_800ADB1C, RF, 0x800ADB1C); ALIAS(D_800ADB2C, RF, 0x800ADB2C);
ALIAS(D_800ADBC4, RF, 0x800ADBC4); ALIAS(D_800ADBC8, RF, 0x800ADBC8); ALIAS(D_800ADBCC, RF, 0x800ADBCC);
ALIAS(D_800ADBFC, RF, 0x800ADBFC);
ALIAS(g_FieldScriptMaxInstructionCount, RH, 0x800AFC7C);
ALIAS(D_800AFD1C, RI, 0x800AFD1C);
ALIAS(D_800AFFEC, RJ, 0x800AFFEC);
ALIAS(D_800B00C0, RK, 0x800B00C0);
ALIAS(g_PlayerActorIndex, RM, 0x800B226C);
uintptr_t g_FieldScriptVMCurActor, g_FieldScriptVMCurScriptData, g_FieldActors, g_pGameState, D_800B06B8,
    g_FieldSpriteData;
uintptr_t g_PartyDataBuffers[3];
__asm__(".globl D_8005A418\n.set D_8005A418, g_PartyDataBuffers + 8");

const Region regions[] = {
    { RA_BASE, RA_SIZE, RA }, { RB_BASE, RB_SIZE, RB }, { RC_BASE, RC_SIZE, RC }, { RD_BASE, RD_SIZE, RD },
    { RE_BASE, RE_SIZE, RE }, { RF_BASE, RF_SIZE, RF }, { RG_BASE, RG_SIZE, RG }, { RH_BASE, RH_SIZE, RH },
    { RI_BASE, RI_SIZE, RI }, { RJ_BASE, RJ_SIZE, RJ }, { RK_BASE, RK_SIZE, RK }, { RL_BASE, RL_SIZE, RL },
    { RM_BASE, RM_SIZE, RM },
};
const unsigned NREG = sizeof(regions) / sizeof(regions[0]);
const PtrGlobal ptrglobals[] = {
    { 0x800B0078u, &g_FieldScriptVMCurActor }, { 0x800ADC00u, &g_FieldScriptVMCurScriptData },
    { 0x800AFB10u, &g_FieldActors }, { 0x8005A39Cu, &g_pGameState }, { 0x800B06B8u, &D_800B06B8 },
    { 0x800AFB1Cu, &g_FieldSpriteData },
    { 0x8005A414u, &g_PartyDataBuffers[0] }, { 0x8005A418u, &g_PartyDataBuffers[1] },
    { 0x8005A41Cu, &g_PartyDataBuffers[2] },
};
const unsigned NPTR = sizeof(ptrglobals) / sizeof(ptrglobals[0]);

/* ---- extra observation log ------------------------------------------------ */
static uint32_t xlog[2][64];
static unsigned xn[2];
static uint32_t stash_arg7;
static void xrec(uint32_t v) {
    int side = active_cpu ? 0 : 1;
    require(xn[side] < 64, "xlog size");
    xlog[side][xn[side]++] = v;
}
static int32_t feed_map(uint32_t id, int32_t raw) {
    if (id == S_76AC0) xrec(active_cpu ? r32(active_cpu->gpr[29] + 0x18) : stash_arg7);
    if (id == S_GETOFF) return raw & 0xFFFF; /* callee returns u_short */
    return raw;
}
static void xlog_pre(void) { xn[1] = 0; }
static void xlog_post(void) {
    require(xn[0] == xn[1], "xlog count");
    require(memcmp(xlog[0], xlog[1], 4 * xn[0]) == 0, "xlog values (extra observations)");
}

SPY(FieldScriptVMGetInstructionArgumentS16, S_GETS16)
SPY(FieldScriptGetBytecodeOffset, S_GETOFF)
SPY(func_80080A74, S_80A74)
SPY(FieldActorCopyPlacement, S_COPYPL)
SPY(FieldScriptVMRun, S_VMRUN)
SPY(func_80077268, S_77268)
SPY(func_8008A558, S_A558)
SPY(ArchiveCdDataSync, S_CDSYNC)
SPY(FieldScriptVMGetArgument, S_GETARG)
SPY(func_8008A790, S_A790)
SPY(func_8008A7DC, S_A7DC)
SPY(DrawSync, S_DRAWSYNC)
SPY(func_8008CF3C, S_CF3C)
SPY(FieldCharacterIdToPartyId, S_IDTOPARTY)
SPY(func_8008C180, S_C180)
SPY(func_8008BF38, S_BF38)
long func_80076AC0(long a, long b, long c, long d, long e, long f, long g) {
    uint64_t in[6] = { (uint64_t)a, (uint64_t)b, (uint64_t)c, (uint64_t)d, (uint64_t)e, (uint64_t)f };
    stash_arg7 = (uint32_t)g;
    return spy_core(S_76AC0, in);
}

const Target targets[] = {
    { 0x800ACD7C, S_GETS16, 1, 1, 0, 0 }, { 0x800A3090, S_GETOFF, 2, 1, 0, 0 }, { 0x80080A74, S_80A74, 1, 0, 0, 0 },
    { 0x8008D380, S_COPYPL, 2, 0, 0, 0 }, { 0x800A1EC8, S_VMRUN, 1, 0, 0, 0 }, { 0x80077268, S_77268, 0, 0, 0, 0 },
    { 0x8008A558, S_A558, 0, 1, 0, 0 }, { 0x80028A60, S_CDSYNC, 1, 0, 0, 0 }, { 0x800ACDEC, S_GETARG, 1, 1, 0, 0 },
    { 0x8008A790, S_A790, 2, 1, 0x2, 1 }, { 0x8008A7DC, S_A7DC, 2, 0, 0, 0 },
    { 0x80076AC0, S_76AC0, 6, 1, 0, 0 }, { 0x800445D0, S_DRAWSYNC, 1, 0, 0, 0 }, { 0x8008CF3C, S_CF3C, 1, 1, 0, 0 },
    { 0x8009FA00, S_IDTOPARTY, 1, 1, 0, 0 }, { 0x8008C180, S_C180, 1, 0, 0, 0 }, { 0x8008BF38, S_BF38, 1, 0, 0, 0 },
};
const unsigned NTARGETS = sizeof(targets) / sizeof(targets[0]);

/* ---- native entry wrappers --------------------------------------------- */
extern void func_8008B5D4(void);
extern void func_8008B978(int32_t);
extern void func_8008BC80(void);
extern void real_func_8008BF38(int32_t);
extern void func_8008C334(void);
static uint32_t n_B5D4(Args a) { (void)a; func_8008B5D4(); return 0; }
static uint32_t n_B978(Args a) { func_8008B978((int32_t)a.a0); return 0; }
static uint32_t n_BC80(Args a) { (void)a; func_8008BC80(); return 0; }
static uint32_t n_BF38(Args a) { real_func_8008BF38((int32_t)a.a0); return 0; }
static uint32_t n_C334(Args a) { (void)a; func_8008C334(); return 0; }

static uint16_t ipv;
static void setup(void) {
    xn[0] = xn[1] = 0;
    w32(0x800B0078u, CURACT); w32(0x800ADC00u, SCRIPT); w32(0x800AFB10u, FACTORS); w32(0x8005A39Cu, GSTATE);
    w32(0x800AFB1Cu, SPRITE);
    for (unsigned k = 0; k < NACT; k++) w32(FACTORS + k * 0x5C + 0x4C, ADATA + k * 0x200);
    ipv = (uint16_t)rnd_range(0x7000);
    w16(CURACT + 0xCC, ipv);
}
static void party_bufs(void) {
    uint32_t b[3] = { PARTY0, PARTY0 + 0x14000, PARTY0 + 0x28000 };
    /* occasionally overlapping buffers: retail copies 16-byte blocks (four
     * loads, then four stores) */
    if (rnd() % 6 == 0) { unsigned i = rnd_range(2); b[i] = b[i + 1] + (uint32_t)(16 * small(-3, 3)); }
    for (unsigned i = 0; i < 3; i++) w32(0x8005A414u + 4 * i, b[i]);
}

static void g_B5D4(Args* a) {
    (void)a; setup();
    uint32_t idx = rnd_range(NACT);
    w32(0x800AFD1C, idx);
    w32(FACTORS + idx * 0x5C, MODEL);
    uint32_t page = rnd_range(2);
    w32(0x800ADB08, page);
    uint32_t cur = PAGES, alt;
    switch (rnd() % 5) {
    case 0: alt = cur; break;
    case 1: alt = cur + (uint32_t)(8 * small(-6, 6)); break;
    default: alt = PAGES + 0x1000; break;
    }
    w32(MODEL + 4, HEADER);
    w32(MODEL + 8 + page * 4, cur);
    w32(MODEL + 8 + (page ^ 1) * 4, alt);
    w32(HEADER + 0x10, CMD);
    unsigned ncmd = rnd_range(7);
    w16(HEADER + 6, (uint16_t)ncmd);
    uint32_t c = CMD;
    for (unsigned i = 0; i < ncmd; i++) {
        uint32_t op = (rnd() % 5 == 0) ? ((rnd() & 1) ? 0xC4u : 0xC8u) : (rnd() & 0xFFu);
        uint32_t n = rnd_range(6);
        uint32_t h = (rnd() & 0xFF00u) | op | (n << 16);
        w32(c, h); c += 4;
        if (op != 0xC4 && op != 0xC8) c += 8 * n;
    }
}
static void g_B978(Args* a) {
    setup();
    a->a0 = (rnd() & 1) ? (uint32_t)small(0, 12) : (uint32_t)rnd_val();
    w32(0x800ADBC8, (rnd() & 1) ? rnd_range(16) : rnd());
    if (rnd() % 4 == 0) w32(0x800ADB1C, 0);
    w32(0x800ADBFC, (uint32_t)small(-1, 12));
    w32(0x800ADBCC, rnd_range(3));
    for (unsigned i = 0; i < 3; i++) w32(0x8006F990 + 4 * i, rnd_range(NACT));
    w32(0x8004F34C, (rnd() & 1) ? (rnd() & ~0xC000u) : rnd());
    for (unsigned k = 0; k < 20; k++) {
        feed[k] = rnd_val();
        uint16_t off = (uint16_t)feed[k];
        w8(SCRIPT + off, (rnd() % 3) ? 0x16 : (uint8_t)rnd());
        w8(SCRIPT + off + 1u, (rnd() % 3) ? (uint8_t)a->a0 : (uint8_t)rnd());
    }
}
static void g_BC80(Args* a) {
    (void)a; setup();
    if (rnd() % 5) w32(0x800ADBC4, 0xFF);
    if (rnd() % 5) w32(0x800ADB2C, 0);
    feed[0] = (rnd() % 5 == 0) ? rnd_val() : 0;
    feed[2] = (rnd() % 5 == 0) ? 0xFF : (rnd() & 1) ? small(0, 40) : rnd_val();
    feed[3] = (rnd() % 3 == 0) ? rnd_val() : 0;
    outfeed[3][0] = (int32_t)rnd_range(3);
}
static void g_BF38(Args* a) {
    setup(); party_bufs();
    a->a0 = rnd_range(2);
    w32(0x8005A444u + 4 * (a->a0 + 1), (rnd() % 3 == 0) ? 0xFFu : rnd_range(NACT));
}
static void g_C334(Args* a) {
    (void)a; setup(); party_bufs();
    if (rnd() % 6) w32(0x800ADBC4, 0xFF);
    feed[2] = (rnd() % 6 == 0) ? rnd_val() : small(-2, 3);
    if (rnd() & 1) w32(0x800ADB1C, 0);
    for (unsigned i = 0; i < 3; i++) if (rnd() % 3 == 0) w32(0x80062590 + 4 * i, 0xFF);
    w32(0x8005A444u, (rnd() % 3 == 0) ? 0xFFu : rnd_range(NACT));
}

Fn fns[] = {
    { "func_8008B5D4", 0x8008B5D4, 176, n_B5D4, g_B5D4, 2000, 0 },
    { "func_8008B978", 0x8008B978, 194, n_B978, g_B978, 2000, 0 },
    { "func_8008BC80", 0x8008BC80, 86, n_BC80, g_BC80, 1500, 0 },
    { "func_8008BF38", 0x8008BF38, 146, n_BF38, g_BF38, 600, 0 },
    { "func_8008C334", 0x8008C334, 297, n_C334, g_C334, 800, 0 },
};
const unsigned NFNS = sizeof(fns) / sizeof(fns[0]);
