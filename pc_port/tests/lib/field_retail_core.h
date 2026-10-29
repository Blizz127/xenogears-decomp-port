/* Shared core for the field retail differentials (hand-written field port
 * functions vs. the pinned disc/field.bin bytes on the port MIPS interpreter).
 * Header-free host code: include after defining the TU configuration:
 *   BUF_BASE/BUF_SIZE (identity-mapped scratch window), FIELD_BASE/FIELD_SIZE,
 *   TESTNAME (printed prefix), regions[]/NREG, ptrglobals[]/NPTR, targets[],
 *   fns[] (Fn table), spy functions (SPY macro), gen hooks.
 * Both sides start from byte-identical guest memory; every touched byte of
 * every window, the return value, callee call order/arguments and callee-saved
 * registers are compared.  No retail bytes are stored in the tree. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>
#include "battle_mips_adapter.h"

#ifndef FEED_MAP
#define FEED_MAP(id, raw) (raw)
#endif
#define STACK 0x801FF000u
#ifndef ORACLE_STEPS
#define ORACLE_STEPS 20000
#endif
#define HALT 0xFFFFFFFCu
#ifndef FIELD_BASE
#define FIELD_BASE 0x8006faf0u
#define FIELD_SIZE 260862u
#endif
#ifndef BUF_BASE
#define BUF_BASE 0x80120000u
#define BUF_SIZE 0x40000u
#endif
#define STR2(x) #x
#define STR(x) STR2(x)
#define REGION(id) uint8_t id[id##_SIZE] __attribute__((aligned(16), used))
#define ALIAS(sym, reg, addr) \
    __asm__(".globl " #sym "\n.set " #sym ", " #reg "+(" STR(addr) "-" STR(reg##_BASE) ")")

typedef struct { uint32_t base, size; uint8_t* nat; } Region;
typedef struct { uint32_t addr; uintptr_t* host; } PtrGlobal;
extern const Region regions[];
extern const unsigned NREG;
extern const PtrGlobal ptrglobals[];
extern const unsigned NPTR;

static uint8_t ram[0x200000], *natbuf;
static uint8_t field_image[FIELD_SIZE];
static unsigned cases, checks;
static const char* cur_name;
static PcPortMipsCpu* active_cpu;
static uint32_t cov_lo, cov_hi;
static uint8_t cov[8192];
static uintptr_t nat_stack_hi;

static void fail(const char* why) {
    fprintf(stderr, TESTNAME " FAIL fn=%s case=%u %s\n", cur_name, cases, why);
    printf(TESTNAME " FAIL fn=%s case=%u %s\n", cur_name, cases, why);
    exit(1);
}
static void require(int ok, const char* why) { checks++; if (!ok) fail(why); }
static uint8_t* gp(uint32_t a) { return ram + (a & 0x1FFFFFu); }
static uint32_t r32(uint32_t a) { uint32_t v; memcpy(&v, gp(a), 4); return v; }
static void w32(uint32_t a, uint32_t v) { memcpy(gp(a), &v, 4); }
static void w16(uint32_t a, uint16_t v) { memcpy(gp(a), &v, 2); }
static __attribute__((unused)) void w8(uint32_t a, uint8_t v) { *gp(a) = v; }

static uint64_t rng_s = 0x9E3779B97F4A7C15ull;
static uint32_t rnd(void) {
    rng_s ^= rng_s << 13; rng_s ^= rng_s >> 7; rng_s ^= rng_s << 17;
    return (uint32_t)(rng_s >> 16);
}
static uint32_t rnd_range(uint32_t n) { return rnd() % n; }
static int32_t rnd_val(void) {
    static const int32_t spec[] = { 0, 1, -1, 2, 3, 0x7F, 0x80, 0xFF, 0x100, 0x7FFF, 0x8000, 0xFFFF,
                                    0x10000, 0x7FFFFFFF, (int32_t)0x80000000, 0x12345, -0x8000 };
    return (rnd() & 1) ? spec[rnd_range(sizeof(spec) / sizeof(spec[0]))] : (int32_t)rnd();
}
static __attribute__((unused)) int32_t small(int32_t lo, int32_t hi) { return lo + (int32_t)rnd_range((uint32_t)(hi - lo + 1)); }

/* ---- boundary spies ----------------------------------------------------- */
typedef struct { uint32_t id, a[6]; } Event;
#ifndef MAXEV
#define MAXEV 64
#endif
static struct { Event ev[MAXEV]; unsigned n; } trace;
static int32_t feed[MAXEV];
static int32_t outfeed[MAXEV][8];
/* outhw/inmask are optional trailing fields: older target tables omit them. */
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
typedef struct { uint32_t addr, id, nargs, ret, outmask, outn, outhw, inmask; } Target;
/* outmask: bit i = arg i is an out pointer (outn words each, from outfeed[k][2*idx..]);
 * outhw (optional): nibble i = halfwords written through out arg i, taken in order
 * from the event's outfeed halfword stream (replaces outn when nonzero);
 * inmask (optional): bit i = arg i points at input data; its first 6-nargs words
 * are recorded in the event after the arguments (before any out writes).
 * Stack-pointer out/in args are recorded as 0xFFFFFFFE (frames differ). */
extern const Target targets[];
extern const unsigned NTARGETS;
static const Target* tinfo(uint32_t id) {
    for (unsigned i = 0; i < NTARGETS; i++) if (targets[i].id == id) return &targets[i];
    return NULL;
}
static int on_native_stack(uintptr_t p) { return p <= nat_stack_hi && p + 0x100000u > nat_stack_hi; }
static long spy_core(uint32_t id, const uint64_t* in) {
    require(trace.n < MAXEV, "spy count");
    unsigned k = trace.n++;
    const Target* t = tinfo(id);
    Event* e = &trace.ev[k];
    e->id = id;
    for (unsigned i = 0; i < 6; i++) e->a[i] = i < t->nargs ? (uint32_t)in[i] : 0;
    for (unsigned i = 0, x = t->nargs; i < 6; i++) {
        if (!(t->inmask & (1u << i))) continue;
        if (on_native_stack((uintptr_t)in[i])) e->a[i] = 0xFFFFFFFEu;
        for (; x < 6; x++) memcpy(&e->a[x], (uint8_t*)(uintptr_t)in[i] + 4 * (x - t->nargs), 4);
    }
    for (unsigned i = 0, idx = 0, hw = 0; i < 6; i++) {
        if (!(t->outmask & (1u << i))) continue;
        uint64_t p = in[i];
        if (on_native_stack((uintptr_t)p)) e->a[i] = 0xFFFFFFFEu;
        if (t->outhw) {
            unsigned n = (t->outhw >> (4 * i)) & 0xF;
            memcpy((uint8_t*)(uintptr_t)p, (uint8_t*)outfeed[k] + 2 * hw, 2 * n); hw += n;
        } else
        for (unsigned w = 0; w < t->outn; w++) { int32_t v = outfeed[k][2 * idx + w]; memcpy((uint8_t*)(uintptr_t)p + 4 * w, &v, 4); }
        idx++;
    }
    return t->ret ? (long)(int32_t)FEED_MAP(id, feed[k]) : 0;
}
#define SPY(name, ID) long name(long a, long b, long c, long d, long e, long f) { \
    uint64_t in[6] = { (uint64_t)a, (uint64_t)b, (uint64_t)c, (uint64_t)d, (uint64_t)e, (uint64_t)f }; \
    return spy_core(ID, in); }

static int read_bus(void* o, uint32_t a, unsigned width, uint32_t* v) {
    (void)o;
    if (a < 0x80000000u || (uint64_t)a + width > 0x80200000u) return -1;
    *v = 0;
    for (unsigned i = 0; i < width; i++) *v |= (uint32_t)gp(a)[i] << (8 * i);
    if (active_cpu && width == 4 && a == active_cpu->pc && a >= cov_lo && a < cov_hi) { uint8_t* c = &cov[(a - cov_lo) / 4]; if (*c != 0xFF) (*c)++; } /* saturate: a u8 wrap reads as uncovered */
    return 0;
}
static int in_window(uint32_t a, unsigned w) {
    if (a >= 0x801FE000u && (uint64_t)a + w <= 0x80200000u) return 1;
    if (a >= BUF_BASE && (uint64_t)a + w <= BUF_BASE + BUF_SIZE) return 1;
    for (unsigned i = 0; i < NREG; i++)
        if (a >= regions[i].base && (uint64_t)a + w <= (uint64_t)regions[i].base + regions[i].size) return 1;
    return 0;
}
static int write_bus(void* o, uint32_t a, unsigned width, uint32_t v) {
    (void)o;
    if (!in_window(a, width)) { fprintf(stderr, "retail stray write %08x w=%u\n", a, width); return -1; }
    for (unsigned i = 0; i < width; i++) gp(a)[i] = (uint8_t)(v >> (8 * i));
    return 0;
}
/* The interpreter consults the bridge at every PC, including the oracle entry
 * point.  A function under test that is also a genuine cross-call target
 * (e.g. func_80086078, called by func_800860F0) would otherwise short-circuit
 * on entry: the first hit at the entry PC is necessarily the entry dispatch,
 * so it is skipped exactly once per oracle run.  Later hits at the same PC
 * are genuine calls (none of the covered bodies are recursive). */
static uint32_t bridge_entry;
static int bridge_entry_armed;
static int bridge(void* o, PcPortMipsCpu* c, uint32_t t) {
    (void)o;
    if (bridge_entry_armed && t == bridge_entry) { bridge_entry_armed = 0; return 0; }
    for (unsigned i = 0; i < NTARGETS; i++) {
        if (targets[i].addr != t) continue;
        uint64_t in[6] = { c->gpr[4], c->gpr[5], c->gpr[6], c->gpr[7], r32(c->gpr[29] + 16), r32(c->gpr[29] + 20) };
        require(trace.n < MAXEV, "spy count");
        unsigned k = trace.n++;
        const Target* tg = &targets[i];
        Event* e = &trace.ev[k];
        e->id = tg->id;
        for (unsigned j = 0; j < 6; j++) e->a[j] = j < tg->nargs ? (uint32_t)in[j] : 0;
        for (unsigned j = 0, x = tg->nargs; j < 6; j++) {
            if (!(tg->inmask & (1u << j))) continue;
            if ((uint32_t)in[j] >= 0x801FE000u) e->a[j] = 0xFFFFFFFEu;
            for (; x < 6; x++) e->a[x] = r32((uint32_t)in[j] + 4 * (x - tg->nargs));
        }
        for (unsigned j = 0, idx = 0, hw = 0; j < 6; j++) {
            if (!(tg->outmask & (1u << j))) continue;
            uint32_t p = (uint32_t)in[j];
            if (p >= 0x801FE000u) e->a[j] = 0xFFFFFFFEu;
            if (tg->outhw) {
                unsigned n = (tg->outhw >> (4 * j)) & 0xF;
                for (unsigned h = 0; h < n; h++, hw++) {
                    uint16_t v; memcpy(&v, (uint8_t*)outfeed[k] + 2 * hw, 2); w16(p + 2 * h, v);
                }
            } else
            for (unsigned w = 0; w < tg->outn; w++) w32(p + 4 * w, (uint32_t)outfeed[k][2 * idx + w]);
            idx++;
        }
        if (tg->ret) c->gpr[2] = (uint32_t)(int32_t)FEED_MAP(tg->id, feed[k]);
        return 1;
    }
    return 0;
}

typedef struct { uint32_t a0, a1, a2, a3, a4; } Args;
typedef struct {
    const char* name; uint32_t entry; unsigned insns;
    uint32_t (*native)(Args); void (*gen)(Args*); unsigned ncases; unsigned hasret;
} Fn;
extern Fn fns[];
extern const unsigned NFNS;

static void feed_fill(void) {
    for (unsigned i = 0; i < MAXEV; i++) { feed[i] = rnd_val(); for (unsigned w = 0; w < 8; w++) outfeed[i][w] = rnd_val(); }
}
static void fill_windows(void) {
    unsigned mode = rnd() & 3;
    uint8_t pat = (uint8_t)rnd();
    uint32_t i;
    for (i = 0; i < BUF_SIZE; i += 4) {
        uint32_t v = mode == 0 ? rnd() : mode == 1 ? 0u : mode == 2 ? 0xFFFFFFFFu : (uint32_t)pat * 0x01010101u;
        if (mode != 0 && (rnd() & 7) == 0) v = rnd();
        w32(BUF_BASE + i, v);
    }
    for (unsigned r = 0; r < NREG; r++)
        for (i = 0; i < regions[r].size; i += 4)
            w32(regions[r].base + i, mode == 0 ? rnd() : mode == 1 ? 0u : mode == 2 ? 0xFFFFFFFFu : (uint32_t)pat * 0x01010101u);
}
static void mirror_in(void) {
    memcpy(natbuf, gp(BUF_BASE), BUF_SIZE);
    for (unsigned r = 0; r < NREG; r++) memcpy(regions[r].nat, gp(regions[r].base), regions[r].size);
    for (unsigned p = 0; p < NPTR; p++) *ptrglobals[p].host = (uintptr_t)r32(ptrglobals[p].addr);
}
static void mirror_out(void) {
    for (unsigned p = 0; p < NPTR; p++) {
        uint32_t v = (uint32_t)*ptrglobals[p].host;
        for (unsigned r = 0; r < NREG; r++)
            if (ptrglobals[p].addr >= regions[r].base && ptrglobals[p].addr + 4 <= regions[r].base + regions[r].size)
                memcpy(regions[r].nat + (ptrglobals[p].addr - regions[r].base), &v, 4);
    }
}

typedef struct { Event ev[MAXEV]; unsigned n; } Trace;

#ifndef NATIVE_PRE
#define NATIVE_PRE() ((void)0)
#define NATIVE_POST() ((void)0)
#endif

static void run_case(Fn* f) {
    Args a = { 0, 0, 0, 0, 0 };
    cur_name = f->name;
    memset(ram, 0, sizeof(ram));
    memcpy(gp(FIELD_BASE), field_image, FIELD_SIZE);
    fill_windows();
    memset(&trace, 0, sizeof(trace));
    feed_fill();
    f->gen(&a);
    static uint8_t start[0x200000];
    memcpy(start, ram, sizeof(start));
    static int32_t feed0[MAXEV], outfeed0[MAXEV][8];
    memcpy(feed0, feed, sizeof(feed)); memcpy(outfeed0, outfeed, sizeof(outfeed));

    trace.n = 0;
    bridge_entry = f->entry; bridge_entry_armed = 1;
    PcPortMipsBus bus = { 0 }; PcPortMipsCpu cpu;
    bus.read = read_bus; bus.write = write_bus; bus.bridge = bridge;
    PcPortMipsCpuInit(&cpu, &bus);
    for (int r = 16; r < 24; r++) cpu.gpr[r] = 0xC0DE0000u + (uint32_t)r;
    cpu.gpr[28] = 0xC0DE001Cu; cpu.gpr[30] = 0xC0DE001Eu;
    cpu.gpr[29] = STACK; cpu.gpr[31] = HALT;
    w32(STACK + 16, a.a4);
    cpu.gpr[4] = a.a0; cpu.gpr[5] = a.a1; cpu.gpr[6] = a.a2; cpu.gpr[7] = a.a3;
    cov_lo = f->entry; cov_hi = f->entry + f->insns * 4;
    active_cpu = &cpu;
    int rc = PcPortMipsRun(&cpu, f->entry, HALT, ORACLE_STEPS);
    active_cpu = NULL;
    if (rc != PC_PORT_MIPS_HALTED) fprintf(stderr, "oracle rc=%d %s\n", rc, cpu.error);
    require(rc == PC_PORT_MIPS_HALTED, "retail run halted");
    require(cpu.gpr[29] == STACK, "retail stack restored");
    for (int r = 16; r < 24; r++) require(cpu.gpr[r] == 0xC0DE0000u + (uint32_t)r, "retail callee-saved reg");
    require(cpu.gpr[28] == 0xC0DE001Cu && cpu.gpr[30] == 0xC0DE001Eu, "retail gp/fp preserved");
    uint32_t retail_v0 = cpu.gpr[2];
    static Trace rt; memcpy(&rt, &trace, sizeof(rt));
    static uint8_t after[0x200000];
    memcpy(after, ram, sizeof(after));

    memcpy(ram, start, sizeof(start));
    memcpy(feed, feed0, sizeof(feed)); memcpy(outfeed, outfeed0, sizeof(outfeed));
    memset(&trace, 0, sizeof(trace));
    mirror_in();
    NATIVE_PRE();
    uint32_t nat_v0 = f->native(a);
    NATIVE_POST();
    mirror_out();
    static Trace nt; memcpy(&nt, &trace, sizeof(nt));

    require(nt.n == rt.n, "spy call count");
    if (memcmp(nt.ev, rt.ev, sizeof(Event) * nt.n) != 0)
        for (unsigned k = 0; k < nt.n; k++)
            if (memcmp(&nt.ev[k], &rt.ev[k], sizeof(Event)) != 0) {
                fprintf(stderr, "event %u: native id=%u a=%08x %08x %08x %08x %08x %08x\n", k, nt.ev[k].id,
                        nt.ev[k].a[0], nt.ev[k].a[1], nt.ev[k].a[2], nt.ev[k].a[3], nt.ev[k].a[4], nt.ev[k].a[5]);
                fprintf(stderr, "event %u: retail id=%u a=%08x %08x %08x %08x %08x %08x\n", k, rt.ev[k].id,
                        rt.ev[k].a[0], rt.ev[k].a[1], rt.ev[k].a[2], rt.ev[k].a[3], rt.ev[k].a[4], rt.ev[k].a[5]);
                break;
            }
    require(memcmp(nt.ev, rt.ev, sizeof(Event) * nt.n) == 0, "spy call trace (order/arguments)");
    if (f->hasret) {
        if (nat_v0 != retail_v0) fprintf(stderr, "ret native=%08x retail=%08x\n", nat_v0, retail_v0);
        require(nat_v0 == retail_v0, "return value");
    }
    if (memcmp(natbuf, after + (BUF_BASE & 0x1FFFFF), BUF_SIZE) != 0) {
        unsigned nd = 0;
        for (uint32_t i = 0; i < BUF_SIZE && nd < 6; i++)
            if (natbuf[i] != after[(BUF_BASE & 0x1FFFFF) + i]) {
                fprintf(stderr, "first diff %08x native=%02x retail=%02x start=%02x\n", BUF_BASE + i,
                        natbuf[i], after[(BUF_BASE & 0x1FFFFF) + i], start[(BUF_BASE & 0x1FFFFF) + i]);
                nd++;
            }
        fail("guest buffer window");
    }
    for (unsigned r = 0; r < NREG; r++) {
        if (memcmp(regions[r].nat, after + (regions[r].base & 0x1FFFFF), regions[r].size) != 0) {
            for (uint32_t i = 0; i < regions[r].size; i++)
                if (regions[r].nat[i] != after[(regions[r].base & 0x1FFFFF) + i]) {
                    fprintf(stderr, "first diff %08x native=%02x retail=%02x\n", regions[r].base + i,
                            regions[r].nat[i], after[(regions[r].base & 0x1FFFFF) + i]);
                    break;
                }
            fail("global region bytes");
        }
    }
    cases++;
}

static void crash_report(int sig) {
    fprintf(stderr, TESTNAME " CRASH signal=%d fn=%s case=%u (%s side)\n", sig, cur_name ? cur_name : "?", cases,
            active_cpu ? "retail" : "native");
    _exit(2);
}
int main(int argc, char** argv) {
    signal(SIGSEGV, crash_report); signal(SIGBUS, crash_report); signal(SIGFPE, crash_report);
    setvbuf(stdout, NULL, _IOLBF, 0);
    int probe; nat_stack_hi = (uintptr_t)&probe + 0x1000;
    require(argc >= 2, "field image argument");
    FILE* fp = fopen(argv[1], "rb"); require(fp != NULL, "open field image");
    require(fread(field_image, 1, FIELD_SIZE, fp) == FIELD_SIZE && fgetc(fp) == EOF, "field image size");
    fclose(fp);
    natbuf = mmap((void*)(uintptr_t)BUF_BASE, BUF_SIZE, PROT_READ | PROT_WRITE,
                  MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
    require(natbuf == (void*)(uintptr_t)BUF_BASE, "identity map of guest buffers");
    const char* only = argc >= 3 ? argv[2] : NULL;
    unsigned total_fn = 0;
    for (unsigned i = 0; i < NFNS; i++) {
        Fn* f = &fns[i];
        if (only && strcmp(only, f->name) != 0) continue;
        memset(cov, 0, sizeof(cov));
        unsigned before = cases;
        for (unsigned c = 0; c < f->ncases; c++) run_case(f);
        unsigned seen = 0, missing = 0;
        for (unsigned k = 0; k < f->insns; k++) { if (cov[k]) seen++; else missing++; }
        printf(TESTNAME " %-18s PASS cases=%u instructions=%u/%u\n", f->name, cases - before, seen, f->insns);
        if (missing) {
            printf("  uncovered:");
            for (unsigned k = 0; k < f->insns; k++) if (!cov[k]) printf(" %08x", f->entry + 4 * k);
            printf("\n");
        }
        total_fn++;
    }
    printf(TESTNAME " PASS functions=%u cases=%u checks=%u\n", total_fn, cases, checks);
    return 0;
}
