/*
 * Differential certificate for the native battle command-callback callee ports:
 *   func_800B905C (mainc107_port.c)   func_800BFA9C (mainc132_port.c)
 *   func_800B8EBC (main106_port.c)    func_800B7E94 (mainc100_port.c)
 *   func_800BF4F0 (mainc127_port.c)   func_800B9B54 (mainl109_port.c)
 *   func_800B9284 (mainc108_port.c)
 *
 * CPU side: the retail overlay (disc/battle.bin, loaded whole at 0x8006FAF0) runs
 * the function on the MIPS adapter.  Native side: the port bodies, same RAM, same
 * arguments.  The batch is exactly those seven functions: a call between two of
 * them (905C -> B9B54, 8EBC -> BFA9C) runs real code on both sides.  Every other
 * callee -- overlay functions (incl. func_800BF3E8, whose port body is renamed
 * away from the link) and main-executable services -- is a recording spy on BOTH
 * sides: on the CPU the bridge intercepts the retail call target, natively the spy
 * is linked in place of the callee.  Both run the same logic (spy_do): they log
 * the call id, the arguments (normalised to the callee's declared parameter
 * width) and a hash of the watched guest memory (so store-vs-call ORDER is
 * checked, not just the final memory), apply an optional state perturbation (to
 * catch a body that caches what retail re-reads) and return a controlled value.
 * After both runs the call logs, the return value, all of guest RAM outside the
 * CPU stack window and the CPU's callee-saved registers must match.
 *
 * Main-executable words the port reads by C extern name (D_80059464, the
 * D_8005A474 package) are linked onto the very RAM bytes the CPU reads
 * (-Wl,--defsym), so both sides see one state.
 *
 * Coverage gate: every retail instruction of the seven functions is fetched at
 * least once by a compared case, and every spy is called.
 * Retail bytes are only read from disc/, never stored in the repo.
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

void func_800B905C(void);
void func_800BFA9C(void);
void func_800B8EBC(void);
u32 func_800B7E94(void);
void func_800BF4F0(u8* a, u8* b);
void func_800B9B54(u8* a, u8* b);
void func_800B9284(u8* rec, u32 kind);

#define FAIL(...)                                    \
    do {                                             \
        fprintf(stderr, "CMDCB FAIL: " __VA_ARGS__); \
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

    kRows = 0x800C3EB0u, /* D_800C3EB0 */
    kRecTbl = 0x800CCB3Cu, /* D_800C3EB0 + 0x8C8C */
    kCurStep = 0x800C360Cu,
    kTaskPtr = 0x800C3610u,
    kCurSpr = 0x800C3E1Cu,

    gTask = 0x80120000u,
    gSprA = 0x80121000u,
    gSprB = 0x80121400u,
    gSprC = 0x80121800u,
    gSprD = 0x80121C00u,
    gBlk = 0x80122000u,
    gTable = 0x80123000u,
    gFile = 0x80124000u,
    gNew = 0x80125000u,
    gMgr = 0x80126000u,
    kTaskMark = 0x80199990u
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
enum { F_905C, F_BFA9C, F_8EBC, F_7E94, F_BF4F0, F_B9B54, F_B9284, F_COUNT };
static struct { const char* name; u32 addr; unsigned n; unsigned char hit[200]; } s_fn[F_COUNT] = {
    [F_905C] = { "func_800B905C", 0x800B905Cu, 127, { 0 } },
    [F_BFA9C] = { "func_800BFA9C", 0x800BFA9Cu, 65, { 0 } },
    [F_8EBC] = { "func_800B8EBC", 0x800B8EBCu, 89, { 0 } },
    [F_7E94] = { "func_800B7E94", 0x800B7E94u, 109, { 0 } },
    [F_BF4F0] = { "func_800BF4F0", 0x800BF4F0u, 62, { 0 } },
    [F_B9B54] = { "func_800B9B54", 0x800B9B54u, 43, { 0 } },
    [F_B9284] = { "func_800B9284", 0x800B9284u, 161, { 0 } },
};

/* ---- callee spies --------------------------------------------------------- */
/* Argument kinds: w word, h s16, H u16, b u8, p pointer (guest address). */
#define SPIES(X)                                                                  \
    X(DrawSync, 0x800445D0u, 1, "w")                                              \
    X(SetSpecialAnim, 0x80021BF0u, 2, "pp")                                       \
    X(TWAdd, 0x8001CC18u, 2, "pp")                                                \
    X(TWRemove, 0x8001CD94u, 1, "p")                                              \
    X(EE68, 0x8001EE68u, 1, "w")                                                  \
    X(F21BF8, 0x80021BF8u, 2, "pw")                                               \
    X(F21FE0, 0x80021FE0u, 2, "ph")                                               \
    X(F22224, 0x80022224u, 5, "ppwww")                                            \
    X(F223B0, 0x800223B0u, 2, "ph")                                               \
    X(F23B84, 0x80023B84u, 3, "ppp")                                              \
    X(F245D8, 0x800245D8u, 2, "ph")                                               \
    X(F3A89C, 0x8003A89Cu, 3, "pww")                                              \
    X(F79E18, 0x80079E18u, 1, "b")                                                \
    X(F79E4C, 0x80079E4Cu, 1, "b")                                                \
    X(F80BD0, 0x80080BD0u, 0, "")                                                 \
    X(B8048, 0x800B8048u, 1, "w")                                                 \
    X(B8354, 0x800B8354u, 0, "")                                                  \
    X(B8D04, 0x800B8D04u, 0, "")                                                  \
    X(B8D7C, 0x800B8D7Cu, 0, "")                                                  \
    X(B9508, 0x800B9508u, 1, "p")                                                 \
    X(B9C00, 0x800B9C00u, 2, "pp")                                                \
    X(BA4E0, 0x800BA4E0u, 1, "w")                                                 \
    X(BB760, 0x800BB760u, 1, "w")                                                 \
    X(BC3F8, 0x800BC3F8u, 1, "w")                                                 \
    X(BC404, 0x800BC404u, 1, "w")                                                 \
    X(BC454, 0x800BC454u, 1, "H")                                                 \
    X(BC460, 0x800BC460u, 1, "w")                                                 \
    X(BE0DC, 0x800BE0DCu, 0, "")                                                  \
    X(BE790, 0x800BE790u, 0, "")                                                  \
    X(BEB04, 0x800BEB04u, 0, "")                                                  \
    X(BEDE8, 0x800BEDE8u, 0, "")                                                  \
    X(BEF24, 0x800BEF24u, 2, "pp")                                                \
    X(BEFF4, 0x800BEFF4u, 1, "w")                                                 \
    X(BF0B4, 0x800BF0B4u, 1, "w")                                                 \
    X(BF0C4, 0x800BF0C4u, 1, "p")                                                 \
    X(BF1EC, 0x800BF1ECu, 2, "pw")                                                \
    X(BF3E8, 0x800BF3E8u, 1, "p")                                                 \
    X(BF600, 0x800BF600u, 2, "wp")                                                \
    X(BF720, 0x800BF720u, 0, "")                                                  \
    X(BF730, 0x800BF730u, 1, "w")                                                 \
    X(BF9EC, 0x800BF9ECu, 0, "")                                                  \
    X(C0314, 0x800C0314u, 0, "")                                                  \
    X(C0564, 0x800C0564u, 0, "")                                                  \
    X(C0FAC, 0x800C0FACu, 1, "p")

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
    u32 extra;
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
    u32 ee68;          /* func_8001EE68 result */
    unsigned waits;    /* func_800BF720 differs from D_80059464 this many calls */
    u32 ret_seed;      /* seeds the spies' result stream */
} Scenario;
static Scenario sc;
static unsigned s_callN[S_COUNT];
static u32 s_spyRng;
static int s_native;      /* which side is running (reads the task buffer) */
static u32 s_taskAddr;    /* CPU side: guest address of the stack task */
static void* s_taskHost;  /* native side: the port's task buffer */
static int s_haveTask;

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
        { 0x800C3600u, 0x300 }, { 0x800D3300u, 0x100 }, { 0x800D30A0u, 0x20 },
        { gTask, 0x100 }, { gSprA, 0x1000 }, { 0x80059464u, 4 }, { 0x800594F0u, 4 },
        { 0x8005A474u, 0x20 }, { kRows + 0xA00u, 0x80 }, { kRecTbl, 0x2C },
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

static u32 task_word8(void)
{
    if (!s_haveTask) {
        return 0;
    }
    return s_native ? *(u32*)((u8*)s_taskHost + 8) : g32(s_taskAddr + 8);
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
    c->extra = task_word8();

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
    case S_TWAdd:
        /* the callee records the task and scribbles its +8 so the body's own
         * zeroing store is observable; the task address itself is not
         * comparable (retail: its stack frame; port: a static buffer) */
        s_haveTask = 1;
        c->a[1] = kTaskMark;
        if (s_native) {
            *(u32*)((u8*)s_taskHost + 8) = 0xDEADBEEFu;
        } else {
            s_taskAddr = raw[1];
            p32(s_taskAddr + 8, 0xDEADBEEFu);
        }
        break;
    case S_TWRemove:
        if (!s_native && raw[0] == s_taskAddr) {
            c->a[0] = kTaskMark;
        }
        break;
    case S_EE68: ret = sc.ee68; break;
    case S_BEF24: ret = spy_rnd(); break;
    case S_BF720: {
        u32 word = g32(0x80059464u);

        ret = (nth <= sc.waits) ? (word ^ (1u + (spy_rnd() & 0xFFu))) : word;
        break;
    }
    case S_F23B84: ret = gNew; break;
    case S_C0FAC: ret = spy_rnd() | 1u; break;
    case S_C0314: ret = spy_rnd(); break;
    default: break;
    }
    return ret;
}

/* ---- native-side spies: the symbols the port TUs call --------------------- */
#define NORM(p) BG_ADDR(p)
#define A0 u32 raw[5] = { 0 }
#define SPY0(id) do { A0; return spy_do(id, raw); } while (0)

void func_800B8354(void) { u32 raw[5] = { 0 }; spy_do(S_B8354, raw); }
void func_800B8D04(void) { u32 raw[5] = { 0 }; spy_do(S_B8D04, raw); }
void func_800B8D7C(void) { u32 raw[5] = { 0 }; spy_do(S_B8D7C, raw); }
void func_800BE0DC(void) { u32 raw[5] = { 0 }; spy_do(S_BE0DC, raw); }
void func_800BE790(void) { u32 raw[5] = { 0 }; spy_do(S_BE790, raw); }
void func_800BEB04(void) { u32 raw[5] = { 0 }; spy_do(S_BEB04, raw); }
void func_800BEDE8(void) { u32 raw[5] = { 0 }; spy_do(S_BEDE8, raw); }
void func_800BF9EC(void) { u32 raw[5] = { 0 }; spy_do(S_BF9EC, raw); }
void func_800C0564(void) { u32 raw[5] = { 0 }; spy_do(S_C0564, raw); }
void func_80080BD0(void) { u32 raw[5] = { 0 }; spy_do(S_F80BD0, raw); }
u32 func_800BF720(void) { u32 raw[5] = { 0 }; return spy_do(S_BF720, raw); }
u32 func_800C0314(void) { u32 raw[5] = { 0 }; return spy_do(S_C0314, raw); }

int DrawSync(int m) { u32 raw[5] = { (u32)m }; return (int)spy_do(S_DrawSync, raw); }
void SpriteSetSpecialAnimFile(void* s, void* f)
{
    u32 raw[5] = { NORM(s), NORM(f) };
    spy_do(S_SetSpecialAnim, raw);
}
void TimerWorkListAddTask(void* o, void* t)
{
    u32 raw[5] = { NORM(o), kTaskMark };

    s_taskHost = t;
    spy_do(S_TWAdd, raw);
}
void TimerWorkListRemoveTask(void* t)
{
    u32 raw[5] = { t == s_taskHost ? kTaskMark : NORM(t) };

    spy_do(S_TWRemove, raw);
}
int func_8001EE68(u8* w) { u32 raw[5] = { (u32)(uintptr_t)w }; return (int)spy_do(S_EE68, raw); }
void func_80021BF8(void* p, s32 v) { u32 raw[5] = { NORM(p), (u32)v }; spy_do(S_F21BF8, raw); }
void func_80021FE0(u8* p, s16 v) { u32 raw[5] = { NORM(p), (u32)(s32)v }; spy_do(S_F21FE0, raw); }
typedef struct {
    s16 x;
    s16 y;
} B7Pair;
void func_80022224(void* pkg, void* file, B7Pair tex, B7Pair clut, u32 a4)
{
    u32 raw[5] = { NORM(pkg), NORM(file), (u32)(u16)tex.x | ((u32)(u16)tex.y << 16),
                   (u32)(u16)clut.x | ((u32)(u16)clut.y << 16), a4 };
    spy_do(S_F22224, raw);
}
void func_800223B0(void* p, s16 v) { u32 raw[5] = { NORM(p), (u32)(s32)v }; spy_do(S_F223B0, raw); }
void* func_80023B84(void* s, void* script, void* pkg)
{
    u32 raw[5] = { NORM(s), NORM(script), NORM(pkg) };
    return bg_ptr(spy_do(S_F23B84, raw));
}
void func_800245D8(void* p, s16 v) { u32 raw[5] = { NORM(p), (u32)(s32)v }; spy_do(S_F245D8, raw); }
void func_8003A89C(void* m, s32 a, s32 b) { u32 raw[5] = { NORM(m), (u32)a, (u32)b }; spy_do(S_F3A89C, raw); }
void func_80079E18(u8 v) { u32 raw[5] = { v }; spy_do(S_F79E18, raw); }
void func_80079E4C(u8 v) { u32 raw[5] = { v }; spy_do(S_F79E4C, raw); }
void func_800B8048(u32 v) { u32 raw[5] = { v }; spy_do(S_B8048, raw); }
void func_800B9508(u8* p) { u32 raw[5] = { NORM(p) }; spy_do(S_B9508, raw); }
void func_800B9C00(u8* a, u8* b) { u32 raw[5] = { NORM(a), NORM(b) }; spy_do(S_B9C00, raw); }
void func_800BA4E0(s32 v) { u32 raw[5] = { (u32)v }; spy_do(S_BA4E0, raw); }
void func_800BB760(u32 v) { u32 raw[5] = { v }; spy_do(S_BB760, raw); }
void func_800BC3F8(u32 v) { u32 raw[5] = { v }; spy_do(S_BC3F8, raw); }
void func_800BC404(u32 v) { u32 raw[5] = { v }; spy_do(S_BC404, raw); }
void func_800BC454(u16 v) { u32 raw[5] = { v }; spy_do(S_BC454, raw); }
void func_800BC460(u32 v) { u32 raw[5] = { v }; spy_do(S_BC460, raw); }
u32 func_800BEF24(u8* a, u8* b) { u32 raw[5] = { NORM(a), NORM(b) }; return spy_do(S_BEF24, raw); }
void func_800BEFF4(u32 v) { u32 raw[5] = { v }; spy_do(S_BEFF4, raw); }
void func_800BF0B4(u32 v) { u32 raw[5] = { v }; spy_do(S_BF0B4, raw); }
void func_800BF0C4(u8* p) { u32 raw[5] = { NORM(p) }; spy_do(S_BF0C4, raw); }
void func_800BF1EC(u8* p, s32 v) { u32 raw[5] = { NORM(p), (u32)v }; spy_do(S_BF1EC, raw); }
void func_800BF3E8(u8* p) { u32 raw[5] = { NORM(p) }; spy_do(S_BF3E8, raw); }
void func_800BF600(u8* id, u8* p) { u32 raw[5] = { (u32)(uintptr_t)id, NORM(p) }; spy_do(S_BF600, raw); }
void func_800BF730(u32 v) { u32 raw[5] = { v }; spy_do(S_BF730, raw); }
u32 func_800C0FAC(u8* p) { u32 raw[5] = { NORM(p) }; return spy_do(S_C0FAC, raw); }

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
    const char* path = getenv("CMDCB_BATTLE_BIN") ? getenv("CMDCB_BATTLE_BIN") : "disc/battle.bin";
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
    for (a = 0x80059400u; a < 0x80059500u; a += 4) p32(a, rnd());
    for (a = 0x8005A460u; a < 0x8005A4A0u; a += 4) p32(a, rnd());
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
    int fn;      /* F_* */
    u32 a0, a1;  /* arguments (guest) */
} Case;

static const s32 kEdge[] = { 0, 1, -1, 0x4F, 0x50, 0x51, -0x50, 0x7FAF, 0x7FB0, 0x7FFF, -0x8000, -0x7FB0,
                             0x8000, 0xFFFF, 0x7F, 0x80 };
static u32 pick_edge(void) { return (u32)kEdge[rnd() % (sizeof(kEdge) / sizeof(kEdge[0]))]; }

/* Build the whole guest state for one case. */
static void build(Case* cs, int fn, unsigned k)
{
    u32 task = gTask, i;

    memcpy(g_PsxRam, s_snapshot, kRamBytes);
    clear_scenario();
    fill_random();
    memset(s_callN, 0, sizeof(s_callN));
    s_haveTask = 0;
    s_taskHost = NULL;
    s_spyRng = (u32)(rnd() | 1u);
    memset(s_log, 0, sizeof(s_log));
    s_nlog = 0;

    /* common: task, current step, current sprite, records */
    p32(kTaskPtr, task);
    p32(task + 4, gSprA);
    p32(kCurStep, rnd() % 6u);
    p32(kCurSpr, (rnd() & 7u) ? gSprA : pick_spr());
    for (i = 0; i < 11; ++i) {
        p32(kRecTbl + i * 4u, (rnd() % 5u) ? pick_spr() : 0u);
    }
    /* sprite links */
    for (i = 0; i < 4; ++i) {
        u32 s = kSpr[i];

        p32(s + 0x74u, pick_spr());
        p32(s + 0x24u, gBlk);
        if (rnd() % 3u == 0) set_slot(s, rnd() % 4u); else set_slot(s, rnd() & 15u);
        p8(s + 0xAFu, (rnd() & 3u) ? rnd() : 0x15u);
    }
    memset(cs, 0, sizeof(*cs));
    cs->fn = fn;

    switch (fn) {
    case F_B9B54: {
        u32 a = pick_spr(), b = (rnd() % 6u) ? pick_spr() : a;

        if (rnd() & 1u) p8(b + 0xAFu, 0x15);
        cs->a0 = a;
        cs->a1 = b;
        if (rnd() % 3u == 0) add_mut(S_BEF24, 1 + rnd() % 3u, b + 0xAFu, 1, (rnd() & 1u) ? 0x15u : 0u);
        if (rnd() % 3u == 0) add_mut(S_BEF24, 1 + rnd() % 4u, a + 0xAFu, 1, 0x15u);
        break;
    }
    case F_905C: {
        u32 s = gSprA, t = (rnd() % 8u) ? pick_spr() : gSprA;
        u32 hx = pick_edge() & 0xFFFFu, hz = pick_edge() & 0xFFFFu, x, z, slot;
        u32 row;

        p32(task + 4, s);
        p32(s + 0x74u, t);
        p32(kCurStep, rnd() % 6u);
        p8(kRows + 0xA78u, (rnd() % 5u) != 0);
        /* t's slot row home position */
        slot = ((g32(t + 0xACu) & 3u) << 2) | (g32(t + 0xA8u) >> 30);
        row = kRows + slot * 28u;
        p16(row + 0xEu, hx);
        p16(row + 0x10u, hz);
        /* positions are 16.16 words: t.x/z and s.x */
        x = (rnd() & 1u) ? hx : pick_edge();
        z = (rnd() & 1u) ? hz : pick_edge();
        if (t != s) {
            p32(t + 0, (x << 16) | (rnd() & 0xFFFFu));
            p32(t + 8, (z << 16) | (rnd() & 0xFFFFu));
        }
        if (rnd() & 1u) p32(s + 0, (pick_edge() << 16) | (rnd() & 0xFFFFu));
        if (rnd() % 3u == 0) {
            u32 sx = (u32)(s32)((s32)(s16)x + (s32)(rnd() % 3u) - 1);

            p32(s + 0, (sx << 16) | (rnd() & 0xFFFFu));
        }
        /* a vector pair to copy */
        for (i = 0; i < 16; i += 2) p16(0x800D30A0u + i, rnd());
        /* re-read probes */
        if (rnd() % 3u == 0) add_mut(S_BF3E8, 1, kRows + 0xA78u, 1, rnd() & 1u);
        if (rnd() % 3u == 0) add_mut(S_BEF24, 1 + rnd() % 3u, s + 0x74u, 4, pick_spr());
        if (rnd() % 3u == 0) add_mut(S_BC460, 1, t + 0, 4, (pick_edge() << 16) | 0x1234u);
        if (rnd() % 3u == 0) add_mut(S_BC460, 1, t + 8, 4, (pick_edge() << 16) | 0x4321u);
        if (rnd() % 4u == 0) add_mut(S_BC460, 1 + (rnd() & 1u), s + 0xACu, 4, rnd());
        break;
    }
    case F_BFA9C: {
        u32 mode = (rnd() & 3u) == 0 ? 0u : ((rnd() & 1u) ? 2u : 1u + (rnd() & 7u));
        unsigned n_wait;

        p8(0x800C362Cu, mode);
        for (i = 0; i < 3; ++i) {
            u32 r = kRows + i * 28u;

            p8(r + 8, (rnd() % 5u) != 0 ? 1u + (rnd() % 3u) : 0u);
            p8(r + 6, (rnd() & 1u) ? 0x0Fu + (rnd() % 4u) : rnd());
        }
        p32(kCurSpr, kSpr[rnd() & 3u]);
        set_slot(g32(kCurSpr), rnd() % 4u);
        n_wait = rnd() % 4u;
        p32(0x800C35D8u, (rnd() % 4u) ? 1u + (rnd() & 0xFFu) : 0u);
        if (g32(0x800C35D8u) != 0) {
            add_mut(S_BE790, 2 + n_wait, 0x800C35D8u, 4, 0);
        }
        /* a slot's callee flips the mode / later rows */
        if (rnd() % 3u == 0) add_mut(S_BB760, 1, 0x800C362Cu, 1, rnd() % 4u);
        if (rnd() % 3u == 0) add_mut(S_BB760, 1, kRows + 28u + 8, 1, rnd() & 1u);
        if (rnd() % 3u == 0) add_mut(S_BB760, 1, kRows + 28u + 6, 1, rnd());
        break;
    }
    case F_8EBC: {
        u32 mode = rnd() & 3u;

        sc.waits = rnd() % 4u;
        if (k % 8u == 0) {
            p32(kTaskPtr, 0);
        } else {
            p32(task + 4, pick_spr());
        }
        p32(task + 0x40u, rnd());
        p32(0x80059464u, rnd() % 6u);
        p8(0x800C35D4u, rnd() & 1u);
        p32(0x800C3E54u, (rnd() & 1u) ? gMgr : 0x4000u + (rnd() & 0xFFFFu));
        p8(0x800C362Cu, (rnd() & 1u) ? mode : 0u);
        for (i = 0; i < 3; ++i) {
            u32 r = kRows + i * 28u;

            p8(r + 8, (rnd() % 4u) != 0);
            p8(r + 6, (rnd() & 1u) ? 0x0Fu + (rnd() % 4u) : rnd());
        }
        p32(0x800C35D8u, (rnd() & 1u) ? 3u : 0u);
        if (g32(0x800C35D8u) != 0) {
            add_mut(S_BE790, 3 + rnd() % 5u, 0x800C35D8u, 4, 0);
        }
        /* task +0x40 is read after func_80080BD0 / DrawSync */
        if (rnd() % 3u == 0) add_mut(S_F80BD0, 1, task + 0x40u, 4, rnd());
        if (rnd() % 3u == 0) add_mut(S_DrawSync, 1, task + 0x40u, 4, rnd());
        if (rnd() % 3u == 0) add_mut(S_F80BD0, 1, 0x800C3624u, 1, 0xA5);
        if (rnd() % 3u == 0) add_mut(S_BF9EC, 1, 0x800C35D4u, 1, rnd() & 1u);
        if (rnd() % 3u == 0) add_mut(S_BEDE8, 1, 0x800C35D4u, 1, rnd() & 1u);
        /* record flags read once each, after the spies */
        if (rnd() % 3u == 0) add_mut(S_BA4E0, 1, kRecTbl, 4, pick_spr());
        break;
    }
    case F_7E94: {
        u32 pkg = 0x8005A474u;

        p32(kCurSpr, (rnd() & 3u) ? gSprA : pick_spr());
        p32(0x800594F0u, (rnd() & 7u) ? gFile : 0x40000u + (rnd() & 0xFFu));
        p32(pkg, rnd());
        p32(pkg + 0x10u, gTable);
        for (i = 0; i < 16; ++i) p16(gTable + i * 2u, rnd() & 0xFFFu);
        p16(0x800C3DF0u, (rnd() & 1u) ? rnd() % 6u : rnd());
        if ((s16)g16(0x800C3DF0u) < -8 || (s16)g16(0x800C3DF0u) > 7) p16(0x800C3DF0u, rnd() % 6u);
        p32(0x800C3E54u, (rnd() & 1u) ? gMgr : 0x4000u + (rnd() & 0xFFF0u));
        sc.ee68 = (rnd() & 1u) ? 1u + (rnd() & 0xFFu) : 0u;
        p32(0x800C3E1Cu, g32(kCurSpr));
        /* the body zeroes the callee's scribble */
        if (rnd() % 3u == 0) add_mut(S_BEB04, 1, kCurSpr, 4, pick_spr());
        if (rnd() % 3u == 0) add_mut(S_EE68, 1, kCurSpr, 4, pick_spr());
        if (rnd() % 3u == 0) add_mut(S_EE68, 1, 0x800C3DF0u, 2, rnd() % 6u);
        if (rnd() % 3u == 0) add_mut(S_F23B84, 1, kCurSpr, 4, pick_spr());
        if (rnd() % 4u == 0) add_mut(S_F22224, 1, 0x800594F0u, 4, gFile + 0x10u);
        break;
    }
    case F_BF4F0: {
        u32 a = gSprA, b = (rnd() % 6u) ? gSprB : gSprC;
        u32 idx = rnd() % 5u;
        u32 row = kRows + idx * 6u;
        u32 r = rnd() % 4u;
        s32 bx = (s32)(s16)pick_edge();

        p32(task + 0x2Cu, idx);
        p16(row + 0xA3Cu, (r & 1u) ? 0xFFFFu : rnd());
        p16(row + 0xA3Eu, (r & 2u) ? 0xFFFFu : rnd());
        if ((r & 3u) == 3u || rnd() % 4u == 0) {
            p16(row + 0xA3Cu, 0xFFFFu);
            p16(row + 0xA3Eu, 0xFFFFu);
        }
        p32(b + 0, ((u32)bx << 16) | (rnd() & 0xFFFFu));
        p32(b + 8, (rnd() << 16) | (rnd() & 0xFFFFu));
        p16(a + 0xA0u, rnd());
        p16(a + 0xA4u, rnd());
        p16(a + 2u, rnd());
        p16(a + 0xAu, rnd());
        /* make the target land where `a` already stands */
        if (rnd() % 2u) {
            u32 old0 = g16(a + 0xA0u);

            switch (rnd() % 3u) {
            case 0: bx = (s32)(s16)old0 + 0x50; break;
            case 1: bx = (s32)(s16)old0 - 0x50; break;
            default: bx = (s32)(s16)old0; break; /* a+2 == bx: the `<` boundary */
            }
            p32(b + 0, ((u32)bx << 16) | (rnd() & 0xFFFFu));
            if (rnd() & 1u) {
                p32(b + 8, ((u32)g16(a + 0xA4u) << 16) | (rnd() & 0xFFFFu));
            }
        }
        cs->a0 = a;
        cs->a1 = b;
        break;
    }
    case F_B9284: {
        static const u32 kKinds[] = { 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xF2, 0xFB, 0xFF, 0x00,
                                      0x100, 0xFFFFFFF3u, 0xF3, 0xF5, 0xFA };
        u32 idx = rnd() % 6u, step = kRows + idx * 0x48u;

        p32(kCurStep, idx);
        p16(step + 0x172u, (rnd() & 1u) ? rnd() : (rnd() & 0xFFFFu) | 0x8000u);
        p16(step + 0x14Eu, rnd());
        p16(0x800C3608u, rnd());
        p16(0x800C3614u, rnd());
        cs->a0 = (rnd() & 3u) ? gSprA : pick_spr();
        cs->a1 = kKinds[k % (sizeof(kKinds) / sizeof(kKinds[0]))];
        if (rnd() % 5u == 0) cs->a1 = rnd() % 0x110u;
        if (rnd() % 3u == 0) add_mut(S_B8048, 1, step + 0x172u, 2, rnd());
        if (rnd() % 3u == 0) add_mut(S_F21BF8, 1, step + 0x172u, 2, rnd());
        if (rnd() % 3u == 0) add_mut(S_BF600, 1, task + 0x48u, 1, rnd());
        break;
    }
    default: break;
    }
}

static void run_native(const Case* cs, u32* ret)
{
    *ret = 0;
    switch (cs->fn) {
    case F_905C: func_800B905C(); break;
    case F_BFA9C: func_800BFA9C(); break;
    case F_8EBC: func_800B8EBC(); break;
    case F_7E94: *ret = func_800B7E94() & 0xFFu; break;
    case F_BF4F0: func_800BF4F0(PSX_ADDR(cs->a0), PSX_ADDR(cs->a1)); break;
    case F_B9B54: func_800B9B54(PSX_ADDR(cs->a0), PSX_ADDR(cs->a1)); break;
    default: func_800B9284(PSX_ADDR(cs->a0), cs->a1); break;
    }
}

static unsigned s_checks, s_cases[F_COUNT];
static Call s_cpuLog[kMaxCalls];
static unsigned s_cpuN;
static u8 s_built[PSX_RAM_SIZE];

static void reset_run(const Scenario* scsave, u32 rngsave, int native)
{
    sc = *scsave;
    s_spyRng = rngsave;
    memset(s_callN, 0, sizeof(s_callN));
    memset(s_log, 0, sizeof(s_log));
    s_nlog = 0;
    s_haveTask = 0;
    s_native = native;
}

static void run_case(const char* name, int fn, unsigned k)
{
    PcPortMipsCpu cpu;
    PcPortMipsBus bus;
    Case cs;
    Scenario scsave;
    u32 i, saved[32], ret_cpu, ret_nat, rngsave;
    int rc;

    build(&cs, fn, k);
    memcpy(s_built, g_PsxRam, kRamBytes);
    scsave = sc;
    rngsave = s_spyRng;

    /* ---- retail CPU ---- */
    reset_run(&scsave, rngsave, 0);
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
    ret_cpu = fn == F_7E94 ? (cpu.gpr[2] & 0xFFu) : 0;
    memcpy(s_cpuRam, g_PsxRam, kRamBytes);
    memcpy(s_cpuLog, s_log, sizeof(s_log));
    s_cpuN = s_nlog;

    /* ---- native port body, same initial state ---- */
    memcpy(g_PsxRam, s_built, kRamBytes);
    reset_run(&scsave, rngsave, 1);
    run_native(&cs, &ret_nat);

    if (ret_cpu != ret_nat) {
        FAIL("%s: return retail=%u native=%u", name, ret_cpu, ret_nat);
    }
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
        if (x->extra != y->extra) {
            FAIL("%s: call %u %s: task word retail=%08x native=%08x", name, i, kSpy[x->id].name, x->extra,
                 y->extra);
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
    (void)k;
}

int main(void)
{
    static const unsigned kCases[F_COUNT] = { 2500, 1200, 2000, 2500, 2500, 800, 2500 };
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
                fprintf(stderr, "CMDCB COVERAGE: %s instruction 0x%08x never ran\n", s_fn[f].name,
                        s_fn[f].addr + k * 4u);
                missing++;
            }
        }
    }
    for (id = 0; id < S_COUNT; ++id) {
        if (s_spyCalled[id] == 0) {
            fprintf(stderr, "CMDCB COVERAGE: spy %s never called\n", kSpy[id].name);
            missing++;
        }
    }
    if (missing != 0) {
        FAIL("coverage gate: %u item(s) missing", missing);
    }
    printf("CMDCB PORT PASS checks=%u (905C:%u BFA9C:%u 8EBC:%u 7E94:%u BF4F0:%u B9B54:%u B9284:%u) "
           "retail instr coverage %u/%u, %u spies\n",
           s_checks, s_cases[0], s_cases[1], s_cases[2], s_cases[3], s_cases[4], s_cases[5], s_cases[6],
           covered, total, (unsigned)S_COUNT);
    return 0;
}
