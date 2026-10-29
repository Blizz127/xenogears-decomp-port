/* Retail differential, src/field/main/misc5.c: func_800A6E70 (builds the
 * 14x20 POLY_GT4 grid + corner-fade tables of the mode-4 field transition in
 * one 0x7A80 heap block).  The production TU is linked unmodified; see
 * lib/field_retail_core.h.  The whole block is pre-filled at random, so every
 * byte the port fails to write is visible. */
#define TESTNAME "FIELD F2 MISC5"
enum { S_HEAPALLOC = 1, S_POLYGT4, S_TPAGE, S_SEMI };
#include <stdint.h>
static int32_t feed_map(uint32_t id, int32_t raw);
#define FEED_MAP(id, raw) feed_map(id, raw)
#define MAXEV 1024
#define ORACLE_STEPS 400000
#define BUF_BASE 0x80120000u
#define BUF_SIZE 0x10000u
#include "field_retail_core.h"

#define RA_BASE 0x800B00C4u   /* D_800B00C4 (host void*): mirrored final value */
#define RA_SIZE 0x4u
REGION(RA);
uintptr_t D_800B00C4;
const Region regions[] = { { RA_BASE, RA_SIZE, RA } };
const unsigned NREG = sizeof(regions) / sizeof(regions[0]);
const PtrGlobal ptrglobals[] = { { 0x800B00C4u, &D_800B00C4 } };
const unsigned NPTR = sizeof(ptrglobals) / sizeof(ptrglobals[0]);

long HeapAlloc(long a, long b, long c, long d, long e, long f) {
    /* pointer result: zero-extend the 32-bit guest address (identity-mapped) */
    uint64_t in[6] = { (uint64_t)a, (uint64_t)b, (uint64_t)c, (uint64_t)d, (uint64_t)e, (uint64_t)f };
    return (long)(uint32_t)spy_core(S_HEAPALLOC, in);
}
SPY(SetPolyGT4, S_POLYGT4)
SPY(GetTPage, S_TPAGE)
SPY(SetSemiTrans, S_SEMI)
const Target targets[] = {
    { 0x80031BDC, S_HEAPALLOC, 2, 1, 0, 0 },
    { 0x80043CD8, S_POLYGT4, 1, 0, 0, 0 },
    { 0x80043A1C, S_TPAGE, 4, 1, 0, 0 },
    { 0x80043BFC, S_SEMI, 2, 0, 0, 0 },
};
const unsigned NTARGETS = sizeof(targets) / sizeof(targets[0]);
static int32_t feed_map(uint32_t id, int32_t raw) {
    if (id == S_HEAPALLOC) return (int32_t)(BUF_BASE + ((uint32_t)raw & 0x7FCu));
    return raw;
}

extern void func_800A6E70(void);
static uint32_t n_6E70(Args a) { (void)a; func_800A6E70(); return 0; }
static void g_6E70(Args* a) { (void)a; }
Fn fns[] = { { "func_800A6E70", 0x800A6E70, 125, n_6E70, g_6E70, 60, 0 } };
const unsigned NFNS = sizeof(fns) / sizeof(fns[0]);
