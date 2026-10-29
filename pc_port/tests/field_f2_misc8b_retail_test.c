/* Retail differential, src/field/main/misc8.c: func_80084A40 (actor floor /
 * collision-layer pass, 676 instructions).  The production TU is linked
 * unmodified; see lib/field_retail_core.h.
 * Coverage limit (retail-inherent): with D_800AFB54 == 1, or an actor state
 * index >= the number of collision layers, retail feeds VectorNormal an
 * uninitialised stack VECTOR; those inputs are not generated (the state index
 * is kept inside the layers func_8007D3D4 filled). */
#define TESTNAME "FIELD F2 MISC8B"
enum { S_492C = 1, S_D3D4, S_80968, S_PRINTF, S_VNORM, S_81C54 };
#include <stdint.h>
static int32_t feed_map(uint32_t id, int32_t raw) {
    if (id == S_D3D4) return ((uint32_t)raw & 7u) == 0 ? raw : 0;
    if (id == S_80968) return ((uint32_t)raw & 3u) == 0 ? raw : (int32_t)((uint32_t)raw & ~0x008000E0u);
    return raw;
}
#define FEED_MAP(id, raw) feed_map(id, raw)
#define BUF_BASE 0x80120000u
#define BUF_SIZE 0x60000u
#include "field_retail_core.h"

#define FACTORS 0x80130000u
#define SPR 0x80140000u
#define TRITAB 0x80150000u
#define ADATA 0x80160000u
#define NACT 8u

#define RA_BASE 0x800ADB00u
#define RA_SIZE 0x10u
#define RB_BASE 0x800ADB90u
#define RB_SIZE 0x10u
#define RC_BASE 0x800AFB10u
#define RC_SIZE 0x50u
#define RD_BASE 0x800B21C0u
#define RD_SIZE 0xC0u
#define RE_BASE 0x800C2680u
#define RE_SIZE 0x10u
REGION(RA); REGION(RB); REGION(RC); REGION(RD); REGION(RE);
ALIAS(D_800ADB00, RA, 0x800ADB00);
ALIAS(D_800ADB98, RB, 0x800ADB98);
ALIAS(D_800AFB24, RC, 0x800AFB24); ALIAS(D_800AFB54, RC, 0x800AFB54);
ALIAS(D_800B21CF, RD, 0x800B21CF); ALIAS(g_PlayerActorIndex, RD, 0x800B226C);
ALIAS(g_FieldSystemMode, RE, 0x800C268C);
uintptr_t g_FieldActors;
char D_8006FC60[4], D_8006FC74[4];

const Region regions[] = {
    { RA_BASE, RA_SIZE, RA }, { RB_BASE, RB_SIZE, RB }, { RC_BASE, RC_SIZE, RC },
    { RD_BASE, RD_SIZE, RD }, { RE_BASE, RE_SIZE, RE },
};
const unsigned NREG = sizeof(regions) / sizeof(regions[0]);
const PtrGlobal ptrglobals[] = { { 0x800AFB10u, &g_FieldActors } };
const unsigned NPTR = sizeof(ptrglobals) / sizeof(ptrglobals[0]);

SPY(func_8008492C, S_492C)
SPY(func_8007D3D4, S_D3D4)
SPY(func_80080968, S_80968)
SPY(VectorNormal, S_VNORM)
SPY(func_80081C54, S_81C54)
long func_800379C8(long a, long b, long c, long d, long e, long f) {
    /* the format string is the host D_8006FC60/74 natively, the guest address in retail */
    uint64_t fmt = a == (long)(uintptr_t)D_8006FC60 ? 0x8006FC60u
                 : a == (long)(uintptr_t)D_8006FC74 ? 0x8006FC74u : (uint64_t)a;
    uint64_t in[6] = { fmt, (uint64_t)b, (uint64_t)c, (uint64_t)d, (uint64_t)e, (uint64_t)f };
    return spy_core(S_PRINTF, in);
}

/* func_8007D3D4(actorData, layer, &low, &normal, &tri, &high): halfwords 2/8/1/2 */
const Target targets[] = {
    { 0x8008492C, S_492C, 1, 1, 0, 0 },
    { 0x8007D3D4, S_D3D4, 6, 1, 0x3C, 0, 0x218200u, 0 },
    { 0x80080968, S_80968, 1, 1, 0, 0 },
    { 0x800379C8, S_PRINTF, 2, 0, 0, 0 },
    { 0x80048D7C, S_VNORM, 2, 0, 0x2, 0, 0x60u, 0x1 },
    { 0x80081C54, S_81C54, 1, 0, 0, 0 },
};
const unsigned NTARGETS = sizeof(targets) / sizeof(targets[0]);

extern int32_t func_80084A40(int32_t, int32_t, void*, uint8_t*, int32_t);
static uint32_t n_4A40(Args a) {
    return (uint32_t)func_80084A40((int32_t)a.a0, (int32_t)a.a1, (void*)(uintptr_t)a.a2,
                                   (uint8_t*)(uintptr_t)a.a3, (int32_t)a.a4);
}

static void g_4A40(Args* a) {
    w32(0x800AFB10u, FACTORS);
    for (unsigned k = 0; k < NACT; k++) {
        w32(FACTORS + k * 0x5C + 0x04, SPR + k * 0x100);
        w32(FACTORS + k * 0x5C + 0x4C, ADATA + k * 0x200);
    }
    unsigned idx = rnd_range(NACT);
    uint32_t ad = ADATA + idx * 0x200, sp = SPR + idx * 0x100;
    a->a0 = idx;
    a->a1 = (uint32_t)((rnd() & 1) ? rnd_val() : small(-0x200, 0x200));
    if (rnd() % 8 == 0) a->a1 = 0x7FFFFFFFu;
    a->a2 = FACTORS + idx * 0x5C;
    a->a3 = ad;
    a->a4 = (rnd() % 3) ? rnd_range(3) : rnd();
    w32(0x800B226C, (rnd() & 1) ? idx : rnd_range(NACT));
    w8(0x800B21CF, (uint8_t)rnd_range(3));
    w32(0x800ADB98, (rnd() % 3) ? 0 : rnd());
    w32(0x800C268C, (rnd() & 1) ? 0 : rnd());
    static const int16_t counts[] = { -1, 0, 2, 3, 4, 4, 3 };
    int16_t n = counts[rnd_range(7)];
    w16(0x800AFB54, (uint16_t)n);
    for (unsigned s = 0; s < 4; s++) w32(0x800AFB24 + 4 * s, TRITAB + s * 0x1000 + rnd_range(16) * 2);
    uint32_t f0 = r32(ad);
    if (rnd() % 8) f0 &= ~0x01010000u;
    if (rnd() & 1) f0 &= ~0x04000000u;
    w32(ad, f0);
    if (rnd() % 8) w32(ad + 4, r32(ad + 4) & ~0x00200000u);
    if (rnd() & 1) w32(ad + 4, r32(ad + 4) & ~7u);
    int16_t states = n >= 2 ? n - 1 : 4;
    w16(ad + 0x10, (uint16_t)rnd_range((uint32_t)states));
    for (unsigned s = 0; s < 4; s++) w16(ad + 8 + 2 * s, (uint16_t)small(-60, 60));
    w16(ad + 0x1A, (uint16_t)small(0, 0x80));
    w32(ad + 0x24, (uint32_t)((rnd() & 1) ? rnd_val() : (uint32_t)small(-0x300, 0x300) << 16));
    if (rnd() & 1) w32(sp + 0x10, 0);
    else if (rnd() & 1) w32(sp + 0x10, (uint32_t)small(-0x8000, 0x8000));
    w32(sp + 0x1C, (uint32_t)((rnd() & 1) ? rnd_val() : small(-0x800, 0x800)));
    if (rnd() & 1) w16(sp + 0x84, r32(ad + 0x24) >> 16);
    else if (rnd() & 1) w16(sp + 0x84, (uint16_t)((r32(ad + 0x24) >> 16) + small(-0x40, 0x40)));
    for (unsigned k = 0; k < MAXEV; k++) {
        int16_t tri = (int16_t)small(-60, 60);   /* halfword 10 = the triangle id */
        memcpy((uint8_t*)outfeed[k] + 20, &tri, 2);
        if (rnd() & 1) { int32_t lo = small(-0x300, 0x300); memcpy(outfeed[k], &lo, 4); }
        if (rnd() & 1) { int32_t hi = small(-0x300, 0x300); memcpy((uint8_t*)outfeed[k] + 22, &hi, 4); }
    }
}

Fn fns[] = {
    { "func_80084A40", 0x80084A40, 676, n_4A40, g_4A40, 6000, 1 },
};
const unsigned NFNS = sizeof(fns) / sizeof(fns[0]);
