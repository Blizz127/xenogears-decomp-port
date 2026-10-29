/* Retail differential, src/field/scripts/virtual_machine.c: func_800A24C4
 * (party-skin per-frame update + scroll drift).  The production TU is linked
 * unmodified; see lib/field_retail_core.h.  The D_800B2174 globals follow the
 * PC block (g_FieldBss_800B20A8 = retail 0x800B2078..0x800B249C), so the slot
 * table at D_800B2264-0x80 is the same bytes on both sides. */
#define TESTNAME "FIELD F2 VM3"
enum { S_22AC = 1, S_D898, S_E8330 };
#include <stdint.h>
#define MAXEV 128
#define BUF_BASE 0x80120000u
#define BUF_SIZE 0x20000u
#include "field_retail_core.h"

#define FACTORS 0x80120000u
#define ADATA 0x80130000u
#define NACT 24u

#define RA_BASE 0x8004F300u
#define RA_SIZE 0x10u
#define RD_BASE 0x800ADBF0u
#define RD_SIZE 0x10u
#define RN_BASE 0x800AFB0Cu
#define RN_SIZE 0x4u
#define RB_BASE 0x800B2078u
#define RB_SIZE 0x424u
REGION(RA); REGION(RD); REGION(RN); REGION(RB);
ALIAS(g_GamePartySkinsInitialized, RA, 0x8004F30C);
ALIAS(D_800ADBFC, RD, 0x800ADBFC);
ALIAS(g_FieldNumActors, RN, 0x800AFB0C);
ALIAS(D_800B21BC, RB, 0x800B21BC); ALIAS(D_800B21C0, RB, 0x800B21C0); ALIAS(D_800B21C4, RB, 0x800B21C4);
ALIAS(D_800B21D2, RB, 0x800B21D2); ALIAS(D_800B2264, RB, 0x800B2264); ALIAS(g_PlayerActorIndex, RB, 0x800B226C);
uintptr_t g_FieldActors;
const Region regions[] = { { RA_BASE, RA_SIZE, RA }, { RD_BASE, RD_SIZE, RD }, { RN_BASE, RN_SIZE, RN }, { RB_BASE, RB_SIZE, RB } };
const unsigned NREG = sizeof(regions) / sizeof(regions[0]);
const PtrGlobal ptrglobals[] = { { 0x800AFB10u, &g_FieldActors } };
const unsigned NPTR = sizeof(ptrglobals) / sizeof(ptrglobals[0]);

SPY(func_800A22AC, S_22AC)
SPY(func_800AD898, S_D898)
SPY(func_801E8330, S_E8330)
const Target targets[] = {
    { 0x800A22AC, S_22AC, 1, 0, 0, 0 },
    { 0x800AD898, S_D898, 0, 0, 0, 0 },
    { 0x801E8330, S_E8330, 3, 0, 0, 0 },
};
const unsigned NTARGETS = sizeof(targets) / sizeof(targets[0]);

extern void func_800A24C4(void);
static uint32_t n_24C4(Args a) { (void)a; func_800A24C4(); return 0; }
static void g_24C4(Args* a) {
    (void)a;
    w32(0x800AFB10u, FACTORS);
    for (unsigned k = 0; k < NACT; k++) {
        w32(FACTORS + k * 0x5C + 0x4C, ADATA + k * 0x200);
        if (rnd() & 1) w32(ADATA + k * 0x200 + 0x12C, r32(ADATA + k * 0x200 + 0x12C) & ~3u);
    }
    w32(0x8004F30C, (rnd() % 6) ? 1 : 0);
    w32(0x800B2264, (uint32_t)small(-1, 40));
    w32(0x800B226C, rnd_range(NACT));
    if (rnd() & 1) w8(ADATA + r32(0x800B226C) * 0x200 + 0x74, 0xFF);
    int32_t n = small(-1, (int32_t)NACT);
    w32(0x800AFB0C, (uint32_t)n);
    w32(0x800ADBFC, (uint32_t)small(-1, n < 0 ? 0 : n));
    static const uint8_t modes[] = { 0, 1, 2, 0x80, 0x81, 0x7F };
    if (rnd() % 4) w8(0x800B21D2, modes[rnd_range(6)]);
}
Fn fns[] = { { "func_800A24C4", 0x800A24C4, 148, n_24C4, g_24C4, 4000, 0 } };
const unsigned NFNS = sizeof(fns) / sizeof(fns[0]);
