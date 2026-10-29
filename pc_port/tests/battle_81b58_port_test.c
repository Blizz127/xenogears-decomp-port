/*
 * Differential certificate for the native battle command-menu state-5 handler
 *   func_80081B58 (src/battle/mainasm_12068_port.c)
 *
 * CPU side: the retail overlay (disc/battle.bin, loaded whole at 0x8006FAF0) runs
 * func_80081B58 on the MIPS adapter.  Native side: the port body, same RAM, same
 * argument word.  Every callee is a recording spy on BOTH sides: on the CPU the
 * bridge intercepts the retail call target, natively the spy is linked in place of
 * the callee.  Both run the same logic (spy_do): they log the call id, the
 * arguments (wide callees compared as full words, so a missing actor mask shows)
 * and a hash of the watched guest memory (store-vs-call ORDER, not just the final
 * RAM), return a random FULL word (so a missing result mask shows), then perturb
 * the state the way the real callees do: the menu state D_800D3014, the context
 * counters (+0x2D4/+0x2D5/+0x2DF/+0x2E1/+0x2E5/+0x2E8/+0x2E9), the actor-block
 * flags, and a swap of the D_800C3EAC / D_800D2D28 pointer words to a second
 * random arena, so a body that caches what retail re-reads differs.  After both
 * runs the call logs, all of guest RAM outside the CPU stack window and the CPU's
 * callee-saved registers must match.  (Retail's v0 on return is residue and is
 * not compared.)
 *
 * Coverage gate: every retail instruction of func_80081B58 is fetched at least
 * once by a compared case, and every spy is called.  Retail bytes are only read
 * from disc/, never stored in the repo.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "battle_mips_adapter.h"
#include "battle_host_guest.h"

u8 g_PsxRam[PSX_RAM_SIZE];
u8 g_PsxScratchpad[4096];

s32 func_80081B58(u32 arg0);

#define FAIL(...)                                    \
    do {                                             \
        fprintf(stderr, "B81B58 FAIL: " __VA_ARGS__); \
        fputc('\n', stderr);                         \
        exit(1);                                     \
    } while (0)

enum {
    kRamBytes = 0x200000,
    kBattleBase = 0x8006FAF0u,
    kStackTop = 0x801FF000u,
    kStackLo = 0x801F0000u, /* CPU stack window, excluded from the RAM compare */
    kHalt = 0xFFFFFFFCu,
    kBudget = 20000,

    kFnAddr = 0x80081B58u,
    kFnInsns = 339,

    kCtxPtr = 0x800C3EACu,
    kBlkPtr = 0x800D2D28u,
    kCtxA = 0x80130000u,
    kCtxB = 0x80131000u,
    kBlkA = 0x80132000u,
    kBlkB = 0x80133000u,
    kArena = 0x800,
    kGameState = 0x8006D634u,
    kGameStateEnd = 0x8006F9F0u,
    kAct32A0 = 0x800D32A0u,
    kAct32A1 = 0x800D32A1u,
    kAct2D24 = 0x800D2D24u
};

/* ---- guest RAM helpers --------------------------------------------------- */
static u8* gp(u32 a) { return g_PsxRam + (a & 0x1FFFFFu); }
static u32 g32(u32 a) { u32 v; memcpy(&v, gp(a), 4); return v; }
static u32 g16(u32 a) { u16 v; memcpy(&v, gp(a), 2); return v; }
static u32 g8(u32 a) { return *gp(a); }
static void p32(u32 a, u32 v) { memcpy(gp(a), &v, 4); }
static void p16(u32 a, u32 v) { u16 h = (u16)v; memcpy(gp(a), &h, 2); }
static void p8(u32 a, u32 v) { *gp(a) = (u8)v; }

static u32 s_rng = 0x1234567u;
static u32 rnd(void)
{
    s_rng ^= s_rng << 13;
    s_rng ^= s_rng >> 17;
    s_rng ^= s_rng << 5;
    return s_rng;
}

static unsigned char s_hit[kFnInsns];

/* ---- callee spies --------------------------------------------------------- */
/* Argument kinds: w word, b u8 (the port declares those callees with u8). */
#define SPIES(X)                                      \
    X(W77980, 0x80077980u, 0, "")                     \
    X(W80B64, 0x80080B64u, 1, "b")                    \
    X(W8189C, 0x8008189Cu, 1, "w")                    \
    X(W819A4, 0x800819A4u, 1, "w")                    \
    X(W84854, 0x80084854u, 2, "ww")                   \
    X(W85D34, 0x80085D34u, 1, "w")                    \
    X(W85EB4, 0x80085EB4u, 2, "ww")                   \
    X(W861D0, 0x800861D0u, 2, "ww")                   \
    X(W877E0, 0x800877E0u, 2, "ww")                   \
    X(W879A8, 0x800879A8u, 2, "bb")                   \
    X(W87AF0, 0x80087AF0u, 2, "ww")                   \
    X(WAA40, 0x8008AA40u, 1, "b")                     \
    X(W9413C, 0x8009413Cu, 2, "bb")                   \
    X(WB8DA4, 0x800B8DA4u, 0, "")

enum {
#define X(id, addr, n, kinds) S_##id,
    SPIES(X)
#undef X
    S_COUNT
};
static const struct { const char* name; u32 addr; unsigned nargs; const char* kinds; } kSpy[S_COUNT] = {
#define X(id, addr, n, kinds) [S_##id] = { #id, addr, n, kinds },
    SPIES(X)
#undef X
};
static unsigned s_spyCalled[S_COUNT];

typedef struct {
    u32 id;
    u32 a[3];
    u32 hash;
} Call;
enum { kMaxCalls = 64 };
static Call s_log[kMaxCalls];
static unsigned s_nlog;
static u32 s_spyRng;
static u32 s_actor;

static u32 spy_rnd(void)
{
    s_spyRng ^= s_spyRng << 13;
    s_spyRng ^= s_spyRng >> 17;
    s_spyRng ^= s_spyRng << 5;
    return s_spyRng;
}

static u32 fnv(u32 h, const u8* p, unsigned n)
{
    while (n--) {
        h = (h ^ *p++) * 16777619u;
    }
    return h;
}

static u32 watch_hash(void)
{
    static const struct { u32 a, n; } kW[] = {
        { kCtxPtr, 4 },          { kBlkPtr, 4 },          { kCtxA, 0x600 },        { kCtxA + 0x2D0, 0x40 },
        { kCtxB, 0x600 },        { kCtxB + 0x2D0, 0x40 }, { kBlkA, 0x100 },        { kBlkB, 0x100 },
        { 0x800D2E00u, 0x40 },   { 0x800D3010u, 0x10 },   { 0x800D3660u, 0x20 },
    };
    u32 h = 2166136261u;
    unsigned i;

    for (i = 0; i < sizeof(kW) / sizeof(kW[0]); ++i) {
        h = fnv(h, gp(kW[i].a), kW[i].n);
    }
    return h;
}

static u32 norm_arg(char kind, u32 v)
{
    return kind == 'b' ? (v & 0xFFu) : v;
}

static void swap_ctx(void) { p32(kCtxPtr, g32(kCtxPtr) == kCtxA ? kCtxB : kCtxA); }
static void swap_blk(void) { p32(kBlkPtr, g32(kBlkPtr) == kBlkA ? kBlkB : kBlkA); }

/* A small counter value: usually 0..5 (so +0x2D4 == +0x2D5, the < 3 and the
 * negative differences all occur), now and then any byte. */
static u32 small_or_any(void)
{
    return (spy_rnd() % 8u == 0) ? (spy_rnd() & 0xFFu) : (spy_rnd() % 6u);
}

/* What the real callees do to the state, deterministically from the spy RNG
 * (so the retail and native runs see the same changes). */
static void drive(int id)
{
    u32 ctx = g32(kCtxPtr), blk = g32(kBlkPtr), row;

    if (spy_rnd() % 3u == 0) {
        u32 r = spy_rnd() % 12u;
        p8(0x800D3014u, r < 8 ? r : (r == 8 ? 0xFFu : (r == 9 ? 5u : (spy_rnd() & 0xFFu))));
    }
    if (spy_rnd() % 4u == 0) { p8(ctx + 0x2D4, small_or_any()); }
    if (spy_rnd() % 4u == 0) { p8(ctx + 0x2D5, (spy_rnd() % 3u == 0) ? g8(ctx + 0x2D4) : small_or_any()); }
    if (spy_rnd() % 6u == 0) { p8(ctx + 0x2D3, spy_rnd()); }
    if (spy_rnd() % 6u == 0) { p8(ctx + 0x2E8, spy_rnd()); }
    if (id == S_W8189C && spy_rnd() % 8u == 0) { p8(ctx + 0x2DF, spy_rnd()); }
    if (spy_rnd() % 12u == 0) { p8(ctx + 0x2E1, spy_rnd() & 1u); }
    if (spy_rnd() % 4u == 0) { p8(ctx + 0x2E5, (spy_rnd() & 1u) ? 0u : spy_rnd()); }
    if (spy_rnd() % 10u == 0) { p8(ctx + 0x2E9, (spy_rnd() & 1u) ? 0u : spy_rnd()); }
    if (spy_rnd() % 3u == 0) { p8(blk + 0xCB, (spy_rnd() & 1u) ? 0u : spy_rnd()); }
    row = ctx + s_actor * 0x40u;
    if (spy_rnd() % 4u == 0) { p16(row + 0x1C, (spy_rnd() & 1u) ? 0u : spy_rnd()); }
    if (spy_rnd() % 4u == 0) { p16(row + 0x1E, (spy_rnd() & 1u) ? 0u : spy_rnd()); }
    if (spy_rnd() % 4u == 0) { p16(row + 0x20, (spy_rnd() & 1u) ? 0u : spy_rnd()); }
    if (spy_rnd() % 7u == 0) swap_ctx();
    if (spy_rnd() % 7u == 0) swap_blk();
}

static u32 spy_do(int id, const u32* raw)
{
    Call* c;
    unsigned i;
    u32 ret = 0;

    if (s_nlog >= kMaxCalls) {
        FAIL("call log overflow (runaway?)");
    }
    s_spyCalled[id]++;
    c = &s_log[s_nlog++];
    memset(c, 0, sizeof(*c));
    c->id = (u32)id;
    for (i = 0; i < kSpy[id].nargs; ++i) {
        c->a[i] = norm_arg(kSpy[id].kinds[i], raw[i]);
    }
    c->hash = watch_hash();

    /* A random full word: the port must mask the byte results itself.  A
     * quarter of the results have a clear low byte (a high bit alone must not
     * count as nonzero). */
    switch (id) {
    case S_W84854:
    case S_W85EB4:
    case S_W87AF0:
    case S_W877E0:
        ret = spy_rnd();
        if (spy_rnd() % 4u == 0) ret &= 0xFFFFFF00u;
        break;
    default: break;
    }
    drive(id);
    return ret;
}

/* ---- native-side spies: the symbols the port TU calls --------------------- */
void func_80077980(void) { u32 raw[3] = { 0 }; spy_do(S_W77980, raw); }
void func_80080B64(u8 v) { u32 raw[3] = { v }; spy_do(S_W80B64, raw); }
void func_8008189C(u32 v) { u32 raw[3] = { v }; spy_do(S_W8189C, raw); }
void func_800819A4(u32 v) { u32 raw[3] = { v }; spy_do(S_W819A4, raw); }
u32 func_80084854(u32 a, u32 b) { u32 raw[3] = { a, b }; return spy_do(S_W84854, raw); }
void func_80085D34(u32 v) { u32 raw[3] = { v }; spy_do(S_W85D34, raw); }
u32 func_80085EB4(u32 a, u32 b) { u32 raw[3] = { a, b }; return spy_do(S_W85EB4, raw); }
void func_800861D0(u32 a, u32 b) { u32 raw[3] = { a, b }; spy_do(S_W861D0, raw); }
u32 xeno_battle_guest_800877E0(u32 a, u32 b) { u32 raw[3] = { a, b }; return spy_do(S_W877E0, raw); }
void func_800879A8(u8 a, u8 b) { u32 raw[3] = { a, b }; spy_do(S_W879A8, raw); }
u32 func_80087AF0(u32 a, u32 b) { u32 raw[3] = { a, b }; return spy_do(S_W87AF0, raw); }
void func_8008AA40(u8 v) { u32 raw[3] = { v }; spy_do(S_WAA40, raw); }
void func_8009413C(u8 a, u8 b) { u32 raw[3] = { a, b }; spy_do(S_W9413C, raw); }
void func_800B8DA4(void) { u32 raw[3] = { 0 }; spy_do(S_WB8DA4, raw); }

/* ---- CPU side ------------------------------------------------------------- */
static u8* bus_ptr(u32 a, unsigned w)
{
    u32 hi = a & 0xFF800000u;

    if ((hi == 0x80000000u || hi == 0xA0000000u || hi == 0u) && (a & 0x1FFFFFu) + w <= 0x200000u) {
        return g_PsxRam + (a & 0x1FFFFFu);
    }
    return NULL;
}

static int bus_read(void* u, u32 a, unsigned w, u32* v)
{
    u8* p = bus_ptr(a, w);
    unsigned i;

    (void)u;
    if (p == NULL) {
        return -1;
    }
    *v = 0;
    for (i = 0; i < w; ++i) {
        *v |= (u32)p[i] << (i * 8);
    }
    return 0;
}

static int bus_write(void* u, u32 a, unsigned w, u32 v)
{
    u8* p = bus_ptr(a, w);
    unsigned i;

    (void)u;
    if (p == NULL) {
        return -1;
    }
    for (i = 0; i < w; ++i) {
        p[i] = (u8)(v >> (i * 8));
    }
    return 0;
}

static int s_record;
static int bus_bridge(void* u, PcPortMipsCpu* c, u32 t)
{
    int id;

    (void)u;
    if (t >= kFnAddr && t < kFnAddr + kFnInsns * 4u) {
        if (s_record) {
            s_hit[(t - kFnAddr) / 4] = 1;
        }
        return 0; /* the batch code runs on the CPU */
    }
    for (id = 0; id < S_COUNT; ++id) {
        if (kSpy[id].addr == t) {
            u32 raw[3];

            raw[0] = c->gpr[4];
            raw[1] = c->gpr[5];
            raw[2] = c->gpr[6];
            c->gpr[2] = spy_do(id, raw);
            return 1;
        }
    }
    return -1; /* an unexpected call target fails the case */
}

static u8 s_snapshot[PSX_RAM_SIZE], s_cpuRam[PSX_RAM_SIZE];

static void load_retail(void)
{
    const char* path = getenv("B81B58_BATTLE_BIN") ? getenv("B81B58_BATTLE_BIN") : "disc/battle.bin";
    FILE* f = fopen(path, "rb");
    size_t n;

    if (f == NULL) {
        FAIL("cannot open battle.bin");
    }
    n = fread(gp(kBattleBase), 1, 0x200000u - (kBattleBase & 0x1FFFFFu), f);
    fclose(f);
    if (n < 0x53000u) {
        FAIL("battle.bin short read (%zu)", n);
    }
}

/* ---- state ---------------------------------------------------------------- */
/* Random words into the overlay's data/bss (including the hit-scale table
 * D_800C31D4: data only, never code -- the CPU runs nothing but the function
 * under test) and the two arenas. */
static void fill_random(void)
{
    u32 a;

    for (a = 0x800C31D4u; a < 0x800C3A60u; a += 4) p32(a, (rnd() % 4u == 0) ? 0u : rnd());
    for (a = 0x800C3A80u; a < 0x800D3C00u; a += 4) p32(a, rnd());
    for (a = kCtxA; a < kBlkB + kArena; a += 4) p32(a, rnd());
}

/* Build the whole guest state for one case; returns the argument word. */
static u32 build(unsigned k)
{
    u32 actor, arg0, i, r;

    (void)k;
    memcpy(g_PsxRam, s_snapshot, kRamBytes);
    fill_random();
    s_spyRng = (u32)(rnd() | 1u);
    memset(s_log, 0, sizeof(s_log));
    s_nlog = 0;

    r = rnd() % 10u;
    actor = (r < 8) ? rnd() % 3u : (r == 8 ? 3u : 4u + rnd() % 0xF0u);
    arg0 = actor;
    r = rnd() % 4u;
    if (r == 1) arg0 |= 0x100u;
    if (r == 2) arg0 |= 0xFFFFFF00u;
    if (r == 3) arg0 |= rnd() & 0xFFFFFF00u;
    s_actor = actor;

    p32(kCtxPtr, kCtxA);
    p32(kBlkPtr, kBlkA);
    for (i = 0; i < 2; ++i) {
        u32 ctx = i ? kCtxB : kCtxA, row = ctx + actor * 0x40u, blk = i ? kBlkB : kBlkA;

        p8(ctx + 0x2E1, rnd() % 8u == 0);
        p8(ctx + 0x2E9, (rnd() % 3u == 0) ? rnd() : 0u);
        p8(ctx + 0x2D4, (rnd() % 8u == 0) ? rnd() : rnd() % 6u);
        p8(ctx + 0x2D5, (rnd() % 3u == 0) ? g8(ctx + 0x2D4) : rnd() % 6u);
        p16(row + 0x1C, (rnd() & 1u) ? 0u : rnd());
        p16(row + 0x1E, (rnd() & 1u) ? 0u : rnd());
        p16(row + 0x20, (rnd() & 1u) ? 0u : rnd());
        p8(blk + 0xCB, (rnd() & 1u) ? 0u : rnd());
    }
    r = rnd() % 12u;
    p8(0x800D3014u, r < 8 ? r : (r == 8 ? 0xFFu : (r == 9 ? 5u : (rnd() & 0xFFu))));
    return arg0;
}

static unsigned s_checks;
static Call s_cpuLog[kMaxCalls];
static unsigned s_cpuN;
static u8 s_built[PSX_RAM_SIZE];

static void run_case(const char* name, unsigned k)
{
    PcPortMipsCpu cpu;
    PcPortMipsBus bus;
    u32 i, saved[32], rngsave, arg0;
    int rc;

    arg0 = build(k);
    memcpy(s_built, g_PsxRam, kRamBytes);
    rngsave = s_spyRng;

    /* ---- retail CPU ---- */
    memset(&bus, 0, sizeof(bus));
    bus.read = bus_read;
    bus.write = bus_write;
    bus.bridge = bus_bridge;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[29] = kStackTop;
    cpu.gpr[31] = kHalt;
    cpu.gpr[4] = arg0;
    for (i = 16; i <= 23; ++i) cpu.gpr[i] = 0x51000000u + i;
    cpu.gpr[30] = 0x5100001Eu;
    memcpy(saved, cpu.gpr, sizeof(saved));
    s_record = 1;
    rc = PcPortMipsRun(&cpu, kFnAddr, kHalt, kBudget);
    s_record = 0;
    if (rc != PC_PORT_MIPS_HALTED) {
        FAIL("%s: retail CPU rc=%d (%s) a0=%08x", name, rc, cpu.error, arg0);
    }
    if (cpu.gpr[29] != kStackTop) FAIL("%s: retail sp not restored", name);
    for (i = 16; i <= 23; ++i) {
        if (cpu.gpr[i] != saved[i]) FAIL("%s: retail clobbered s%u", name, i - 16);
    }
    if (cpu.gpr[30] != saved[30]) FAIL("%s: retail clobbered fp", name);
    memcpy(s_cpuRam, g_PsxRam, kRamBytes);
    memcpy(s_cpuLog, s_log, sizeof(s_log));
    s_cpuN = s_nlog;

    /* ---- native port body, same initial state ---- */
    memcpy(g_PsxRam, s_built, kRamBytes);
    s_spyRng = rngsave;
    memset(s_log, 0, sizeof(s_log));
    s_nlog = 0;
    func_80081B58(arg0);

    if (s_cpuN != s_nlog) {
        FAIL("%s: callee count retail=%u native=%u (a0=%08x)", name, s_cpuN, s_nlog, arg0);
    }
    for (i = 0; i < s_nlog; ++i) {
        const Call* x = &s_cpuLog[i];
        const Call* y = &s_log[i];
        unsigned j;

        if (x->id != y->id) {
            FAIL("%s: call %u retail=%s native=%s (a0=%08x)", name, i, kSpy[x->id].name,
                 kSpy[y->id].name, arg0);
        }
        for (j = 0; j < kSpy[x->id].nargs; ++j) {
            if (x->a[j] != y->a[j]) {
                FAIL("%s: call %u %s arg%u retail=%08x native=%08x", name, i, kSpy[x->id].name, j,
                     x->a[j], y->a[j]);
            }
        }
        if (x->hash != y->hash) {
            FAIL("%s: call %u %s: guest state at the call differs (store order / cached read)", name, i,
                 kSpy[x->id].name);
        }
    }
    {
        u32 lo = kStackLo & 0x1FFFFFu, hi = lo + 0x10000u;

        if (memcmp(s_cpuRam, g_PsxRam, lo) != 0 ||
            memcmp(s_cpuRam + hi, g_PsxRam + hi, kRamBytes - hi) != 0) {
            for (i = 0; i < kRamBytes; ++i) {
                if (i >= lo && i < hi) continue;
                if (s_cpuRam[i] != g_PsxRam[i]) {
                    FAIL("%s: RAM 0x%08x retail=%02x native=%02x (a0=%08x)", name, 0x80000000u + i,
                         s_cpuRam[i], g_PsxRam[i], arg0);
                }
            }
        }
    }
    s_checks++;
}

int main(void)
{
    unsigned k, missing = 0, covered = 0, unreachable = 0, id, cases = 6000;
    char name[64];

    if (getenv("B81B58_CASES") != NULL) {
        cases = (unsigned)atoi(getenv("B81B58_CASES"));
    }
    load_retail();
    memcpy(s_snapshot, g_PsxRam, kRamBytes);
    for (k = 0; k < cases; ++k) {
        s_rng = 0x9E3779B9u + k * 2654435761u + 7u;
        snprintf(name, sizeof(name), "func_80081B58 #%u", k);
        if (getenv("B81B58_TRACE") != NULL) {
            fprintf(stderr, "%s\n", name);
        }
        run_case(name, k);
    }

    /* Coverage gate: every retail instruction, every spy called. */
    for (k = 0; k < kFnInsns; ++k) {
        /* 0x80081D18/D1C (`j .L80081DA0` and its delay slot) sit on the path
         * "state byte < 2 but neither 0 nor 1" of retail's state compare
         * chain: unreachable for an unsigned byte. */
        if (kFnAddr + k * 4u == 0x80081D18u || kFnAddr + k * 4u == 0x80081D1Cu) {
            if (s_hit[k]) {
                FAIL("the unreachable instruction 0x%08x ran", kFnAddr + k * 4u);
            }
            covered++;
            unreachable++;
            continue;
        }
        if (s_hit[k]) {
            covered++;
        } else {
            fprintf(stderr, "B81B58 COVERAGE: instruction 0x%08x never ran\n", kFnAddr + k * 4u);
            missing++;
        }
    }
    for (id = 0; id < S_COUNT; ++id) {
        if (s_spyCalled[id] == 0) {
            fprintf(stderr, "B81B58 COVERAGE: spy %s never called\n", kSpy[id].name);
            missing++;
        }
    }
    if (getenv("B81B58_SPYCOUNTS") != NULL) {
        for (id = 0; id < S_COUNT; ++id) {
            fprintf(stderr, "spy %-8s called %u\n", kSpy[id].name, s_spyCalled[id]);
        }
    }
    if (missing != 0) {
        FAIL("coverage gate: %u item(s) missing", missing);
    }
    printf("B81B58 PORT PASS checks=%u retail instr coverage %u/%u (%u provably unreachable), %u spies\n",
           s_checks, covered, (unsigned)kFnInsns, unreachable, (unsigned)S_COUNT);
    return 0;
}

