/*
 * Differential certificate for the native battle step driver
 *   func_800B9508 (src/battle/mainc108_port.c)
 * and the two port bodies it reaches through the real code path:
 *   func_800B9284 (mainc108_port.c) and func_800BF0C4 (mainc126.c port arm).
 *
 * CPU side: the retail overlay (disc/battle.bin, loaded whole at 0x8006FAF0) runs
 * the function on the MIPS adapter.  Native side: the port bodies, same RAM, same
 * arguments.  The batch is exactly those three functions.  Every other callee --
 * overlay functions and main-executable services (HeapAlloc/HeapFree/
 * HeapGetNextBlockHeader included) -- is a recording spy on BOTH sides: on the CPU
 * the bridge intercepts the retail call target, natively the spy is linked in place
 * of the callee.  Both run the same logic (spy_do): they log the call id, the
 * arguments (normalised to the callee's declared parameter width) and a hash of the
 * watched guest memory (so store-vs-call ORDER is checked, not just the final
 * memory), apply an optional state perturbation (to catch a body that caches what
 * retail re-reads) and return a controlled value.  func_800BEFF4's spy performs the
 * production side effect (task +0x24 = slot, task +4 = table[slot], returned on the
 * CPU as v0).  After both runs the call logs, all of guest RAM outside the CPU
 * stack window and the CPU's callee-saved registers must match.
 *
 * Coverage gate: every retail instruction of the batch is fetched at least once by
 * a compared case, and every spy is called.  Retail bytes are only read from disc/,
 * never stored in the repo.
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

void func_800B9508(u8* rec);
void func_800B9284(u8* rec, u32 kind);
void func_800BF0C4(u8* obj);

#define FAIL(...)                                    \
    do {                                             \
        fprintf(stderr, "B9508 FAIL: " __VA_ARGS__); \
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

    kRows = 0x800C3EB0u,   /* D_800C3EB0: 28-byte rows, 72-byte steps */
    kRecTbl = 0x800CCB3Cu, /* D_800C3EB0 + 0x8C8C */
    kCurStep = 0x800C360Cu,
    kTaskPtr = 0x800C3610u,
    kWait = 0x800C3614u,
    kGfxBuf = 0x800C3618u,
    kD3350 = 0x800D3350u,

    gTask = 0x80120000u,
    gSprA = 0x80121000u,
    gSprB = 0x80121400u,
    gSprC = 0x80121800u,
    gSprD = 0x80121C00u,
    gBlk = 0x80122000u,
    gBlks = 0x80127000u,  /* per-sprite +0x20 blocks, 0x40 apart */
    gSeven = 0x80128000u  /* per-sprite +0x7C areas, 0x100 apart */
};
static const u32 kSpr[4] = { gSprA, gSprB, gSprC, gSprD };

/* ---- guest RAM helpers --------------------------------------------------- */
static u8* gp(u32 a) { return g_PsxRam + (a & 0x1FFFFFu); }
static u32 g32(u32 a) { u32 v; memcpy(&v, gp(a), 4); return v; }
static u32 g16(u32 a) { u16 v; memcpy(&v, gp(a), 2); return v; }
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

/* ---- the batch ------------------------------------------------------------ */
enum { F_B9508, F_B9284, F_BF0C4, F_COUNT };
static struct { const char* name; u32 addr; unsigned n; unsigned char hit[400]; } s_fn[F_COUNT] = {
    [F_B9508] = { "func_800B9508", 0x800B9508u, 394, { 0 } },
    [F_B9284] = { "func_800B9284", 0x800B9284u, 161, { 0 } },
    [F_BF0C4] = { "func_800BF0C4", 0x800BF0C4u, 74, { 0 } },
};

/* ---- callee spies --------------------------------------------------------- */
/* Argument kinds: w word, h s16, H u16, b u8, p pointer (guest address). */
#define SPIES(X)                                                                  \
    X(EE68, 0x8001EE68u, 1, "w")                                                  \
    X(SetSpecialAnim, 0x80021BF0u, 2, "pw")                                       \
    X(F21BF8, 0x80021BF8u, 2, "pw")                                               \
    X(F21FB8, 0x80021FB8u, 2, "pb")                                               \
    X(F245D8, 0x800245D8u, 2, "ph")                                               \
    X(HeapNext, 0x80031894u, 1, "w")                                              \
    X(HeapAlloc, 0x80031BDCu, 2, "ww")                                            \
    X(HeapFree, 0x800320E8u, 1, "w")                                              \
    X(F79E18, 0x80079E18u, 1, "b")                                                \
    X(F79E4C, 0x80079E4Cu, 1, "b")                                                \
    X(B8048, 0x800B8048u, 1, "w")                                                 \
    X(B8354, 0x800B8354u, 0, "")                                                  \
    X(B9B30, 0x800B9B30u, 0, "")                                                  \
    X(BC404, 0x800BC404u, 1, "w")                                                 \
    X(BC460, 0x800BC460u, 1, "w")                                                 \
    X(BEFF4, 0x800BEFF4u, 1, "w")                                                 \
    X(BF0B4, 0x800BF0B4u, 1, "w")                                                 \
    X(BF1EC, 0x800BF1ECu, 2, "pw")                                                \
    X(BF354, 0x800BF354u, 0, "")                                                  \
    X(BF3E8, 0x800BF3E8u, 1, "p")                                                 \
    X(BF4F0, 0x800BF4F0u, 2, "pp")                                                \
    X(BF600, 0x800BF600u, 2, "wp")                                                \
    X(BF730, 0x800BF730u, 1, "w")                                                 \
    X(C0314, 0x800C0314u, 0, "")

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
    u32 a[5];
    u32 hash;
} Call;
enum { kMaxCalls = 160 };
static Call s_log[kMaxCalls];
static unsigned s_nlog;

typedef struct {
    int id; /* spy id whose nth (1-based) call applies this write; -1 unused */
    unsigned nth;
    u32 addr;
    unsigned width;
    u32 val;
} Mut;
enum { kMaxMut = 6 };

typedef struct {
    Mut mut[kMaxMut];
    u32 ee68;   /* func_8001EE68 result */
    u32 c0314;  /* func_800C0314 result */
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
        { 0x800C3600u, 0x300 }, { 0x800D3300u, 0x100 }, { gTask, 0x100 }, { gSprA, 0x1000 },
        { gBlk, 0x40 },         { gBlks, 0x100 },       { gSeven, 0x400 }, { kRows, 0x200 },
        { kRecTbl, 0x2C },
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
    switch (kind) {
    case 'h': return (u32)(s32)(s16)v;
    case 'H': return v & 0xFFFFu;
    case 'b': return v & 0xFFu;
    default: return v;
    }
}

static u32 spy_do(int id, const u32* raw)
{
    Call* c;
    unsigned i, nth;
    u32 ret = 0;

    if (s_nlog >= kMaxCalls) {
        FAIL("call log overflow (runaway loop?)");
    }
    s_spyCalled[id]++;
    nth = ++s_callN[id];
    c = &s_log[s_nlog++];
    memset(c, 0, sizeof(*c));
    c->id = (u32)id;
    for (i = 0; i < kSpy[id].nargs; ++i) {
        c->a[i] = norm_arg(kSpy[id].kinds[i], raw[i]);
    }
    c->hash = watch_hash();

    /* Perturbations: retail re-reads these; a body that cached them differs. */
    for (i = 0; i < kMaxMut; ++i) {
        if (sc.mut[i].id == id && sc.mut[i].nth == nth) {
            switch (sc.mut[i].width) {
            case 1: p8(sc.mut[i].addr, sc.mut[i].val); break;
            case 2: p16(sc.mut[i].addr, sc.mut[i].val); break;
            default: p32(sc.mut[i].addr, sc.mut[i].val); break;
            }
        }
    }

    switch (id) {
    case S_EE68: ret = sc.ee68; break;
    case S_C0314: ret = sc.c0314; break;
    case S_BF354: ret = spy_rnd(); break;
    case S_HeapNext: ret = spy_rnd() & 0xFFFFu; break;
    case S_HeapAlloc: ret = spy_rnd() | 1u; break;
    case S_BEFF4: {
        /* production func_800BEFF4: task +0x24 = slot, task +4 = table[slot] */
        u32 task = g32(kTaskPtr);

        p32(task + 0x24, raw[0]);
        ret = g32(kRecTbl + raw[0] * 4u);
        p32(task + 4, ret);
        break;
    }
    default: break;
    }
    return ret;
}

/* ---- native-side spies: the symbols the port TUs call --------------------- */
#define NORM(p) BG_ADDR(p)

int func_8001EE68(u8* w) { u32 raw[5] = { NORM(w) }; return (int)spy_do(S_EE68, raw); }
void SpriteSetSpecialAnimFile(void* s, void* f)
{
    u32 raw[5] = { NORM(s), (u32)(uintptr_t)f };
    spy_do(S_SetSpecialAnim, raw);
}
void func_80021BF8(void* p, s32 v) { u32 raw[5] = { NORM(p), (u32)v }; spy_do(S_F21BF8, raw); }
void func_80021FB8(u8* p, u8 v) { u32 raw[5] = { NORM(p), v }; spy_do(S_F21FB8, raw); }
void func_800245D8(void* p, s16 v) { u32 raw[5] = { NORM(p), (u32)(s32)v }; spy_do(S_F245D8, raw); }
void* HeapGetNextBlockHeader(void* p)
{
    u32 raw[5] = { (u32)(uintptr_t)p };
    return (void*)(uintptr_t)spy_do(S_HeapNext, raw);
}
void* HeapAlloc(unsigned size, unsigned flags)
{
    u32 raw[5] = { size, flags };
    return (void*)(uintptr_t)spy_do(S_HeapAlloc, raw);
}
unsigned HeapFree(void* p)
{
    u32 raw[5] = { (u32)(uintptr_t)p };
    return spy_do(S_HeapFree, raw);
}
void func_80079E18(u8 v) { u32 raw[5] = { v }; spy_do(S_F79E18, raw); }
void func_80079E4C(u8 v) { u32 raw[5] = { v }; spy_do(S_F79E4C, raw); }
void func_800B8048(u32 v) { u32 raw[5] = { v }; spy_do(S_B8048, raw); }
void func_800B8354(void) { u32 raw[5] = { 0 }; spy_do(S_B8354, raw); }
void func_800B9B30(void) { u32 raw[5] = { 0 }; spy_do(S_B9B30, raw); }
void func_800BC404(u32 v) { u32 raw[5] = { v }; spy_do(S_BC404, raw); }
void func_800BC460(u32 v) { u32 raw[5] = { v }; spy_do(S_BC460, raw); }
void func_800BEFF4(u32 v) { u32 raw[5] = { v }; spy_do(S_BEFF4, raw); }
void func_800BF0B4(u32 v) { u32 raw[5] = { v }; spy_do(S_BF0B4, raw); }
void func_800BF1EC(u8* p, s32 v) { u32 raw[5] = { NORM(p), (u32)v }; spy_do(S_BF1EC, raw); }
u32 func_800BF354(void) { u32 raw[5] = { 0 }; return spy_do(S_BF354, raw); }
void func_800BF3E8(u8* p) { u32 raw[5] = { NORM(p) }; spy_do(S_BF3E8, raw); }
void func_800BF4F0(u8* a, u8* b) { u32 raw[5] = { NORM(a), NORM(b) }; spy_do(S_BF4F0, raw); }
void func_800BF600(u8* id, u8* p) { u32 raw[5] = { (u32)(uintptr_t)id, NORM(p) }; spy_do(S_BF600, raw); }
void func_800BF730(u32 v) { u32 raw[5] = { v }; spy_do(S_BF730, raw); }
u32 func_800C0314(void) { u32 raw[5] = { 0 }; return spy_do(S_C0314, raw); }

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
    unsigned f;
    int id;

    (void)u;
    for (f = 0; f < F_COUNT; ++f) {
        if (t >= s_fn[f].addr && t < s_fn[f].addr + s_fn[f].n * 4u) {
            if (s_record) {
                s_fn[f].hit[(t - s_fn[f].addr) / 4] = 1;
            }
            return 0; /* batch code runs on the CPU */
        }
    }
    for (id = 0; id < S_COUNT; ++id) {
        if (kSpy[id].addr == t) {
            u32 raw[5];

            raw[0] = c->gpr[4];
            raw[1] = c->gpr[5];
            raw[2] = c->gpr[6];
            raw[3] = c->gpr[7];
            raw[4] = g32(c->gpr[29] + 0x10u); /* fifth argument: stack */
            c->gpr[2] = spy_do(id, raw);
            return 1;
        }
    }
    return -1; /* an unexpected call target fails the case */
}

static u8 s_snapshot[PSX_RAM_SIZE], s_cpuRam[PSX_RAM_SIZE];

static void load_retail(void)
{
    const char* path = getenv("B9508_BATTLE_BIN") ? getenv("B9508_BATTLE_BIN") : "disc/battle.bin";
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
/* Random words into the overlay's data/bss and the object arena (never code:
 * battle.bin ends at 0x800C3A70 and the batch lies below 0x800C0000). */
static void fill_random(void)
{
    u32 a;

    for (a = 0x800C3A80u; a < 0x800CD400u; a += 4) p32(a, rnd());
    for (a = 0x800D3000u; a < 0x800D3700u; a += 4) p32(a, rnd());
    for (a = 0x80120000u; a < 0x80130000u; a += 4) p32(a, rnd());
    for (a = 0x800C3500u; a < 0x800C3A70u; a += 4) p32(a, rnd()); /* data tail of the image */
}

static u32 pick_spr(void) { return kSpr[rnd() & 3u]; }

/* Slot-class bits: +0xA8 bits 30..31 are the low two bits, +0xAC bits 0..1 the
 * high two; the rest random. */
static void set_slot(u32 spr, u32 slot)
{
    p32(spr + 0xA8u, (g32(spr + 0xA8u) & 0x3FFFFFFFu) | ((slot & 3u) << 30));
    p32(spr + 0xACu, (g32(spr + 0xACu) & ~3u) | ((slot >> 2) & 3u));
}

static void clear_scenario(void)
{
    unsigned i;

    memset(&sc, 0, sizeof(sc));
    for (i = 0; i < kMaxMut; ++i) sc.mut[i].id = -1;
}

static void add_mut(int id, unsigned nth, u32 addr, unsigned width, u32 val)
{
    unsigned i;

    for (i = 0; i < kMaxMut; ++i) {
        if (sc.mut[i].id < 0) {
            sc.mut[i].id = id;
            sc.mut[i].nth = nth;
            sc.mut[i].addr = addr;
            sc.mut[i].width = width;
            sc.mut[i].val = val;
            return;
        }
    }
}

typedef struct {
    int fn;
    u32 a0, a1;
} Case;

static const s32 kEdge[] = { 0, 1, -1, 0x4F, 0x50, 0x51, -0x50, 0x7FAF, 0x7FB0, 0x7FFF, -0x8000, -0x7FB0,
                             0x8000, 0xFFFF, 0x7F, 0x80 };
static u32 pick_edge(void) { return (u32)kEdge[rnd() % (sizeof(kEdge) / sizeof(kEdge[0]))]; }

/* A raw host word (a heap/file pointer): below 0x80000000 so it never looks like
 * a guest address, never zero, and outside g_PsxRam (a host pointer into g_PsxRam
 * would be logged as its KSEG0 form by the native spies). */
static u32 raw_word(void)
{
    u32 w;

    do {
        w = 0x10000u + (rnd() & 0x3FFFFFF0u);
    } while (w + 0x10u >= (u32)(uintptr_t)g_PsxRam && w < (u32)(uintptr_t)g_PsxRam + PSX_RAM_SIZE);
    return w;
}

static const u32 kKinds[] = { 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF,
                              0x00, 0x01, 0x05, 0x0F, 0x10, 0x11, 0x20, 0xF0, 0xF1, 0xF2, 0x7F, 0x80 };

/* Build the whole guest state for one case. */
static void build(Case* cs, int fn, unsigned k)
{
    u32 task = gTask, i, step, kind;

    memcpy(g_PsxRam, s_snapshot, kRamBytes);
    clear_scenario();
    fill_random();
    memset(s_callN, 0, sizeof(s_callN));
    s_spyRng = (u32)(rnd() | 1u);
    memset(s_log, 0, sizeof(s_log));
    s_nlog = 0;

    /* task, step, wait counter, graphics buffer, flags */
    p32(kTaskPtr, task);
    p32(task + 4, gSprA);
    p32(task + 0x20, rnd() % 11u);
    p32(task + 0x24, rnd() % 11u);
    p32(task + 0x2C, rnd() % 5u);
    step = rnd() % 6u;
    p32(kCurStep, step);
    p16(kWait, (rnd() % 8u == 0) ? ((rnd() & 1u) ? 0x8000u : 1u + rnd() % 3u) : 0u);
    p32(kGfxBuf, (rnd() & 1u) ? 0u : raw_word());
    p8(kD3350, rnd() & 1u);
    p16(0x800C3626u, rnd());
    for (i = 0; i < 11; ++i) {
        p32(kRecTbl + i * 4u, pick_spr());
    }
    /* sprites: link, block, +0x7C area, +0x24 -> gBlk, slot class, mode, state byte */
    p32(gBlk, (rnd() & 1u) ? raw_word() : (0x80000000u | ((rnd() & 0x1FFFF0u))));
    for (i = 0; i < 4; ++i) {
        u32 s = kSpr[i];

        p32(s + 0x74u, pick_spr());
        p32(s + 0x24u, gBlk);
        p32(s + 0x20u, gBlks + i * 0x40u);
        p32(s + 0x7Cu, gSeven + i * 0x100u);
        if (rnd() % 3u == 0) set_slot(s, rnd() % 4u); else set_slot(s, rnd() & 15u);
        if (rnd() & 1u) p32(s + 0xA8u, g32(s + 0xA8u) & ~0x30000000u);
        p8(s + 0xAFu, rnd());
    }
    for (i = 0; i < 16; ++i) {
        u32 row = kRows + i * 28u;

        p8(row + 7, (rnd() % 3u) != 0 ? 0u : rnd());
        p16(row + 0xE, rnd());
        p16(row + 0x10, rnd());
        if (rnd() & 1u) p8(row + 6, rnd() % 6u);
    }
    sc.ee68 = rnd() & 1u;
    sc.c0314 = (rnd() % 3u == 0) ? (spy_rnd() | 1u) : 0u;
    memset(cs, 0, sizeof(*cs));
    cs->fn = fn;

    switch (fn) {
    case F_B9508: {
        u32 spr = (rnd() & 3u) ? gSprA : pick_spr();
        u32 stepRow = kRows + step * 0x48u;
        u32 slot0 = g32(task + 0x20u);

        kind = kKinds[k % (sizeof(kKinds) / sizeof(kKinds[0]))];
        if (rnd() % 4u == 0) kind = rnd() & 0xFFu;
        p8(stepRow + 0x17Fu, kind);
        p16(stepRow + 0x172u, (kind == 0xFB) ? rnd() % 11u : ((rnd() % 3u == 0) ? 0u : rnd()));
        p16(stepRow + 0x14Eu, rnd());
        if (kind == 0xFB) {
            /* a mid-call step change must land on a valid record index too */
            for (i = 0; i < 6; ++i) p16(kRows + i * 0x48u + 0x172u, rnd() % 11u);
        }
        cs->a0 = spr;
        /* FE: make the spawned record sit on its row's target (or one off) */
        if (kind == 0xFE || kind == 0xFF) {
            u32 rec = g32(kRecTbl + slot0 * 4u), row = kRows + slot0 * 28u;
            u32 x = (rnd() & 1u) ? pick_edge() & 0xFFFFu : rnd() & 0xFFFFu;
            u32 y = (rnd() & 1u) ? pick_edge() & 0xFFFFu : rnd() & 0xFFFFu;

            /* independent x/z so each s16-vs-u16 compare is exercised alone */
            if (rnd() & 1u) p16(rec + 2, x), p16(row + 0xE, x ^ (rnd() % 6u == 0 ? 1u : 0u));
            if (rnd() & 1u) p16(rec + 0xA, y), p16(row + 0x10, y ^ (rnd() % 6u == 0 ? 1u : 0u));
        }
        /* effect ids: put D_800C3648 on, one under and one over the computed id */
        if (kind >= 0x10 && kind != 0xFB && kind != 0xFC && kind != 0xFD && kind != 0xFE && kind != 0xFF &&
            (rnd() % 2u) == 0) {
            u32 slot = ((g32(spr + 0xACu) & 3u) << 2) | (g32(spr + 0xA8u) >> 30);
            u32 cls = *gp(kRows + slot * 28u + 6u);
            s32 id = (s32)kind - 0x10 + (s16)g16(0x800C3630u + cls * 2u);

            p16(0x800C3648u + cls * 2u, (u32)(id + (s32)(rnd() % 3u) - 1));
        }
        /* re-read probes */
        if (rnd() % 3u == 0) add_mut(S_B8048, 1, spr + 0x74u, 4, pick_spr());
        if (rnd() % 3u == 0) add_mut(S_F21BF8, 1, kCurStep, 4, rnd() % 6u);
        if (rnd() % 3u == 0) add_mut(S_F21BF8, 1 + (rnd() & 1u), task + 0x48u, 1, rnd());
        if (rnd() % 3u == 0) add_mut(S_BF354, 1, kGfxBuf, 4, (rnd() & 1u) ? 0u : raw_word());
        if (rnd() % 3u == 0) add_mut(S_SetSpecialAnim, 1, task + 0x20u, 4, rnd() % 16u);
        if (rnd() % 3u == 0) add_mut(S_BF0B4, 1, task + 0x24u, 4, rnd() % 11u);
        if (rnd() % 3u == 0) add_mut(S_F245D8, 1, spr + 0x74u, 4, pick_spr());
        if (rnd() % 3u == 0) add_mut(S_C0314, 1, spr + 0xA8u, 4, rnd());
        if (rnd() % 3u == 0) add_mut(S_BEFF4, 1, task + 0x24u, 4, rnd() % 11u);
        if (rnd() % 4u == 0) add_mut(S_HeapNext, 1, spr + 0x20u, 4, gBlks + 0x100u);
        break;
    }
    case F_BF0C4: {
        u32 idx = rnd() % 5u, rec = kRows + idx * 6u, r = rnd() % 4u;
        u32 spr = (rnd() & 3u) ? gSprA : pick_spr();

        p32(task + 0x2Cu, idx);
        p16(rec + 0xA3Cu, (r & 1u) ? 0xFFFFu : rnd());
        p16(rec + 0xA3Eu, (r & 2u) ? 0xFFFFu : rnd());
        if (rnd() % 3u == 0) { p16(rec + 0xA3Cu, 0xFFFFu); p16(rec + 0xA3Eu, 0xFFFFu); }
        p8(rec + 0xA40u, rnd() & 1u);
        p16(spr + 2u, pick_edge());
        p16(spr + 0xAu, pick_edge());
        cs->a0 = spr;
        if (rnd() % 3u == 0) add_mut(S_BF1EC, 1, task + 0x2Cu, 4, rnd() % 5u);
        break;
    }
    case F_B9284: {
        static const u32 kK[] = { 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xF2, 0xFB, 0xFF, 0x00,
                                  0x100, 0xFFFFFFF3u, 0xF3, 0xF5, 0xFA };
        u32 idx = rnd() % 6u, stepRow = kRows + idx * 0x48u;

        p32(kCurStep, idx);
        p16(stepRow + 0x172u, (rnd() & 1u) ? rnd() : (rnd() & 0xFFFFu) | 0x8000u);
        p16(stepRow + 0x14Eu, rnd());
        p16(0x800C3608u, rnd());
        p16(0x800C3614u, rnd());
        cs->a0 = (rnd() & 3u) ? gSprA : pick_spr();
        cs->a1 = kK[k % (sizeof(kK) / sizeof(kK[0]))];
        if (rnd() % 5u == 0) cs->a1 = rnd() % 0x110u;
        if (rnd() % 3u == 0) add_mut(S_B8048, 1, stepRow + 0x172u, 2, rnd());
        if (rnd() % 3u == 0) add_mut(S_F21BF8, 1, stepRow + 0x172u, 2, rnd());
        if (rnd() % 3u == 0) add_mut(S_BF600, 1, task + 0x48u, 1, rnd());
        break;
    }
    default: break;
    }
}

static void run_native(const Case* cs)
{
    switch (cs->fn) {
    case F_B9508: func_800B9508(PSX_ADDR(cs->a0)); break;
    case F_BF0C4: func_800BF0C4(PSX_ADDR(cs->a0)); break;
    default: func_800B9284(PSX_ADDR(cs->a0), cs->a1); break;
    }
}

static unsigned s_checks, s_cases[F_COUNT];
static Call s_cpuLog[kMaxCalls];
static unsigned s_cpuN;
static u8 s_built[PSX_RAM_SIZE];

static void reset_run(const Scenario* scsave, u32 rngsave)
{
    sc = *scsave;
    s_spyRng = rngsave;
    memset(s_callN, 0, sizeof(s_callN));
    memset(s_log, 0, sizeof(s_log));
    s_nlog = 0;
}

static void run_case(const char* name, int fn, unsigned k)
{
    PcPortMipsCpu cpu;
    PcPortMipsBus bus;
    Case cs;
    Scenario scsave;
    u32 i, saved[32], rngsave;
    int rc;

    if (getenv("B9508_TRACE") != NULL) {
        fprintf(stderr, "%s\n", name);
    }
    build(&cs, fn, k);
    memcpy(s_built, g_PsxRam, kRamBytes);
    scsave = sc;
    rngsave = s_spyRng;

    /* ---- retail CPU ---- */
    reset_run(&scsave, rngsave);
    memset(&bus, 0, sizeof(bus));
    bus.read = bus_read;
    bus.write = bus_write;
    bus.bridge = bus_bridge;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[29] = kStackTop;
    cpu.gpr[31] = kHalt;
    cpu.gpr[4] = cs.a0;
    cpu.gpr[5] = cs.a1;
    for (i = 16; i <= 23; ++i) cpu.gpr[i] = 0x51000000u + i;
    cpu.gpr[30] = 0x5100001Eu;
    memcpy(saved, cpu.gpr, sizeof(saved));
    s_record = 1;
    rc = PcPortMipsRun(&cpu, s_fn[fn].addr, kHalt, kBudget);
    s_record = 0;
    if (rc != PC_PORT_MIPS_HALTED) {
        FAIL("%s: retail CPU rc=%d (%s) a0=%08x a1=%08x", name, rc, cpu.error, cs.a0, cs.a1);
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
    reset_run(&scsave, rngsave);
    run_native(&cs);

    if (s_cpuN != s_nlog) {
        FAIL("%s: callee count retail=%u native=%u (a0=%08x a1=%08x)", name, s_cpuN, s_nlog, cs.a0, cs.a1);
    }
    for (i = 0; i < s_nlog; ++i) {
        const Call* x = &s_cpuLog[i];
        const Call* y = &s_log[i];
        unsigned j;

        if (x->id != y->id) {
            FAIL("%s: call %u retail=%s native=%s (a0=%08x a1=%08x)", name, i, kSpy[x->id].name,
                 kSpy[y->id].name, cs.a0, cs.a1);
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
                    FAIL("%s: RAM 0x%08x retail=%02x native=%02x (a0=%08x a1=%08x)", name, 0x80000000u + i,
                         s_cpuRam[i], g_PsxRam[i], cs.a0, cs.a1);
                }
            }
        }
    }
    s_checks++;
    s_cases[fn]++;
}

int main(void)
{
    static const unsigned kCases[F_COUNT] = { 8000, 2000, 1200 };
    unsigned f, k, missing = 0, covered = 0, total = 0, id;
    char name[64];

    load_retail();
    memcpy(s_snapshot, g_PsxRam, kRamBytes);
    for (f = 0; f < F_COUNT; ++f) {
        for (k = 0; k < kCases[f]; ++k) {
            s_rng = 0x9E3779B9u * (f + 1u) + k * 2654435761u + 7u;
            snprintf(name, sizeof(name), "%s #%u", s_fn[f].name, k);
            run_case(name, (int)f, k);
        }
    }

    /* Coverage gate: every retail instruction of the batch, every spy called. */
    for (f = 0; f < F_COUNT; ++f) {
        for (k = 0; k < s_fn[f].n; ++k) {
            total++;
            if (s_fn[f].hit[k]) {
                covered++;
            } else {
                fprintf(stderr, "B9508 COVERAGE: %s instruction 0x%08x never ran\n", s_fn[f].name,
                        s_fn[f].addr + k * 4u);
                missing++;
            }
        }
    }
    for (id = 0; id < S_COUNT; ++id) {
        if (s_spyCalled[id] == 0) {
            fprintf(stderr, "B9508 COVERAGE: spy %s never called\n", kSpy[id].name);
            missing++;
        }
    }
    if (missing != 0) {
        FAIL("coverage gate: %u item(s) missing", missing);
    }
    printf("B9508 PORT PASS checks=%u (B9508:%u B9284:%u BF0C4:%u) retail instr coverage %u/%u, %u spies\n",
           s_checks, s_cases[0], s_cases[1], s_cases[2], covered, total, (unsigned)S_COUNT);
    return 0;
}
