/* Retail differential, src/field/party/stats.c hand-written arm:
 * func_80097108 (restore MP of the masked party members).  The production TU
 * is linked unmodified (FieldPartyMemberIncreaseMp, same TU, is spied through
 * WEAKEN); see lib/field_retail_core.h. */
#define TESTNAME "FIELD F2 STATS"
enum { S_ARG1 = 1, S_INCMP };
#include <stdint.h>
#define BUF_BASE 0x80120000u
#define BUF_SIZE 0x40000u
#include "field_retail_core.h"

#define SCRIPT 0x80120000u
#define CURACT 0x80130000u

#define RA_BASE 0x800B0070u
#define RA_SIZE 0x10u
#define RB_BASE 0x800AEA20u
#define RB_SIZE 0x20u
#define RC_BASE 0x80062580u
#define RC_SIZE 0x20u
REGION(RA); REGION(RB); REGION(RC);
ALIAS(g_FieldNumPartyMembersMasks, RB, 0x800AEA2C);
ALIAS(g_GamePartyMembers, RC, 0x80062590);
uintptr_t g_FieldScriptVMCurActor, g_FieldScriptVMCurScriptData;
const Region regions[] = { { RA_BASE, RA_SIZE, RA }, { RB_BASE, RB_SIZE, RB }, { RC_BASE, RC_SIZE, RC } };
const unsigned NREG = sizeof(regions) / sizeof(regions[0]);
const PtrGlobal ptrglobals[] = { { 0x800B0078u, &g_FieldScriptVMCurActor }, { 0x800ADC00u, &g_FieldScriptVMCurScriptData } };
const unsigned NPTR = sizeof(ptrglobals) / sizeof(ptrglobals[0]);

SPY(FieldScriptArgument1, S_ARG1)
SPY(FieldPartyMemberIncreaseMp, S_INCMP)
const Target targets[] = { { 0x8009CF78, S_ARG1, 2, 1, 0, 0 }, { 0x80096920, S_INCMP, 2, 0, 0, 0 } };
const unsigned NTARGETS = sizeof(targets) / sizeof(targets[0]);

extern void func_80097108(void);
static uint32_t n_7108(Args a) { (void)a; func_80097108(); return 0; }
static void g_7108(Args* a) {
    (void)a;
    w32(0x800B0078u, CURACT);
    w32(0x800ADC00u, SCRIPT);
    uint16_t ip = (uint16_t)((rnd() & 3) ? rnd_range(0x8000) : rnd());
    w16(CURACT + 0xCC, ip);
    if (rnd() & 1) w8(SCRIPT + (uint16_t)(ip + 3), (uint8_t)rnd_range(4));
    if (rnd() & 1) feed[0] = small(-100, 100);
    for (unsigned i = 0; i < 3; i++) {
        unsigned m = rnd_range(4);
        if (m == 0) w32(0x80062590 + 4 * i, 0xFF);
        else if (m == 1) w32(0x80062590 + 4 * i, rnd_range(11));
    }
    for (unsigned i = 0; i < 4; i++) if (rnd() & 1) w16(0x800AEA2C + 2 * i, (uint16_t)rnd_range(8));
}
Fn fns[] = { { "func_80097108", 0x80097108, 62, n_7108, g_7108, 3000, 0 } };
const unsigned NFNS = sizeof(fns) / sizeof(fns[0]);
