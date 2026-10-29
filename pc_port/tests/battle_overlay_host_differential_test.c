/* Differential prover for the adopted battle overlay C bodies.
 *
 * For every function named in pc_port/src/battle_overlay_host_leaves.inc this
 * runs the retail MIPS bytes out of disc/battle.bin through the interpreter and
 * the adopted host C body, on identical guest RAM and identical arguments, then
 * compares the return registers and the guest RAM the call changed.
 *
 * The host side is reached exactly the way production reaches it: the runtime's
 * own dlsym bridge, with the same argument translation. That makes this a test
 * of the allowlist, the guest-RAM D_* aliases and the native boundary ABI as
 * well as of the C body itself. The interpreted side forces every guest call
 * back into the interpreter (BattleMipsRuntime.force_interpret), so a body that
 * calls another adopted body is compared as retail ran it, not as the host
 * happens to compose it.
 *
 * What this does NOT cover:
 *  - the guest stack window is excluded, because the interpreter writes its
 *    frame there and a host C body keeps its frame on the host stack;
 *  - writes to variables that resolve to a *native* copy (main-exe globals with
 *    a shared binding) land outside g_PsxRam and are invisible to the RAM diff;
 *  - an interpreted run that does not halt is reported as inconclusive rather
 *    than failed, so unexercised paths do not masquerade as proof.
 */

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
/* bg_jalr, the native indirect-call entry the guest-ABI bodies use. */
#include "battle_host_guest.h"

/* Guest RAM plus a guard tail. PSX_ADDR masks a guest address into 2 MiB, so a
 * pointer loaded out of seeded random data can land in the last bytes of the
 * region and a body that writes past it would run off the array. On hardware
 * the address wraps; the guard keeps the harness alive so the divergence can be
 * reported instead of crashing. Only the first PSX_RAM_SIZE bytes are compared. */
uint8_t g_PsxRam[PSX_RAM_SIZE + 0x1000];
uint8_t g_PsxScratchpad[4096];
#ifndef PROVER_REAL_GTE
unsigned int MFC2(int reg) { (void)reg; abort(); }
unsigned int CFC2(int reg) { (void)reg; abort(); }
void MTC2(unsigned int v, int reg) { (void)v; (void)reg; abort(); }
void CTC2(unsigned int v, int reg) { (void)v; (void)reg; abort(); }
int doCOP2(int op) { (void)op; abort(); }
#endif
#ifdef PROVER_REAL_GTE
/* PsyCross's GTE register file (gte/PsyX_GTE.cpp: GTERegisters, CP0 + CP2
 * data + CP2 control, 3 x 32 words).  It is process state that neither side's
 * RAM snapshot covers: compare() gives both sides the same starting registers
 * and compares the registers each side leaves (a callee such as SetRotMatrix
 * leaves its result there for the caller). */
#define PROVER_GTE_BYTES 0x180u
extern uint8_t gteRegs[PROVER_GTE_BYTES];
#endif
int g_GPUDisabledState;
unsigned g_PcPortPresentedFrames;
void ControllerPushState(void) {}
void ControllerPoll(void) {}
void PcPort_ServiceVblank(void) {}
void PcPort_PadVblankPump(void) {}
void PsyX_UpdateInput(void) {}
/* runtime_bridge's debug hook (pc_port/src/god_mode.c, not linked here). It is
 * port instrumentation, not a game callee, so it must not come from the
 * generated placeholders: those mark the case inconclusive. */
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

/* The allowlist is the unit under test; the width of the return value each body
 * defines is generated from src/battle so this cannot drift either. */
typedef struct Leaf {
    const char *name;
    unsigned return_bits;
    /* Bit i set when parameter i is declared as a pointer. */
    unsigned pointer_params;
} Leaf;

static const Leaf kLeaves[] = {
#include "overlay_leaf_types.inc"
/* h-frontier: module fixtures, outside the battle overlay allowlist. */
    {"func_801E6F00", 0, 1},
    {"func_801E6AC4", 0, 0},
    {"func_801E6E48", 0, 1},
    {"func_801E6D6C", 0, 1},
    {"func_801E6D34", 0, 1},

};
#define LEAF_COUNT (sizeof(kLeaves) / sizeof(kLeaves[0]))
#define FRONTIER_MODULE_LEAF_COUNT 5u
static int frontier_module_only; /* h-frontier scoped module proof */
/* h-frontier: when set, compare() runs each side through it (retail = 1)
 * instead of calling the leaf (the task-callback cases, below). */
static int (*frontier_side_run)(int retail);

/* Guest addresses whose overlay alias is a pointer: the guest word holds a PSX
 * address that a body dereferences. The sweep seeds each of these with one live
 * RAM address, because a zero word makes retail fault on the dereference and the
 * case could never complete. Generated from the same aliases the bodies use, so
 * a body cannot silently lose its seed. */
static const uint32_t kPointerGlobals[] = {
#include "overlay_pointer_globals.inc"
};
#define POINTER_GLOBAL_COUNT \
    (sizeof(kPointerGlobals) / sizeof(kPointerGlobals[0]))

#define OVERLAY_BASE 0x8006FAF0u
#define SCRATCH      0x800C4000u   /* overlay BSS: raw guest RAM on both sides */
#define SCRATCH_END  0x800C8000u
#define IMAGE_MAX    0x60000u
#define STEP_LIMIT   4000000u
/* The interpreter's own frame lives here; the host keeps its frame off-RAM. */
#define STACK_SKIP_LO 0x1FF000u
#define STACK_SKIP_HI 0x200000u

#define WORD 0x800C5000u
#define HALF 0x800C5100u
#define REC  0x800C5200u
#define SRC  0x800C5400u
#define DST  0x800C5600u
#define PTRT 0x801E0000u   /* inside RAM, outside the excluded stack window */
/* Bytes of PTRT the pointer-argument cases fill with the RAM pattern. Large
 * enough for the struct an overlay body typically touches through a pointer. */
#define POINTER_TARGET_SIZE 0x400u

static BattleMipsRuntime rt;
/* The native g_GameState both sides share. Main-exe globals resolve to one
 * native copy for the interpreted and the host run alike, so a write there is
 * invisible to the guest-RAM diff. Snapshot, seed and compare it explicitly:
 * many battle bodies do most of their work in the character records. */
#define NATIVE_GS_SIZE 0x2358u
static uint8_t *native_gs;
static uint8_t native_before[NATIVE_GS_SIZE];
static uint8_t native_expected[NATIVE_GS_SIZE];

/* ---- BEGIN h-frontier: main-executable globals with native bindings ------
 * Other main-executable globals the runtime binds to a native copy are just
 * as invisible to the guest-RAM diff as g_GameState: 800BEB04's only effect
 * on its common path is D_800591B0 = 1.  Each global listed here is seeded
 * before the case's snapshot (compare() -> frontier_case_setup).  compare()'s
 * native-global write check then restores and compares a bound one like every
 * other binding (this block compared them itself until that check existed);
 * a name with no native binding is seeded in guest RAM at its address, and
 * the RAM diff covers it.  Globals with the same group get the same seed byte
 * (800BEB04 reloads a module only when D_800591B2 != D_800591B3, a path whose
 * spied archive calls receive uninitialised locals, so the pair is kept
 * equal).  The seed never draws from rnd(), and it depends only
 * on begin_case()'s fill pattern and the case's index within its leaf, so a
 * globals' seeds do not move when other leaves gain or lose cases.  Called
 * from begin_case(), compare() and main(), each call marked "h-frontier". */
static const struct {
    const char *name;
    unsigned group;
} kFrontierGlobals[] = {
    {"D_800591B0", 0}, {"D_800591B2", 1}, {"D_800591B3", 1},
};
#define FRONTIER_GLOBAL_COUNT (sizeof(kFrontierGlobals) / sizeof(kFrontierGlobals[0]))
static struct {
    uint8_t *host;      /* the native binding; NULL: seeded in guest RAM */
    uint32_t address;   /* guest address, from the D_XXXXXXXX name */
} frontier_globals[FRONTIER_GLOBAL_COUNT];
static uint32_t frontier_seed_pattern;   /* begin_case()'s fill pattern */
static unsigned frontier_seed_cases[LEAF_COUNT]; /* persistent index per leaf */

static void frontier_globals_bind(const ResolvedData *data, size_t count)
{
    size_t g, d;

    for (g = 0; g < FRONTIER_GLOBAL_COUNT; g++) {
        frontier_globals[g].address =
            (uint32_t)strtoul(kFrontierGlobals[g].name + 2, NULL, 16);
        for (d = 0; d < count; d++) {
            if (strcmp(data[d].name, kFrontierGlobals[g].name) != 0)
                continue;
            frontier_globals[g].host = data[d].host;
        }
        fprintf(stderr, "BATTLE OVERLAY DIFFERENTIAL note: h-frontier global %s %s\n",
                kFrontierGlobals[g].name,
                frontier_globals[g].host ? "native, native-global write check"
                                         : "not native, RAM diff");
    }
}

static void frontier_globals_seed(uint32_t pattern)
{
    frontier_seed_pattern = pattern;
}

/* Before the case's snapshot (frontier_case_setup). */
static void frontier_globals_apply(const char *name)
{
    uint8_t seed[4];
    unsigned g, k, leaf_index;

    for (leaf_index = 0; leaf_index < LEAF_COUNT; leaf_index++)
        if (strcmp(kLeaves[leaf_index].name, name) == 0)
            break;
    assert(leaf_index < LEAF_COUNT);
    for (k = 0; k < 4; k++)
        seed[k] = frontier_seed_pattern == 0   ? 0u
                  : frontier_seed_pattern == 1 ? 0xA5u
                  : (uint8_t)((frontier_seed_cases[leaf_index] * 5u + k * 3u) & 3u);
    frontier_seed_cases[leaf_index]++;
    for (g = 0; g < FRONTIER_GLOBAL_COUNT; g++) {
        uint8_t byte = seed[kFrontierGlobals[g].group & 3u];
        if (frontier_globals[g].host != NULL)
            frontier_globals[g].host[0] = byte;
        else
            *((uint8_t *)PSX_ADDR(frontier_globals[g].address)) = byte;
    }
}

/* Overlay pointer tables the lifted window family (main49/main6/main8,
 * frontier step 2) indexes by window slot.  Unseeded they read as zero and
 * every case faults at a low address, so in a seed_pointers case each of
 * the 8 slots gets a live target inside the pattern-filled pointer window,
 * as kPointerGlobals does (D_800D2D28 and D_800C3EA4 are already there):
 * the prim blocks at PTRT, the rect records clear of the argument words.
 * Only for the leaves listed below: seeding these words for every leaf
 * would change other leaves' inputs.  begin_case() records whether the case
 * is a pointer-seeded one; compare() seeds before its snapshot. */
static const struct {
    uint32_t base;
    unsigned count;
    uint32_t target;
} kFrontierPointerTables[] = {
    {0x800D2E38u, 8u, PTRT},          /* D_800D2E38: window prim blocks (0x5A8) */
    {0x800D2D90u, 8u, PTRT + 0x380u}, /* D_800D2D90: window rects (0xE) */
};
static const char *const kFrontierWindowLeaves[] = {
    "func_8007819C", "func_8008F8F4", "func_80077454", "func_8008F6E4",
    "func_8008DC34", "func_8008DE04", "func_8008E430", "func_8008EA70",
    "func_8008F0A8",
};
static int frontier_case_seeded;

static void frontier_pointer_seed(int seed_pointers)
{
    frontier_case_seeded = seed_pointers;
}

static void frontier_case_setup(const char *name)
{
    unsigned t, k;
    int listed = 0;

    /* 800BB248 (frontier step 4, a slot actor's work callback) reads its
     * actor through task +4: give the pointer-seeded task (PTRT) a live,
     * pattern-filled actor at PTRT + 0x100. */
    frontier_globals_apply(name);
    if (frontier_case_seeded && strcmp(name, "func_800BB248") == 0)
        *((uint32_t *)PSX_ADDR(PTRT + 4u)) = PTRT + 0x100u;
    for (k = 0; k < sizeof(kFrontierWindowLeaves) / sizeof(kFrontierWindowLeaves[0]); k++)
        listed |= strcmp(name, kFrontierWindowLeaves[k]) == 0;
    if (!frontier_case_seeded || !listed)
        return;
    for (t = 0; t < sizeof(kFrontierPointerTables) / sizeof(kFrontierPointerTables[0]); t++)
        for (k = 0; k < kFrontierPointerTables[t].count; k++)
            *((uint32_t *)PSX_ADDR(kFrontierPointerTables[t].base + 4u * k)) =
                kFrontierPointerTables[t].target;
}
/* ---- END h-frontier ------------------------------------------------------ */
static PcPortMipsCpu cpu;
static uint8_t image[IMAGE_MAX];
static size_t image_size;
static uint8_t before[PSX_RAM_SIZE];
static uint8_t expected[PSX_RAM_SIZE];
static uint8_t actual[PSX_RAM_SIZE];

static unsigned checks;
static unsigned inconclusive;
static unsigned failures;
static unsigned hand_only;
/* Per-leaf count of comparisons that actually completed. A leaf with none is
 * reported UNPROVEN: being allowlisted without evidence is not a claim this
 * harness is willing to make. */
static unsigned proven[LEAF_COUNT];
static unsigned inconclusive_logged[LEAF_COUNT];
static unsigned inconclusive_per_leaf[LEAF_COUNT];

/* Bumped by every generated bridge placeholder (see the runner): a callee that
 * neither the link nor PsyCross provides. Both sides reach the same
 * placeholder -- the interpreter resolves it through the bridge map -- and
 * compare an undefined return register, so a case that touched one proves
 * nothing and is counted inconclusive. */
unsigned g_prover_placeholder_hits;

/* Spied main-executable services (SPIES in the run script): each call is
 * logged with its first argc argument words, each CLASSIFIED (spy_classify)
 * and compared pairwise (spy_logs_match), precisely:
 *  - GUEST: a guest address -- a host pointer into g_PsxRam, the scratchpad
 *    or a runtime-bound main-executable global (rt.data, the copy retail's
 *    loads and the host bodies' symbols share), or a guest-shaped word --
 *    compared by its canonical guest address;
 *  - frames: the two sides keep their frames in different places by design.
 *    A retail word in the interpreter's stack window (STACK_SKIP) is a frame
 *    address; it must meet a host frame address -- a guest stack-window
 *    address, a word inside this executable's image that no binding claims
 *    (a lifted body's static frame area, bl_stack), or a host stack address
 *    -- and frames match by IDENTITY: within a case, each distinct retail
 *    frame address always meets the same host frame address and vice versa;
 *  - anything else is compared by its low 32 bits.
 * The retail side never holds a host image address, so a retail word inside
 * the image range is a scalar and is compared exactly.  (Before, every word
 * in the image range -- scalar or pointer -- and every stack-window word
 * collapsed into one STACK token on both sides, so two different scalars
 * there, or two different frame slots, compared equal.) */
#define SPY_MAX 256u
enum {
    SPY_SCALAR = 0,  /* low 32 bits */
    SPY_GUEST,       /* canonical guest address */
    SPY_GSTACK,      /* canonical guest address in the stack window */
    SPY_IMAGE,       /* host address inside this executable, no binding */
    SPY_HSTACK       /* host stack address */
};
typedef struct {
    const char *name;
    unsigned argc;
    uint8_t kind[12];
    uint32_t args[12];            /* canonical guest address or low 32 bits */
    unsigned long long raw[12];   /* the word as the spy received it */
} SpyCall;
static SpyCall spy_log[SPY_MAX], spy_expected[SPY_MAX];
static unsigned spy_count, spy_expected_count, spy_overflow;
extern char __executable_start[], _end[];
static int guest_shaped(uint32_t a);
/* Step-11 switched-stack window; zero outside BB6E0's ready dispatch. */
static uint32_t frontier_heap_stack_lo, frontier_heap_stack_hi;
static int frontier_archive_active;

static uint8_t spy_classify(unsigned long long raw, uintptr_t host_sp,
                            uint32_t *canonical)
{
    uintptr_t p = (uintptr_t)raw;
    uintptr_t ram = (uintptr_t)g_PsxRam, pad = (uintptr_t)g_PsxScratchpad;
    uint32_t v = (uint32_t)raw;
    size_t d;

    if (p >= ram && p < ram + PSX_RAM_SIZE) {
        v = 0x80000000u | (uint32_t)(p - ram);
    } else if (p >= pad && p < pad + 0x1000u) {
        v = 0x1F800000u | (uint32_t)(p - pad);
    } else {
        for (d = 0; d < rt.data_count; d++) {
            uintptr_t h = (uintptr_t)rt.data[d].host;
            if (h != 0 && p >= h && p - h < rt.data[d].size) {
                *canonical = rt.data[d].address + (uint32_t)(p - h);
                return SPY_GUEST;
            }
        }
        *canonical = v;
        if ((raw >> 32) == 0 && guest_shaped(v))
            ;  /* a guest word, classified below */
        else if (p >= (uintptr_t)__executable_start && p < (uintptr_t)_end)
            return SPY_IMAGE;
        else if (p + 0x800000u >= host_sp && p < host_sp + 0x100000u)
            return SPY_HSTACK;
        else
            return SPY_SCALAR;
    }
    *canonical = v;
    if ((v & 0xFF800000u) == 0x80000000u || (v & 0xFF800000u) == 0xA0000000u) {
        uint32_t off = v & 0x1FFFFFu;
        if ((off >= STACK_SKIP_LO && off < STACK_SKIP_HI) ||
            (frontier_heap_stack_hi != 0 && off >= frontier_heap_stack_lo &&
             off < frontier_heap_stack_hi))
            return SPY_GSTACK;
    }
    return SPY_GUEST;
}

unsigned long long prover_spy(const char *name, unsigned argc,
                              const unsigned long long *args, unsigned ret);
unsigned long long prover_spy(const char *name, unsigned argc,
                              const unsigned long long *args, unsigned ret)
{
    unsigned i;
    char probe;
    if (spy_count >= SPY_MAX) {
        spy_overflow = 1;
        return ret;
    }
    memset(&spy_log[spy_count], 0, sizeof(SpyCall));
    spy_log[spy_count].name = name;
    spy_log[spy_count].argc = argc;
    for (i = 0; i < 12 && i < argc; i++) {
        spy_log[spy_count].raw[i] = args[i];
        spy_log[spy_count].kind[i] =
            spy_classify(args[i], (uintptr_t)&probe, &spy_log[spy_count].args[i]);
    }
    spy_count++;
    return ret;
}

/* Retail (spy_expected) against host (spy_log), call by call and argument
 * by argument (see the classes above).  Returns 1 when they match; else 0,
 * with the first differing call in *at and the argument in *arg (argc for a
 * name / count difference). */
#define SPY_FRAME_PAIRS 64u
/* Matched spied arguments by rule (the spy-precision summary line): frame
 * identities, words inside the image range compared exactly (folded to one
 * token before), and guest addresses. */
static unsigned long spy_frame_args, spy_image_exact, spy_guest_args;
static int spy_logs_match(unsigned *at, unsigned *arg)
{
    uint32_t frame_r[SPY_FRAME_PAIRS];
    unsigned long long frame_h[SPY_FRAME_PAIRS];
    unsigned frames = 0, k, i, f;
    unsigned long n_frame = 0, n_image = 0, n_guest = 0;

    for (k = 0; k < spy_count && k < spy_expected_count; k++) {
        const SpyCall *r = &spy_expected[k], *h = &spy_log[k];
        *at = k;
        *arg = r->argc;
        if (strcmp(r->name, h->name) != 0 || r->argc != h->argc)
            return 0;
        for (i = 0; i < 12 && i < r->argc; i++) {
            *arg = i;
            if (r->kind[i] == SPY_GSTACK) {
                unsigned long long hk;
                if (h->kind[i] != SPY_GSTACK && h->kind[i] != SPY_IMAGE &&
                    h->kind[i] != SPY_HSTACK)
                    return 0;
                hk = h->kind[i] == SPY_GSTACK ? h->args[i] : h->raw[i];
                for (f = 0; f < frames; f++)
                    if (frame_r[f] == r->args[i] || frame_h[f] == hk)
                        break;
                if (f < frames) {
                    if (frame_r[f] != r->args[i] || frame_h[f] != hk)
                        return 0;
                } else if (frames < SPY_FRAME_PAIRS) {
                    frame_r[frames] = r->args[i];
                    frame_h[frames] = hk;
                    frames++;
                } else {
                    return 0;
                }
                n_frame++;
            } else if (r->args[i] != h->args[i] || h->kind[i] == SPY_HSTACK ||
                       h->kind[i] == SPY_GSTACK) {
                return 0;
            } else if (r->kind[i] == SPY_IMAGE || h->kind[i] == SPY_IMAGE) {
                n_image++;
            } else if (r->kind[i] == SPY_GUEST) {
                n_guest++;
            }
        }
    }
    *at = k;
    *arg = 0;
    if (spy_count != spy_expected_count)
        return 0;
    spy_frame_args += n_frame;
    spy_image_exact += n_image;
    spy_guest_args += n_guest;
    return 1;
}

static const char *spy_kind_tag(const SpyCall *c, unsigned i)
{
    static const char *const tags[] = {"S", "G", "F", "I", "H"};
    return i < c->argc && i < 12 && c->kind[i] < 5 ? tags[c->kind[i]] : "-";
}

static uint32_t rng_state = 0x12345678u;
static uint32_t rnd(void)
{
    rng_state = rng_state * 1664525u + 1013904223u;
    return rng_state >> 8;
}

static void fail(const char *name, const char *what, const char *detail)
{
    fprintf(stderr, "BATTLE OVERLAY DIFFERENTIAL FAIL %s %s %s\n", name, what,
            detail);
    failures++;
}

static void begin_case(uint32_t pattern, int seed_pointers)
{
    uint32_t a;
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memcpy(PSX_ADDR(OVERLAY_BASE), image, image_size);
    for (a = SCRATCH; a < SCRATCH_END; a++) {
        uint8_t v;
        if (pattern == 0)
            v = 0;
        else if (pattern == 1)
            v = 0xA5;
        else
            v = (uint8_t)rnd();
        *((uint8_t *)PSX_ADDR(a)) = v;
    }
    /* Small values only (0..3). Bodies index the character records with
     * bytes read from g_GameState; the shared binding covers 0x2358 bytes, and
     * past it the interpreter falls through to guest RAM while native code
     * keeps the native array. A wild index therefore diverges for a reason
     * that is the harness's binding, not the body, so keep indices in range
     * while still giving id comparisons something to match. */
    if (native_gs != NULL) {
        for (a = 0; a < NATIVE_GS_SIZE; a++) {
            uint8_t v;
            if (pattern == 0)
                v = 0;
            else if (pattern == 1)
                v = 1;
            else
                v = (uint8_t)(rnd() & 3u);
            native_gs[a] = v;
        }
    }
    frontier_globals_seed(pattern); /* h-frontier */
    /* Seeded after the fill: a pointer global that the pattern would leave null
     * (pattern 0) or wild (patterns 1-2) gets a live target instead, which is
     * what lets a body that dereferences it finish on both sides. The target
     * window then gets the pattern too, so a body that reads through a pointer
     * sees pattern data instead of the zero fill. */
    if (seed_pointers) {
        unsigned g;
        for (g = 0; g < POINTER_GLOBAL_COUNT; g++)
            *((uint32_t *)PSX_ADDR(kPointerGlobals[g])) = PTRT;
        for (a = PTRT; a < PTRT + POINTER_TARGET_SIZE; a++) {
            uint8_t v;
            if (pattern == 0)
                v = 0;
            else if (pattern == 1)
                v = 0xA5;
            else
                v = (uint8_t)rnd();
            *((uint8_t *)PSX_ADDR(a)) = v;
        }
        /* An argument can be a pointer to a pointer (u8**): give the first word
         * of every argument target a live target of its own, so the inner
         * dereference lands in emulated RAM instead of address zero. */
        for (a = 0; a < 4; a++)
            *((uint32_t *)PSX_ADDR(PTRT + 0x100u * a)) = PTRT + 0x200u;
    }
    frontier_pointer_seed(seed_pointers); /* h-frontier */
}

static void load(void)
{
    FILE *f = fopen("disc/battle.bin", "rb");
    if (f == NULL) {
        perror("disc/battle.bin");
        exit(2);
    }
    image_size = fread(image, 1, sizeof(image), f);
    if (image_size == 0) {
        fprintf(stderr, "BATTLE OVERLAY DIFFERENTIAL FAIL empty disc/battle.bin\n");
        exit(2);
    }
    fclose(f);
}

static void runtime_up(void)
{
    memset(&rt, 0, sizeof(rt));
    g_ActiveBattleRuntime = &rt;
    initialize_runtime(&rt);
    rt.bridge_cpu = &cpu;
}

/* Seeded into t0 on both sides.  func_80079ED8/func_8007A280 return the
 * caller's t0 unchanged on their store path (a3 == 0); see the rule in the
 * return comparison below. */
#define PROVER_T0_SEED 0x7E57A00Du
static unsigned store_pass_through_checks;
/* The guest $sp both sides start a case with (0: BATTLE_STACK_TOP).  Retail
 * battle code runs some work on stacks of its own (800A4654: scratchpad
 * 0x1F8003F8; 800B62C8 / 800BEE2C: a HeapAlloc'd block, 4-byte aligned), and
 * a native body reached from there must make its calls from that $sp. */
static uint32_t case_sp;
/* Cases dropped because the retail side used a non-guest address. */
static unsigned host_ref_cases;
/* Proven cases that made at least one spied service call. */
static unsigned spied_cases;

static int is_store_pass_through(const char *name, const uint32_t *args)
{
    return (args[3] & 0xFFu) == 0 &&
           (strcmp(name, "func_80079ED8") == 0 ||
            strcmp(name, "func_8007A280") == 0);
}

/* Instruction coverage of the retail side for the larger adopted leaves:
 * the distinct instruction addresses of the leaf's own body (the retail
 * extent, asm/battle/.../func_X.s) that the interpreter executed in cases
 * that were then PROVEN.  The interpreter consults the bus bridge before
 * every instruction, so wrapping it sees each executed pc. */
typedef struct {
    const char *name;
    uint32_t start;
    unsigned insns;
} CoverageLeaf;
static const CoverageLeaf kCoverage[] = {
    {"func_800BA8F4", 0x800BA8F4u, 36},
    {"func_800BB13C", 0x800BB13Cu, 67},
    {"func_800BAC50", 0x800BAC50u, 27},
    {"func_80076AC8", 0x80076AC8u, 14},
    {"func_80076C34", 0x80076C34u, 17},
    {"func_800AAA20", 0x800AAA20u, 69},
    {"func_800A3E98", 0x800A3E98u, 257},
    {"func_800A0838", 0x800A0838u, 1222},
    {"func_800AAD54", 0x800AAD54u, 3186},
    {"func_800AFF9C", 0x800AFF9Cu, 49},
    {"func_8009EF3C", 0x8009EF3Cu, 162},
    {"func_8009F1C4", 0x8009F1C4u, 251},
    {"func_800AE2A4", 0x800AE2A4u, 786},
    {"func_80077364", 0x80077364u, 60},
    {"func_80076C78", 0x80076C78u, 28},
    {"func_80076CE8", 0x80076CE8u, 28},
    {"func_800780A8", 0x800780A8u, 61},
    {"func_800B3B94", 0x800B3B94u, 38},
    {"func_800C11CC", 0x800C11CCu, 928}, /* h-hotpath */
    {"func_800BB844", 0x800BB844u, 100}, /* h-hotpath */
    {"func_800BD810", 0x800BD810u, 89},  /* h-hotpath */
    {"func_800BC460", 0x800BC460u, 401}, /* h-hotpath */
    {"func_80075168", 0x80075168u, 500}, /* h-hotpath */
    {"func_80075938", 0x80075938u, 696}, /* h-hotpath */
    {"func_800745EC", 0x800745ECu, 307}, /* h-hotpath */
    {"func_800743A4", 0x800743A4u, 70}, /* h-hotpath */
    {"func_800744BC", 0x800744BCu, 38}, /* h-hotpath */
    {"func_80074554", 0x80074554u, 38}, /* h-hotpath */
};
#define COVERAGE_COUNT (sizeof(kCoverage) / sizeof(kCoverage[0]))
#define COVERAGE_MAX_INSNS 4096u
static uint8_t cov_seen[COVERAGE_COUNT][COVERAGE_MAX_INSNS];
/* The scratchpad (g_PsxScratchpad, 0x1F800000) is guest-visible state
 * outside g_PsxRam: both sides start from the same bytes and must leave the
 * same bytes, like RAM.  Before this, a side inherited the other side's
 * scratchpad, so a body reading scratch it did not write in this call would
 * diverge for no reason (seen with the not-adopted 8009F844 draft). */
static uint8_t pad_before[4096], pad_expected[4096];
#ifdef PROVER_REAL_GTE
static uint8_t gte_before[PROVER_GTE_BYTES], gte_expected[PROVER_GTE_BYTES];
/* The 64 COP2 registers as the R3000 reads them (MFC2/CFC2), from a saved
 * register file: the storage of a 16-bit register keeps whatever upper half
 * its last writer left (PsyCross's native helpers store full ints, the
 * interpreter's MTC2 stores the moved word), which no guest code can see.
 * Data: MFC2 (PsyCross's own read normalisation: sign/zero extension, SXYP,
 * IRGB/ORGB from IR1..3).  Control: the s16 registers (RT33, L33, LB3, H,
 * DQA, ZSF3, ZSF4) read sign-extended on hardware. */
static void gte_visible(const uint8_t *file, uint32_t *out)
{
    uint8_t saved[PROVER_GTE_BYTES];
    uint32_t c;
    unsigned r;
    memcpy(saved, gteRegs, PROVER_GTE_BYTES);
    memcpy(gteRegs, file, PROVER_GTE_BYTES);
    for (r = 0; r < 32; r++)
        out[r] = MFC2((int)r);
    for (r = 0; r < 32; r++) {
        c = CFC2((int)r);
        if (r == 4 || r == 12 || r == 20 || r == 26 || r == 27 || r == 29 || r == 30)
            c = (uint32_t)(int32_t)(int16_t)c;
        out[32 + r] = c;
    }
    memcpy(gteRegs, saved, PROVER_GTE_BYTES);
}
#endif
static uint8_t cov_case[COVERAGE_MAX_INSNS];
static int cov_index = -1;
static int (*cov_inner_bridge)(void *, PcPortMipsCpu *, uint32_t);

static int cov_bridge(void *opaque, PcPortMipsCpu *c, uint32_t target)
{
    if (cov_index >= 0) {
        const CoverageLeaf *l = &kCoverage[cov_index];
        if (target >= l->start && target < l->start + 4u * l->insns)
            cov_case[(target - l->start) >> 2] = 1;
    }
    return cov_inner_bridge(opaque, c, target);
}

static int coverage_slot(const char *name)
{
    unsigned i;
    for (i = 0; i < COVERAGE_COUNT; i++)
        if (strcmp(kCoverage[i].name, name) == 0)
            return (int)i;
    return -1;
}

static void coverage_report(void)
{
    unsigned i, k, n;
    for (i = 0; i < COVERAGE_COUNT; i++) {
        for (k = 0, n = 0; k < kCoverage[i].insns; k++)
            n += cov_seen[i][k];
        printf("BATTLE OVERLAY DIFFERENTIAL coverage %s insns=%u/%u (%.1f%%)\n",
               kCoverage[i].name, n, kCoverage[i].insns,
               100.0 * n / kCoverage[i].insns);
        if (getenv("BATTLE_OVERLAY_DIFFERENTIAL_COVERAGE_MISSES") != NULL) {
            for (k = 0; k < kCoverage[i].insns; k++)
                if (!cov_seen[i][k])
                    fprintf(stderr, "[prover] uncovered %s %08x\n",
                            kCoverage[i].name, kCoverage[i].start + 4u * k);
        }
    }
}

/* Retail-side loads and stores of a word that is not a guest address.  The
 * shared resolver (pc_port/src/battle_guest_resolver.h) hands such a word to
 * the host as a raw -no-pie address, and the interpreter accepts it when it
 * lies in any mapping of this process: a wild pointer built from seeded
 * random bytes then reads or writes the harness's OWN state (the before/
 * expected/actual snapshots, a lifted TU's frame area), which differs
 * between the two sides and between the three mode binaries.  Such a case
 * compares nothing the hardware does (a bus error there), so it is
 * inconclusive, never a match or a divergence. */
static unsigned retail_host_refs;
static int (*retail_inner_read)(void *, uint32_t, unsigned, uint32_t *);
static int (*retail_inner_write)(void *, uint32_t, unsigned, uint32_t);

static int guest_shaped(uint32_t a)
{
    return (a & 0xFF800000u) == 0x80000000u || (a & 0xFF800000u) == 0xA0000000u ||
           (a & 0xFFFFF000u) == 0x1F800000u;
}

/* h-hotpath: set only by a hand case that drives a lifted body through a
 * NULL-based pointer.  The interpreter's bus aliases KUSEG [0, 2 MB) to main
 * RAM as the PS1 does (runtime_read/runtime_write), so for that case the
 * retail side's KUSEG accesses are guest accesses, not host references.
 * Every other case keeps the rule above. */
static int retail_kuseg_guest;
static int retail_guest(uint32_t a)
{
    return guest_shaped(a) || (retail_kuseg_guest && a < 0x00200000u);
}

static int retail_read(void *opaque, uint32_t address, unsigned width, uint32_t *value)
{
    if (!retail_guest(address))
        retail_host_refs++;
    return retail_inner_read(opaque, address, width, value);
}

static int retail_write(void *opaque, uint32_t address, unsigned width, uint32_t value)
{
    if (!retail_guest(address))
        retail_host_refs++;
    return retail_inner_write(opaque, address, width, value);
}

static int run_interpreted(uint32_t target, const uint32_t *args)
{
    unsigned i;
    memset(&cpu, 0, sizeof(cpu));
    initialize_cpu(&cpu, &rt);
    cov_inner_bridge = cpu.bus.bridge;
    cpu.bus.bridge = cov_bridge;
    retail_inner_read = cpu.bus.read;
    retail_inner_write = cpu.bus.write;
    cpu.bus.read = retail_read;
    cpu.bus.write = retail_write;
    retail_host_refs = 0;
    if (case_sp != 0)
        cpu.gpr[29] = case_sp;
    memset(cov_case, 0, sizeof(cov_case));
    for (i = 0; i < 4; i++)
        cpu.gpr[4 + i] = args[i];
    cpu.gpr[8] = PROVER_T0_SEED;
    rt.force_interpret = 1;
    /*
     * force_interpret must be cleared before the next host call; PcPortMipsRun
     * only returns here.
     */
    return PcPortMipsRun(&cpu, target, BATTLE_HALT_PC, STEP_LIMIT);
}

static int run_host(uint32_t target, const uint32_t *args)
{
    unsigned i;
    memset(&cpu, 0, sizeof(cpu));
    initialize_cpu(&cpu, &rt);
    for (i = 0; i < 4; i++)
        cpu.gpr[4 + i] = args[i];
    cpu.gpr[8] = PROVER_T0_SEED;
    if (case_sp != 0)
        cpu.gpr[29] = case_sp;
    rt.force_interpret = 0;
    return runtime_bridge(&rt, &cpu, target);
}

/* The interpreter validates every address through its bus; the host C body runs
 * on the real CPU and does not. A body that computes an out-of-range guest
 * address (the 2 MiB wrap the hardware does and the port does not model) walks
 * off g_PsxRam and faults instead of returning. Catch that so one runaway body
 * cannot abort the whole sweep and hide every other leaf's result; the case is
 * reported as a divergence, because retail completed it. */
static sigjmp_buf host_fault_jmp;

static void host_fault_handler(int sig)
{
    (void)sig;
    siglongjmp(host_fault_jmp, 1);
}

static uint32_t leaf_address(const char *name)
{
    /* "func_XXXXXXXX" -> 0xXXXXXXXX */
    return (uint32_t)strtoul(name + 5, NULL, 16);
}

/* Native-global write check, for every case.  The runtime binds main-
 * executable globals (rt.data) to native storage that the interpreted retail
 * side and the host bodies share.  Their writes land outside g_PsxRam, so the
 * RAM diff cannot see them, and the host side used to start from whatever the
 * retail side had left there.  compare() now snapshots every binding when it
 * starts, gives the host side the same starting bytes, and requires both sides
 * to leave the same bytes ("native" FAIL naming the binding).  Bindings that
 * alias guest RAM or the scratchpad are left to those diffs.  (g_GameState also
 * keeps its seeded snapshot and its own check above this one.) */
static uint8_t *native_watch_start, *native_watch_retail;
static size_t native_watch_bytes;
static unsigned native_watch_count;

static int native_watch_skip(const ResolvedData *d)
{
    uintptr_t h = (uintptr_t)d->host;
    uintptr_t ram = (uintptr_t)g_PsxRam, pad = (uintptr_t)g_PsxScratchpad;
    if (d->host == NULL || d->size == 0)
        return 1;
    if (h < ram + PSX_RAM_SIZE + 0x1000u && h + d->size > ram)
        return 1;
    return h < pad + 0x1000u && h + d->size > pad;
}

/* which: 0 = the start snapshot, 1 = the retail side's finals. */
static void native_watch_copy(int which, int save)
{
    size_t d, off = 0;
    uint8_t *buf;
    if (native_watch_start == NULL) {
        for (d = 0; d < rt.data_count; d++)
            if (!native_watch_skip(&rt.data[d])) {
                native_watch_bytes += rt.data[d].size;
                native_watch_count++;
            }
        native_watch_start = malloc(native_watch_bytes + 1);
        native_watch_retail = malloc(native_watch_bytes + 1);
        if (native_watch_start == NULL || native_watch_retail == NULL)
            abort();
        printf("BATTLE OVERLAY DIFFERENTIAL native-watch bindings=%u bytes=%zu\n",
               native_watch_count, native_watch_bytes);
    }
    buf = which == 0 ? native_watch_start : native_watch_retail;
    for (d = 0; d < rt.data_count; d++) {
        const ResolvedData *r = &rt.data[d];
        if (native_watch_skip(r))
            continue;
        if (save)
            memcpy(buf + off, r->host, r->size);
        else if (memcmp(r->host, buf + off, r->size) != 0)
            /* Only bytes a side changed are written back: a binding in
             * read-only storage cannot have changed, and writing it would
             * fault outside run_host (prover safety). */
            memcpy(r->host, buf + off, r->size);
        off += r->size;
    }
}

/* 0 when the bindings hold what `retail` recorded; else the first differing
 * binding and offset, formatted into detail. */
static int native_watch_differs(char *detail, size_t len)
{
    size_t d, off = 0, i;
    for (d = 0; d < rt.data_count; d++) {
        const ResolvedData *r = &rt.data[d];
        if (native_watch_skip(r))
            continue;
        for (i = 0; i < r->size; i++) {
            if (native_watch_retail[off + i] != r->host[i]) {
                snprintf(detail, len, "%s+0x%zx (guest %08x) retail=%02x host=%02x",
                         r->name != NULL ? r->name : "?", i,
                         (unsigned)(r->address + i), native_watch_retail[off + i],
                         r->host[i]);
                return 1;
            }
        }
        off += r->size;
    }
    return 0;
}

static void placeholder_inconclusive(const Leaf *leaf, const char *side)
{
    unsigned index = (unsigned)(leaf - kLeaves);
    inconclusive++;
    inconclusive_per_leaf[index]++;
    if (inconclusive_logged[index] == 0) {
        inconclusive_logged[index] = 1;
        fprintf(stderr, "[prover] inconclusive %s: %s side reached a bridge "
                "placeholder callee\n", leaf->name, side);
    }
}

/* Optional per-case setup run identically before EACH side (after the RAM
 * snapshot/restore), for leaves whose native callees need valid state that
 * seeded random RAM cannot provide (the heap for 8008ABB8).  Its own writes
 * land in both sides' RAM alike; placeholder hits it causes are not charged to
 * the body.  force_return_bits overrides the generated return width for a leaf
 * whose bridge returns a guest-domain value (typed 8008ABB8). */
static void (*pre_side)(void);
static unsigned force_return_bits;
/* Outcome of the most recent compare(): set only when it proved a match, so
 * a hand case can require non-vacuous coverage (both result classes). */
static int last_case_proven;
static uint32_t last_retail_v0;

static void compare(const Leaf *leaf, const uint32_t *args)
{
    const char *name = leaf->name;
    uint32_t target = leaf_address(name);
    uint32_t iv0, hv0, mask;
    int irc, hrc;
    uint32_t a;
    size_t diffs = 0;
    uint32_t first = 0;
    char detail[200];

    last_case_proven = 0;
    cov_index = coverage_slot(name);
    frontier_case_setup(name); /* h-frontier */
    memcpy(before, g_PsxRam, sizeof(before));
    memcpy(pad_before, g_PsxScratchpad, sizeof(pad_before));
#ifdef PROVER_REAL_GTE
    memcpy(gte_before, gteRegs, PROVER_GTE_BYTES);
#endif
    native_watch_copy(0, 1);
    if (native_gs != NULL)
        memcpy(native_before, native_gs, NATIVE_GS_SIZE);
    /* Both sides call the same rand(); start each run from the same point or
     * every random-dependent body would differ for no reason. */
    srand(1);
    if (pre_side != NULL)
        pre_side();
    g_prover_placeholder_hits = 0;
    spy_count = 0;
    spy_overflow = 0;
    /* The interpreted side calls main-executable code natively through the
     * bridge (HeapFree, rcos, ...). If that native callee faults, retail did
     * not complete on this input as far as the harness can tell: count the
     * case inconclusive. Without this the handler would jump through a stale
     * buffer left by an earlier host run and blame the wrong side. */
    if (sigsetjmp(host_fault_jmp, 1) != 0) {
        unsigned index = (unsigned)(leaf - kLeaves);
        rt.force_interpret = 0;
        inconclusive++;
        inconclusive_per_leaf[index]++;
        if (inconclusive_logged[index] == 0) {
            inconclusive_logged[index] = 1;
            fprintf(stderr, "[prover] inconclusive %s: a native callee of the "
                    "retail side faulted\n", name);
        }
        return;
    }
    irc = frontier_side_run != NULL ? frontier_side_run(1) /* h-frontier */
                                    : run_interpreted(target, args);
    if (irc == PC_PORT_MIPS_HALTED && g_prover_placeholder_hits != 0) {
        rt.force_interpret = 0;
        placeholder_inconclusive(leaf, "retail");
        return;
    }
    if (irc == PC_PORT_MIPS_HALTED && retail_host_refs != 0) {
        unsigned index = (unsigned)(leaf - kLeaves);
        rt.force_interpret = 0;
        inconclusive++;
        inconclusive_per_leaf[index]++;
        host_ref_cases++;
        if (inconclusive_logged[index] == 0) {
            inconclusive_logged[index] = 1;
            fprintf(stderr, "[prover] inconclusive %s: the retail side "
                    "dereferenced a non-guest (host) address\n", name);
        }
        return;
    }
    if (irc != PC_PORT_MIPS_HALTED) {
        unsigned index = (unsigned)(leaf - kLeaves);
        inconclusive++;
        inconclusive_per_leaf[index]++;
        /* Explain the first skip per leaf: "unproven" is only actionable if the
         * reason is on the record. */
        if (inconclusive_logged[index] == 0) {
            inconclusive_logged[index] = 1;
            fprintf(stderr, "[prover] inconclusive %s rc=%d err=%s\n", name, irc,
                    cpu.error);
        }
        rt.force_interpret = 0;
        return;
    }
    memcpy(expected, g_PsxRam, sizeof(expected));
    native_watch_copy(1, 1);
    memcpy(spy_expected, spy_log, sizeof(SpyCall) * spy_count);
    spy_expected_count = spy_count;
    spy_count = 0;
    iv0 = cpu.gpr[2];
    rt.force_interpret = 0;
    if (native_gs != NULL) {
        memcpy(native_expected, native_gs, NATIVE_GS_SIZE);
        memcpy(native_gs, native_before, NATIVE_GS_SIZE);
    }

    memcpy(g_PsxRam, before, sizeof(g_PsxRam));
    memcpy(pad_expected, g_PsxScratchpad, sizeof(pad_expected));
    memcpy(g_PsxScratchpad, pad_before, sizeof(pad_before));
#ifdef PROVER_REAL_GTE
    memcpy(gte_expected, gteRegs, PROVER_GTE_BYTES);
    memcpy(gteRegs, gte_before, PROVER_GTE_BYTES);
#endif
    native_watch_copy(0, 0);
    srand(1);
    if (pre_side != NULL) {
        unsigned hits = g_prover_placeholder_hits;
        pre_side();
        g_prover_placeholder_hits = hits;
    }
    /* A set-up's own service calls are not the body's: the host side's spy
     * log starts after its pre_side, as the retail side's does.  (Before, a
     * spied call made by the set-up -- e.g. HeapReset's func_8003747C in the
     * 8008ABB8 / 8007FEC4 heap set-up, had it been spied -- led only the host
     * log: it failed a correct body, and it hid a body that dropped the same
     * leading call.) */
    spy_count = 0;
    spy_overflow = 0;
    if (sigsetjmp(host_fault_jmp, 1) != 0) {
        fail(name, "host-fault",
             "host body faulted where the interpreter completed (wild address)");
        return;
    }
    if (frontier_module_only) {
        PcPortMipsExecutionStats start = PcPortMipsExecutionStatsGet();
        hrc = frontier_side_run != NULL ? frontier_side_run(0) /* h-frontier */
                                        : run_host(target, args);
        PcPortMipsExecutionStats end = PcPortMipsExecutionStatsGet();
        if (start.interpreted_instructions == 0 ||
            end.interpreted_instructions != start.interpreted_instructions) {
            fail(name, "native-cpu", "module host interpreted guest instructions or oracle meter inactive");
            return;
        }
        fprintf(stderr, "[module-proof] %s host_interpreted_instructions=0\n", name);
    } else
        hrc = frontier_side_run != NULL ? frontier_side_run(0) /* h-frontier */
                                        : run_host(target, args);
    if (hrc != 1) {
        snprintf(detail, sizeof(detail), "rc=%d (not adopted)", hrc);
        fail(name, "bridge", detail);
        return;
    }
    memcpy(actual, g_PsxRam, sizeof(actual));
    hv0 = cpu.gpr[2];
    if (g_prover_placeholder_hits != 0) {
        placeholder_inconclusive(leaf, "host");
        return;
    }
    if (spy_overflow) {
        unsigned index = (unsigned)(leaf - kLeaves);
        inconclusive++;
        inconclusive_per_leaf[index]++;
        if (inconclusive_logged[index] == 0) {
            inconclusive_logged[index] = 1;
            fprintf(stderr, "[prover] inconclusive %s: more than %u spied calls\n",
                    name, SPY_MAX);
        }
        return;
    }
    {
        unsigned k, ai;
        if (!spy_logs_match(&k, &ai)) {
            /* Classes: S scalar, G guest address, F guest stack frame,
             * I host image address, H host stack address.  The argument the
             * comparison stopped at is shown as `@argN` (argc: name/count). */
            const SpyCall *r = k < spy_expected_count ? &spy_expected[k] : NULL;
            const SpyCall *h = k < spy_count ? &spy_log[k] : NULL;
            unsigned j = ai < 12 ? ai : 0;
            snprintf(detail, sizeof(detail),
                     "spied calls differ at #%u @arg%u (retail %u, host %u): "
                     "retail %s(%s:%08x,%s:%08x) host %s(%s:%08x,%s:%08x)",
                     k, ai, spy_expected_count, spy_count,
                     r ? r->name : "-", r ? spy_kind_tag(r, j) : "-",
                     r ? r->args[j] : 0u,
                     r ? spy_kind_tag(r, j + 1) : "-", r ? r->args[j + 1 < 12 ? j + 1 : j] : 0u,
                     h ? h->name : "-", h ? spy_kind_tag(h, j) : "-",
                     h ? h->args[j] : 0u,
                     h ? spy_kind_tag(h, j + 1) : "-", h ? h->args[j + 1 < 12 ? j + 1 : j] : 0u);
            fail(name, "spy", detail);
            return;
        }
    }
    spied_cases += spy_count != 0;

    /* Compare only the bits the body's return type defines. A void body leaves
     * whatever the callee happened to have in rax, and a u8/u16 return leaves
     * the upper bits undefined by the ABI. Retail's v1 is not part of any of
     * these functions' contract either. */
    switch (force_return_bits != 0 ? force_return_bits : leaf->return_bits) {
    case 8:  mask = 0xFFu; break;
    case 16: mask = 0xFFFFu; break;
    case 32: mask = 0xFFFFFFFFu; break;
    default: mask = 0; break;
    }
    /* Store path of the two table accessors: retail's v0 is the caller's t0
     * passed through (jr ra; addu v0, t0, zero), which a C body cannot
     * reproduce.  Every retail jal site and every native caller was checked to
     * not read v0 after a store (native-audit/battle-decomp/callerscan), so
     * the port body returns a defined 0 there.  Prove both halves of that
     * statement instead of comparing the two values. */
    if (mask != 0 && is_store_pass_through(name, args)) {
        if (iv0 != PROVER_T0_SEED || (hv0 & mask) != 0) {
            snprintf(detail, sizeof(detail),
                     "store-path v0 retail=%08x (expected t0 %08x) host=%08x (expected 0) a0=%08x a1=%08x a2=%08x",
                     iv0, PROVER_T0_SEED, hv0, args[0], args[1], args[2]);
            fail(name, "return", detail);
            return;
        }
        store_pass_through_checks++;
        mask = 0;
    }
    if (mask != 0 && (iv0 & mask) != (hv0 & mask)) {
        snprintf(detail, sizeof(detail),
                 "v0 retail=%08x host=%08x mask=%08x a0=%08x a1=%08x a2=%08x a3=%08x",
                 iv0, hv0, mask, args[0], args[1], args[2], args[3]);
        fail(name, "return", detail);
        return;
    }
    for (a = 0; a < PSX_RAM_SIZE; a++) {
        if ((a >= STACK_SKIP_LO && a < STACK_SKIP_HI) ||
            (frontier_heap_stack_hi != 0 && a >= frontier_heap_stack_lo &&
             a < frontier_heap_stack_hi))
            continue;
        if (expected[a] != actual[a]) {
            if (diffs == 0)
                first = a;
            diffs++;
        }
    }
    if (diffs != 0) {
        snprintf(detail, sizeof(detail),
                 "%zu bytes differ, first at guest %08x retail=%02x host=%02x "
                 "a0=%08x a1=%08x a2=%08x a3=%08x",
                 diffs, (unsigned)(0x80000000u | first), expected[first],
                 actual[first], args[0], args[1], args[2], args[3]);
        fail(name, "ram", detail);
        return;
    }
    for (a = 0; a < sizeof(pad_expected); a++) {
        /* A scratchpad-stack case: the retail frames live below its $sp,
         * the host body's frame does not (as STACK_SKIP for RAM). */
        if ((case_sp & 0xFFFFF000u) == 0x1F800000u && a < (case_sp & 0xFFFu))
            continue;
        if (pad_expected[a] != g_PsxScratchpad[a]) {
            snprintf(detail, sizeof(detail),
                     "scratchpad differs, first at %08x retail=%02x host=%02x a0=%08x a1=%08x",
                     0x1F800000u + (unsigned)a, pad_expected[a], g_PsxScratchpad[a],
                     args[0], args[1]);
            fail(name, "scratchpad", detail);
            return;
        }
    }
#ifdef PROVER_REAL_GTE
    {
        uint32_t want[64], got[64];
        unsigned r;
        gte_visible(gteRegs, got);
        gte_visible(gte_expected, want);
        for (r = 0; r < 64; r++) {
            if (want[r] != got[r]) {
                snprintf(detail, sizeof(detail),
                         "GTE %s register %u retail=%08x host=%08x a0=%08x a1=%08x",
                         r < 32 ? "data" : "control", r & 31u, want[r], got[r],
                         args[0], args[1]);
                fail(name, "gte", detail);
                return;
            }
        }
    }
#endif
    if (native_gs != NULL) {
        for (a = 0; a < NATIVE_GS_SIZE; a++) {
            if (native_expected[a] != native_gs[a]) {
                if (diffs == 0)
                    first = a;
                diffs++;
            }
        }
        if (diffs != 0) {
            snprintf(detail, sizeof(detail),
                     "%zu bytes differ, first at g_GameState+%04x retail=%02x "
                     "host=%02x", diffs, (unsigned)first,
                     native_expected[first], native_gs[first]);
            fail(name, "native", detail);
            return;
        }
    }
    if (native_watch_differs(detail, sizeof(detail))) {
        fail(name, "native", detail);
        return;
    }
    checks++;
    proven[(unsigned)(leaf - kLeaves)]++;
    last_case_proven = 1;
    {
        int slot = coverage_slot(name);
        unsigned k;
        if (slot >= 0)
            for (k = 0; k < kCoverage[slot].insns; k++)
                cov_seen[slot][k] |= cov_case[k];
    }
    last_retail_v0 = iv0;
}

static const Leaf *leaf_by_name(const char *name)
{
    unsigned i;
    for (i = 0; i < LEAF_COUNT; i++) {
        if (strcmp(kLeaves[i].name, name) == 0)
            return &kLeaves[i];
    }
    fprintf(stderr, "BATTLE OVERLAY DIFFERENTIAL FAIL unknown leaf %s\n", name);
    exit(2);
}

static void compare_name(const char *name, const uint32_t *args)
{
    compare(leaf_by_name(name), args);
}

/* 8008ABB8 = HeapChangeCurrentUser(2, 0); return HeapAlloc(size, mode).
 * Both sides reach the real native allocator, whose blocks live in guest RAM
 * and whose globals (g_Heap, current user/content, consolidation flag) are
 * native, so every side starts from the same freshly initialised 128 KiB heap.
 * The typed bridge returns the guest address, so all 32 bits of v0 are
 * compared, unlike the generic pointer-return rule. */
#define ABB8_HEAP_LO 0x80140000u
#define ABB8_HEAP_HI 0x80160000u
extern void HeapInit(void *pHeapStart, void *pHeapEnd);
static void abb8_heap(void)
{
    HeapInit(PSX_ADDR(ABB8_HEAP_LO), PSX_ADDR(ABB8_HEAP_HI));
}

static void hand_cases_8008ABB8(void)
{
    static const uint32_t sizes[] = {0u, 4u, 0x10Cu, 0x2F8u, 0xA2B4u, 0x8000u};
    uint32_t args[4];
    unsigned s, mode;

    pre_side = abb8_heap;
    force_return_bits = 32;
    for (s = 0; s < sizeof(sizes) / sizeof(sizes[0]); s++) {
        for (mode = 0; mode <= 1; mode++) {
            begin_case(s % 3u, 0);
            args[0] = sizes[s]; args[1] = mode; args[2] = args[3] = 0;
            compare_name("func_8008ABB8", args);
        }
    }
    pre_side = NULL;
    force_return_bits = 0;
}


/* ---- Battle guest ABI prototype (M-a): mesh fixture --------------------
 * The main69.c ground-mesh family reads two guest tables through guest
 * pointers held in overlay globals: D_800D3344 -> 8-byte SVECTOR vertices,
 * D_800D39CC -> 14-byte triangles (s16 v0,v1,v2; s16 neighbours n0,n1,n2,
 * -1 = none; u8 +0xC attribute; u8 +0xD pass mark), D_800D3348 = triangle
 * count, D_800D2F64 = pass counter.  The fixture is a 4x4-vertex grid (18
 * triangles) with real neighbour links, in either winding, so the
 * point-in-triangle test sees both inside and outside points. */
#define MESH_VERT   0x800C6000u
#define MESH_TRI    0x800C6800u
#define MESH_PT     0x800C7000u
#define MESH_NORMAL 0x800C7100u
#define MESH_N      4
static unsigned mesh_tri_count;

static void mesh_fixture(int winding, int seed)
{
    int i, j, t, u, e, f;
    int16_t tri[2 * (MESH_N - 1) * (MESH_N - 1)][3];

    for (i = 0; i < MESH_N; i++) {
        for (j = 0; j < MESH_N; j++) {
            int16_t *v = (int16_t *)PSX_ADDR(MESH_VERT + 8u * (unsigned)(i * MESH_N + j));
            v[0] = (int16_t)(i * 256 - 384 + ((i * 7 + j * 3 + seed) % 5) * 9);
            v[1] = (int16_t)((i * 37 + j * 53 + seed * 11) % 97 - 48);
            v[2] = (int16_t)(j * 256 - 384 + ((i * 5 + j * 11 + seed) % 7) * 6);
            v[3] = 0;
        }
    }
    t = 0;
    for (i = 0; i + 1 < MESH_N; i++) {
        for (j = 0; j + 1 < MESH_N; j++) {
            int16_t a = (int16_t)(i * MESH_N + j), b = (int16_t)((i + 1) * MESH_N + j);
            int16_t c = (int16_t)(i * MESH_N + j + 1), d = (int16_t)((i + 1) * MESH_N + j + 1);
            tri[t][0] = a; tri[t][1] = winding ? d : b; tri[t][2] = winding ? b : d; t++;
            tri[t][0] = a; tri[t][1] = winding ? c : d; tri[t][2] = winding ? d : c; t++;
        }
    }
    mesh_tri_count = (unsigned)t;
    for (u = 0; u < t; u++) {
        uint8_t *r = (uint8_t *)PSX_ADDR(MESH_TRI + 14u * (unsigned)u);
        int16_t *h = (int16_t *)r;
        h[0] = tri[u][0]; h[1] = tri[u][1]; h[2] = tri[u][2];
        for (e = 0; e < 3; e++) {
            int16_t p0 = tri[u][e], p1 = tri[u][(e + 1) % 3];
            int found = -1;
            for (f = 0; f < t && found < 0; f++) {
                int k;
                if (f == u) continue;
                for (k = 0; k < 3; k++) {
                    if (tri[f][k] == p1 && tri[f][(k + 1) % 3] == p0)
                        found = f;
                }
            }
            h[3 + e] = (int16_t)found;
        }
        r[0xC] = (uint8_t)(u ^ 0x5A);
        r[0xD] = 0;
    }
    *(uint32_t *)PSX_ADDR(0x800D3344u) = MESH_VERT;
    *(uint32_t *)PSX_ADDR(0x800D39CCu) = MESH_TRI;
    *(uint32_t *)PSX_ADDR(0x800D3348u) = mesh_tri_count;
    *(uint8_t *)PSX_ADDR(0x800D2F64u) = 1;
}

static void mesh_point(int x, int y, int z)
{
    int16_t *p = (int16_t *)PSX_ADDR(MESH_PT);
    p[0] = (int16_t)x; p[1] = (int16_t)y; p[2] = (int16_t)z; p[3] = 0;
}

/* Fails the run unless `name` proved a case for every listed retail result
 * class -- a prover that only ever saw one branch is vacuous. */
static void require_outcomes(const char *name, unsigned seen_mask,
                             unsigned want_mask)
{
    if ((seen_mask & want_mask) != want_mask) {
        char detail[80];
        snprintf(detail, sizeof(detail), "outcome classes seen=%x want=%x",
                 seen_mask, want_mask);
        fail(name, "coverage", detail);
    }
}

static int abi_leaf_listed(const char *name)
{
    unsigned i;
    for (i = 0; i < LEAF_COUNT; i++)
        if (strcmp(kLeaves[i].name, name) == 0)
            return 1;
    return 0;
}

static void hand_cases_mesh(void)
{
    uint32_t args[4];
    int winding, x, z;
    unsigned tri, seen;

    if (!abi_leaf_listed("func_800A5A48"))
        return;
    /* 800A5A48(v0, v1, v2, point): every triangle against a 5x5 point grid
     * that covers the mesh and its outside, both windings.  Outcome class 1 =
     * retail returned -1 (inside), 2 = retail returned 0. */
    seen = 0;
    for (winding = 0; winding < 2; winding++) {
        begin_case(0, 0);
        mesh_fixture(winding, winding + 1);
        for (x = -450; x <= 450; x += 225) {
            for (z = -450; z <= 450; z += 225) {
                mesh_point(x + 17, 5, z - 23);
                for (tri = 0; tri < mesh_tri_count; tri++) {
                    const int16_t *h = (const int16_t *)PSX_ADDR(MESH_TRI + 14u * tri);
                    args[0] = MESH_VERT + 8u * (uint16_t)h[0];
                    args[1] = MESH_VERT + 8u * (uint16_t)h[1];
                    args[2] = MESH_VERT + 8u * (uint16_t)h[2];
                    args[3] = MESH_PT;
                    compare_name("func_800A5A48", args);
                    if (last_case_proven)
                        seen |= last_retail_v0 == 0xFFFFFFFFu ? 1u : 2u;
                }
            }
        }
    }
    require_outcomes("func_800A5A48", seen, 3u);

    /* 800A579C(point): first containing triangle or -1; plus the
     * tables-not-loaded early-outs.  Class 1 = a triangle index, 2 = -1. */
    if (abi_leaf_listed("func_800A579C")) {
        seen = 0;
        for (winding = 0; winding < 2; winding++) {
            begin_case(0, 0);
            mesh_fixture(winding, winding + 3);
            for (x = -500; x <= 500; x += 125) {
                for (z = -500; z <= 500; z += 250) {
                    mesh_point(x + 9, 0, z + 31);
                    args[0] = MESH_PT; args[1] = args[2] = args[3] = 0;
                    compare_name("func_800A579C", args);
                    if (last_case_proven)
                        seen |= last_retail_v0 == 0xFFFFFFFFu ? 2u : 1u;
                }
            }
            *(uint32_t *)PSX_ADDR(0x800D39CCu) = 0;
            compare_name("func_800A579C", args);
            *(uint32_t *)PSX_ADDR(0x800D39CCu) = MESH_TRI;
            *(uint32_t *)PSX_ADDR(0x800D3344u) = 0;
            compare_name("func_800A579C", args);
        }
        require_outcomes("func_800A579C", seen, 3u);
    }

    /* 800A5D54(point, tri, depth): neighbour search with pass marks.  The
     * RAM diff covers the +0xD marks it writes.  Class 1 = found, 2 = -1. */
    if (abi_leaf_listed("func_800A5D54")) {
        int depth, start;
        seen = 0;
        for (winding = 0; winding < 2; winding++) {
            for (depth = 0; depth <= 3; depth++) {
                for (start = -1; start < 18; start += 3) {
                    begin_case(0, 0);
                    mesh_fixture(winding, depth + 5);
                    /* Some triangles already carry this pass's mark. */
                    *(uint8_t *)PSX_ADDR(MESH_TRI + 14u * 4u + 0xDu) = 1;
                    mesh_point(-130 + depth * 60, 0, 200 - depth * 90);
                    args[0] = MESH_PT; args[1] = (uint32_t)start;
                    args[2] = (uint32_t)depth; args[3] = 0;
                    compare_name("func_800A5D54", args);
                    if (last_case_proven)
                        seen |= last_retail_v0 == 0xFFFFFFFFu ? 2u : 1u;
                }
            }
        }
        require_outcomes("func_800A5D54", seen, 3u);
    }

    /* 800A5914(point, tri, tries, result): tries loop over 800A5D54, the
     * pass-counter increment and its 0xFF -> 0 wrap that clears every +0xD
     * mark.  Class 1 = found, 2 = -1 / pass-through. */
    if (abi_leaf_listed("func_800A5914")) {
        int tries, start, wrap;
        seen = 0;
        for (wrap = 0; wrap < 2; wrap++) {
            for (tries = 0; tries <= 3; tries++) {
                for (start = -1; start <= 19; start += 4) {
                    begin_case(0, 0);
                    mesh_fixture(tries & 1, tries + 7);
                    if (wrap) {
                        unsigned t;
                        *(uint8_t *)PSX_ADDR(0x800D2F64u) = 0xFF;
                        for (t = 0; t < mesh_tri_count; t++)
                            *(uint8_t *)PSX_ADDR(MESH_TRI + 14u * t + 0xDu) = (uint8_t)(t * 37u);
                    }
                    mesh_point(60 - tries * 70, 0, -40 + tries * 80);
                    args[0] = MESH_PT; args[1] = (uint32_t)start;
                    args[2] = (uint32_t)tries; args[3] = (uint32_t)(0x1234 + tries);
                    compare_name("func_800A5914", args);
                    if (last_case_proven)
                        seen |= (int32_t)last_retail_v0 >= 0 && last_retail_v0 < 0x100u ? 1u : 2u;
                }
            }
            /* Tables not loaded. */
            begin_case(0, 0);
            args[0] = MESH_PT; args[1] = 0; args[2] = 1; args[3] = 0;
            compare_name("func_800A5914", args);
        }
        require_outcomes("func_800A5914", seen, 3u);

        /* Bridge domain (typed thunks): with tries <= 0 the body returns its
         * fourth argument unchanged.  Hand it address-shaped words (KSEG0,
         * KSEG1, scratchpad) through the BRIDGE: retail returns the guest
         * word, so v0 must be that word, not a translated host pointer.  The
         * legacy heuristic bridge (BATTLE_BRIDGE_LEGACY_ARGS) fails here. */
        {
            static const uint32_t kShaped[] = {
                0x80123450u, 0xA0123450u, 0x1F800010u, MESH_PT
            };
            unsigned k, bridge_proven = 0, bridge_cases = 0;
            for (k = 0; k < sizeof(kShaped) / sizeof(kShaped[0]); k++) {
                for (tries = -1; tries <= 0; tries++) {
                    begin_case(0, 0);
                    mesh_fixture(0, 11);
                    mesh_point(10, 0, 10);
                    args[0] = MESH_PT; args[1] = 2;
                    args[2] = (uint32_t)tries; args[3] = kShaped[k];
                    bridge_cases++;
                    compare_name("func_800A5914", args);
                    if (last_case_proven && last_retail_v0 == kShaped[k])
                        bridge_proven++;
                }
            }
            printf("BATTLE OVERLAY DIFFERENTIAL bridge-domain func_800A5914 "
                   "tries<=0 address-shaped a3: proven=%u/%u\n",
                   bridge_proven, bridge_cases);
            if (bridge_proven != bridge_cases)
                fail("func_800A5914", "bridge-domain",
                     "address-shaped pass-through argument not returned in the guest domain");
        }
    }
}

/* Indirect calls from native code (bg_jalr -> PcPort_BattleMipsCallGuest):
 * an adopted guest-ABI leaf must run natively through the guest-address ->
 * native map (g_GuestNativeIndirectCalls counts it) and give retail's v0 and
 * RAM; with force_interpret the same call must still go to the interpreter. */
static void hand_cases_indirect(void)
{
#ifndef BATTLE_BRIDGE_LEGACY_ARGS
    uint32_t args[4];
    unsigned tri, cases = 0, native = 0, match = 0, interp = 0, seen = 0;
    int x;

    if (!abi_leaf_listed("func_800A5A48") || !abi_leaf_listed("func_800A5914"))
        return;
    for (x = -300; x <= 300; x += 300) {
        for (tri = 0; tri < 18; tri += 1) {
            uint32_t iv0, hv0 = 0xDEADBEEFu, cv0 = 0xDEADBEEFu;
            unsigned long n0;
            int irc, ok;
            const int16_t *h;
            uint32_t a;

            begin_case(0, 0);
            mesh_fixture(tri & 1, 13);
            mesh_point(x + 7, 0, 33 - x / 2);
            h = (const int16_t *)PSX_ADDR(MESH_TRI + 14u * tri);
            args[0] = MESH_VERT + 8u * (uint16_t)h[0];
            args[1] = MESH_VERT + 8u * (uint16_t)h[1];
            args[2] = MESH_VERT + 8u * (uint16_t)h[2];
            args[3] = MESH_PT;
            memcpy(before, g_PsxRam, sizeof(before));
            irc = run_interpreted(0x800A5A48u, args);
            rt.force_interpret = 0;
            if (irc != PC_PORT_MIPS_HALTED) {
                fail("func_800A5A48", "indirect", "retail did not halt");
                continue;
            }
            iv0 = cpu.gpr[2];
            memcpy(expected, g_PsxRam, sizeof(expected));
            memcpy(g_PsxRam, before, sizeof(g_PsxRam));
            memset(&cpu, 0, sizeof(cpu));
            initialize_cpu(&cpu, &rt);
            rt.bridge_cpu = &cpu;
            n0 = g_GuestNativeIndirectCalls;
            hv0 = bg_jalr(0x800A5A48u, args[0], args[1], args[2], args[3]);
            cases++;
            if (g_GuestNativeIndirectCalls == n0 + 1)
                native++;
            ok = hv0 == iv0;
            for (a = 0; ok && a < PSX_RAM_SIZE; a++) {
                if (a >= STACK_SKIP_LO && a < STACK_SKIP_HI)
                    continue;
                ok = expected[a] == g_PsxRam[a];
            }
            if (ok) {
                match++;
                seen |= iv0 == 0xFFFFFFFFu ? 1u : 2u;
            }
            /* Control: the same indirect call, interpreter forced. */
            memcpy(g_PsxRam, before, sizeof(g_PsxRam));
            n0 = g_GuestNativeIndirectCalls;
            rt.force_interpret = 1;
            cv0 = bg_jalr(0x800A5A48u, args[0], args[1], args[2], args[3]);
            rt.force_interpret = 0;
            if (g_GuestNativeIndirectCalls == n0 && cv0 == iv0)
                interp++;
        }
    }
    /* Typed kinds on the indirect path too: 800A5914 tries=0 hands back an
     * address-shaped fourth word unchanged, in the guest domain. */
    {
        uint32_t w[4];
        uint32_t v0 = 0;
        unsigned long n0 = g_GuestNativeIndirectCalls;
        begin_case(0, 0);
        mesh_fixture(0, 14);
        mesh_point(10, 0, 10);
        memset(&cpu, 0, sizeof(cpu));
        initialize_cpu(&cpu, &rt);
        rt.bridge_cpu = &cpu;
        w[0] = MESH_PT; w[1] = 2; w[2] = 0; w[3] = 0x80123450u;
        cases++;
        if (PcPort_BattleMipsCallGuest(0x800A5914u, w, 4, &v0) == 0 &&
            g_GuestNativeIndirectCalls == n0 + 1)
            native++;
        if (v0 == 0x80123450u)
            match++;
    }
    printf("BATTLE OVERLAY DIFFERENTIAL indirect-native bg_jalr/CallGuest: "
           "cases=%u native=%u retail-match=%u forced-interpreter-control=%u/%u "
           "outcomes=%u\n", cases, native, match, interp, cases - 1u, seen);
    if (native != cases || match != cases || interp != cases - 1u || seen != 3u)
        fail("func_800A5A48", "indirect",
             "indirect call not served natively with retail's result");
#endif
}

/* 800A5BE8 takes a fifth argument (VECTOR *normal) from the guest stack
 * frame, o32 slot sp+0x10.  Both sides start at BATTLE_STACK_TOP and the
 * bridge reads that slot, so it is written before each side. */
static uint32_t stack_arg4;
static void stack_arg4_side(void)
{
    uint32_t sp = case_sp != 0 ? case_sp : BATTLE_STACK_TOP;
    *(uint32_t *)battle_guest_resolve_native(sp + 0x10u) = stack_arg4;
}

static void hand_cases_mesh_height(void)
{
    uint32_t args[4];
    int winding, x;
    unsigned tri, seen;

    /* 800A5BE8(v0, v1, v2, point, normal): plane normal into *normal and the
     * height into point.vy.  Class 1 = sloped plane, 2 = vertical plane
     * (normal.vy == 0 path), checked on the retail RAM after the case. */
    if (abi_leaf_listed("func_800A5BE8")) {
        seen = 0;
        stack_arg4 = MESH_NORMAL;
        pre_side = stack_arg4_side;
        for (winding = 0; winding < 2; winding++) {
            begin_case(0, 0);
            mesh_fixture(winding, winding + 9);
            for (x = -300; x <= 300; x += 200) {
                for (tri = 0; tri < mesh_tri_count; tri += 2) {
                    const int16_t *h = (const int16_t *)PSX_ADDR(MESH_TRI + 14u * tri);
                    mesh_point(x + 11, 777, -x / 2 + 5);
                    args[0] = MESH_VERT + 8u * (uint16_t)h[0];
                    args[1] = MESH_VERT + 8u * (uint16_t)h[1];
                    args[2] = MESH_VERT + 8u * (uint16_t)h[2];
                    args[3] = MESH_PT;
                    compare_name("func_800A5BE8", args);
                    if (last_case_proven)
                        seen |= *(int32_t *)PSX_ADDR(MESH_NORMAL + 4u) != 0 ? 1u : 2u;
                }
            }
        }
        /* A vertical triangle (plane z = const): normal.vy == 0. */
        begin_case(0, 0);
        {
            int16_t *v = (int16_t *)PSX_ADDR(MESH_VERT);
            v[0] = 0;   v[1] = 0;   v[2] = 40;  v[3] = 0;
            v[4] = 300; v[5] = 0;   v[6] = 40;  v[7] = 0;
            v[8] = 0;   v[9] = 300; v[10] = 40; v[11] = 0;
        }
        mesh_point(50, 999, 60);
        args[0] = MESH_VERT; args[1] = MESH_VERT + 8u; args[2] = MESH_VERT + 16u;
        args[3] = MESH_PT;
        compare_name("func_800A5BE8", args);
        if (last_case_proven)
            seen |= *(int32_t *)PSX_ADDR(MESH_NORMAL + 4u) != 0 ? 1u : 2u;
        pre_side = NULL;
        require_outcomes("func_800A5BE8", seen, 3u);
    }

    /* 800A5870(point, tri, normal): 800A5BE8 on triangle `tri`, returns its
     * +0xC attribute byte; -1 for tri < 0 or tables not loaded. */
    if (abi_leaf_listed("func_800A5870")) {
        int t;
        seen = 0;
        for (winding = 0; winding < 2; winding++) {
            begin_case(0, 0);
            mesh_fixture(winding, winding + 13);
            for (t = -2; t < 18; t += 3) {
                mesh_point(t * 23 - 100, 3, 150 - t * 17);
                args[0] = MESH_PT; args[1] = (uint32_t)t; args[2] = MESH_NORMAL;
                args[3] = 0;
                compare_name("func_800A5870", args);
                if (last_case_proven)
                    seen |= last_retail_v0 == 0xFFFFFFFFu ? 2u : 1u;
            }
        }
        begin_case(0, 0);
        args[0] = MESH_PT; args[1] = 1; args[2] = MESH_NORMAL; args[3] = 0;
        compare_name("func_800A5870", args);
        require_outcomes("func_800A5870", seen, 3u);
    }
}

/* ---- Battle guest ABI prototype: slot and heap fixtures -----------------
 * Board opcodes take ppBoard, the address of a 4-byte GUEST slot holding the
 * script IP (guest address of the operand bytes).  The slot is at PTRT, the
 * operands at SRC, and the D_800D3410/20/30 rows get small values. */
static void slot_fixture(uint32_t salt, int high_bit)
{
    unsigned i;
    uint8_t *ops = (uint8_t *)PSX_ADDR(SRC);
    *(uint32_t *)PSX_ADDR(PTRT) = SRC;
    ops[0] = 0x2A;
    ops[1] = (uint8_t)((salt * 7u + 3u) & 0x3Fu);
    if (high_bit)
        ops[1] |= 0x80u;
    ops[2] = (uint8_t)((salt * 13u + 5u) & 0x3Fu);
    ops[3] = (uint8_t)((salt * 29u + 1u) & 0x3Fu);
    for (i = 0; i < 0x40u * 8u; i++)
        *(uint8_t *)PSX_ADDR(0x800D3410u + i) = (uint8_t)((i * 31u + salt * 17u) & 0xFFu);
}

static void hand_cases_abi_slots(void)
{
    uint32_t args[4];
    uint32_t salt;

    for (salt = 0; salt < 12; salt++) {
        if (abi_leaf_listed("func_8007AE38")) {
            begin_case(0, 0);
            slot_fixture(salt, 0);
            args[0] = PTRT; args[1] = salt % 6u; args[2] = args[3] = 0;
            compare_name("func_8007AE38", args);
        }
        if (abi_leaf_listed("func_8007E8E0")) {
            begin_case(0, 0);
            slot_fixture(salt, 0);
            args[0] = PTRT; args[1] = salt % 5u; args[2] = args[3] = 0;
            compare_name("func_8007E8E0", args);
        }
        if (abi_leaf_listed("func_8007E154")) {
            int arm;
            for (arm = 0; arm < 2; arm++) {
                begin_case(0, 0);
                slot_fixture(salt, arm);
                args[0] = PTRT; args[1] = salt % 4u; args[2] = args[3] = 0;
                compare_name("func_8007E154", args);
            }
        }
    }
    /* 8007FEC4: allocate 0x5DA4 bytes through the native 8008ABB8, store the
     * block's GUEST address at 0x800D2DB4, clear it, and set the two halves
     * at +0x5D9C/+0x5D9E.  The heap fixture is re-initialised for each side;
     * the RAM diff covers the slot word (domain) and the whole block. */
    if (abi_leaf_listed("func_8007FEC4")) {
        int pattern;
        for (pattern = 0; pattern < 3; pattern++) {
            begin_case((uint32_t)pattern, 0);
            pre_side = abb8_heap;
            args[0] = args[1] = args[2] = args[3] = 0;
            compare_name("func_8007FEC4", args);
            pre_side = NULL;
            if (last_case_proven) {
                uint32_t slot = *(uint32_t *)PSX_ADDR(0x800D2DB4u);
                if ((slot & 0xFF800000u) != 0x80000000u)
                    fail("func_8007FEC4", "domain", "0x800D2DB4 does not hold a guest address");
            }
        }
    }
}

/* Codemod sweep batch 1 (ppBoard family, main19/main30): slot fixture with
 * forced operands.  Variant 0 leaves the fixture's pattern bytes, variant 1
 * forces the "low" class (no saturation / comparison false), variant 2 the
 * "high" class (saturation / comparison true).  Every forced variant must be
 * proven for each leaf, so neither result class is left to chance. */
typedef struct SlotSweepLeaf {
    const char *name;
    int wide;      /* 2: u32 at D_800D3410 rows; 1: u16 at D_800D3420; 0: u8 at D_800D3430 */
    int forced;    /* 1 when variants 1/2 mean something for this leaf */
    /* operand values for variant 1 and 2: row[p[1]], row[p[2]] */
    uint32_t lo_a, lo_b, hi_a, hi_b;
    int imm;       /* compare against the immediate p[2] (| p[3] << 8 when wide) */
} SlotSweepLeaf;

static const SlotSweepLeaf kSlotSweep[] = {
    { "func_8007AE98", 0, 1, 9, 4, 4, 9, 0 },             /* u8 sat sub: low = no clamp */
    { "func_8007AFFC", 0, 0, 0, 0, 0, 0, 0 },             /* u8 and */
    { "func_8007B040", 0, 0, 0, 0, 0, 0, 0 },             /* u8 or */
    { "func_8007B084", 0, 0, 0, 0, 0, 0, 0 },             /* u8 xor */
    { "func_8007B0C8", 1, 1, 0x100, 0x22, 0xFFF0, 0x40, 0 }, /* u16 sat add */
    { "func_8007B134", 1, 1, 0x300, 0x22, 0x22, 0x300, 0 },  /* u16 sat sub */
    { "func_8007E954", 0, 1, 0x11, 0, 0x2B, 0, 1 },       /* row[p1] == p2 */
    { "func_8007E98C", 1, 1, 0x1111, 0, 0x2B2B, 0, 1 },   /* u16 row[p1] == p2|p3<<8 */
    { "func_8007E9D0", 0, 1, 0x7F, 0, 0x20, 0, 1 },       /* p2 >= row[p1] */
    { "func_8007EAC8", 0, 1, 0x11, 0x12, 0x5A, 0x5A, 0 }, /* row[p1] == row[p2] */
    /* Wrap-faithful multiply and bare-DIVU siblings: high = clamp / zero divisor. */
    { "func_8007B198", 1, 1, 0x100, 0x22, 0x300, 0x300, 0 },  /* u16 sat mul */
    { "func_8007AF5C", 0, 1, 0xC9, 0x0B, 0xC9, 0, 0 },     /* u8 div, 0 -> 0xFF */
    { "func_8007AFAC", 0, 1, 0xC9, 0x0B, 0xC9, 0, 0 },     /* u8 rem, 0 -> dividend */
    { "func_8007B208", 1, 1, 0xBEEF, 0x123, 0xBEEF, 0, 0 },  /* u16 div, 0 -> 0xFFFF */
    { "func_8007B264", 1, 1, 0xBEEF, 0x123, 0xBEEF, 0, 0 },  /* u16 rem, 0 -> dividend */
    /* Codemod sweep batch 2: main30 compare leaves. */
    { "func_8007EA08", 1, 1, 0x9000, 0x0, 0x2B2B, 0x0, 1 },
    { "func_8007EA4C", 0, 1, 0x10, 0x0, 0x2B, 0x0, 1 },
    { "func_8007EA84", 1, 1, 0x1000, 0x0, 0x2B2B, 0x0, 1 },
    { "func_8007EB08", 1, 1, 0x1111, 0x1112, 0x5A5A, 0x5A5A, 0 },
    { "func_8007EB50", 0, 1, 0x20, 0x10, 0x20, 0x20, 0 },
    { "func_8007EB90", 1, 1, 0x2000, 0x1000, 0x2000, 0x2000, 0 },
    { "func_8007EBD8", 0, 1, 0xD4, 0x0, 0x2B, 0x0, 1 },
    { "func_8007EC10", 1, 1, 0xD4D4, 0x0, 0x2B2B, 0x0, 1 },
    { "func_8007EC54", 0, 1, 0xF0, 0xF, 0xF0, 0x30, 0 },
    { "func_8007EC94", 1, 1, 0xF0F0, 0xF0F, 0xF0F0, 0x3030, 0 },
    /* Codemod sweep batch 3: main30 compare leaves. */
    { "func_8007ECDC", 0, 1, 0x11, 0x0, 0x2B, 0x0, 1 },
    { "func_8007ED14", 1, 1, 0x1111, 0x0, 0x2B2B, 0x0, 1 },
    { "func_8007ED58", 0, 1, 0x11, 0x12, 0x5A, 0x5A, 0 },
    { "func_8007ED98", 1, 1, 0x1111, 0x1112, 0x5A5A, 0x5A5A, 0 },
    { "func_8007EDE0", 2, 1, 0x11111111, 0x11111112, 0x5A5A5A5A, 0x5A5A5A5A, 0 },
    { "func_8007EE28", 2, 1, 0x80000000, 0x7FFFFFFF, 0x7FFFFFFF, 0x80000000, 0 },
    /* Codemod sweep batch 3: main22 row copies (u16 / u32). */
    { "func_8007B98C", 1, 0, 0, 0, 0, 0, 0 },
    { "func_8007B9C8", 2, 0, 0, 0, 0, 0, 0 },
};

/* Extra forced operand pairs that must each be proven (class outside lo/hi):
 * the multiply whose promoted-int product exceeds 0x7FFFFFFF (retail MULT
 * wraps negative, skips the clamp and stores the low half), and a zero
 * dividend over a zero divisor for the DIVU siblings. */
typedef struct SlotSweepExtra {
    const char *name;
    int wide;
    uint32_t a, b;
} SlotSweepExtra;

static const SlotSweepExtra kSlotSweepExtra[] = {
    { "func_8007B198", 1, 39033, 55481 },
    { "func_8007B198", 1, 0xFFFF, 0xFFFF },
    { "func_8007B198", 1, 3, 21845 },          /* exactly 0xFFFF: no clamp */
    { "func_8007AF5C", 0, 0, 0 },
    { "func_8007AFAC", 0, 0, 0 },
    { "func_8007B208", 1, 0, 0 },
    { "func_8007B264", 1, 0, 0 },
    { "func_8007B208", 1, 0xFFFF, 1 },
    { "func_8007B264", 1, 0x8001, 0x8000 },
};

static void hand_cases_slot_extra(void)
{
    uint32_t args[4];
    unsigned e, salt;

    for (e = 0; e < sizeof(kSlotSweepExtra) / sizeof(kSlotSweepExtra[0]); e++) {
        const SlotSweepExtra *x = &kSlotSweepExtra[e];
        unsigned proven = 0;
        if (!abi_leaf_listed(x->name))
            continue;
        for (salt = 0; salt < 6; salt++) {
            uint32_t index = salt % 5u;
            uint8_t *ops = (uint8_t *)PSX_ADDR(SRC);
            begin_case(0, 0);
            slot_fixture(salt, 0);
            if (ops[2] == ops[1])
                ops[2] = (uint8_t)((ops[1] + 1u) & 0x3Fu);
            if (x->wide) {
                uint8_t *row = (uint8_t *)PSX_ADDR(0x800D3420u + (index << 6));
                *(uint16_t *)(row + 2u * ops[1]) = (uint16_t)x->a;
                *(uint16_t *)(row + 2u * ops[2]) = (uint16_t)x->b;
            } else {
                uint8_t *row = (uint8_t *)PSX_ADDR(0x800D3430u + (index << 6));
                row[ops[1]] = (uint8_t)x->a;
                row[ops[2]] = (uint8_t)x->b;
            }
            args[0] = PTRT; args[1] = index; args[2] = args[3] = 0;
            compare_name(x->name, args);
            if (last_case_proven)
                proven++;
        }
        if (proven == 0)
            fail(x->name, "coverage", "a forced extra operand pair has no proven case");
    }
}

static void hand_cases_slot_sweep(void)
{
    uint32_t args[4];
    unsigned l, salt, variant;

    for (l = 0; l < sizeof(kSlotSweep) / sizeof(kSlotSweep[0]); l++) {
        const SlotSweepLeaf *s = &kSlotSweep[l];
        unsigned proven_variant[3] = {0, 0, 0};
        unsigned seen = 0;
        if (!abi_leaf_listed(s->name))
            continue;
        for (salt = 0; salt < 12; salt++) {
            for (variant = 0; variant < 3; variant++) {
                uint32_t index = salt % 5u;
                uint8_t *ops = (uint8_t *)PSX_ADDR(SRC);
                if (variant != 0 && !s->forced)
                    break;
                begin_case(0, 0);
                slot_fixture(salt, 0);
                if (ops[2] == ops[1])
                    ops[2] = (uint8_t)((ops[1] + 1u) & 0x3Fu);
                if (variant != 0) {
                    uint32_t a = variant == 1 ? s->lo_a : s->hi_a;
                    uint32_t b = variant == 1 ? s->lo_b : s->hi_b;
                    if (s->imm) {
                        /* immediate compare: p[2] (and p[3]) carry the value */
                        ops[2] = (uint8_t)(s->hi_a & 0xFFu);
                        ops[3] = (uint8_t)((s->hi_a >> 8) & 0xFFu);
                    }
                    if (s->wide == 2) {
                        uint8_t *row = (uint8_t *)PSX_ADDR(0x800D3410u + (index << 6));
                        *(uint32_t *)(row + 4u * ops[1]) = a;
                        if (!s->imm)
                            *(uint32_t *)(row + 4u * ops[2]) = b;
                    } else if (s->wide) {
                        uint8_t *row = (uint8_t *)PSX_ADDR(0x800D3420u + (index << 6));
                        *(uint16_t *)(row + 2u * ops[1]) = (uint16_t)a;
                        if (!s->imm)
                            *(uint16_t *)(row + 2u * ops[2]) = (uint16_t)b;
                    } else {
                        uint8_t *row = (uint8_t *)PSX_ADDR(0x800D3430u + (index << 6));
                        row[ops[1]] = (uint8_t)a;
                        if (!s->imm)
                            row[ops[2]] = (uint8_t)b;
                    }
                }
                args[0] = PTRT; args[1] = index; args[2] = args[3] = 0;
                compare_name(s->name, args);
                if (last_case_proven) {
                    proven_variant[variant]++;
                    seen |= last_retail_v0 != 0 ? 1u : 2u;
                }
            }
        }
        if (proven_variant[0] == 0 ||
            (s->forced && (proven_variant[1] == 0 || proven_variant[2] == 0)))
            fail(s->name, "coverage", "a forced operand class has no proven case");
        /* Comparison leaves (u32 return) must prove both results. */
        if (strncmp(s->name, "func_8007E", 10) == 0)
            require_outcomes(s->name, seen, 3u);
    }
}

/* 8007A828 (main14): dst[(a2 << 3) + p[1]] = p[2]; returns a2, plus one when
 * p[1] == 0.  dst is the guest board the dispatcher passes (0x800D2E5C);
 * both return classes must be proven. */
static void hand_cases_a828(void)
{
    uint32_t args[4];
    unsigned salt, seen = 0;

    if (!abi_leaf_listed("func_8007A828"))
        return;
    for (salt = 0; salt < 12; salt++) {
        uint8_t *ops = (uint8_t *)PSX_ADDR(SRC);
        begin_case(0, 0);
        slot_fixture(salt, 0);
        if ((salt & 1u) == 0)
            ops[1] = 0;
        args[0] = PTRT; args[1] = 0x800D2E5Cu; args[2] = (salt * 37u + 5u) & 0xFFu; args[3] = 0;
        compare_name("func_8007A828", args);
        if (last_case_proven)
            seen |= last_retail_v0 == args[2] ? 1u : 2u;
    }
    require_outcomes("func_8007A828", seen, 3u);
}

/* 80085454 (main36 port body): folds the 11 incoming (amount, kind) pairs at
 * D_800D2C54 (u16 every 4 bytes) / D_800D2C88 into the running totals
 * D_800D2D70 (u16[11]) and kinds D_800D2D5C, and mirrors them into action
 * row (a0 & 0xFF) of D_800C3FE8 (72-byte rows).  The scalar sweep leaves
 * the amounts at the pattern and the totals unexercised; here every slot
 * gets a nonzero amount and total, the kinds cover 0/2/5 against running
 * 0/2/5 (add and signed netting both ways) and the copy-only kinds, and the
 * D_800D2D90 window table that follows the totals holds live pointer words,
 * so a body that indexes the totals with the wrong stride is caught. */
static void hand_cases_85454(void)
{
    static const uint8_t kKinds[] = {0, 2, 5, 1, 3, 4, 6};
    uint32_t args[4];
    unsigned salt, i;

    if (!abi_leaf_listed("func_80085454"))
        return;
    for (salt = 0; salt < 24; salt++) {
        uint8_t *amount = (uint8_t *)PSX_ADDR(0x800D2C54u);
        uint8_t *kind = (uint8_t *)PSX_ADDR(0x800D2C88u);
        uint8_t *running = (uint8_t *)PSX_ADDR(0x800D2D5Cu);
        uint8_t *total = (uint8_t *)PSX_ADDR(0x800D2D70u);
        uint8_t *window = (uint8_t *)PSX_ADDR(0x800D2D90u);
        begin_case(salt % 3u, 0);
        for (i = 0; i < 11; i++) {
            uint16_t a = (uint16_t)(salt * 131u + i * 977u + 1u);
            uint16_t t = (uint16_t)(salt * 71u + i * 313u + 7u);
            amount[i * 4] = (uint8_t)a;
            amount[i * 4 + 1] = (uint8_t)(a >> 8);
            kind[i] = kKinds[(salt + i) % 7u];
            running[i] = kKinds[(salt * 3u + i * 5u) % 7u];
            total[i * 2] = (uint8_t)t;
            total[i * 2 + 1] = (uint8_t)(t >> 8);
        }
        for (i = 0; i < 4; i++) {
            uint32_t p = 0x800C5000u + i * 0x40u;
            window[i * 4] = (uint8_t)p;
            window[i * 4 + 1] = (uint8_t)(p >> 8);
            window[i * 4 + 2] = (uint8_t)(p >> 16);
            window[i * 4 + 3] = (uint8_t)(p >> 24);
        }
        args[0] = salt < 20u ? salt : 0xFFu - (salt - 20u);
        args[1] = 0; args[2] = 0; args[3] = 0;
        compare_name("func_80085454", args);
    }
}

/* 8007A7BC (main14): dst = a1 + 8*a3; dst[0] = 0x80, dst[1..4] = the four
 * operand bytes the slot points at; returns a3 + 1 (u8, so 0xFF wraps). */
static void hand_cases_a7bc(void)
{
    uint32_t args[4];
    unsigned salt;

    if (!abi_leaf_listed("func_8007A7BC"))
        return;
    for (salt = 0; salt < 12; salt++) {
        begin_case(0, 0);
        slot_fixture(salt, salt & 1u);
        args[0] = PTRT; args[1] = 0x800D2E5Cu; args[2] = salt;
        args[3] = salt == 11u ? 0xFFu : (salt * 23u + 1u) & 0xFFu;
        compare_name("func_8007A7BC", args);
    }
}

/* 800BA8F4 (mainl110, guest ABI): sprite floor query.  The sprite record is
 * at REC: s16 x/y/z at +2/+6/+0xA, cached triangle +0x78, floor height out
 * at +0x84.  Cached triangles hit, miss (fallback full search 800A579C) and
 * out of range, points inside and outside the mesh, both windings, the
 * pass-counter wrap, and tables not loaded.  Classes: 1 = a triangle cached
 * afterwards, 2 = -1 cached afterwards.  The locals the body hands to the
 * main69 queries are GFRAME (static under the port), so this also proves
 * the guest-word round trip of a native frame address. */
static void hand_cases_ba8f4(void)
{
    static const int32_t kCached[] = {-1, 0, 5, 11, 17, 18, 40};
    uint32_t args[4];
    unsigned seen = 0, c;
    int winding, x, z;

    if (!abi_leaf_listed("func_800BA8F4"))
        return;
    for (winding = 0; winding < 2; winding++) {
        for (c = 0; c < sizeof(kCached) / sizeof(kCached[0]); c++) {
            for (x = -500; x <= 500; x += 250) {
                for (z = -500; z <= 500; z += 250) {
                    uint8_t *s;
                    begin_case(c % 3u == 2u ? 2u : 0u, 0);
                    mesh_fixture(winding, (int)c + 2 * winding);
                    if (c == 3u && x == 0) {
                        unsigned t;
                        *(uint8_t *)PSX_ADDR(0x800D2F64u) = 0xFF;
                        for (t = 0; t < mesh_tri_count; t++)
                            *(uint8_t *)PSX_ADDR(MESH_TRI + 14u * t + 0xDu) = (uint8_t)(t * 37u);
                    }
                    s = (uint8_t *)PSX_ADDR(REC);
                    *(int16_t *)(s + 2) = (int16_t)(x + 13);
                    *(int16_t *)(s + 6) = (int16_t)(z / 10);
                    *(int16_t *)(s + 0xA) = (int16_t)(z - 7);
                    *(int32_t *)(s + 0x78) = kCached[c];
                    *(uint16_t *)(s + 0x84) = 0xBEEF;
                    args[0] = REC; args[1] = args[2] = args[3] = 0;
                    compare_name("func_800BA8F4", args);
                    if (last_case_proven)
                        seen |= *(int32_t *)(s + 0x78) >= 0 ? 1u : 2u;
                }
            }
        }
    }
    /* Tables not loaded: every query returns -1. */
    begin_case(0, 0);
    *(int32_t *)PSX_ADDR(REC + 0x78) = 3;
    args[0] = REC; args[1] = args[2] = args[3] = 0;
    compare_name("func_800BA8F4", args);
    require_outcomes("func_800BA8F4", seen, 3u);
}

/* Sprite record for the timer callbacks at REC (guest), the task at DST
 * (+4 = REC).  AnimScriptTick/func_80022CDC/func_80022B2C are the shipped
 * main-executable bodies (temp1.c) on both sides: the wait timer +0x9E is
 * kept above the tick count so the animation script interpreter
 * (func_800248D4) is never entered; +0x3C bit 26 selects the simple
 * vertical step or the floor query (func_800BA8F4, tables not loaded). */
static void timer_sprite(unsigned variant)
{
    uint8_t *o = (uint8_t *)PSX_ADDR(REC);
    *(uint32_t *)PSX_ADDR(DST + 4) = REC;
    *(uint32_t *)(o + 0x0) = 0x00120000u + variant * 0x1111u;
    *(uint32_t *)(o + 0x4) = 0xFFF80000u + variant * 0x2345u;
    *(uint32_t *)(o + 0x8) = 0x00340000u - variant * 0x0F0Fu;
    *(int32_t *)(o + 0xC) = (int32_t)(variant * 0x321u) - 0x400;
    *(int32_t *)(o + 0x10) = (int32_t)(variant * 0x123u) - 0x200;
    *(int32_t *)(o + 0x14) = 0x280 - (int32_t)(variant * 0x77u);
    *(int32_t *)(o + 0x1C) = (variant & 1u) ? 0x40 : -0x30;
    *(uint16_t *)(o + 0x3A) = (uint16_t)((variant % 3u) * 0x300u);
    *(uint32_t *)(o + 0x3C) = (variant & 2u) ? (1u << 26) : 0u;
    *(int32_t *)(o + 0x78) = -1;
    *(int16_t *)(o + 0x84) = (int16_t)(variant * 3u);
    *(int16_t *)(o + 0x9E) = (int16_t)(4 + variant);
    *(uint32_t *)(o + 0xAC) = (variant & 4u) ? 0x40u : 0u;
}

static void hand_cases_timer_callbacks(void)
{
    uint32_t args[4];
    unsigned v;

    if (abi_leaf_listed("func_800BAC50")) {
        unsigned seen = 0;
        for (v = 0; v < 16; v++) {
            begin_case(v % 3u, 0);
            timer_sprite(v);
            *(uint8_t *)PSX_ADDR(0x800C3664u) = (uint8_t)((v % 4u) == 3u ? 1u + (v & 8u) * 16u : 0u);
            args[0] = DST; args[1] = args[2] = args[3] = 0;
            compare_name("func_800BAC50", args);
            if (last_case_proven)
                seen |= *(uint8_t *)PSX_ADDR(0x800C3664u) != 0 ? 1u : 2u;
        }
        require_outcomes("func_800BAC50", seen, 3u);
    }
    /* 800BB13C: slot = ((+0xAC & 3) << 2) | (+0xA8 >> 30) selects the
     * D_800D3368 model record (at SRC, +4 -> SRC + 0x80 holding the
     * +0x5C/+0x60/+0x64 position).  Classes: 1 = no model (early out),
     * 2 = +0x9E idle clears +0x64, 4 = ticked. */
    if (abi_leaf_listed("func_800BB13C")) {
        unsigned seen = 0;
        for (v = 0; v < 24; v++) {
            uint8_t *o = (uint8_t *)PSX_ADDR(REC);
            unsigned slot;
            begin_case(v % 3u, 0);
            timer_sprite(v);
            *(uint32_t *)(o + 0xAC) |= v & 3u;
            *(uint32_t *)(o + 0xA8) = (v * 0x40000000u) | 0x1234u;
            slot = ((v & 3u) << 2) | (v & 3u);
            *(uint32_t *)(o + 0x64) = (v % 5u) == 1u ? 0u : 0x10u + v;
            if ((v % 6u) == 2u)
                *(int16_t *)(o + 0x9E) = 0;
            *(uint32_t *)PSX_ADDR(0x800D3368u + 4u * slot) = (v % 7u) == 3u ? 0u : SRC;
            *(uint32_t *)PSX_ADDR(SRC + 4) = SRC + 0x80u;
            *(uint32_t *)PSX_ADDR(SRC + 0x80u + 0x5C) = 0x0123u + v * 0x101u;
            *(uint32_t *)PSX_ADDR(SRC + 0x80u + 0x60) = 0xFF80u - v;
            *(uint32_t *)PSX_ADDR(SRC + 0x80u + 0x64) = 0x8001u + v * 7u;
            args[0] = DST; args[1] = args[2] = args[3] = 0;
            compare_name("func_800BB13C", args);
            if (last_case_proven)
                seen |= (v % 7u) == 3u ? 1u : ((v % 6u) == 2u ? 2u : 4u);
        }
        require_outcomes("func_800BB13C", seen, 7u);
    }
    /* 80076AC8 / 80076C34: SetSemiTrans(p, 1); SetShadeTex(p, 0) on a
     * primitive at DST (the PsyCross libgpu helpers the port binds the retail
     * 0x80043BFC entries to), then for C34 the 0x40 colour and +0x16 bit 6.
     * Every code byte value is tried. */
    for (v = 0; v < 256; v += 5) {
        uint8_t *p = (uint8_t *)PSX_ADDR(DST);
        begin_case(v % 3u, 0);
        p[3] = (uint8_t)(v * 3u);
        p[7] = (uint8_t)v;
        *(uint16_t *)(p + 0x16) = (uint16_t)(v * 0x0101u);
        args[0] = DST; args[1] = args[2] = args[3] = 0;
        if (abi_leaf_listed("func_80076AC8"))
            compare_name("func_80076AC8", args);
        if (abi_leaf_listed("func_80076C34"))
            compare_name("func_80076C34", args);
    }
}

/* 800AAA20 (main72 port body): obj at REC (+4 = model word -> SRC), the
 * fifth argument in the guest stack slot.  Its callees (800AFF9C,
 * 8009F1C4/8009EF3C, 800A0838, 800AE2A4, 800AAD54) are interpreted on the
 * retail side and reached through PcPort_BattleMipsCallGuest (their adopted
 * native bodies) on the host side.  800AAD54 returns
 * at once for steps == 0 or obj +0x10 == 0.  Classes: 1 = *obj == 0,
 * 2 = +0x34 clear (800AAD54 only), 4 = +0x34 set (stepping loop). */
static void hand_cases_aaa20(void)
{
    uint32_t args[4];
    unsigned v, seen = 0;

    if (!abi_leaf_listed("func_800AAA20"))
        return;
    pre_side = stack_arg4_side;
    for (v = 0; v < 48; v++) {
        uint8_t *o = (uint8_t *)PSX_ADDR(REC);
        unsigned cls = (v % 3u == 0u) ? 1u : ((v % 3u == 1u) ? 2u : 4u);
        begin_case(v % 2u, 0);
        *(uint32_t *)(o + 0x0) = cls == 1u ? 0u : 0x800C5300u;
        *(uint32_t *)(o + 0x4) = SRC;
        *(uint32_t *)(o + 0x10) = (v & 4u) ? SRC + 0x40u : 0u;
        *(int16_t *)(o + 0x1C) = (int16_t)(v * 3u);
        o[0x34] = cls == 4u ? (uint8_t)(1u + v) : 0u;
        o[0x37] = (uint8_t)((v >> 3) & 1u);
        *(uint16_t *)(o + 0x3C) = (uint16_t)(v * 0x111u);
        stack_arg4 = v * 0x10001u;
        args[0] = REC;
        args[1] = v % 4u;
        args[2] = (v & 8u) ? 0u : (v % 3u);  /* steps */
        args[3] = v & 1u;
        compare_name("func_800AAA20", args);
        if (last_case_proven)
            seen |= cls;
    }
    pre_side = NULL;
    require_outcomes("func_800AAA20", seen, 7u);
}

/* 800A3E98 (main68 port body): effect-image record at REC, linked record at
 * REC + 0x100.  The +0x24 progression callback is one of the four retail
 * entries (800A3490/3514/3578/35C8; native in the port body, interpreted on
 * the retail side).  Types 4/5 rebuild through 800A4348/800A43F8 into the
 * +0x1C buffer; types 0/1 reach main-exe func_80026F44 (and LoadImage when
 * unlinked), which this harness does not link, so those cases are
 * inconclusive by construction.  Classes: 1 = inactive (+0x1A == 0, -1),
 * 2 = finished (frame < 0, 800A429C), 4 = same frame, 8 = new frame. */
static void hand_cases_a3e98(void)
{
    static const uint32_t kCallbacks[] = {
        0x800A3490u, 0x800A3514u, 0x800A3578u, 0x800A35C8u
    };
    static const uint8_t kTypes[] = {4, 5, 4, 5, 0, 1, 2, 5};
    uint32_t args[4];
    unsigned v, seen = 0;

    if (!abi_leaf_listed("func_800A3E98"))
        return;
    for (v = 0; v < 160; v++) {
        uint8_t *e = (uint8_t *)PSX_ADDR(REC);
        uint8_t *l = (uint8_t *)PSX_ADDR(REC + 0x100u);
        unsigned cb = (v % 8u == 6u) ? 1u : v % 4u;
        unsigned ticks = v % 3u;
        int16_t div = (int16_t)(1 + v % 5u);
        int16_t base = (int16_t)((v % 7u == 3u) ? 0x40 : (int)(v % 4u));
        int inactive = (v % 13u) == 5u;
        int same = (v % 8u) == 6u;
        /* Same-frame cases keep the phase small so 800A3514 stays < 0x21. */
        uint16_t phase = (uint16_t)(same ? v % 8u : v * 37u);
        uint16_t step = (uint16_t)(v % 3u);
        unsigned cls;

        begin_case(v % 3u, 0);
        *(uint32_t *)(e + 0x00) = (v & 1u) ? REC + 0x100u : 0u;
        *(uint32_t *)(e + 0x04) = SRC;
        *(uint32_t *)(e + 0x08) = SRC + 0x40u;
        *(uint32_t *)(e + 0x0C) = DST;
        *(uint32_t *)(e + 0x1C) = DST + 0x100u;
        e[0x10] = kTypes[(v / 2u) % 8u];
        e[0x11] = 0;
        *(int16_t *)(e + 0x12) = 4;
        *(uint16_t *)(e + 0x14) = phase;
        *(uint16_t *)(e + 0x16) = step;
        *(uint16_t *)(e + 0x18) = 0x7FFF;
        *(uint16_t *)(e + 0x1A) = inactive ? 0 : 1;
        *(int16_t *)(e + 0x20) = div;
        *(int16_t *)(e + 0x22) = base;
        *(uint32_t *)(e + 0x24) = kCallbacks[cb];
        *(int16_t *)(e + 0x28) = (int16_t)(v % 2u);
        *(int16_t *)(e + 0x2A) = (int16_t)((v >> 1) % 2u);
        *(int16_t *)(e + 0x2C) = (int16_t)(2 + v % 2u);
        *(int16_t *)(e + 0x2E) = 2;
        if (same) {
            /* 800A3514: base + phase' / divisor, -1 at >= 0x21. */
            int16_t p2 = (int16_t)(uint16_t)(phase + step * (ticks + 1u));
            int16_t f = (int16_t)(base + p2 / div);
            *(uint16_t *)(e + 0x18) = (uint16_t)(f < 0x21 ? f : -1);
        }
        *(uint32_t *)(l + 0x04) = DST + 0x180u;
        l[0x10] = (v & 2u) ? 4u : 0u;
        *(uint16_t *)(l + 0x1A) = (v % 11u) == 7u ? 0 : 1;
        *(int16_t *)(l + 0x28) = (int16_t)(v % 3u);
        *(int16_t *)(l + 0x2A) = (int16_t)((v >> 2) % 2u);
        *(int16_t *)(l + 0x2C) = 3;
        *(int16_t *)(l + 0x2E) = 2;
        args[0] = REC; args[1] = ticks; args[2] = args[3] = 0;
        compare_name("func_800A3E98", args);
        if (!last_case_proven)
            continue;
        if (inactive)
            cls = 1u;
        else if ((int32_t)last_retail_v0 < 0)
            cls = 2u;
        else
            cls = same ? 4u : 8u;
        seen |= cls;
    }
    require_outcomes("func_800A3E98", seen, 15u);
}

/* 800A0838 (main65, lifted port body): model at REC (+0xA entry count,
 * 0x7C-byte entries from REC + 4), each entry with up to three track records
 * (+0x6C, +0x70, +0x74) at A0838_TRK and their byte streams at A0838_DAT.
 * Every track gets a random kind/skip byte (+2), finished flag (+1), owner
 * byte (+3, equal to a2 in half the cases), step counter/limit (+0x10/+0x12,
 * limit nonzero) and nonzero s16 parameters, so the 9-, 6- and third-track
 * jump tables are all reached; a0 is the track pool (HALF) that
 * func_800A23E8 (interpreted on both sides) releases a finished track into.
 * Classes: 1 = no flags, 2 = a running track (0x100), 4 = a finished,
 * released track (0x200), 8 = a finished looping track (0x400). */
#define A0838_TRK 0x800C6000u
#define A0838_DAT 0x800C7000u
/* High bits of the harness LCG: its low bits have short periods. */
static uint32_t hrnd(void)
{
    return rnd() >> 10;
}
static void hand_cases_a0838(void)
{
    uint32_t args[4];
    unsigned v, seen = 0;

    if (!abi_leaf_listed("func_800A0838"))
        return;
    for (v = 0; v < 1500; v++) {
        uint8_t *m = (uint8_t *)PSX_ADDR(REC);
        unsigned n = 1u + v % 3u, k, j;
        uint32_t a2 = v % 5u;
        unsigned cls;

        begin_case((v % 7u == 0u) ? 1u : 2u, 0);
        *(uint16_t *)(m + 0xA) = (uint16_t)(v % 13u == 0u ? 0u : n);
        for (k = 0; k < n; k++) {
            uint8_t *e = m + 4u + k * 0x7Cu;
            for (j = 0; j < 3; j++) {
                unsigned idx = k * 3u + j;
                uint32_t t = A0838_TRK + idx * 0x20u;
                uint32_t d = A0838_DAT + idx * 0x80u;
                uint8_t *r = (uint8_t *)PSX_ADDR(t);

                if ((hrnd() & 7u) == 0u) {
                    *(uint32_t *)(e + 0x6C + j * 4u) = 0u;
                    continue;
                }
                *(uint32_t *)(e + 0x6C + j * 4u) = t;
                r[1] = (hrnd() % 3u) == 0u ? 1u : 0u;
                r[2] = (uint8_t)hrnd();
                r[3] = (hrnd() & 1u) ? (uint8_t)a2 : (uint8_t)(a2 + 1u);
                *(uint32_t *)(r + 0x4) = d;
                *(uint32_t *)(r + 0x8) = d + (hrnd() & 0x1Eu);
                *(int16_t *)(r + 0xA) = (int16_t)(hrnd() % 0x800u) - 0x400;
                *(int16_t *)(r + 0xC) = (int16_t)(1 + hrnd() % 0x400u);
                *(int16_t *)(r + 0xE) = (int16_t)(hrnd() % 0x800u) - 0x400;
                *(int16_t *)(r + 0x12) = (int16_t)(1 + hrnd() % 4u);
                *(int16_t *)(r + 0x10) = (int16_t)(hrnd() % 6u) - 1;
                /* +8 (stream cursor) and +0xA (first parameter) share bytes;
                 * the stream kinds need a live cursor. */
                if (hrnd() & 1u)
                    *(uint32_t *)(r + 0x8) = d + (hrnd() & 0x1Eu);
                if ((hrnd() & 3u) == 0u) {
                    /* Delta streams: the -0x80 escape (16-bit delta). */
                    uint8_t *b = (uint8_t *)PSX_ADDR(d);
                    unsigned q;
                    for (q = 0; q < 0x40u; q++)
                        if ((hrnd() & 3u) == 0u)
                            b[q] = 0x80u;
                }
                if ((hrnd() & 3u) == 0u) {
                    /* Interpolation already at its target (zero step). */
                    if (j == 0u) {
                        *(int16_t *)(e + 0x50) = *(int16_t *)(r + 0xA);
                        *(int16_t *)(e + 0x52) = *(int16_t *)(r + 0xC);
                        *(int16_t *)(e + 0x54) = *(int16_t *)(r + 0xE);
                    } else if (j == 1u) {
                        *(int32_t *)(e + 0x58) = *(int16_t *)(r + 0xA);
                        *(int32_t *)(e + 0x5C) = *(int16_t *)(r + 0xC);
                        *(int32_t *)(e + 0x60) = *(int16_t *)(r + 0xE);
                    } else {
                        *(int16_t *)(r + 0xA) = *(int16_t *)(r + 0x4);
                        *(int16_t *)(r + 0xC) = *(int16_t *)(r + 0x6);
                        *(int16_t *)(r + 0xE) = *(int16_t *)(r + 0x8);
                    }
                }
            }
        }
        /* a0 is the track pool func_800A23E8 releases into: +0 base, +4
         * lowest free index. */
        *(uint32_t *)PSX_ADDR(HALF) = A0838_TRK;
        *(uint16_t *)PSX_ADDR(HALF + 4u) = (uint16_t)(v % 16u);
        args[0] = HALF;
        args[1] = REC;
        args[2] = a2;
        args[3] = (uint32_t)(int32_t)(int16_t)(v * 7u);
        compare_name("func_800A0838", args);
        if (!last_case_proven)
            continue;
        cls = last_retail_v0 == 0u ? 1u : 0u;
        if (last_retail_v0 & 0x100u) cls |= 2u;
        if (last_retail_v0 & 0x200u) cls |= 4u;
        if (last_retail_v0 & 0x400u) cls |= 8u;
        seen |= cls;
    }
    require_outcomes("func_800A0838", seen, 15u);
}

/* 800AFF9C (main73, lifted port body): floor query of object REC on the
 * mesh fixture: model at DST (+0x5C/+0x60/+0x64 position words), +0x1E
 * cached triangle, +0x36 keep-height flag.  Its callees 800A5914 / 800A579C /
 * 800A5870 are typed native leaves, so the body runs with no interpreter.
 * Classes: 1 = model height written back (+0x36 == 0), 2 = kept. */
static void hand_cases_aff9c(void)
{
    static const int16_t kCached[] = {-1, 0, 5, 11, 17, 40};
    uint32_t args[4];
    unsigned seen = 0, c;
    int winding, x, z;

    if (!abi_leaf_listed("func_800AFF9C"))
        return;
    for (winding = 0; winding < 2; winding++) {
        for (c = 0; c < sizeof(kCached) / sizeof(kCached[0]); c++) {
            for (x = -500; x <= 500; x += 250) {
                for (z = -500; z <= 500; z += 250) {
                    uint8_t *o = (uint8_t *)PSX_ADDR(REC);
                    uint8_t *m = (uint8_t *)PSX_ADDR(DST);
                    begin_case(c % 3u == 2u ? 2u : 0u, 0);
                    mesh_fixture(winding, (int)c + 2 * winding);
                    *(uint32_t *)(o + 0x4) = DST;
                    *(int32_t *)(m + 0x5C) = x + 13;
                    *(int32_t *)(m + 0x60) = z / 10;
                    *(int32_t *)(m + 0x64) = z - 7;
                    *(int16_t *)(o + 0x1E) = kCached[c];
                    o[0x36] = (uint8_t)((x / 250 + z / 250) & 1);
                    args[0] = REC; args[1] = args[2] = args[3] = 0;
                    compare_name("func_800AFF9C", args);
                    if (last_case_proven)
                        seen |= o[0x36] ? 2u : 1u;
                }
            }
        }
    }
    require_outcomes("func_800AFF9C", seen, 3u);
}

/* 8009EF3C / 8009F1C4 (main64, lifted port bodies): the model hierarchy
 * matrix rebuild of a model at DST (+0xA entry count, 0x7C-byte entries:
 * +0 parent entry or 0, +4/+5/+6 flags, +0xC / +0x2C matrices, +0x54
 * rotation, +0x4C..0x50 scale), argument a1 the frame/scale word.  Parents
 * point only at earlier entries, as a hierarchy does.  Both are reached
 * from 800AAA20 too.  Classes: 1 = one entry, 2 = several. */
static void hand_cases_model_matrices(const char *name)
{
    uint32_t args[4];
    unsigned v, seen = 0;

    if (!abi_leaf_listed(name))
        return;
    for (v = 0; v < 400; v++) {
        uint8_t *m = (uint8_t *)PSX_ADDR(DST);
        unsigned n = 1u + v % 6u, k;

        begin_case((v % 5u == 0u) ? 1u : 2u, 0);
        *(uint16_t *)(m + 0xA) = (uint16_t)n;
        for (k = 0; k < n; k++) {
            uint8_t *e = m + k * 0x7Cu;
            *(uint32_t *)(e + 0x0) = (k == 0u || (hrnd() & 3u) == 0u)
                ? 0u : DST + (hrnd() % k) * 0x7Cu;
            e[0x4] = (uint8_t)(hrnd() & 1u);
            e[0x6] = (uint8_t)(hrnd() & 1u);
            e[0x5] = (uint8_t)(hrnd() & 1u);
        }
        args[0] = DST;
        args[1] = (uint32_t)(int32_t)(int16_t)(hrnd() % 0x2000u);
        args[2] = args[3] = 0;
        compare_name(name, args);
        if (last_case_proven)
            seen |= n == 1u ? 1u : 2u;
    }
    require_outcomes(name, seen, 3u);
}

/* 800AAD54 (main72, lifted port body; every call through the jal service,
 * PcPort_BattleMipsCallJal): motion object at REC (+4 model at DST, +0x10
 * motion state at SRC, the +0x70..0x86 velocity block), a1 id, a2 the
 * per-step flags 800A0838 returned, a3 steps (0: early return), the fifth
 * word in the guest stack slot.  Archive, heap, sound and sprite callees are
 * main-exe code this harness only partly links: cases that reach a
 * placeholder are inconclusive by construction.  Classes: 1 = no steps,
 * 2 = stepped and completed. */
static void hand_cases_aad54_at(uint32_t sp, unsigned count, unsigned want)
{
    static const uint32_t kFlags[] = {0u, 0x1u, 0x2u, 0x4u, 0x100u, 0x200u, 0x400u,
                                      0x101u, 0x202u, 0x404u, 0x707u};
    uint32_t args[4];
    unsigned v, seen = 0;

    if (!abi_leaf_listed("func_800AAD54"))
        return;
    pre_side = stack_arg4_side;
    case_sp = sp;
    for (v = 0; v < count; v++) {
        uint8_t *o = (uint8_t *)PSX_ADDR(REC);
        unsigned steps = (v % 6u == 0u) ? 0u : 1u + hrnd() % 2u;

        begin_case((v % 5u == 0u) ? 1u : 2u, 0);
        *(uint32_t *)(o + 0x0) = (hrnd() & 3u) ? REC + 0x100u : 0u;
        *(uint32_t *)(o + 0x4) = DST;
        *(uint32_t *)(o + 0x10) = (hrnd() & 7u) ? SRC : 0u;
        /* Alternate script slots (+0x4C/+0x50/+0x54) and +0x58: off, or
         * a second script at SRC + 0x100. */
        *(uint32_t *)(o + 0x4C) = (hrnd() % 6u) == 0u ? SRC + 0x100u : 0u;
        *(uint32_t *)(o + 0x50) = (hrnd() % 6u) == 0u ? SRC + 0x100u : 0u;
        *(uint32_t *)(o + 0x54) = 0u;
        *(int16_t *)(o + 0x58) = 0;
        *(uint16_t *)((uint8_t *)PSX_ADDR(DST) + 0xA) = (uint16_t)(1u + hrnd() % 3u);
        stack_arg4 = hrnd() % 3u;
        args[0] = REC;
        args[1] = v % 4u;
        args[2] = kFlags[hrnd() % (sizeof(kFlags) / sizeof(kFlags[0]))];
        args[3] = steps;
        compare_name("func_800AAD54", args);
        if (last_case_proven)
            seen |= steps == 0u ? 1u : 2u;
    }
    pre_side = NULL;
    case_sp = 0;
    require_outcomes("func_800AAD54", seen, want);
}

/* 800AAD54 from the stacks retail really calls it on.  Class 2 (stepped)
 * makes the 800AADA8 jal HeapChangeCurrentUser (0x80032498) through
 * PcPort_BattleMipsCallJal from that $sp: the r6.10 opening battle aborted
 * there ("func_800AAD54: jal failed 0x80032498") because the call service
 * only accepted an 8-aligned KSEG0 $sp at or below BATTLE_STACK_TOP.  The
 * run script's control rebuilds that rule (BATTLE_JAL_LEGACY_SP_CHECK) and
 * requires exactly that abort. */
static void hand_cases_aad54(void)
{
    hand_cases_aad54_at(0u, 600u, 3u);
    hand_cases_aad54_at(0x801FFC04u, 150u, 2u);  /* heap stack: 4-aligned */
    hand_cases_aad54_at(0x1F8003F8u, 150u, 2u);  /* 800A4654's scratchpad stack */
}

/* 800AE2A4 (main72, lifted port body; every callee through the
 * interpreter's jal path or the bridge's direct contract): object at REC,
 * +0x98 current frame (< 0: inactive), +0x9A loop length, +0x9C/+0x9E event
 * cursor/count, +0xA0/+0xA4 event cursor/list start into a packed stream of
 * variable-size records at AE2A4_EVT (s16 frame, kind byte +2: 1..9 through
 * jtbl_8007075C, anything else skipped), +4 model at DST (0x7C-byte
 * entries), +0x20 side index, +0x10C/+0x110 the kind-3 effect table (0x70
 * stride), +0x10E/+0x118 the kind-9 image table (0x30 stride), +0x10A the
 * kind-8 actor mask, +0xB0 the kind-4 sound bank chain.  World state: the
 * D_800C3E30 actor flags (func_800AF400), the D_800D3368 actor table
 * (records at AE2A4_ACT), D_800C360C/D_800C4000 the kind-8 selector table,
 * D_800D2FC0 the kind-2/9 work block, and the main-exe camera pointer
 * D_800658C8 (a native binding: written through the shared resolver, so
 * both sides read the same camera block at AE2A4_CAM).  Record bytes are
 * random with the index-like fields kept small.  Classes: 1 = inactive,
 * 2 = an event consumed (+0xA0 moved), 4 = no event at this frame. */
#define AE2A4_EVT 0x800C6000u
#define AE2A4_ACT 0x800C6400u
#define AE2A4_CAM 0x800C7000u
#define AE2A4_TAB 0x800C6A00u
static void hand_cases_ae2a4(void)
{
    uint32_t args[4];
    unsigned v, seen = 0;

    if (!abi_leaf_listed("func_800AE2A4"))
        return;
    for (v = 0; v < 2400; v++) {
        uint8_t *o = (uint8_t *)PSX_ADDR(REC);
        uint8_t *ev = (uint8_t *)PSX_ADDR(AE2A4_EVT);
        uint8_t *cam = (uint8_t *)PSX_ADDR(AE2A4_CAM);
        int16_t frame = (int16_t)(v % 5u);
        int inactive = (v % 17u) == 3u;
        int miss = (v % 11u) == 4u;
        unsigned cls, k, r;

        begin_case((v % 7u == 0u) ? 1u : 2u, 0);
        *(int16_t *)(o + 0x98) = inactive ? (int16_t)-1 : frame;
        *(int16_t *)(o + 0x9A) = (int16_t)(hrnd() % 8u) - 1;
        *(int16_t *)(o + 0x9C) = 0;
        *(int16_t *)(o + 0x9E) = (int16_t)(1u + hrnd() % 3u);
        *(uint32_t *)(o + 0xA0) = AE2A4_EVT;
        *(uint32_t *)(o + 0xA4) = AE2A4_EVT;
        *(uint32_t *)(o + 0x4) = DST;
        o[0x20] = (uint8_t)(hrnd() % 4u);
        o[0x10C] = (uint8_t)(hrnd() % 4u);
        *(uint32_t *)(o + 0x110) = AE2A4_TAB;
        o[0x10E] = (uint8_t)(hrnd() % 4u);
        *(uint32_t *)(o + 0x118) = AE2A4_TAB + 0x180u;
        *(uint16_t *)(o + 0x10A) = (uint16_t)(hrnd() & 0x1FFFu);
        *(uint32_t *)(o + 0xB0) = AE2A4_CAM + 0x600u;
        *(uint32_t *)PSX_ADDR(AE2A4_CAM + 0x608u) = AE2A4_CAM + 0x640u;
        *(uint16_t *)((uint8_t *)PSX_ADDR(DST) + 0xA) = (uint16_t)(1u + hrnd() % 3u);
        /* World state. */
        *(uint16_t *)PSX_ADDR(0x800C3E30u) = (hrnd() % 5u) == 0u ? 0u : (uint16_t)(hrnd() & 0x1FFFu);
        *(uint32_t *)PSX_ADDR(0x800C360Cu) = 1u + hrnd() % 2u;
        for (k = 0; k < 13; k++) {
            uint32_t a = AE2A4_ACT + k * 0x50u;
            *(uint32_t *)PSX_ADDR(0x800D3368u + 4u * k) = (hrnd() % 3u) == 0u ? 0u : a;
            *(uint16_t *)PSX_ADDR(a + 0x4Au) = (uint16_t)(hrnd() & 3u);
        }
        for (k = 0; k < 0x90u; k++)
            *(uint8_t *)PSX_ADDR(0x800C4000u + k) = (uint8_t)(hrnd() % 7u);
        *(uint32_t *)PSX_ADDR(0x800D2FC0u) = AE2A4_CAM + 0x700u;
        cam[0x35F] = (uint8_t)(hrnd() % 4u);
        *(uint32_t *)battle_guest_resolve_native(0x800658C8u) =
            (hrnd() % 6u) == 0u ? 0u : AE2A4_CAM;
        /* The event records: random bodies, index fields small. */
        for (r = 0; r < 3u; r++) {
            uint8_t *e = ev + r * 0x1Cu;
            for (k = 3; k < 0x1Cu; k++)
                e[k] = (uint8_t)hrnd();
            *(int16_t *)(e + 0x0) = (r == 0u && miss) ? (int16_t)(frame + 1) : frame;
            e[2] = r == 0u ? (uint8_t)(v % 11u) : (uint8_t)(1u + hrnd() % 9u);
            e[3] = (uint8_t)(hrnd() % 3u);
            switch (hrnd() % 4u) {
            case 0: e[4] = 0; break;
            case 1: e[4] = (uint8_t)(hrnd() % 3u); break;
            case 2: e[4] = (uint8_t)(0x80u | (hrnd() % 3u)); break;
            default: break;
            }
            e[5] = (hrnd() % 5u) == 0u ? 0xFFu : (uint8_t)(hrnd() % 3u);
            e[6] = (uint8_t)((hrnd() & 0x80u) | (hrnd() % 6u));
            e[7] = (uint8_t)(hrnd() % 4u);
            e[0xC] = (uint8_t)((hrnd() & 0x80u) | (hrnd() % 4u));
            e[0xD] = (uint8_t)(hrnd() & 1u);
            e[0x12] = (uint8_t)(((hrnd() % 3u) << 4) | (hrnd() & 1u));
        }
        args[0] = REC;
        args[1] = v % 4u;
        args[2] = hrnd() % 3u;
        args[3] = 0;
        compare_name("func_800AE2A4", args);
        if (!last_case_proven)
            continue;
        if (inactive)
            cls = 1u;
        else if (*(uint32_t *)(o + 0xA0) != AE2A4_EVT)
            cls = 2u;
        else
            cls = 4u;
        seen |= cls;
    }
    *(uint32_t *)battle_guest_resolve_native(0x800658C8u) = 0u;
    require_outcomes("func_800AE2A4", seen, 7u);
}

/* ==== BEGIN h-battle block: func_8008A9C0 closure ===========================
 * The battle frame input dispatch main43 func_8008A9C0 (the D_800C3E4C mode
 * switch the native frame tick calls) and its static callees: the mode
 * handlers 8008A684 / 8008A3EC (main43 port bodies) and 8008A274 (main42),
 * 8008A144 (texture-strip upload), 80089CCC (debug-mode handler), 8008AA40
 * (sfx helper, reached from 80089CCC through the adopted 8008AA74) and
 * 8007171C (actor wait countdowns).  Everything for them is between this
 * line and the END marker, plus one call at the end of hand_cases().
 *
 * Scripted pad services.  The handlers poll main-executable pad services
 * this harness does not link.  They are defined here, not as run-script
 * SPIES, because a case scripts what they return: ControllerGetType(0)
 * reports no pad for the first hb_pad.disconnected polls, func_80036410()
 * asks for a pad reset for the first hb_pad.resets polls, and
 * ControllerPopState() pops hb_pad.events states into
 * g_C1ButtonStatePressedOnce / g_C1ButtonStateReleased -- the native copies
 * the runtime binds, so the interpreted retail side reads the same words.
 * Every call is logged through prover_spy, so the two sides' service call
 * sequences (and arguments) must be identical.  hb_pre_side rewinds the
 * scripts and the closure's native globals before EACH side; compare()'s
 * native-global write check then requires both sides to leave those globals
 * (all runtime bindings, outside guest RAM) alike.  With the empty script -- every
 * case outside this block -- the services are an idle, connected pad (no
 * reset, nothing queued) whatever the call count.  SoundMute /
 * EnableAllSpuChannels and GR_CopyVRAM are also defined below (side effects
 * only while the block runs); the pause-letter, pad-reset, sfx and
 * font-load services are SPIES in the run script (func_8003747C stays a
 * placeholder; see the run script).
 *
 * Guest calls.  8008A684 calls 0x801DF270 and 0x801DF4C0 (module at
 * 0x801DE000, no C body): retail jals them, the port body uses
 * BATTLE_GUEST_CALL0.  Each case plants two tiny MIPS routines there that
 * bump counters at HB_GCALL (the second also copies the first's count), so
 * the RAM diff sees both calls and their order.
 *
 * *D_8005917C.  8008A3EC and 80089CCC read the debug flag through it while
 * paused, and 80089CCC for its released 1 / 2 debug commands.  Retail
 * dereferences the pointer's low word as a guest address; the host bodies
 * dereference the host pointer.  hb_debug_flag makes both the same word
 * (see there); without that page those paths are skipped, not failed.
 *
 * Not covered: a call that starts paused and resumes without having paused
 * or lost the pad itself (retail restores an uninitialised `saved`, the
 * port bodies the current D_80059488: a documented port difference), and
 * 80089CCC's debug-font load (func_8003747C, a placeholder here; its port
 * body passes the raw guest word 0x80200000 where the bridge would hand
 * retail's callee a translated pointer). */
#include <sys/mman.h>
#ifndef MAP_FIXED_NOREPLACE
#define MAP_FIXED_NOREPLACE 0x100000
#endif
#define HB_MAX_EVENTS 4u
typedef struct {
    unsigned disconnected;  /* ControllerGetType(0) == 0 for this many polls */
    unsigned resets;        /* func_80036410() != 0 for this many polls */
    unsigned events;        /* states ControllerPopState() pops */
    uint16_t pressed[HB_MAX_EVENTS];
    uint16_t released[HB_MAX_EVENTS];
} HbPad;
static HbPad hb_pad;
static unsigned hb_polls, hb_reset_polls, hb_popped;

#define HB_PRESSED  0x800594A4u  /* g_C1ButtonStatePressedOnce */
#define HB_RELEASED 0x8005948Cu  /* g_C1ButtonStateReleased */

int ControllerGetType(int index);
int ControllerGetType(int index)
{
    unsigned long long a[1];
    unsigned r = hb_polls >= hb_pad.disconnected ? 1u : 0u;

    hb_polls++;
    a[0] = (unsigned)index;
    return (int)prover_spy("ControllerGetType", 1, a, r);
}

int func_80036410(void);
int func_80036410(void)
{
    unsigned r = hb_reset_polls < hb_pad.resets ? 1u : 0u;

    hb_reset_polls++;
    return (int)prover_spy("func_80036410", 0, NULL, r);
}

int ControllerPopState(void);
int ControllerPopState(void)
{
    unsigned r = 0;

    if (hb_popped < hb_pad.events) {
        *(uint16_t *)battle_guest_resolve_native(HB_PRESSED) = hb_pad.pressed[hb_popped];
        *(uint16_t *)battle_guest_resolve_native(HB_RELEASED) = hb_pad.released[hb_popped];
        hb_popped++;
        r = 1;
    }
    return (int)prover_spy("ControllerPopState", 0, NULL, r);
}

/* Spies whose side effects only this block's cases see (hb_active).
 *
 * SoundMute/EnableAllSpuChannels: retail keeps D_80059488 across a mute
 * (saved after the mute, restored after the unmute).  Nothing in this
 * harness changes that word, so a body that dropped the restore would pass;
 * here the mute stores HB_SPU_MUTED and the unmute HB_SPU_ENABLED into it,
 * and compare()'s native-global check compares the final word: a correct body ends at the muted
 * value, a missing restore at the unmuted one, a save taken before the mute
 * at the initial one.  Outside the block they are the plain spies the run
 * script used to generate (no side effect).
 *
 * GR_CopyVRAM: PsyCross's LoadImage ends here (LIBGPU.C).  In the block it
 * is a spy, so 8008A144's four strip uploads (source address and rectangle)
 * are compared; outside it, it is exactly the generated placeholder (the
 * case is inconclusive) as for every other leaf. */
#define HB_SPU_MUTED   0x0BADF00Du
#define HB_SPU_ENABLED 0x600DCAFEu
static int hb_active;

void SoundMuteAllSpuChannels(void);
void SoundMuteAllSpuChannels(void)
{
    if (hb_active)
        *(uint32_t *)battle_guest_resolve_native(0x80059488u) = HB_SPU_MUTED;
    (void)prover_spy("SoundMuteAllSpuChannels", 0, NULL, 0);
}

void SoundEnableAllSpuChannels(void);
void SoundEnableAllSpuChannels(void)
{
    if (hb_active)
        *(uint32_t *)battle_guest_resolve_native(0x80059488u) = HB_SPU_ENABLED;
    (void)prover_spy("SoundEnableAllSpuChannels", 0, NULL, 0);
}

void GR_CopyVRAM(unsigned short *src, int x, int y, int w, int h, int dst_x, int dst_y);
void GR_CopyVRAM(unsigned short *src, int x, int y, int w, int h, int dst_x, int dst_y)
{
    static int told;
    unsigned long long a[7];

    if (!hb_active) {
        g_prover_placeholder_hits++;
        if (!told) {
            told = 1;
            fprintf(stderr, "[prover] unavailable callee GR_CopyVRAM (case inconclusive)\n");
        }
        return;
    }
    a[0] = (unsigned long long)(uintptr_t)src;
    a[1] = (unsigned)x;
    a[2] = (unsigned)y;
    a[3] = (unsigned)w;
    a[4] = (unsigned)h;
    a[5] = (unsigned)dst_x;
    a[6] = (unsigned)dst_y;
    (void)prover_spy("GR_CopyVRAM", 7, a, 0);
}

/* The closure's main-executable globals, through the runtime's bindings (one
 * storage with the host bodies' symbols; hb_bindings_ok checks that).
 * D_8005917C is the debug-flag POINTER: 8 bytes on the host, whose low word
 * retail loads as a guest address (see hb_map_flag_page). */
static const struct {
    uint32_t address;
    unsigned size;
    const char *name;
} kHbNatives[] = {
    {0x80059488u, 4u, "D_80059488"},   /* SPU word kept across a pause */
    {HB_PRESSED, 2u, "g_C1ButtonStatePressedOnce"},
    {HB_RELEASED, 2u, "g_C1ButtonStateReleased"},
    {0x8005919Cu, 4u, "D_8005919C"},   /* sfx bank; 8008AA40 reads +0x14 */
    {0x8005959Cu, 1u, "D_8005959C"},   /* 80089CCC debug font cycle */
    {0x8005917Cu, 8u, "D_8005917C"},   /* -> debug flag (s32) */
};
#define HB_NATIVE_COUNT (sizeof(kHbNatives) / sizeof(kHbNatives[0]))
#define HB_BANK_SLOT 3u
#define HB_FONT_SLOT 4u
#define HB_FLAGPTR_SLOT 5u
static uint8_t hb_native_saved[HB_NATIVE_COUNT][8];   /* restored after the block */
static uint8_t hb_native_init[HB_NATIVE_COUNT][8];    /* each side starts here */

static uint8_t *hb_native(unsigned i)
{
    return (uint8_t *)battle_guest_resolve_native(kHbNatives[i].address);
}

static int hb_bound(unsigned i)
{
    void *sym = dlsym(RTLD_DEFAULT, kHbNatives[i].name);
    return sym != NULL && (uint8_t *)sym == hb_native(i);
}

/* The words the scripts and checks rely on.  D_8005917C is checked where the
 * debug-flag page is set up (only those paths need it). */
static int hb_bindings_ok(void)
{
    unsigned i;

    for (i = 0; i < HB_NATIVE_COUNT; i++) {
        void *sym = dlsym(RTLD_DEFAULT, kHbNatives[i].name);
        if (i == HB_FLAGPTR_SLOT)
            continue;
        if (!hb_bound(i)) {
            fprintf(stderr, "BATTLE OVERLAY DIFFERENTIAL FAIL h-battle fixture: "
                    "%s is not one shared native binding (symbol %p, runtime %p)\n",
                    kHbNatives[i].name, sym, (void *)hb_native(i));
            failures++;
            return 0;
        }
    }
    return 1;
}

static void hb_pre_side(void)
{
    unsigned i;

    hb_polls = hb_reset_polls = hb_popped = 0;
    for (i = 0; i < HB_NATIVE_COUNT; i++)
        memcpy(hb_native(i), hb_native_init[i], kHbNatives[i].size);
}

/* compare() for one a0.  The closure's native globals (all runtime
 * bindings) are compared by compare()'s native-global write check; this
 * block used to compare them itself.  Returns 1 when the case was proven. */
static int hb_compare(const char *name, uint32_t a0)
{
    uint32_t args[4];

    args[0] = a0;
    args[1] = args[2] = args[3] = 0;
    compare_name(name, args);
    return last_case_proven;
}

#define HB_SFX_DEFAULT 0x5A17u /* the bank's +0x14 id (see HB_SFX_IDS) */
#define HB_WORK   0x800C6200u  /* *D_800D2D28 battle work block */
#define HB_TIMERS 0x800C6800u  /* *D_800D3278: 16 timers of 0x38 */
#define HB_EAC    0x800C6C00u  /* *D_800C3EAC (+0x2DB) */
#define HB_BANK   0x800C6F00u  /* D_8005919C sfx bank (+0x14 id) */
#define HB_FLAGS  0x800C7000u  /* the three D_800D32F8 records' +0x15F8.., 0x20 apart */
#define HB_TEX    0x80100000u  /* *D_800C3EA4 (+0x6410.., rects +0x8950) */
#define HB_GCALL  0x800C5F80u  /* bumped by the 0x801DF270 / 0x801DF4C0 stubs */

static const uint32_t kHbStub270[] = {
    0x3C08800Cu, /* lui   t0, 0x800C */
    0x91095F80u, /* lbu   t1, 0x5F80(t0) */
    0x00000000u, /* nop */
    0x25290001u, /* addiu t1, t1, 1 */
    0xA1095F80u, /* sb    t1, 0x5F80(t0) */
    0x03E00008u, /* jr    ra */
    0x00000000u, /* nop */
};
static const uint32_t kHbStub4C0[] = {
    0x3C08800Cu, /* lui   t0, 0x800C */
    0x91095F80u, /* lbu   t1, 0x5F80(t0) */
    0x910A5F82u, /* lbu   t2, 0x5F82(t0) */
    0x00000000u, /* nop */
    0xA1095F81u, /* sb    t1, 0x5F81(t0): 0x801DF270's count so far */
    0x254A0001u, /* addiu t2, t2, 1 */
    0xA10A5F82u, /* sb    t2, 0x5F82(t0) */
    0x03E00008u, /* jr    ra */
    0x00000000u, /* nop */
};

/* 32 random bits: the high halves of two LCG steps (the fixtures below take
 * fields up to bit 31). */
static uint32_t hb_rng;
static uint32_t hb_rnd(void)
{
    uint32_t hi;

    hb_rng = hb_rng * 1664525u + 1013904223u;
    hi = hb_rng >> 16;
    hb_rng = hb_rng * 1664525u + 1013904223u;
    return (hi << 16) | (hb_rng >> 16);
}

static uint8_t *hb_g8(uint32_t address)
{
    return (uint8_t *)PSX_ADDR(address);
}

/* *D_8005917C, the debug flag (-1 in a retail game).  The host bodies follow
 * D_8005917C as a host pointer; retail loads its low word and reads that
 * guest address.  A guest address the host can ALSO dereference makes both
 * read the same s32: HB_DEBUG_FLAG is a guest RAM word and, through a page
 * this block maps at the same host address, a host word, both set to the
 * case's flag.  Without the page (mapping refused) those paths stay
 * unexercised; D_8005917C is never pointed at an unmapped address. */
#define HB_DEBUG_FLAG 0x800C5F00u
static void *hb_flag_page;

static void hb_map_flag_page(void)
{
    void *want = (void *)(uintptr_t)(HB_DEBUG_FLAG & ~0xFFFu);
    void *got = mmap(want, 0x1000u, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);

    hb_flag_page = NULL;
    if (got == MAP_FAILED) {
        fprintf(stderr, "[prover] h-battle: no host page at %p; the *D_8005917C "
                "paths are not exercised\n", want);
        return;
    }
    if (got != want) {
        munmap(got, 0x1000u);
        fprintf(stderr, "[prover] h-battle: host page landed at %p, not %p; the "
                "*D_8005917C paths are not exercised\n", got, want);
        return;
    }
    hb_flag_page = got;
}

/* Point D_8005917C at a debug flag of `value` for this case (both sides).
 * Returns 0 when the page is missing: the caller then skips the case. */
static int hb_debug_flag(int32_t value)
{
    uint64_t pointer = HB_DEBUG_FLAG;

    if (hb_flag_page == NULL)
        return 0;
    *(int32_t *)PSX_ADDR(HB_DEBUG_FLAG) = value;
    *(int32_t *)(uintptr_t)HB_DEBUG_FLAG = value;
    memcpy(hb_native_init[HB_FLAGPTR_SLOT], &pointer, sizeof(pointer));
    return 1;
}

/* One case's RAM: pattern fill, the closure's pointer globals at their
 * blocks, the guest-call stubs, an unpaused idle battle (pause not allowed,
 * sfx off, 8007171C idle), 8008A144 on its no-upload parity, and an idle
 * connected pad. */
static void hb_fixture(unsigned pattern, uint8_t mode)
{
    uint8_t *w = hb_g8(HB_WORK);
    unsigned k;

    begin_case(pattern, 0);
    memset(&hb_pad, 0, sizeof(hb_pad));
    *(uint32_t *)PSX_ADDR(0x800D2D28u) = HB_WORK;
    *(uint32_t *)PSX_ADDR(0x800C3EA4u) = HB_TEX;
    *(uint32_t *)PSX_ADDR(0x800D3278u) = HB_TIMERS;
    *(uint32_t *)PSX_ADDR(0x800C3EACu) = HB_EAC;
    for (k = 0; k < 3; k++)
        *(uint32_t *)PSX_ADDR(0x800D32F8u + 4u * k) = HB_FLAGS + 0x20u * k - 0x15F8u;
    memcpy(PSX_ADDR(0x801DF270u), kHbStub270, sizeof(kHbStub270));
    memcpy(PSX_ADDR(0x801DF4C0u), kHbStub4C0, sizeof(kHbStub4C0));
    *hb_g8(0x800C3E4Cu) = mode;
    *hb_g8(0x800C3444u) = 0;     /* not paused */
    *hb_g8(0x800CCC58u) = 0;     /* pause not allowed; 8008A274 skips 8007171C */
    *hb_g8(0x800C48EAu) = 0;
    *hb_g8(0x800D366Cu) = 0;     /* 8008AA74: no sfx */
    *hb_g8(0x800D3014u) = 0x33;  /* command byte: any write shows */
    *hb_g8(0x800D3298u) = 0;     /* 8007171C idle */
    *hb_g8(0x800D39D4u) = 0;
    *(int32_t *)PSX_ADDR(0x800D3288u) = 0x100;
    hb_g8(HB_EAC)[0x2DB] = 0;
    w[0xA0] = 0;                 /* 8008A684: countdown tail off */
    w[0xCA] = 0;                 /* 8008A3EC: timers off */
    w[0xCF] = 1;                 /* D_800D3014 not reset to 0xFF */
    w[0xAA] = 1;                 /* 8008A144: ++ -> 2, no upload */
    *(uint32_t *)(w + 0x30) = 0x10;
    /* Bank id: see HB_SFX_IDS for why it is not a small value. */
    *(uint16_t *)PSX_ADDR(HB_BANK + 0x14u) = HB_SFX_DEFAULT;
    /* 8008A144's four strip rectangles (x, y, w, h). */
    for (k = 0; k < 4; k++) {
        int16_t *rect = (int16_t *)PSX_ADDR(HB_TEX + 0x8950u + 8u * k);
        rect[0] = (int16_t)(0x300 + 0x10 * (int)k);
        rect[1] = (int16_t)(0x1C0 + (int)k);
        rect[2] = 4;
        rect[3] = (int16_t)(0x20 + (int)k);
    }
    memset(hb_native_init, 0, sizeof(hb_native_init));
    *(uint32_t *)hb_native_init[0] = 0x1234ABCDu;
    *(uint32_t *)hb_native_init[HB_BANK_SLOT] = HB_BANK;
    hb_native_init[HB_FONT_SLOT][0] = 2;
}

static void hb_pad_event(uint16_t pressed, uint16_t released)
{
    if (hb_pad.events < HB_MAX_EVENTS) {
        hb_pad.pressed[hb_pad.events] = pressed;
        hb_pad.released[hb_pad.events] = released;
        hb_pad.events++;
    }
}

/* 8007171C over 11 actors.  Classes (from each case's inputs, counted when
 * the case is proven): 1 = D_800D3298 clear, 2 = every-other-frame actor
 * skipped (bit 12, D_800D2E1C 0 -> 1), 4 = D_800CCE44 countdown (bit 13),
 * 8 = bit 7 / D_800CCD68 bit 12 hold, 16 = D_800D2E06 countdown, 32 = a
 * step of 2 (bit 15), 64 = an actor reaching zero. */
static void hb_cases_7171c(void)
{
    unsigned v, seen = 0;

    for (v = 0; v < 64; v++) {
        unsigned i, cls = 0;
        int on = (v % 8u) != 0;

        hb_fixture(v % 3u, 0);
        *hb_g8(0x800D3298u) = on ? (uint8_t)(1u + v) : 0u;
        if (!on)
            cls |= 1u;
        for (i = 0; i < 11; i++) {
            uint32_t r = hb_rnd();
            uint16_t f64 = (uint16_t)(r & 0x0C7Fu);  /* bits outside 7 / 12..15 */
            uint16_t hold68 = 0;
            int active = (r & 0x7000u) != 0;
            uint8_t ce44 = (uint8_t)((r >> 16) % 4u);
            uint16_t count = (uint16_t)((r >> 18) % 4u);
            int step2 = ((r >> 21) & 3u) == 0u;
            uint16_t tick = (uint16_t)((r >> 23) & 1u);

            switch ((r >> 12) % 6u) {
            case 0: f64 |= 0x1000u; break;
            case 1: f64 |= 0x2000u; break;
            case 2: f64 |= 0x0080u; break;
            case 3: hold68 = 0x1000u; break;
            default: break;
            }
            if ((r >> 24) % 7u == 0u)
                count = (uint16_t)(0x8000u | count);  /* already negative */
            hb_g8(0x800D2DCCu)[i] = active ? (uint8_t)(1u + (r & 3u)) : 0u;
            hb_g8(0x800D2DE4u)[i] = (r >> 25) % 6u == 0u ? 1u : 0u;
            *(uint16_t *)PSX_ADDR(0x800CCD64u + 0x170u * i) = f64;
            *(uint16_t *)PSX_ADDR(0x800CCD68u + 0x170u * i) =
                (uint16_t)(hold68 | (uint16_t)((r >> 4) & 0x0FFFu));
            *(uint16_t *)PSX_ADDR(0x800CCD6Cu + 0x170u * i) = step2 ? 0x8000u : 0u;
            *(uint16_t *)PSX_ADDR(0x800CCD6Eu + 0x170u * i) =
                (uint16_t)((r >> 26) & 1u ? 0x7FFFu : 0u);
            hb_g8(0x800CCE44u + 0x170u * i)[0] = ce44;
            *(uint16_t *)PSX_ADDR(0x800D2E06u + 2u * i) = count;
            *(uint16_t *)PSX_ADDR(0x800D2E1Cu + 2u * i) = tick;
            if (!on || !active || hb_g8(0x800D2DE4u)[i] != 0)
                continue;
            if (step2)
                cls |= 32u;
            if ((f64 & 0x1000u) && tick == 0) {
                cls |= 2u;
                continue;
            }
            if (f64 & 0x2000u)
                cls |= 4u;
            else if ((f64 & 0x80u) || hold68)
                cls |= 8u;
            else {
                cls |= 16u;
                if ((int16_t)(count - (step2 ? 2u : 1u)) <= 0)
                    cls |= 64u;
            }
        }
        if (hb_compare("func_8007171C", 0))
            seen |= cls;
    }
    require_outcomes("func_8007171C", seen, 0x7Fu);
}

/* 8008A144: on odd parity, four LoadImage strip uploads (rectangle at
 * +0x8950 + 8i, column words from +0x8970 + 0x630i); LoadImage ends in the
 * GR_CopyVRAM spy, so the source addresses and rectangles are compared.
 * Columns across the wrap at 0xC7.  Classes: 1 = no upload (even parity),
 * 2 = upload, 4 = upload wrapping the column to 0. */
static void hb_cases_8a144(void)
{
    static const uint32_t cols[] = {0u, 4u, 0x60u, 0xC0u, 0xC4u, 0xC6u};
    unsigned v, seen = 0;

    for (v = 0; v < 36; v++) {
        uint8_t *w = hb_g8(HB_WORK);
        uint32_t col = cols[v % 6u];
        int upload = ((v / 6u) & 1u) != 0;

        hb_fixture(v % 3u, 0);
        w[0xAA] = (uint8_t)(upload ? 2u * v : 1u + 2u * v);
        *(uint32_t *)(w + 0x30) = col;
        if (hb_compare("func_8008A144", 0))
            seen |= !upload ? 1u : (col + 4u >= 0xC7u ? 4u : 2u);
    }
    require_outcomes("func_8008A144", seen, 7u);
}

/* 8008AA40(a): the bank word D_8005919C (KSEG0 or KSEG1 address), its +0x14
 * id and the u8 argument (a0 above 0xFF too: retail's andi).  Classes:
 * 1 = KSEG0 bank, 2 = KSEG1 bank.
 * HB_SFX_IDS: the spied func_80039DB8 word is (id << 16) | a.  Ids keep it
 * out of the KSEG / scratchpad shapes the legacy bridge would turn into host
 * pointers on the retail side.  (They were also chosen outside this
 * executable's image while the spy comparison folded every word there into
 * one token; spy_logs_match now compares such words exactly.) */
#define HB_SFX_IDS {0x0000u, 0x0001u, 0x0C35u, 0x7FFFu, 0xFFFFu, 0x1F7Fu}
static void hb_cases_8aa40(void)
{
    static const uint16_t ids[] = HB_SFX_IDS;
    unsigned v, seen = 0;

    for (v = 0; v < 24; v++) {
        uint32_t bank = (v & 1u) ? HB_BANK : HB_BANK + 0x20000000u;
        hb_fixture(v % 3u, 0);
        *(uint32_t *)hb_native_init[HB_BANK_SLOT] = bank;
        *(uint16_t *)PSX_ADDR(HB_BANK + 0x14u) = ids[v % 6u];
        if (hb_compare("func_8008AA40", (v % 5u == 4u) ? 0x100u + v * 7u : v * 37u))
            seen |= (v & 1u) ? 1u : 2u;
    }
    require_outcomes("func_8008AA40", seen, 3u);
}

/* 80089CCC (debug-mode handler).  Classes: 1 = idle (command 8), 2 = pad
 * disconnected then back, 4 = D_800C48EA / D_800C3EAC+0x2DB (0xFF),
 * 8 = pressed 0x2000/0x4000/0x8000/0x1000 (commands 0-3, sfx 0x4C),
 * 16 = released 0x20/0x40/0x80/0x10 (4-7), 32 = released 0x100 (0xD),
 * 64 = released 0x800 with pause not allowed (0xE), 128 = pad reset,
 * 256 = a state matching nothing, then the next one.  With the debug-flag
 * page (hb_debug_flag): 512 = Start pauses and Start resumes (flag -1),
 * 1024 = paused, 4 + 8 with a debug flag resumes (D_800C48EA = 1),
 * 2048 = released 1 with a debug flag (party slots reset), 4096 = released
 * 1 with flag -1 (command 0xC only), 8192 = released 2 with flag -1 (no
 * command), 16384 = released 2 with a debug flag and the font already
 * loaded (0xB, D_8005959C cycles, wrapping at 5).  Released 2 with the font
 * not loaded calls func_8003747C, a placeholder here: not exercised. */
static void hb_cases_89ccc(void)
{
    static const uint16_t pressed[] = {0x2000u, 0x4000u, 0x8000u, 0x1000u};
    static const uint16_t released[] = {0x20u, 0x40u, 0x80u, 0x10u};
    unsigned v, seen = 0, want = 0x1FFu;

    for (v = 0; v < 120; v++) {
        unsigned sc = v % 15u, cls = 1u << sc;

        hb_fixture(v % 3u, 3);
        *hb_g8(0x800D366Cu) = (uint8_t)(((v / 15u) >> 2) & 1u);  /* sfx on/off */
        if (sc >= 9u) {
            if (!hb_debug_flag((sc == 9u || sc == 12u || sc == 13u) ? -1 : 0))
                continue;
            *hb_g8(0x800CCC58u) = 1;
            *hb_g8(0x800C3AA0u) = 1;
            hb_native_init[HB_FONT_SLOT][0] = (uint8_t)((v / 15u) % 2u ? 4u : 2u);
        }
        switch (sc) {
        case 9: hb_pad_event(0, 0x800u); hb_pad_event(0, 0x800u); break;
        case 10: hb_pad_event(0, 0x800u); hb_pad_event(0x000Cu, 0); break;
        case 11:
        case 12: hb_pad_event(0, 0x0001u); break;
        case 13:
        case 14: hb_pad_event(0, 0x0002u); break;
        default: break;
        }
        switch (sc) {
        case 0: break;
        case 1: hb_pad.disconnected = 1u + (v / 15u) % 3u; break;
        case 2:
            if ((v / 15u) & 2u)
                hb_g8(HB_EAC)[0x2DB] = 1;
            else
                *hb_g8(0x800C48EAu) = 1;
            hb_pad_event(0x2000u, 0x20u);
            break;
        case 3: hb_pad_event(pressed[(v / 15u) % 4u], 0x20u); break;
        case 4: hb_pad_event(0x0004u, released[(v / 15u) % 4u]); break;
        case 5: hb_pad_event(0, 0x100u); break;
        case 6: hb_pad_event(0, 0x800u); break;
        case 7: hb_pad.resets = 1; hb_pad_event(0, 0x20u); break;
        case 8:
            hb_pad_event(0x0200u, 0x0200u);
            hb_pad_event(pressed[(v / 15u) % 4u], 0);
            break;
        default: break;
        }
        if (hb_compare("func_80089CCC", (v / 15u) & 1u))
            seen |= cls;
    }
    if (hb_flag_page != NULL)
        want |= 0x7E00u;
    require_outcomes("func_80089CCC", seen, want);
}

/* 8008A3EC (mode 2 handler).  Classes: 1 = idle, 2 = 16 timers counted down
 * (active / inactive, 0 clamped, negative), 4 = pad disconnected then back,
 * 8 = released 0x20 (command 4), 16 = released 0x800 with pause not allowed,
 * 32 = pad reset, 64 = D_800D2D28+0xCF clear (command 0xFF), 128 = a state
 * matching nothing, then 0x20.  With the debug-flag page (pause allowed):
 * 256 = Start pauses and Start resumes (flag -1), 512 = paused, 4 + 8 with
 * a debug flag resumes (D_800C48EA = 1), 1024 = paused, released 0x20 is
 * ignored, then Start resumes.  Both odd and even 8008A144 parities. */
static void hb_cases_8a3ec(void)
{
    static const int16_t timers[] = {0, 1, 2, 5, -3, 0x7FFF, -0x8000, 0x100};
    unsigned v, seen = 0, want = 0xFFu;

    for (v = 0; v < 88; v++) {
        unsigned sc = v % 11u, cls = 1u << sc, t;
        uint8_t *w = hb_g8(HB_WORK);

        hb_fixture(v % 3u, 2);
        if ((v / 11u) & 1u)
            w[0xAA] = (uint8_t)(2u * v);   /* 8008A144 uploads */
        if (sc >= 8u) {
            if (!hb_debug_flag(sc == 9u ? 0 : -1))
                continue;
            *hb_g8(0x800CCC58u) = 1;
            hb_pad_event(0, 0x800u);
            if (sc == 9u)
                hb_pad_event(0x000Cu, 0);
            if (sc == 10u)
                hb_pad_event(0, 0x20u);
            if (sc != 9u)
                hb_pad_event(0, 0x800u);
        }
        switch (sc) {
        case 0: break;
        case 1:
            w[0xCA] = (uint8_t)(1u + v);
            for (t = 0; t < 16; t++) {
                uint8_t *rec = hb_g8(HB_TIMERS + 0x38u * t);
                rec[0x28] = (uint8_t)((t + v) % 3u);
                *(int16_t *)(rec + 0x26) = timers[(t + v) % 8u];
            }
            break;
        case 2: hb_pad.disconnected = 1u + (v / 8u) % 3u; break;
        case 3: hb_pad_event(0, 0x20u); break;
        case 4: hb_pad_event(0, 0x800u); break;
        case 5: hb_pad.resets = 1; hb_pad_event(0, 0x20u); break;
        case 6: w[0xCF] = 0; break;
        case 7: hb_pad_event(0x0003u, 0x0001u); hb_pad_event(0, 0x20u); break;
        default: break;
        }
        if (hb_compare("func_8008A3EC", v & 1u))
            seen |= cls;
    }
    if (hb_flag_page != NULL)
        want |= 0x700u;
    require_outcomes("func_8008A3EC", seen, want);
}

/* 8008A684 (mode 0 handler).  Classes: 1 = idle, 2 = pad disconnected then
 * back, 4 = released 0x20 (command 4), 8 = Start pauses and Start resumes
 * (mute, letters, D_80059488 kept), 16 = pad reset, 32 = countdown tail
 * with every record done (+0x15F9 cleared), 64 = countdown tail counting
 * (the pairs at 0x800CDCB8 / 0x800CDCD0) with the 0x801DF270 / 0x801DF4C0
 * guest calls, 128 = D_800D2D28+0xCF clear (command 0xFF). */
static void hb_cases_8a684(void)
{
    unsigned v, seen = 0;

    for (v = 0; v < 64; v++) {
        unsigned cls = 1u << (v % 8u), k;
        uint8_t *w = hb_g8(HB_WORK);

        hb_fixture(v % 3u, 0);
        switch (v % 8u) {
        case 0: break;
        case 1: hb_pad.disconnected = 1u + (v / 8u) % 3u; break;
        case 2: hb_pad_event(0, 0x20u); break;
        case 3: hb_pad_event(0, 0x800u); hb_pad_event(0, 0x800u); break;
        case 4: hb_pad.resets = 1; break;
        case 5:
        case 6:
            w[0xA0] = 1;
            for (k = 0; k < 3; k++) {
                uint8_t *f = hb_g8(HB_FLAGS + 0x20u * k);
                uint32_t r = hb_rnd();
                int done = (v % 8u) == 5u;
                f[0] = (uint8_t)((k == 0u) ? (r & 1u) : (r & 0xFFu));  /* +0x15F8 */
                f[1] = (k == 0u) ? 1u : (uint8_t)(r >> 8);              /* +0x15F9 */
                /* Retail ANDs the flag bytes themselves into `all`: done
                 * records get odd flags (1 / 3) so all stays 1; while
                 * counting, 0 / 1 / 2 (2 also ends with all == 0). */
                f[2] = done ? (uint8_t)(1u + 2u * ((r >> 16) % 2u)) : (uint8_t)((r >> 16) % 3u);
                f[3] = done ? (uint8_t)(1u + 2u * ((r >> 18) % 2u)) : (uint8_t)((r >> 18) % 3u);
                *(int32_t *)PSX_ADDR(0x800CDCD0u + 8u * k) = (int32_t)((r >> 20) % 3u);
                *(int32_t *)PSX_ADDR(0x800CDCD4u + 8u * k) = (int32_t)((r >> 22) % 3u);
                *(int32_t *)PSX_ADDR(0x800CDCB8u + 8u * k) = (int32_t)(r % 0x50u);
                *(int32_t *)PSX_ADDR(0x800CDCBCu + 8u * k) = (int32_t)((r >> 4) % 0x50u);
            }
            if ((v % 8u) == 6u) {
                /* record 0 still counting: all == 0, the guest calls run */
                hb_g8(HB_FLAGS)[2] = 0;
                *(int32_t *)PSX_ADDR(0x800CDCD0u) = 2;
            }
            break;
        default: w[0xCF] = 0; break;
        }
        {
            /* After compare(), RAM is the host side's (equal to retail's for
             * a proven case): the tail classes count only when they ran. */
            uint8_t calls = hb_g8(HB_GCALL)[2];
            if (!hb_compare("func_8008A684", v & 1u))
                continue;
            if (cls == 32u && hb_g8(HB_FLAGS)[1] != 0)
                cls = 0;
            if (cls == 64u && hb_g8(HB_GCALL)[2] != (uint8_t)(calls + 1u))
                cls = 0;
            seen |= cls;
        }
    }
    require_outcomes("func_8008A684", seen, 0xFFu);
}

/* 8008A274 (mode 1 handler).  Classes: 1 = argument 0 (80089CCC idle),
 * 2 = D_800CCC58 set (8007171C), 4 = the +0x6416 == 0 bounce up (incl. the
 * 0x80 turn), 8 = the bounce down (incl. the turn below 0), 16 = D_800D39D4
 * 1/3 (+1), 32 = 2/4 (-1), 64 = any other D_800D39D4. */
static void hb_cases_8a274(void)
{
    static const int32_t pos[] = {0x7C, 0x7D, 0x80, 0x81, 0, 3, 4, -1, 0x40};
    unsigned v, seen = 0;

    for (v = 0; v < 54; v++) {
        uint8_t *t = hb_g8(HB_TEX);
        unsigned cls = 0;
        uint32_t arg = (v % 4u == 0u) ? 0u : 1u + v;

        hb_fixture(v % 3u, 1);
        if ((v / 9u) & 1u)
            hb_g8(HB_WORK)[0xAA] = (uint8_t)(2u * v);   /* 8008A144 uploads */
        if (arg == 0u)
            cls |= 1u;
        if (v % 3u == 1u) {
            *hb_g8(0x800CCC58u) = 1;
            cls |= 2u;
        }
        t[0x6415] = (uint8_t)(v % 5u != 0u);
        t[0x6416] = (uint8_t)((v >> 1) & 1u);
        *(int32_t *)(t + 0x6410) = pos[v % 9u];
        if (t[0x6415] != 0)
            cls |= t[0x6416] == 0 ? 4u : 8u;
        *hb_g8(0x800D39D4u) = (uint8_t)(v % 6u);
        cls |= (v % 6u == 1u || v % 6u == 3u) ? 16u
             : (v % 6u == 2u || v % 6u == 4u) ? 32u : 64u;
        if (hb_compare("func_8008A274", arg))
            seen |= cls;
    }
    require_outcomes("func_8008A274", seen, 0x7Fu);
}

/* 8008A9C0(v): the D_800C3E4C mode switch.  Classes: 1 = mode 0 (8008A684),
 * 2 = mode 1 (8008A274), 4 = mode 2 (8008A3EC), 8 = any other mode. */
static void hb_cases_8a9c0(void)
{
    static const uint8_t modes[] = {0u, 1u, 2u, 3u, 0x80u, 0xFFu};
    unsigned v, seen = 0;

    for (v = 0; v < 24; v++) {
        uint8_t mode = modes[v % 6u];
        hb_fixture(v % 3u, mode);
        if (v % 4u == 3u)
            hb_pad_event(0, 0x20u);
        if (hb_compare("func_8008A9C0", (v / 6u) & 1u))
            seen |= mode <= 2u ? 1u << mode : 8u;
    }
    require_outcomes("func_8008A9C0", seen, 0xFu);
}

static void hand_cases_hbattle(void)
{
    static const char *const leaves[] = {
        "func_8008A9C0", "func_8008A684", "func_8008A3EC", "func_8008A274",
        "func_8008A144", "func_80089CCC", "func_8008AA40", "func_8007171C",
    };
    uint32_t saved_rng = rng_state;
    unsigned i;

    for (i = 0; i < sizeof(leaves) / sizeof(leaves[0]); i++)
        if (!abi_leaf_listed(leaves[i]))
            return;
    if (!hb_bindings_ok())
        return;
    for (i = 0; i < HB_NATIVE_COUNT; i++)
        memcpy(hb_native_saved[i], hb_native(i), kHbNatives[i].size);
    if (hb_bound(HB_FLAGPTR_SLOT))
        hb_map_flag_page();
    else
        fprintf(stderr, "[prover] h-battle: D_8005917C is not a shared native "
                "binding; the *D_8005917C paths are not exercised\n");
    hb_rng = 0x8A9C0u;
    hb_active = 1;
    pre_side = hb_pre_side;
    hb_cases_7171c();
    hb_cases_8a144();
    hb_cases_8aa40();
    hb_cases_89ccc();
    hb_cases_8a3ec();
    hb_cases_8a684();
    hb_cases_8a274();
    hb_cases_8a9c0();
    pre_side = NULL;
    hb_active = 0;
    memset(&hb_pad, 0, sizeof(hb_pad));
    for (i = 0; i < HB_NATIVE_COUNT; i++)
        memcpy(hb_native(i), hb_native_saved[i], kHbNatives[i].size);
    if (hb_flag_page != NULL) {
        munmap(hb_flag_page, 0x1000u);
        hb_flag_page = NULL;
    }
    rng_state = saved_rng;
}
/* ==== END h-battle block ==================================================== */

/* One case per adopted leaf, with the arguments the port's bridge test already
 * established as meaningful for it. */
/* ---- BEGIN h-frontier: window-family hand cases ---------------------------
 * The scalar sweep cannot prove the frontier step-2 window family.  8008F8F4
 * and its caller 8007819C allocate through the native 8008ABB8 heap, which
 * no sweep case initialises.  8008F6E4 and 8008DC34 index the 8-slot
 * D_800D2E38 table by a window kind the sweep draws out of range.  These
 * cases use the retail call shapes on pointer-seeded RAM (D_800D2D28 = PTRT;
 * frontier_case_setup seeds the window tables).  Each side gets a fresh
 * native heap (abb8_heap, as the 8008ABB8 and 8007FEC4 cases do) and the
 * o32 stack arguments (sp+0x10..) written before it runs.  D_800CCB34, the
 * double-buffer index, is 0 or 1.  refresh is always 0: its 800716D8 call is
 * the 0x8028022C hard stop. */
/* func_8002675C (temp1f.c) builds a glyph's POLY_FT4 run from the sprite
 * atlas, and 80076A10, the window glyph routine, calls it.  The runner does
 * not link it: PORT_REAL_OBJS still names temp1.c, where it no longer lives.
 * Linking temp1f.c for real let a sweep case's garbage item count run native
 * code without the interpreter's step budget, and an O0 run stalled until
 * its timeout.  So it is a spy here: it builds nothing, returns one sprite
 * and logs its arguments, and both sides' calls must match exactly. */
int func_8002675C(void *table, int index, void *polys, int ctx, int x, int y,
                  int scale);
int func_8002675C(void *table, int index, void *polys, int ctx, int x, int y,
                  int scale)
{
    unsigned long long a[7];

    a[0] = (unsigned long long)(uintptr_t)table;
    a[1] = (unsigned)index;
    a[2] = (unsigned long long)(uintptr_t)polys;
    a[3] = (unsigned)ctx;
    a[4] = (unsigned)x;
    a[5] = (unsigned)y;
    a[6] = (unsigned)scale;
    return (int)prover_spy("func_8002675C", 7, a, 1u);
}

static uint32_t frontier_stack_args[3];
static void frontier_window_side(void)
{
    uint32_t sp = case_sp != 0 ? case_sp : BATTLE_STACK_TOP;
    unsigned k;

    abb8_heap();
    for (k = 0; k < 3; k++)
        *(uint32_t *)battle_guest_resolve_native(sp + 0x10u + 4u * k) =
            frontier_stack_args[k];
}

static void frontier_window_case(const char *name, uint32_t fill, uint32_t buffer,
                                 const uint32_t *a, unsigned n)
{
    uint32_t args[4] = {0, 0, 0, 0};
    unsigned k;

    begin_case(fill, 1);
    *((uint32_t *)PSX_ADDR(0x800CCB34u)) = buffer;
    for (k = 0; k < n && k < 4; k++)
        args[k] = a[k];
    for (k = 0; k < 3; k++)
        frontier_stack_args[k] = k + 4 < n ? a[k + 4] : 0u;
    pre_side = frontier_window_side;
    compare_name(name, args);
    pre_side = NULL;
}

static void frontier_hand_cases(void)
{
    /* 8008F8F4(index, x, y, w, h, animate, refresh): both 8007819C calls, a
     * main49 call and an animate=1 call.  Fill 0 leaves the slot flags
     * (D_800D2D28 + 0xB0 + index) clear: the allocate path.  Fill 1 sets them
     * (0xA5): the reuse path through the seeded tables. */
    static const uint32_t open[][7] = {
        {5, 0x08, 0x2A, 0x70, 0x12, 0, 0},
        {4, 0x20, 0xC8, 0xF4, 0x12, 0, 0},
        {2, 0x1C, 0xA0, 0x50, 0x18, 0, 0},
        {0, 0x74, 0xA0, 0xAC, 0x38, 1, 0},
    };
    /* 8008F6E4 / 8008DC34(kind, x, y, w, h): every kind, retail-like boxes. */
    static const uint32_t box[][4] = {
        {0x08, 0x2A, 0x70, 0x12}, {0x20, 0xC8, 0xF4, 0x12},
        {0x74, 0xA0, 0xAC, 0x38}, {0x84, 0x4C, 0x9C, 0x50},
    };
    unsigned c, fill, kind;

    if (!abi_leaf_listed("func_8008F8F4") || !abi_leaf_listed("func_8007819C") ||
        !abi_leaf_listed("func_8008F6E4") || !abi_leaf_listed("func_8008DC34"))
        return;
    for (fill = 0; fill < 2; fill++)
        for (c = 0; c < sizeof(open) / sizeof(open[0]); c++)
            frontier_window_case("func_8008F8F4", fill, c & 1u, open[c], 7);
    for (fill = 0; fill < 2; fill++)
        frontier_window_case("func_8007819C", fill, fill, NULL, 0);
    for (kind = 0; kind < 8; kind++) {
        const uint32_t a[5] = {kind, box[kind & 3u][0], box[kind & 3u][1],
                               box[kind & 3u][2], box[kind & 3u][3]};
        frontier_window_case("func_8008F6E4", kind % 3u, kind & 1u, a, 5);
        frontier_window_case("func_8008DC34", kind % 3u, kind & 1u, a, 5);
    }
}
/* ---- END h-frontier (hand cases) ------------------------------------------ */

/* ---- BEGIN h-frontier: archive 0x0C/file 4 module fixtures -----------------
 * These are minimal body/ABI comparisons, separate from the battle allowlist.
 * Retail code comes from the SHA-pinned payload extracted from the user's
 * disc. Archive and callback-setter services are spies; actor binding remains
 * unproven/refusing. The actor-update case exercises a real mesh query and the
 * real animation initializer's NULL-default-animation early return only. */
#define FRONTIER_MODULE_BASE 0x801E4000u
#define FRONTIER_MODULE_SIZE 0x56C0u
static uint8_t frontier_module_image[FRONTIER_MODULE_SIZE];
static int frontier_module_loaded;
static unsigned frontier_archive_busy;

int ArchiveDataSync(void)
{
    return (int)prover_spy("ArchiveDataSync", 0, NULL, frontier_archive_busy);
}

int func_80029AFC(void *entries, int mode, int flags)
{
    if (!frontier_module_only && !frontier_archive_active) {
        g_prover_placeholder_hits++;
        return 0;
    }
    const unsigned long long args[] = {
        (unsigned long long)(uintptr_t)entries, (unsigned)mode, (unsigned)flags
    };
    return (int)prover_spy("func_80029AFC", 3, args, 0);
}

static int frontier_tasks_active; /* h-frontier task-callback cases, below */
void prover_wl_TimerWorkListSetTaskCallback(void *task, void (*callback)(void *));

void TimerWorkListSetTaskCallback(void *task, void *callback)
{
    if (!frontier_module_only && !frontier_tasks_active) {
        g_prover_placeholder_hits++;
        return;
    }
    const unsigned long long args[] = {
        (unsigned long long)(uintptr_t)task,
        (unsigned long long)(uintptr_t)callback
    };
    (void)prover_spy("TimerWorkListSetTaskCallback", 2, args, 0);
    /* The task-callback cases also set it, on the real (renamed) work list. */
    if (frontier_tasks_active)
        prover_wl_TimerWorkListSetTaskCallback(task,
            (void (*)(void *))(uintptr_t)callback);
}

/* Step 9 (801E6E48 / 801E6DC8): the main-executable services the module's
 * timer callback reaches are spies.  func_8002DDE4 and SoundAddSedsEntry are
 * the run script's generated spies (SPIES), as everywhere.  func_80022224
 * and DrawSync are spies in the module-only scope only: elsewhere
 * func_80022224 is a counted placeholder (it had no definition here at all
 * before: the bridge could not resolve it, inconclusive either way) and
 * DrawSync is PsyCross's (the run script renames LIBGPU's definition
 * prover_real_DrawSync). */
int prover_real_DrawSync(int mode);
int DrawSync(int mode)
{
    if (!frontier_module_only)
        return prover_real_DrawSync(mode);
    const unsigned long long args[] = {(unsigned)mode};
    return (int)prover_spy("DrawSync", 1, args, 0);
}

void func_80022224(void *package, void *data, int a2, int a3, int a4)
{
    if (!frontier_module_only) {
        g_prover_placeholder_hits++;
        return;
    }
    const unsigned long long args[] = {
        (unsigned long long)(uintptr_t)package, (unsigned long long)(uintptr_t)data,
        (unsigned)a2, (unsigned)a3, (unsigned)a4
    };
    (void)prover_spy("func_80022224", 5, args, 0);
}

/* Step 10b (801E6D6C): func_8003BDFC, the sound-transfer query, is a spy in
 * the module-only scope that returns the case's value; elsewhere a counted
 * placeholder (it had no definition here before: the bridge could not
 * resolve it, inconclusive either way).  The port binds its own override
 * (game_overrides.c); these cases prove the module bodies, not it. */
static uint32_t frontier_8003bdfc_ret;
int func_8003BDFC(int flags)
{
    if (!frontier_module_only) {
        g_prover_placeholder_hits++;
        return 0;
    }
    const unsigned long long args[] = {(unsigned)flags};
    return (int)prover_spy("func_8003BDFC", 1, args, frontier_8003bdfc_ret);
}

static int frontier_module_leaf(const char *name)
{
    return strcmp(name, "func_801E6F00") == 0 ||
           strcmp(name, "func_801E6AC4") == 0 ||
           strcmp(name, "func_801E6E48") == 0 ||
           strcmp(name, "func_801E6D6C") == 0 ||
           strcmp(name, "func_801E6D34") == 0;
}

static void frontier_module_load(void)
{
    if (!frontier_module_loaded) {
        FILE *f = fopen("disc/battle_module_0c4.bin", "rb");
        uint8_t hash[32];
        const char *expected_hash =
            "4300fdd99d19805a7a68ded0b21db49e1261688e90bf58e10dead9b978b52885";
        if (f == NULL) {
            perror("disc/battle_module_0c4.bin (run extract_battle_module_0c4.py)");
            exit(2);
        }
        if (fread(frontier_module_image, 1, FRONTIER_MODULE_SIZE, f) !=
                FRONTIER_MODULE_SIZE || fgetc(f) != EOF) {
            fprintf(stderr, "BATTLE OVERLAY DIFFERENTIAL FAIL module image size\n");
            fclose(f);
            exit(2);
        }
        fclose(f);
        file1_digest(frontier_module_image, FRONTIER_MODULE_SIZE, hash);
        for (unsigned i = 0; i < 32; i++) {
            if (file1_hex_digit(hash[i] >> 4) != expected_hash[2 * i] ||
                file1_hex_digit(hash[i] & 15) != expected_hash[2 * i + 1]) {
                fprintf(stderr, "BATTLE OVERLAY DIFFERENTIAL FAIL module image hash\n");
                exit(2);
            }
        }
        frontier_module_loaded = 1;
    }
    memcpy(PSX_ADDR(FRONTIER_MODULE_BASE), frontier_module_image,
           FRONTIER_MODULE_SIZE);
}

static void frontier_module_begin(void)
{
    begin_case(0, 0);
    frontier_module_load();
    /* All first-loop slot kinds are invalid: startup actor bind (whose
     * 800BA984 has its own hand cases) stays outside these fixtures. */
    for (unsigned i = 0; i < 3; i++)
        *(uint8_t *)PSX_ADDR(0x800C3EB6u + i * 0x1Cu) = 0xFF;
    *(uint8_t *)PSX_ADDR(0x800D36B8u) = 1; /* skip second actor-update loop */
}

static void frontier_module_identity_cases(void)
{
    static const uint32_t targets[] = {0x801E6AC4u, 0x801E6F00u, 0x801E6DC8u,
                                       0x801E6E48u, 0x801E6A4Cu, 0x801E6D6Cu,
                                       0x801E6D34u};
    static const char *const names[] = {"func_801E6AC4", "func_801E6F00",
                                        "func_801E6DC8", "func_801E6E48",
                                        "func_801E6A4C", "func_801E6D6C",
                                        "func_801E6D34"};
    static const uint32_t windows[] = {0x801E4000u, 0x801E5840u, 0};
    const uint32_t args[4] = {PTRT, 0, 0, 0};
    unsigned declined = 0;
    for (unsigned t = 0; t < 7; t++) {
        for (unsigned w = 0; w < 3; w++) {
            frontier_module_begin();
            uint32_t at = windows[w] != 0 ? windows[w] : targets[t];
            *(uint8_t *)PSX_ADDR(at) ^= 1;
            if (run_host(targets[t], args) != 0)
                fail(names[t], "identity", "corrupt module identity was adopted");
            else
                declined++;
        }
    }
    fprintf(stderr, "[module-identity] corrupt snapshots declined=%u/21\n", declined);
}

static void frontier_module_timer_cases(void); /* step 9, below */
static void frontier_module_6d6c_cases(void); /* step 10b, below */
static void frontier_module_6d34_cases(void); /* step 10c, below */

static void frontier_module_hand_cases(void)
{
    const char *old_startup = getenv("XENO_BATTLE_NATIVE_STARTUP_ACTORS");
    const char *old_timer = getenv("XENO_BATTLE_NATIVE_TIMER_TASK");
    char *saved_startup = old_startup != NULL ? strdup(old_startup) : NULL;
    char *saved_timer = old_timer != NULL ? strdup(old_timer) : NULL;
    uint32_t args[4] = {PTRT, 0, 0, 0};
    setenv("XENO_BATTLE_NATIVE_STARTUP_ACTORS", "1", 1);
    /* The retail side force-interprets module code; disable the timer host
     * on the native side too, so both callback-setter calls reach the spy. */
    setenv("XENO_BATTLE_NATIVE_TIMER_TASK", "0", 1);
    frontier_archive_busy = 1; /* safe early path if a broken gate adopts */
    frontier_module_identity_cases();
    frontier_archive_busy = 0;
    for (unsigned busy = 0; busy < 2; busy++) {
        frontier_module_begin();
        frontier_archive_busy = busy;
        pre_side = abb8_heap;
        compare_name("func_801E6F00", args);
        if (!last_case_proven)
            fail("func_801E6F00", "fixture", "ready/busy fixture did not complete");
        pre_side = NULL;
    }
    frontier_archive_busy = 0;
    frontier_module_begin();
    compare_name("func_801E6AC4", args); /* both actor loops skipped */
    if (!last_case_proven)
        fail("func_801E6AC4", "fixture", "skip fixture did not complete");
    for (unsigned c = 0; c < 3; c++) {
        frontier_module_begin();
        mesh_fixture((int)(c & 1u), (int)c);
        *(uint8_t *)PSX_ADDR(0x800D36B8u) = 0;
        *(uint32_t *)PSX_ADDR(0x800CCB3Cu) = REC;
        *(int16_t *)PSX_ADDR(REC + 2u) = (int16_t)(13 + c * 50);
        *(int16_t *)PSX_ADDR(REC + 6u) = (int16_t)(-7 - (int)c);
        *(int16_t *)PSX_ADDR(REC + 10u) = (int16_t)(27 + c * 30);
        *(int32_t *)PSX_ADDR(REC + 0x78u) = -1;
        *(uint32_t *)PSX_ADDR(REC + 0x48u) = 0; /* real initializer early return */
        compare_name("func_801E6AC4", args);
        if (!last_case_proven)
            fail("func_801E6AC4", "fixture", "actor-update fixture did not complete");
    }
    frontier_module_timer_cases();
    frontier_module_6d6c_cases();
    frontier_module_6d34_cases();
    if (saved_startup != NULL) {
        setenv("XENO_BATTLE_NATIVE_STARTUP_ACTORS", saved_startup, 1);
        free(saved_startup);
    } else
        unsetenv("XENO_BATTLE_NATIVE_STARTUP_ACTORS");
    if (saved_timer != NULL) {
        setenv("XENO_BATTLE_NATIVE_TIMER_TASK", saved_timer, 1);
        free(saved_timer);
    } else
        unsetenv("XENO_BATTLE_NATIVE_TIMER_TASK");
}
/* ---- END h-frontier (module fixtures) ----------------------------------- */

/* ---- BEGIN h-frontier: 800BA984 startup-actor task fixtures ---------------
 * 800BA984 (frontier step 7) makes a startup actor's sprite task: a 0x19C-byte
 * task from WorkListsAddTasks with its sprite at +0x38, built by main-exe
 * func_800242F4 from the package, placed at (x, y, z) 16.16, given its
 * animation, +0x32 and scale, and started through func_800245D8; it returns
 * the task.  It takes 14 o32 argument words (a0..a3, sp+0x10..+0x34) and
 * reads 4..10 and 13.  func_800242F4 / func_800245D8 and the sprite code they
 * reach (temp1.c) run for real here, on a fresh heap per side.  The six
 * services that chain reaches and this harness does not link
 * (WorkListsAddTasks, SpriteSetScale, func_8001EE74, func_800222BC,
 * func_800223B0, SpriteComputeTransformMatrix) are spies while one of these
 * cases runs (frontier_ba984_active), and the run script's placeholder
 * otherwise (counted, case inconclusive, 0 returned), so no other leaf's
 * cases change.  WorkListsAddTasks hands back the seeded task.
 * func_800222BC's binding of the package's animation table is not run, so
 * each case leaves the word it would leave -- sprite +0x110 +0x10, a 32-bit
 * host address as the port's sprite code keeps it -- pointing at the table.
 * The SpriteSetScale spy also records D_800591B8, which holds the 14th word
 * while func_800242F4 runs, so all 32 bits of it are compared.  Not
 * observable here: sp+0x14 (the port's func_8002435C ignores its arg6) and
 * sp+0x2C / +0x30 (never read).  The generic sweep skips 800BA984: a random
 * case's placeholder task word would reach native sprite code as a pointer. */
#define FRONTIER_BA984_TASK 0x800C8000u /* 0x19C bytes; the sprite at +0x38 */
#define FRONTIER_BA984_PKG  0x800C8400u /* +8: the frame table's offset */
#define FRONTIER_BA984_ANIM 0x800C8600u /* +2 + 2i: animation i's offset */
static int frontier_ba984_active;
static uint32_t frontier_ba984_stack[10]; /* sp+0x10..+0x34: words 4..13 */
extern unsigned char D_800591B8[];        /* func_800242F4's flags word */

/* Outside these cases: the run script's placeholder, as before. */
static void frontier_ba984_unlinked(const char *name)
{
    static const char *seen[16];
    static unsigned count;
    unsigned i;

    g_prover_placeholder_hits++;
    for (i = 0; i < count; i++)
        if (seen[i] == name)
            return;
    if (count < 16)
        seen[count++] = name;
    fprintf(stderr, "[prover] unavailable callee %s (case inconclusive)\n", name);
}

void *WorkListsAddTasks(int size, int arg1, void *cb0, void *cb1, void *cb2);
void *WorkListsAddTasks(int size, int arg1, void *cb0, void *cb1, void *cb2)
{
    unsigned long long a[5];

    if (!frontier_ba984_active) {
        frontier_ba984_unlinked("WorkListsAddTasks");
        return NULL;
    }
    a[0] = (unsigned)size;
    a[1] = (unsigned)arg1;
    a[2] = (unsigned long long)(uintptr_t)cb0;
    a[3] = (unsigned long long)(uintptr_t)cb1;
    a[4] = (unsigned long long)(uintptr_t)cb2;
    return (void *)(uintptr_t)prover_spy("WorkListsAddTasks", 5, a,
                                         FRONTIER_BA984_TASK);
}

void SpriteSetScale(void *sprite, short scale);
void SpriteSetScale(void *sprite, short scale)
{
    unsigned long long a[3];
    uint32_t flags;

    if (!frontier_ba984_active) {
        frontier_ba984_unlinked("SpriteSetScale");
        return;
    }
    memcpy(&flags, D_800591B8, sizeof(flags));
    a[0] = (unsigned long long)(uintptr_t)sprite;
    a[1] = (unsigned)(int)scale;
    a[2] = flags;
    (void)prover_spy("SpriteSetScale", 3, a, 0);
}

int func_8001EE74(unsigned short *frames);
int func_8001EE74(unsigned short *frames)
{
    unsigned long long a[1];

    if (!frontier_ba984_active) {
        frontier_ba984_unlinked("func_8001EE74");
        return 0;
    }
    a[0] = (unsigned long long)(uintptr_t)frames;
    /* rendering.c's body: the frame count in bits 9..14. */
    return (int)prover_spy("func_8001EE74", 1, a,
                           (unsigned)((frames[0] >> 9) & 0x3Fu));
}

void func_800222BC(void *sprite, void *package);
void func_800222BC(void *sprite, void *package)
{
    unsigned long long a[2];

    if (!frontier_ba984_active) {
        frontier_ba984_unlinked("func_800222BC");
        return;
    }
    a[0] = (unsigned long long)(uintptr_t)sprite;
    a[1] = (unsigned long long)(uintptr_t)package;
    (void)prover_spy("func_800222BC", 2, a, 0);
}

void func_800223B0(void *sprite, short angle);
void func_800223B0(void *sprite, short angle)
{
    unsigned long long a[2];

    if (!frontier_ba984_active) {
        frontier_ba984_unlinked("func_800223B0");
        return;
    }
    a[0] = (unsigned long long)(uintptr_t)sprite;
    a[1] = (unsigned)(int)angle;
    (void)prover_spy("func_800223B0", 2, a, 0);
}

void SpriteComputeTransformMatrix(void *sprite);
void SpriteComputeTransformMatrix(void *sprite)
{
    unsigned long long a[1];

    if (!frontier_ba984_active) {
        frontier_ba984_unlinked("SpriteComputeTransformMatrix");
        return;
    }
    a[0] = (unsigned long long)(uintptr_t)sprite;
    (void)prover_spy("SpriteComputeTransformMatrix", 1, a, 0);
}

/* The main-executable globals the sprite code reads natively.  They are
 * runtime bindings, which keep their bytes from case to case: the cases set
 * them, and restore what they held afterwards so no later case's inputs
 * move (D_800591AD = 1 sends other leaves' sprite code into placeholders). */
static const struct {
    const char *name;
    size_t width;
} kFrontierBa984Globals[] = {
    {"D_800591AD", 1}, {"D_80059198", 4}, {"D_800591A8", 4},
};
#define FRONTIER_BA984_GLOBALS \
    (sizeof(kFrontierBa984Globals) / sizeof(kFrontierBa984Globals[0]))

static uint8_t *frontier_ba984_binding(unsigned g)
{
    size_t d;

    for (d = 0; d < rt.data_count; d++) {
        if (rt.data[d].name != NULL &&
            strcmp(rt.data[d].name, kFrontierBa984Globals[g].name) == 0 &&
            rt.data[d].host != NULL && rt.data[d].size >= kFrontierBa984Globals[g].width)
            return rt.data[d].host;
    }
    fprintf(stderr, "BATTLE OVERLAY DIFFERENTIAL FAIL h-frontier fixture: %s is "
            "not a native binding\n", kFrontierBa984Globals[g].name);
    failures++;
    return NULL;
}

static void frontier_ba984_global(unsigned g, uint32_t value)
{
    uint8_t *host = frontier_ba984_binding(g);

    if (host != NULL)
        memcpy(host, &value, kFrontierBa984Globals[g].width);
}

static void frontier_ba984_side(void)
{
    uint32_t sp = case_sp != 0 ? case_sp : BATTLE_STACK_TOP;
    unsigned k;

    abb8_heap();
    for (k = 0; k < 10; k++)
        *(uint32_t *)battle_guest_resolve_native(sp + 0x10u + 4u * k) =
            frontier_ba984_stack[k];
}

static void frontier_ba984_hand_cases(void)
{
    const uint32_t sprite = FRONTIER_BA984_TASK + 0x38u;
    const uint32_t saved_rng = rng_state;
    uint8_t saved_globals[FRONTIER_BA984_GLOBALS][4];
    uint32_t args[4];
    unsigned c, i;

    if (!abi_leaf_listed("func_800BA984"))
        return;
    for (i = 0; i < FRONTIER_BA984_GLOBALS; i++) {
        uint8_t *host = frontier_ba984_binding(i);
        if (host != NULL)
            memcpy(saved_globals[i], host, kFrontierBa984Globals[i].width);
    }
    /* Its own sequence, and the shared one restored afterwards, so no later
     * hand case's inputs move. */
    rng_state = 0x12345678u ^ 0x800BA984u;
    for (c = 0; c < 8; c++) {
        begin_case(1u + (c & 1u), 0); /* nonzero fills only */
        /* The fixture area lies past begin_case()'s fill: fill it the same
         * way, so every byte the task and sprite start with is nonzero. */
        for (i = 0; i < 0x800u; i++)
            *(uint8_t *)PSX_ADDR(FRONTIER_BA984_TASK + i) =
                (c & 1u) == 0 ? 0xA5u : (uint8_t)(rnd() | 1u);
        /* The package: +8 -> its frame table, whose first halfword holds
         * the frame count (1..3) in bits 9..14. */
        *(uint32_t *)PSX_ADDR(FRONTIER_BA984_PKG + 8u) = 0x20u;
        *(uint16_t *)PSX_ADDR(FRONTIER_BA984_PKG + 0x20u) =
            (uint16_t)(0x8155u | ((1u + c % 3u) << 9));
        /* The animation table: +0 the low six bits func_8002435C reads, then
         * the offsets of animations 0..3; each animation: +0 flags (bits
         * 0..1, the signed scale in 2..7, the 11/12/13 skips), +2 / +4 the
         * offsets func_80023538 resolves. */
        *(uint16_t *)PSX_ADDR(FRONTIER_BA984_ANIM) = (uint16_t)(0x5A00u | (3u + c));
        for (i = 0; i < 4; i++) {
            uint32_t anim = FRONTIER_BA984_ANIM + 0x10u + 0x10u * i;
            *(uint16_t *)PSX_ADDR(FRONTIER_BA984_ANIM + 2u + 2u * i) =
                (uint16_t)(0x10u + 0x10u * i);
            *(uint16_t *)PSX_ADDR(anim) =
                (uint16_t)(((c + i) & 3u) | (((c * 5u + i * 9u) & 0x3Fu) << 2) |
                           ((((c >> 1) + i) & 7u) << 11) | 0x0400u);
            *(uint16_t *)PSX_ADDR(anim + 2u) = (uint16_t)(0x40u + 2u * i);
            *(uint16_t *)PSX_ADDR(anim + 4u) = (uint16_t)(0x60u + 2u * i);
        }
        /* The word func_800222BC would leave (sprite +0x110 +0x10). */
        *(uint32_t *)PSX_ADDR(sprite + 0x120u) =
            (uint32_t)(uintptr_t)PSX_ADDR(FRONTIER_BA984_ANIM);
        frontier_ba984_global(0, (c >> 1) & 1u);   /* D_800591AD */
        frontier_ba984_global(1, 1u + (c & 3u));   /* D_80059198 */
        frontier_ba984_global(2, 0x1800u + c);     /* D_800591A8 */
        /* Scalars stay out of the KSEG / scratchpad shapes the bridge
         * translates; their upper halves are nonzero. */
        args[0] = FRONTIER_BA984_PKG;
        args[1] = 0x5A5A0010u + c;                 /* texX */
        args[2] = 0x3C3C0020u + c;                 /* texY */
        args[3] = 0x0F0F0030u + c;                 /* clutX */
        frontier_ba984_stack[0] = 0x5A5A0040u + c; /* clutY */
        frontier_ba984_stack[1] = 0x3C3C0050u + c; /* func_8002435C arg6 */
        frontier_ba984_stack[2] = 0x0F0F1234u + c; /* x */
        frontier_ba984_stack[3] = 0x5A5A2345u + c; /* y */
        frontier_ba984_stack[4] = 0x3C3C3456u + c; /* z */
        frontier_ba984_stack[5] = 0x0F0F0000u + (c & 3u); /* animation */
        frontier_ba984_stack[6] = 0x5A5A4567u + c; /* +0x32 */
        frontier_ba984_stack[7] = 0x3C3C5678u + c; /* never read */
        frontier_ba984_stack[8] = 0x0F0F6789u + c; /* never read */
        frontier_ba984_stack[9] = 0x00A5000Bu + c; /* flags (word 13) */
        pre_side = frontier_ba984_side;
        frontier_ba984_active = 1;
        compare_name("func_800BA984", args);
        frontier_ba984_active = 0;
        pre_side = NULL;
    }
    rng_state = saved_rng;
    for (i = 0; i < FRONTIER_BA984_GLOBALS; i++) {
        uint8_t *host = frontier_ba984_binding(i);
        if (host != NULL)
            memcpy(host, saved_globals[i], kFrontierBa984Globals[i].width);
    }
}
/* ---- END h-frontier (800BA984) -------------------------------------------- */

/* ---- BEGIN h-frontier: 800BA984's task callbacks, through the real work list
 * 800BA984 registers its task with WorkListsAddTasks(0x19C, 0, 800BAC50,
 * 800BAB0C, 800BABDC): the timer, work and free callbacks.  These cases run
 * an adopted callback the way the port does.  The port's own work list
 * (work_list_port.c, linked by the run script with every global renamed
 * prover_wl_*, so nothing else here resolves to it) allocates the task on a
 * fresh heap and links it, and the callback reaches the body through the
 * work list's own WorkListInvokeCallback and PcPort_BattleMipsDispatchCallback
 * with the entry's host address, as in the game: the work callback from the
 * update pump (WorkListUpdate); the free callback the way WorkListsFreeAllEntries
 * and func_8001CE74 invoke one, through PcPort_WorkListInvokeSavedCallback
 * with the task's +0xC (WorkListsFreeAllEntries itself stops only once the
 * callback has unlinked the head, so a broken body would never return).
 * compare() runs each side through frontier_side_run: the retail side runs
 * the dispatch with force_interpret, so the interpreter runs the retail
 * callback; the host side runs it with XENO_DISABLE_CPU_INTERPRETER set, the
 * native-only route's rule, so only the adopted native body can run.  Each
 * side's setup (pre_side) resets the list state, makes the task as 800BA984
 * does -- the real WorkListsAddTasks call, then 800BA984's own writes (+4
 * and +0x20 = the sprite at +0x38, +0x1C = 0; its cases prove those) -- and
 * lays down the case's sprite.  The list heads, cursors and counters live
 * outside guest RAM: each side's are recorded after its run and compared
 * ("work-list" FAIL).  The callbacks' unlinked main-executable services are
 * spies while these cases run and the run script's placeholders otherwise:
 * func_8001E298 (the sprite renderer, not linked here) only records;
 * func_8001CE74, func_8001D3F4, WorkListRemoveTask and TimerWorkListRemoveTask
 * record, then run the real work list's.  Not seen by the harness's own
 * checks: the dispatched retail callback runs on run_guest_callback's CPU, so
 * the host-address rule and branch coverage do not apply to it (its host
 * addresses are the heap blocks in guest RAM). */
#define FRONTIER_TASKS_OT 0x800C9000u /* g_GfxCurOT: an OT the spy only sees */
typedef void (*FrontierTaskCallback)(void *);
void *prover_wl_WorkListsAddTasks(uint32_t size, void *owner,
                                  FrontierTaskCallback timer,
                                  FrontierTaskCallback work,
                                  FrontierTaskCallback on_free);
void *prover_wl_TimerWorkListAllocateTask(void *owner, int32_t size);
void prover_wl_WorkListsReset(void);
void prover_wl_WorkListUpdate(void);
void prover_wl_TimerWorkListUpdate(void);
void prover_wl_TimerWorkListSetTaskCallback(void *task, FrontierTaskCallback callback);
void prover_wl_PcPort_WorkListInvokeSavedCallback(uint32_t callback, void *argument);
void prover_wl_func_8001CE74(void *owner);
void prover_wl_func_8001D3F4(void *sprite);
void prover_wl_WorkListRemoveTask(void *entry);
void prover_wl_TimerWorkListRemoveTask(void *entry);
void *HeapAlloc(unsigned size, unsigned flags); /* memory.c, linked */
/* work_list_port.c's own globals (its BSS, renamed with it). */
extern int32_t prover_wl_D_80059184; /* entry id counter */
extern int32_t prover_wl_D_80059464;
extern uint8_t prover_wl_D_800591AC;
extern uint8_t prover_wl_D_800591AF; /* HeapAlloc mode */
/* What it references and the port defines elsewhere. */
void *prover_wl_g_WorkList, *prover_wl_g_TimerWorkList;
void *prover_wl_D_80059590, *prover_wl_D_800594C0; /* the pumps' cursors */
int32_t prover_wl_g_NumWorkListEntries, prover_wl_g_NumTimerWorkListEntries;
int32_t prover_wl_g_WorkListCurTimer;
int32_t prover_wl_D_80059190; /* the sprite list (func_8001D2B0 / 8001D3F4) */
int16_t prover_wl_D_80059494;
uint8_t prover_wl_D_8005A474[16], prover_wl_D_8006BE10[16]; /* compared by address */
void prover_wl_func_8001DAE8(void);
void prover_wl_func_8001DAE8(void) { frontier_ba984_unlinked("func_8001DAE8"); }
void prover_wl_func_800234AC(void);
void prover_wl_func_800234AC(void) { frontier_ba984_unlinked("func_800234AC"); }
void prover_wl_func_8001F8E8(void);
void prover_wl_func_8001F8E8(void) { frontier_ba984_unlinked("func_8001F8E8"); }

static void (*frontier_tasks_pump)(void);     /* the work list's dispatch */
static uint32_t frontier_tasks_callback;      /* the callback it reaches */
static void (*frontier_tasks_lay)(uint8_t *task); /* the case's sprite */
static uint8_t *frontier_tasks_task;          /* the task the setup made */
/* Step 9's frame-equivalence rule (see the 801E6E48 block): the word, the
 * retail side's value there, the host frame it stands for, cases mapped. */
static uint8_t *frontier_tasks_frame_word;
static uint32_t frontier_tasks_frame_retail;
static uint32_t (*frontier_tasks_frame_expect)(uint32_t cursor);
static unsigned frontier_tasks_frame_mapped;

void func_8001E298(void *sprite, void *ot);
void func_8001E298(void *sprite, void *ot)
{
    unsigned long long a[2];

    if (!frontier_tasks_active) {
        frontier_ba984_unlinked("func_8001E298");
        return;
    }
    a[0] = (unsigned long long)(uintptr_t)sprite;
    a[1] = (unsigned long long)(uintptr_t)ot;
    (void)prover_spy("func_8001E298", 2, a, 0);
}

/* A spied service that then runs the real work list's (1), or, outside these
 * cases, the placeholder (0). */
static int frontier_tasks_service(const char *name, void *argument)
{
    unsigned long long a[1];

    if (!frontier_tasks_active) {
        frontier_ba984_unlinked(name);
        return 0;
    }
    a[0] = (unsigned long long)(uintptr_t)argument;
    (void)prover_spy(name, 1, a, 0);
    return 1;
}

void func_8001CE74(void *owner);
void func_8001CE74(void *owner)
{
    if (frontier_tasks_service("func_8001CE74", owner))
        prover_wl_func_8001CE74(owner);
}

void func_8001D3F4(void *sprite);
void func_8001D3F4(void *sprite)
{
    if (frontier_tasks_service("func_8001D3F4", sprite))
        prover_wl_func_8001D3F4(sprite);
}

void WorkListRemoveTask(void *entry);
void WorkListRemoveTask(void *entry)
{
    if (frontier_tasks_service("WorkListRemoveTask", entry))
        prover_wl_WorkListRemoveTask(entry);
}

void TimerWorkListRemoveTask(void *entry);
void TimerWorkListRemoveTask(void *entry)
{
    if (frontier_tasks_service("TimerWorkListRemoveTask", entry))
        prover_wl_TimerWorkListRemoveTask(entry);
}

/* Step 10a (800BB760): TimerWorkListAllocateTask is a spy in these cases that
 * then runs the real (renamed) one, which first gets the D_800591AC /
 * D_800591AF bytes the caller just set (the port keeps one copy of each;
 * the renamed work list reads its own), and the run script's placeholder
 * otherwise (it had no definition here before: the bridge could not resolve
 * it, inconclusive either way). */
void *TimerWorkListAllocateTask(void *owner, int size);
void *TimerWorkListAllocateTask(void *owner, int size)
{
    unsigned long long a[2];

    if (!frontier_tasks_active) {
        frontier_ba984_unlinked("TimerWorkListAllocateTask");
        return NULL;
    }
    a[0] = (unsigned long long)(uintptr_t)owner;
    a[1] = (unsigned)size;
    (void)prover_spy("TimerWorkListAllocateTask", 2, a, 0);
    prover_wl_D_800591AC = *(uint8_t *)battle_guest_resolve_native(0x800591ACu);
    prover_wl_D_800591AF = *(uint8_t *)battle_guest_resolve_native(0x800591AFu);
    return prover_wl_TimerWorkListAllocateTask(owner, size);
}

/* The work list's state outside guest RAM, recorded after each side's run. */
typedef struct {
    void *work, *timer, *cursor, *current;
    int32_t work_count, timer_count, flagged, sprites, next_id;
} FrontierListState;
static FrontierListState frontier_tasks_after[2]; /* retail, host */
static int frontier_tasks_recorded[2];

static void frontier_tasks_record(int side)
{
    FrontierListState *s = &frontier_tasks_after[side];

    memset(s, 0, sizeof(*s));
    s->work = prover_wl_g_WorkList;
    s->timer = prover_wl_g_TimerWorkList;
    s->cursor = prover_wl_D_80059590;
    s->current = prover_wl_D_800594C0;
    s->work_count = prover_wl_g_NumWorkListEntries;
    s->timer_count = prover_wl_g_NumTimerWorkListEntries;
    s->flagged = prover_wl_D_80059464;
    s->sprites = prover_wl_D_80059190;
    s->next_id = prover_wl_D_80059184;
    frontier_tasks_recorded[side] = 1;
}

/* Whether the host side's dispatch can only run native code: an adopted
 * leaf with its typed row, or a body the module host would run -- and the
 * same for the bodies a module callback calls. */
static int frontier_tasks_native_one(uint32_t target)
{
    const BattleGuestThunk *thunk;

    if (target >= 0x801E4000u && target < 0x801E96C0u)
        return battle_module_0c4_lift_ready(target);
    thunk = battle_guest_thunk(target);
    return thunk != NULL && battle_guest_native_host(thunk) != NULL;
}

static int frontier_tasks_native(uint32_t callback)
{
    static const struct {
        uint32_t callback, callee[2];
    } kCallees[] = {
        {0x801E6E48u, {0x801E6DC8u, 0}},
        {0x801E6D6Cu, {0x801E6A4Cu, 0x800BB760u}},
        {0x800BB6E0u, {0x800BB690u, 0x800A9540u}},
        {0x800BB690u, {0x800A9540u, 0}},
    };
    unsigned i, k;

    if (!frontier_tasks_native_one(callback))
        return 0;
    for (i = 0; i < sizeof(kCallees) / sizeof(kCallees[0]); i++)
        if (kCallees[i].callback == callback)
            for (k = 0; k < 2; k++)
                if (kCallees[i].callee[k] != 0 &&
                    !frontier_tasks_native_one(kCallees[i].callee[k]))
                    return 0;
    return 1;
}

/* The frame-equivalence rule, host side: the word holds exactly the frame
 * the lift saved and the retail side's value there is a guest stack-window
 * address, so the two stand for the same frame; only then is the host word
 * rewritten to the retail one. */
static void frontier_tasks_frame_map(uint32_t cursor)
{
    const uint32_t r = frontier_tasks_frame_retail, off = r & 0x1FFFFFu;
    uint32_t host;

    memcpy(&host, frontier_tasks_frame_word, 4);
    if ((r & 0xFFE00000u) == 0x80000000u && off >= STACK_SKIP_LO &&
        off < STACK_SKIP_HI && host == frontier_tasks_frame_expect(cursor)) {
        memcpy(frontier_tasks_frame_word, &r, 4);
        frontier_tasks_frame_mapped++;
    }
}

/* compare()'s side runner for these cases. */
static int frontier_tasks_run(int retail)
{
    PcPortMipsCpu *const saved_bridge = rt.bridge_cpu;
    const uint32_t cursor = bl_cursor; /* the module host's lift frames */

    memset(&cpu, 0, sizeof(cpu)); /* v0 of a void callback is not compared */
    retail_host_refs = 0;         /* run_interpreted()'s counter: not this CPU */
    rt.bridge_cpu = NULL;         /* both sides start at BATTLE_STACK_TOP */
    unsetenv("XENO_DISABLE_CPU_INTERPRETER");
    if (retail) {
        rt.force_interpret = 1;
        frontier_tasks_pump();
        rt.bridge_cpu = saved_bridge;
        if (frontier_tasks_frame_word != NULL)
            memcpy(&frontier_tasks_frame_retail, frontier_tasks_frame_word, 4);
        frontier_tasks_record(0);
        return PC_PORT_MIPS_HALTED;
    }
    rt.force_interpret = 0;
    /* With the interpreter disabled a non-native callback aborts in the
     * dispatch; report it as a bridge FAIL instead. */
    if (!frontier_tasks_native(frontier_tasks_callback)) {
        rt.bridge_cpu = saved_bridge;
        return -2;
    }
    setenv("XENO_DISABLE_CPU_INTERPRETER", "1", 1);
    frontier_tasks_pump();
    unsetenv("XENO_DISABLE_CPU_INTERPRETER");
    rt.bridge_cpu = saved_bridge;
    if (frontier_tasks_frame_word != NULL)
        frontier_tasks_frame_map(cursor);
    frontier_tasks_record(1);
    return 1;
}

/* The free callback's dispatch: the task's +0xC through the work list's own
 * WorkListInvokeCallback, as WorkListsFreeAllEntries and func_8001CE74 call
 * each entry's. */
static void frontier_tasks_free(void)
{
    uint32_t on_free;

    memcpy(&on_free, frontier_tasks_task + 0xC, 4);
    prover_wl_PcPort_WorkListInvokeSavedCallback(on_free, frontier_tasks_task);
}

static unsigned frontier_tasks_case;
static uint8_t frontier_tasks_fill[0x19Cu - 0x38u]; /* the sprite's start */

/* The renamed work list's state, the same before each side. */
static void frontier_tasks_reset(void)
{
    prover_wl_WorkListsReset();
    prover_wl_D_80059184 = 0x40;
    prover_wl_D_80059464 = 0;
    prover_wl_D_800591AC = 0;
    prover_wl_D_800591AF = 0;
    prover_wl_D_80059590 = prover_wl_D_800594C0 = NULL;
    prover_wl_D_80059494 = 0;
    prover_wl_D_80059190 = 0;
}

/* pre_side: the same list state, task and sprite before each side. */
static void frontier_tasks_side(void)
{
    uint8_t *task, *sprite;
    uint32_t word;

    abb8_heap();
    frontier_tasks_reset();
    task = prover_wl_WorkListsAddTasks(
        0x19Cu, NULL, (FrontierTaskCallback)(uintptr_t)0x800BAC50u,
        (FrontierTaskCallback)(uintptr_t)0x800BAB0Cu,
        (FrontierTaskCallback)(uintptr_t)0x800BABDCu);
    frontier_tasks_task = task;
    if (task == NULL) { /* a fresh 128 KiB heap; never expected */
        fprintf(stderr, "BATTLE OVERLAY DIFFERENTIAL FAIL h-frontier fixture: "
                "WorkListsAddTasks returned no task\n");
        failures++;
        return;
    }
    sprite = task + 0x38;
    memcpy(sprite, frontier_tasks_fill, sizeof(frontier_tasks_fill));
    word = (uint32_t)(uintptr_t)sprite; /* -no-pie: a host address fits */
    memcpy(task + 0x04, &word, 4);
    memcpy(task + 0x20, &word, 4);
    memset(task + 0x1C, 0, 4);
    frontier_tasks_lay(task);
}

/* Shared by the callbacks' cases: one dispatched comparison, with `side`
 * as each side's setup. */
static void frontier_tasks_compare(const char *name, void (*side)(void))
{
    static const uint32_t args[4] = {0, 0, 0, 0}; /* the dispatch supplies a0 */
    char detail[200];
    const FrontierListState *r = &frontier_tasks_after[0], *h = &frontier_tasks_after[1];

    frontier_tasks_recorded[0] = frontier_tasks_recorded[1] = 0;
    pre_side = side;
    frontier_side_run = frontier_tasks_run;
    frontier_tasks_active = 1;
    compare_name(name, args);
    /* Also after a host fault, which leaves the runner early. */
    frontier_tasks_active = 0;
    frontier_side_run = NULL;
    pre_side = NULL;
    frontier_tasks_frame_word = NULL;
    frontier_heap_stack_lo = frontier_heap_stack_hi = 0;
    unsetenv("XENO_DISABLE_CPU_INTERPRETER");
    rt.force_interpret = 0;
    if (!frontier_tasks_recorded[0] || !frontier_tasks_recorded[1] ||
        memcmp(r, h, sizeof(*r)) == 0)
        return;
    snprintf(detail, sizeof(detail),
             "retail work=%08x timer=%08x n=%d/%d sprites=%08x "
             "host work=%08x timer=%08x n=%d/%d sprites=%08x",
             (unsigned)(uintptr_t)r->work, (unsigned)(uintptr_t)r->timer,
             r->work_count, r->timer_count, (unsigned)r->sprites,
             (unsigned)(uintptr_t)h->work, (unsigned)(uintptr_t)h->timer,
             h->work_count, h->timer_count, (unsigned)h->sprites);
    fail(name, "work-list", detail);
    last_case_proven = 0;
}

/* 800BAB0C, the work callback: unless D_800C3664 is set, RotTransPers the
 * sprite's (+2, +6, +0xA) through D_800D30BC, depth = (OTZ >> D_80050100) +
 * sprite +0x30 (0 when flag bit 15 is set), stored at +0x2E, and
 * func_8001E298(sprite, g_GfxCurOT + depth * 4) when 1 <= depth <= 0xFFF.
 * With D_800D30BC's third row r31..r33 and TRZ, SZ3 = TRZ + (r31 vx + r32 vy
 * + r33 vz) / 0x1000 and OTZ = SZ3 / 4.  Classes: 1 inactive, 2 rendered,
 * 4 not rendered (depth 0, past 0xFFF, or negative). */
static const struct {
    int16_t vx, vy, vz, bias; /* sprite +2, +6, +0xA, +0x30 */
    int16_t r31, r32, r33;    /* D_800D30BC third row */
    int32_t trz;
    uint8_t shift, inactive;  /* D_80050100, D_800C3664 */
} kFrontierBab0c[] = {
    {12, -8, 40, 0, 0, 0, 0x1000, 0x800, 0, 0},              /* rendered */
    {-30, 25, -60, 5, 0, 0, 0x1000, 0x1000, 1, 0},
    {100, -50, 90, -12, 0x200, -0x100, 0xF00, 0x2000, 2, 0},
    {0, 0, 0, 1, 0, 0, 0x1000, 0x400, 0, 0},
    {-200, 300, 50, 0x40, 0x300, 0x200, 0xE00, 0x1800, 1, 0},
    {0, 0, 0, -0x100, 0, 0, 0x1000, 0x400, 0, 0},           /* depth 0 */
    {20, 20, 20, 0, 0, 0, 0x1000, 0x6000, 0, 0},            /* past 0xFFF */
    {-5, 5, -5, 0xFFF, 0, 0, 0x1000, 0x400, 0, 0},          /* past 0xFFF */
    {7, 7, 7, -0x7FFF, 0, 0, 0x1000, 0x800, 1, 0},          /* negative */
    {3, -3, 3, 2, 0, 0, 0x1000, -0x100, 0, 0},              /* behind: SZ3 0 */
    {12, -8, 40, 0, 0, 0, 0x1000, 0x800, 0, 1},             /* inactive */
    {-30, 25, -60, 5, 0x200, -0x100, 0xF00, 0x6000, 2, 0x80},
};
#define FRONTIER_BAB0C_CASES (sizeof(kFrontierBab0c) / sizeof(kFrontierBab0c[0]))

static void frontier_bab0c_lay(uint8_t *task)
{
    uint8_t *sprite = task + 0x38;

    memcpy(sprite + 0x02, &kFrontierBab0c[frontier_tasks_case].vx, 2);
    memcpy(sprite + 0x06, &kFrontierBab0c[frontier_tasks_case].vy, 2);
    memcpy(sprite + 0x0A, &kFrontierBab0c[frontier_tasks_case].vz, 2);
    memcpy(sprite + 0x30, &kFrontierBab0c[frontier_tasks_case].bias, 2);
}

static void frontier_bab0c_cases(uint8_t *shift_host, uint8_t *ot_host)
{
    unsigned c, i, seen = 0;

    rng_state = 0x12345678u ^ 0x800BAB0Cu;
    for (c = 0; c < FRONTIER_BAB0C_CASES; c++) {
        uint64_t ot = (uint64_t)(uintptr_t)PSX_ADDR(FRONTIER_TASKS_OT);
        uint32_t shift = kFrontierBab0c[c].shift;
        uint8_t *m;

        begin_case(1u + (c & 1u), 0); /* nonzero fills only */
        for (i = 0; i < sizeof(frontier_tasks_fill); i++)
            frontier_tasks_fill[i] = (c & 1u) == 0 ? 0xA5u : (uint8_t)(rnd() | 1u);
        *(uint8_t *)PSX_ADDR(0x800C3664u) = kFrontierBab0c[c].inactive;
        /* D_800D30BC: rows 1-2 an identity-like rotation, row 3 the case's,
         * translation (0x40 + c, -0x20, TRZ). */
        m = (uint8_t *)PSX_ADDR(0x800D30BCu);
        memset(m, 0, 0x20);
        *(int16_t *)(m + 0x0) = 0x1000;
        *(int16_t *)(m + 0x2) = (int16_t)(0x10 * c);
        *(int16_t *)(m + 0x8) = 0x1000;
        memcpy(m + 0xC, &kFrontierBab0c[c].r31, 2);
        memcpy(m + 0xE, &kFrontierBab0c[c].r32, 2);
        memcpy(m + 0x10, &kFrontierBab0c[c].r33, 2);
        *(int32_t *)(m + 0x14) = (int32_t)(0x40 + c);
        *(int32_t *)(m + 0x18) = -0x20;
        memcpy(m + 0x1C, &kFrontierBab0c[c].trz, 4);
        memcpy(shift_host, &shift, 4);
        memcpy(ot_host, &ot, 8);
        frontier_tasks_case = c;
        frontier_tasks_lay = frontier_bab0c_lay;
        frontier_tasks_pump = prover_wl_WorkListUpdate;
        frontier_tasks_callback = 0x800BAB0Cu;
        frontier_tasks_compare("func_800BAB0C", frontier_tasks_side);
        if (last_case_proven)
            seen |= kFrontierBab0c[c].inactive != 0 ? 1u
                    : spy_expected_count != 0      ? 2u
                                                   : 4u;
    }
    require_outcomes("func_800BAB0C", seen, 7u);
}

/* 800BABDC, the free callback: HeapFree the sprite's +0x20 record's +0x2C
 * buffer when set, func_8001CE74(task) (unlink and free the task's children),
 * func_8001D3F4(sprite) (take it off the D_80059190 sprite list), unlink the
 * work and timer entries, HeapFree the task.  The record is a heap block:
 * +0x2C the buffer, +0x38 the sprite list's link (func_8001D2B0 pushes a
 * sprite on D_80059190 through it).  Case c: bit 0 a buffer; bit 1 a child
 * (TimerWorkListAllocateTask(task, 0x10), freed through its own free
 * callback); c % 3 the sprite list: ours first, between two others, or not
 * on it.  Classes: 1 buffer freed, 2 no buffer, 4 a child released (the
 * retail side's timer list is empty afterwards). */
static uint8_t *frontier_babdc_sprite(void)
{
    uint8_t *sprite = HeapAlloc(0x40u, 0), *sub = HeapAlloc(0x40u, 0);
    uint32_t word = (uint32_t)(uintptr_t)sub;

    if (sprite == NULL || sub == NULL)
        return NULL;
    memset(sprite, 0x5A, 0x40);
    memset(sub, 0x3C, 0x40);
    memcpy(sprite + 0x20, &word, 4);
    return sprite;
}

static void frontier_babdc_link(uint8_t *sprite, const uint8_t *next)
{
    uint32_t sub, word = (uint32_t)(uintptr_t)next;

    memcpy(&sub, sprite + 0x20, 4);
    memcpy((uint8_t *)(uintptr_t)sub + 0x38, &word, 4);
}

static void frontier_babdc_lay(uint8_t *task)
{
    const unsigned c = frontier_tasks_case;
    uint8_t *sprite = task + 0x38, *sub = HeapAlloc(0x40u, 0);
    uint8_t *buffer = (c & 1u) != 0 ? HeapAlloc(0x20u, 0) : NULL;
    uint8_t *first = frontier_babdc_sprite(), *last = frontier_babdc_sprite();
    uint32_t word;

    if (sub == NULL || ((c & 1u) != 0 && buffer == NULL) || first == NULL ||
        last == NULL) { /* a fresh 128 KiB heap; never expected */
        fprintf(stderr, "BATTLE OVERLAY DIFFERENTIAL FAIL h-frontier fixture: "
                "HeapAlloc failed\n");
        failures++;
        return;
    }
    memset(sub, 0x69, 0x40);
    word = (uint32_t)(uintptr_t)sub;
    memcpy(sprite + 0x20, &word, 4);
    word = (uint32_t)(uintptr_t)buffer;
    memcpy(sub + 0x2C, &word, 4);
    frontier_babdc_link(last, NULL);
    switch (c % 3u) {
    case 0: /* ours, last */
        frontier_babdc_link(sprite, last);
        prover_wl_D_80059190 = (int32_t)(uintptr_t)sprite;
        break;
    case 1: /* first, ours, last */
        frontier_babdc_link(first, sprite);
        frontier_babdc_link(sprite, last);
        prover_wl_D_80059190 = (int32_t)(uintptr_t)first;
        break;
    default: /* first, last */
        frontier_babdc_link(first, last);
        frontier_babdc_link(sprite, NULL);
        prover_wl_D_80059190 = (int32_t)(uintptr_t)first;
        break;
    }
    if ((c & 2u) != 0)
        (void)prover_wl_TimerWorkListAllocateTask(task, 0x10);
}

static void frontier_babdc_cases(void)
{
    unsigned c, i, seen = 0;

    if (!abi_leaf_listed("func_800BABDC"))
        return;
    rng_state = 0x12345678u ^ 0x800BABDCu;
    for (c = 0; c < 8; c++) {
        begin_case(1u + (c & 1u), 0); /* nonzero fills only */
        for (i = 0; i < sizeof(frontier_tasks_fill); i++)
            frontier_tasks_fill[i] = (c & 1u) == 0 ? 0xA5u : (uint8_t)(rnd() | 1u);
        frontier_tasks_case = c;
        frontier_tasks_lay = frontier_babdc_lay;
        frontier_tasks_pump = frontier_tasks_free;
        frontier_tasks_callback = 0x800BABDCu;
        frontier_tasks_compare("func_800BABDC", frontier_tasks_side);
        if (!last_case_proven)
            continue;
        seen |= (c & 1u) != 0 ? 1u : 2u;
        /* The child is released only if func_8001CE74 found it: the timer
         * list (child first, then the task) must be empty afterwards. */
        if ((c & 2u) != 0) {
            if (frontier_tasks_after[0].timer == NULL)
                seen |= 4u;
            else
                fail("func_800BABDC", "fixture", "the child timer task was not released");
        }
    }
    require_outcomes("func_800BABDC", seen, 7u);
}

/* A native binding the work callback reads: seeded per case, restored after. */
static uint8_t *frontier_tasks_binding(const char *name, size_t width)
{
    size_t d;

    for (d = 0; d < rt.data_count; d++) {
        if (rt.data[d].name != NULL && strcmp(rt.data[d].name, name) == 0 &&
            rt.data[d].host != NULL && rt.data[d].size >= width)
            return rt.data[d].host;
    }
    fprintf(stderr, "BATTLE OVERLAY DIFFERENTIAL FAIL h-frontier fixture: %s is "
            "not a native binding\n", name);
    failures++;
    return NULL;
}

/* 800BB760 (step 10a): 801E6A4C (and 800BAF48) call it with a slot; it
 * queues a 4-byte timer task with the callback 800BB6E0 and the slot at
 * +0x1C, D_800591AC cleared and D_800591AF set around the allocation (the
 * first restored, the second cleared after), and bumps D_800C35D8.  Each
 * side calls it through the runtime's own jal service
 * (PcPort_BattleMipsCallJal, the path a lifted caller's jal takes): retail
 * interpreted, host with the interpreter disabled, as for the dispatched
 * callbacks; the task comes from the real (renamed) work list on a fresh
 * heap.  Classes: 1 D_800591AC was set (its restore is seen), 2 clear. */
static uint32_t frontier_tasks_arg; /* a0 of a called leaf */
static uint8_t frontier_bb760_flag;
static uint32_t frontier_bb760_pending;

static void frontier_tasks_call(void)
{
    uint32_t args[1] = {frontier_tasks_arg}, v0 = 0;

    initialize_cpu(&cpu, &rt); /* the caller: $sp at BATTLE_STACK_TOP */
    rt.bridge_cpu = &cpu;
    (void)PcPort_BattleMipsCallJal(frontier_tasks_callback, args, 1, &v0, NULL);
}

/* pre_side: the same lists, heap and globals before each side. */
static void frontier_bb760_side(void)
{
    abb8_heap();
    frontier_tasks_reset();
    frontier_tasks_task = NULL;
    *(uint8_t *)battle_guest_resolve_native(0x800591ACu) = frontier_bb760_flag;
    *(uint8_t *)battle_guest_resolve_native(0x800591AFu) = 0;
    *(uint32_t *)PSX_ADDR(0x800C35D8u) = frontier_bb760_pending;
}

static void frontier_bb760_cases(void)
{
    uint8_t saved_flag, saved_mode;
    unsigned c, seen = 0;

    if (!abi_leaf_listed("func_800BB760"))
        return;
    saved_flag = *(uint8_t *)battle_guest_resolve_native(0x800591ACu);
    saved_mode = *(uint8_t *)battle_guest_resolve_native(0x800591AFu);
    for (c = 0; c < 6; c++) {
        begin_case(1u + (c & 1u), 0); /* nonzero fills only */
        frontier_bb760_flag = c >= 3 ? 1u : 0u;
        frontier_bb760_pending = c * 0x101u;
        frontier_tasks_arg = c % 3u; /* the slot */
        frontier_tasks_pump = frontier_tasks_call;
        frontier_tasks_callback = 0x800BB760u;
        frontier_tasks_compare("func_800BB760", frontier_bb760_side);
        if (last_case_proven)
            seen |= frontier_bb760_flag != 0 ? 1u : 2u;
    }
    require_outcomes("func_800BB760", seen, 3u);
    *(uint8_t *)battle_guest_resolve_native(0x800591ACu) = saved_flag;
    *(uint8_t *)battle_guest_resolve_native(0x800591AFu) = saved_mode;
}

/* Step 11: archive-loading slot callbacks. Retail interprets all three bodies;
 * host dispatch disables the interpreter. Archive services are spies (the
 * production 80029AFC is archive_port.c's override); the heap and timer list
 * are real. These are body/ABI fixtures, not archive I/O parity evidence.
 *
 * Switched-stack equivalence: ONLY BB6E0's ready dispatch excludes the used
 * callee-frame span below block+EFC from RAM comparison and classifies it as
 * a frame for spy arguments. Sizes come from the retail prologue words.
 * The saved caller sp at +F00 still requires the exact mainc113 frame;
 * every other byte compares normally. Output reads from the span remain
 * observed in the archive-service call log. Both kinds of mutant are gated. */
extern uint32_t bl_frame_top_mainc113(void); /* production never calls it */
static unsigned frontier_archive_case;
static uint32_t frontier_archive_target;
static unsigned frontier_archive_seen_map;

int ArchiveGetArchiveOffsetIndices(int32_t *index, int32_t *sub);
int ArchiveGetArchiveOffsetIndices(int32_t *index, int32_t *sub)
{
    const unsigned long long a[] = {(uintptr_t)index, (uintptr_t)sub};
    int32_t i = 0x13 + (int32_t)frontier_archive_case;
    int32_t s = 0x27 + (int32_t)frontier_archive_case * 3;

    if (!frontier_archive_active) {
        g_prover_placeholder_hits++;
        return 0;
    }
    (void)prover_spy("ArchiveGetArchiveOffsetIndices", 2, a, 0);
    memcpy(index, &i, 4);
    memcpy(sub, &s, 4);
    return 0;
}

static uint32_t frontier_bb6e0_frame(uint32_t cursor)
{
    (void)cursor; /* the exported TU is distinct from this test's frame area */
    return ((bl_frame_top_mainc113() - 0x60u) & ~7u) - 0x20u;
}

static unsigned frontier_archive_prologue(uint32_t address)
{
    uint32_t word;
    memcpy(&word, PSX_ADDR(address), 4);
    if ((word & 0xFFFF0000u) != 0x27BD0000u || (int16_t)word >= 0) {
        fail("func_800BB6E0", "fixture", "callee has no addiu sp,sp,-N prologue");
        return 0;
    }
    return (unsigned)-(int16_t)word;
}

static void frontier_archive_side(void)
{
    unsigned kind = frontier_archive_case % 3u;
    unsigned slot = frontier_archive_case % 3u;
    uint8_t *task = NULL, *block;
    uint32_t off;

    abb8_heap();
    frontier_tasks_reset();
    frontier_tasks_frame_word = NULL;
    frontier_heap_stack_lo = frontier_heap_stack_hi = 0;
    *(uint8_t *)PSX_ADDR(0x800CCD88u + slot * 0x170u) = (uint8_t)kind;
    *(uint8_t *)PSX_ADDR(0x800CCDE8u + slot * 0x170u) =
        frontier_archive_case % 3u == 0 ? 0u : frontier_archive_case % 3u == 1 ? 2u : 5u;
    *(uint8_t *)PSX_ADDR(0x800C3508u + kind * 2u) = (uint8_t)(9u + kind * 5u);
    *(uint8_t *)PSX_ADDR(0x800C3509u + kind * 2u) = 3;
    *(uint8_t *)PSX_ADDR(0x800C3CB8u) =
        frontier_archive_target == 0x800BB6E0u && frontier_archive_case < 4u
            ? (frontier_archive_case < 2u ? 0u : 0x83u) : 0u;
    frontier_archive_busy = frontier_archive_target == 0x800BB6E0u &&
                            frontier_archive_case < 2u ? 1u : 0u;
    if (frontier_archive_target == 0x800A9540u) {
        frontier_tasks_arg = slot;
        frontier_tasks_task = NULL;
        return;
    }
    task = prover_wl_TimerWorkListAllocateTask(NULL, 4);
    frontier_tasks_task = task;
    if (task == NULL) {
        fail("func_800BB6E0", "fixture", "timer allocation failed");
        return;
    }
    memcpy(task + 0x1C, &slot, 4);
    prover_wl_TimerWorkListSetTaskCallback(task,
        (FrontierTaskCallback)(uintptr_t)frontier_archive_target);
    if (frontier_archive_target != 0x800BB6E0u || frontier_archive_case < 4u)
        return;
    block = HeapAlloc(0x1000u, 1);
    if (block == NULL) {
        fail("func_800BB6E0", "fixture", "stack allocation failed");
        return;
    }
    HeapFree(block);
    off = (uint32_t)(block - g_PsxRam);
    frontier_heap_stack_hi = off + 0xEFCu;
    frontier_heap_stack_lo = frontier_heap_stack_hi -
        frontier_archive_prologue(0x800BB690u) - frontier_archive_prologue(0x800A9540u);
    frontier_tasks_frame_word = block + 0xF00u;
    frontier_tasks_frame_expect = frontier_bb6e0_frame;
}

static void frontier_archive_cases(void)
{
    static const struct { const char *name; uint32_t target; unsigned cases; } runs[] = {
        {"func_800A9540", 0x800A9540u, 6},
        {"func_800BB690", 0x800BB690u, 6},
        {"func_800BB6E0", 0x800BB6E0u, 10},
    };
    unsigned r, c, mapped, seen;

    frontier_archive_active = 1;
    for (r = 0; r < sizeof(runs) / sizeof(runs[0]); r++) {
        if (!abi_leaf_listed(runs[r].name))
            continue;
        seen = 0;
        frontier_archive_target = runs[r].target;
        frontier_tasks_callback = runs[r].target;
        frontier_tasks_pump = runs[r].target == 0x800A9540u
            ? frontier_tasks_call : prover_wl_TimerWorkListUpdate;
        for (c = 0; c < runs[r].cases; c++) {
            begin_case(1u + (c & 1u), 0);
            frontier_archive_case = c;
            mapped = frontier_tasks_frame_mapped;
            frontier_tasks_compare(runs[r].name, frontier_archive_side);
            if (last_case_proven) {
                if (runs[r].target != 0x800A9540u) {
                    uint32_t callback, off = (uint32_t)(frontier_tasks_task - g_PsxRam);
                    uint32_t want = runs[r].target == 0x800BB6E0u && c < 4u
                        ? 0x800BB6E0u : 0x800BB620u;
                    memcpy(&callback, expected + off + 8u, 4);
                    if (callback != want || frontier_tasks_after[0].timer_count != 1 ||
                        expected[0xC3CB8u] != (want == 0x800BB620u ? 1u : c < 2u ? 0u : 0x83u)) {
                        fail(runs[r].name, "fixture", "retail callback/counter outcome missing");
                        continue;
                    }
                }
                if (runs[r].target == 0x800BB6E0u && c >= 4u) {
                    if (frontier_tasks_frame_mapped != mapped + 1u) {
                        fail(runs[r].name, "fixture", "saved sp did not map exactly");
                        continue;
                    }
                    frontier_archive_seen_map++;
                }
                seen++;
            }
        }
        if (seen != runs[r].cases)
            fail(runs[r].name, "fixture", "not all archive cases completed");
    }
    frontier_archive_active = 0;
    frontier_archive_busy = 0;
    fprintf(stderr, "[archive-frame-word] mapped=%u\n", frontier_archive_seen_map);
}

/* Registered native callback parity through lifted bl_jal vs interpreted
 * jalr. The real renamed TimerWorkListDeleteTask is deliberately the target:
 * the linked GCC port object has an unaligned x86 function entry. Do not align
 * a wrapper and accidentally conceal the production guard problem. */
void prover_wl_TimerWorkListDeleteTask(void *task);
void TimerWorkListDeleteTask(void *task);
void TimerWorkListDeleteTask(void *task)
{
    (void)task;
    g_prover_placeholder_hits++; /* no new service outside the scoped fixture */
}

#define FRONTIER_JALR_PROGRAM 0x801E1000u
static uint32_t frontier_jalr_target;

static void frontier_jalr_setup(void)
{
    uint32_t program[] = {
        0x27BDFFE0u, /* addiu sp,sp,-20 */
        0xAFBF001Cu, /* sw ra,1c(sp) */
        0x00A0C821u, /* addu t9,a1,zero */
        0,          /* nop */
        0x0320F809u, /* jalr t9 */
        0,          /* nop */
        0x8FBF001Cu, /* lw ra,1c(sp) */
        0x27BD0020u, /* addiu sp,sp,20 */
        0x03E00008u, /* jr ra */
        0,
    };
    abb8_heap();
    frontier_tasks_reset();
    frontier_tasks_task = prover_wl_TimerWorkListAllocateTask(NULL, 4);
    memcpy(PSX_ADDR(FRONTIER_JALR_PROGRAM), program, sizeof(program));
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable" /* generated register file */
static void frontier_jalr_lift(u32 arg0, u32 arg1) {
    static const char FN[] = "func_801E1000";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x20u, &bl_saved);
    r_a0 = arg0;
    r_a1 = arg1;
    r_sp = r_sp + 0xFFFFFFE0u; /* 801E1000 addiu $sp, $sp, -0x20 */
    bl_sw(r_sp + 0x1Cu, r_ra); /* 801E1004 sw $ra, 0x1C($sp) */
    r_t9 = r_a1 + 0u; /* 801E1008 addu $t9, $a1, $zero */
    ; /* 801E100C nop  */
    /* 801E1010 jalr $t9 */
    { u32 t_ = r_t9; ; bl_jal(FN, t_, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); } /* indirect call */
    r_ra = bl_lw(r_sp + 0x1Cu); /* 801E1018 lw $ra, 0x1C($sp) */
    r_sp = r_sp + 0x20u; /* 801E101C addiu $sp, $sp, 0x20 */
    /* 801E1020 jr $ra */
    ; goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    (void)r_v0;
}

#pragma GCC diagnostic pop

static void frontier_jalr_cases(void)
{
    size_t f, a;
    void *saved_host;
    /* Observe the linked address as an integer. At O2 Clang may otherwise
     * fold low bits using its assumed function alignment, while this real
     * GCC-compiled port function has an odd entry offset. */
    volatile uintptr_t observed_callback = (uintptr_t)prover_wl_TimerWorkListDeleteTask;
    uintptr_t callback = observed_callback;
    PcPortMipsCpu *saved_bridge = rt.bridge_cpu;
    unsigned form, native_forms = 0, guest_forms = 0, refused = 0;
    int rc;
    const uint32_t bad[] = {0x70000001u, 0x8001CE45u, 0x1F800001u, 0x00000001u};

    if (callback > UINT32_MAX || (callback & 3u) == 0) {
        fail("registered-jalr", "fixture", "real native callback is not unaligned below4GiB");
        return;
    }
    for (f = 0; f < rt.function_count; f++)
        if (rt.functions[f].address == 0x8001CE44u) break;
    if (f == rt.function_count) {
        fail("registered-jalr", "fixture", "timer-delete retail identity unavailable");
        return;
    }
    saved_host = rt.functions[f].host;
    rt.functions[f].host = (void *)callback;
    memset(rt.call_cache, 0, sizeof(rt.call_cache));
    for (form = 0; form < 2; form++) {
        begin_case(1, 0);
        frontier_jalr_target = form == 0 ? (uint32_t)callback : 0x8001CE44u;
        /* Snapshot BEFORE setup, as compare() does. HeapInit need not erase
         * unused heap metadata, so replaying setup on an allocated image is
         * not equivalent to starting both sides from the same RAM. */
        memcpy(before, g_PsxRam, sizeof(before));
        frontier_jalr_setup();
        if (frontier_tasks_task == NULL) {
            fail("registered-jalr", "fixture", "timer allocation failed");
            continue;
        }
        initialize_cpu(&cpu, &rt);
        cpu.gpr[4] = (uint32_t)(uintptr_t)frontier_tasks_task;
        cpu.gpr[5] = frontier_jalr_target;
        rt.force_interpret = 1;
        rc = PcPortMipsRun(&cpu, FRONTIER_JALR_PROGRAM, BATTLE_HALT_PC, 100);
        rt.force_interpret = 0;
        if (rc != PC_PORT_MIPS_HALTED || prover_wl_g_NumTimerWorkListEntries != 0 ||
            prover_wl_g_TimerWorkList != NULL) {
            fail("registered-jalr", "oracle", "interpreted jalr did not delete task");
            continue;
        }
        memcpy(expected, g_PsxRam, sizeof(expected));
        frontier_tasks_record(0);
        memcpy(g_PsxRam, before, sizeof(before));
        frontier_jalr_setup();
        initialize_cpu(&cpu, &rt);
        rt.bridge_cpu = &cpu;
        {
            PcPortMipsExecutionStats start = PcPortMipsExecutionStatsGet();
            setenv("XENO_DISABLE_CPU_INTERPRETER", "1", 1);
            frontier_jalr_lift((uint32_t)(uintptr_t)frontier_tasks_task, frontier_jalr_target);
            unsetenv("XENO_DISABLE_CPU_INTERPRETER");
            PcPortMipsExecutionStats end = PcPortMipsExecutionStatsGet();
            if (end.interpreted_instructions != start.interpreted_instructions) {
                fail("registered-jalr", "native-cpu", "lifted callback interpreted instructions");
                continue;
            }
        }
        frontier_tasks_record(1);
        if (prover_wl_g_NumTimerWorkListEntries != 0 || prover_wl_g_TimerWorkList != NULL ||
            memcmp(&frontier_tasks_after[0], &frontier_tasks_after[1],
                   sizeof(frontier_tasks_after[0])) != 0) {
            fail("registered-jalr", "native", "lifted jal did not match list deletion");
            continue;
        }
        for (a = 0; a < PSX_RAM_SIZE; a++)
            if (!(a >= STACK_SKIP_LO && a < STACK_SKIP_HI) && expected[a] != g_PsxRam[a])
                break;
        if (a != PSX_RAM_SIZE) {
            char detail[128];
            snprintf(detail, sizeof(detail), "first difference guest=%08x oracle=%02x native=%02x",
                     (unsigned)(0x80000000u | a), expected[a], g_PsxRam[a]);
            fail("registered-jalr", "ram", detail);
            continue;
        }
        if (form == 0) native_forms++; else guest_forms++;
    }
    initialize_cpu(&cpu, &rt);
    rt.bridge_cpu = &cpu;
    for (form = 0; form < sizeof(bad) / sizeof(bad[0]); form++) {
        fprintf(stderr, "[registered-jalr] expecting refusal target=%08x\n", bad[form]);
        rc = PcPort_BattleMipsCallJal(bad[form], NULL, 0, NULL, NULL);
        if (rc == -1) refused++;
        else fail("registered-jalr", "refusal", "unregistered/guest-unaligned target accepted");
    }
    rt.functions[f].host = saved_host;
    memset(rt.call_cache, 0, sizeof(rt.call_cache));
    rt.bridge_cpu = saved_bridge;
    rt.force_interpret = 0;
    if (native_forms != 1 || guest_forms != 1 || refused != 4)
        fail("registered-jalr", "fixture", "missing required callback/refusal outcome");
    fprintf(stderr, "[registered-jalr] native=%u guest=%u refused=%u callback=%08x\n",
            native_forms, guest_forms, refused, (unsigned)callback);
}

static void frontier_tasks_hand_cases(void)
{
    const uint32_t saved_rng = rng_state;
    uint8_t saved_shift[4], saved_ot[8];
#ifdef PROVER_REAL_GTE
    uint8_t saved_gte[PROVER_GTE_BYTES];
#endif
    uint8_t *shift_host, *ot_host;

    if (abi_leaf_listed("func_800BAB0C")) {
        shift_host = frontier_tasks_binding("D_80050100", 4);
        /* Bound by its retail width (4); the host variable is a u_long *,
         * which is saved, set and restored whole. */
        ot_host = frontier_tasks_binding("g_GfxCurOT", 4);
        if (shift_host != NULL && ot_host != NULL) {
            memcpy(saved_shift, shift_host, 4);
            memcpy(saved_ot, ot_host, 8);
#ifdef PROVER_REAL_GTE
            memcpy(saved_gte, gteRegs, PROVER_GTE_BYTES);
#endif
            frontier_bab0c_cases(shift_host, ot_host);
            memcpy(shift_host, saved_shift, 4);
            memcpy(ot_host, saved_ot, 8);
#ifdef PROVER_REAL_GTE
            memcpy(gteRegs, saved_gte, PROVER_GTE_BYTES);
#endif
        }
    }
    frontier_babdc_cases();
    frontier_bb760_cases();
    frontier_archive_cases();
    frontier_jalr_cases();
    prover_wl_WorkListsReset();
    prover_wl_D_80059190 = 0;
    rng_state = saved_rng;
}
/* ---- END h-frontier (800BA984's task callbacks) ---------------------------- */

/* ---- BEGIN h-frontier: module 0x0C/4's timer callback 801E6E48 (step 9) ---
 * 801E6F00 gives its task the timer callback 801E6E48 (TimerWorkListSet-
 * TaskCallback at 801E6FCC).  Unless ArchiveDataSync reports the archive
 * busy, 801E6E48 publishes the task's +0x28 buffer in D_801E96B4, calls the
 * module's 801E6DC8 -- func_8002DDE4(D_801E96B4, 0, 0, 0, 0, 0, 0) and
 * DrawSync(0) on a stack of its own (a 0x2000-byte heap block, the caller's
 * $sp saved at +0x1F00), then the block freed -- then func_80022224(D_8006BE10,
 * +0x24, 0x380, 0x01D10000, 0) and SoundAddSedsEntry(+0x2C), stores +0x2C in
 * D_8005919C, sets the task's next timer callback (801E6D6C) and calls
 * 800B14B8.  The module host runs both bodies natively, each identity-
 * checked.  These cases run 801E6E48 the way the game does: a timer task
 * from the real TimerWorkListAllocateTask, its callback set by the real
 * TimerWorkListSetTaskCallback, dispatched by the real TimerWorkListUpdate
 * (frontier_tasks_run: retail interpreted, host with the interpreter
 * disabled).  The task's +0x24 / +0x28 / +0x2C are heap blocks, as 801E6F00
 * leaves them.  801E6DC8 has no cases of its own: 801E6E48 is its only
 * caller.
 * Frame-equivalence rule (one word only): 801E6DC8 saves its caller's $sp
 * at block +0x1F00.  The interpreted retail body stores its guest stack
 * frame there, the lift its own frame in the host's lift frame area.  The
 * host word stands for the retail word only if it is exactly the frame
 * bl_enter gives 801E6DC8's lift when 801E6E48's lift calls it from the
 * dispatch's cursor, and the retail word is a guest stack-window address;
 * the host word is then rewritten to the retail word before the RAM diff,
 * so every other byte must match exactly.  The block is located
 * beforehand: each side's setup allocates and frees a 0x2000-byte block,
 * which the callback's HeapAlloc then gets again (a "fixture" FAIL when the
 * rule did not apply in a case that reached 801E6DC8).  The number of
 * mapped cases is printed as [module-frame-word]. */
static uint8_t frontier_6e48_fill[0x60u];

/* The frame 801E6DC8's lift saves: 801E6E48's lift takes bl_enter(.., 0x38)
 * below the dispatch's cursor, 801E6DC8's bl_enter(.., 0x28) below that, and
 * its prologue's addiu $sp, $sp, -0x28. */
static uint32_t frontier_6e48_frame(uint32_t cursor)
{
    uint32_t sp = cursor != 0 ? cursor : (uint32_t)((uintptr_t)bl_stack + BL_STACK_SIZE);

    sp = ((sp - 0x60u) & ~7u) - 0x38u;
    sp = (sp - 0x60u) & ~7u;
    return sp - 0x28u;
}

/* pre_side: the same list state, task and buffers before each side. */
static void frontier_6e48_side(void)
{
    uint8_t *task, *block;
    uint32_t word;
    unsigned k;

    abb8_heap();
    frontier_tasks_reset();
    task = prover_wl_TimerWorkListAllocateTask(NULL, 0x60);
    frontier_tasks_task = task;
    if (task == NULL) { /* a fresh 128 KiB heap; never expected */
        fprintf(stderr, "BATTLE OVERLAY DIFFERENTIAL FAIL h-frontier fixture: "
                "TimerWorkListAllocateTask returned no task\n");
        failures++;
        return;
    }
    prover_wl_TimerWorkListSetTaskCallback(task,
        (FrontierTaskCallback)(uintptr_t)0x801E6E48u);
    memcpy(task + 0x1C, frontier_6e48_fill, sizeof(frontier_6e48_fill));
    for (k = 0; k < 3; k++) { /* +0x24 / +0x28 / +0x2C */
        uint8_t *buffer = HeapAlloc(0x40u + 0x20u * k, 0);

        if (buffer == NULL)
            break;
        memset(buffer, 0x31 + (int)k, 0x40u + 0x20u * k);
        word = (uint32_t)(uintptr_t)buffer;
        memcpy(task + 0x24 + 4u * k, &word, 4);
    }
    /* Where 801E6DC8's block will be: the same allocation, made and freed. */
    block = HeapAlloc(0x2000u, 1);
    if (block != NULL)
        HeapFree(block);
    frontier_tasks_frame_word = block != NULL ? block + 0x1F00u : NULL;
    frontier_tasks_frame_expect = frontier_6e48_frame;
}

static void frontier_module_timer_cases(void)
{
    const uint32_t saved_rng = rng_state;
    unsigned c, i, seen = 0, mapped;

    rng_state = 0x12345678u ^ 0x801E6E48u;
    for (c = 0; c < 6; c++) {
        frontier_module_begin();
        for (i = 0; i < sizeof(frontier_6e48_fill); i++)
            frontier_6e48_fill[i] = (c & 1u) == 0 ? 0xA5u : (uint8_t)(rnd() | 1u);
        *(uint8_t *)PSX_ADDR(0x800C3D6Cu) = (uint8_t)(2u + c); /* 800B14B8 sets 1 */
        frontier_archive_busy = c < 2 ? 1u : 0u;
        frontier_tasks_pump = prover_wl_TimerWorkListUpdate;
        frontier_tasks_callback = 0x801E6E48u;
        mapped = frontier_tasks_frame_mapped;
        frontier_tasks_compare("func_801E6E48", frontier_6e48_side);
        if (!last_case_proven) {
            fail("func_801E6E48", "fixture", "timer dispatch case did not complete");
            continue;
        }
        if (frontier_archive_busy != 0)
            seen |= 1u;
        else if (frontier_tasks_frame_mapped != mapped)
            seen |= 2u;
        else
            fail("func_801E6E48", "fixture",
                 "the frame-equivalence rule did not apply to 801E6DC8's saved $sp");
    }
    frontier_archive_busy = 0;
    require_outcomes("func_801E6E48", seen, 3u);
    fprintf(stderr, "[module-frame-word] mapped=%u (801E6DC8's saved $sp at "
            "block +0x1F00)\n", frontier_tasks_frame_mapped);
    prover_wl_WorkListsReset();
    rng_state = saved_rng;
}
/* ---- END h-frontier (module 0x0C/4's timer callback) ---------------------- */

/* ---- BEGIN h-frontier: module 0x0C/4's timer callback 801E6D6C (step 10b) --
 * 801E6E48 sets 801E6D6C as its task's next timer callback.  Unless the low
 * half of func_8003BDFC(0) is nonzero (a sound transfer still queued: it tests
 * sll $v0, 16), it frees the task's +0x28 buffer, runs the module's 801E6A4C
 * -- 800BB760(slot) for each of the three D_800C3EB0 slots (0x1C apart) whose
 * kind (+6) is below 17 and whose flag (+8) is set; 800BB760 is native since
 * step 10a -- and sets 801E6D34 as the next callback.  The module host runs
 * 801E6D6C and 801E6A4C natively, each identity-checked.  Dispatched like
 * 801E6E48: a timer task from the real TimerWorkListAllocateTask with this
 * callback set by the real TimerWorkListSetTaskCallback, run by the real
 * TimerWorkListUpdate (retail interpreted, host with the interpreter
 * disabled).  func_8003BDFC returns the case's value (a spy here).  Classes:
 * 1 busy, 2 ready with no slot taken (kind 17, or flag clear), 4 ready with
 * a slot taken -- the last two observed: the retail side's timer list holds
 * the case's task plus one per slot taken. */
static const struct {
    uint32_t sound;              /* func_8003BDFC's return */
    uint8_t kind[3], flag[3];    /* the slots' +6 / +8 */
    uint8_t heap_flag;           /* D_800591AC before */
} kFrontier6d6c[] = {
    {0x00000001u, {0, 1, 2}, {1, 1, 1}, 0},          /* busy */
    {0xFFFF8000u, {0, 1, 2}, {1, 1, 1}, 1},          /* busy: low half 0x8000 */
    {0x00000000u, {17, 0x20, 0xFF}, {1, 1, 1}, 0},   /* ready: kinds skip */
    {0x00010000u, {16, 0, 3}, {0, 0, 0}, 1},         /* ready (low half 0): flags skip */
    {0x00000000u, {17, 16, 2}, {1, 1, 0}, 0},        /* ready: slot 1 taken */
    {0x00000000u, {0, 5, 16}, {1, 2, 0xFF}, 1},      /* ready: all three taken */
    {0x7FFF0000u, {1, 17, 9}, {1, 1, 4}, 0},         /* ready: slots 0 and 2 */
    {0x00000010u, {0, 0, 0}, {1, 1, 1}, 1},          /* busy */
};
#define FRONTIER_6D6C_CASES (sizeof(kFrontier6d6c) / sizeof(kFrontier6d6c[0]))
static unsigned frontier_6d6c_case;
static uint8_t frontier_6d6c_fill[0x60u];

/* pre_side: the same lists, heap, task and heap-flag bytes before each side. */
static void frontier_6d6c_side(void)
{
    uint8_t *task, *buffer;
    uint32_t word;

    abb8_heap();
    frontier_tasks_reset();
    task = prover_wl_TimerWorkListAllocateTask(NULL, 0x60);
    frontier_tasks_task = task;
    if (task == NULL) { /* a fresh 128 KiB heap; never expected */
        fprintf(stderr, "BATTLE OVERLAY DIFFERENTIAL FAIL h-frontier fixture: "
                "TimerWorkListAllocateTask returned no task\n");
        failures++;
        return;
    }
    prover_wl_TimerWorkListSetTaskCallback(task,
        (FrontierTaskCallback)(uintptr_t)0x801E6D6Cu);
    memcpy(task + 0x1C, frontier_6d6c_fill, sizeof(frontier_6d6c_fill));
    buffer = HeapAlloc(0x80u, 0); /* +0x28: the buffer 801E6D6C frees */
    if (buffer != NULL)
        memset(buffer, 0x47, 0x80u);
    word = (uint32_t)(uintptr_t)buffer;
    memcpy(task + 0x28, &word, 4);
    *(uint8_t *)battle_guest_resolve_native(0x800591ACu) =
        kFrontier6d6c[frontier_6d6c_case].heap_flag;
    *(uint8_t *)battle_guest_resolve_native(0x800591AFu) = 0;
}

static void frontier_module_6d6c_cases(void)
{
    const uint32_t saved_rng = rng_state;
    uint8_t saved_flag, saved_mode;
    unsigned c, i, k, seen = 0;

    saved_flag = *(uint8_t *)battle_guest_resolve_native(0x800591ACu);
    saved_mode = *(uint8_t *)battle_guest_resolve_native(0x800591AFu);
    rng_state = 0x12345678u ^ 0x801E6D6Cu;
    for (c = 0; c < FRONTIER_6D6C_CASES; c++) {
        frontier_module_begin();
        for (i = 0; i < sizeof(frontier_6d6c_fill); i++)
            frontier_6d6c_fill[i] = (c & 1u) == 0 ? 0xA5u : (uint8_t)(rnd() | 1u);
        for (k = 0; k < 3; k++) {
            *(uint8_t *)PSX_ADDR(0x800C3EB0u + 0x1Cu * k + 6u) = kFrontier6d6c[c].kind[k];
            *(uint8_t *)PSX_ADDR(0x800C3EB0u + 0x1Cu * k + 8u) = kFrontier6d6c[c].flag[k];
        }
        *(uint32_t *)PSX_ADDR(0x800C35D8u) = c * 3u; /* 800BB760 counts on */
        frontier_8003bdfc_ret = kFrontier6d6c[c].sound;
        frontier_6d6c_case = c;
        frontier_tasks_pump = prover_wl_TimerWorkListUpdate;
        frontier_tasks_callback = 0x801E6D6Cu;
        frontier_tasks_compare("func_801E6D6C", frontier_6d6c_side);
        if (!last_case_proven) {
            fail("func_801E6D6C", "fixture", "timer dispatch case did not complete");
            continue;
        }
        if ((kFrontier6d6c[c].sound & 0xFFFFu) != 0) {
            seen |= 1u;
        } else {
            /* Observed, not configured: each taken slot queued one timer
             * task (800BB760) next to the case's own. */
            unsigned taken = 0;

            for (k = 0; k < 3; k++)
                taken += kFrontier6d6c[c].kind[k] < 17 && kFrontier6d6c[c].flag[k] != 0;
            if (frontier_tasks_after[0].timer_count != (int32_t)(1u + taken))
                fail("func_801E6D6C", "fixture", "the slot tasks queued differ from the slots taken");
            else
                seen |= taken != 0 ? 4u : 2u;
        }
    }
    frontier_8003bdfc_ret = 0;
    require_outcomes("func_801E6D6C", seen, 7u);
    *(uint8_t *)battle_guest_resolve_native(0x800591ACu) = saved_flag;
    *(uint8_t *)battle_guest_resolve_native(0x800591AFu) = saved_mode;
    prover_wl_WorkListsReset();
    rng_state = saved_rng;
}
/* ---- END h-frontier (module 0x0C/4's timer callback 801E6D6C) ------------- */

/* ---- BEGIN h-frontier: module 0x0C/4's timer callback 801E6D34 (step 10c) --
 * 801E6D6C sets 801E6D34 as the module task's next timer callback.  While
 * D_800C35D8 (the slot tasks 800BB760 counted) is nonzero it waits; once it
 * is 0 it stores 0x10 at the task's +0x90 and sets 801E6C80 as the next
 * callback.  The module host runs it natively, identity-checked.
 * Dispatched like 801E6E48 (real TimerWorkListAllocateTask /
 * TimerWorkListSetTaskCallback / TimerWorkListUpdate; retail interpreted,
 * host with the interpreter disabled).  Classes, observed on the retail
 * side: 1 waiting (no callback set), 2 done (801E6C80 set). */
static const uint32_t kFrontier6d34Pending[] = {0, 1, 0, 3, 0x7FFFFFFFu, 0};
#define FRONTIER_6D34_CASES (sizeof(kFrontier6d34Pending) / sizeof(kFrontier6d34Pending[0]))
static uint8_t frontier_6d34_fill[0x80u];

/* pre_side: the same lists, heap and task before each side. */
static void frontier_6d34_side(void)
{
    uint8_t *task;

    abb8_heap();
    frontier_tasks_reset();
    task = prover_wl_TimerWorkListAllocateTask(NULL, 0x80);
    frontier_tasks_task = task;
    if (task == NULL) { /* a fresh 128 KiB heap; never expected */
        fprintf(stderr, "BATTLE OVERLAY DIFFERENTIAL FAIL h-frontier fixture: "
                "TimerWorkListAllocateTask returned no task\n");
        failures++;
        return;
    }
    prover_wl_TimerWorkListSetTaskCallback(task,
        (FrontierTaskCallback)(uintptr_t)0x801E6D34u);
    memcpy(task + 0x1C, frontier_6d34_fill, sizeof(frontier_6d34_fill));
}

static void frontier_module_6d34_cases(void)
{
    const uint32_t saved_rng = rng_state;
    unsigned c, i, seen = 0;

    rng_state = 0x12345678u ^ 0x801E6D34u;
    for (c = 0; c < FRONTIER_6D34_CASES; c++) {
        frontier_module_begin();
        for (i = 0; i < sizeof(frontier_6d34_fill); i++)
            frontier_6d34_fill[i] = (c & 1u) == 0 ? 0xA5u : (uint8_t)(rnd() | 1u);
        *(uint32_t *)PSX_ADDR(0x800C35D8u) = kFrontier6d34Pending[c];
        frontier_tasks_pump = prover_wl_TimerWorkListUpdate;
        frontier_tasks_callback = 0x801E6D34u;
        frontier_tasks_compare("func_801E6D34", frontier_6d34_side);
        if (!last_case_proven) {
            fail("func_801E6D34", "fixture", "timer dispatch case did not complete");
            continue;
        }
        /* The retail side's spied calls: TimerWorkListSetTaskCallback only
         * once the count is 0. */
        seen |= spy_expected_count != 0 ? 2u : 1u;
    }
    require_outcomes("func_801E6D34", seen, 3u);
    prover_wl_WorkListsReset();
    rng_state = saved_rng;
}
/* ---- END h-frontier (module 0x0C/4's timer callback 801E6D34) ------------- */

/* ---- BEGIN h-hotpath: batch 1 hand cases ---------------------------------
 * 800AA514 (main70, lifted): the byte min(a2 + (s16)a1 * (s16)a0 / 256, 0xFF).
 * The sweep already drives it with random words; these add the edges: upper
 * garbage the retail sll/sra pair discards, negative products (the bgez +0xFF
 * rounding), the 0x100 saturation boundary and a2 overflow.
 *
 * 800C11CC (main135, lifted): the battle sprite animation-script interpreter.
 * Each case builds a sprite at HP_SPRITE (pattern-filled, then the fields the
 * script reads made live: wait 0, script, speed, frame, kind, target, unk20),
 * a target sprite at HP_TARGET, an SVECTOR block at HP_VEC and the script at
 * HP_SCRIPT, and gives the opcode-length table D_8004FC40 (a main-executable
 * shared global; both sides resolve it to the same copy) the lengths these
 * scripts use.  Every script ends in a frame-step opcode (< 0x80), which
 * returns.  The trivial wait != 0 return is one case only. */
#define HP_SPRITE 0x80180000u
#define HP_TARGET 0x80180100u
#define HP_VEC    0x80180200u
#define HP_SCRIPT 0x80180400u
#define HP_FLAG   0x80180500u

static void hp_w8(uint32_t at, uint8_t v) { *((uint8_t *)PSX_ADDR(at)) = v; }
static void hp_w16(uint32_t at, uint16_t v) { memcpy(PSX_ADDR(at), &v, 2); }
static void hp_w32(uint32_t at, uint32_t v) { memcpy(PSX_ADDR(at), &v, 4); }
static uint32_t hp_r32(uint32_t at) { uint32_t v; memcpy(&v, PSX_ADDR(at), 4); return v; }

static void hp_fill(uint32_t at, unsigned n, unsigned salt)
{
    unsigned i;
    for (i = 0; i < n; i++)
        hp_w8(at + i, (uint8_t)(salt * 37u + i * 11u + 5u));
}

static void hp_sprite(uint32_t at, unsigned salt, unsigned kind, unsigned speed)
{
    uint32_t w;
    hp_fill(at, 0xB4u, salt);
    hp_w32(at + 0x00u, 0x00120000u + salt * 0x10000u);      /* x */
    hp_w32(at + 0x04u, 0xFFF40000u + salt * 0x3000u);       /* y */
    hp_w32(at + 0x08u, 0x00350000u - salt * 0x20000u);      /* z */
    hp_w32(at + 0x20u, HP_VEC);                             /* unk20 */
    hp_w16(at + 0x2Cu, (uint16_t)(0x0800u + salt * 0x100u)); /* unk2C */
    hp_w16(at + 0x34u, (uint16_t)(3u + salt));              /* frame */
    hp_w32(at + 0x64u, HP_SCRIPT);                          /* script */
    hp_w32(at + 0x68u, 0u);                                 /* callback */
    hp_w32(at + 0x74u, HP_TARGET);                          /* target */
    hp_w16(at + 0x84u, (uint16_t)(0x20u + salt));           /* unk84 */
    hp_w16(at + 0x9Eu, 0u);                                 /* wait */
    w = hp_r32(at + 0x40u);
    hp_w32(at + 0x40u, (w & ~(0xFu << 13)) | ((kind & 0xFu) << 13));    /* kind40 */
    w = hp_r32(at + 0xACu);
    hp_w32(at + 0xACu, (w & ~(0xFFFu << 7)) | ((speed & 0xFFFu) << 7)); /* speed */
}

/* The sprite argument of hp_case_c11cc.  Below 2 MB it is a NULL-based
 * sprite (KUSEG): the fields are seeded through their KSEG0 view, and the
 * retail side's KUSEG accesses count as guest accesses for that case. */
static uint32_t hp_c11cc_sprite = HP_SPRITE;

static void hp_case_c11cc(const uint8_t *script, unsigned len, unsigned salt, unsigned kind,
                          unsigned speed, int glob_flag, uint16_t wait)
{
    static const struct { uint8_t op, len; } kLen[] = {
        {0x9D, 1}, {0x99, 1}, {0xC2, 2}, {0x9E, 1}, {0xE3, 3}, {0x9B, 1}, {0x9F, 1},
    };
    uint8_t *lengths = (uint8_t *)battle_guest_resolve_native(0x8004FC40u);
    uint8_t saved[sizeof(kLen) / sizeof(kLen[0])];
    uint16_t *d9454, saved9454;
    uint32_t args[4] = {hp_c11cc_sprite, 0, 0, 0};
    const uint32_t at = hp_c11cc_sprite < 0x00200000u ? hp_c11cc_sprite | 0x80000000u
                                                       : hp_c11cc_sprite;
    unsigned i;

    begin_case(salt % 3u, 0);
    /* The table may be a native copy that outlives begin_case: restore it
     * after the case so later leaves see what they would have seen. */
    for (i = 0; i < sizeof(kLen) / sizeof(kLen[0]); i++) {
        saved[i] = lengths[kLen[i].op];
        lengths[kLen[i].op] = kLen[i].len;
    }
    hp_sprite(at, salt, kind, speed);
    hp_sprite(HP_TARGET, salt + 7u, kind ^ 1u, speed + 0x40u);
    hp_w16(at + 0x9Eu, wait);
    hp_fill(HP_VEC, 0x40u, salt + 3u);
    for (i = 0; i < 16u; i++)
        hp_w8(HP_SCRIPT + i, i < len ? script[i] : 0x01u);
    /* D_800C3EB0 is a struct at its own address (retail lui/addiu, then
     * lhu 0x8C58 off it): opcode 0x9E tests bit 0x100 of that halfword. */
    hp_w16(0x800C3EB0u + 0x8C58u, glob_flag ? 0x0100u : 0x0080u);
    hp_w32(0x800C3624u, HP_FLAG);                           /* D_800C3624 */
    hp_fill(0x800D30B0u, 8u, salt + 9u);                    /* D_800D30B0[0] */
    hp_w16(0x800D30B8u, (uint16_t)(0x0123u + salt * 0x0101u)); /* D_800D30B8[0]: nonzero */
    d9454 = (uint16_t *)battle_guest_resolve_native(0x80059454u); /* D_80059454 (main exe) */
    saved9454 = *d9454;
    *d9454 = (uint16_t)(0x0040u + salt * 3u);
    retail_kuseg_guest = at != hp_c11cc_sprite;
    compare_name("func_800C11CC", args);
    retail_kuseg_guest = 0;
    *d9454 = saved9454;
    for (i = 0; i < sizeof(kLen) / sizeof(kLen[0]); i++)
        lengths[kLen[i].op] = saved[i];
}

static void hotpath_hand_cases_aa514(void)
{
    static const uint32_t kAA514[][3] = {
        {0x00007FFFu, 0x00007FFFu, 0x00000000u}, {0xFFFF8000u, 0x00007FFFu, 0x00000080u},
        {0x12340010u, 0xABCD0020u, 0x00000005u}, {0x00000000u, 0x00000000u, 0x00000100u},
        {0x000000FFu, 0x00000100u, 0xFFFFFF00u}, {0x00000040u, 0x00000040u, 0x7FFFFFFFu},
        {0xDEAD8001u, 0x0000FFFFu, 0x000000FEu}, {0x00000010u, 0xFFFFFFF0u, 0x00000000u},
    };
    unsigned i;

    if (!abi_leaf_listed("func_800AA514"))
        return;
    for (i = 0; i < sizeof(kAA514) / sizeof(kAA514[0]); i++) {
        uint32_t args[4] = {kAA514[i][0], kAA514[i][1], kAA514[i][2], 0};
        begin_case(i % 3u, 0);
        compare_name("func_800AA514", args);
    }
}

static void hotpath_hand_cases_c11cc(void)
{
    /* Opcodes 0x00-0x0F and 0x20-0x2F step the frame through func_8001D2B0,
     * and the default handler is func_8001FBE4: this harness links neither
     * (bridge placeholders), so those paths are not provable here.  Every
     * script below therefore ends in a delay-only opcode (0x30-0x3F, which
     * calls nothing) and steps frames with 0x10-0x1F (func_80022D44, linked
     * from temp1.c). */
    static const uint8_t s_step[][1] = {{0x13}, {0x1A}, {0x35}, {0x3F}, {0x45}, {0x7F}};
    static const uint8_t s_9d[] = {0x9D, 0x32};
    static const uint8_t s_99[] = {0x99, 0x36};
    static const uint8_t s_c2[] = {0xC2, 0x10, 0x37};
    static const uint8_t s_c2n[] = {0xC2, 0xF0, 0x38};
    static const uint8_t s_e1[] = {0xE1, 0x04, 0x00, 0xFF, 0x3C};
    static const uint8_t s_d4[] = {0xD4, 0x04, 0x00, 0xFF, 0x33};
    static const uint8_t s_9e[] = {0x9E, 0x3B};
    static const uint8_t s_8f[] = {0x8F};
    static const uint8_t s_e3[] = {0xE3, 0x02, 0x00, 0x3D};
    static const uint8_t s_9b[] = {0x9B, 0x34};
    static const uint8_t s_9f[] = {0x9F, 0x9D, 0x31};
    static const unsigned kSpeeds[] = {0x100u, 0x180u, 0x040u, 0xFFFu};
    unsigned i, salt;

    if (!abi_leaf_listed("func_800C11CC"))
        return;
    for (salt = 0; salt < 4u; salt++) {
        for (i = 0; i < sizeof(s_step) / sizeof(s_step[0]); i++)
            hp_case_c11cc(s_step[i], 1u, salt, 0xAu, kSpeeds[salt], 1, 0);
        hp_case_c11cc(s_9d, sizeof s_9d, salt, salt & 1u ? 0xAu : 0x3u, kSpeeds[salt], 1, 0);
        hp_case_c11cc(s_99, sizeof s_99, salt, 0xBu, kSpeeds[salt], 1, 0);
        hp_case_c11cc(s_c2, sizeof s_c2, salt, 0xAu, kSpeeds[salt], 1, 0);
        hp_case_c11cc(s_c2n, sizeof s_c2n, salt, 0xAu, kSpeeds[salt], 1, 0);
        hp_case_c11cc(s_e1, sizeof s_e1, salt, 0xAu, kSpeeds[salt], 1, 0);
        hp_case_c11cc(s_d4, sizeof s_d4, salt, 0xAu, kSpeeds[salt], 1, 0);
        hp_case_c11cc(s_9e, sizeof s_9e, salt, 0xAu, kSpeeds[salt], (int)(salt & 1u), 0);
        hp_case_c11cc(s_8f, sizeof s_8f, salt, 0xAu, kSpeeds[salt], 1, 0);
        hp_case_c11cc(s_e3, sizeof s_e3, salt, 0xAu, kSpeeds[salt], 1, 0);
        hp_case_c11cc(s_9b, sizeof s_9b, salt, 0xAu, kSpeeds[salt], 1, 0);
        hp_case_c11cc(s_9f, sizeof s_9f, salt, salt & 1u ? 0xAu : 0x5u, kSpeeds[salt], 1, 0);
    }
    hp_case_c11cc(s_step[2], 1u, 0, 0xAu, 0x100u, 1, 5); /* wait != 0: returns at once */
}

/* include/battle_lift.h's KUSEG alias: 800C11CC with a NULL sprite.  Its
 * fields then sit at KUSEG 0x00..0xB3, which retail's interpreter bus maps
 * to main RAM 0x80000000 as the PS1 does.  A wait step (0x30-0x3F, no
 * callee) loads wait, script and speed through the NULL sprite and stores
 * the advanced script pointer (+0x64) and the new wait (+0x9E) back through
 * it.  Without the alias the lift faults on its first load (host-fault).
 * Proven is required, plus both NULL-based stores on the host side. */
static void hotpath_hand_cases_c11cc_null(void)
{
    static const uint8_t s_wait[] = {0x35u};
    static const unsigned kSpeeds[] = {0x100u, 0x180u};
    unsigned i, ok = 0;

    if (!abi_leaf_listed("func_800C11CC"))
        return;
    hp_c11cc_sprite = 0u;
    for (i = 0; i < 2u; i++) {
        hp_case_c11cc(s_wait, sizeof s_wait, i, 0xAu, kSpeeds[i], 1, 0);
        if (last_case_proven && hp_r32(0x80000064u) == HP_SCRIPT + 1u &&
            *(uint16_t *)battle_guest_resolve_native(0x8000009Eu) != 0)
            ok++;
    }
    hp_c11cc_sprite = HP_SPRITE;
    if (ok != 2u)
        fail("func_800C11CC", "kuseg-null",
             "NULL-sprite case not proven with both NULL-based stores");
    fprintf(stderr, "[prover] h-hotpath func_800C11CC NULL sprite (KUSEG) proven=%u of 2\n", ok);
}
/* func_80021B14 (src/slus_006.64/system/animation_scripts.c, which this
 * runner does not link): the 4-instruction SLUS setVector, sw a1/a2/a3 to
 * a0+0/4/8.  800BB844 hands it a stack VECTOR (the guest stack on the retail
 * side, the lift's frame area on the host side; prover_spy maps both to the
 * stack token), so the spy does the three stores and logs all four words.
 * No other adopted leaf reaches it. */
void func_80021B14(void *v, int x, int y, int z);
void func_80021B14(void *v, int x, int y, int z)
{
    unsigned long long a[4];

    a[0] = (unsigned long long)(uintptr_t)v;
    a[1] = (unsigned)x;
    a[2] = (unsigned)y;
    a[3] = (unsigned)z;
    ((int32_t *)v)[0] = x;
    ((int32_t *)v)[1] = y;
    ((int32_t *)v)[2] = z;
    (void)prover_spy("func_80021B14", 4, a, 0u);
}

/* 800BB844 (mainc114, lifted): the battle view matrix from eye, target and
 * up SVECTORs into a MATRIX.  All four arguments are guest addresses. */
#define HP2_MAT 0x80181000u
#define HP2_EYE 0x80181040u
#define HP2_TGT 0x80181048u
#define HP2_UP  0x80181050u
static void hp_svec(uint32_t at, int16_t x, int16_t y, int16_t z)
{
    hp_w16(at, (uint16_t)x); hp_w16(at + 2u, (uint16_t)y); hp_w16(at + 4u, (uint16_t)z);
    hp_w16(at + 6u, 0x5A5Au);
}

static void hotpath_hand_cases_bb844(void)
{
    static const int16_t kEye[][3] = {
        {0, -800, -2400}, {1200, -600, 900}, {-3000, -200, 50}, {16, 16, 16},
        {0x7000, -0x7000, 0x1234}, {-1, 0, 1},
    };
    static const int16_t kTgt[][3] = {
        {0, -400, 0}, {-200, -350, 100}, {400, 0, -2000}, {-16, 16, -16},
        {0, 0, 0}, {1, 0, -1},
    };
    static const int16_t kUp[][3] = {{0, -4096, 0}, {0, 4096, 0}, {512, -4000, 300}};
    uint32_t args[4] = {HP2_MAT, HP2_EYE, HP2_TGT, HP2_UP};
    unsigned i, u;

    if (!abi_leaf_listed("func_800BB844"))
        return;
    for (i = 0; i < sizeof(kEye) / sizeof(kEye[0]); i++) {
        for (u = 0; u < sizeof(kUp) / sizeof(kUp[0]); u++) {
            begin_case((i + u) % 3u, 0);
            hp_fill(HP2_MAT, 0x20u, i * 3u + u);
            hp_svec(HP2_EYE, kEye[i][0], kEye[i][1], kEye[i][2]);
            hp_svec(HP2_TGT, kTgt[i][0], kTgt[i][1], kTgt[i][2]);
            hp_svec(HP2_UP, kUp[u][0], kUp[u][1], kUp[u][2]);
            compare_name("func_800BB844", args);
        }
    }
}

/* 800BD810 (mainc118 retail, lifted into mainc114): one POLY_FT4 for a glyph
 * record, projected with RotTransPers4 and linked at g_GfxCurOT[0].  The
 * work-buffer, end and OT words are main-executable shared globals: native
 * copies that compare() neither snapshots nor restores, and the body advances
 * the work-buffer word.  So pre_side (run before each side, after the RAM
 * restore) re-seeds all three with guest addresses, and the old values are
 * given back after the case.  compare()'s native-watch requires both sides to
 * leave every native binding alike (a store of the wrong advance fails it as
 * "native"); it restores its start snapshot -- taken before pre_side -- in
 * between, so the re-seed must run before each side.  The case then counts,
 * from the proven host finals, that both work-buffer outcomes were checked. */
#define HP2_GLYPH 0x80181100u
#define HP2_WB    0x80182000u
#define HP2_OT    0x80183000u
static uint32_t hp2_wb, hp2_wbe, hp2_ot;
static void hp_bd810_side(void)
{
    *(uint32_t *)battle_guest_resolve_native(0x80059580u) = hp2_wb;   /* g_GfxCurWorkBuffer */
    *(uint32_t *)battle_guest_resolve_native(0x80059534u) = hp2_wbe;  /* g_GfxCurWorkBufferEnd */
    *(uint32_t *)battle_guest_resolve_native(0x8005956Cu) = hp2_ot;   /* g_GfxCurOT */
}

static void hotpath_hand_cases_bd810(void)
{
    static const uint32_t kRgb[] = {0x2C808080u, 0x2D10C0F0u, 0x2CFFFFFFu, 0x2C000000u};
    uint32_t args[4] = {HP2_GLYPH, 0, 0, 0};
    uint32_t *wb = (uint32_t *)battle_guest_resolve_native(0x80059580u);
    uint32_t *wbe = (uint32_t *)battle_guest_resolve_native(0x80059534u);
    uint32_t *ot = (uint32_t *)battle_guest_resolve_native(0x8005956Cu);
    uint32_t saved_wb, saved_wbe, saved_ot;
    unsigned i, checked = 0, advanced = 0, full = 0;
    char detail[200];

    if (!abi_leaf_listed("func_800BD810"))
        return;
    saved_wb = *wb; saved_wbe = *wbe; saved_ot = *ot;
    for (i = 0; i < 9u; i++) {
        begin_case(i % 3u, 0);
        hp2_wb = HP2_WB + i * 0x28u;
        hp2_wbe = i == 8u ? hp2_wb + 0x20u : HP2_WB + 0x400u; /* case 8: buffer full */
        hp2_ot = HP2_OT + i * 4u;
        hp_fill(HP2_WB, 0x400u, i + 1u);
        hp_w32(HP2_OT + i * 4u, i & 1u ? 0x00FFFFFFu : 0x04182340u);
        hp_w16(HP2_GLYPH + 0x0u, (uint16_t)(10u + i * 37u));     /* x */
        hp_w16(HP2_GLYPH + 0x2u, (uint16_t)(20u + i * 29u));     /* y */
        hp_w8(HP2_GLYPH + 0x4u, (uint8_t)(i * 16u));              /* u */
        hp_w8(HP2_GLYPH + 0x5u, (uint8_t)(i * 8u + 3u));          /* v */
        hp_w8(HP2_GLYPH + 0x6u, (uint8_t)(8u + i));               /* w */
        hp_w8(HP2_GLYPH + 0x7u, (uint8_t)(12u + i * 2u));         /* h */
        hp_w16(HP2_GLYPH + 0xAu, (uint16_t)(0x0010u + i));        /* tpage */
        hp_w16(HP2_GLYPH + 0xCu, (uint16_t)(0x7FC0u + i));        /* clut */
        args[1] = kRgb[i % 4u];
        pre_side = hp_bd810_side;
        compare_name("func_800BD810", args);
        pre_side = NULL;
        if (!last_case_proven)
            continue; /* failed (already counted) or inconclusive */
        /* Proven includes the native-watch: the host's finals are retail's. */
        checked++;
        if (*wb == hp2_wb + 0x28u)
            advanced++;
        else if (*wb == hp2_wb)
            full++;
    }
    *wb = saved_wb; *wbe = saved_wbe; *ot = saved_ot;
    /* Non-vacuous: both work-buffer outcomes were checked (advance by one
     * POLY_FT4, and the buffer-full early exit that leaves it alone). */
    if (checked == 0 || advanced == 0 || full == 0) {
        snprintf(detail, sizeof(detail),
                 "native work-buffer check vacuous: checked=%u advanced=%u full=%u",
                 checked, advanced, full);
        fail("func_800BD810", "hand-coverage", detail);
    }
    fprintf(stderr, "[prover] h-hotpath func_800BD810 native globals checked=%u advanced=%u full=%u\n",
            checked, advanced, full);
}

/* 800BC460 calls RotMatrixZYX and ReadGeomScreen, which are the port's compat
 * entries (pc_port/src/psyq_compat_leaf.inc).  The port links them through
 * retail_leaf_adapters.c; this harness does not link that TU, so they were
 * unresolved call targets.  Both sides reach them through the same bridge
 * path (bridge_target_camera_graphics), so the harness binds the compat code
 * itself.  The .inc's other entries are renamed while it is included, so
 * only these two names become bindable and no other leaf's callees change. */
#define SetMulMatrix hp3_unbound_SetMulMatrix
#define SetPolyFT3 hp3_unbound_SetPolyFT3
#define SetPolyG3 hp3_unbound_SetPolyG3
#define SetPolyGT3 hp3_unbound_SetPolyGT3
#define SetLineF2 hp3_unbound_SetLineF2
#define SetLineG2 hp3_unbound_SetLineG2
#include "psyq_compat_leaf.inc"
#undef SetMulMatrix
#undef SetPolyFT3
#undef SetPolyG3
#undef SetPolyGT3
#undef SetLineF2
#undef SetLineG2

/* 800BC460 (mainc115 retail, lifted into mainc114): the target-chain camera.
 * Inputs: the mask (low 11 bits select rows), per row the skip flag (+7)
 * and the head flag (+8) of D_800C3EB0, the Position pointers at
 * D_800C3EB0+0x8C8C (x/y/z 16.16 at +0/+4/+8, height at +0x36),
 * D_800C3688[0] (no heads), the view angles D_800C3740 and the up vector
 * D_800C3730 -- all battle BSS in guest RAM -- and the GTE (H, offset).
 * Outputs: D_800C3678, D_800C3CDC, D_800D30A0 (RAM) and the rotation and
 * translation it leaves in the GTE, which compare() checks.  The GTE
 * register file is saved around these cases so no later case inherits the
 * H/offset they set. */
#define HP3_POS        0x80181800u                 /* Position records, 0x40 apart */
#define HP3_ROWS       0x800C3EB0u                 /* D_800C3EB0 rows, 0x1C apart */
#define HP3_POSITIONS  (0x800C3EB0u + 0x8C8Cu)     /* D_800C3EB0.positions[11] */
typedef struct { int32_t x, y, z; uint16_t height; } Hp3Pos;
/* SquareRoot0 (patched PsyCross, psycross_sqrt_retail_memory.patch) reads the
 * executable's SQRT table from guest RAM at 0x80056A00, as the retail SDK
 * does.  begin_case clears RAM, so each case loads that window from
 * disc/SLUS_006.64 (load address 0x80010000, 0x800-byte header); without it
 * every radius is 0 and the scaled-distance branch never runs. */
#define HP3_SQRT_AT  0x80056900u
static uint8_t hp3_sqrt[0x300];

static void hp3_case(uint32_t mask, const Hp3Pos *pos, unsigned n, uint32_t skip,
                     uint32_t head, uint32_t null_rows, uint8_t no_heads,
                     const int16_t angle[3], uint16_t h, uint32_t ofy, unsigned salt)
{
    uint32_t args[4] = {mask, 0, 0, 0};
    unsigned i;

    begin_case(salt % 3u, 0);
    memcpy(PSX_ADDR(HP3_SQRT_AT), hp3_sqrt, sizeof(hp3_sqrt));
    for (i = 0; i < 11u; i++) {
        const uint32_t row = HP3_ROWS + i * 0x1Cu, at = HP3_POS + i * 0x40u;
        hp_w8(row + 7u, (skip >> i) & 1u ? (uint8_t)(1u + salt) : 0u);
        hp_w8(row + 8u, (head >> i) & 1u ? (uint8_t)(2u + salt) : 0u);
        if (i >= n || ((null_rows >> i) & 1u)) {
            hp_w32(HP3_POSITIONS + i * 4u, 0u);
            continue;
        }
        hp_fill(at, 0x40u, salt + i);
        hp_w32(HP3_POSITIONS + i * 4u, at);
        hp_w32(at + 0x0u, (uint32_t)pos[i].x);
        hp_w32(at + 0x4u, (uint32_t)pos[i].y);
        hp_w32(at + 0x8u, (uint32_t)pos[i].z);
        hp_w16(at + 0x36u, pos[i].height);
    }
    hp_w8(0x800C3688u, no_heads);
    hp_svec(0x800C3740u, angle[0], angle[1], angle[2]);
    hp_svec(0x800C3730u, 0, -4096, 0);
    CTC2((unsigned)h, 26);                 /* H (ReadGeomScreen) */
    CTC2(160u << 16, 24);                  /* OFX */
    CTC2(ofy, 25);                         /* OFY */
    compare_name("func_800BC460", args);
}

static void hotpath_hand_cases_bc460(void)
{
#define P(x, y, z, hh) {(int32_t)((x) * 65536), (int32_t)((y) * 65536), (int32_t)((z) * 65536), (hh)}
    static const Hp3Pos kNear[11] = {
        P(10, -20, 30, 0x300), P(-12, -18, 26, 0x280), P(8, -22, 34, 0x200),
        P(-6, -16, 28, 0x380), P(4, -24, 32, 0x100), P(-2, -20, 30, 0x200),
        P(14, -18, 24, 0x240), P(-14, -22, 36, 0x2C0), P(0, -20, 30, 0x300),
        P(6, -26, 22, 0x180), P(-8, -14, 38, 0x220)};
    static const Hp3Pos kFar[11] = {
        P(1800, -400, 900, 0x600), P(-1700, -300, -1200, 0x500), P(600, -900, 2000, 0x700),
        P(-900, -100, 1500, 0x400), P(1200, -700, -1600, 0x300), P(-300, -500, 100, 0x200),
        P(2000, -200, -300, 0x650), P(-2000, -800, 700, 0x450), P(100, -1000, -2000, 0x500),
        P(700, -50, 1800, 0x380), P(-1500, -650, -900, 0x5A0)};
#undef P
    static const int16_t kAngles[][3] = {
        {0x0000, 0x0000, 0x0000}, {0x0100, 0x0400, 0x0000}, {0xFF00, 0x0C00, 0x0080},
        {0x0200, 0x0800, 0x0010}};
    uint8_t gte_saved[PROVER_GTE_BYTES];
    unsigned salt;
    FILE *f;

    if (!abi_leaf_listed("func_800BC460"))
        return;
    f = fopen("disc/SLUS_006.64", "rb");
    if (f == NULL || fseek(f, (long)(HP3_SQRT_AT - 0x8000F800u), SEEK_SET) != 0 ||
        fread(hp3_sqrt, 1, sizeof(hp3_sqrt), f) != sizeof(hp3_sqrt)) {
        if (f != NULL)
            fclose(f);
        fail("func_800BC460", "hand-setup", "cannot read the SQRT table from disc/SLUS_006.64");
        return;
    }
    fclose(f);
    memcpy(gte_saved, gteRegs, PROVER_GTE_BYTES);
    for (salt = 0; salt < 4u; salt++) {
        const int16_t *a = kAngles[salt];
        const uint16_t h = (uint16_t)(0x100u << (salt & 1u)) + (uint16_t)(salt * 0x40u);
        /* count 0: empty mask; all rows NULL; all rows skipped. */
        hp3_case(0u, kNear, 11u, 0, 0, 0, 0, a, h, 120u << 16, salt);
        hp3_case(0x7FFu, kNear, 0u, 0, 0, 0, 0, a, h, 120u << 16, salt);
        hp3_case(0x7FFu, kNear, 11u, 0x7FFu, 0, 0, 0, a, h, 120u << 16, salt);
        /* near targets centred on the projection origin: radius < 120 */
        hp3_case(0x1u, kNear, 1u, 0, 0, 0, 0, a, h, 164u << 16, salt);
        hp3_case(0x7u << salt, kNear, 11u, 0, 0, 0, 0, a, h, 164u << 16, salt);
        /* far targets: radius >= 120 (scaled distance) */
        hp3_case(0x3u, kFar, 11u, 0, 0, 0, 0, a, h, 120u << 16, salt);
        hp3_case(0x7FFu, kFar, 11u, 0x020u, 0, 0x100u, 0, a, h, 120u << 16, salt);
        /* heads: reprojected when flagged and D_800C3688[0] is clear */
        hp3_case(0x2Du, kFar, 11u, 0, 0x25u, 0, 0, a, h, 120u << 16, salt);
        hp3_case(0x2Du, kFar, 11u, 0, 0x25u, 0, 1, a, h, 120u << 16, salt);
        hp3_case(0x2Du, kNear, 11u, 0, 0x2Du, 0x08u, 0, a, h, 164u << 16, salt);
        /* bits above the 11 rows only reach D_800C3678 */
        hp3_case(0xFFFFF805u, kFar, 11u, 0x4u, 0x1u, 0, 0, a, h, 120u << 16, salt);
    }
    memcpy(gteRegs, gte_saved, PROVER_GTE_BYTES);
}

/* Batch 4: the main2 party-HUD layout 80075168 and bars 80075938.  Both read
 * battle globals outside the seeded SCRATCH window, so each case seeds them:
 * the HUD prim block *(D_800C3EA4) and the menu block *(D_800D2D28) as guest
 * buffers, the double-buffer index D_800CCB34, the x-table index D_800D3280,
 * per slot: the HUD state byte (+0x853D + i*0x1E4), the slot state
 * D_800C3EB6 (+i*0x1C), the menu flag (+0x90 + i), D_800D32A0/1 (+i*8),
 * D_800D2D24, HP cur/max rows D_800CCDC4/6 (+i*0x170), the status rows
 * D_800CCD64/6C/6E, the gauge words D_800D2E06[i] and [i - 11].
 * 80075938 reads w[slot] at sp+0x10+4i before writing it (800759BC) and
 * clamps it at 0 (800759CC); the retail side enters at BATTLE_STACK_TOP, so
 * those slots are 0x801FFEE0+4i (the stack window, not compared).  Each 80075938
 * shape runs twice, with negative and with non-negative garbage there, while
 * the lift's static frame holds its own: both must be proven, so that read
 * is dead in every case. */
#define HP4_HUD   0x80140000u
#define HP4_MENU  0x8014C000u
#define HP4_WSLOT 0x801FFEE0u
typedef struct {
    uint8_t hud_state[3], slot_state[3], in_menu[3], row8[3], d24[3];
    uint16_t cur[3], max[3], status[3];
    int16_t gauge[3], gauge_max[3];
    uint8_t cb;
    unsigned buf, idx;
} Hp4Cfg;

static void hp4_seed(const Hp4Cfg *c, unsigned salt)
{
    unsigned i;

    begin_case(salt % 3u, 0);
    hp_fill(HP4_HUD, 0xA300u, salt);
    hp_fill(HP4_MENU, 0x100u, salt + 1u);
    hp_w32(0x800C3EA4u, HP4_HUD);                  /* D_800C3EA4 */
    hp_w32(0x800D2D28u, HP4_MENU);                 /* D_800D2D28 */
    hp_w32(0x800CCB34u, c->buf);                   /* D_800CCB34[0] */
    hp_w8(0x800D3280u, (uint8_t)c->idx);           /* D_800D3280 */
    hp_w8(HP4_MENU + 0xCBu, c->cb);
    for (i = 0; i < 3u; i++) {
        hp_w8(HP4_HUD + 0x853Du + i * 0x1E4u, c->hud_state[i]);
        hp_w8(0x800C3EB6u + i * 0x1Cu, c->slot_state[i]);
        hp_w8(HP4_MENU + 0x90u + i, c->in_menu[i]);
        hp_w8(0x800D32A0u + i * 8u, (uint8_t)(c->row8[i] * 3u + 5u));
        hp_w8(0x800D32A1u + i * 8u, c->row8[i]);
        hp_w8(0x800D2D24u + i, c->d24[i]);
        hp_w16(0x800CCDC4u + i * 0x170u, c->cur[i]);
        hp_w16(0x800CCDC6u + i * 0x170u, c->max[i]);
        hp_w16(0x800CCD64u + i * 0x170u, c->status[i]);
        hp_w16(0x800CCD6Cu + i * 0x170u, (uint16_t)(c->status[i] << 1));
        hp_w16(0x800CCD6Eu + i * 0x170u, (uint16_t)(c->status[i] >> 1));
        hp_w16(0x800D2E06u + i * 2u, (uint16_t)c->gauge[i]);
        hp_w16(0x800D2DF0u + i * 2u, (uint16_t)c->gauge_max[i]); /* D_800D2E06[i - 11] */
    }
}

static void hotpath_hand_cases_main2_hud(void)
{
    static const Hp4Cfg kCfg[] = {
        /* hud_state  slot_state     in_menu   row8       d24        cur              max              status                gauge         gauge_max      cb buf idx */
        {{1, 2, 0}, {0, 1, 0x7F}, {0, 1, 0}, {0, 9, 3}, {7, 2, 7}, {900, 100, 10}, {1000, 1000, 1000}, {0, 0x0800, 0x4000}, {40, 7, 99}, {100, 50, 100}, 0, 0, 0},
        {{2, 2, 2}, {3, 0, 2},   {1, 1, 1}, {1, 0, 2}, {0, 7, 1}, {250, 125, 3},  {1000, 1000, 1000}, {0x0400, 0, 0x0800}, {0, 50, 100},  {100, 100, 100}, 1, 1, 1},
        {{2, 1, 3}, {0, 0, 0},   {0, 0, 0}, {5, 5, 5}, {1, 1, 1}, {7, 0, 4000},   {0, 9, 4000},       {0x0800, 0x4000, 0},  {10, 20, 30}, {0, 40, 60},   0, 1, 2},
        {{0, 3, 1}, {0x7F, 0x7F, 0x7F}, {1, 0, 1}, {2, 2, 2}, {7, 7, 7}, {1, 1, 1}, {2, 2, 2}, {0, 0, 0},          {1, 1, 1},    {1, 1, 1},     1, 0, 3},
        {{2, 2, 1}, {1, 2, 3},   {0, 1, 0}, {0, 4, 0}, {6, 7, 6}, {499, 124, 61}, {1000, 999, 500},   {0x1000, 0x0800, 0x1000}, {33, 66, 99}, {100, 100, 100}, 1, 0, 1},
        {{1, 1, 1}, {2, 0, 1},   {1, 0, 0}, {7, 1, 0}, {2, 3, 7}, {32767, 2, 0},  {32767, 3, 7},      {0x4000, 0x1000, 0x0800}, {-5, 0, 300}, {100, -1, 300}, 0, 1, 0},
    };
    unsigned k, salt, neg, proven68 = 0, proven38 = 0, total38 = 0;

    if (abi_leaf_listed("func_80075168")) {
        for (k = 0; k < sizeof(kCfg) / sizeof(kCfg[0]); k++)
            for (salt = 0; salt < 3u; salt++) {
                uint32_t args[4] = {0, 0, 0, 0};
                hp4_seed(&kCfg[k], salt + k);
                compare_name("func_80075168", args);
                proven68 += last_case_proven ? 1u : 0u;
            }
    }
    if (!abi_leaf_listed("func_80075938"))
        return;
    for (k = 0; k < sizeof(kCfg) / sizeof(kCfg[0]); k++)
        for (neg = 0; neg < 2u; neg++)
            for (salt = 0; salt < 2u; salt++) {
                uint32_t args[4] = {0, 0, 0, 0};
                unsigned i;
                hp4_seed(&kCfg[k], salt * 5u + k);
                for (i = 0; i < 3u; i++)    /* retail's unwritten w[i] */
                    hp_w32(HP4_WSLOT + i * 4u,
                           neg ? 0x80000000u | (salt * 0x01234567u + i) : 0x00000100u + salt + i);
                compare_name("func_80075938", args);
                total38++;
                proven38 += last_case_proven ? 1u : 0u;
            }
    if (proven38 != total38) {
        char detail[160];
        snprintf(detail, sizeof(detail),
                 "w[] garbage cases: %u of %u proven (a garbage-dependent output must stop the batch)",
                 proven38, total38);
        fail("func_80075938", "hand-coverage", detail);
    }
    fprintf(stderr, "[prover] h-hotpath main2 HUD: 80075168 proven=%u, 80075938 proven=%u of %u "
            "(w[] garbage negative and non-negative)\n", proven68, proven38, total38);
}

/* 800745EC (main2 retail): the menu panel's extra prims.  Inputs: the panel
 * state D_800D2D28[0xB7], the panel set *(*(D_800C3EA4) + 0xA230) (its
 * flags/indices at +0x668..+0x66F and its 0x28-byte prims), and the OT base
 * D_800CCB04[0] (entry 1 = +4).  Its callees run through the jal service:
 * AddPrim, func_800743A4/800744BC/80074554 and func_800728B8 (interpreted on
 * both sides here). */
#define HP4_PANEL 0x80150000u
#define HP4_OT    0x80158000u
static void hotpath_hand_cases_panel(void)
{
    static const uint8_t kFlags[][8] = { /* +0x668 (buffer), +0x669, +0x66A (idx), +0x66B, +0x66C (idx), +0x66D, +0x66E, +0x66F */
        {0, 1, 0, 1, 0, 1, 1, 1}, {1, 1, 1, 1, 1, 0, 0, 0}, {0, 0, 1, 0, 1, 1, 3, 1},
        {1, 3, 0, 2, 1, 4, 0, 5}, {1, 1, 1, 0, 0, 0, 2, 0},
    };
    unsigned state, k, i, proven = 0, total = 0;

    if (!abi_leaf_listed("func_800745EC"))
        return;
    for (state = 0; state <= 6u; state++)
        for (k = 0; k < sizeof(kFlags) / sizeof(kFlags[0]); k++) {
            uint32_t args[4] = {0, 0, 0, 0};
            begin_case((state + k) % 3u, 0);
            hp_w32(0x800C3EA4u, HP4_HUD);                 /* D_800C3EA4 */
            hp_w32(HP4_HUD + 0xA230u, HP4_PANEL);         /* the panel set */
            hp_fill(HP4_PANEL, 0x700u, state * 7u + k);
            for (i = 0; i < 8u; i++)
                hp_w8(HP4_PANEL + 0x668u + i, kFlags[k][i]);
            hp_w32(0x800CCB04u, HP4_OT);                  /* D_800CCB04[0] */
            hp_fill(HP4_OT, 0x40u, k);
            hp_w32(HP4_OT + 4u, 0x00FFFFFFu - k);         /* OT entry 1 */
            hp_w32(0x800D2D28u, HP4_MENU);                /* D_800D2D28 */
            hp_fill(HP4_MENU, 0x100u, k + 2u);
            hp_w8(HP4_MENU + 0xB7u, (uint8_t)state);
            compare_name("func_800745EC", args);
            total++;
            proven += last_case_proven ? 1u : 0u;
        }
    fprintf(stderr, "[prover] h-hotpath func_800745EC panel cases proven=%u of %u\n", proven, total);
}

/* Batch 4b: 800745EC's AddPrim leaves 800743A4 / 800744BC / 80074554, each
 * with its own cases over the panel set's buffer/index/enable bytes
 * (+0x668/+0x66A/+0x669) and the OT entry-1 word; same panel and OT layout
 * as the 800745EC cases. */
static void hp4b_panel_seed(unsigned buffer, unsigned enable, unsigned index, unsigned salt)
{
    begin_case(salt % 3u, 0);
    hp_w32(0x800C3EA4u, HP4_HUD);
    hp_w32(HP4_HUD + 0xA230u, HP4_PANEL);
    hp_fill(HP4_PANEL, 0x700u, salt * 3u + 1u);
    hp_w8(HP4_PANEL + 0x668u, (uint8_t)buffer);
    hp_w8(HP4_PANEL + 0x669u, (uint8_t)enable);
    hp_w8(HP4_PANEL + 0x66Au, (uint8_t)index);
    hp_w32(0x800CCB04u, HP4_OT);
    hp_fill(HP4_OT, 0x40u, salt);
    hp_w32(HP4_OT + 4u, salt & 1u ? 0x00FFFFFFu : 0x04158010u + salt * 0x28u);
}

static void hotpath_hand_cases_panel_leaves(void)
{
    static const char *const kLeaf[3] = {"func_800743A4", "func_800744BC", "func_80074554"};
    unsigned l, b, e, salt, proven[3] = {0, 0, 0}, total[3] = {0, 0, 0};

    for (l = 0; l < 3u; l++) {
        if (!abi_leaf_listed(kLeaf[l]))
            continue;
        for (b = 0; b < 2u; b++)          /* buffer / index 0, 1 */
            for (e = 0; e < 3u; e++)      /* enable 0, 1, 0x80 */
                for (salt = 0; salt < 2u; salt++) {
                    uint32_t args[4] = {0, 0, 0, 0};
                    hp4b_panel_seed(b, e == 0 ? 0u : e == 1 ? 1u : 0x80u, b ^ salt, salt + e + l);
                    compare_name(kLeaf[l], args);
                    total[l]++;
                    proven[l] += last_case_proven ? 1u : 0u;
                }
    }
    fprintf(stderr, "[prover] h-hotpath panel leaves proven: 800743A4 %u/%u, 800744BC %u/%u, 80074554 %u/%u\n",
            proven[0], total[0], proven[1], total[1], proven[2], total[2]);
}
/* ---- END h-hotpath (hand cases) ------------------------------------------- */

static void hand_cases(void)
{
    uint32_t args[4];

    hand_cases_8008ABB8();
    hand_cases_mesh();
    hand_cases_indirect();
    hand_cases_abi_slots();
    hand_cases_slot_sweep();
    hand_cases_slot_extra();
    hand_cases_a828();
    hand_cases_a7bc();
    hand_cases_85454();
    hand_cases_mesh_height();
    hand_cases_ba8f4();
    hand_cases_timer_callbacks();
    hand_cases_aaa20();
    hand_cases_a3e98();
    hand_cases_a0838();
    hand_cases_aad54();
    hand_cases_ae2a4();
    hand_cases_aff9c();
    hand_cases_model_matrices("func_8009EF3C");
    hand_cases_model_matrices("func_8009F1C4");
    frontier_hand_cases(); /* h-frontier */
    frontier_ba984_hand_cases(); /* h-frontier */
    frontier_tasks_hand_cases(); /* h-frontier */
    hotpath_hand_cases_aa514(); /* h-hotpath */
    hotpath_hand_cases_c11cc(); /* h-hotpath */
    hotpath_hand_cases_c11cc_null(); /* h-hotpath: battle_lift.h KUSEG alias */
    hotpath_hand_cases_bb844(); /* h-hotpath */
    hotpath_hand_cases_bd810(); /* h-hotpath */
    hotpath_hand_cases_bc460(); /* h-hotpath */
    hotpath_hand_cases_main2_hud(); /* h-hotpath */
    hotpath_hand_cases_panel(); /* h-hotpath */
    hotpath_hand_cases_panel_leaves(); /* h-hotpath */

    /* AA80AE4's result is undefined when no rotation slot qualifies. Seed a
     * valid 11-entry cycle so both sides return a defined selected value. */
    begin_case(0, 0);
    {
        uint8_t *base = (uint8_t *)PSX_ADDR(0x800D2DD7u);
        unsigned i;
        base[0] = 0;
        for (i = 0; i < 11; i++) {
            uint8_t value = (uint8_t)(i + 1);
            base[1 + i] = value;
            *(int16_t *)(base + 0x2F + value * 2) = (int16_t)((11 - i) * 16);
        }
    }
    args[0] = 0xFE; args[1] = args[2] = args[3] = 0;
    compare_name("func_80080AE4", args);

    /* AA600 follows a guest table slot -> record -> subrecord pointer chain.
     * Give the retail interpreter and the shipped host body the same valid
     * graph so this adapter is proven through the production runtime bridge. */
    begin_case(0, 0);
    *((uint32_t *)PSX_ADDR(0x800D3368u)) = REC;
    *((uint32_t *)PSX_ADDR(REC + 4)) = SRC;
    *((int16_t *)PSX_ADDR(REC + 0x1C)) = 0x300;
    *((int16_t *)PSX_ADDR(SRC + 0x4E)) = 0x280;
    *((int16_t *)PSX_ADDR(REC + 0x24)) = 0x500;
    args[0] = 0; args[1] = args[2] = args[3] = 0;
    compare_name("func_800AA600", args);

    begin_case(0, 0);
    *((uint32_t *)PSX_ADDR(WORD)) = 10;
    args[0] = WORD; args[1] = args[2] = args[3] = 0;
    compare_name("func_80079934", args);

    begin_case(0, 0);
    *((uint16_t *)PSX_ADDR(HALF)) = 0x1234;
    args[0] = HALF; args[1] = args[2] = args[3] = 0;
    compare_name("func_800A3484", args);

    begin_case(0, 0);
    *((uint16_t *)PSX_ADDR(REC + 0x98)) = 0x7777;
    args[0] = REC; args[1] = args[2] = args[3] = 0;
    compare_name("func_800AEEEC", args);

    begin_case(0, 0);
    args[0] = 7; args[1] = 7; args[2] = args[3] = 0;
    compare_name("func_80089B50", args);
    args[0] = 0xFFFF; args[1] = 3;
    compare_name("func_80089B50", args);
    args[0] = 0; args[1] = 0xFFFF;
    compare_name("func_80089B50", args);

    begin_case(0, 0);
    *((uint32_t *)PSX_ADDR(SRC + 0x10)) = 0x18;
    *((uint32_t *)PSX_ADDR(SRC + 0x14)) = 1;
    args[0] = SRC; args[1] = args[2] = args[3] = 0;
    compare_name("func_800B16A4", args);

    begin_case(0, 0);
    *((uint16_t *)PSX_ADDR(SRC)) = 2;
    *((uint16_t *)PSX_ADDR(SRC + 2)) = 1;
    *((uint16_t *)PSX_ADDR(SRC + 4)) = 2;
    *((uint16_t *)PSX_ADDR(SRC + 6)) = 3;
    args[0] = DST; args[1] = SRC; args[2] = args[3] = 0;
    compare_name("func_800B6930", args);

    begin_case(0, 0);
    *((uint16_t *)PSX_ADDR(SRC)) = 2;
    *((uint16_t *)PSX_ADDR(SRC + 2)) = 0x0011;
    *((uint16_t *)PSX_ADDR(SRC + 4)) = 0x0022;
    *((uint16_t *)PSX_ADDR(SRC + 6)) = 0x0033;
    args[0] = DST; args[1] = SRC; args[2] = args[3] = 0;
    compare_name("func_800B6990", args);

    begin_case(0, 0);
    *((uint16_t *)PSX_ADDR(DST)) = 10;
    *((uint16_t *)PSX_ADDR(DST + 2)) = 20;
    *((uint16_t *)PSX_ADDR(DST + 4)) = 30;
    args[0] = DST; args[1] = SRC; args[2] = args[3] = 0;
    compare_name("func_800B69E4", args);

    begin_case(0, 0);
    args[0] = 0; args[1] = 0; args[2] = 0xAB; args[3] = 0;
    compare_name("func_80079ED8", args);
    args[3] = 1;
    compare_name("func_80079ED8", args);

    begin_case(0, 0);
    args[0] = 0; args[1] = 0; args[2] = 0x3344; args[3] = 0;
    compare_name("func_8007A280", args);
    args[3] = 1;
    compare_name("func_8007A280", args);

    /* D_800D2D28 is pointer-typed in the retail image; give it a live target so
     * both sides dereference the same guest word. */
    begin_case(0, 0);
    *((uint32_t *)PSX_ADDR(0x800D2D28u)) = PTRT;
    args[0] = 1; args[1] = args[2] = args[3] = 0;
    compare_name("func_80079E18", args);
    compare_name("func_80079E4C", args);

    begin_case(0, 0);
    args[0] = DST; args[1] = 0; args[2] = 0x11111111u; args[3] = 0x22222222u;
    compare_name("func_800AA898", args);

    /* h-battle block (func_8008A9C0 closure): last, so no earlier hand case
     * sees a different rng stream or native state. */
    hand_cases_hbattle();
}

/* Deterministic sweep: scalar-argument leaves, several argument shapes, five RAM
 * seeds; then a pointer-argument pass for leaves that declare a pointer.
 *
 * The bridge cannot tell a scalar from a pointer: an argument that looks like a
 * KSEG0/KSEG1 address is translated to a host pointer before the body sees it.
 * Feeding pointer-shaped values to a scalar parameter would therefore measure
 * that convention rather than the body, and feeding scalar values to a pointer
 * parameter would make the host dereference a wild address that the interpreter
 * (which validates through the bus) would simply fault on. Leaves that declare a
 * pointer parameter are therefore given PTRT for exactly those parameters, with
 * the target window and the pointer globals seeded, which is the same typed
 * shape the hand cases construct by hand.
 *
 * Patterns 0-2 fill RAM only; patterns 3-4 repeat two of those fills with the
 * pointer-typed overlay globals seeded. Without the seeded pass, a body that
 * dereferences such a global faults on the retail side in every case and can
 * never be proven. The unseeded patterns still run: for those bodies they are
 * reported inconclusive, not passed. */
static void sweep(void)
{
    unsigned index;
    unsigned round;
    uint32_t pattern;

    for (index = 0; index < LEAF_COUNT; index++) {
        if (frontier_module_leaf(kLeaves[index].name)) {
            continue;
        }
        /* The dedicated hand case and remu differential cover AA80AE4's
         * valid input domain. Random scalar values can leave best undefined. */
        if (strcmp(kLeaves[index].name, "func_80080AE4") == 0)
            continue;
        /* 8007FEC4 allocates from the native heap, whose globals are not
         * part of the snapshot: only its hand case (fresh heap per side)
         * is a fair comparison. */
        if (strcmp(kLeaves[index].name, "func_8007FEC4") == 0)
            continue;
        /* h-frontier: 800BA984's cases are its seeded hand cases; a random
         * case's placeholder task word would reach native sprite code. */
        if (strcmp(kLeaves[index].name, "func_800BA984") == 0)
            continue;
        /* Step 11: these call the native allocator; its globals survive RAM
         * snapshots. Only the fresh-heap hand cases have a valid heap. */
        if (strcmp(kLeaves[index].name, "func_800BB6E0") == 0 ||
            strcmp(kLeaves[index].name, "func_800BB690") == 0 ||
            strcmp(kLeaves[index].name, "func_800A9540") == 0)
            continue;
        /* Each leaf draws its own sequence, seeded from its address, so
         * adding or removing an allowlist entry does not silently change the
         * inputs (and so the evidence) of every leaf after it. */
        rng_state = 0x12345678u ^ leaf_address(kLeaves[index].name);
        if (kLeaves[index].pointer_params != 0) {
            unsigned mask = kLeaves[index].pointer_params;

            hand_only++;
            for (pattern = 0; pattern < 3; pattern++) {
                for (round = 0; round < 4; round++) {
                    uint32_t args[4];
                    unsigned a;
                    begin_case(pattern, 1);
                    for (a = 0; a < 4; a++) {
                        if ((mask >> a) & 1u)
                            args[a] = PTRT + 0x100u * a;
                        else
                            args[a] = (round == 0) ? 0u : (rnd() & 0xFFFFu);
                    }
                    compare(&kLeaves[index], args);
                }
            }
            continue;
        }
        for (pattern = 0; pattern < 5; pattern++) {
            int seeded = pattern >= 3;
            uint32_t fill = seeded ? pattern - 3 : pattern;

            for (round = 0; round < 6; round++) {
                begin_case(fill, seeded);
                uint32_t args[4];
                unsigned a;
                for (a = 0; a < 4; a++) {
                    switch (rnd() % 4u) {
                    case 0: args[a] = 0; break;
                    case 1: args[a] = rnd() & 0xFFFFu; break;
                    case 2: args[a] = 1u; break;
                    default: args[a] = 0xFFFFFFFFu; break;
                    }
                }
                compare(&kLeaves[index], args);
            }
        }
    }
}

/* XENO_BATTLE_NATIVE_LEAVES (battle_mips_runtime.c): default keeps every
 * adopted leaf, 0 declines every one at the bridge and the indirect native
 * map, legacy keeps only the leaves before the guest-ABI section.  Runs after
 * the proof; it resets the read-once mode and the adoption caches it touches
 * and restores the default.  Returns the number of failures. */
static unsigned native_leaves_switch_test(void)
{
    static const char *const modes[] = {NULL, "0", "legacy", "1"};
    unsigned bad = 0, m;
    size_t legacy = 0, i;

    for (i = 0; i < OVERLAY_LEAF_COUNT; i++) {
        if (strcmp(kOverlayLeaves[i], OVERLAY_FIRST_GUEST_ABI_LEAF) == 0)
            break;
        legacy++;
    }
    if (legacy == 0 || legacy == OVERLAY_LEAF_COUNT) {
        fprintf(stderr, "BATTLE OVERLAY DIFFERENTIAL FAIL native-leaves switch: "
                "no legacy/guest-ABI boundary (legacy=%zu)\n", legacy);
        return 1;
    }
    for (m = 0; m < sizeof(modes) / sizeof(modes[0]); m++) {
        size_t expect = modes[m] == NULL ? OVERLAY_LEAF_COUNT
                      : strcmp(modes[m], "0") == 0 ? 0
                      : strcmp(modes[m], "legacy") == 0 ? legacy : OVERLAY_LEAF_COUNT;
        size_t limit, declined = 0;
        unsigned t;

        if (modes[m] == NULL)
            unsetenv("XENO_BATTLE_NATIVE_LEAVES");
        else
            setenv("XENO_BATTLE_NATIVE_LEAVES", modes[m], 1);
        g_OverlayLeafLimit = (size_t)-1;
        memset(rt.call_cache, 0, sizeof(rt.call_cache));
        memset(g_GuestNativeState, 0, sizeof(g_GuestNativeState));
        limit = PcPort_BattleNativeLeafLimit();
        if (limit != expect) {
            fprintf(stderr, "BATTLE OVERLAY DIFFERENTIAL FAIL native-leaves switch "
                    "mode=%s limit=%zu expected=%zu\n",
                    modes[m] ? modes[m] : "unset", limit, expect);
            bad++;
            continue;
        }
        for (i = 0; i < OVERLAY_LEAF_COUNT; i++) {
            int want = i < expect;
            uint32_t target = (uint32_t)strtoul(kOverlayLeaves[i] + 5, NULL, 16);
            if (PcPort_BattleNativeLeafAllowed(kOverlayLeaves[i]) != want) {
                fprintf(stderr, "BATTLE OVERLAY DIFFERENTIAL FAIL native-leaves switch "
                        "mode=%s %s allowed=%d expected=%d\n",
                        modes[m] ? modes[m] : "unset", kOverlayLeaves[i], !want, want);
                bad++;
            }
            if (want)
                continue;
            /* A declined leaf: the JAL bridge must hand it to the interpreter
             * (0, no host call), and the indirect native map must not hold it. */
            initialize_cpu(&cpu, &rt);
            if (runtime_bridge_call(&rt, &cpu, target) != 0) {
                fprintf(stderr, "BATTLE OVERLAY DIFFERENTIAL FAIL native-leaves switch "
                        "mode=%s bridge adopted declined %s\n",
                        modes[m] ? modes[m] : "unset", kOverlayLeaves[i]);
                bad++;
            }
            for (t = 0; t < BATTLE_GUEST_THUNK_COUNT; t++) {
                if (kBattleGuestThunks[t].address == target &&
                    battle_guest_native_host(&kBattleGuestThunks[t]) != NULL) {
                    fprintf(stderr, "BATTLE OVERLAY DIFFERENTIAL FAIL native-leaves "
                            "switch mode=%s indirect map kept declined %s\n",
                            modes[m] ? modes[m] : "unset", kOverlayLeaves[i]);
                    bad++;
                }
            }
            declined++;
        }
        printf("BATTLE OVERLAY DIFFERENTIAL native-leaves switch mode=%s native=%zu "
               "declined=%zu bridge-declined-ok\n",
               modes[m] ? modes[m] : "unset", limit, declined);
    }
    unsetenv("XENO_BATTLE_NATIVE_LEAVES");
    g_OverlayLeafLimit = (size_t)-1;
    memset(rt.call_cache, 0, sizeof(rt.call_cache));
    memset(g_GuestNativeState, 0, sizeof(g_GuestNativeState));
    return bad;
}

int main(void)
{
    const char *module_scope = getenv("BATTLE_OVERLAY_DIFFERENTIAL_MODULE0C4_ONLY");
    frontier_module_only = module_scope != NULL && strcmp(module_scope, "1") == 0;
    if (frontier_module_only) {
        setenv("XENO_BATTLE_NATIVE_HOSTS", "1", 1);
        setenv("XENO_CPU_EXECUTION_STATS", "1", 1);
        PcPortMipsExecutionStatsInit();
    }
    load();
    runtime_up();
    {
        size_t d;
        for (d = 0; d < rt.data_count; d++) {
            if (strcmp(rt.data[d].name, "g_GameState") == 0 &&
                rt.data[d].size >= NATIVE_GS_SIZE)
                native_gs = rt.data[d].host;
        }
        if (native_gs == NULL)
            fprintf(stderr, "BATTLE OVERLAY DIFFERENTIAL note: g_GameState "
                    "is not a shared native binding; not compared\n");
        frontier_globals_bind(rt.data, rt.data_count); /* h-frontier */
    }

    /* A host body that runs off g_PsxRam must not take the whole run with it. */
    signal(SIGSEGV, host_fault_handler);
    signal(SIGBUS, host_fault_handler);

    /* The prover is worthless if the host bodies are absent; fail loudly. */
    if (rt.function_count == 0)
        fprintf(stderr, "BATTLE OVERLAY DIFFERENTIAL note: no native bindings\n");

    if (frontier_module_only)
        frontier_module_hand_cases();
    else {
        hand_cases();
        sweep();
    }

    if (!frontier_module_only)
        coverage_report();
    if (getenv("BATTLE_OVERLAY_DIFFERENTIAL_PER_LEAF") != NULL) {
        unsigned i;
        for (i = 0; i < LEAF_COUNT; i++) {
            if (frontier_module_leaf(kLeaves[i].name) != frontier_module_only)
                continue;
            fprintf(stderr, "[prover] leaf %s proven=%u inconclusive=%u\n",
                    kLeaves[i].name, proven[i], inconclusive_per_leaf[i]);
        }
    }
    if (failures == 0) {
        /* An allowlisted leaf with no completed comparison is not evidence of
         * anything. Say so by name; the batch has to be pruned. */
        unsigned i;
        for (i = 0; i < LEAF_COUNT; i++) {
            if (frontier_module_leaf(kLeaves[i].name) != frontier_module_only)
                continue;
            if (proven[i] != 0)
                continue;
            fprintf(stderr, "BATTLE OVERLAY DIFFERENTIAL UNPROVEN %s\n",
                    kLeaves[i].name);
            failures++;
        }
    }
    if (!frontier_module_only)
        failures += native_leaves_switch_test();
    /* XENO_BATTLE_EXIT_HASH formatter on the harness RAM (smoke: every region
     * must be readable through the bus). */
    battle_exit_hash_print(&rt, "harness");
    printf("BATTLE OVERLAY DIFFERENTIAL spy-precision matched-args frame=%lu "
           "image-exact=%lu guest=%lu\n",
           spy_frame_args, spy_image_exact, spy_guest_args);
    if (failures != 0) {
        fprintf(stderr, "BATTLE OVERLAY DIFFERENTIAL FAIL total=%u\n", failures);
        return 1;
    }
    printf("BATTLE OVERLAY DIFFERENTIAL PASS checks=%u inconclusive=%u "
           "hand-only=%u leaves=%u store-pass-through=%u host-address=%u "
           "spied-cases=%u\n",
           checks, inconclusive, hand_only,
           frontier_module_only ? FRONTIER_MODULE_LEAF_COUNT :
               (unsigned)LEAF_COUNT - FRONTIER_MODULE_LEAF_COUNT,
           store_pass_through_checks, host_ref_cases, spied_cases);
    return 0;
}
