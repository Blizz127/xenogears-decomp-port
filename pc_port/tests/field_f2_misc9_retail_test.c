/* Retail differential, src/field/main/misc9.c: func_800AA9DC (model bounds
 * centre/radius) and func_800AAA74 (screen-space cull test).  The production
 * TU is linked unmodified; see lib/field_retail_core.h.  The RotTransPers
 * corner input is recorded (vx,vy / vz,pad): retail's pad is untouched stack,
 * zero in this harness, the port writes 0. */
#define TESTNAME "FIELD F2 MISC9"
enum { S_RT = 1, S_SETROT, S_SETTRANS, S_RTP };
#include <stdint.h>
#define BUF_BASE 0x80120000u
#define BUF_SIZE 0x20000u
#include "field_retail_core.h"

#define MODEL 0x80120000u
#define BOUNDS 0x80128000u

#define RA_BASE 0x800B00E0u
#define RA_SIZE 0x30u
#define RB_BASE 0x800C3A50u
#define RB_SIZE 0x20u
REGION(RA); REGION(RB);
ALIAS(D_800B00E8, RA, 0x800B00E8);
ALIAS(D_800C3A5C, RB, 0x800C3A5C); ALIAS(D_800C3A60, RB, 0x800C3A60);
const Region regions[] = { { RA_BASE, RA_SIZE, RA }, { RB_BASE, RB_SIZE, RB } };
const unsigned NREG = sizeof(regions) / sizeof(regions[0]);
const PtrGlobal ptrglobals[] = { { 0, NULL } };
const unsigned NPTR = 0;

static uint64_t h2g(long p) {
    for (unsigned r = 0; r < NREG; r++)
        if ((uintptr_t)p >= (uintptr_t)regions[r].nat && (uintptr_t)p < (uintptr_t)regions[r].nat + regions[r].size)
            return regions[r].base + (uint32_t)((uintptr_t)p - (uintptr_t)regions[r].nat);
    return (uint64_t)p;
}
SPY(RotTrans, S_RT)
SPY(RotTransPers, S_RTP)
long SetRotMatrix(long a, long b, long c, long d, long e, long f) {
    uint64_t in[6] = { h2g(a), (uint64_t)b, (uint64_t)c, (uint64_t)d, (uint64_t)e, (uint64_t)f };
    return spy_core(S_SETROT, in);
}
long SetTransMatrix(long a, long b, long c, long d, long e, long f) {
    uint64_t in[6] = { h2g(a), (uint64_t)b, (uint64_t)c, (uint64_t)d, (uint64_t)e, (uint64_t)f };
    return spy_core(S_SETTRANS, in);
}

/* RotTrans(in, VECTOR* out (3 words), long* flag); RotTransPers(in, &sxy, &p, &flag) */
const Target targets[] = {
    { 0x8004A6DC, S_RT, 3, 0, 0x6, 0, 0x260u, 0 },
    { 0x80049EFC, S_SETROT, 1, 0, 0, 0 },
    { 0x80049F8C, S_SETTRANS, 1, 0, 0, 0 },
    { 0x8004A64C, S_RTP, 4, 0, 0xE, 0, 0x2220u, 0x1 },
};
const unsigned NTARGETS = sizeof(targets) / sizeof(targets[0]);

extern void func_800AA9DC(void*);
extern int32_t func_800AAA74(void*);
static uint32_t n_A9DC(Args a) { func_800AA9DC((void*)(uintptr_t)a.a0); return 0; }
static uint32_t n_AA74(Args a) { return (uint32_t)func_800AAA74((void*)(uintptr_t)a.a0); }

static void g_A9DC(Args* a) {
    uint32_t m = MODEL + 0x40 * rnd_range(64), b = BOUNDS + 0x40 * rnd_range(64);
    a->a0 = m;
    w32(m + 4, b);
    for (unsigned k = 0; k < 6; k++)
        w16(b + 0x20 + 2 * k + (k >= 3 ? 2 : 0), (uint16_t)((rnd() & 1) ? (int16_t)rnd() : small(-0x400, 0x400)));
}
static void g_AA74(Args* a) {
    uint32_t m = MODEL + 0x40 * rnd_range(64);
    a->a0 = m;
    if (rnd() & 1) w16(m + 0x20, (uint16_t)small(-0x200, 0x200));
    w32(0x800C3A5C, (uint32_t)((rnd() & 1) ? rnd_val() : small(-0x200, 0x200)));
    w32(0x800C3A60, (uint32_t)((rnd() & 1) ? rnd_val() : small(-0x200, 0x200)));
    for (unsigned k = 0; k < MAXEV; k++)
        if (rnd() & 1) { outfeed[k][0] = (int32_t)(((uint32_t)small(-0x300, 0x300) << 16) | (uint16_t)small(-0x300, 0x300)); }
}

Fn fns[] = {
    { "func_800AA9DC", 0x800AA9DC, 38, n_A9DC, g_A9DC, 3000, 0 },
    { "func_800AAA74", 0x800AAA74, 89, n_AA74, g_AA74, 4000, 1 },
};
const unsigned NFNS = sizeof(fns) / sizeof(fns[0]);
