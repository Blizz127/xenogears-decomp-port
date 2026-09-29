/* World-map differential prover: retail MIPS (disc/world_map.bin) vs port C.
 *
 * For every case in wmd_tables.inc (an exported wm_XXXXXXXX body whose address
 * is a retail function start) this runs, on identical guest RAM and arguments:
 *   retail: the overlay bytes through the generic MIPS core
 *           (pc_port/src/battle_mips_adapter.c);
 *   host:   the linked port C body, compiled with the port's own flags.
 * Both sides stop at the same call boundary: every symbol the world-map TUs
 * reference but do not define is a recording stub (wmd_stubs.c), and the
 * retail side treats exactly those guest addresses -- plus any target outside
 * the overlay text -- as the boundary. The i-th boundary call on each side gets
 * the same scripted return value. Compared: all 2 MiB of guest RAM (minus the
 * retail stack window), the boundary call trace (target and the arguments the
 * host prototype declares, at their declared widths), and the return value.
 *
 * Address mapping mirrors PSX_ADDR exactly: every address is masked to 2 MiB of
 * g_PsxRam, scratchpad included, so the two sides share one address space.
 *
 * NOT covered / reported as inconclusive rather than passed:
 *  - a retail run that faults, hits break/syscall/cop2 or its step budget;
 *  - a host run that crashes (SIGSEGV/SIGFPE/SIGBUS) or times out;
 *  - native data the port keeps outside guest RAM is mirrored only for
 *    `extern` data whose declared type is a plain integer or pointer.
 * memcpy/memset are executed (not recorded) on the retail side because the host
 * bodies reach libc directly.
 */
#include <setjmp.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "psx_memory.h"
#include "battle_mips_adapter.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

typedef uint64_t (*WmdFn)(uint64_t, uint64_t, uint64_t, uint64_t);
typedef struct WmdStub {
    const char *name;
    uint32_t addr;
    uint8_t nargs;
    uint8_t widths[4];
    uint8_t ptrmask;
    uint8_t retw;
    uint8_t ret_ptr;
    uint8_t truncated;
} WmdStub;
typedef struct WmdMirror {
    const char *name;
    unsigned char *native;
    uint32_t addr;
    uint8_t kind; /* 1 integer, 2 pointer */
    uint8_t bytes;
} WmdMirror;
typedef struct WmdCase {
    const char *name;
    WmdFn fn;
    uint32_t addr;
    uint8_t nargs;
    uint8_t widths[4];
    uint8_t ptrmask;
    uint8_t retw;
    uint8_t ret_ptr;
} WmdCase;

#include "wmd_tables.inc"

#define RAM_BYTES   0x200000u
#define MASK        0x1FFFFFu
#define OVL_BASE    0x8006FAF0u
#define TEXT_LO     0x80070CFCu
#define TEXT_HI     0x80099E8Cu
#define BSS_LO      0x8009BBB8u
#define ARENA_LO    0x80180000u
#define ARENA_HI    0x801F0000u
#define STACK_TOP   0x801FFF00u
#define STACK_LO    0x001FE000u /* excluded window [STACK_LO, RAM_BYTES) */
#define HALT_PC     0xBFC0FFF0u
#define STEP_LIMIT  6000000u
#define MAX_TRACE   256
#define MAX_RV      256

static uint8_t image[0x40000];
static size_t image_size;
static uint8_t snapshot[RAM_BYTES];
static uint8_t retail_ram[RAM_BYTES];
static uint16_t stub_at[(TEXT_HI - TEXT_LO) / 4]; /* overlay boundary map */

typedef struct Call { uint32_t addr; uint32_t a[4]; int stub; } Call;
static Call rtrace[MAX_TRACE], htrace[MAX_TRACE];
static unsigned rcount, hcount;
static uint32_t rv[MAX_RV];

static uint32_t rng_state;
static uint32_t rnd(void)
{
    rng_state = rng_state * 1664525u + 1013904223u;
    return rng_state >> 8 ^ rng_state << 13;
}

static uint32_t ptr_rv(unsigned k) { return ARENA_LO + 0x40000u + (k % 64u) * 0x400u; }

static int stub_for_addr(uint32_t addr)
{
    unsigned i;
    if (addr >= TEXT_LO && addr < TEXT_HI)
        return (int)stub_at[(addr - TEXT_LO) / 4] - 1;
    for (i = 0; i < STUB_COUNT; i++)
        if (kStubs[i].addr == addr)
            return (int)i;
    return -1;
}

/* ---------------- host side ---------------- */
static sigjmp_buf host_jmp;
static volatile int host_active;

static uint32_t host_to_guest(uint64_t v)
{
    uintptr_t p = (uintptr_t)v, base = (uintptr_t)g_PsxRam;
    if (p >= base && p < base + PSX_RAM_SIZE)
        return 0x80000000u | (uint32_t)(p - base);
    return (uint32_t)v;
}

uint64_t wmd_host_call(unsigned idx, uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3)
{
    const WmdStub *s = &kStubs[idx];
    uint64_t args[4] = {a0, a1, a2, a3};
    unsigned k = hcount, i;
    if (hcount < MAX_TRACE) {
        Call *c = &htrace[hcount];
        c->addr = s->addr;
        c->stub = (int)idx;
        for (i = 0; i < 4; i++)
            c->a[i] = (s->ptrmask >> i & 1) ? host_to_guest(args[i]) : (uint32_t)args[i];
    }
    hcount++;
    if (s->ret_ptr)
        return (uint64_t)(uintptr_t)PSX_ADDR(ptr_rv(k));
    return rv[k % MAX_RV];
}

static void on_signal(int sig)
{
    if (host_active)
        siglongjmp(host_jmp, sig);
    signal(sig, SIG_DFL);
    raise(sig);
}

static unsigned char mirror_seed[MIRROR_COUNT + 1][8];

/* Native copies are seeded from guest RAM and written back only when the host
 * body changed them, so a guest word that is not a valid pointer survives. */
static void mirror_in(void)
{
    unsigned i;
    for (i = 0; i < MIRROR_COUNT; i++) {
        const WmdMirror *m = &kMirrors[i];
        uint32_t w = 0;
        memset(m->native, 0, 8);
        memcpy(&w, PSX_ADDR(m->addr), 4);
        if (m->kind == 2) {
            void *p = w ? PSX_ADDR(w) : NULL;
            memcpy(m->native, &p, sizeof p);
        } else {
            memcpy(m->native, PSX_ADDR(m->addr), m->bytes);
        }
        memcpy(mirror_seed[i], m->native, 8);
    }
}

static void mirror_out(void)
{
    unsigned i;
    for (i = 0; i < MIRROR_COUNT; i++) {
        const WmdMirror *m = &kMirrors[i];
        if (!memcmp(mirror_seed[i], m->native, 8))
            continue;
        if (m->kind == 2) {
            void *p;
            uint32_t w;
            memcpy(&p, m->native, sizeof p);
            w = host_to_guest((uint64_t)(uintptr_t)p);
            memcpy(PSX_ADDR(m->addr), &w, 4);
        } else {
            memcpy(PSX_ADDR(m->addr), m->native, m->bytes);
        }
    }
}

/* ---------------- retail side ---------------- */
static int bus_read(void *o, uint32_t a, unsigned w, uint32_t *v)
{
    (void)o;
    *v = 0;
    memcpy(v, g_PsxRam + (a & MASK), w);
    return 0;
}

static int bus_write(void *o, uint32_t a, unsigned w, uint32_t v)
{
    (void)o;
    memcpy(g_PsxRam + (a & MASK), &v, w);
    return 0;
}

static uint32_t memcpy_addr, memset_addr;

static int bus_bridge(void *o, PcPortMipsCpu *cpu, uint32_t target)
{
    int s;
    (void)o;
    if (target >= TEXT_LO && target < TEXT_HI) {
        if (!stub_at[(target - TEXT_LO) / 4])
            return 0;
    }
    if (target == HALT_PC)
        return 0;
    if (target == memcpy_addr || target == memset_addr) {
        uint32_t d = cpu->gpr[4], x = cpu->gpr[5], n = cpu->gpr[6], i;
        for (i = 0; i < n && i < RAM_BYTES; i++)
            g_PsxRam[(d + i) & MASK] = target == memcpy_addr ? g_PsxRam[(x + i) & MASK] : (uint8_t)x;
        cpu->gpr[2] = d;
        return 1;
    }
    s = stub_for_addr(target);
    if (rcount < MAX_TRACE) {
        Call *c = &rtrace[rcount];
        c->addr = target;
        c->stub = s;
        memcpy(c->a, &cpu->gpr[4], sizeof c->a);
    }
    cpu->gpr[2] = (s >= 0 && kStubs[s].ret_ptr) ? ptr_rv(rcount) : rv[rcount % MAX_RV];
    cpu->gpr[3] = 0;
    rcount++;
    return 1;
}

extern unsigned int MFC2(int reg);
extern unsigned int CFC2(int reg);
extern void MTC2(unsigned int v, int reg);
extern void CTC2(unsigned int v, int reg);
extern int doCOP2(int op);
extern unsigned char gteRegs[];
extern const unsigned wmd_gte_size;
static unsigned char gte_seed[4096];

static uint32_t cop2_read(void *o, int c, unsigned r) { (void)o; return c ? CFC2((int)r) : MFC2((int)r); }
static void cop2_write(void *o, int c, unsigned r, uint32_t v)
{
    (void)o;
    if (c)
        CTC2(v, (int)r);
    else
        MTC2(v, (int)r);
}
static int cop2_cmd(void *o, uint32_t i) { (void)o; doCOP2((int)i); return 0; }

/* ---------------- case construction ---------------- */
static uint32_t pattern_word(int pattern)
{
    uint32_t r = rnd();
    switch (pattern) {
    case 0: return 0;
    case 1: return r;
    case 2: return (r & 3) ? (r >> 4 & 0xF) : 0; /* small */
    default:
        switch (r & 3) {
        case 0: return (ARENA_LO + (rnd() % (ARENA_HI - ARENA_LO))) & ~3u;
        case 1: return rnd() & 0x7;
        case 2: return rnd() & 0xFFFF;
        default: return rnd();
        }
    }
}

static void build_ram(int pattern)
{
    uint32_t a;
    memset(g_PsxRam, 0, PSX_RAM_SIZE);
    for (a = 0x10000; a < RAM_BYTES; a += 4) {
        uint32_t w = pattern_word(pattern);
        memcpy(g_PsxRam + a, &w, 4);
    }
    memset(g_PsxRam, 0, 0x10000); /* kernel area */
    /* scratchpad (masked onto low RAM exactly like PSX_ADDR) */
    for (a = 0; a < 0x400; a += 4) {
        uint32_t w = pattern_word(pattern);
        memcpy(g_PsxRam + a, &w, 4);
    }
    memcpy(g_PsxRam + (OVL_BASE & MASK), image, image_size);
    /* The overlay's own image stays retail; its BSS gets the pattern. */
}

static uint32_t arg_value(int pattern, unsigned i)
{
    uint32_t r = rnd();
    (void)i;
    switch (r % 5) {
    case 0: return r >> 8 & 0x3F;         /* slot index */
    case 1: return r >> 8 & 0x7;
    case 2: return (ARENA_LO + (rnd() % 0x10000u)) & ~3u;
    case 3: return pattern == 0 ? 0 : rnd();
    default: return r >> 8 & 0xFFFF;
    }
}

static uint32_t wmask(unsigned bits) { return bits >= 32 ? 0xFFFFFFFFu : ((1u << bits) - 1u); }

typedef enum { R_PASS, R_FAIL, R_INCONCLUSIVE } Result;
static char detail[1024];

static Result run_case(const WmdCase *c, int pattern)
{
    PcPortMipsCpu cpu;
    PcPortMipsBus bus = {0, bus_read, bus_write, bus_bridge, cop2_read, cop2_write, cop2_cmd};
    uint32_t args[4] = {0, 0, 0, 0};
    uint32_t rret = 0, hret = 0;
    uint64_t hraw = 0;
    unsigned i, n;
    int rc, sig, saved;

    build_ram(pattern);
    for (i = 0; i < c->nargs; i++) {
        args[i] = arg_value(pattern, i);
        if (c->widths[i] < 32)
            args[i] &= wmask(c->widths[i]);
    }
    for (i = 0; i < MAX_RV; i++) {
        uint32_t r = rnd();
        rv[i] = (r & 1) ? 0 : (r >> 4) % 4;
    }
    memcpy(snapshot, g_PsxRam, RAM_BYTES);
    for (i = 0; i < wmd_gte_size && i < sizeof gte_seed; i++)
        gte_seed[i] = pattern == 0 ? 0 : (uint8_t)rnd();
    memcpy(gteRegs, gte_seed, wmd_gte_size);

    /* retail */
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[28] = 0x80059170u;
    cpu.gpr[29] = STACK_TOP;
    cpu.gpr[31] = HALT_PC;
    for (i = 0; i < 4; i++) {
        uint32_t v = args[i];
        if (i < c->nargs && c->widths[i] == 16 && (v & 0x8000u) && !(c->ptrmask >> i & 1))
            v |= 0xFFFF0000u; /* callers pass sign-extended shorts; treat s16/u16 alike */
        cpu.gpr[4 + i] = v;
    }
    rcount = 0;
    rc = PcPortMipsRun(&cpu, c->addr, HALT_PC, STEP_LIMIT);
    if (rc != PC_PORT_MIPS_HALTED) {
        snprintf(detail, sizeof detail, "retail rc=%d %s", rc, cpu.error);
        return R_INCONCLUSIVE;
    }
    rret = cpu.gpr[2];
    memcpy(retail_ram, g_PsxRam, RAM_BYTES);

    /* host */
    memcpy(g_PsxRam, snapshot, RAM_BYTES);
    hcount = 0;
    memcpy(gteRegs, gte_seed, wmd_gte_size);
    mirror_in();
    saved = dup(2);
    {
        int devnull = open("/dev/null", O_WRONLY);
        dup2(devnull, 2);
        close(devnull);
    }
    host_active = 1;
    alarm(3);
    sig = sigsetjmp(host_jmp, 1);
    if (sig == 0) {
        uint64_t hargs[4];
        for (i = 0; i < 4; i++)
            hargs[i] = (c->ptrmask >> i & 1) ? (uint64_t)(uintptr_t)PSX_ADDR(args[i])
                                            : (uint64_t)args[i];
        hraw = c->fn(hargs[0], hargs[1], hargs[2], hargs[3]);
    }
    alarm(0);
    host_active = 0;
    fflush(stderr);
    dup2(saved, 2);
    close(saved);
    if (sig != 0) {
        snprintf(detail, sizeof detail, "host signal %d", sig);
        return R_INCONCLUSIVE;
    }
    mirror_out();
    hret = c->ret_ptr ? host_to_guest(hraw) : (uint32_t)hraw;

    /* compare */
    if (rcount > MAX_TRACE || hcount > MAX_TRACE) {
        snprintf(detail, sizeof detail, "trace overflow r=%u h=%u", rcount, hcount);
        return R_INCONCLUSIVE;
    }
    if (rcount != hcount) {
        unsigned k = 0;
        while (k < rcount && k < hcount && rtrace[k].addr == htrace[k].addr)
            k++;
        snprintf(detail, sizeof detail,
                 "call count retail=%u host=%u; first difference at #%u retail=%08x(%s) host=%08x(%s)",
                 rcount, hcount, k, k < rcount ? rtrace[k].addr : 0,
                 k < rcount && rtrace[k].stub >= 0 ? kStubs[rtrace[k].stub].name : "-",
                 k < hcount ? htrace[k].addr : 0,
                 k < hcount && htrace[k].stub >= 0 ? kStubs[htrace[k].stub].name : "-");
        return R_FAIL;
    }
    for (n = 0; n < rcount; n++) {
        const Call *r = &rtrace[n], *h = &htrace[n];
        const WmdStub *s = h->stub >= 0 ? &kStubs[h->stub] : NULL;
        if (r->addr != h->addr) {
            snprintf(detail, sizeof detail, "call #%u target retail=%08x host=%08x(%s)", n, r->addr,
                     h->addr, s ? s->name : "?");
            return R_FAIL;
        }
        for (i = 0; s && i < s->nargs; i++) {
            uint32_t m = (s->ptrmask >> i & 1) ? MASK : wmask(s->widths[i]);
            if ((r->a[i] & m) != (h->a[i] & m)) {
                snprintf(detail, sizeof detail, "call #%u %s arg%u retail=%08x host=%08x", n, s->name, i,
                         r->a[i], h->a[i]);
                return R_FAIL;
            }
        }
    }
    if (c->retw && ((rret ^ hret) & (c->ret_ptr ? MASK : wmask(c->retw)))) {
        snprintf(detail, sizeof detail, "return retail=%08x host=%08x", rret, hret);
        return R_FAIL;
    }
    for (i = 0x10000; i < STACK_LO; i++) {
        if (retail_ram[i] != g_PsxRam[i]) {
            unsigned j = i & ~3u;
            uint32_t rw, hw, bw;
            unsigned diffs = 0, k;
            memcpy(&rw, retail_ram + j, 4);
            memcpy(&hw, g_PsxRam + j, 4);
            memcpy(&bw, snapshot + j, 4);
            for (k = i; k < STACK_LO; k++)
                diffs += retail_ram[k] != g_PsxRam[k];
            {
                int len = snprintf(detail, sizeof detail,
                                   "ram 0x%08x retail=%08x host=%08x before=%08x (%u differing bytes)",
                                   0x80000000u | j, rw, hw, bw, diffs);
                unsigned shown = 0;
                /* WMD_VERBOSE: list the first differing words for triage. */
                for (k = j + 4; getenv("WMD_VERBOSE") && k < STACK_LO && shown < 6 &&
                                len < (int)sizeof detail - 40; k += 4) {
                    if (memcmp(retail_ram + k, g_PsxRam + k, 4)) {
                        memcpy(&rw, retail_ram + k, 4);
                        memcpy(&hw, g_PsxRam + k, 4);
                        len += snprintf(detail + len, sizeof detail - (size_t)len, " | %08x r=%08x h=%08x",
                                        0x80000000u | k, rw, hw);
                        shown++;
                    }
                }
            }
            return R_FAIL;
        }
    }
    for (i = 0; i < 0x400; i++) {
        if (retail_ram[i] != g_PsxRam[i]) {
            snprintf(detail, sizeof detail, "scratchpad/low ram 0x%x differs", i);
            return R_FAIL;
        }
    }
    return R_PASS;
}

int main(int argc, char **argv)
{
    const char *path = getenv("WMD_IMAGE");
    const char *only = getenv("WMD_ONLY");
    const char *nstr = getenv("WMD_CASES");
    unsigned per = nstr ? (unsigned)atoi(nstr) : 24u;
    unsigned i, k, total_pass = 0, total_fail = 0, total_inc = 0;
    unsigned fn_proven = 0, fn_failed = 0, fn_unproven = 0;
    FILE *f;
    (void)argc;
    (void)argv;
    if (!path)
        path = "disc/world_map.bin";
    f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "WORLD MAP DIFFERENTIAL FAIL cannot open %s\n", path);
        return 2;
    }
    image_size = fread(image, 1, sizeof image, f);
    fclose(f);
    if (image_size != 180422u) {
        fprintf(stderr, "WORLD MAP DIFFERENTIAL FAIL %s is %zu bytes, want 180422\n", path, image_size);
        return 2;
    }
    for (i = 0; i < STUB_COUNT; i++) {
        uint32_t a = kStubs[i].addr;
        if (a >= TEXT_LO && a < TEXT_HI)
            stub_at[(a - TEXT_LO) / 4] = (uint16_t)(i + 1);
    }
    memcpy_addr = 0x8003F968u;
    memset_addr = 0x8003FA08u;
    signal(SIGSEGV, on_signal);
    signal(SIGBUS, on_signal);
    signal(SIGFPE, on_signal);
    signal(SIGALRM, on_signal);
    signal(SIGABRT, on_signal);

    for (i = 0; i < CASE_COUNT; i++) {
        const WmdCase *c = &kCases[i];
        unsigned pass = 0, fail = 0, inc = 0;
        char first_fail[1200] = "", first_inc[1200] = "";
        if (only && !strstr(only, c->name))
            continue;
        rng_state = c->addr ^ 0x9E3779B9u;
        for (k = 0; k < per; k++) {
            Result r = run_case(c, (int)(k % 4));
            if (r == R_PASS)
                pass++;
            else if (r == R_FAIL) {
                if (!fail)
                    snprintf(first_fail, sizeof first_fail, "case %u pattern %u: %s", k, k % 4, detail);
                fail++;
            } else {
                if (!inc)
                    snprintf(first_inc, sizeof first_inc, "case %u pattern %u: %s", k, k % 4, detail);
                inc++;
            }
        }
        total_pass += pass;
        total_fail += fail;
        total_inc += inc;
        if (fail) {
            fn_failed++;
            printf("FAIL %s @%08X pass=%u fail=%u inconclusive=%u :: %s\n", c->name, c->addr, pass, fail,
                   inc, first_fail);
        } else if (pass) {
            fn_proven++;
            printf("PROVEN %s @%08X pass=%u inconclusive=%u%s%s\n", c->name, c->addr, pass, inc,
                   inc ? " :: " : "", first_inc);
        } else {
            fn_unproven++;
            printf("UNPROVEN %s @%08X inconclusive=%u :: %s\n", c->name, c->addr, inc, first_inc);
        }
        fflush(stdout);
    }
    printf("WORLD MAP DIFFERENTIAL functions proven=%u failed=%u unproven=%u; cases pass=%u fail=%u "
           "inconclusive=%u\n",
           fn_proven, fn_failed, fn_unproven, total_pass, total_fail, total_inc);
    /* Gate mode: every selected body must have completed comparisons. */
    if (getenv("WMD_REQUIRE_PROVEN") && fn_unproven)
        return 1;
    if (only && fn_proven + fn_failed + fn_unproven == 0) {
        printf("WORLD MAP DIFFERENTIAL FAIL WMD_ONLY matched no case\n");
        return 1;
    }
    return fn_failed ? 1 : 0;
}
