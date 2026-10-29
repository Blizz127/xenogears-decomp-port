/* Shared retail-differential oracle harness for hand-written functions of the
 * MAIN EXECUTABLE (slus_006.64).
 *
 * Retail side: the pinned SLUS_006.64 instructions run on the port's MIPS
 * interpreter (PcPortMipsRun), TEST ORACLE ONLY.  Guest RAM is g_PsxRam with
 * the SLUS loaded at 0x80010000 (read from disc/SLUS_006.64 at run time only;
 * no retail bytes live in the tree).  Fixtures live in a fixed low mmap region
 * (LOW_BASE) so a guest address IS the host pointer: the native port body and
 * the retail code see the same pointer values and the same bytes.
 *
 * Both sides start from byte-identical memory, identical guest globals
 * (native variable <-> guest RAM word, see Glob), an identical GTE register
 * file, and call identical callee spies/bridges (arguments are recorded and
 * compared).  After the call the whole low window, every global, the complete
 * GTE register file, the return value and the spy traces are compared
 * byte-for-byte.  Libgte/libc helpers are bridged to ONE shared host
 * implementation, so any difference is the port body's, not the helper's.
 * This is not a PS1 hardware oracle. */
#ifndef SLUS_ORACLE_H
#define SLUS_ORACLE_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include "battle_mips_adapter.h"
#include "psx/gtereg.h"

typedef uint8_t u8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;

extern unsigned int MFC2(int), CFC2(int);
extern void MTC2(unsigned int, int), CTC2(unsigned int, int);
extern int doCOP2(int);

#ifndef LOW_BASE
#define LOW_BASE 0x00E00000u
#endif
#define LOW_SIZE 0x10000u
#define LOW_CMP  0x8000u   /* compared window */
#define GP_VALUE 0x80059170u
#define SP_TOP   0x801ff000u
#define HALT_PC  0xfffffffcu

uint8_t g_PsxRam[0x200000];
static uint8_t *low;
static uint8_t low_init[LOW_SIZE], low_retail[LOW_SIZE];

/* ---- deterministic rng ---- */
static uint64_t rng_s = 1;
static uint32_t rnd(void) { rng_s ^= rng_s << 13; rng_s ^= rng_s >> 7; rng_s ^= rng_s << 17; return (uint32_t)(rng_s >> 16); }
static uint32_t rn(uint32_t n) { return rnd() % n; }
static int32_t rrange(int32_t lo, int32_t hi) { return lo + (int32_t)rn((uint32_t)(hi - lo + 1)); }
static void seed_case(uint32_t id) { rng_s = (uint64_t)id * 0x9E3779B97F4A7C15ull + 0x1234567ull; for (int i = 0; i < 8; i++) rnd(); }
/* value generator biased toward edge values */
static int32_t rnd_val(void)
{
    static const int32_t spec[] = { 0, 1, -1, 2, 3, 0x7F, 0x80, 0xFF, 0x100, 0x7FFF, 0x8000, 0xFFFF, 0x10000,
                                    0x7FFFFFFF, (int32_t)0x80000000, 0x12345, -0x8000, 0xFFF, 0x1000, -0x1000 };
    return (rnd() & 1) ? spec[rn(sizeof(spec) / sizeof(spec[0]))] : (int32_t)rnd();
}

/* ---- guest <-> host memory ---- */
static void *hp(uint32_t a)
{
    if (a >= LOW_BASE && (uint64_t)a < (uint64_t)LOW_BASE + LOW_SIZE) return (void *)(uintptr_t)a;
    if (a >= 0x80000000u && a < 0x80200000u) return g_PsxRam + (a & 0x1fffffu);
    fprintf(stderr, "ORACLE FAIL stray guest address %08x\n", a); exit(2);
}
static uint8_t *gaddr(uint32_t a, unsigned w)
{
    if (a >= LOW_BASE && (uint64_t)a + w <= (uint64_t)LOW_BASE + LOW_SIZE) return (uint8_t *)(uintptr_t)a;
    if (a >= 0x80000000u && (uint64_t)a + w <= 0x80200000u) return g_PsxRam + (a & 0x1fffffu);
    if ((uint64_t)a + w <= 0x00200000u) return g_PsxRam + a; /* KUSEG alias of RAM (address 0 reads) */
    return NULL;
}
static int h_rd(void *u, uint32_t a, unsigned w, uint32_t *v)
{
    (void)u; uint8_t *p = gaddr(a, w); if (!p) return -1;
    *v = 0; for (unsigned i = 0; i < w; i++) *v |= (uint32_t)p[i] << (8 * i);
    return 0;
}
static int h_wr(void *u, uint32_t a, unsigned w, uint32_t v)
{
    (void)u; uint8_t *p = gaddr(a, w); if (!p) return -1;
    for (unsigned i = 0; i < w; i++) p[i] = (uint8_t)(v >> (8 * i));
    return 0;
}
static uint32_t g32(uint32_t a) { uint32_t v; if (h_rd(0, a, 4, &v)) abort(); return v; }
static void put16(uint8_t *p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static void put32(uint8_t *p, uint32_t v) { put16(p, v); put16(p + 2, v >> 16); }
static uint32_t lo_addr(uint32_t off) { return LOW_BASE + off; }
static uint8_t *lo(uint32_t off) { return low + off; }

static void map_low(void)
{
    void *p = mmap((void *)(uintptr_t)LOW_BASE, LOW_SIZE, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
    if (p != (void *)(uintptr_t)LOW_BASE) { perror("mmap low fixture"); exit(2); }
    low = p;
}
static void load_slus(const char *what)
{
    const char *path = getenv("XENO_SLUS"); if (!path) path = "disc/SLUS_006.64";
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "%s FAIL cannot open %s (set XENO_SLUS)\n", what, path); exit(2); }
    if (fseek(f, 0x800, SEEK_SET)) exit(2);
    size_t n = fread(g_PsxRam + 0x10000, 1, 0x48800, f); fclose(f);
    if (n < 0x47000) { fprintf(stderr, "%s FAIL short SLUS read %zu\n", what, n); exit(2); }
}

/* ---- guest global <-> native variable sync ----
 * nat points at the native variable, g is its guest address.  isptr: the
 * native variable is a host pointer (8 bytes) that holds a low-region address;
 * the guest word is its low 32 bits.  The native value is the master. */
typedef struct { const char *name; void *nat; uint32_t g; unsigned size; int isptr; } Glob;
static const Glob *globs; static unsigned nglobs;
static uint8_t glob_retail[64][256];
static void glob_push(void)
{
    for (unsigned i = 0; i < nglobs; i++) {
        const Glob *x = &globs[i];
        if (x->isptr) { uint32_t v = (uint32_t)(uintptr_t)*(void **)x->nat; memcpy(hp(x->g), &v, 4); }
        else memcpy(hp(x->g), x->nat, x->size);
    }
}
static void glob_snapshot(void)
{
    for (unsigned i = 0; i < nglobs; i++) memcpy(glob_retail[i], hp(globs[i].g), globs[i].isptr ? 4 : globs[i].size);
}
static const char *glob_diff(void)
{
    for (unsigned i = 0; i < nglobs; i++) {
        const Glob *x = &globs[i];
        if (x->isptr) {
            uint32_t v = (uint32_t)(uintptr_t)*(void **)x->nat;
            if (memcmp(&v, glob_retail[i], 4)) return x->name;
        } else if (memcmp(x->nat, glob_retail[i], x->size)) return x->name;
    }
    return NULL;
}

/* ---- boundary spies: recorded identically on both sides ---- */
typedef struct { uint32_t id, a[6]; } Ev;
static Ev trace[32]; static unsigned ntrace;
static int32_t feed[32];
static int32_t spy(uint32_t id, uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5)
{
    unsigned k = ntrace;
    if (ntrace < 32) trace[ntrace] = (Ev){ id, { a0, a1, a2, a3, a4, a5 } };
    ntrace++;
    return k < 32 ? feed[k] : 0;
}

/* ---- GTE ---- */
static uint8_t gte_snap_init[sizeof(gteRegs)], gte_snap_retail[sizeof(gteRegs)];
static void gte_save(uint8_t *to) { memcpy(to, &gteRegs, sizeof(gteRegs)); }
static void gte_load(const uint8_t *from) { memcpy(&gteRegs, from, sizeof(gteRegs)); }
static void gte_seed_random(void)
{
    memset(&gteRegs, 0, sizeof(gteRegs));
    for (int i = 0; i < 5; i++) CTC2((uint32_t)rnd() & 0x1fff1fffu, i);        /* rotation */
    for (int i = 5; i < 8; i++) CTC2((uint32_t)rrange(-4096, 4096), i);       /* TR */
    for (int i = 8; i < 13; i++) CTC2((uint32_t)rnd() & 0x1fff1fffu, i);      /* LLM */
    for (int i = 13; i < 16; i++) CTC2((uint32_t)rrange(0, 4096), i);         /* BK */
    for (int i = 16; i < 21; i++) CTC2((uint32_t)rnd() & 0x1fff1fffu, i);     /* LCM */
    for (int i = 21; i < 24; i++) CTC2((uint32_t)rrange(0, 255), i);          /* FC */
    CTC2((uint32_t)rrange(-4096, 4096) << 16, 24); CTC2((uint32_t)rrange(-4096, 4096) << 16, 25);
    CTC2((uint32_t)rrange(100, 1000), 26); CTC2((uint32_t)rrange(-100, 100), 27); CTC2((uint32_t)rrange(-100, 100), 28);
    CTC2(341, 29); CTC2(256, 30);
    for (int i = 0; i < 6; i++) MTC2((uint32_t)rnd() & 0x7fff7fffu, i);       /* V0..V2 */
    MTC2(rnd() & 0xffffffu, 6);                                                /* RGBC */
    for (int i = 9; i < 12; i++) MTC2((uint32_t)rrange(-32768, 32767), i);    /* IR1..3 */
}
static uint32_t h_c2r(void *u, int ctl, unsigned r) { (void)u; return ctl ? CFC2((int)r) : MFC2((int)r); }
static void h_c2w(void *u, int ctl, unsigned r, uint32_t v) { (void)u; if (ctl) CTC2(v, (int)r); else MTC2(v, (int)r); }
static int h_c2c(void *u, uint32_t v) { (void)u; doCOP2((int)v); return 0; }

/* ---- retail execution ---- */
/* Bridge: return 1 when t was serviced (result in v0), 0 for guest code. */
static int (*user_bridge)(PcPortMipsCpu *c, uint32_t t);
/* optional: rewrite retail-only representations (guest code addresses stored as callbacks) into the
 * native representation (host function address low 32 bits) before the memory comparison. */
static void (*retail_norm)(uint8_t *low_retail);
static int h_bridge(void *u, PcPortMipsCpu *c, uint32_t t) { (void)u; return user_bridge ? user_bridge(c, t) : 0; }
static uint32_t retail_v0;
static uint32_t retail_stack_args[8]; static unsigned retail_nstack;
static void retail_call(uint32_t entry, uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3, const char *what)
{
    PcPortMipsBus bus = {.read = h_rd, .write = h_wr, .bridge = h_bridge, .cop2_read = h_c2r, .cop2_write = h_c2w, .cop2_command = h_c2c};
    PcPortMipsCpu cpu; PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = a0; cpu.gpr[5] = a1; cpu.gpr[6] = a2; cpu.gpr[7] = a3;
    cpu.gpr[28] = GP_VALUE; cpu.gpr[29] = SP_TOP; cpu.gpr[31] = HALT_PC;
    memset(g_PsxRam + 0x1ffe00, 0xd3, 0x200);
    for (unsigned i = 0; i < retail_nstack; i++) put32(g_PsxRam + 0x1ff010 + 4 * i, retail_stack_args[i]);
    int rc = PcPortMipsRun(&cpu, entry, HALT_PC, 400000);
    if (rc != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "%s FAIL retail oracle did not halt (rc=%d) pc=%08x: %s\n", what, rc, cpu.pc, cpu.error);
        exit(2);
    }
    retail_v0 = cpu.gpr[2];
}

/* ---- per-function case driver ---- */
typedef struct Fn {
    const char *name;
    uint32_t entry;
    int hasret;
    void (*setup)(uint32_t id);          /* fill native state (low fixture, globals), sets A[] */
    uint32_t (*native)(void);            /* run the port body on A[] */
} Fn;
static uint32_t A[4];
static const char *cur_what = "ORACLE";
static unsigned prints_left = 2;
static void report(const char *fn, unsigned id, const char *why, unsigned off, unsigned nat, unsigned ret)
{
    if (!prints_left) return;
    prints_left--;
    fprintf(stderr, "%s DIVERGE fn=%s case=%u %s at 0x%x native=%x retail=%x\n", cur_what, fn, id, why, off, nat, ret);
    printf("%s DIVERGE fn=%s case=%u %s at 0x%x native=%x retail=%x\n", cur_what, fn, id, why, off, nat, ret);
}

/* returns 0 = match, 1 = diverge */
static int run_case(const Fn *f, uint32_t id, int check_gte)
{
    uint32_t nat_v0;
    unsigned i;
    seed_case(id * 2654435761u + 7);
    memset(low, 0, LOW_SIZE);
    memset(feed, 0, sizeof(feed));
    ntrace = 0;
    f->setup(id);
    memcpy(low_init, low, LOW_SIZE);
    glob_push();
    gte_seed_random(); gte_save(gte_snap_init);
    gte_load(gte_snap_init);
    ntrace = 0;
    retail_call(f->entry, A[0], A[1], A[2], A[3], f->name);
    memcpy(low_retail, low, LOW_SIZE);
    if (retail_norm) retail_norm(low_retail);
    glob_snapshot(); gte_save(gte_snap_retail);
    unsigned nt_retail = ntrace; Ev tr_retail[32]; memcpy(tr_retail, trace, sizeof(trace));
    uint32_t v0_retail = retail_v0;

    memcpy(low, low_init, LOW_SIZE);
    gte_load(gte_snap_init);
    ntrace = 0;
    nat_v0 = f->native();

    for (i = 0; i < LOW_CMP; i++)
        if (low[i] != low_retail[i]) { report(f->name, id, "memory", i, low[i], low_retail[i]); return 1; }
    const char *g = glob_diff();
    if (g) { printf("%s DIVERGE fn=%s case=%u global %s\n", cur_what, f->name, id, g); fprintf(stderr, "%s DIVERGE fn=%s case=%u global %s\n", cur_what, f->name, id, g); return 1; }
    if (f->hasret && nat_v0 != v0_retail) { report(f->name, id, "return", 0, nat_v0, v0_retail); return 1; }
    if (nt_retail != ntrace) { report(f->name, id, "callee-count", 0, ntrace, nt_retail); return 1; }
    for (i = 0; i < ntrace && i < 32; i++)
        if (memcmp(&trace[i], &tr_retail[i], sizeof(Ev))) {
            report(f->name, id, "callee-args", i, trace[i].id, tr_retail[i].id);
            return 1;
        }
    if (check_gte) {
        uint8_t gte_nat[sizeof(gteRegs)]; gte_save(gte_nat);
        if (memcmp(gte_nat, gte_snap_retail, sizeof(gteRegs))) {
            for (i = 0; i < sizeof(gteRegs); i++) if (gte_nat[i] != gte_snap_retail[i]) break;
            report(f->name, id, "GTE-state", i, gte_nat[i], gte_snap_retail[i]);
            return 1;
        }
    }
    return 0;
}
#endif
