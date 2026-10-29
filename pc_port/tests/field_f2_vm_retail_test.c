/* Retail differential, src/field/scripts/virtual_machine.c hand-written arms
 * (batch 1): func_800A2030 func_800A30FC func_800A31E8.
 * The production TU is linked unmodified; see lib/field_retail_core.h.
 * FieldScriptMemoryWriteU16 / FieldScriptWritePartyMemberIDs / func_800A30FC
 * are NOT spied: the real retail and real native bodies run on both sides and
 * their stores are compared byte-for-byte in g_FieldScriptMemory (the native
 * WriteU16 takes a u16, retail passes the full register; the store is what
 * matters).  Port-only: g_PcPortOpcode56Func8009FEE4ConditionCount (host
 * instrumentation counter, no game state) is a host variable outside guest
 * memory. */
#define TESTNAME "FIELD F2 VM"
enum { S_VMRUN = 1, S_BCOFF, S_PDIR, S_CDIR, S_FEE4, S_GETVAR };
#include <stdint.h>
static int32_t feed_map(uint32_t id, int32_t raw) {
    uint32_t r = (uint32_t)raw;
    if (id == S_BCOFF) return (int32_t)(r & 0xFFFFu);
    if (id == S_GETVAR) {
        static const uint8_t sec[] = { 0, 1, 0x3B, 0x3C, 0x3D, 0xFF };
        static const uint8_t min[] = { 0, 1, 0xFE, 0xFF };
        if ((r & 3u) != 0) {
            uint32_t s = sec[(r >> 4) % 6u], m = min[(r >> 8) & 3u];
            return (int32_t)((r & 0xFFFF0000u) | (m << 8) | s);
        }
    }
    return raw;
}
#define FEED_MAP(id, raw) feed_map(id, raw)
#define BUF_BASE 0x80120000u
#define BUF_SIZE 0x60000u
#include "field_retail_core.h"

#define FACTORS 0x80130000u
#define ADATA 0x80140000u
#define GSTATE 0x80150000u
#define NACT 8u

#define R4F_BASE 0x8004F2F0u
#define R4F_SIZE 0x70u
#define R59_BASE 0x80059410u
#define R59_SIZE 0xD0u
#define R5A_BASE 0x8005A390u
#define R5A_SIZE 0xD0u
#define R62_BASE 0x80062580u
#define R62_SIZE 0x20u
#define RDB_BASE 0x800ADB10u
#define RDB_SIZE 0x100u
#define RSC_BASE 0x800AF990u
#define RSC_SIZE 0x140u
#define RFB_BASE 0x800AFB00u
#define RFB_SIZE 0x30u
#define RFC_BASE 0x800AFC40u
#define RFC_SIZE 0x100u
#define RFE_BASE 0x800AFE90u
#define RFE_SIZE 0x10u
#define RFF_BASE 0x800AFFE0u
#define RFF_SIZE 0x10u
#define RB0_BASE 0x800B0070u
#define RB0_SIZE 0x10u
#define RB2_BASE 0x800B02C0u
#define RB2_SIZE 0x10u
#define RB6_BASE 0x800B06B0u
#define RB6_SIZE 0x10u
#define R21_BASE 0x800B21C0u
#define R21_SIZE 0xC0u
#define RSM_BASE 0x800C3A60u
#define RSM_SIZE 0x810u
REGION(R4F); REGION(R59); REGION(R5A); REGION(R62); REGION(RDB); REGION(RSC); REGION(RFB); REGION(RFC);
REGION(RFE); REGION(RFF); REGION(RB0); REGION(RB2); REGION(RB6); REGION(R21); REGION(RSM);
ALIAS(D_8004F2F4, R4F, 0x8004F2F4); ALIAS(D_8004F318, R4F, 0x8004F318); ALIAS(D_8004F324, R4F, 0x8004F324);
ALIAS(D_8004F328, R4F, 0x8004F328); ALIAS(g_GameSceneMapNum, R4F, 0x8004F34C);
ALIAS(D_80059418, R59, 0x80059418); ALIAS(D_8005941C, R59, 0x8005941C); ALIAS(D_80059420, R59, 0x80059420);
ALIAS(D_80059484, R59, 0x80059484); ALIAS(D_800594D0, R59, 0x800594D0);
ALIAS(D_8005A444, R5A, 0x8005A444);
ALIAS(g_GamePartyMembers, R62, 0x80062590);
ALIAS(D_800ADB1C, RDB, 0x800ADB1C); ALIAS(D_800ADB68, RDB, 0x800ADB68); ALIAS(D_800ADB74, RDB, 0x800ADB74);
ALIAS(D_800ADBE0, RDB, 0x800ADBE0); ALIAS(D_800ADBE4, RDB, 0x800ADBE4); ALIAS(D_800ADBEC, RDB, 0x800ADBEC);
ALIAS(D_800ADBFC, RDB, 0x800ADBFC);
ALIAS(g_Scene, RSC, 0x800AF990);
ALIAS(D_800AFC6C, RFC, 0x800AFC6C); ALIAS(D_800AFD1C, RFC, 0x800AFD1C);
ALIAS(D_800AFE9C, RFE, 0x800AFE9C);
ALIAS(D_800AFFEC, RFF, 0x800AFFEC);
ALIAS(D_800B02C8, RB2, 0x800B02C8);
ALIAS(D_800B21CC, R21, 0x800B21CC); ALIAS(g_PlayerActorIndex, R21, 0x800B226C);
ALIAS(g_FieldScriptMemory, RSM, 0x800C3A68); ALIAS(D_800C4268, RSM, 0x800C4268);
uintptr_t g_FieldActors, g_FieldScriptVMCurActor, D_800B06B8, g_pGameState;
uint32_t g_PcPortOpcode56Func8009FEE4ConditionCount;

const Region regions[] = {
    { R4F_BASE, R4F_SIZE, R4F }, { R59_BASE, R59_SIZE, R59 }, { R5A_BASE, R5A_SIZE, R5A }, { R62_BASE, R62_SIZE, R62 },
    { RDB_BASE, RDB_SIZE, RDB }, { RSC_BASE, RSC_SIZE, RSC }, { RFB_BASE, RFB_SIZE, RFB }, { RFC_BASE, RFC_SIZE, RFC },
    { RFE_BASE, RFE_SIZE, RFE }, { RFF_BASE, RFF_SIZE, RFF }, { RB0_BASE, RB0_SIZE, RB0 }, { RB2_BASE, RB2_SIZE, RB2 },
    { RB6_BASE, RB6_SIZE, RB6 }, { R21_BASE, R21_SIZE, R21 }, { RSM_BASE, RSM_SIZE, RSM },
};
const unsigned NREG = sizeof(regions) / sizeof(regions[0]);
const PtrGlobal ptrglobals[] = {
    { 0x800AFB10u, &g_FieldActors }, { 0x800B0078u, &g_FieldScriptVMCurActor },
    { 0x800B06B8u, &D_800B06B8 }, { 0x8005A39Cu, &g_pGameState },
};
const unsigned NPTR = sizeof(ptrglobals) / sizeof(ptrglobals[0]);

SPY(FieldScriptVMRun, S_VMRUN)
SPY(FieldScriptGetBytecodeOffset, S_BCOFF)
SPY(FieldGetPlayerActorDirection, S_PDIR)
SPY(FieldGetCameraDirection, S_CDIR)
SPY(func_8009FEE4, S_FEE4)
SPY(FieldScriptVMGetVariableValue, S_GETVAR)

const Target targets[] = {
    { 0x800A1EC8, S_VMRUN, 1, 0, 0, 0 }, { 0x800A3090, S_BCOFF, 2, 1, 0, 0 },
    { 0x8009744C, S_PDIR, 0, 1, 0, 0 }, { 0x8009A514, S_CDIR, 0, 1, 0, 0 },
    { 0x8009FEE4, S_FEE4, 1, 0, 0, 0 }, { 0x800A3018, S_GETVAR, 1, 1, 0, 0 },
};
const unsigned NTARGETS = sizeof(targets) / sizeof(targets[0]);

extern void func_800A2030(void);
extern void func_800A30FC(void);
extern void func_800A31E8(void);
static uint32_t n_2030(Args a) { (void)a; func_800A2030(); return 0; }
static uint32_t n_30FC(Args a) { (void)a; func_800A30FC(); return 0; }
static uint32_t n_31E8(Args a) { (void)a; func_800A31E8(); return 0; }

static void setup_actors(void) {
    w32(0x800AFB10u, FACTORS);
    for (unsigned k = 0; k < NACT; k++) w32(FACTORS + k * 0x5C + 0x4C, ADATA + k * 0x200);
    w32(0x8005A39Cu, GSTATE);
}

static void g_2030(Args* a) {
    (void)a; setup_actors();
    w32(0x800ADB74, (rnd() % 4 == 0) ? 1 : (uint32_t)rnd_val());
    w32(0x800ADBFC, (rnd() % 8 == 0) ? (uint32_t)rnd_val() % 3u - 1u : (uint32_t)small(-1, NACT));
    if ((int32_t)r32(0x800ADBFC) > (int32_t)NACT) w32(0x800ADBFC, NACT);
    w32(0x800ADB1C, (rnd() % 2) ? 0 : rnd());
    w32(0x800ADBE0, (rnd() % 6) ? 1 + rnd_range(5) : 0);
    w32(0x800ADBE4, (rnd() % 6) ? 1 + rnd_range(5) : 0);
    w32(0x800ADBEC, (rnd() % 6) ? 1 + rnd_range(5) : 0);
    w8(0x800B21CC, (rnd() % 2) ? 0 : (uint8_t)rnd());
    for (unsigned i = 0; i < 3; i++)
        w32(0x8005A444 + 4 * i, (rnd() % 3 == 0) ? 0xFF : (rnd() % 8 == 0) ? (uint32_t)rnd_val() : rnd_range(NACT));
    for (unsigned k = 0; k < NACT; k++) {
        uint32_t fa = FACTORS + k * 0x5C, ad = ADATA + k * 0x200;
        w16(fa + 0x58, (uint16_t)((rnd() % 4 == 0) ? (rnd() & ~0xF00u) : rnd()));
        w32(ad + 0x4, (rnd() % 4 == 0) ? (r32(ad + 4) | 0x100000u) : (r32(ad + 4) & ~0x100000u));
        if (rnd() & 1) w32(ad, r32(ad) & ~1u);
        unsigned allF = rnd() % 4 == 0;
        for (unsigned s = 0; s < 8; s++) {
            uint32_t v = rnd();
            if (allF) v |= 0xFu << 18;
            else if (rnd() % 3 == 0) v = (v & ~(0xFu << 18)) | (rnd_range(16) << 18);
            w32(ad + 0x90 + 8 * s, v);
        }
    }
}
static void g_30FC(Args* a) {
    (void)a; setup_actors();
}
static void g_31E8(Args* a) {
    (void)a; setup_actors();
    w8(0x800B02C8, (rnd() % 6 == 0) ? 1 : (uint8_t)rnd());
    w32(0x8004F318, (rnd() % 3) ? (uint32_t)small(0x1C, 0x22) : (uint32_t)rnd_val());
    w32(0x800B226C, rnd_range(NACT));
    for (unsigned i = 0; i < 3; i++) w8(GSTATE + 0x22B1 + i, (rnd() & 1) ? 1 : (uint8_t)rnd());
}

Fn fns[] = {
    { "func_800A2030", 0x800A2030, 159, n_2030, g_2030, 2000, 0 },
    { "func_800A30FC", 0x800A30FC, 59, n_30FC, g_30FC, 400, 0 },
    { "func_800A31E8", 0x800A31E8, 163, n_31E8, g_31E8, 2000, 0 },
};
const unsigned NFNS = sizeof(fns) / sizeof(fns[0]);
