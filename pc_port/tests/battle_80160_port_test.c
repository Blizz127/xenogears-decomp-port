/*
 * Differential certificate for the native battle command-menu driver
 *   func_80080160 (src/battle/main34_q1_port.c)
 *
 * CPU side: the retail overlay (disc/battle.bin, loaded whole at 0x8006FAF0) runs
 * func_80080160 on the MIPS adapter.  Native side: the port body, same RAM, same
 * argument word.  Every callee -- the frame wait, the ring/menu builders, the
 * eighteen state handlers, the draw/mask helpers -- is a recording spy on BOTH
 * sides: on the CPU the bridge intercepts the retail call target, natively the spy
 * is linked in place of the callee.  Both run the same logic (spy_do): they log
 * the call id, the arguments (handlers and wide callees compared as full words, so
 * a missing actor mask shows) and a hash of the watched guest memory (store-vs-call
 * ORDER, not just the final RAM), then run a deterministic "driver" that perturbs
 * the state the way the real callees do: the frame wait completes the slot wait
 * and may end the loop, handlers change the menu state (+0x2DD, every value 0..0xFF
 * class), set the command/exit flags, and any spy may swap D_800C3EAC / D_800D2D28
 * to a second fully random arena or flip the actor's D_800D32A1 flag mid-loop, so a
 * body that caches what retail re-reads differs.  Every exit site of the loop is
 * driven.  After both runs the call logs, all of guest RAM outside the CPU stack
 * window and the CPU's callee-saved registers must match.
 *
 * Coverage gate: every retail instruction of func_80080160 is fetched at least once
 * by a compared case, and every spy is called.  Retail bytes are only read from
 * disc/, never stored in the repo.
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

void func_80080160(u32 arg0);

#define FAIL(...)                                    \
    do {                                             \
        fprintf(stderr, "B80160 FAIL: " __VA_ARGS__); \
        fputc('\n', stderr);                         \
        exit(1);                                     \
    } while (0)

enum {
    kRamBytes = 0x200000,
    kBattleBase = 0x8006FAF0u,
    kStackTop = 0x801FF000u,
    kStackLo = 0x801F0000u, /* CPU stack window, excluded from the RAM compare */
    kHalt = 0xFFFFFFFCu,
    kBudget = 400000,

    kFnAddr = 0x80080160u,
    kFnInsns = 609,

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
/* Argument kinds: w word, b u8. */
#define SPIES(X)                                      \
    X(W716D8, 0x800716D8u, 0, "")                     \
    X(AA74, 0x8008AA74u, 1, "w")                      \
    X(FEC4, 0x8007FEC4u, 0, "")                       \
    X(FF14, 0x8007FF14u, 1, "w")                      \
    X(E78, 0x80085E78u, 0, "")                        \
    X(AB00, 0x8009AB00u, 1, "b")                      \
    X(C08, 0x80089C08u, 1, "b")                       \
    X(BC404, 0x800BC404u, 1, "w")                     \
    X(BCD98, 0x800BCD98u, 1, "w")                     \
    X(FBE0, 0x8007FBE0u, 2, "bb")                     \
    X(D598, 0x8008D598u, 3, "bbb")                    \
    X(BAF40, 0x800BAF40u, 2, "ww")                    \
    X(FCE8, 0x8007FCE8u, 0, "")                       \
    X(FDEC, 0x8007FDECu, 0, "")                 \
    X(E8, 0x800800E8u, 1, "b")                        \
    X(FB70, 0x8007FB70u, 1, "b")                      \
    X(H8115C, 0x8008115Cu, 1, "w")                    \
    X(H81318, 0x80081318u, 1, "w")                    \
    X(H81504, 0x80081504u, 1, "w")                    \
    X(H816F8, 0x800816F8u, 1, "w")                    \
    X(H81B58, 0x80081B58u, 1, "w")                    \
    X(H820A4, 0x800820A4u, 1, "w")                    \
    X(H822C4, 0x800822C4u, 1, "w")                    \
    X(H82504, 0x80082504u, 1, "w")                    \
    X(H82820, 0x80082820u, 1, "w")                    \
    X(H829F4, 0x800829F4u, 1, "w")                    \
    X(H82BB0, 0x80082BB0u, 1, "w")                    \
    X(H82D4C, 0x80082D4Cu, 1, "w")                    \
    X(H82F7C, 0x80082F7Cu, 1, "w")                    \
    X(H830A8, 0x800830A8u, 1, "w")                    \
    X(H83340, 0x80083340u, 1, "w")                    \
    X(H83580, 0x80083580u, 1, "w")                    \
    X(H83748, 0x80083748u, 1, "w")                    \
    X(H83948, 0x80083948u, 1, "w")

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
enum { kMaxCalls = 400 };
static Call s_log[kMaxCalls];
static unsigned s_nlog;

typedef struct {
    u32 actor;
    unsigned waitN;     /* the slot wait completes at this frame-wait call */
    unsigned swapWait;  /* swap D_800D2D28 on the first wait call */
    unsigned exitMode;  /* which loop exit the driver forces once late */
    unsigned exitN;     /* ... after this many logged calls */
    unsigned phase;     /* 0 = slot wait, 1 = after the sound call */
    unsigned waitCalls;
    unsigned inLoop;    /* the menu loop's own calls have started */
} Scenario;
static Scenario sc;
static unsigned s_callN[S_COUNT];
static u32 s_spyRng;

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
        { kCtxPtr, 4 },        { kBlkPtr, 4 },        { kCtxA, 0x600 },      { kCtxA + 0x2D0, 0x40 },
        { kCtxB, 0x600 },      { kCtxB + 0x2D0, 0x40 }, { kBlkA, 0x100 },    { kBlkB, 0x100 },
        { 0x800C3E20u, 0x20 }, { 0x800C3230u, 0x30 }, { 0x800C48E8u, 0x50 }, { 0x800D2D20u, 0x40 },
        { 0x800D2E00u, 0x40 }, { 0x800D3010u, 0x10 }, { 0x800D32A0u, 0xB0 }, { 0x800D3660u, 0x20 },
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

/* Menu states the handlers may leave behind: every handled value, the gaps
 * (6, 0x14, 0x1A+), the reset states (0x64, 0x65) and the table edge (0x66). */
static const u8 kStates[] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14,
    0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x63, 0x64, 0x65, 0x66, 0x67, 0xFF,
    1, 2, 3, 4, 5, 0x64, 0x65, 0x10, 0x13
};

static int is_handler(int id) { return id >= S_H8115C && id <= S_H83948; }

static void swap_ctx(void) { p32(kCtxPtr, g32(kCtxPtr) == kCtxA ? kCtxB : kCtxA); }
static void swap_blk(void) { p32(kBlkPtr, g32(kBlkPtr) == kBlkA ? kBlkB : kBlkA); }

/* What the real callees do to the menu state, deterministically from the spy RNG
 * (so the retail and native runs see the same changes). */
static void drive(int id)
{
    u32 ctx, act = sc.actor;
    unsigned late;

    if (id == S_W716D8 && sc.phase == 0) {
        sc.waitCalls++;
        if (sc.swapWait && sc.waitCalls == 1) {
            p32(kBlkPtr, kBlkB);
        }
        if (sc.waitCalls >= sc.waitN) {
            p8(g32(kBlkPtr) + act + 0x90u, 1);
        }
        return;
    }
    if (id == S_AA74) {
        sc.phase = 1;
    }
    if (id == S_FBE0 || id == S_D598 || (id == S_W716D8 && sc.phase == 1)) {
        sc.inLoop = 1;
    }
    ctx = g32(kCtxPtr);
    late = s_nlog >= sc.exitN;
    if (!sc.inLoop) {
        /* before the loop: rarely a callee already satisfies an exit
         * condition (the loop then never runs); mode 5 does it at the last
         * pre-loop call. */
        u32 q = spy_rnd() % 60u;

        if (q == 0) p8(ctx + 0x2DE, spy_rnd() | 1u);
        if (q == 1) p8(0x800C48EAu, spy_rnd() | 1u);
        if (q == 2) p8(ctx + 0x2DB, spy_rnd() | 1u);
        if (id == S_BCD98 && sc.exitMode == 5) {
            u32 w = spy_rnd() % 3u;

            if (w == 0) p8(g32(kCtxPtr) + 0x2DE, spy_rnd() | 1u);
            if (w == 1) p8(0x800C48EAu, spy_rnd() | 1u);
            if (w == 2) p8(g32(kCtxPtr) + 0x2DB, spy_rnd() | 1u);
        }
        if (spy_rnd() % 6u == 0) swap_ctx();
        if ((id == S_C08 || id == S_BC404) && spy_rnd() % 4u == 0) {
            p8(g32(kCtxPtr) + act * 0x40u + 0x3C, (spy_rnd() & 1u) ? 0xFFu : spy_rnd() % 12u);
        }
        return;
    }
    if (!late) {
        if (spy_rnd() % (is_handler(id) ? 2u : 6u) == 0) p8(ctx + 0x2DD, kStates[spy_rnd() % sizeof(kStates)]);
        if (spy_rnd() % 6u == 0) p8(ctx + 0x2E0, (spy_rnd() & 1u) ? 0u : spy_rnd());
        if (spy_rnd() % 3u == 0) p8(ctx + 0x2E3, (spy_rnd() % 3u == 0) ? 0u : spy_rnd());
        if (spy_rnd() % 6u == 0) { u32 v = spy_rnd(); p8(ctx + 0x2D4, v); p8(ctx + 0x2D5, spy_rnd()); }
        if (spy_rnd() % 6u == 0) p8(ctx + 0x2E2, (spy_rnd() & 1u) ? g8(ctx + 0x2DD) : spy_rnd());
        if (spy_rnd() % 8u == 0) swap_ctx();
        if (spy_rnd() % 8u == 0) swap_blk();
        if (spy_rnd() % 10u == 0) p8(kAct32A1 + act * 8u, g8(kAct32A1 + act * 8u) ? 0u : 1u + (spy_rnd() & 0x7Eu));
        if (spy_rnd() % 40u == 0) p8(0x800D3014u, 0xFF);
        else if (spy_rnd() % 8u == 0) p8(0x800D3014u, spy_rnd() & 0x7Fu);
        if (spy_rnd() % 40u == 0) p8(ctx + 0x2DE, spy_rnd() | 1u);
        if (spy_rnd() % 40u == 0) p8(0x800C48EAu, spy_rnd() | 1u);
        if (spy_rnd() % 40u == 0) p8(ctx + 0x2DB, spy_rnd() | 1u);
        return;
    }
    switch (sc.exitMode) {
    case 0: if (is_handler(id)) p8(ctx + 0x2DE, 1u + (spy_rnd() & 0x7Fu)); break;
    case 1: if (is_handler(id)) p8(0x800C48EAu, 1u + (spy_rnd() & 0x7Fu)); break;
    case 2: if (is_handler(id)) p8(ctx + 0x2DB, 1u + (spy_rnd() & 0x7Fu)); break;
    case 3: if (id == S_W716D8) p8(ctx + 0x2DB, 1u + (spy_rnd() & 0x7Fu)); break;
    case 4: if (id == S_D598 || id == S_FBE0 || is_handler(id)) p8(0x800D3014u, 0xFF); break;
    default: break;
    }
    if (s_nlog >= sc.exitN + 24u) {
        p8(ctx + 0x2DE, 1);
    }
}

static u32 spy_do(int id, const u32* raw)
{
    Call* c;
    unsigned i;
    u32 ret = 0;

    if (s_nlog >= kMaxCalls) {
        FAIL("call log overflow (runaway loop?)");
    }
    s_spyCalled[id]++;
    s_callN[id]++;
    c = &s_log[s_nlog++];
    memset(c, 0, sizeof(*c));
    c->id = (u32)id;
    for (i = 0; i < kSpy[id].nargs; ++i) {
        c->a[i] = norm_arg(kSpy[id].kinds[i], raw[i]);
    }
    c->hash = watch_hash();

    switch (id) {
    case S_C08: ret = spy_rnd() & 0xFFFFu; break;
    case S_D598: ret = spy_rnd() & 0xFFu; break;
    case S_H81B58:
    case S_H83948: ret = spy_rnd(); break;
    default: break;
    }
    drive(id);
    return ret;
}

/* ---- native-side spies: the symbols the port TU calls --------------------- */
void func_800716D8(void) { u32 raw[3] = { 0 }; spy_do(S_W716D8, raw); }
void func_8008AA74(u32 v) { u32 raw[3] = { v }; spy_do(S_AA74, raw); }
void func_8007FEC4(void) { u32 raw[3] = { 0 }; spy_do(S_FEC4, raw); }
void func_8007FF14(u32 v) { u32 raw[3] = { v }; spy_do(S_FF14, raw); }
void func_80085E78(void) { u32 raw[3] = { 0 }; spy_do(S_E78, raw); }
void func_8009AB00(u8 v) { u32 raw[3] = { v }; spy_do(S_AB00, raw); }
u16 func_80089C08(u8 v) { u32 raw[3] = { v }; return (u16)spy_do(S_C08, raw); }
void func_800BC404(u32 v) { u32 raw[3] = { v }; spy_do(S_BC404, raw); }
void func_800BCD98(u32 v) { u32 raw[3] = { v }; spy_do(S_BCD98, raw); }
void func_8007FBE0(u8 a, u8 b) { u32 raw[3] = { a, b }; spy_do(S_FBE0, raw); }
s32 func_8008D598(u8 a, u8 b, u8 c) { u32 raw[3] = { a, b, c }; return (s32)spy_do(S_D598, raw); }
void func_800BAF40(u32 a, u32 b) { u32 raw[3] = { a, b }; spy_do(S_BAF40, raw); }
void func_8007FCE8(void) { u32 raw[3] = { 0 }; spy_do(S_FCE8, raw); }
void func_8007FDEC(void) { u32 raw[3] = { 0 }; spy_do(S_FDEC, raw); }
void func_800800E8(u8 v) { u32 raw[3] = { v }; spy_do(S_E8, raw); }
void func_8007FB70(u8 v) { u32 raw[3] = { v }; spy_do(S_FB70, raw); }
#define HANDLER(hi, lo) \
    void func_##hi##lo(u32 v) { u32 raw[3] = { v }; spy_do(S_H##lo, raw); }
HANDLER(800,8115C)
HANDLER(800,81318)
HANDLER(800,81504)
HANDLER(800,816F8)
HANDLER(800,820A4)
HANDLER(800,822C4)
HANDLER(800,82504)
HANDLER(800,82820)
HANDLER(800,829F4)
HANDLER(800,82BB0)
HANDLER(800,82D4C)
HANDLER(800,82F7C)
HANDLER(800,830A8)
HANDLER(800,83340)
HANDLER(800,83580)
HANDLER(800,83748)
s32 func_80081B58(u32 v) { u32 raw[3] = { v }; return (s32)spy_do(S_H81B58, raw); }
s32 func_80083948(u32 v) { u32 raw[3] = { v }; return (s32)spy_do(S_H83948, raw); }

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
    const char* path = getenv("B80160_BATTLE_BIN") ? getenv("B80160_BATTLE_BIN") : "disc/battle.bin";
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
/* Random words into the overlay's data/bss, the two arenas and the g_GameState
 * range (never code or the jump table: battle.bin ends at 0x800C3A70, the batch
 * and its table lie below).  g_GameState is mostly zero so the zero-tests of the
 * Magic/Item branches go both ways. */
static void fill_random(void)
{
    u32 a;

    for (a = 0x800C3A80u; a < 0x800D3C00u; a += 4) p32(a, rnd());
    for (a = kCtxA; a < kBlkB + kArena; a += 4) p32(a, rnd());
    for (a = kGameState; a < kGameStateEnd; ++a) p8(a, (rnd() % 3u == 0) ? rnd() : 0u);
}

static const u32 kKinds[] = { 4, 4, 4, 7, 7, 7, 0, 1, 5, 0xFF };

/* Build the whole guest state for one case; returns the argument word. */
static u32 build(unsigned k)
{
    u32 actor, arg0, i, r;

    memcpy(g_PsxRam, s_snapshot, kRamBytes);
    fill_random();
    memset(&sc, 0, sizeof(sc));
    memset(s_callN, 0, sizeof(s_callN));
    s_spyRng = (u32)(rnd() | 1u);
    memset(s_log, 0, sizeof(s_log));
    s_nlog = 0;

    /* D_800D2D24 is a 4-entry kind array directly below the D_800D2D28
     * pointer: an actor above 3 reads the pointer's bytes (retail does too,
     * the game only passes 0..2), so those are the rare case. */
    r = rnd() % 10u;
    actor = (r < 8) ? rnd() % 3u : (r == 8 ? 3u : 4u + rnd() % 0x12u);
    arg0 = actor;
    r = rnd() % 4u;
    if (r == 1) arg0 |= 0x100u;
    if (r == 2) arg0 |= 0xFFFFFF00u;
    if (r == 3) arg0 |= rnd() & 0xFFFFFF00u;
    sc.actor = actor;
    sc.waitN = 1u + rnd() % 3u;
    sc.swapWait = (rnd() % 4u == 0);
    sc.exitMode = k % 6u;
    sc.exitN = 12u + rnd() % 40u;

    p32(kCtxPtr, kCtxA);
    p32(kBlkPtr, kBlkA);
    /* per-actor state in both arenas' rows and the global tables */
    for (i = 0; i < 2; ++i) {
        u32 ctx = i ? kCtxB : kCtxA, row = ctx + actor * 0x40u;

        /* the exit flags start clear in both arenas (a swap must not end the
         * loop by itself); the driver sets them */
        p8(ctx + 0x2DE, 0);
        p8(ctx + 0x2DB, 0);
        p16(row + 0x2E, (rnd() & 1u) ? 0u : rnd());
        p8(row + 0x3C, (rnd() % 4u == 0) ? 0xFFu : rnd() % 12u);
    }
    if (actor < 4) p8(kAct2D24 + actor, kKinds[rnd() % (sizeof(kKinds) / sizeof(kKinds[0]))]);
    p8(kAct32A1 + actor * 8u, (rnd() & 1u) ? 0u : 1u + (rnd() & 0x7Eu));
    {
        u32 c = rnd() % 4u;
        p8(kAct32A0 + actor * 8u, c == 0 ? rnd() : (c == 1 ? 0xF0u + rnd() % 16u : rnd() % 0x1Eu));
        p8(0x800D32A4u + actor * 8u, (rnd() % 3u == 0) ? 0x20u : rnd() % 0x20u);
    }
    p16(0x800CCD64u + actor * 0x170u, (rnd() & 1u) ? (rnd() | 2u) : (rnd() & ~2u));
    p8(0x800D3014u, (rnd() % 10u == 0) ? 0xFFu : (rnd() & 0x7Fu));
    if (rnd() % 12u == 0) p8(0x800C48EAu, 1);
    else p8(0x800C48EAu, 0);
    /* the g_GameState index bytes of the kind-4 branch */
    p8(kGameState + 0x56B, rnd() % 0x40u);
    p8(kGameState + 0x56E, rnd() % 0x40u);
    p8(kGameState + 0x59C, (rnd() % 10u == 0) ? rnd() : rnd() % 11u);
    for (i = 0; i < 11; ++i) {
        p8(kGameState + 0x97C + i * 0xA4u, rnd() % 0x40u);
        p8(kGameState + 0x97F + i * 0xA4u, rnd() % 0x40u);
    }
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
    Scenario scsave;
    u32 i, saved[32], rngsave, arg0;
    int rc;

    arg0 = build(k);
    memcpy(s_built, g_PsxRam, kRamBytes);
    scsave = sc;
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
    sc = scsave;
    s_spyRng = rngsave;
    memset(s_callN, 0, sizeof(s_callN));
    memset(s_log, 0, sizeof(s_log));
    s_nlog = 0;
    func_80080160(arg0);

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
    unsigned k, missing = 0, covered = 0, id, cases = 6000;
    char name[64];

    if (getenv("B80160_CASES") != NULL) {
        cases = (unsigned)atoi(getenv("B80160_CASES"));
    }
    load_retail();
    memcpy(s_snapshot, g_PsxRam, kRamBytes);
    for (k = 0; k < cases; ++k) {
        s_rng = 0x9E3779B9u + k * 2654435761u + 7u;
        snprintf(name, sizeof(name), "func_80080160 #%u", k);
        if (getenv("B80160_TRACE") != NULL) {
            fprintf(stderr, "%s\n", name);
        }
        run_case(name, k);
    }

    /* Coverage gate: every retail instruction, every spy called. */
    for (k = 0; k < kFnInsns; ++k) {
        if (s_hit[k]) {
            covered++;
        } else {
            fprintf(stderr, "B80160 COVERAGE: instruction 0x%08x never ran\n", kFnAddr + k * 4u);
            missing++;
        }
    }
    for (id = 0; id < S_COUNT; ++id) {
        if (s_spyCalled[id] == 0) {
            fprintf(stderr, "B80160 COVERAGE: spy %s never called\n", kSpy[id].name);
            missing++;
        }
    }
    if (getenv("B80160_SPYCOUNTS") != NULL) {
        for (id = 0; id < S_COUNT; ++id) {
            fprintf(stderr, "spy %-8s called %u\n", kSpy[id].name, s_spyCalled[id]);
        }
    }
    if (missing != 0) {
        FAIL("coverage gate: %u item(s) missing", missing);
    }
    printf("B80160 PORT PASS checks=%u retail instr coverage %u/%u, %u spies\n", s_checks, covered,
           (unsigned)kFnInsns, (unsigned)S_COUNT);
    return 0;
}
