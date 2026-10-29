/* Retail differential, src/field/scripts/virtual_machine.c: the field-state
 * save/restore pair func_800A3F4C (save into D_8005A4E4) and func_800A3474
 * (restore).  Global windows follow the PC data layout (data_field.c blocks:
 * g_FieldBss_800AF85C holds g_CameraEye..g_Scene, so retail's 0x1C8-byte
 * camera copy spans the same bytes on both sides).  D_800AFC50 (host u8*) is
 * a pointer global; its final value is mirrored into an unnamed window and
 * compared.  func_80021EBC (sprite serialiser) is spied: it writes 30 fed
 * bytes through its out pointer; the remaining 0x12 bytes keep the buffer's
 * previous contents on both sides.
 * Coverage limit (retail-inherent): the unaligned (lwl/lwr) arms of copies
 * between fixed, word-aligned retail globals are dead code; only the *D_800AFB20
 * and heap-block copies are exercised misaligned. */
#define TESTNAME "FIELD F2 VM2"
enum { S_HEAPALLOC = 1, S_21EBC, S_PRINTF };
#include <stdint.h>
static int32_t feed_map(uint32_t id, int32_t raw);
#define FEED_MAP(id, raw) feed_map(id, raw)
static void native_post(void);
#define NATIVE_PRE() ((void)0)
#define NATIVE_POST() native_post()
#define BUF_BASE 0x80120000u
#define BUF_SIZE 0x60000u
#include "field_retail_core.h"

#define FACTORS 0x80130000u
#define ADATA 0x80140000u
#define HEAPB 0x80150000u
#define TEX400 0x80160000u
#define GSTATE 0x80170000u
#define NACT 8u

#define RS_BASE 0x8005A400u   /* D_8005A408[3] */
#define RS_SIZE 0x20u
#define RV_BASE 0x8005A4E0u   /* D_8005A4E4 save image */
#define RV_SIZE 0x2200u
#define RD_BASE 0x800ADBF0u
#define RD_SIZE 0x10u
#define RC_BASE 0x800AF85Cu   /* g_FieldBss_800AF85C (camera .. g_Scene) */
#define RC_SIZE 0x2A0u
#define RN_BASE 0x800AFB0Cu   /* g_FieldNumActors */
#define RN_SIZE 0x4u
#define RT_BASE 0x800AFB20u   /* g_FieldBss_800AFB20 */
#define RT_SIZE 0x108u
#define RP_BASE 0x800AFC50u   /* D_800AFC50 final value (mirrored) */
#define RP_SIZE 0x4u
#define RB_BASE 0x800B007Cu   /* g_FieldBss_800B007C */
#define RB_SIZE 0x40u
#define RE_BASE 0x800B2078u   /* g_FieldEffects block */
#define RE_SIZE 0x424u
#define RM_BASE 0x800C2680u
#define RM_SIZE 0x10u
#define RK_BASE 0x800C3A68u   /* g_FieldScriptMemory */
#define RK_SIZE 0x800u
REGION(RS); REGION(RV); REGION(RD); REGION(RC); REGION(RN); REGION(RT); REGION(RP); REGION(RB);
REGION(RE); REGION(RM); REGION(RK);
ALIAS(D_8005A408, RS, 0x8005A408);
ALIAS(D_8005A4E4, RV, 0x8005A4E4);
ALIAS(D_800ADBFC, RD, 0x800ADBFC);
ALIAS(g_CameraEye, RC, 0x800AF880); ALIAS(g_Scene, RC, 0x800AF990);
ALIAS(g_FieldNumActors, RN, 0x800AFB0C);
ALIAS(D_800AFB20, RT, 0x800AFB20);
ALIAS(D_800B007C, RB, 0x800B007C);
ALIAS(g_FieldEffects, RE, 0x800B2078);
ALIAS(g_FieldSystemMode, RM, 0x800C268C);
ALIAS(g_FieldScriptMemory, RK, 0x800C3A68);
uintptr_t g_FieldActors, g_pGameState, D_800AFC50;
char D_8006FD9C[4];

const Region regions[] = {
    { RS_BASE, RS_SIZE, RS }, { RV_BASE, RV_SIZE, RV }, { RD_BASE, RD_SIZE, RD }, { RC_BASE, RC_SIZE, RC },
    { RN_BASE, RN_SIZE, RN }, { RT_BASE, RT_SIZE, RT }, { RP_BASE, RP_SIZE, RP }, { RB_BASE, RB_SIZE, RB },
    { RE_BASE, RE_SIZE, RE }, { RM_BASE, RM_SIZE, RM }, { RK_BASE, RK_SIZE, RK },
};
const unsigned NREG = sizeof(regions) / sizeof(regions[0]);
const PtrGlobal ptrglobals[] = {
    { 0x800AFB10u, &g_FieldActors }, { 0x8005A39Cu, &g_pGameState }, { 0x800AFC50u, &D_800AFC50 },
};
const unsigned NPTR = sizeof(ptrglobals) / sizeof(ptrglobals[0]);

static uint64_t h2g(long p) {
    for (unsigned r = 0; r < NREG; r++)
        if ((uintptr_t)p >= (uintptr_t)regions[r].nat && (uintptr_t)p < (uintptr_t)regions[r].nat + regions[r].size)
            return regions[r].base + (uint32_t)((uintptr_t)p - (uintptr_t)regions[r].nat);
    if ((uintptr_t)p == (uintptr_t)D_8006FD9C) return 0x8006FD9Cu;
    return (uint64_t)(uint32_t)p;
}
SPY(HeapAlloc, S_HEAPALLOC)
long func_80021EBC(long a, long b, long c, long d, long e, long f) {
    /* the core writes the fed bytes through the host pointer; record the guest address */
    uint64_t in[6] = { (uint64_t)a, (uint64_t)b, (uint64_t)c, (uint64_t)d, (uint64_t)e, (uint64_t)f };
    long r = spy_core(S_21EBC, in);
    trace.ev[trace.n - 1].a[1] = (uint32_t)h2g(b);
    return r;
}
long func_800379C8(long a, long b, long c, long d, long e, long f) {
    uint64_t in[6] = { h2g(a), (uint64_t)b, (uint64_t)c, (uint64_t)d, (uint64_t)e, (uint64_t)f };
    return spy_core(S_PRINTF, in);
}

/* func_80021EBC's guest-side write is done by the bridge (outhw nibble 1 = 15 halfwords). */
const Target targets[] = {
    { 0x80031BDC, S_HEAPALLOC, 2, 1, 0, 0 },
    { 0x80021EBC, S_21EBC, 2, 0, 0x2, 0, 0xF0u, 0 },
    { 0x800379C8, S_PRINTF, 3, 0, 0, 0 },
};
const unsigned NTARGETS = sizeof(targets) / sizeof(targets[0]);

static unsigned heap_k;
static int32_t feed_map(uint32_t id, int32_t raw) {
    if (id == S_HEAPALLOC) { uint32_t k = heap_k++; return (int32_t)(HEAPB + 0x20 * (k % 0x400) + ((uint32_t)raw & 3u)); }
    return raw;
}

/* the native D_800AFC50 is a host pointer into the image window */
static void native_post(void) { D_800AFC50 = (uintptr_t)h2g((long)D_800AFC50); }

extern void func_800A3474(void);
extern void func_800A3F4C(void);
static uint32_t n_3474(Args a) { (void)a; heap_k = 0; func_800A3474(); return 0; }
static uint32_t n_3F4C(Args a) { (void)a; func_800A3F4C(); return 0; }

static void setup(void) {
    w32(0x800AFB10u, FACTORS); w32(0x8005A39Cu, GSTATE); w32(0x800AFB20u, TEX400 + ((rnd() & 1) ? rnd_range(4) : 0));   /* exercise the lwl/lwr copy paths */
    int32_t n = (rnd() % 8 == 0) ? small(-2, 0) : small(1, NACT);
    w32(0x800ADBFC, (uint32_t)n);
    for (unsigned k = 0; k < NACT; k++) {
        w32(FACTORS + k * 0x5C + 0x04, ADATA + 0x8000 + k * 0x100);
        w32(FACTORS + k * 0x5C + 0x4C, ADATA + k * 0x200);
        w32(ADATA + k * 0x200 + 0x110, HEAPB + 0x8000 + k * 0x40 + ((rnd() & 1) ? rnd_range(4) : 0));
        w32(ADATA + k * 0x200 + 0x114, HEAPB + 0x8000 + k * 0x40 + 0x20 + ((rnd() & 1) ? rnd_range(4) : 0));
    }
    w32(0x800C268C, (rnd() & 1) ? 0 : rnd());
}
static void g_3474(Args* a) {
    (void)a; setup(); heap_k = 0;
    /* flags that steer the restore live in the save image: per actor the
     * 0x138-byte ActorData copy lands at image offset +0x3C of its record */
    uint32_t cur = 0x8005A4E4u + 0xA4C;
    for (int32_t k = 0; k < (int32_t)r32(0x800ADBFC) && k < (int32_t)NACT; k++) {
        uint32_t ad = cur + 0x3C;
        uint32_t f134 = r32(ad + 0x134), f12c = r32(ad + 0x12C);
        f134 = (rnd() & 1) ? (f134 | 0x80) : (f134 & ~0x80u);
        f12c = (rnd() & 1) ? (f12c | 0x1000) : (f12c & ~0x1000u);
        w32(ad + 0x134, f134); w32(ad + 0x12C, f12c);
        cur = ad + 0x138 + ((f134 & 0x80) ? 0xC : 0) + ((f12c & 0x1000) ? 0x10 : 0);
    }
}
static void g_3F4C(Args* a) {
    (void)a; setup();
    for (unsigned k = 0; k < NACT; k++) {
        uint32_t ad = ADATA + k * 0x200;
        if (rnd() & 1) w32(ad + 0x134, r32(ad + 0x134) ^ 0x80);
        if (rnd() & 1) w32(ad + 0x12C, r32(ad + 0x12C) ^ 0x1000);
    }
}

Fn fns[] = {
    { "func_800A3474", 0x800A3474, 518, n_3474, g_3474, 600, 0 },
    { "func_800A3F4C", 0x800A3F4C, 511, n_3F4C, g_3F4C, 600, 0 },
};
const unsigned NFNS = sizeof(fns) / sizeof(fns[0]);
