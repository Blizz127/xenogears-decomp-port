/* Retail differential, src/field/main/misc10.c hand-written arms:
 * func_80096214 (item quantity -> script variable) and func_8009635C (add one
 * item).  The production TU is linked unmodified; see lib/field_retail_core.h.
 * Coverage limit: the item slot returned by func_80095124 / func_800951B8 is
 * kept to -1 or 0..0x3F (the inventory tables are 0x100-byte buffers here);
 * any other index would address unrelated retail RAM. */
#define TESTNAME "FIELD F2 MISC10"
enum { S_GETARG = 1, S_5124, S_50A0, S_501C, S_51B8, S_INSTARG, S_WRITE16 };
#include <stdint.h>
static int32_t feed_map(uint32_t id, int32_t raw) {
    if (id == S_5124 || id == S_51B8) return ((uint32_t)raw & 3u) == 0 ? -1 : (int32_t)((uint32_t)raw & 0x3Fu);
    return raw;
}
#define FEED_MAP(id, raw) feed_map(id, raw)
#define BUF_BASE 0x80120000u
#define BUF_SIZE 0x40000u
#include "field_retail_core.h"

#define SCRIPT 0x80120000u
#define CURACT 0x80130000u
#define QTY 0x80140000u
#define IDS 0x80141000u

#define RA_BASE 0x800B0070u
#define RA_SIZE 0x10u
REGION(RA);
uintptr_t g_FieldScriptVMCurActor, g_FieldScriptVMCurScriptData;
const Region regions[] = { { RA_BASE, RA_SIZE, RA } };
const unsigned NREG = sizeof(regions) / sizeof(regions[0]);
const PtrGlobal ptrglobals[] = { { 0x800B0078u, &g_FieldScriptVMCurActor }, { 0x800ADC00u, &g_FieldScriptVMCurScriptData } };
const unsigned NPTR = sizeof(ptrglobals) / sizeof(ptrglobals[0]);

SPY(FieldScriptVMGetArgument, S_GETARG)
SPY(func_80095124, S_5124)
SPY(func_800951B8, S_51B8)
SPY(FieldScriptVMGetInstructionArgument, S_INSTARG)
SPY(FieldScriptMemoryWriteU16, S_WRITE16)
/* pointer-returning callees: the guest address is zero-extended natively */
uint8_t* func_800950A0(long a) { uint64_t in[6] = { (uint64_t)a }; return (uint8_t*)(uintptr_t)(uint32_t)spy_core(S_50A0, in); }
uint8_t* func_8009501C(long a) { uint64_t in[6] = { (uint64_t)a }; return (uint8_t*)(uintptr_t)(uint32_t)spy_core(S_501C, in); }

const Target targets[] = {
    { 0x800ACDEC, S_GETARG, 1, 1, 0, 0 }, { 0x80095124, S_5124, 1, 1, 0, 0 }, { 0x800950A0, S_50A0, 1, 1, 0, 0 },
    { 0x8009501C, S_501C, 1, 1, 0, 0 }, { 0x800951B8, S_51B8, 1, 1, 0, 0 }, { 0x800ACDB8, S_INSTARG, 1, 1, 0, 0 },
    { 0x800A3074, S_WRITE16, 2, 0, 0, 0 },
};
const unsigned NTARGETS = sizeof(targets) / sizeof(targets[0]);

extern void func_80096214(void);
extern void func_8009635C(int32_t);
static uint32_t n_6214(Args a) { (void)a; func_80096214(); return 0; }
static uint32_t n_635C(Args a) { func_8009635C((int32_t)a.a0); return 0; }

static void setup(void) {
    w32(0x800B0078u, CURACT);
    w32(0x800ADC00u, SCRIPT);
    w16(CURACT + 0xCC, (uint16_t)((rnd() & 3) ? rnd_range(0x8000) : rnd()));
}
/* feed order: event 0 GetArgument, then 5124, 50A0, 501C, ... */
static void table_feeds(unsigned first) {
    feed[first + 1] = (int32_t)QTY;
    feed[first + 2] = (int32_t)IDS;
}
static void g_6214(Args* a) {
    (void)a; setup();
    table_feeds(1);
    if (rnd() & 1) for (unsigned i = 0; i < 0x40; i++) w8(QTY + i, (uint8_t)small(0, 0x70));
}
static void g_635C(Args* a) {
    a->a0 = (uint32_t)rnd_val();
    table_feeds(0);
    for (unsigned i = 0; i < 0x40; i++) {
        unsigned m = rnd_range(4);
        w8(QTY + i, m == 0 ? 0x62 : m == 1 ? 0x63 : (uint8_t)rnd());
    }
}

Fn fns[] = {
    { "func_80096214", 0x80096214, 43, n_6214, g_6214, 3000, 0 },
    { "func_8009635C", 0x8009635C, 44, n_635C, g_635C, 3000, 0 },
};
const unsigned NFNS = sizeof(fns) / sizeof(fns[0]);
