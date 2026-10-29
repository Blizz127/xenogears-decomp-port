/* Retail differential, src/field/main/misc5.c: func_800A5924 (field
 * transition fade orchestrator).  The production TU is linked unmodified;
 * see lib/field_retail_core.h.  Every callee is spied.  Side effect model
 * (identical on both sides, applied through FEED_MAP): each func_80078B5C
 * call may rewrite D_800ADB38 (ending the mode-3 spin / re-dispatching) and
 * occasionally zero D_800ADB3C, which exercises retail's non-trapping div
 * (lo = -1) in the fade loops. */
#define TESTNAME "FIELD F2 MISC5B"
enum { S_FONTFREE = 1, S_70C84, S_A915C, S_A4748, S_DRAWSYNC, S_SWAPOT, S_RSYNC, S_ZINIT, S_A5774, S_FWHITE,
       S_FBLACK, S_A5600, S_77DAC, S_ZUPD, S_7554C, S_78B5C, S_A5884, S_A6E70, S_A6C40, S_A7064, S_A91F0, S_77544 };
#include <stdint.h>
static int32_t feed_map(uint32_t id, int32_t raw);
#define FEED_MAP(id, raw) feed_map(id, raw)
#define MAXEV 512
#define BUF_BASE 0x80120000u
#define BUF_SIZE 0x10000u
#include "field_retail_core.h"

#define RA_BASE 0x800ADB30u
#define RA_SIZE 0x10u
#define RB_BASE 0x800ADC00u
#define RB_SIZE 0x10u
#define RC_BASE 0x800C3A40u
#define RC_SIZE 0x4u
REGION(RA); REGION(RB); REGION(RC);
ALIAS(D_800ADB38, RA, 0x800ADB38); ALIAS(D_800ADB3C, RA, 0x800ADB3C);
ALIAS(D_800ADC08, RB, 0x800ADC08);
ALIAS(D_800C3A40, RC, 0x800C3A40);
const Region regions[] = { { RA_BASE, RA_SIZE, RA }, { RB_BASE, RB_SIZE, RB }, { RC_BASE, RC_SIZE, RC } };
const unsigned NREG = sizeof(regions) / sizeof(regions[0]);
const PtrGlobal ptrglobals[] = { { 0, NULL } };
const unsigned NPTR = 0;

SPY(FontFree, S_FONTFREE) SPY(func_80070C84, S_70C84) SPY(func_800A915C, S_A915C) SPY(func_800A4748, S_A4748)
SPY(DrawSync, S_DRAWSYNC) SPY(FieldClearAndSwapOTag, S_SWAPOT) SPY(FieldRenderSync, S_RSYNC)
SPY(FieldZoomFadeEffectInitialize, S_ZINIT) SPY(func_800A5774, S_A5774) SPY(FieldFadeToWhite, S_FWHITE)
SPY(FieldFadeToBlack, S_FBLACK) SPY(func_800A5600, S_A5600) SPY(func_80077DAC, S_77DAC)
SPY(FieldZoomFadeEffectUpdate, S_ZUPD) SPY(func_8007554C, S_7554C) SPY(func_80078B5C, S_78B5C)
SPY(func_800A5884, S_A5884) SPY(func_800A6E70, S_A6E70) SPY(func_800A6C40, S_A6C40) SPY(func_800A7064, S_A7064)
SPY(func_800A91F0, S_A91F0) SPY(func_80077544, S_77544)
const Target targets[] = {
    { 0x8003748C, S_FONTFREE, 0, 0, 0, 0 }, { 0x80070C84, S_70C84, 0, 0, 0, 0 }, { 0x800A915C, S_A915C, 0, 0, 0, 0 },
    { 0x800A4748, S_A4748, 0, 0, 0, 0 }, { 0x800445D0, S_DRAWSYNC, 1, 0, 0, 0 }, { 0x80073FE0, S_SWAPOT, 0, 0, 0, 0 },
    { 0x800775F8, S_RSYNC, 0, 0, 0, 0 }, { 0x800A663C, S_ZINIT, 2, 0, 0, 0 }, { 0x800A5774, S_A5774, 3, 0, 0, 0 },
    { 0x80071DCC, S_FWHITE, 1, 0, 0, 0 }, { 0x80071E58, S_FBLACK, 1, 0, 0, 0 }, { 0x800A5600, S_A5600, 1, 0, 0, 0 },
    { 0x80077DAC, S_77DAC, 0, 0, 0, 0 }, { 0x800A6408, S_ZUPD, 0, 0, 0, 0 }, { 0x8007554C, S_7554C, 0, 0, 0, 0 },
    { 0x80078B5C, S_78B5C, 0, 1, 0, 0 },   /* ret=1 only so FEED_MAP applies the side effect; v0 dead */
    { 0x800A5884, S_A5884, 2, 0, 0, 0 }, { 0x800A6E70, S_A6E70, 0, 0, 0, 0 }, { 0x800A6C40, S_A6C40, 0, 0, 0, 0 },
    { 0x800A7064, S_A7064, 0, 0, 0, 0 }, { 0x800A91F0, S_A91F0, 0, 0, 0, 0 }, { 0x80077544, S_77544, 0, 0, 0, 0 },
};
const unsigned NTARGETS = sizeof(targets) / sizeof(targets[0]);
static void set32(uint32_t addr, uint32_t v) {
    if (active_cpu) w32(addr, v); else memcpy(RA + (addr - RA_BASE), &v, 4);
}
static int32_t feed_map(uint32_t id, int32_t raw) {
    uint32_t r = (uint32_t)raw;
    if (id == S_78B5C) {
        if ((r & 3u) == 0) set32(0x800ADB38, (r >> 4) % 6u);   /* new mode 0..5 */
        if ((r & 0x3F0000u) == 0) set32(0x800ADB3C, 0);
        return 0;
    }
    return raw;
}

extern void func_800A5924(void);
static uint32_t n_5924(Args a) { (void)a; func_800A5924(); return 0; }
static void g_5924(Args* a) {
    (void)a;
    w32(0x800ADB38, rnd_range(6));
    w32(0x800ADB3C, (uint32_t)small(-1, 12));
    /* D_800ADB3C (frame count) kept small so the per-frame callee traces fit MAXEV */
}
Fn fns[] = { { "func_800A5924", 0x800A5924, 199, n_5924, g_5924, 4000, 0 } };
const unsigned NFNS = sizeof(fns) / sizeof(fns[0]);
