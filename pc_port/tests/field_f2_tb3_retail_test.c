/* Retail differential, src/field/dialogue/text_box.c: func_8009C5A8 (open a
 * dialog box: placement, sizing, face portrait).  The production TU is linked
 * unmodified; see lib/field_retail_core.h.  func_8007F8DC's stack arguments
 * 7..11 are captured on both sides and compared in NATIVE_POST.
 * Not generated (retail-inherent, reported): placement type 3 (retail uses
 * an uninitialised stack screenX; the port keeps 0xA0 -> DIVERGES on x) and
 * func_80080760() == 0xFFFF (retail writes g_FieldTextBoxes[0xFFFF]).
 * Port-only: XENO_NPC_EVENT_DUMP getenv/printf (unset here; no state). */
#define TESTNAME "FIELD F2 TB3"
enum { S_8A558 = 1, S_C154, S_CD18, S_GETIARG, S_80720, S_80760, S_807B4, S_DW, S_DH, S_CAMDIR, S_F814,
       S_F8DC, S_CCF8 };
#include <stdint.h>
static int32_t feed_map(uint32_t id, int32_t raw);
#define FEED_MAP(id, raw) feed_map(id, raw)
static void native_pre(void);
static void native_post(void);
#define NATIVE_PRE() native_pre()
#define NATIVE_POST() native_post()
#define BUF_BASE 0x80120000u
#define BUF_SIZE 0x40000u
#include "field_retail_core.h"

#define SCRIPT 0x80120000u
#define FACTORS 0x80130000u
#define ADATA 0x80131000u
#define DIALOG 0x80140000u
#define NACT 8u

#define RA_BASE 0x800ADB20u
#define RA_SIZE 0x60u
#define RF_BASE 0x800AFC60u
#define RF_SIZE 0xC0u
#define RV_BASE 0x800B00C0u
#define RV_SIZE 0x4u
#define RT_BASE 0x800C2688u
#define RT_SIZE 0x1270u
#define RG_BASE 0x800C4260u
#define RG_SIZE 0x10u
REGION(RA); REGION(RF); REGION(RV); REGION(RT); REGION(RG);
ALIAS(D_800ADB2C, RA, 0x800ADB2C); ALIAS(D_800ADB64, RA, 0x800ADB64); ALIAS(D_800ADB70, RA, 0x800ADB70);
ALIAS(g_FieldScriptMaxInstructionCount, RF, 0x800AFC7C); ALIAS(D_800AFD04, RF, 0x800AFD04);
ALIAS(D_800AFD1C, RF, 0x800AFD1C);
ALIAS(D_800B00C0, RV, 0x800B00C0);
ALIAS(g_FieldTextBoxes, RT, 0x800C2698);
ALIAS(D_800C4268, RG, 0x800C4268);
uintptr_t g_FieldActors, g_FieldScriptVMCurActor, g_FieldScriptVMCurScriptData, D_800ADBF0;
uint16_t D_800B2174[4];
const Region regions[] = { { RA_BASE, RA_SIZE, RA }, { RF_BASE, RF_SIZE, RF }, { RV_BASE, RV_SIZE, RV },
                           { RT_BASE, RT_SIZE, RT }, { RG_BASE, RG_SIZE, RG } };
const unsigned NREG = sizeof(regions) / sizeof(regions[0]);
const PtrGlobal ptrglobals[] = { { 0x800AFB10u, &g_FieldActors }, { 0x800B0078u, &g_FieldScriptVMCurActor },
                                 { 0x800ADC00u, &g_FieldScriptVMCurScriptData }, { 0x800ADBF0u, &D_800ADBF0 } };
const unsigned NPTR = sizeof(ptrglobals) / sizeof(ptrglobals[0]);

static uint32_t xr[5 * MAXEV], xn[5 * MAXEV];
static unsigned xr_n, xn_n;
SPY(func_8008A558, S_8A558)
SPY(func_8009C154, S_C154)
long func_8009CD18(long a, long b, long c, long d, long e, long f) {
    /* fed box index mapped to 0..3 on both sides (retail: in FEED_MAP via a0) */
    uint64_t in[6] = { (uint64_t)a, (uint64_t)b, (uint64_t)c, (uint64_t)d, (uint64_t)e, (uint64_t)f };
    long r = spy_core(S_CD18, in);
    *(int32_t*)(uintptr_t)a = (int32_t)((uint32_t)*(int32_t*)(uintptr_t)a % 4u);
    return r;
}
SPY(FieldScriptVMGetInstructionArgument, S_GETIARG)
SPY(func_80080720, S_80720)
SPY(func_80080760, S_80760)
SPY(func_800807B4, S_807B4)
SPY(DialogGetWidth, S_DW)
SPY(DialogGetHeight, S_DH)
SPY(FieldGetCameraDirection, S_CAMDIR)
SPY(func_8007F814, S_F814)
SPY(func_8009CCF8, S_CCF8)
long func_8007F8DC(long a, long b, long c, long d, long e, long f, long g, long h, long i, long j, long k) {
    require(xn_n + 5 <= 5 * MAXEV, "extra");
    xn[xn_n++] = (uint32_t)g; xn[xn_n++] = (uint32_t)h; xn[xn_n++] = (uint32_t)i; xn[xn_n++] = (uint32_t)j;
    xn[xn_n++] = (uint32_t)k;
    uint64_t in[6] = { (uint64_t)a, (uint64_t)b, (uint64_t)c, (uint64_t)d, (uint64_t)e, (uint64_t)f };
    return spy_core(S_F8DC, in);
}
const Target targets[] = {
    { 0x8008A558, S_8A558, 0, 1, 0, 0 },
    { 0x8009C154, S_C154, 1, 1, 0, 0 },
    { 0x8009CD18, S_CD18, 1, 1, 0x1, 1 },
    { 0x800ACDB8, S_GETIARG, 1, 1, 0, 0 },
    { 0x80080720, S_80720, 0, 1, 0, 0 },
    { 0x80080760, S_80760, 0, 1, 0, 0 },
    { 0x800807B4, S_807B4, 0, 1, 0, 0 },
    { 0x8003373C, S_DW, 2, 1, 0, 0 },
    { 0x80033760, S_DH, 2, 1, 0, 0 },
    { 0x8009A514, S_CAMDIR, 0, 1, 0, 0 },
    { 0x8007F814, S_F814, 4, 0, 0x6, 1 },
    { 0x8007F8DC, S_F8DC, 6, 1, 0, 0 },   /* ret=1 only so FEED_MAP captures args 7..11 */
    { 0x8009CCF8, S_CCF8, 1, 0, 0, 0 },
};
const unsigned NTARGETS = sizeof(targets) / sizeof(targets[0]);
static int32_t feed_map(uint32_t id, int32_t raw) {
    uint32_t r = (uint32_t)raw;
    switch (id) {
    case S_8A558: return (r & 3u) ? 0 : raw;
    case S_C154: return (r % 4u) ? 0 : -1;
    case S_CD18:
        if (active_cpu) w32(active_cpu->gpr[4], r32(active_cpu->gpr[4]) % 4u);
        return (r % 5u) ? -1 : 0;
    case S_80720: return (r & 1u) ? 0 : 1;
    case S_80760: return (int32_t)(r % 4u);   /* 0xFFFF (fallthrough with box 0xFFFF) not generated */
    case S_807B4: return (int32_t)(r % 4u);
    case S_DW: case S_DH: return (int32_t)(r & 0xFFu);
    case S_F8DC:
        if (active_cpu) {
            require(xr_n + 5 <= 5 * MAXEV, "extra");
            for (unsigned k = 0; k < 5; k++) xr[xr_n++] = r32(active_cpu->gpr[29] + 0x18 + 4 * k);
        }
        return 0;
    }
    return raw;
}
static void native_pre(void) { xn_n = 0; }
static void native_post(void) {
    require(xn_n == xr_n, "stack-argument count");
    require(memcmp(xn, xr, 4 * xn_n) == 0, "stack arguments 7..11");
}

extern int32_t func_8009C5A8(int32_t, int32_t);
static uint32_t n_C5A8(Args a) { return (uint32_t)func_8009C5A8((int32_t)a.a0, (int32_t)a.a1); }
static void g_C5A8(Args* a) {
    xr_n = 0;
    w32(0x800AFB10u, FACTORS); w32(0x800ADC00u, SCRIPT); w32(0x800ADBF0u, DIALOG);
    unsigned cur = rnd_range(NACT);
    w32(0x800B0078u, ADATA + cur * 0x200);
    for (unsigned k = 0; k < NACT; k++) w32(FACTORS + k * 0x5C + 0x4C, ADATA + k * 0x200);
    a->a0 = rnd_range(NACT);
    static const uint32_t modes[] = { 0, 0, 3, 3, 1, 2, 5 };
    a->a1 = modes[rnd_range(7)];
    if (rnd() % 4) w32(0x800ADB2C, 0);
    if (rnd() % 4) w32(0x800AFD04, 0);
    if (rnd() % 4) w32(0x800C4268, 0);
    if (rnd() % 4) w32(0x800ADB64, 0xFF);
    if (rnd() & 1) w32(0x800ADB70, (rnd() & 1) ? 0 : 1);
    uint32_t ad = ADATA + cur * 0x200;
    if (rnd() & 1) w8(ad + 0x80, 0xFF);
    for (unsigned o = 0x82; o <= 0x83; o++) if (rnd() & 1) w8(ad + o, 0);
    for (unsigned o = 0x88; o <= 0x8A; o += 2) if (rnd() & 1) w16(ad + o, 0);
    /* placement type 3 is not generated (see header) */
    uint32_t df = r32(ad + 0x84);
    if (((df >> 4) & 3u) == 3u) df &= ~0x10u;
    w32(ad + 0x84, df);
    uint16_t ip = (uint16_t)rnd_range(0x8000);
    w16(ad + 0xCC, ip);
    uint8_t b3 = (rnd() & 1) ? 0 : (uint8_t)rnd();
    if (((b3 >> 4) & 3u) == 3u) b3 &= (uint8_t)~0x10u;
    w8(SCRIPT + ip + 3, b3);
    for (unsigned k = 0; k < 4; k++) if (rnd() & 1) w16(0x800C2698u + 0x498u * k + 0x40Eu, 0);
}
Fn fns[] = { { "func_8009C5A8", 0x8009C5A8, 468, n_C5A8, g_C5A8, 8000, 1 } };
const unsigned NFNS = sizeof(fns) / sizeof(fns[0]);
