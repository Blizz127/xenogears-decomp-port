/* Retail differential, src/field/main/misc5.c: func_800A5C40 (field
 * transition orchestrator: teardown, save-image relocation, per-mode fade,
 * FieldLoad).  The production TU is linked unmodified except that its
 * memcpy calls are routed (conf PROTOS #define) to f2_memcpy, a spy that
 * records the call like retail's `jal memcpy` and performs the copy; the
 * retail side copies in FEED_MAP.  Side-effect models (same on both sides):
 * FieldLoad may clobber D_800B0048/D_800AFD14 (the function saves and
 * restores them around it); ArchiveDataSync returns busy until a fed 0;
 * func_80078B5C may zero D_800AFD14 (retail non-trapping div, lo = -1).
 * MoveImage's RECT is recorded (x,y / w,h; the following stack word is
 * not part of the argument and is masked on both sides). */
#define TESTNAME "FIELD F2 MISC5C"
enum { S_FONTFREE = 1, S_PFREE, S_864F0, S_7FFE8, S_A915C, S_A4748, S_DRAWSYNC, S_SWAPOT, S_RSYNC, S_FFREE,
       S_HEAPALLOC, S_MEMCPY, S_UNPIN, S_HEAPFREE, S_A90B4, S_PIN, S_FWHITE, S_FADEDRAW, S_DISPLAY, S_ZINIT,
       S_ZUPD, S_A5710, S_SKINSYNC, S_STREAMSYNC, S_FLOAD, S_70488, S_70508, S_85B20, S_A56A8, S_A5884,
       S_ADSYNC, S_78C5C, S_FBLACK, S_77DAC, S_7554C, S_78B5C, S_A5600, S_MOVEIMAGE, S_A91F0, S_77544, S_CONSOL };
#include <stdint.h>
#include <stddef.h>
static int32_t feed_map(uint32_t id, int32_t raw);
#define FEED_MAP(id, raw) feed_map(id, raw)
#define MAXEV 512
#define BUF_BASE 0x80120000u
#define BUF_SIZE 0x20000u
#include "field_retail_core.h"

#define CTX 0x80120000u
#define SAVEIMG 0x80130000u
#define HEAPB 0x80138000u

#define R4F_BASE 0x8004F300u
#define R4F_SIZE 0x30u
#define R5A_BASE 0x8005A4C0u
#define R5A_SIZE 0x4u
#define RDB_BASE 0x800ADB00u
#define RDB_SIZE 0x70u
#define RFD_BASE 0x800AFD00u
#define RFD_SIZE 0x18u
#define RB0_BASE 0x800B0048u
#define RB0_SIZE 0x4u
#define RC2_BASE 0x800C2684u
#define RC2_SIZE 0x4u
#define RP_BASE 0x8005A4E0u   /* D_8005A4E0 (host void*) mirrored */
#define RP_SIZE 0x4u
#define RQ_BASE 0x800ADC14u   /* D_800ADC14 (host void*) mirrored */
#define RQ_SIZE 0x4u
REGION(R4F); REGION(R5A); REGION(RDB); REGION(RFD); REGION(RB0); REGION(RC2); REGION(RP); REGION(RQ);
ALIAS(D_8004F308, R4F, 0x8004F308); ALIAS(D_8004F324, R4F, 0x8004F324);
ALIAS(D_8005A4C0, R5A, 0x8005A4C0);
ALIAS(g_FieldCurRenderContextIndex, RDB, 0x800ADB08); ALIAS(D_800ADB60, RDB, 0x800ADB60);
ALIAS(D_800AFD04, RFD, 0x800AFD04); ALIAS(D_800AFD14, RFD, 0x800AFD14);
ALIAS(D_800B0048, RB0, 0x800B0048);
ALIAS(D_800C2684, RC2, 0x800C2684);
uintptr_t D_8005A4E0, D_800ADC14, g_FieldCurRenderContext;
const Region regions[] = {
    { R4F_BASE, R4F_SIZE, R4F }, { R5A_BASE, R5A_SIZE, R5A }, { RDB_BASE, RDB_SIZE, RDB }, { RFD_BASE, RFD_SIZE, RFD },
    { RB0_BASE, RB0_SIZE, RB0 }, { RC2_BASE, RC2_SIZE, RC2 }, { RP_BASE, RP_SIZE, RP }, { RQ_BASE, RQ_SIZE, RQ },
};
const unsigned NREG = sizeof(regions) / sizeof(regions[0]);
const PtrGlobal ptrglobals[] = { { 0x8005A4E0u, &D_8005A4E0 }, { 0x800ADC14u, &D_800ADC14 },
                                 { 0x800C426Cu, &g_FieldCurRenderContext } };
const unsigned NPTR = sizeof(ptrglobals) / sizeof(ptrglobals[0]);

SPY(FontFree, S_FONTFREE) SPY(FieldParticlesFreeAll, S_PFREE) SPY(func_800864F0, S_864F0)
SPY(func_8007FFE8, S_7FFE8) SPY(func_800A915C, S_A915C) SPY(func_800A4748, S_A4748) SPY(DrawSync, S_DRAWSYNC)
SPY(FieldClearAndSwapOTag, S_SWAPOT) SPY(FieldRenderSync, S_RSYNC) SPY(FieldFree, S_FFREE)
SPY(HeapUnpinBlock, S_UNPIN) SPY(HeapFree, S_HEAPFREE) SPY(func_800A90B4, S_A90B4) SPY(HeapPinBlock, S_PIN)
SPY(FieldFadeToWhite, S_FWHITE) SPY(FieldFadeUpdateAndDraw, S_FADEDRAW) SPY(FieldDisplay, S_DISPLAY)
SPY(FieldZoomFadeEffectInitialize, S_ZINIT) SPY(FieldZoomFadeEffectUpdate, S_ZUPD) SPY(func_800A5710, S_A5710)
SPY(GamePartySyncSkinData, S_SKINSYNC) SPY(GamePartySyncStreamedData, S_STREAMSYNC) SPY(FieldLoad, S_FLOAD)
SPY(func_80070488, S_70488) SPY(func_80070508, S_70508) SPY(func_80085B20, S_85B20) SPY(func_800A56A8, S_A56A8)
SPY(func_800A5884, S_A5884) SPY(ArchiveDataSync, S_ADSYNC) SPY(func_80078C5C, S_78C5C)
SPY(FieldFadeToBlack, S_FBLACK) SPY(func_80077DAC, S_77DAC) SPY(func_8007554C, S_7554C)
SPY(func_80078B5C, S_78B5C) SPY(func_800A5600, S_A5600) SPY(func_800A91F0, S_A91F0) SPY(func_80077544, S_77544)
SPY(HeapConsolidate, S_CONSOL)
long HeapAlloc(long a, long b, long c, long d, long e, long f) {
    uint64_t in[6] = { (uint64_t)a, (uint64_t)b, (uint64_t)c, (uint64_t)d, (uint64_t)e, (uint64_t)f };
    return (long)(uint32_t)spy_core(S_HEAPALLOC, in);
}
void* f2_memcpy(void* d, const void* s, size_t n) {
    uint64_t in[6] = { (uint64_t)(uintptr_t)d, (uint64_t)(uintptr_t)s, (uint64_t)n, 0, 0, 0 };
    spy_core(S_MEMCPY, in);
    memmove(d, s, n);
    return d;
}
long MoveImage(long a, long b, long c, long d, long e, long f) {
    uint64_t in[6] = { (uint64_t)a, (uint64_t)b, (uint64_t)c, (uint64_t)d, (uint64_t)e, (uint64_t)f };
    long r = spy_core(S_MOVEIMAGE, in);
    trace.ev[trace.n - 1].a[5] = 0;
    return r;
}
const Target targets[] = {
    { 0x8003748C, S_FONTFREE, 0, 0, 0, 0 }, { 0x800A9460, S_PFREE, 0, 0, 0, 0 }, { 0x800864F0, S_864F0, 0, 0, 0, 0 },
    { 0x8007FFE8, S_7FFE8, 0, 0, 0, 0 }, { 0x800A915C, S_A915C, 0, 0, 0, 0 }, { 0x800A4748, S_A4748, 0, 0, 0, 0 },
    { 0x800445D0, S_DRAWSYNC, 1, 0, 0, 0 }, { 0x80073FE0, S_SWAPOT, 0, 0, 0, 0 }, { 0x800775F8, S_RSYNC, 0, 0, 0, 0 },
    { 0x800700B0, S_FFREE, 0, 0, 0, 0 }, { 0x80031BDC, S_HEAPALLOC, 2, 1, 0, 0 },
    { 0x8003F968, S_MEMCPY, 3, 1, 0, 0 },   /* ret=1: FEED_MAP performs the guest copy, returns dst */
    { 0x800320B8, S_UNPIN, 1, 0, 0, 0 }, { 0x800320E8, S_HEAPFREE, 1, 0, 0, 0 }, { 0x800A90B4, S_A90B4, 1, 0, 0, 0 },
    { 0x800320A4, S_PIN, 1, 0, 0, 0 }, { 0x80071DCC, S_FWHITE, 1, 0, 0, 0 }, { 0x80071CB4, S_FADEDRAW, 2, 0, 0, 0 },
    { 0x800A6924, S_DISPLAY, 0, 0, 0, 0 }, { 0x800A663C, S_ZINIT, 2, 0, 0, 0 }, { 0x800A6408, S_ZUPD, 0, 0, 0, 0 },
    { 0x800A5710, S_A5710, 1, 0, 0, 0 }, { 0x8001B044, S_SKINSYNC, 0, 0, 0, 0 }, { 0x8001B3A8, S_STREAMSYNC, 0, 0, 0, 0 },
    { 0x80070CC8, S_FLOAD, 0, 1, 0, 0 },   /* ret=1: side effect model */
    { 0x80070488, S_70488, 0, 0, 0, 0 }, { 0x80070508, S_70508, 0, 0, 0, 0 }, { 0x80085B20, S_85B20, 2, 0, 0, 0 },
    { 0x800A56A8, S_A56A8, 1, 0, 0, 0 }, { 0x800A5884, S_A5884, 2, 0, 0, 0 }, { 0x800286CC, S_ADSYNC, 0, 1, 0, 0 },
    { 0x80078C5C, S_78C5C, 0, 0, 0, 0 }, { 0x80071E58, S_FBLACK, 1, 0, 0, 0 }, { 0x80077DAC, S_77DAC, 0, 0, 0, 0 },
    { 0x8007554C, S_7554C, 0, 0, 0, 0 }, { 0x80078B5C, S_78B5C, 0, 1, 0, 0 }, { 0x800A5600, S_A5600, 1, 0, 0, 0 },
    { 0x8004495C, S_MOVEIMAGE, 3, 1, 0, 0, 0, 0x1 }, { 0x800A91F0, S_A91F0, 0, 0, 0, 0 },
    { 0x80077544, S_77544, 0, 0, 0, 0 }, { 0x80031FF8, S_CONSOL, 0, 0, 0, 0 },
};
const unsigned NTARGETS = sizeof(targets) / sizeof(targets[0]);
static void set32(uint32_t addr, uint32_t v) {
    if (active_cpu) { w32(addr, v); return; }
    for (unsigned r = 0; r < NREG; r++)
        if (addr >= regions[r].base && addr + 4 <= regions[r].base + regions[r].size) { memcpy(regions[r].nat + (addr - regions[r].base), &v, 4); return; }
}
static int32_t feed_map(uint32_t id, int32_t raw) {
    uint32_t r = (uint32_t)raw;
    switch (id) {
    case S_HEAPALLOC: return (int32_t)(HEAPB + ((r & 7u) << 12));
    case S_MEMCPY:
        if (active_cpu) {
            uint32_t d = active_cpu->gpr[4], s = active_cpu->gpr[5], n = active_cpu->gpr[6];
            memmove(gp(d), gp(s), n);
            return (int32_t)d;
        }
        return 0;
    case S_FLOAD:
        if (r & 1u) { set32(0x800B0048, r % 9u); set32(0x800AFD14, (r >> 8) & 0xFFFFu); }
        return 0;
    case S_ADSYNC: return (r % 3u) ? (int32_t)(r | 1u) : 0;
    case S_78B5C:
        if ((r & 0x1F0000u) == 0) set32(0x800AFD14, 0);
        return 0;
    case S_MOVEIMAGE:
        if (active_cpu) trace.ev[trace.n - 1].a[5] = 0;
        return 0;
    }
    return raw;
}

extern void func_800A5C40(void);
static uint32_t n_5C40(Args a) { (void)a; func_800A5C40(); return 0; }
static void g_5C40(Args* a) {
    (void)a;
    w32(0x800C426Cu, CTX);
    w32(0x8005A4E0u, SAVEIMG + 4 * rnd_range(16));
    w32(0x800ADC14u, HEAPB + 0x10000 + 0x100 * rnd_range(16));
    w32(0x8005A4C0, rnd_range(0x400));
    static const uint32_t modes[] = { 0, 1, 2, 3, 4, 5, 6, 7, 0xFFFFFFFFu };
    w32(0x800B0048, modes[rnd_range(9)]);
    w32(0x800AFD14, (uint32_t)small(-1, 8));
    w32(0x8004F308, (rnd() & 1) ? 0xFFFFFFFFu : rnd());
    w32(0x800ADB60, (rnd() & 1) ? 1 : rnd_range(3));
    w32(0x800C2684, (uint32_t)((rnd() & 1) ? small(0x2200, 0x2300) : (int32_t)rnd()));
}
Fn fns[] = { { "func_800A5C40", 0x800A5C40, 498, n_5C40, g_5C40, 3000, 0 } };
const unsigned NFNS = sizeof(fns) / sizeof(fns[0]);
