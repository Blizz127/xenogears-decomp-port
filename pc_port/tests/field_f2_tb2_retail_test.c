/* Retail differential, src/field/dialogue/text_box.c: func_8009C154 (face
 * portrait slot allocator / loader).  The production TU is linked
 * unmodified; see lib/field_retail_core.h.
 * Host layout note (not a behaviour difference): the stream queue
 * D_800B00C8 is StreamDataQueueEntry {s16 idx; u16 pad; void* pData} =
 * 16 bytes on the host, 8 on retail; its only consumer (native
 * func_80029AFC) uses the same host struct.  The native host entries are
 * serialised to the retail layout (idx at +0, low 32 bits of pData at +4)
 * before the byte compare; entries the port did not write keep their
 * start bytes.  D_800ADB10/D_800ADB14 are host void* pointer globals. */
#define TESTNAME "FIELD F2 TB2"
enum { S_CDSYNC = 1, S_TIM, S_HEAPFREE, S_C538, S_SETIDX, S_DECODE, S_HEAPALLOC, S_29AFC };
#include <stdint.h>
static int32_t feed_map(uint32_t id, int32_t raw);
#define FEED_MAP(id, raw) feed_map(id, raw)
static void native_pre(void);
static void native_post(void);
#define NATIVE_PRE() native_pre()
#define NATIVE_POST() native_post()
#define BUF_BASE 0x80120000u
#define BUF_SIZE 0x20000u
#include "field_retail_core.h"

#define CURACT 0x80120000u

#define RD_BASE 0x800ADB00u   /* D_800ADB0C; D_800ADB10/14 mirrored */
#define RD_SIZE 0x20u
#define RE_BASE 0x800AE1E0u   /* face archive pairs */
#define RE_SIZE 0xB4u
#define RC_BASE 0x800AEAE0u   /* D_800AEAE4 coords */
#define RC_SIZE 0x50u
#define RQ_BASE 0x800B00C8u   /* stream queue (retail layout) */
#define RQ_SIZE 0x20u
#define RS_BASE 0x800B06A4u   /* face slots, 3 x {s16,s16,s16} */
#define RS_SIZE 0x14u
REGION(RD); REGION(RE); REGION(RC); REGION(RQ); REGION(RS);
ALIAS(D_800ADB0C, RD, 0x800ADB0C);
ALIAS(D_800AE1E0, RE, 0x800AE1E0);
ALIAS(D_800AEAE4, RC, 0x800AEAE4);
ALIAS(D_800B06A4, RS, 0x800B06A4); ALIAS(D_800B06A6, RS, 0x800B06A6); ALIAS(D_800B06A8, RS, 0x800B06A8);
typedef struct { int16_t idx; uint16_t pad; void* p; } HostEntry;
HostEntry D_800B00C8[4];
uintptr_t g_FieldScriptVMCurActor, D_800ADB10, D_800ADB14;
const Region regions[] = { { RD_BASE, RD_SIZE, RD }, { RE_BASE, RE_SIZE, RE }, { RC_BASE, RC_SIZE, RC },
                           { RQ_BASE, RQ_SIZE, RQ }, { RS_BASE, RS_SIZE, RS } };
const unsigned NREG = sizeof(regions) / sizeof(regions[0]);
const PtrGlobal ptrglobals[] = { { 0x800B0078u, &g_FieldScriptVMCurActor }, { 0x800ADB10u, &D_800ADB10 },
                                 { 0x800ADB14u, &D_800ADB14 } };
const unsigned NPTR = sizeof(ptrglobals) / sizeof(ptrglobals[0]);

static uint32_t xr[64], xn[64];
static unsigned xr_n, xn_n;
SPY(ArchiveCdDataSync, S_CDSYNC)
SPY(func_8009C538, S_C538)
SPY(ArchiveSetIndex, S_SETIDX)
SPY(ArchiveDecodeAlignedSize, S_DECODE)
long HeapFree(long a, long b, long c, long d, long e, long f) {
    uint64_t in[6] = { (uint64_t)(uint32_t)a, (uint64_t)b, (uint64_t)c, (uint64_t)d, (uint64_t)e, (uint64_t)f };
    return spy_core(S_HEAPFREE, in);
}
long HeapAlloc(long a, long b, long c, long d, long e, long f) {
    uint64_t in[6] = { (uint64_t)a, (uint64_t)b, (uint64_t)c, (uint64_t)d, (uint64_t)e, (uint64_t)f };
    return (long)(uint32_t)spy_core(S_HEAPALLOC, in);
}
long FieldLoadTIMWithClut(long a, long b, long c, long d, long e, long f, long g) {
    require(xn_n < 64, "extra"); xn[xn_n++] = (uint32_t)g;
    uint64_t in[6] = { (uint64_t)(uint32_t)a, (uint64_t)b, (uint64_t)c, (uint64_t)d, (uint64_t)e, (uint64_t)f };
    return spy_core(S_TIM, in);
}
long func_80029AFC(long a, long b, long c, long d, long e, long f) {
    uint64_t in[6] = { a == (long)(uintptr_t)D_800B00C8 ? RQ_BASE : (uint64_t)a, (uint64_t)b, (uint64_t)c,
                       (uint64_t)d, (uint64_t)e, (uint64_t)f };
    return spy_core(S_29AFC, in);
}
const Target targets[] = {
    { 0x80028A60, S_CDSYNC, 1, 1, 0, 0 },
    { 0x80070340, S_TIM, 6, 1, 0, 0 },   /* ret=1 only so FEED_MAP runs: void callee, v0 dead */
    { 0x800320E8, S_HEAPFREE, 1, 0, 0, 0 },
    { 0x8009C538, S_C538, 1, 1, 0, 0 },
    { 0x80028470, S_SETIDX, 2, 0, 0, 0 },
    { 0x800288EC, S_DECODE, 1, 1, 0, 0 },
    { 0x80031BDC, S_HEAPALLOC, 2, 1, 0, 0 },
    { 0x80029AFC, S_29AFC, 3, 0, 0, 0 },
};
const unsigned NTARGETS = sizeof(targets) / sizeof(targets[0]);
static int32_t feed_map(uint32_t id, int32_t raw) {
    if (id == S_CDSYNC) return ((uint32_t)raw & 1u) ? 0 : raw;
    if (id == S_C538) return ((uint32_t)raw % 3u) ? (int32_t)((uint32_t)raw & 1u) : raw;
    if (id == S_HEAPALLOC) return (int32_t)(0x80130000u + ((uint32_t)raw & 0xFFF0u));
    if (id == S_TIM) { if (active_cpu) { require(xr_n < 64, "extra"); xr[xr_n++] = r32(active_cpu->gpr[29] + 0x18); } return 0; }
    return raw;
}
static void native_pre(void) {
    xn_n = 0;
    for (unsigned k = 0; k < 4; k++) { D_800B00C8[k].idx = 0x7777; D_800B00C8[k].p = (void*)(uintptr_t)0x77777777u; }
}
static void native_post(void) {
    require(xn_n == xr_n, "stack-argument count");
    require(memcmp(xn, xr, 4 * xn_n) == 0, "stack argument 7");
    for (unsigned k = 0; k < 4; k++) {
        if (D_800B00C8[k].idx != 0x7777) memcpy(RQ + 8 * k, &D_800B00C8[k].idx, 2);
        if ((uintptr_t)D_800B00C8[k].p != 0x77777777u) { uint32_t v = (uint32_t)(uintptr_t)D_800B00C8[k].p; memcpy(RQ + 8 * k + 4, &v, 4); }
    }
}

extern int32_t func_8009C154(int32_t);
static uint32_t n_C154(Args a) { return (uint32_t)func_8009C154((int32_t)a.a0); }
static void g_C154(Args* a) {
    xr_n = 0;
    w32(0x800B0078u, CURACT);
    a->a0 = (rnd() % 8) ? rnd_range(0x5A) : (uint32_t)rnd_val();
    if (a->a0 >= 0x5A) a->a0 = rnd_range(0x5A);   /* retail indexes D_800AE1E0[faceId*2] (0xB4 bytes) */
    for (unsigned s = 0; s < 3; s++) {
        uint32_t e = 0x800B06A4u + 6 * s;
        w16(e + 0, (uint16_t)((rnd() & 1) ? rnd_range(0x5A) : (uint16_t)rnd()));
        w16(e + 2, (uint16_t)((rnd() % 3) ? 0 : small(-1, 3)));
        w16(e + 4, (uint16_t)small(-1, 2));
    }
    if (rnd() & 1) w16(0x800B06A4u + 6 * rnd_range(3), (uint16_t)a->a0);
    w32(0x800ADB0C, (uint32_t)small(-1, 4));   /* < -1 makes retail index slot -1 (OOB, not generated) */
    w32(0x800ADB10, 0x80140000u + 0x10 * rnd_range(0x100));
    w32(0x800ADB14, 0x80148000u + 0x10 * rnd_range(0x100));
    if (rnd() & 1) w8(0x800AE1E0 + 2 * a->a0 + 1, *gp(0x800AE1E0 + 2 * a->a0));
}
Fn fns[] = { { "func_8009C154", 0x8009C154, 249, n_C154, g_C154, 6000, 1 } };
const unsigned NFNS = sizeof(fns) / sizeof(fns[0]);
