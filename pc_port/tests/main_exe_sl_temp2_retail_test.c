/* Retail differential: main-exe arms of src/slus_006.64/system/temp2.c not covered by
 * main_exe_temp2_retail_test.c:  func_8002BA58 func_80031798 func_800317E0 func_80031804
 * func_800303C8 func_800305D8.  Callees (Heap*, Cd*, func_800301C8/30228/302D4, the anim
 * channel callback) are SPIES on both sides.  Pointers stored in fixture memory are low-region
 * host addresses, so guest word == host pointer low 32 bits.  The one representation
 * difference: retail stores guest code address 0x8003014C as the channel callback in
 * func_800303C8; the port stores the host address of func_8003014C (low 32 bits); the
 * retail_norm hook maps that single word (documented port-only representation).
 * func_80031798 links against the REAL production PcPort_AddPrimDomainAware
 * (pc_port/src/guest_prim_link.c, compiled into the harness); fixtures keep
 * the OT word's high byte at 0, the guest-representable subdomain where the
 * domain-aware link is byte-identical to retail (the high-byte preservation
 * for host OTs is deliberate port-only behavior). func_800317E0/31804 here
 * exercise the temp2.c TU's GPU_LINK_FUNC bodies; production links the
 * game_overrides.c twins (port_owned_overrides.txt), which are the same
 * three operations in the same order (old=*ot; *ot=prim&0xFFFFFF;
 * *prim=old|tag) and differ only in operand spelling. */
#include "slus_oracle.h"

extern void func_8002BA58(void);
extern void func_80031798(void*, void*);
extern void func_800317E0(void*, void*);
extern void func_80031804(void*, void*);
extern void* func_800303C8(u8*, s32);
extern void func_800305D8(u8*);
extern s32 func_8003014C(s32*);

s32 D_8004FE40; s16 D_8004FE28; s32 D_8004FE38; s32 g_ArchiveCurFileSize; s32 D_8004FDFC;
void *D_8004FE2C;

int g_GPUDisabledState; unsigned g_PcPortPresentedFrames; uint8_t g_PsxScratchpad[4096];

/* spies (native side) */
void *CdDataCallback(void *f) { spy(1, (uint32_t)(uintptr_t)f, 0, 0, 0, 0, 0); return 0; }
void ArchiveCdSeekToFile(int e) { spy(2, (uint32_t)e, 0, 0, 0, 0, 0); }
void HeapSetCurrentContentType(int t) { spy(3, (uint32_t)t, 0, 0, 0, 0, 0); }
static uint32_t heap_next;
void *HeapAlloc(unsigned sz, unsigned fl) { spy(4, sz, fl, 0, 0, 0, 0); uint32_t a = heap_next; heap_next += (sz + 15u) & ~15u; return (void *)(uintptr_t)a; }
void func_800301C8(u8 *d, u8 *s, s32 n, s16 *i) { spy(5, (uint32_t)(uintptr_t)d, (uint32_t)(uintptr_t)s, (uint32_t)n, (uint32_t)(uintptr_t)i, 0, 0); }
void func_80030228(u8 *v, s16 *d, s32 n, s32 sc) { spy(6, (uint32_t)(uintptr_t)v, (uint32_t)(uintptr_t)d, (uint32_t)n, (uint32_t)sc, 0, 0); }
void func_800302D4(u8 *v, s16 *d, s32 n, s32 sc) { spy(7, (uint32_t)(uintptr_t)v, (uint32_t)(uintptr_t)d, (uint32_t)n, (uint32_t)sc, 0, 0); }
static s32 chan_cb(s32 *p) { return spy(8, (uint32_t)(uintptr_t)p, 0, 0, 0, 0, 0); }

/* retail targets */
#define R_CDCB   0x800413ECu
#define R_SEEK   0x8002A394u
#define R_HALLOC 0x80031BDCu
#define R_HSET   0x800324B8u
#define R_301C8  0x800301C8u
#define R_30228  0x80030228u
#define R_302D4  0x800302D4u
static int bridge(PcPortMipsCpu *c, uint32_t t)
{
    uint32_t *r = c->gpr;
    switch (t) {
    case R_CDCB: r[2] = (uint32_t)spy(1, r[4], 0, 0, 0, 0, 0); return 1;
    case R_SEEK: spy(2, r[4], 0, 0, 0, 0, 0); return 1;
    case R_HSET: spy(3, r[4], 0, 0, 0, 0, 0); return 1;
    case R_HALLOC: { spy(4, r[4], r[5], 0, 0, 0, 0); uint32_t a = heap_next; heap_next += (r[4] + 15u) & ~15u; r[2] = a; return 1; }
    case R_301C8: spy(5, r[4], r[5], r[6], r[7], 0, 0); return 1;
    case R_30228: spy(6, r[4], r[5], r[6], r[7], 0, 0); return 1;
    case R_302D4: spy(7, r[4], r[5], r[6], r[7], 0, 0); return 1;
    default:
        if (t == (uint32_t)(uintptr_t)chan_cb) { r[2] = (uint32_t)spy(8, r[4], 0, 0, 0, 0, 0); return 1; }
        return 0;
    }
}
static unsigned norm_slots;
static void norm(uint8_t *lr)
{ /* retail guest callback address -> native host callback address in the func_800303C8 slot table
   * (slots start at heap block 0 + 0x14, i.e. low offset 0x4014, stride 0x20) */
    uint32_t cbn = (uint32_t)(uintptr_t)func_8003014C;
    for (unsigned i = 0; i < norm_slots; i++)
        if (!memcmp(lr + 0x4014 + 0x20 * i, "\x4C\x01\x03\x80", 4)) memcpy(lr + 0x4014 + 0x20 * i, &cbn, 4);
}

#define OFF_SLOTS 0x100u
static const Glob g_arch[] = {
    { "D_8004FE40", &D_8004FE40, 0x8004FE40u, 4, 0 }, { "D_8004FE28", &D_8004FE28, 0x8004FE28u, 2, 0 },
    { "D_8004FE38", &D_8004FE38, 0x8004FE38u, 4, 0 }, { "g_ArchiveCurFileSize", &g_ArchiveCurFileSize, 0x8004FDF8u, 4, 0 },
    { "D_8004FDFC", &D_8004FDFC, 0x8004FDFCu, 4, 0 }, { "D_8004FE2C", &D_8004FE2C, 0x8004FE2Cu, 8, 1 },
};
static const Glob g_none[] = { { "-", &rng_s, 0x80000000u + 0x1ff800u, 0, 0 } };

static void setup_ba58(uint32_t id)
{
    unsigned n = rn(8);
    /* slotCount is bounded to the terminating subdomain (0..129): retail walks
     * with a 16-bit-wrapped counter, so counts above 32767 hang BOTH sides,
     * and counts above ~8k read past the mapped fixture on the native side.
     * 129 keeps every visited slot inside fixture-randomized memory. */
    norm_slots = 0;
    for (unsigned i = 0; i < 0x800; i++) low[i] = (u8)rnd();
    D_8004FE40 = (id % 9 == 0) ? (s32)rn(130) : (s32)n;
    D_8004FE28 = (s16)(rn(3) ? rn(4) : rnd());
    D_8004FE38 = (s32)rnd(); g_ArchiveCurFileSize = (id % 3 == 0) ? (s32)rnd_val() : (s32)rn(3) - 1; D_8004FDFC = (s32)rrange(-1, 3);
    D_8004FE2C = lo(OFF_SLOTS);
    for (unsigned i = 0; i < 8; i++) {
        put16(lo(OFF_SLOTS + 8 * i), rn(3) ? 1 : rn(5));
        put16(lo(OFF_SLOTS + 8 * i + 2), rn(3) ? (u16)D_8004FE28 : rn(6));
    }
    feed[0] = 0;
}
static uint32_t nat_ba58(void) { func_8002BA58(); return 0; }

static void setup_link(uint32_t id)
{
    (void)id;
    /* func_80031798 is byte-identical iff the OT word's high byte is 0: the
     * port routes through PcPort_AddPrimDomainAware, which preserves the OT
     * high byte, while retail stores prim&0xFFFFFF over the full word.
     * (Deliberate port-only representation for host OTs; fixtures stay in the
     * guest-representable subdomain.) func_800317E0/31804 need no constraint:
     * both sides store full words there. Pointers stay below 16 MiB so the
     * retail 24-bit mask is lossless on both sides. */
    norm_slots = 0;
    for (unsigned i = 0; i < 0x100; i++) low[i] = (u8)rnd();
    A[0] = lo_addr(0x20); A[1] = lo_addr(0x40 + 0x10 * rn(4));
    if (rn(4) == 0) A[1] = (A[1] & ~0xFFu) | 0x80; /* non-adjacent */
    *(uint32_t *)lo(0x20) &= 0x00FFFFFFu;
}
static uint32_t nat_31798(void) { func_80031798(lo(0x20), (void *)(uintptr_t)A[1]); return 0; }
static uint32_t nat_317E0(void) { func_800317E0(lo(0x20), (void *)(uintptr_t)A[1]); return 0; }
static uint32_t nat_31804(void) { func_80031804(lo(0x20), (void *)(uintptr_t)A[1]); return 0; }

static void setup_303c8(uint32_t id)
{
    (void)id;
    for (unsigned i = 0; i < 0x1000; i++) low[i] = (u8)rnd();
    heap_next = lo_addr(0x4000);
    uint32_t model = 0x100, desc = 0x200, vtx = 0x400, nrm = 0x800;
    put16(lo(model + 0), rn(2) ? 0x10 : (rnd() & 0xFFEF)); put16(lo(model + 2), rn(6));
    put32(lo(model + 8), lo_addr(vtx)); put32(lo(model + 0xC), lo_addr(nrm));
    /* desc is always mapped: a NULL table makes the native side fault on a
     * host NULL deref while retail reads the KUSEG alias (g_PsxRam[0]), so
     * that input is outside the differential subdomain by construction. */
    put32(lo(model + 0x1C), lo_addr(desc));
    put32(lo(desc), rn(5));
    A[0] = lo_addr(model); A[1] = rnd() & 3;
    norm_slots = *(u32 *)lo(desc);
}
/* heap_next is shared across the retail/native sides of a case: rewind it so
 * both sides allocate the identical block sequence at identical addresses. */
static uint32_t nat_303c8(void)
{
    heap_next = lo_addr(0x4000);
    return (uint32_t)(uintptr_t)func_800303C8(lo(0x100), (s32)A[1]);
}

static void setup_305d8(uint32_t id)
{
    (void)id;
    norm_slots = 0; /* the 303C8 callback norm must not touch other fns' cases */
    for (unsigned i = 0; i < 0x1000; i++) low[i] = (u8)rnd();
    uint32_t anim = 0x100, model = 0x200, ch = 0x300, jt = 0x500;
    unsigned cnt = rn(5);
    put32(lo(anim + 0), lo_addr(model)); put32(lo(anim + 4), lo_addr(0x700));
    put32(lo(anim + 0xC), cnt); put32(lo(anim + 0x10), lo_addr(ch));
    put16(lo(model + 0), rn(2) ? 0x10 : (rnd() & 0xFFEF));
    put32(lo(model + 8), lo_addr(0x900)); put32(lo(model + 0xC), lo_addr(0xA00));
    put32(lo(model + 0x1C), lo_addr(jt - 4));
    for (unsigned i = 0; i < 6; i++) {
        put32(lo(ch + 0x20 * i), (uint32_t)(uintptr_t)chan_cb);
        put32(lo(jt + 12 * i + 0), rrange(0, 50)); put32(lo(jt + 12 * i + 4), lo_addr(0xB00 + 0x40 * i));
        put32(lo(jt + 12 * i + 8), lo_addr(0xC00 + 0x40 * i));
    }
    put32(lo(jt + 12 * cnt), lo_addr(0xD00)); /* the index table pointer read at joints+count*12 */
    for (unsigned i = 0; i < 6; i++) feed[i] = (s32)rnd_val();
    A[0] = (id % 11 == 0) ? 0 : lo_addr(anim);
}
static uint32_t nat_305d8(void) { func_800305D8(A[0] ? lo(0x100) : NULL); return 0; }

static const Fn fns[] = {
    { "func_8002BA58", 0x8002BA58u, 0, setup_ba58, nat_ba58 },
    { "func_80031798", 0x80031798u, 0, setup_link, nat_31798 },
    { "func_800317E0", 0x800317E0u, 0, setup_link, nat_317E0 },
    { "func_80031804", 0x80031804u, 0, setup_link, nat_31804 },
    { "func_800303C8", 0x800303C8u, 1, setup_303c8, nat_303c8 },
    { "func_800305D8", 0x800305D8u, 0, setup_305d8, nat_305d8 },
};

int main(int argc, char **argv)
{
    const char *only = argc > 1 ? argv[1] : NULL;
    unsigned cases = getenv("XENO_CASES") ? (unsigned)atoi(getenv("XENO_CASES")) : 2000;
    int bad = 0, ran = 0;
    cur_what = "MAIN EXE SL TEMP2";
    setvbuf(stdout, NULL, _IOLBF, 0);
    map_low(); load_slus(cur_what);
    user_bridge = bridge; retail_norm = norm;
    for (unsigned k = 0; k < sizeof(fns) / sizeof(fns[0]); k++) {
        const Fn *f = &fns[k]; unsigned fails = 0;
        if (only && strcmp(only, f->name)) continue;
        if (!strcmp(f->name, "func_8002BA58")) { globs = g_arch; nglobs = 6; } else { globs = g_none; nglobs = 0; }
        prints_left = 1;
        for (unsigned id = 0; id < cases; id++) fails += (unsigned)run_case(f, id + 1, 0);
        printf("MAIN EXE SL TEMP2 %s %s cases=%u diverged=%u\n", f->name, fails ? "DIVERGES" : "PASS", cases, fails);
        ran++; bad += fails ? 1 : 0;
    }
    if (!ran) { fprintf(stderr, "no such function\n"); return 2; }
    printf("MAIN EXE SL TEMP2 %s (%d function(s) diverge)\n", bad ? "FAIL" : "ALL PASS", bad);
    return bad ? 1 : 0;
}
