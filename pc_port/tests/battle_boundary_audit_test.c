/* Native-only boundary audit (test only; claude/h-boundary-audit).
 *
 * Since 07901ea0 (F1) runtime_bridge_call binds a MAIN-EXECUTABLE target to
 * its legacy native C body (generic resolution, translate_argument) when a
 * caller-host hook declines it (returns 0).  Native-only builds enable the
 * caller hosts, so call edges the integrator recorded as "refuse before
 * legacy C" in runtime_bridge_caller_hosts now run legacy C:
 *   800223B0 from 0x801E68F8, 80021FE0 from 0x801E6904 (startup actors),
 *   80022A70 from 0x801E6620, 8002F2E0 / 8002FAE8 from 0x8002C86C (packet-
 *   render callbacks), 8003101C from 0x8002C744.
 *
 * For a target this runs, on identical guest state:
 *  - RETAIL: the target's bytes from disc/SLUS_006.64 (loaded into guest RAM
 *    at its PS-X EXE load address), interpreted end to end -- every main-exe
 *    pc of the call tree -- except a target's delegated services (80022A70:
 *    HeapAlloc, HeapFree, LoadImage), which retail reaches through the
 *    runtime bridge and the legacy body calls directly: the same native code;
 *  - NATIVE: the production path, runtime_bridge(&rt, cpu, target) with the
 *    retail return address in $ra and the caller hosts enabled
 *    (XENO_BATTLE_NATIVE_HOSTS=1 selects the caller hosts XENO_NATIVE_ONLY
 *    compiles in). A hook's BRIDGE_REFUSE now stops in both builds; it does
 *    not bind legacy C. Converted targets return 1 before that mapping, and
 *    the audited bodies themselves have no XENO_NATIVE_ONLY code.
 * Compared: guest RAM outside the interpreter's stack window, v0 at the
 * declared width, every runtime-bound main-exe global, the GTE registers,
 * and the LoadImage call log (rect and data address).  A differing word is
 * classified against its value at the call's entry; aligned words where
 * retail stored a guest-shaped address and the native body a non-guest one
 * are UNSAFE-POINTER-STOREs.  A native fault (a guest word followed as a
 * host pointer) is caught per call and reported as HOST-FAULT, with the
 * faulting function and the address it touched.
 *
 * A report, not a gate: the exit status is 0 whatever the verdicts (only a
 * set-up failure stops it), except for the targets BATTLE_BOUNDARY_AUDIT_REQUIRE
 * names (converted bodies): each must come out EQUIVALENT with every class
 * in every regime and decline off its call site, or the status is 1.
 *
 * usage: battle_boundary_audit_test disc/SLUS_006.64 [target-name ...] */
#include <dlfcn.h>
#include <link.h>
#include <setjmp.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "psyq/libgpu.h"
#include "psyq/libgte.h"

#ifndef BATTLE_RUNTIME_SOURCE
#define BATTLE_RUNTIME_SOURCE "../src/battle_mips_runtime.c"
#endif
#include BATTLE_RUNTIME_SOURCE

uint8_t g_PsxRam[PSX_RAM_SIZE + 0x1000];
uint8_t g_PsxScratchpad[4096];
int g_GPUDisabledState;
unsigned g_PcPortPresentedFrames;
void ControllerPushState(void) {}
void ControllerPoll(void) {}
void PcPort_ServiceVblank(void) {}
void PcPort_PadVblankPump(void) {}
void PsyX_UpdateInput(void) {}
void PcPort_GodModeBeforeGuest(PcPortMipsCpu *cpu, uint32_t target);
void PcPort_GodModeBeforeGuest(PcPortMipsCpu *cpu, uint32_t target)
{
    (void)cpu;
    (void)target;
}
int PsyX_Sys_GetVBlankCount(void) { return 0; }
char PsyX_BeginScene(void) { return 1; }
void ClearSplits(void) { abort(); }
void DrawAllSplits(void) { abort(); }
void ParsePrimitivesLinkedList(u_long *p, int s) { (void)p; (void)s; abort(); }

/* The interpreter's own frame lives here (RAM offsets, as in the overlay
 * prover); a native body keeps its frame on the host stack. */
#define STACK_SKIP_LO 0x1FF000u
#define STACK_SKIP_HI 0x200000u
#define PROVER_GTE_BYTES 0x180u
extern uint8_t gteRegs[PROVER_GTE_BYTES];
extern char __executable_start[], _end[];

/* ---- placeholders and spies (the runner generates them for symbols the
 * link lacks; a case that reaches a placeholder is inconclusive) ---------- */
unsigned g_prover_placeholder_hits;
#define SPY_MAX 256u
typedef struct {
    const char *name;
    unsigned argc;
    uint32_t args[12];
} SpyCall;
static SpyCall spy_log[SPY_MAX], spy_retail[SPY_MAX];
static unsigned spy_count, spy_retail_count, spy_overflow;
static BattleMipsRuntime rt;

/* Canonical form: a host pointer into guest RAM / the scratchpad / a bound
 * main-exe global becomes its guest address; anything else its low word. */
static uint32_t spy_canonical(unsigned long long raw)
{
    uintptr_t p = (uintptr_t)raw, ram = (uintptr_t)g_PsxRam;
    uintptr_t pad = (uintptr_t)g_PsxScratchpad;
    size_t d;
    if (p >= ram && p < ram + PSX_RAM_SIZE)
        return 0x80000000u | (uint32_t)(p - ram);
    if (p >= pad && p < pad + 0x1000u)
        return 0x1F800000u | (uint32_t)(p - pad);
    for (d = 0; d < rt.data_count; d++) {
        uintptr_t h = (uintptr_t)rt.data[d].host;
        if (h != 0 && p >= h && p - h < rt.data[d].size)
            return rt.data[d].address + (uint32_t)(p - h);
    }
    return (uint32_t)raw;
}

unsigned long long prover_spy(const char *name, unsigned argc,
                              const unsigned long long *args, unsigned ret);
unsigned long long prover_spy(const char *name, unsigned argc,
                              const unsigned long long *args, unsigned ret)
{
    unsigned i;
    if (spy_count >= SPY_MAX) {
        spy_overflow = 1;
        return ret;
    }
    memset(&spy_log[spy_count], 0, sizeof(SpyCall));
    spy_log[spy_count].name = name;
    spy_log[spy_count].argc = argc;
    for (i = 0; i < 12 && i < argc; i++)
        spy_log[spy_count].args[i] = spy_canonical(args[i]);
    spy_count++;
    return ret;
}

/* LoadImage (PsyCross's is not linked): the rectangle as read at call time
 * and the canonical data pointer, so a host stack RECT matches a guest one. */
int LoadImage(RECT16 *rect, u_long *p);
int LoadImage(RECT16 *rect, u_long *p)
{
    unsigned long long a[5];
    a[0] = (unsigned long long)(uint16_t)rect->x;
    a[1] = (unsigned long long)(uint16_t)rect->y;
    a[2] = (unsigned long long)(uint16_t)rect->w;
    a[3] = (unsigned long long)(uint16_t)rect->h;
    a[4] = (unsigned long long)(uintptr_t)p;
    (void)prover_spy("LoadImage", 5, a, 0);
    return 0;
}

/* ---- the retail image ------------------------------------------------------ */
static uint8_t *slus;
static size_t slus_size;
static uint32_t slus_taddr, slus_tsize;

static uint32_t le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static void load_slus(const char *path)
{
    FILE *f = fopen(path, "rb");
    long n;
    if (f == NULL) {
        perror(path);
        exit(2);
    }
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, 0, SEEK_SET);
    slus = malloc((size_t)n);
    if (slus == NULL || fread(slus, 1, (size_t)n, f) != (size_t)n) {
        fprintf(stderr, "AUDIT FAIL cannot read %s\n", path);
        exit(2);
    }
    fclose(f);
    slus_size = (size_t)n;
    slus_taddr = le32(slus + 0x18);
    slus_tsize = le32(slus + 0x1C);
    if (memcmp(slus, "PS-X EXE", 8) != 0 || slus_tsize + 0x800u > slus_size) {
        fprintf(stderr, "AUDIT FAIL %s is not the expected PS-X EXE\n", path);
        exit(2);
    }
}

/* ---- the two sides ---------------------------------------------------------- */
static PcPortMipsCpu cpu;
static int (*rt_bridge)(void *, PcPortMipsCpu *, uint32_t);
static const uint32_t *delegated;
static unsigned delegated_count;

static int retail_bridge(void *opaque, PcPortMipsCpu *c, uint32_t target)
{
    unsigned i;
    if (target >= slus_taddr && target < slus_taddr + slus_tsize) {
        for (i = 0; i < delegated_count; i++)
            if (delegated[i] == target)
                return rt_bridge(opaque, c, target);
        return 0;  /* interpret the retail main-exe bytes */
    }
    return rt_bridge(opaque, c, target);
}

/* Retail oracle bus: a KUSEG word below 2 MiB is physical RAM on the
 * hardware (retail's packet code masks packet pointers to their 24-bit DMA
 * identity and reads back through them); the runtime bus, which never runs
 * main-executable code in production, refuses it.  The oracle aliases it to
 * KSEG0; everything else goes to the runtime bus unchanged. */
static int (*rt_read)(void *, uint32_t, unsigned, uint32_t *);
static int (*rt_write)(void *, uint32_t, unsigned, uint32_t);
static int retail_read(void *opaque, uint32_t address, unsigned width, uint32_t *value)
{
    if (address < 0x00200000u)
        address |= 0x80000000u;
    return rt_read(opaque, address, width, value);
}
static int retail_write(void *opaque, uint32_t address, unsigned width, uint32_t value)
{
    if (address < 0x00200000u)
        address |= 0x80000000u;
    return rt_write(opaque, address, width, value);
}

#define AUDIT_STEP_LIMIT 2000000u
/* The main executable's $gp (gp_value in config/battle.yaml): its sdata globals
 * (D_800592F0 ...) are addressed relative to it. */
#define AUDIT_MAIN_GP 0x80059170u
static int run_retail(uint32_t target, const uint32_t *args)
{
    unsigned i;
    memset(&cpu, 0, sizeof(cpu));
    initialize_cpu(&cpu, &rt);
    rt_bridge = cpu.bus.bridge;
    cpu.bus.bridge = retail_bridge;
    rt_read = cpu.bus.read;
    rt_write = cpu.bus.write;
    cpu.bus.read = retail_read;
    cpu.bus.write = retail_write;
    for (i = 0; i < 4; i++)
        cpu.gpr[4 + i] = args[i];
    cpu.gpr[28] = AUDIT_MAIN_GP;  /* sdata globals are $gp-relative */
    rt.force_interpret = 1;
    i = (unsigned)PcPortMipsRun(&cpu, target, BATTLE_HALT_PC, AUDIT_STEP_LIMIT);
    rt.force_interpret = 0;
    return (int)i;
}

static sigjmp_buf fault_jmp;
static volatile sig_atomic_t fault_armed;
static volatile sig_atomic_t fault_sig;
static void *volatile fault_addr;  /* si_addr: the address the body touched */
static void *volatile fault_ip;    /* the faulting instruction */

static void fault_handler(int sig, siginfo_t *si, void *uc)
{
    if (fault_armed) {
        fault_armed = 0;
        fault_sig = sig;
        fault_addr = si->si_addr;
#if defined(__x86_64__)
        fault_ip = (void *)((ucontext_t *)uc)->uc_mcontext.gregs[REG_RIP];
#else
        (void)uc;
        fault_ip = NULL;
#endif
        siglongjmp(fault_jmp, 1);
    }
    signal(sig, SIG_DFL);
    raise(sig);
}

/* "func_X+0xNN" for the faulting instruction (the link is -rdynamic). */
static void describe_ip(void *ip, char *out, size_t size)
{
    Dl_info info;
    if (ip != NULL && dladdr(ip, &info) != 0 && info.dli_sname != NULL)
        snprintf(out, size, "%s+0x%lx", info.dli_sname,
                 (unsigned long)((uintptr_t)ip - (uintptr_t)info.dli_saddr));
    else
        snprintf(out, size, "%p", ip);
}

/* Instructions the interpreter executed for the native side
 * (XENO_CPU_EXECUTION_STATS counts them; this binary has the interpreter, a
 * native-only one has not, so any is a path native-only would refuse).
 * Bridged entries (a host reached through PcPortMipsRun) execute none. */
static uint64_t native_interp_insns;
static PcPortMipsExecutionStats native_stats_before;

/* The production path; -2 when the native body faulted. */
static int run_native(uint32_t target, const uint32_t *args, uint32_t ra)
{
    unsigned i;
    int rc;
    memset(&cpu, 0, sizeof(cpu));
    initialize_cpu(&cpu, &rt);
    for (i = 0; i < 4; i++)
        cpu.gpr[4 + i] = args[i];
    cpu.gpr[28] = AUDIT_MAIN_GP;
    cpu.gpr[31] = ra;
    rt.force_interpret = 0;
    fault_sig = 0;
    if (sigsetjmp(fault_jmp, 1) != 0)
        return -2;
    native_stats_before = PcPortMipsExecutionStatsGet();
    fault_armed = 1;
    rc = runtime_bridge(&rt, &cpu, target);
    fault_armed = 0;
    native_interp_insns = PcPortMipsExecutionStatsGet().interpreted_instructions -
                          native_stats_before.interpreted_instructions;
    return rc;
}

/* ---- snapshots -------------------------------------------------------------- */
/* *_before: the state before the set-up hook (restored for the native side);
 * *_entry: the state at the call's entry, after the hook, which classifies a
 * differing word as missing or extra. */
static uint8_t ram_before[PSX_RAM_SIZE], ram_entry[PSX_RAM_SIZE], ram_retail[PSX_RAM_SIZE];
static uint8_t pad_before[4096], pad_retail[4096];
static uint8_t gte_before[PROVER_GTE_BYTES], gte_retail[PROVER_GTE_BYTES];
static uint8_t *nat_before, *nat_entry, *nat_retail;
static size_t nat_bytes;

static int nat_skip(const ResolvedData *d)
{
    uintptr_t h = (uintptr_t)d->host, ram = (uintptr_t)g_PsxRam;
    return d->host == NULL || d->size == 0 ||
           (h < ram + PSX_RAM_SIZE + 0x1000u && h + d->size > ram);
}

static void nat_copy(uint8_t *buf, int save)
{
    size_t d, off = 0;
    for (d = 0; d < rt.data_count; d++) {
        const ResolvedData *r = &rt.data[d];
        if (nat_skip(r))
            continue;
        if (save)
            memcpy(buf + off, r->host, r->size);
        else if (memcmp(r->host, buf + off, r->size) != 0)
            memcpy(r->host, buf + off, r->size);
        off += r->size;
    }
}

static void nat_init(void)
{
    size_t d;
    for (d = 0; d < rt.data_count; d++)
        if (!nat_skip(&rt.data[d]))
            nat_bytes += rt.data[d].size;
    nat_before = malloc(nat_bytes + 1);
    nat_entry = malloc(nat_bytes + 1);
    nat_retail = malloc(nat_bytes + 1);
    if (nat_before == NULL || nat_entry == NULL || nat_retail == NULL)
        abort();
}

/* Bindings wider than the native object they land on: the runtime sizes a
 * binding by the explicit symbol size or the distance to the next guest
 * symbol, which can run past the host object into its link neighbours.
 * dladdr1(RTLD_DL_SYMENT) gives the exported ELF symbol containing the host
 * address (the link is -rdynamic) and its st_size.  Report only. */
static void binding_size_report(void)
{
    size_t d;
    unsigned checked = 0, flagged = 0, unknown = 0;
    for (d = 0; d < rt.data_count; d++) {
        const ResolvedData *r = &rt.data[d];
        const ElfW(Sym) *sym = NULL;
        Dl_info info;
        uintptr_t start, end;
        if (nat_skip(r))
            continue;
        if (dladdr1(r->host, &info, (void **)&sym, RTLD_DL_SYMENT) == 0 || sym == NULL ||
            info.dli_saddr == NULL) {
            unknown++;
            continue;
        }
        start = (uintptr_t)info.dli_saddr;
        end = start + sym->st_size;
        if ((uintptr_t)r->host >= end) {   /* not inside an exported object */
            unknown++;
            continue;
        }
        checked++;
        if ((uintptr_t)r->host + r->size > end) {
            flagged++;
            printf("AUDIT binding-size %08x %s: binding %zu bytes, but %s is %zu bytes (binding at +0x%zx)\n",
                   r->address, r->name ? r->name : "?", (size_t)r->size, info.dli_sname,
                   (size_t)sym->st_size, (size_t)((uintptr_t)r->host - start));
        }
    }
    printf("AUDIT binding-size checked=%u flagged=%u not-exported=%u\n", checked, flagged, unknown);
}

/* ---- one case --------------------------------------------------------------- */
typedef struct {
    const char *name;      /* func_XXXXXXXX */
    uint32_t address;
    uint32_t ra;           /* the boundary's return address */
    unsigned ret_bits;     /* 0 = void */
    unsigned flags;        /* AUDIT_GPRS */
} Target;
/* Compare the whole register file at return (a converted body hands every
 * GPR back to its caller), with a pending delay-slot load applied, and check
 * the native side's pending-load shape against what the packet-render
 * caller accepts (reg 23, mask 0). */
#define AUDIT_GPRS 1u

/* Per-target evidence.  Each differing category of a case is counted once
 * with its first example:
 *   ram-missing  a word retail wrote and the native body left as it was;
 *   ram-extra    a word only the native body wrote;
 *   ram-value    both wrote, different values;
 *   ram-ptr      both wrote, retail a guest address, native a non-guest word
 *                (an UNSAFE-POINTER-STORE);
 *   nat / nat-ptr  the same for a runtime-bound main-exe global (8-byte host
 *                pointers in a 4-byte guest slot show as nat-ptr);
 *   v0, pad, gte, spy  return, scratchpad, GTE registers, delegated calls;
 *   gpr          the register file (AUDIT_GPRS targets). */
enum { C_RAM_MISSING, C_RAM_EXTRA, C_RAM_VALUE, C_RAM_PTR, C_NAT, C_NAT_PTR,
       C_V0, C_PAD, C_GTE, C_SPY, C_GPR, C_COUNT };
static const char *const kCategory[C_COUNT] = {
    "ram-missing", "ram-extra", "ram-value", "ram-ptr", "nat", "nat-ptr",
    "v0", "pad", "gte", "spy", "gpr" };
typedef struct {
    unsigned cases, proven, inconclusive, faults, bridge, interp;
    unsigned cat[C_COUNT];
    char example[C_COUNT][160];
    char fault[224];
} Tally;

static int host_shaped(uint32_t w)
{
    uintptr_t ram = (uintptr_t)g_PsxRam;
    return ((uintptr_t)w >= (uintptr_t)__executable_start && (uintptr_t)w < (uintptr_t)_end) ||
           ((uintptr_t)w >= ram && (uintptr_t)w < ram + PSX_RAM_SIZE);
}

static int guest_shaped_word(uint32_t w)
{
    return (w & 0xFF800000u) == 0x80000000u || (w & 0xFF800000u) == 0xA0000000u;
}

/* Optional per-side set-up (run on each side's restored state). */
static void (*pre_side)(void);
static int audit_verbose;

static void hit(Tally *t, unsigned *seen, unsigned c, const char *detail)
{
    if (!(*seen & (1u << c))) {
        *seen |= 1u << c;
        t->cat[c]++;
        if (t->example[c][0] == '\0')
            snprintf(t->example[c], sizeof(t->example[c]), "%s", detail);
    }
}

static PcPortMipsCpu cpu_retail;

/* Register r at return, with a pending load (the delay slot of the return)
 * committed the way the interpreter commits it. */
static uint32_t gpr_after(const PcPortMipsCpu *c, unsigned r)
{
    if (c->load_valid && c->load_reg == r)
        return (c->gpr[r] & c->load_mask) | c->load_value;
    return c->gpr[r];
}

/* Runs one case on the guest state the caller prepared (RAM, scratchpad,
 * natives, GTE).  Returns 1 when proven equal. */
static int audit_case(const Target *tg, const uint32_t *args, Tally *t)
{
    char detail[200];
    uint32_t rv0, nv0, mask;
    unsigned seen = 0, printed = 0;
    int irc, nrc;
    size_t a;

    t->cases++;
    memcpy(ram_before, g_PsxRam, PSX_RAM_SIZE);
    memcpy(pad_before, g_PsxScratchpad, sizeof(pad_before));
    memcpy(gte_before, gteRegs, PROVER_GTE_BYTES);
    nat_copy(nat_before, 1);

    if (pre_side != NULL)
        pre_side();
    memcpy(ram_entry, g_PsxRam, PSX_RAM_SIZE);
    nat_copy(nat_entry, 1);
    g_prover_placeholder_hits = 0;
    spy_count = 0;
    spy_overflow = 0;
    irc = run_retail(tg->address, args);
    if (irc != PC_PORT_MIPS_HALTED || g_prover_placeholder_hits != 0 || spy_overflow) {
        t->inconclusive++;
        if (t->inconclusive == 1)
            fprintf(stderr, "[audit] %s inconclusive (retail rc=%d placeholders=%u): %s\n",
                    tg->name, irc, g_prover_placeholder_hits, cpu.error);
        return 0;
    }
    rv0 = cpu.gpr[2];
    cpu_retail = cpu;
    memcpy(ram_retail, g_PsxRam, PSX_RAM_SIZE);
    memcpy(pad_retail, g_PsxScratchpad, sizeof(pad_retail));
    memcpy(gte_retail, gteRegs, PROVER_GTE_BYTES);
    nat_copy(nat_retail, 1);
    memcpy(spy_retail, spy_log, sizeof(SpyCall) * spy_count);
    spy_retail_count = spy_count;

    memcpy(g_PsxRam, ram_before, PSX_RAM_SIZE);
    memcpy(g_PsxScratchpad, pad_before, sizeof(pad_before));
    memcpy(gteRegs, gte_before, PROVER_GTE_BYTES);
    nat_copy(nat_before, 0);
    if (pre_side != NULL)
        pre_side();
    g_prover_placeholder_hits = 0;
    spy_count = 0;
    spy_overflow = 0;
    nrc = run_native(tg->address, args, tg->ra);
    if (nrc == -2) {
        t->faults++;
        if (t->fault[0] == '\0') {
            char where[96];
            describe_ip(fault_ip, where, sizeof(where));
            snprintf(t->fault, sizeof(t->fault),
                     "native body took signal %d at %s (ip %p) touching %p (a0=%08x a1=%08x)",
                     (int)fault_sig, where, fault_ip, fault_addr, args[0], args[1]);
        }
        return 0;
    }
    if (native_interp_insns != 0) {
        t->interp++;
        if (t->interp == 1)
            fprintf(stderr, "[audit] %s: the native side interpreted %llu instructions\n",
                    tg->name, (unsigned long long)native_interp_insns);
        return 0;
    }
    if (nrc != 1) {
        t->bridge++;
        if (t->bridge == 1)
            fprintf(stderr, "[audit] %s: runtime_bridge returned %d, not native handling\n",
                    tg->name, nrc);
        return 0;
    }
    if (g_prover_placeholder_hits != 0 || spy_overflow) {
        t->inconclusive++;
        return 0;
    }
    nv0 = cpu.gpr[2];
    mask = tg->ret_bits == 32 ? 0xFFFFFFFFu : tg->ret_bits == 16 ? 0xFFFFu :
           tg->ret_bits == 8 ? 0xFFu : 0u;

    for (a = 0; a + 4 <= PSX_RAM_SIZE; a += 4) {
        uint32_t r, h, b;
        unsigned c;
        if (a >= STACK_SKIP_LO && a < STACK_SKIP_HI)
            continue;
        if (memcmp(ram_retail + a, g_PsxRam + a, 4) == 0)
            continue;
        r = le32(ram_retail + a);
        h = le32(g_PsxRam + a);
        b = le32(ram_entry + a);
        if (h == b)
            c = C_RAM_MISSING;
        else if (r == b)
            c = C_RAM_EXTRA;
        else if (guest_shaped_word(r) && !guest_shaped_word(h))
            c = C_RAM_PTR;
        else
            c = C_RAM_VALUE;
        snprintf(detail, sizeof(detail), "guest %08x entry=%08x retail=%08x native=%08x%s",
                 (unsigned)(0x80000000u | a), b, r, h, host_shaped(h) ? " (native host address)" : "");
        hit(t, &seen, c, detail);
        if (audit_verbose && printed < 12 && t->cases - t->proven - t->inconclusive <= 1) {
            printf("AUDIT   %s %s %s\n", tg->name, kCategory[c], detail);
            printed++;
        }
    }
    if ((rv0 & mask) != (nv0 & mask)) {
        snprintf(detail, sizeof(detail), "retail=%08x native=%08x", rv0, nv0);
        hit(t, &seen, C_V0, detail);
    }
    if (memcmp(pad_retail, g_PsxScratchpad, sizeof(pad_retail)) != 0)
        hit(t, &seen, C_PAD, "scratchpad differs");
    {
        size_t d, off = 0, i;
        for (d = 0; d < rt.data_count; d++) {
            const ResolvedData *r = &rt.data[d];
            if (nat_skip(r))
                continue;
            for (i = 0; i + 4 <= r->size || (i < r->size && i == 0); i += 4) {
                size_t n = r->size - i < 4 ? r->size - i : 4;
                uint32_t rw = 0, hw = 0, bw = 0;
                if (memcmp(nat_retail + off + i, r->host + i, n) == 0)
                    continue;
                memcpy(&rw, nat_retail + off + i, n);
                memcpy(&hw, r->host + i, n);
                memcpy(&bw, nat_entry + off + i, n);
                snprintf(detail, sizeof(detail), "%s+0x%zx entry=%08x retail=%08x native=%08x",
                         r->name ? r->name : "?", i, bw, rw, hw);
                hit(t, &seen, (hw != bw && guest_shaped_word(rw) && !guest_shaped_word(hw)) ?
                              C_NAT_PTR : C_NAT, detail);
                if (audit_verbose && printed < 16 && t->cases - t->proven - t->inconclusive <= 1) {
                    printf("AUDIT   %s bound %s\n", tg->name, detail);
                    printed++;
                }
            }
            off += r->size;
        }
    }
    if (memcmp(gte_retail, gteRegs, PROVER_GTE_BYTES) != 0)
        hit(t, &seen, C_GTE, "GTE register file differs");
    if (spy_count != spy_retail_count ||
        memcmp(spy_log, spy_retail, sizeof(SpyCall) * spy_count) != 0) {
        unsigned k = 0;
        while (k < spy_count && k < spy_retail_count &&
               memcmp(&spy_log[k], &spy_retail[k], sizeof(SpyCall)) == 0)
            k++;
        snprintf(detail, sizeof(detail), "call #%u: retail %s(%08x,%08x,%08x,%08x,%08x) native %s(%08x,%08x,%08x,%08x,%08x) (retail %u calls, native %u)",
                 k,
                 k < spy_retail_count ? spy_retail[k].name : "-",
                 k < spy_retail_count ? spy_retail[k].args[0] : 0u, k < spy_retail_count ? spy_retail[k].args[1] : 0u,
                 k < spy_retail_count ? spy_retail[k].args[2] : 0u, k < spy_retail_count ? spy_retail[k].args[3] : 0u,
                 k < spy_retail_count ? spy_retail[k].args[4] : 0u,
                 k < spy_count ? spy_log[k].name : "-",
                 k < spy_count ? spy_log[k].args[0] : 0u, k < spy_count ? spy_log[k].args[1] : 0u,
                 k < spy_count ? spy_log[k].args[2] : 0u, k < spy_count ? spy_log[k].args[3] : 0u,
                 k < spy_count ? spy_log[k].args[4] : 0u,
                 spy_retail_count, spy_count);
        hit(t, &seen, C_SPY, detail);
    }
    if (tg->flags & AUDIT_GPRS) {
        unsigned r;
        for (r = 1; r < 32; r++) {
            uint32_t rv = gpr_after(&cpu_retail, r), nv = gpr_after(&cpu, r);
            if (r == 31 || rv == nv)   /* $ra: the halt address vs the real one */
                continue;
            snprintf(detail, sizeof(detail), "register %u retail=%08x native=%08x", r, rv, nv);
            hit(t, &seen, C_GPR, detail);
            break;
        }
        if (cpu_retail.hi != cpu.hi || cpu_retail.lo != cpu.lo)
            hit(t, &seen, C_GPR, "hi/lo differ");
        if (cpu.load_valid && (cpu.load_reg != 23 || cpu.load_mask != 0)) {
            snprintf(detail, sizeof(detail),
                     "native pending load reg %u mask %08x (the packet caller accepts reg 23, mask 0)",
                     cpu.load_reg, cpu.load_mask);
            hit(t, &seen, C_GPR, detail);
        }
    }
    if (seen != 0)
        return 0;
    t->proven++;
    return 1;
}

static int report(const Target *tg, const Tally *t, unsigned seen, unsigned want)
{
    const char *verdict;
    unsigned c, other = 0;
    for (c = 0; c < C_COUNT; c++)
        if (c != C_RAM_PTR && c != C_NAT_PTR)
            other += t->cat[c];
    if (t->faults != 0)
        verdict = "DIVERGES (HOST-FAULT)";
    else if (t->interp != 0)
        verdict = "DIVERGES (INTERPRETER-ENTRY)";
    else if (t->cat[C_RAM_PTR] + t->cat[C_NAT_PTR] != 0)
        verdict = "UNSAFE-POINTER-STORE";
    else if (other != 0 || t->bridge != 0)
        verdict = "DIVERGES";
    else if (t->proven != 0 && (seen & want) == want)
        verdict = "EQUIVALENT";
    else
        verdict = "UNPROVEN";
    printf("AUDIT %s from ra %08x: %s cases=%u proven=%u inconclusive=%u host-faults=%u "
           "not-native=%u interpreted=%u classes seen=%x want=%x\n",
           tg->name, tg->ra, verdict, t->cases, t->proven, t->inconclusive, t->faults,
           t->bridge, t->interp, seen, want);
    if (t->fault[0] != '\0')
        printf("AUDIT   %s host-fault: %s\n", tg->name, t->fault);
    for (c = 0; c < C_COUNT; c++)
        if (t->cat[c] != 0)
            printf("AUDIT   %s %s cases=%u e.g. %s\n", tg->name, kCategory[c], t->cat[c],
                   t->example[c]);
    return strcmp(verdict, "EQUIVALENT") == 0;
}

/* BATTLE_BOUNDARY_AUDIT_REQUIRE="func_A func_B": targets that must come out
 * EQUIVALENT with every class in every regime (a converted body); the exit
 * status is 1 when one does not. */
static const char *audit_require;
static unsigned require_failures;
static int required(const char *name)
{
    size_t n = strlen(name);
    const char *p = audit_require;
    while (p != NULL && (p = strstr(p, name)) != NULL) {
        if ((p == audit_require || p[-1] == ' ') && (p[n] == '\0' || p[n] == ' '))
            return 1;
        p += n;
    }
    return 0;
}
static void require_ok(const char *name, const char *what, int ok)
{
    if (!required(name))
        return;
    printf("AUDIT REQUIRE %s %s: %s\n", name, what, ok ? "ok" : "FAIL");
    if (!ok)
        require_failures++;
}

/* ---- fixtures --------------------------------------------------------------- */
static uint32_t rng = 0xB0A2D17u;
static uint32_t rnd32(void)
{
    uint32_t hi;
    rng = rng * 1664525u + 1013904223u;
    hi = rng >> 16;
    rng = rng * 1664525u + 1013904223u;
    return (hi << 16) | (rng >> 16);
}

static void fresh_state(void)
{
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memcpy(PSX_ADDR(slus_taddr), slus + 0x800, slus_tsize);
    memset(g_PsxScratchpad, 0, sizeof(g_PsxScratchpad));
}


static void w16(uint32_t a, uint16_t v) { memcpy(PSX_ADDR(a), &v, 2); }
static void w32(uint32_t a, uint32_t v) { memcpy(PSX_ADDR(a), &v, 4); }

#define SPRITE 0x800C5200u
#define ANIM   0x800C5400u

/* 800223B0(sprite, s16 angle) -- retail 0x800223B0 (172 insns).  The sprite
 * record's +0x58 word is the current animation.  Guest regime: a guest
 * address (what retail bytes, or a host replaying retail stores, would
 * write).  Host regime: the 32-bit host address of the same bytes, which is
 * what the startup path at 0x801E68F8 holds by static reading: 800BA984
 * (jal at 800BAACC) sets +0x58 through main-exe func_800245D8, native C in
 * every build (temp1.c `*(u32*)(pData + 0x58) = (u32)(uintptr_t)pAnimation`,
 * which itself faults on guest-domain inputs), and the startup-actors bind
 * host does not store to +0x58 between the two calls.  Classes:
 * 1 = +0x48 == 0 early return, 2/4/8 = mode 0/1/2 with the direction kept,
 * 16 = direction change (bytecode re-armed, 80022660 runs). */
/* The direction 800223B0 settles on for a mode and angle (its own formulas,
 * used only to choose the old direction so a case keeps or changes it). */
static unsigned direction_223b0(unsigned mode, int16_t angle)
{
    unsigned d;
    if (mode == 1) {
        d = (unsigned)((angle + 0x600) >> 10) & 3u;
        return d;
    }
    if (mode == 2) {
        d = (unsigned)((angle + 0x500) >> 9) & 7u;
        return d < 5u ? d : ((d - 5u) ^ 3u);
    }
    return 0;
}

static void fixture_223b0(unsigned v, int host_regime, uint32_t *args, unsigned *cls)
{
    unsigned mode = v % 3u, k, dir;
    uint32_t anim_word = host_regime ? (uint32_t)(uintptr_t)PSX_ADDR(ANIM) : ANIM;
    int16_t angle = (int16_t)(rnd32() & 0xFFFu);
    int change = (v % 4u) == 1u;
    uint32_t a8;

    fresh_state();
    /* the animation table: +2 the bytecode offset, +4.. per-direction
     * offsets; the bytecode itself is never executed (see below). */
    w16(ANIM + 0x2, 0x40);
    for (k = 0; k < 8; k++)
        w16(ANIM + 0x4 + 2u * k, (uint16_t)(0x20 + 4u * k));
    w32(SPRITE + 0x48, (v % 7u) == 3u ? 0u : 1u);
    w32(SPRITE + 0x58, anim_word);
    /* A direction change re-arms +0x64 at the table's bytecode and runs the
     * VM (80022660) until it is back at the old bytecode pointer with the
     * old sub-index: old +0x64 == the new bytecode and sub-index 0 make that
     * immediate. */
    w32(SPRITE + 0x64, anim_word + 0x42u);
    dir = direction_223b0(mode, angle);
    if (change)
        dir = (dir + 1u + (rnd32() % 6u)) & 7u;
    a8 = (rnd32() & 0x0001FFFFu) | (dir << 17) | (mode << 20) |
         ((rnd32() & 0xFu) << 28);
    w32(SPRITE + 0xA8, a8);
    w32(SPRITE + 0xAC, rnd32() & ~0x4u);
    w32(SPRITE + 0x3C, rnd32());
    w16(SPRITE + 0x9E, (uint16_t)rnd32());
    args[0] = SPRITE;
    args[1] = (uint32_t)(int32_t)angle;
    args[2] = args[3] = 0;
    *cls = (v % 7u) == 3u ? 1u : (change && dir != direction_223b0(mode, angle) ? 16u : 0u) |
           (2u << mode);
}

/* 80021FE0(sprite, s16 angle): +0x32 = angle, then 80022974 recomputes +0x0C
 * / +0x14 from +0x18 (radius), +0xAC bits 7..18 (divisor) and rsin/rcos of
 * the angle.  Classes: 1 = divisor 0, 2 = positive radius, 4 = negative. */
static void fixture_21fe0(unsigned v, uint32_t *args, unsigned *cls)
{
    uint32_t div = (v % 5u) == 0u ? 0u : 1u + (rnd32() % 0xFFFu);
    int32_t radius = (int32_t)(rnd32() % 0x40000u) - ((v & 1u) ? 0x20000 : 0);

    fresh_state();
    w32(SPRITE + 0x18, (uint32_t)radius);
    w32(SPRITE + 0xAC, (rnd32() & ~(0xFFFu << 7)) | (div << 7));
    w32(SPRITE + 0x0C, rnd32());
    w32(SPRITE + 0x14, rnd32());
    args[0] = SPRITE;
    args[1] = (uint32_t)(int32_t)(int16_t)rnd32();
    args[2] = args[3] = 0;
    *cls = div == 0 ? 1u : (radius >= 0 ? 2u : 4u);
}

#define BOUNDS 0x800C5600u
/* 8003101C(bounds, mode): returns 1 unless a sampled AABB point projects
 * on screen (GTE rotation/translation as the caller set them).  Classes:
 * 1 = mode & 3 == 0, 2 = visible (0), 4 = all culled (1). */
static void fixture_3101c(unsigned v, uint32_t *args)
{
    unsigned k;
    fresh_state();
    for (k = 0; k < 6; k++)
        w16(BOUNDS + 0x20 + 2u * (k < 3 ? k : k + 1u),
            (uint16_t)((int16_t)(rnd32() % 0x2000u) - 0x1000 + (int)(v % 4u) * 0x600));
    /* GTE: identity rotation, a translation that puts some boxes in front of
     * the camera near the centre and others behind or beside it, a
     * projection plane and screen offset; the screen bounds the body tests
     * against are the bound globals D_800500F8 (x) / D_800500FC (y << 16). */
    {
        MATRIX m;
        int near = (v % 3u) == 0u;
        memset(&m, 0, sizeof(m));
        m.m[0][0] = m.m[1][1] = m.m[2][2] = 0x1000;
        m.t[0] = near ? (long)(int32_t)(rnd32() % 0x200u) - 0x100 : (long)(int32_t)(rnd32() % 0x8000u) - 0x4000;
        m.t[1] = near ? (long)(int32_t)(rnd32() % 0x200u) - 0x100 : (long)(int32_t)(rnd32() % 0x8000u) - 0x4000;
        m.t[2] = near ? 0x2000 + (long)(rnd32() % 0x1000u) : (long)(int32_t)(rnd32() % 0x6000u) - 0x3000;
        SetRotMatrix(&m);
        SetTransMatrix(&m);
        SetGeomScreen(0x100);
        SetGeomOffset(160, 120);
        *(int32_t *)battle_guest_resolve_native(0x800500F8u) = 320;
        *(int32_t *)battle_guest_resolve_native(0x800500FCu) = 240 << 16;
    }
    args[0] = BOUNDS;
    args[1] = v % 4u;
    args[2] = args[3] = 0;
}

#define CLUTS 0x800C5800u
#define HEAP_LO 0x80140000u
#define HEAP_HI 0x80160000u
extern void HeapInit(void *pHeapStart, void *pHeapEnd);
static void heap_side(void)
{
    HeapInit(PSX_ADDR(HEAP_LO), PSX_ADDR(HEAP_HI));
}
/* 80022A70 = GfxLoadClutsAccelerated(clut data, x, y): for each of the
 * count CLUT records (offset table after the count word) upload rect
 * {x + 64*i, y, w, h} from record+4 through GfxLoadImageAccelerated
 * (HeapAlloc 0x2000, LoadImage(D_800592F0, D_800592F4), HeapFree).
 * Delegated on the retail side: HeapAlloc / HeapFree (the native heap,
 * re-initialised for each side) and LoadImage (a recording spy that reads
 * the rect at call time).  Classes: 1 = count 0, 2 = one record, 4 = more.
 * Expected residue: retail GfxLoadImageAccelerated saves $sp at buffer+0x1F00
 * and runs LoadImage on that heap block; the port body has no stack switch,
 * so that word of the freed block shows as ram-missing. */
static void fixture_22a70(unsigned v, uint32_t *args, unsigned *cls)
{
    unsigned count = v % 4u, i;
    uint32_t rec = CLUTS + 4u + 4u * count;

    fresh_state();
    w32(CLUTS, count);
    for (i = 0; i < count; i++) {
        w32(CLUTS + 4u + 4u * i, rec - CLUTS);
        w16(rec, (uint16_t)(16u + 16u * (rnd32() % 16u)));
        w16(rec + 2u, (uint16_t)(1u + rnd32() % 4u));
        w32(rec + 4u, rnd32());
        rec += 0x40u;
    }
    args[0] = CLUTS;
    args[1] = (uint32_t)(rnd32() % 0x400u);
    args[2] = (uint32_t)(0x1E0u + rnd32() % 0x20u);
    args[3] = 0;
    *cls = count == 0 ? 1u : (count == 1 ? 2u : 4u);
}

/* The lit (variant 1) packet callbacks the battle packet-render host calls
 * at 0x8002C864: FT3 8002F2E0, FT4 8002FAE8, GT3 8002F4B4, F3 8002ED20, G3
 * 8002F6B4 and F4 8002F8D0.  That host sets the model's working pointers
 * through the bus -- vertices 0x8005953C, normals or lighting records
 * 0x80059498, vertex normals 0x8005952C, packet cursor 0x80059424, OT
 * 0x80059568 -- which the runtime
 * keeps in guest RAM ("model-packet-canonical"), and the shared bound
 * globals D_80059578 (emitted), D_800500F8 / D_800500FC (screen bounds),
 * D_80050100 (OT shift).  The legacy native bodies read the NATIVE C globals
 * of the same names (a separate LP64 interface the runtime deliberately does
 * not bind); the battle-context hosts (battle_lit_ft_packets_host.inc) read
 * the guest words.  Regimes for the native globals: 0 = fresh (never set by
 * battle), 1 = stale (host pointers to a separate copy of the model data).
 *
 * Commands are 8 bytes (index 0 | index 1 << 16, index 2 | index 3 << 16);
 * a call with count n evaluates n faces and reads n + 1 commands (the last
 * is the lookahead RTPT).  Each case has one mode, and a proven case counts
 * its class only when the retail-side effects (emitted-count delta, OT
 * links, the first packet's XY word) show the mode's outcome:
 *   0x001 count 0: prefetch and the lookahead RTPT only
 *   0x002 front faces in bounds: counted and linked
 *   0x004 back faces: rejected (the triangles first store XY words)
 *   0x008 y bound 0: rejected on y          0x010 x bound 0: rejected on x
 *   0x020 a near vertex (H/SZ divide overflow), and still drawn: retail's
 *         "flag" tests read data register 31 (LZCR), never negative, not
 *         FLAG, so they never reject
 *   0x040 ZSF3 / ZSF4 0 (OTZ 0): as prim_otz0 says for the target
 *   0x080 quads only: the fourth vertex near (RTPS overflow), still drawn
 *   0x100 edges, emitting: only the last vertex inside y or x, faces wholly
 *         at negative screen y or x (unsigned bounds), a 24-bit (KUSEG)
 *         cursor, quad index-0 words with bit 13 set (retail keeps 13 bits) */
#define G_VERT 0x800C6000u
#define G_FAR  0x800D6000u      /* G_VERT + 0x10000: a 16-bit FT4 mask reads here */
#define G_NORM 0x800C5800u
#define G_CMD  0x800C6800u
#define G_PKT  0x800C7000u
#define G_OT   0x800C8000u
#define G_OT_BYTES 0x2000u
#define G_LIT  0x800C5400u      /* lighting records (G3 / F3 / F4), below G_NORM */
#define PRIM_PATTERN(k) (0x09000000u | ((uint32_t)(k) << 4))
static uint8_t stale_vert[0x400], stale_norm[0x400], stale_lit[0x400], stale_pkt[0x800], stale_ot[0x400];
static unsigned prim_mode, prim_faces;
/* The lighting data of the lit callback a case targets:
 *   PRIM_FACE_NORMAL    FT3 / FT4: a normal per face (8 bytes) at the
 *                       0x80059498 cursor (G_NORM);
 *   PRIM_VERTEX_NORMAL  GT3: a normal per vertex index, G_NORM + 8 * index,
 *                       its base in 0x8005952C;
 *   PRIM_VERTEX_COLOUR  G3: those vertex normals, plus a colour word per face
 *                       (4 bytes) at the 0x80059498 cursor (G_LIT);
 *   PRIM_FACE_RECORD    F3 / F4: a 12-byte record per face (colour word, then
 *                       the normal) at the 0x80059498 cursor (G_LIT).
 * G_LIT keeps the 0x80059498 data apart from the vertex normals, so a body
 * that reads the wrong base global does not pass. */
enum { PRIM_FACE_NORMAL, PRIM_VERTEX_NORMAL, PRIM_VERTEX_COLOUR, PRIM_FACE_RECORD };
/* How an OTZ-0 face (mode 6) shows: counted, not linked (FT3, FT4, F4);
 * neither (GT3, G3 count after the OTZ test); counted and linked at OT[0]
 * (F3 has no OTZ test). */
enum { OTZ0_COUNTED, OTZ0_SKIPPED, OTZ0_LINKED };
static int prim_kind, prim_otz0;
static int32_t prim_emit0;
static uint32_t prim_cursor;

static void prim_gte(void)
{
    memset(&gteRegs, 0, sizeof(gteRegs));
    CTC2(4096, 0); CTC2(4096, 2); CTC2(4096, 4);
    CTC2(1024, 7);
    CTC2(4096, 8); CTC2(4096, 10); CTC2(4096, 12);
    CTC2(4096, 16); CTC2(4096, 18); CTC2(4096, 20);
    CTC2(0x00302010, 21); CTC2(0x00403020, 22); CTC2(0x00504030, 23);
    CTC2(160u << 16, 24); CTC2(112u << 16, 25); CTC2(512, 26);
    CTC2(0x100, 27); CTC2(0x200000, 28); CTC2(1365, 29); CTC2(1024, 30);
    MTC2(0x24808080u, 6);
}

static void set_native_word(const char *name, uint64_t value, size_t width)
{
    void *p = dlsym(RTLD_DEFAULT, name);
    if (p == NULL) {
        fprintf(stderr, "AUDIT FAIL no native %s\n", name);
        exit(2);
    }
    memcpy(p, &value, width);
}

/* Model coordinates that project to screen (sx, sy) at view depth zv
 * (identity rotation, TRZ 1024, H 512, offset 160, 112). */
static void put_vertex(uint32_t a, int sx, int sy, int zv)
{
    int16_t xyz[4];
    xyz[0] = (int16_t)((sx - 160) * zv / 512);
    xyz[1] = (int16_t)((sy - 112) * zv / 512);
    xyz[2] = (int16_t)(zv - 1024);
    xyz[3] = 0;
    memcpy(PSX_ADDR(a), xyz, 8);
}

/* Face f of a case in mode m: vertices 4f..4f+3, wound front-facing (NCLIP
 * > 0; back-facing in mode 2).  Returns the two command words. */
static void prim_face(unsigned f, unsigned m, int quads, uint32_t *w0, uint32_t *w1)
{
    int px[4], py[4], zv[4], k, r = (int)(rnd32() % 20u), d = 12 + (int)(rnd32() % 20u);
    int cx = 30 + (int)(rnd32() % 220u), cy = 30 + (int)(rnd32() % 130u);
    int z = 1100 + (int)(rnd32() % 400u);
    unsigned idx[4], base = 4u * f, edge = m == 8u ? rnd32() % 5u : 99u;
    long cross;

    px[0] = cx; py[0] = cy; px[1] = cx + d; py[1] = cy;
    px[2] = cx; py[2] = cy + d; px[3] = cx + d; py[3] = cy + d;
    for (k = 0; k < 4; k++)
        zv[k] = z + (int)(rnd32() % 40u);
    switch (edge) {
    case 0:   /* only the last vertex (C; D for FT4) inside y */
        py[0] = py[1] = 230 + r;
        py[2] = quads ? 230 + r + d : 205;
        py[3] = 205;
        break;
    case 1:   /* only the last vertex inside x */
        px[0] = px[2] = 330 + r;
        px[1] = 330 + r + d;
        if (quads)
            px[3] = 300;
        else
            px[2] = 300;
        break;
    case 2:   /* wholly at negative screen y */
        for (k = 0; k < 4; k++)
            py[k] -= 220;
        break;
    case 3:   /* wholly at negative screen x */
        for (k = 0; k < 4; k++)
            px[k] -= 300;
        break;
    default:
        break;
    }
    if (m == 5u)
        zv[rnd32() % 3u] = 100;          /* RTPT divide overflow */
    if (m == 7u && quads)
        zv[3] = 100;                     /* RTPS divide overflow */
    cross = (long)(px[1] - px[0]) * (py[2] - py[0]) - (long)(py[1] - py[0]) * (px[2] - px[0]);
    if ((cross > 0) != (m != 2u)) {      /* swap A and B: the last vertex stays */
        k = px[0]; px[0] = px[1]; px[1] = k;
        k = py[0]; py[0] = py[1]; py[1] = k;
        k = zv[0]; zv[0] = zv[1]; zv[1] = k;
    }
    for (k = 0; k < 4; k++) {
        idx[k] = base + (unsigned)k;
        put_vertex(G_VERT + 8u * idx[k], px[k], py[k], zv[k]);
    }
    if (quads && m == 8u && f != 0u) {
        put_vertex(G_FAR + 8u * idx[0], px[0] + 7, py[0] - 9, zv[0] + 50);
        idx[0] |= 0x2000u;
    }
    *w0 = idx[0] | (idx[1] << 16);
    *w1 = idx[2] | (idx[3] << 16);
}

static void prim_common(int32_t emit0, unsigned mode)
{
    unsigned k;
    for (k = 0; k < 0x800u; k += 4u)
        w32(G_PKT + k, PRIM_PATTERN(k));
    prim_emit0 = emit0;
    *(int32_t *)battle_guest_resolve_native(0x80059578u) = emit0;
    *(int32_t *)battle_guest_resolve_native(0x800500F8u) = mode == 4u ? 0 : 320;
    *(int32_t *)battle_guest_resolve_native(0x800500FCu) = mode == 3u ? 0 : 224 << 16;
    *(int32_t *)battle_guest_resolve_native(0x80050100u) = (int32_t)(rnd32() % 4u);
}

static void prim_native_globals(int stale)
{
    if (stale) {
        memcpy(stale_vert, PSX_ADDR(G_VERT), sizeof(stale_vert));
        memcpy(stale_norm, PSX_ADDR(G_NORM), sizeof(stale_norm));
        memcpy(stale_lit, PSX_ADDR(G_LIT), sizeof(stale_lit));
        memcpy(stale_pkt, PSX_ADDR(G_PKT), sizeof(stale_pkt));
        memset(stale_ot, 0, sizeof(stale_ot));
        set_native_word("D_8005953C", (uint32_t)(uintptr_t)stale_vert, 4);
        set_native_word("D_80059498", (uint32_t)(uintptr_t)
                        (prim_kind >= PRIM_VERTEX_COLOUR ? stale_lit : stale_norm), 4);
        set_native_word("D_80059424", (uint64_t)(uintptr_t)(stale_pkt + 0x40), 8);
        set_native_word("D_80059568", (uint32_t)(uintptr_t)stale_ot, 4);
        if (prim_kind == PRIM_VERTEX_NORMAL || prim_kind == PRIM_VERTEX_COLOUR)
            set_native_word("D_8005952C", (uint32_t)(uintptr_t)stale_norm, 4);
    } else {
        set_native_word("D_8005953C", 0, 4);
        set_native_word("D_80059498", 0, 4);
        set_native_word("D_80059424", 0, 8);
        set_native_word("D_80059568", 0, 4);
        if (prim_kind == PRIM_VERTEX_NORMAL || prim_kind == PRIM_VERTEX_COLOUR)
            set_native_word("D_8005952C", 0, 4);
    }
}

/* The 0x80059498 data of G3 / F3 / F4 at G_LIT for n faces: colour words
 * (any code byte), each followed by a face normal for F3 / F4. */
static void prim_lit_records(unsigned n)
{
    unsigned f, stride = prim_kind == PRIM_FACE_RECORD ? 12u : 4u;
    for (f = 0; f < n; f++) {
        w32(G_LIT + stride * f, rnd32());
        if (stride == 12u) {
            w32(G_LIT + stride * f + 4u, rnd32() & 0x0FFF0FFFu);
            w32(G_LIT + stride * f + 8u, rnd32() & 0x0FFFu);
        }
    }
}

static void fixture_prim(unsigned v, int stale, int quads, uint32_t *args, unsigned *cls)
{
    unsigned mode = v % 9u, faces = mode == 0u ? 0u : 1u + rnd32() % 4u, f;
    uint32_t w0, w1;

    fresh_state();
    prim_gte();
    if (mode == 6u) {
        CTC2(0, 29);
        CTC2(0, 30);
    }
    for (f = 0; f <= faces; f++) {     /* faces + the lookahead command */
        prim_face(f, mode, quads, &w0, &w1);
        w32(G_CMD + 8u * f, w0);
        w32(G_CMD + 8u * f + 4u, w1);
        w32(G_NORM + 8u * f, rnd32() & 0x0FFF0FFFu);
        w32(G_NORM + 8u * f + 4u, rnd32() & 0x0FFFu);
    }
    if (prim_kind == PRIM_VERTEX_NORMAL || prim_kind == PRIM_VERTEX_COLOUR) {
        for (f = 0; f < 4u * (faces + 1u); f++) {   /* one normal per vertex index */
            w32(G_NORM + 8u * f, rnd32() & 0x0FFF0FFFu);
            w32(G_NORM + 8u * f + 4u, rnd32() & 0x0FFFu);
        }
        w32(0x8005952Cu, G_NORM);
    }
    if (prim_kind >= PRIM_VERTEX_COLOUR)
        prim_lit_records(faces + 1u);
    prim_common((int32_t)(rnd32() % 8u), mode);
    prim_cursor = G_PKT + 0x40u;
    if (mode == 8u || (mode == 1u && (v & 16u)))
        prim_cursor &= 0x00FFFFFFu;    /* as left by an earlier emitting callback */
    /* the guest-visible model state, as the dispatcher host writes it */
    w32(0x8005953Cu, G_VERT);
    w32(0x80059498u, prim_kind >= PRIM_VERTEX_COLOUR ? G_LIT : G_NORM);
    w32(0x80059424u, prim_cursor);
    w32(0x80059568u, G_OT);
    prim_native_globals(stale);
    prim_mode = mode;
    prim_faces = faces;
    args[0] = G_CMD;
    args[1] = faces;
    args[2] = args[3] = 0;
    *cls = 1u << mode;
}

/* The class a proven case shows (0 when its effects do not match its mode). */
static unsigned prim_observed(int quads)
{
    int32_t emitted = *(int32_t *)battle_guest_resolve_native(0x80059578u) - prim_emit0;
    int linked = 0;
    uint32_t a;
    for (a = G_OT; a < G_OT + G_OT_BYTES; a += 4u)
        if (le32(PSX_ADDR(a)) != 0) {
            linked = 1;
            break;
        }
    switch (prim_mode) {
    case 0: return emitted == 0 && !linked ? 0x001u : 0u;
    case 1: return emitted == (int32_t)prim_faces && linked ? 0x002u : 0u;
    case 2:
        if (emitted != 0 || linked)
            return 0u;
        if (!quads && le32(PSX_ADDR(G_PKT + 0x48u)) == PRIM_PATTERN(0x48u))
            return 0u;   /* FT3 stores XY0 / XY1 before the backface test */
        return 0x004u;
    case 3: return emitted == 0 && !linked ? 0x008u : 0u;
    case 4: return emitted == 0 && !linked ? 0x010u : 0u;
    case 5: return emitted > 0 && linked ? 0x020u : 0u;
    case 6:
        if (prim_otz0 == OTZ0_SKIPPED)
            return emitted == 0 && !linked ? 0x040u : 0u;
        if (prim_otz0 == OTZ0_LINKED)
            return emitted == (int32_t)prim_faces && linked ? 0x040u : 0u;
        return emitted == (int32_t)prim_faces && !linked ? 0x040u : 0u;
    case 7: return quads && emitted > 0 && linked ? 0x080u : 0u;
    default: return emitted > 0 && linked ? 0x100u : 0u;
    }
}

/* 8002C700, the packet-render dispatcher (battle_packet_render_host.inc on
 * the native side), over a six-group stream, column 1 (a3): every lit
 * callback of the table's variant 1 -- FT3 (type 1), FT4 (9), GT3 (3), F3
 * (0), G3 (2) and F4 (8).  The vertex normals (GT3 / G3) come from the
 * model's +0x0C base (0x8005952C), at G_NORM; the 0x80059498 cursor (+0x18)
 * walks G_LIT across the groups (8 bytes a face for FT3 / FT4, 4 for G3, 12
 * for F3 / F4, none for GT3).  This is how battle reaches the callbacks:
 * battle_packet_render_call -> PcPortMipsRun -> the bridge -> the hosts at
 * return address 0x8002C86C.  Each group header (type | count << 16) is
 * followed by its commands; the next header is the lookahead.  Classes:
 * 1 = mode 1 groups (counted and linked), 2 = mode 8 (edges). */
#define G_DESC 0x800C5C00u
static void fixture_dispatch(unsigned v, uint32_t *args, unsigned *cls)
{
    static const uint32_t kType[6] = {1u, 9u, 3u, 0u, 2u, 8u};
    unsigned g, f, mode = (v & 1u) ? 8u : 1u, face = 0, count[6];
    uint32_t s = G_CMD, w0, w1;

    fresh_state();
    prim_gte();
    for (f = 0; f < 0x80u; f++) {      /* vertex normals (GT3 / G3) */
        w32(G_NORM + 8u * f, rnd32() & 0x0FFF0FFFu);
        w32(G_NORM + 8u * f + 4u, rnd32() & 0x0FFFu);
    }
    for (f = 0; f < 0x100u; f++)       /* the 0x80059498 data, every stride */
        w32(G_LIT + 4u * f, rnd32() & 0x0FFF0FFFu);
    for (g = 0; g < 6; g++) {
        count[g] = 1u + rnd32() % 3u;
        w32(s, kType[g] | (count[g] << 16));
        s += 4u;
        for (f = 0; f < count[g]; f++, face++) {
            prim_face(face, mode, kType[g] >= 8u, &w0, &w1);
            w32(s, w0);
            w32(s + 4u, w1);
            s += 8u;
        }
    }
    w32(s, 0);                          /* the last group's lookahead */
    w32(s + 4u, 0);
    prim_common((int32_t)(rnd32() % 8u), mode);
    w16(G_DESC + 4u, 0);
    w16(G_DESC + 6u, 6);
    w32(G_DESC + 8u, G_VERT);
    w32(G_DESC + 0xCu, G_NORM);       /* vertex normals: 0x8005952C */
    w32(G_DESC + 0x10u, G_CMD);
    w32(G_DESC + 0x18u, G_LIT);       /* the 0x80059498 cursor */
    *(int32_t *)battle_guest_resolve_native(0x80050104u) = 0;   /* no cull call */
    *(int32_t *)battle_guest_resolve_native(0x800595C0u) = (int32_t)(rnd32() % 64u);
    prim_native_globals(0);
    prim_cursor = G_PKT + 0x40u;
    if (v & 2u)
        prim_cursor &= 0x00FFFFFFu;
    prim_mode = mode;
    prim_faces = count[0] + count[1] + count[2] + count[3] + count[4] + count[5];
    args[0] = G_DESC;
    args[1] = prim_cursor;
    args[2] = G_OT;
    args[3] = 1;
    *cls = mode == 1u ? 1u : 2u;
}

/* ---- main ------------------------------------------------------------------- */
static int wanted(int argc, char **argv, const char *name)
{
    int i;
    if (argc <= 2)
        return 1;
    for (i = 2; i < argc; i++)
        if (strcmp(argv[i], name) == 0)
            return 1;
    return 0;
}

int main(int argc, char **argv)
{
    static const uint32_t kNone[] = {0};
    unsigned v, seen;
    uint32_t args[4];
    unsigned cls;

    if (argc < 2) {
        fprintf(stderr, "usage: %s disc/SLUS_006.64 [target ...]\n", argv[0]);
        return 2;
    }
    setenv("XENO_BATTLE_NATIVE_HOSTS", "1", 1);
    setenv("XENO_CPU_EXECUTION_STATS", "1", 1);  /* native-side interpreter entries */
    audit_verbose = getenv("AUDIT_VERBOSE") != NULL;
    load_slus(argv[1]);
    audit_require = getenv("BATTLE_BOUNDARY_AUDIT_REQUIRE");
    /* Hosts verify their retail bytes against the loaded image; give the
     * slot this run's file (a constructor may already have failed it). */
    g_XenoRetailFiles[XENO_RD_SLUS].bytes = slus;
    g_XenoRetailFiles[XENO_RD_SLUS].size = slus_size;
    g_XenoRetailFiles[XENO_RD_SLUS].state = 1;
    memset(&rt, 0, sizeof(rt));
    g_ActiveBattleRuntime = &rt;
    initialize_runtime(&rt);
    rt.bridge_cpu = &cpu;
    nat_init();
    {
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_sigaction = fault_handler;
        sa.sa_flags = SA_SIGINFO;
        sigemptyset(&sa.sa_mask);
        sigaction(SIGSEGV, &sa, NULL);
        sigaction(SIGBUS, &sa, NULL);
    }
    printf("AUDIT setup: SLUS text %08x+%x, %u bound globals (%zu bytes)\n",
           slus_taddr, slus_tsize, (unsigned)rt.data_count, nat_bytes);
    {
        static const uint32_t probe[] = {0x800500F8u, 0x800500FCu, 0x80018644u,
                                         0x800592F0u, 0x800592F4u};
        unsigned i;
        for (i = 0; i < 5; i++) {
            uint8_t *h = battle_guest_resolve_native(probe[i]);
            printf("AUDIT binding %08x: %s\n", probe[i],
                   (h >= g_PsxRam && h < g_PsxRam + PSX_RAM_SIZE) ? "UNBOUND (guest RAM)" : "bound native copy");
        }
    }
    binding_size_report();
    delegated = kNone;
    delegated_count = 0;

    if (wanted(argc, argv, "func_800223B0")) {
        static const Target tg = {"func_800223B0", 0x800223B0u, 0x801E68F8u, 0, 0};
        int regime;
        for (regime = 0; regime < 2; regime++) {
            Tally t;
            memset(&t, 0, sizeof(t));
            seen = 0;
            for (v = 0; v < 42; v++) {
                fixture_223b0(v, regime, args, &cls);
                if (audit_case(&tg, args, &t))
                    seen |= cls;
            }
            printf("AUDIT regime=%s ", regime ? "host(+0x58 holds a host address)"
                                               : "guest(+0x58 holds a guest address)");
            report(&tg, &t, seen, 0x1Fu);
        }
    }
    if (wanted(argc, argv, "func_80021FE0")) {
        static const Target tg = {"func_80021FE0", 0x80021FE0u, 0x801E6904u, 0, 0};
        Tally t;
        memset(&t, 0, sizeof(t));
        seen = 0;
        for (v = 0; v < 60; v++) {
            fixture_21fe0(v, args, &cls);
            if (audit_case(&tg, args, &t))
                seen |= cls;
        }
        report(&tg, &t, seen, 7u);
    }
    if (wanted(argc, argv, "func_80022A70")) {
        static const Target tg = {"func_80022A70", 0x80022A70u, 0x801E6620u, 0, 0};
        static const uint32_t kHeapImage[] = {0x80031BDCu, 0x800320E8u, 0x80044894u};
        Tally t;
        memset(&t, 0, sizeof(t));
        seen = 0;
        delegated = kHeapImage;
        delegated_count = 3;
        pre_side = heap_side;
        for (v = 0; v < 24; v++) {
            fixture_22a70(v, args, &cls);
            if (audit_case(&tg, args, &t))
                seen |= cls;
        }
        pre_side = NULL;
        delegated = kNone;
        delegated_count = 0;
        report(&tg, &t, seen, 7u);
    }
    {
        /* The lit (variant 1) callbacks: triangles want every class but 0x080
         * (an RTPS overflow, quads only). */
        static const Target kPrim[6] = {
            {"func_8002F2E0", 0x8002F2E0u, 0x8002C86Cu, 32, AUDIT_GPRS},
            {"func_8002FAE8", 0x8002FAE8u, 0x8002C86Cu, 32, AUDIT_GPRS},
            {"func_8002F4B4", 0x8002F4B4u, 0x8002C86Cu, 32, AUDIT_GPRS},
            {"func_8002ED20", 0x8002ED20u, 0x8002C86Cu, 32, AUDIT_GPRS},
            {"func_8002F6B4", 0x8002F6B4u, 0x8002C86Cu, 32, AUDIT_GPRS},
            {"func_8002F8D0", 0x8002F8D0u, 0x8002C86Cu, 32, AUDIT_GPRS},
        };
        static const int kQuads[6] = {0, 1, 0, 0, 0, 1};
        static const int kKind[6] = {PRIM_FACE_NORMAL, PRIM_FACE_NORMAL, PRIM_VERTEX_NORMAL,
                                     PRIM_FACE_RECORD, PRIM_VERTEX_COLOUR, PRIM_FACE_RECORD};
        static const int kOtz0[6] = {OTZ0_COUNTED, OTZ0_COUNTED, OTZ0_SKIPPED,
                                     OTZ0_LINKED, OTZ0_SKIPPED, OTZ0_COUNTED};
        unsigned q;
        for (q = 0; q < 6; q++) {
            int regime, all = 1, quads = kQuads[q];
            unsigned want = quads ? 0x1FFu : 0x17Fu;
            if (!wanted(argc, argv, kPrim[q].name))
                continue;
            prim_kind = kKind[q];
            prim_otz0 = kOtz0[q];
            for (regime = 0; regime < 2; regime++) {
                Tally t;
                memset(&t, 0, sizeof(t));
                seen = 0;
                for (v = 0; v < 72; v++) {
                    fixture_prim(v, regime, quads, args, &cls);
                    if (audit_case(&kPrim[q], args, &t)) {
                        unsigned shown = prim_observed(quads) & cls;
                        seen |= shown;
                        if (audit_verbose && shown == 0)
                            printf("AUDIT   %s case %u mode %u: proven, but its effects do not show the mode "
                                   "(emitted %d of %u faces)\n", kPrim[q].name, v, prim_mode,
                                   (int)(*(int32_t *)battle_guest_resolve_native(0x80059578u) - prim_emit0),
                                   prim_faces);
                    }
                }
                printf("AUDIT regime=%s ", regime ? "stale(native pointer globals: an earlier native model)"
                                                  : "fresh(native pointer globals never set by battle)");
                all &= report(&kPrim[q], &t, seen, want);
            }
            require_ok(kPrim[q].name, "EQUIVALENT with every class in both regimes", all);
            /* The host must run only at its call site and while enabled: a
             * decline reaches the legacy body (or the native-only refusal),
             * which faults on the fresh native globals or is not native. */
            {
                Target off = kPrim[q];
                int pass;
                for (pass = 0; pass < 2; pass++) {
                    Tally t;
                    memset(&t, 0, sizeof(t));
                    if (pass == 0)
                        off.ra = 0x8002C870u;
                    else
                        setenv("XENO_BATTLE_NATIVE_LIT_FT_PACKETS", "0", 1);
                    fixture_prim(1u, 0, quads, args, &cls);
                    audit_case(&off, args, &t);
                    if (pass == 1)
                        unsetenv("XENO_BATTLE_NATIVE_LIT_FT_PACKETS");
                    off = kPrim[q];
                    printf("AUDIT decline %s %s: proven=%u host-faults=%u not-native=%u\n",
                           kPrim[q].name, pass ? "XENO_BATTLE_NATIVE_LIT_FT_PACKETS=0" : "ra 8002c870",
                           t.proven, t.faults, t.bridge);
                    require_ok(kPrim[q].name, pass ? "declines when switched off" : "declines off its call site",
                               t.proven == 0 && t.faults + t.bridge != 0);
                }
            }
            prim_kind = PRIM_FACE_NORMAL;
            prim_otz0 = OTZ0_COUNTED;
        }
    }
    if (wanted(argc, argv, "func_8002C700")) {
        static const Target tg = {"func_8002C700", 0x8002C700u, 0x800A0000u, 32, AUDIT_GPRS};
        Tally t;
        memset(&t, 0, sizeof(t));
        seen = 0;
        for (v = 0; v < 24; v++) {
            fixture_dispatch(v, args, &cls);
            if (audit_case(&tg, args, &t) && prim_observed(1) != 0)
                seen |= cls;
        }
        require_ok(tg.name, "EQUIVALENT through the dispatcher host", report(&tg, &t, seen, 3u));
    }
    /* Hosts-crash fix A, first half (this build is the hybrid one): a packet
     * callback no host owns, dispatched by the hosted 8002C700 (ra
     * 0x8002C86C), fails closed -- never its legacy body, which reads the
     * native packet interface the hosted path does not set (in the all-hosts
     * battle 1, F1's legacy 8002F4B4 there corrupted the heap).  80030750
     * (row 16, every variant) stays unconverted, and its legacy body is in
     * this link (temp2.c), so F1 would bind it; the table's other unconverted
     * callbacks have no legacy body (unresolved: a stop either way). */
    if (wanted(argc, argv, "func_80030750")) {
        int nrc;
        fixture_prim(0u, 1, 0, args, &cls);
        nrc = run_native(0x80030750u, args, 0x8002C86Cu);
        printf("AUDIT fail-closed func_80030750 from ra 8002c86c: runtime_bridge=%d (want -1: a stop, no legacy C)\n", nrc);
        require_ok("func_80030750", "fails closed from the hosted 8002C700 (hybrid)", nrc == -1);
    }
    /* Fix A, second half: with the packet-render host switched off, a model
     * whose pointer words are guest-shaped (the caller hosts build such
     * models) is refused instead of reaching the legacy LP64 8002C700, which
     * followed them as host pointers (SIGSEGV in the all-hosts battle 1). */
    if (wanted(argc, argv, "func_8002C700")) {
        int nrc;
        setenv("XENO_BATTLE_NATIVE_PACKET_RENDER", "0", 1);
        fixture_dispatch(0u, args, &cls);
        nrc = run_native(0x8002C700u, args, 0x800A0000u);
        unsetenv("XENO_BATTLE_NATIVE_PACKET_RENDER");
        printf("AUDIT packet-render off: func_8002C700 with a guest-shaped model: runtime_bridge=%d (want -1)\n", nrc);
        require_ok("func_8002C700", "refuses a guest-shaped model when the packet-render host is off", nrc == -1);
    }
    /* Hosts-crash fix C(a): an unconverted battle-frame callback that is
     * overlay guest code (800B9F78 from 0x800BEA80) declines to the
     * interpreter in the hybrid (runtime_bridge 0: run the retail bytes)
     * instead of stopping the battle; native-only still refuses it. */
    if (wanted(argc, argv, "func_800B9F78")) {
        int nrc;
        memset(args, 0, sizeof(args));
        nrc = run_native(0x800B9F78u, args, 0x800BEA88u);
        printf("AUDIT battle-frame callback func_800B9F78 from ra 800bea88: runtime_bridge=%d (want 0: interpret)\n", nrc);
        require_ok("func_800B9F78", "declines to the interpreter from the battle frame (hybrid)", nrc == 0);
    }
    if (wanted(argc, argv, "func_8003101C")) {
        static const Target tg = {"func_8003101C", 0x8003101Cu, 0x8002C744u, 32, 0};
        Tally t;
        memset(&t, 0, sizeof(t));
        seen = 0;
        for (v = 0; v < 80; v++) {
            fixture_3101c(v, args);
            if (audit_case(&tg, args, &t))
                seen |= (args[1] & 3u) == 0 ? 1u : (cpu.gpr[2] == 0 ? 2u : 4u);
        }
        report(&tg, &t, seen, 7u);
    }
    /* The counters are live: the retail sides account for every run. */
    printf("AUDIT interpreter runs=%llu instructions=%llu bridged=%llu (native sides interpret none unless reported)\n",
           (unsigned long long)PcPortMipsExecutionStatsGet().entries,
           (unsigned long long)PcPortMipsExecutionStatsGet().interpreted_instructions,
           (unsigned long long)PcPortMipsExecutionStatsGet().native_bridges);
    if (require_failures != 0) {
        printf("AUDIT REQUIRE FAIL %u requirement(s)\n", require_failures);
        return 1;
    }
    return 0;
}
