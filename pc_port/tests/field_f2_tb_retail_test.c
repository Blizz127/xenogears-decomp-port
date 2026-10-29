/* Retail differential, src/field/dialogue/text_box.c: func_8009BC98 (text
 * box wait/scroll opcode).  The production TU is linked unmodified; see
 * lib/field_retail_core.h.  g_FieldTextBoxes follows the PC block
 * g_FieldBss_800C2688 (0x1270 bytes, boxes at +0x10, stride 0x498). */
#define TESTNAME "FIELD F2 TB"
enum { S_CD18 = 1, S_33CD0, S_34800 };
#include <stdint.h>
static int32_t feed_map(uint32_t id, int32_t raw);
#define FEED_MAP(id, raw) feed_map(id, raw)
#define BUF_BASE 0x80120000u
#define BUF_SIZE 0x20000u
#include "field_retail_core.h"

#define SCRIPT 0x80120000u
#define CURACT 0x80130000u

#define RF_BASE 0x800AFC60u
#define RF_SIZE 0x30u
#define RV_BASE 0x800B00C0u
#define RV_SIZE 0x4u
#define RT_BASE 0x800C2688u
#define RT_SIZE 0x1270u
REGION(RF); REGION(RV); REGION(RT);
ALIAS(g_FieldScriptMaxInstructionCount, RF, 0x800AFC7C);
ALIAS(D_800B00C0, RV, 0x800B00C0);
ALIAS(g_FieldTextBoxes, RT, 0x800C2698);
uintptr_t g_FieldScriptVMCurActor, g_FieldScriptVMCurScriptData;
const Region regions[] = { { RF_BASE, RF_SIZE, RF }, { RV_BASE, RV_SIZE, RV }, { RT_BASE, RT_SIZE, RT } };
const unsigned NREG = sizeof(regions) / sizeof(regions[0]);
const PtrGlobal ptrglobals[] = { { 0x800B0078u, &g_FieldScriptVMCurActor }, { 0x800ADC00u, &g_FieldScriptVMCurScriptData } };
const unsigned NPTR = sizeof(ptrglobals) / sizeof(ptrglobals[0]);

static uint64_t h2g(long p) {
    for (unsigned r = 0; r < NREG; r++)
        if ((uintptr_t)p >= (uintptr_t)regions[r].nat && (uintptr_t)p < (uintptr_t)regions[r].nat + regions[r].size)
            return regions[r].base + (uint32_t)((uintptr_t)p - (uintptr_t)regions[r].nat);
    return (uint64_t)p;
}
SPY(func_8009CD18, S_CD18)
long func_80033CD0(long a, long b, long c, long d, long e, long f) {
    uint64_t in[6] = { h2g(a), (uint64_t)b, (uint64_t)c, (uint64_t)d, (uint64_t)e, (uint64_t)f };
    return spy_core(S_33CD0, in);
}
long func_80034800(long a, long b, long c, long d, long e, long f) {
    uint64_t in[6] = { h2g(a), (uint64_t)b, (uint64_t)c, (uint64_t)d, (uint64_t)e, (uint64_t)f };
    return spy_core(S_34800, in);
}
static int32_t feed_map(uint32_t id, int32_t raw) {
    if (id == S_CD18) return ((uint32_t)raw % 3u) ? 0 : raw;
    if (id == S_33CD0) return ((uint32_t)raw & 1u) ? 1 : raw;
    return raw;
}
const Target targets[] = {
    { 0x8009CD18, S_CD18, 1, 1, 0x1, 1 },
    { 0x80033CD0, S_33CD0, 1, 1, 0, 0 },
    { 0x80034800, S_34800, 4, 0, 0, 0 },
};
const unsigned NTARGETS = sizeof(targets) / sizeof(targets[0]);

extern void func_8009BC98(void);
static uint32_t n_BC98(Args a) { (void)a; func_8009BC98(); return 0; }
static void g_BC98(Args* a) {
    (void)a;
    w32(0x800B0078u, CURACT); w32(0x800ADC00u, SCRIPT);
    uint16_t ip = (uint16_t)rnd_range(0x8000);
    w16(CURACT + 0xCC, ip);
    outfeed[0][0] = (int32_t)rnd_range(4);           /* the box index written by func_8009CD18 */
    uint32_t box = 0x800C2698u + 0x498u * (uint32_t)outfeed[0][0];
    if (rnd() & 1) w16(box + 0x9C, 0);
    if (rnd() & 1) w8(box + 0x84, 0);
    if (rnd() & 1) w16(box + 0x84, 0);
    if (rnd() & 1) w8(box + 0x6C, 0);
}
Fn fns[] = { { "func_8009BC98", 0x8009BC98, 112, n_BC98, g_BC98, 4000, 0 } };
const unsigned NFNS = sizeof(fns) / sizeof(fns[0]);
