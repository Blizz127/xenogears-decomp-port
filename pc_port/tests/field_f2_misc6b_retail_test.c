/* Retail differential, src/field/main/misc6.c port arms (batch b):
 * func_800A0228 (party/gear sprite bind) and func_800A08B8 (party sprite bind).
 * The production TU is linked unmodified; see lib/field_retail_core.h.
 * func_80076AC0 takes 7 arguments; the core records 6, so the 7th (retail
 * sp+0x18) is captured separately on both sides and compared in NATIVE_POST.
 * Coverage limits: the FieldScriptVMGetArgument slot is kept in 0..5 (retail
 * indexes D_8006F990/g_GamePartyMembers with it unchecked when negative);
 * characterId is kept inside D_800AE294's mapped window and D_800B2268 small
 * (retail reads the skin table at g_FieldSpriteData + 4*(skin+D_800B2268)+4). */
#define TESTNAME "FIELD F2 MISC6B"
enum { S_GETARG = 1, S_GEAR, S_A0158, S_76AC0, S_9E574, S_A0C94, S_A0D3C, S_8CF3C, S_TOPARTY, S_GETVAR, S_FA54 };
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

#define CURACT 0x80130000u
#define FACTORS 0x80131000u
#define SKIN 0x80150000u
#define GSTATE 0x80158000u
#define NACT 16u

#define RC_BASE 0x800AE290u
#define RC_SIZE 0x110u
#define RE_BASE 0x800AFB10u
#define RE_SIZE 0x50u
#define RF_BASE 0x800AFD10u
#define RF_SIZE 0x20u
#define RH_BASE 0x800AFFE0u
#define RH_SIZE 0x10u
#define RI_BASE 0x800B0070u
#define RI_SIZE 0x60u
#define RJ_BASE 0x800B2170u
#define RJ_SIZE 0x240u
#define RL_BASE 0x8004F340u
#define RL_SIZE 0x10u
#define RM_BASE 0x8005A390u
#define RM_SIZE 0xF0u
#define RN_BASE 0x80062580u
#define RN_SIZE 0x20u
#define RO_BASE 0x8006F980u
#define RO_SIZE 0x20u
REGION(RC); REGION(RE); REGION(RF); REGION(RH); REGION(RI); REGION(RJ); REGION(RL); REGION(RM); REGION(RN); REGION(RO);
ALIAS(D_800AE294, RC, 0x800AE294);
ALIAS(D_800AFB54, RE, 0x800AFB54);
ALIAS(D_800AFD1C, RF, 0x800AFD1C); ALIAS(D_800AFD20, RF, 0x800AFD20);
ALIAS(D_800AFFEC, RH, 0x800AFFEC);
ALIAS(D_800B00C0, RI, 0x800B00C0);
ALIAS(D_800B2268, RJ, 0x800B2268); ALIAS(g_PlayerActorIndex, RJ, 0x800B226C); ALIAS(D_800B233E, RJ, 0x800B233E);
ALIAS(g_GameSceneMapNum, RL, 0x8004F34C);
ALIAS(D_8005A444, RM, 0x8005A444);
ALIAS(g_GamePartyMembers, RN, 0x80062590);
ALIAS(D_8006F990, RO, 0x8006F990);
uintptr_t g_FieldActors, g_FieldScriptVMCurActor, g_FieldSpriteData, g_pGameState, g_PartyDataBuffers[3];

const Region regions[] = {
    { RC_BASE, RC_SIZE, RC }, { RE_BASE, RE_SIZE, RE }, { RF_BASE, RF_SIZE, RF }, { RH_BASE, RH_SIZE, RH },
    { RI_BASE, RI_SIZE, RI }, { RJ_BASE, RJ_SIZE, RJ }, { RL_BASE, RL_SIZE, RL }, { RM_BASE, RM_SIZE, RM },
    { RN_BASE, RN_SIZE, RN }, { RO_BASE, RO_SIZE, RO },
};
const unsigned NREG = sizeof(regions) / sizeof(regions[0]);
const PtrGlobal ptrglobals[] = {
    { 0x800AFB10u, &g_FieldActors }, { 0x800B0078u, &g_FieldScriptVMCurActor },
    { 0x800AFB1Cu, &g_FieldSpriteData }, { 0x8005A39Cu, &g_pGameState },
    { 0x8005A414u, &g_PartyDataBuffers[0] }, { 0x8005A418u, &g_PartyDataBuffers[1] },
    { 0x8005A41Cu, &g_PartyDataBuffers[2] },
};
const unsigned NPTR = sizeof(ptrglobals) / sizeof(ptrglobals[0]);

/* 7th argument of func_80076AC0, per call, both sides */
static uint32_t x7_ret[MAXEV], x7_nat[MAXEV];
static unsigned x7_rn, x7_nn;

SPY(FieldScriptVMGetArgument, S_GETARG)
SPY(GameCharacterGetGearID, S_GEAR)
SPY(func_800A0158, S_A0158)
SPY(func_8009E574, S_9E574)
SPY(func_800A0C94, S_A0C94)
SPY(func_800A0D3C, S_A0D3C)
SPY(func_8008CF3C, S_8CF3C)
SPY(FieldCharacterIdToPartyId, S_TOPARTY)
SPY(FieldScriptVMGetVariableValue, S_GETVAR)
SPY(func_8009FA54, S_FA54)
long func_80076AC0(long a, long b, long c, long d, long e, long f, long g) {
    require(x7_nn < MAXEV, "x7 count");
    x7_nat[x7_nn++] = (uint32_t)g;
    uint64_t in[6] = { (uint64_t)a, (uint64_t)b, (uint64_t)c, (uint64_t)d, (uint64_t)e, (uint64_t)f };
    return spy_core(S_76AC0, in);
}

const Target targets[] = {
    { 0x800ACDEC, S_GETARG, 1, 1, 0, 0 },
    { 0x8001ACF0, S_GEAR, 1, 1, 0, 0 },
    { 0x800A0158, S_A0158, 4, 0, 0xE, 1 },
    { 0x80076AC0, S_76AC0, 6, 1, 0, 0 },
    { 0x8009E574, S_9E574, 2, 0, 0, 0 },
    { 0x800A0C94, S_A0C94, 0, 0, 0, 0 },
    { 0x800A0D3C, S_A0D3C, 0, 0, 0, 0 },
    { 0x8008CF3C, S_8CF3C, 1, 1, 0, 0 },
    { 0x8009FA00, S_TOPARTY, 1, 1, 0, 0 },
    { 0x800A3018, S_GETVAR, 1, 1, 0, 0 },
    { 0x8009FA54, S_FA54, 1, 0, 0, 0 },
};
const unsigned NTARGETS = sizeof(targets) / sizeof(targets[0]);

static int32_t feed_map(uint32_t id, int32_t raw) {
    if (id == S_TOPARTY) return (int32_t)((uint32_t)raw & 3u) - 1; /* -1..2 */
    if (id == S_76AC0) {   /* void callee: v0 is dead after the call in both bodies */
        if (active_cpu) { require(x7_rn < MAXEV, "x7 count"); x7_ret[x7_rn++] = r32(active_cpu->gpr[29] + 24); }
        return 0;
    }
    return raw;
}
static void native_pre(void) { x7_nn = 0; }
static void native_post(void) {
    require(x7_nn == x7_rn, "func_80076AC0 7th-arg count");
    require(memcmp(x7_nat, x7_ret, 4 * x7_nn) == 0, "func_80076AC0 7th argument");
}

extern void func_800A0228(void);
extern void func_800A08B8(void);
static uint32_t n_0228(Args a) { (void)a; func_800A0228(); return 0; }
static uint32_t n_08B8(Args a) { (void)a; func_800A08B8(); return 0; }

static void setup_ptrs(void) {
    x7_rn = 0;
    w32(0x800AFB10u, FACTORS); w32(0x800B0078u, CURACT); w32(0x800AFB1Cu, SKIN); w32(0x8005A39Cu, GSTATE);
    w32(0x800AFD1C, rnd_range(NACT));
    w16(0x800AFB54, (uint16_t)small(0, 5));
    w16(CURACT + 0x10, (uint16_t)small(-2, 6));
    for (unsigned i = 0; i < 3; i++) w32(0x8006F990 + 4 * i, rnd_range(NACT));
    for (unsigned i = 0; i < 3; i++) w8(GSTATE + 0x22B1 + i, (rnd() & 1) ? 0 : (uint8_t)rnd());
}

static void g_0228(Args* a) {
    (void)a; setup_ptrs();
    feed[0] = small(0, 5);
    static const int32_t mem[] = { 0xFF, 0, 1, 2, 3, 7, 7, 0x10 };
    for (unsigned i = 0; i < 3; i++) w32(0x80062590 + 4 * i, (uint32_t)mem[rnd_range(8)]);
    if (rnd() % 4 == 0) feed[1] = 0xFF;
    uint32_t map = rnd();
    w32(0x8004F34C, map);
    int32_t x = (int32_t)rnd();
    if (rnd() % 3) x = (int32_t)(((uint32_t)x & ~0x3FFFu) | (map & 0xFFF));
    outfeed[2][0] = x;
}
static void g_08B8(Args* a) {
    (void)a; setup_ptrs();
    feed[1] = small(0, 0x40);
    w32(0x800B2268, (rnd() & 1) ? 0 : (uint32_t)small(1, 3));
    for (unsigned i = 0; i < 0x50; i++) w8(0x800AE294 + i, (uint8_t)small(0, 0x30));
}

Fn fns[] = {
    { "func_800A0228", 0x800A0228, 191, n_0228, g_0228, 4000, 0 },
    { "func_800A08B8", 0x800A08B8, 229, n_08B8, g_08B8, 4000, 0 },
};
const unsigned NFNS = sizeof(fns) / sizeof(fns[0]);
