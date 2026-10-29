/* Retail differential for the hand-written XENO_PC_PORT arms of src/movie/main.c
 * (movie.bin, game state 6) against the pinned disc/movie.bin (+ SLUS_006.64 for
 * the real libgpu leaves the retail code calls) on the port MIPS interpreter
 * (PcPortMipsRun = TEST ORACLE ONLY; production stays native C).
 *
 * Both sides start from byte-identical guest memory (g_PsxRam with the SLUS at
 * 0x80010000 and movie.bin at 0x8006FAF0, read from the user's disc at run time
 * only; no retail bytes live in the tree), identical overlay/SLUS globals
 * (native variable <-> guest word, see Glob in slus_oracle.h), identical fixture
 * window, and call identical callee spies.  Every callee that leaves the movie
 * overlay (archive/CD/PC-HDD I/O, libgpu/libetc drawing and sync, the movie
 * player module, FontPrintf, heap, controller, sound) is a spy: retail jal's
 * are serviced by a bridge, native calls by a same-named stub; argument values
 * and call order are compared, results are scripted identically.  Callees that
 * live inside the TU run as real code on both sides (func_80074AF0 RNG,
 * func_800747AC pad, func_800768D8 frame callback ...).
 *
 * MovieRenderEnv: the native struct holds 8-byte u_long ot[] slots, so its host
 * layout differs from retail's 0x138 stride.  The native array lives outside
 * guest RAM; before a case it is serialised into the retail layout in guest RAM
 * and after both runs the native env is serialised again and compared with the
 * retail guest bytes (generated layout: movie_retail_layout.h).
 *
 * usage: movie_retail_test <group> [fn-name]     groups: leaf sector play main fat */
/* the test image (g_PsxRam + snapshots + traces) is larger than the default fixture base allows (UBSan) */
#define LOW_BASE 0x08000000u
#include "slus_oracle.h"
#include "movie_retail_layout.h"
#include <setjmp.h>

#ifndef GRP
#error GRP must be set (1 leaf, 2 sector, 3 play, 4 main, 5 fat)
#endif

#define MOVIE_BASE 0x8006FAF0u
#define MOVIE_SIZE 0x7453u
#define RENV 0x80077124u
#define RENV_STRIDE 0x138u
#define OV_LO 0x80010000u
#define OV_HI 0x80080000u
#define STK_LO 0x801E0000u
#define NENV (MVH_ENV_SIZE)

/* ------------------------------------------------------------------------- */
/* native symbols under test / globals                                       */
/* ------------------------------------------------------------------------- */
#if GRP == 1
extern void func_80072D84(void*, void*, s16, s16, s32, s32);
extern void func_80072F98(void*, void*, s16, s16, s32, s32);
extern void func_80073328(void*, void*, s16, s16, s32, s32);
extern void func_800734B8(void*, void*, s16, s16, s32, s32);
#endif
extern s32 func_80074AF0(void);
extern void func_800768D8(u16, u16, u16);
#if GRP == 2
extern s32 func_80074BA4(s32);
extern s32 func_8007519C(void);
#endif
#if GRP == 3
extern void func_8007625C(void);
extern void func_800763BC(u8);
extern s32 func_80076488(void);
extern void func_800769A4(void);
#endif
#if GRP == 4
extern void MovieMain(void);
extern s32 func_800747AC(s32, u32, s32*);
#endif
#if GRP == 5
extern s32 func_80075D4C(s32);
extern void func_80075D8C(void);
#endif

/* overlay globals defined by the TU (.data / .bss) */
extern s32 D_80076EEC, D_80076EF0, D_80076EF4, D_80076EF8, D_80076F04, D_80076F08;
extern u8 D_80076F8C[4][4], D_80076F9C[4][4], D_80076FCC[4][4], D_80076FDC[4][4];
extern s32 D_80076FAC[4], D_80076FBC[4], D_80076FEC[4], D_80076FFC[4];
extern s32 D_8007700C, D_80077010, D_80077014, D_80077018, D_8007701C, D_80077020, D_80077024, D_80077028;
extern u32 D_80077118;
extern s32 D_8007711C;
extern void* D_80077120;
extern u8 D_80077124[];
extern s32 D_80077394, D_80077398, D_8007739C, D_800773A0, D_800773A4, D_800773A8, D_800773AC, D_800773B0,
    D_800773B4;
extern s32 D_800773B8[16][2];
extern s32 D_80077438, D_8007743C, D_80077440, D_80077444, D_80077448, D_8007744C, D_80077450, D_80077454;

/* SLUS / player-module globals the TU only declares */
u8 D_8004FE44, D_8004FE45, D_8004FE46, D_8004FE47;
s32 D_8005A49C, D_8005A4A4, D_8005A4DC;
u32 D_8005A4A8, D_8005A4B4;
s16 D_8005A4B8;
u16 D_80062514;
s32 D_801D68B4, D_801E89D4;
u32 g_ArchiveTable;

/* link-only: the TU's heap/archive helpers are never reached (all spied) */
void func_80028F30(void) { abort(); }

/* ------------------------------------------------------------------------- */
/* per-case script                                                           */
/* ------------------------------------------------------------------------- */
#define MAXEV 6000
#define MAXV 14
typedef struct { uint32_t id, n, v[MAXV]; } MvEv;
static MvEv ev[2][MAXEV];
static unsigned nev[2];
static int side; /* 0 retail, 1 native */

#define SPIES(X) \
    X(ArchiveSetIndex, 0x80028470, "ii") \
    X(ArchiveDecodeSizeAbsolute, 0x80028928, "i") \
    X(ArchiveGetFilePath, 0x80028998, "i") \
    X(func_8002C3D8, 0x8002C3D8, "") \
    X(PCopen, 0x8004C318, "pii") \
    X(PClseek, 0x8004C348, "iii") \
    X(PCread, 0x8004C398, "ipi") \
    X(PCclose, 0x8004C338, "i") \
    X(ArchiveDecodeSector, 0x800289D0, "i") \
    X(ArchiveReadFileFromCdSector, 0x8002954C, "ipiii") \
    X(ArchiveCdDataSync, 0x80028A60, "i") \
    X(ArchiveDecodeAlignedSize, 0x800288EC, "i") \
    X(ArchiveDecodeSize, 0x80028738, "i") \
    X(SetDispMask, 0x80044534, "i") \
    X(Vsync, 0x8004B54C, "i") \
    X(ClearImage, 0x80044764, "riii") \
    X(DrawSync, 0x800445D0, "i") \
    X(PutDrawEnv, 0x80044C44, "p") \
    X(PutDispEnv, 0x80044E9C, "p") \
    X(GameCheckAndHandleSoftReset, 0x80019CA0, "") \
    X(MoveImage, 0x8004495C, "rii") \
    X(ControllerGetButtonState, 0x8003569C, "i") \
    X(SoundSetCdVolumeWithFade, 0x80038D18, "ii") \
    X(HeapChangeCurrentUser, 0x80032498, "ip") \
    X(HeapAlloc, 0x80031BDC, "ii") \
    X(HeapFree, 0x800320E8, "p") \
    X(ArchiveReadFileToBuffer, 0x800295D8, "ipii") \
    X(SetDefDrawEnv, 0x80043928, "piiii") \
    X(SetDefDispEnv, 0x800439E0, "piiii") \
    X(ChangeGameState, 0x8001996C, "i") \
    X(MainLoop, 0x80019ACC, "i") \
    X(FontLoadFont, 0x800374E8, "iiiiiiiiiii") \
    X(ClearOTagR, 0x80044AD8, "pi") \
    X(ArchiveGetDiscNumber, 0x80028530, "") \
    X(FontPrintf, 0x8003700C, "x") \
    X(FontDrawLetters, 0x80037324, "p") \
    X(HeapDebugDump, 0x8003278C, "iiii") \
    X(DrawOTag, 0x80044BD0, "p") \
    X(ArchiveGetArchiveOffsetIndices, 0x800284B4, "oo") \
    X(func_801D3538, 0x801D3538, "iiiiiii") \
    X(func_801D37CC, 0x801D37CC, "hihhhihhhhhhf") \
    X(func_801D3F7C, 0x801D3F7C, "") \
    X(func_801D4318, 0x801D4318, "") \
    X(func_801D43B0, 0x801D43B0, "") \
    X(func_80072D84, 0x80072D84, "ppiiii") \
    X(func_80072F98, 0x80072F98, "ppiiii") \
    X(func_80073328, 0x80073328, "ppiiii") \
    X(func_800734B8, 0x800734B8, "ppiiii") \
    X(func_80074BA4, 0x80074BA4, "i") \
    X(func_8007519C, 0x8007519C, "") \
    X(func_80075534, 0x80075534, "") \
    X(func_800704E8, 0x800704E8, "") \
    X(func_80072480, 0x80072480, "") \
    X(func_80075D8C, 0x80075D8C, "") \
    X(func_8007625C, 0x8007625C, "") \
    X(func_800763BC, 0x800763BC, "b")

typedef enum {
#define X(n, a, s) S_##n,
    SPIES(X)
#undef X
    S_COUNT
} SpyId;
static const struct { const char* name; uint32_t addr; const char* spec; } spydef[] = {
#define X(n, a, s) { #n, a, s },
    SPIES(X)
#undef X
};
static int spy_active[S_COUNT]; /* retail bridge enabled */

typedef struct { uint32_t v, v2; void* host; } Arg;

static struct {
    uint32_t rv[S_COUNT][8];
    unsigned calls[S_COUNT];
    uint32_t pad[64]; unsigned padn;
    unsigned term_after; /* GameCheckAndHandleSoftReset call count that ends the case */
    int terminated;
    /* disc model */
    unsigned total, ssz, fpos, base, fsize;
    uint16_t st[600], ty[600], si[600], sc[600]; uint32_t fr[600];
    int frame_cb, cb_frame; uint32_t cb_y[2];
    unsigned pump_calls;
    unsigned term_fdl;
} S;

static uintptr_t stk_anchor[2];
static jmp_buf term_jb;

/* ------------------------------------------------------------------------- */
/* pointer normalisation                                                     */
/* ------------------------------------------------------------------------- */
static uint32_t env_member_map(unsigned off)
{
    if (off < 0x70u) return off;
    if (off >= MVH_ot && off < MVH_ot + 32u * MVH_ULONG) return 0x70u + (off - MVH_ot) / MVH_ULONG * 4u;
    if (off >= MVH_box && off < MVH_box + MVH_POLY_SIZE) return 0xF0u + (off - MVH_box);
    if (off >= MVH_frame && off < MVH_frame + MVH_POLY_SIZE) return 0x114u + (off - MVH_frame);
    fprintf(stderr, "ORACLE FAIL native env offset %x\n", off); exit(2);
}
static uint32_t n2v(const void* p)
{
    uintptr_t v = (uintptr_t)p;
    if (!v) return 0;
    if (v >= (uintptr_t)g_PsxRam && v < (uintptr_t)g_PsxRam + 0x200000) return 0x80000000u + (uint32_t)(v - (uintptr_t)g_PsxRam);
    if (v >= LOW_BASE && v < LOW_BASE + LOW_SIZE) return (uint32_t)v;
    if (v >= (uintptr_t)D_80077124 && v < (uintptr_t)D_80077124 + 2u * NENV) {
        unsigned o = (unsigned)(v - (uintptr_t)D_80077124);
        return RENV + o / NENV * RENV_STRIDE + env_member_map(o % NENV);
    }
    if (!stk_anchor[1]) stk_anchor[1] = v;
    return 0xB0000000u + (uint32_t)(v - stk_anchor[1]);
}
static uint32_t g2v(uint32_t a)
{
    if (a >= STK_LO && a < 0x80200000u) {
        if (!stk_anchor[0]) stk_anchor[0] = a;
        return 0xB0000000u + (a - (uint32_t)stk_anchor[0]);
    }
    return a;
}
static void* v2h(uint32_t a) { return a ? gaddr(a, 1) : NULL; } /* scripted guest pointer -> host */

/* ------------------------------------------------------------------------- */
/* spy core                                                                  */
/* ------------------------------------------------------------------------- */
static unsigned count_conv(const char* f, char* kinds)
{
    unsigned n = 0;
    for (; *f; f++) {
        if (*f != '%') continue;
        f++;
        while (*f && (*f == '-' || *f == '+' || *f == ' ' || *f == '0' || (*f >= '1' && *f <= '9') || *f == '.')) f++;
        if (*f == '%') continue;
        if (!*f) break;
        if (n < 8) kinds[n++] = *f == 's' ? 'p' : 'i';
    }
    return n;
}

static int term_requested;
static uint32_t effect(int id, Arg* a, unsigned n);

/* one raw argument -> Arg.  Retail raw is the 32-bit guest register/stack word,
 * native raw is the 64-bit C argument. */
static void decode_arg(char kind, uint64_t raw, Arg* a)
{
    memset(a, 0, sizeof(*a));
    switch (kind) {
    case 'i': a->v = (uint32_t)raw; break;
    case 'h': a->v = (uint32_t)raw & 0xFFFFu; break;
    case 'b': a->v = (uint32_t)raw & 0xFFu; break;
    case 'p':
        a->host = side ? (void*)(uintptr_t)raw : v2h((uint32_t)raw);
        a->v = side ? n2v((void*)(uintptr_t)raw) : g2v((uint32_t)raw);
        break;
    case 'o': a->host = side ? (void*)(uintptr_t)raw : v2h((uint32_t)raw); break;
    case 'f': a->v = side ? (raw == (uint64_t)(uintptr_t)&func_800768D8 ? 0x800768D8u : (uint32_t)raw) : (uint32_t)raw; break;
    case 'r': {
        uint8_t* h = side ? (uint8_t*)(uintptr_t)raw : (uint8_t*)v2h((uint32_t)raw);
        uint32_t w0 = 0, w1 = 0;
        if (h) { memcpy(&w0, h, 4); memcpy(&w1, h + 4, 4); }
        a->v = w0; a->v2 = w1; a->host = h;
        break;
    }
    }
}
static uint32_t spy_run(int id, const uint64_t* raw /* up to 13 */)
{
    const char* sp = spydef[id].spec;
    Arg a[16]; unsigned na = 0, k;
    MvEv* e;
    memset(a, 0, sizeof(a));
    if (nev[side] >= MAXEV) { fprintf(stderr, "ORACLE FAIL event overflow\n"); exit(2); }
    e = &ev[side][nev[side]++];
    memset(e, 0, sizeof(*e));
    e->id = (uint32_t)id;
    if (sp[0] == 'x') { /* FontPrintf: the format string drives the argument count */
        char kinds[8]; unsigned nc;
        decode_arg('p', raw[0], &a[0]); na = 1;
        nc = count_conv((const char*)a[0].host, kinds);
        for (k = 0; k < nc && k < 5; k++) {
            decode_arg(kinds[k], raw[1 + k], &a[na]);
            if (kinds[k] == 'p') decode_arg('p', raw[1 + k], &a[na]);
            na++;
        }
        /* the format is compared by content: the overlay holds several identical literals ("\n", "SET") and the
         * port reads whichever address it names, so the pointer is not behaviour */
        {
            uint32_t h = 2166136261u;
            for (const unsigned char* c = (const unsigned char*)a[0].host; *c; c++) h = (h ^ *c) * 16777619u;
            a[0].v = h;
        }
        for (k = 0; k < na; k++) e->v[e->n++] = a[k].v;
        return effect(id, a, na);
    }
    for (k = 0; sp[k]; k++) { decode_arg(sp[k], raw[k], &a[na]); na++; }
    for (k = 0; k < na; k++) {
        e->v[e->n++] = a[k].v;
        if (sp[k] == 'r') e->v[e->n++] = a[k].v2;
    }
    return effect(id, a, na);
}
static long nat_spy(int id, const long* v)
{
    uint64_t raw[13];
    uint32_t r;
    for (int i = 0; i < 13; i++) raw[i] = (uint64_t)v[i];
    r = spy_run(id, raw);
    if (term_requested) { term_requested = 0; longjmp(term_jb, 1); }
    if (id == S_ArchiveGetFilePath || id == S_HeapAlloc || id == S_ClearOTagR) return (long)(uintptr_t)v2h(r);
    return (long)(int32_t)r;
}
#define STUB(n) \
    long n(long a0, long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8, long a9, long a10, long a11, long a12) \
    { long v[13] = { a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12 }; return nat_spy(S_##n, v); }
STUB(ArchiveSetIndex) STUB(ArchiveDecodeSizeAbsolute) STUB(ArchiveGetFilePath) STUB(func_8002C3D8) STUB(PCopen)
STUB(PClseek) STUB(PCread) STUB(PCclose) STUB(ArchiveDecodeSector) STUB(ArchiveReadFileFromCdSector)
STUB(ArchiveCdDataSync) STUB(ArchiveDecodeAlignedSize) STUB(ArchiveDecodeSize) STUB(SetDispMask) STUB(Vsync)
STUB(ClearImage) STUB(DrawSync) STUB(PutDrawEnv) STUB(PutDispEnv) STUB(GameCheckAndHandleSoftReset) STUB(MoveImage)
STUB(ControllerGetButtonState) STUB(SoundSetCdVolumeWithFade) STUB(HeapChangeCurrentUser) STUB(HeapAlloc)
STUB(HeapFree) STUB(ArchiveReadFileToBuffer) STUB(SetDefDrawEnv) STUB(SetDefDispEnv) STUB(ChangeGameState)
STUB(MainLoop) STUB(FontLoadFont) STUB(ClearOTagR) STUB(ArchiveGetDiscNumber) STUB(FontPrintf)
STUB(FontDrawLetters) STUB(HeapDebugDump) STUB(DrawOTag) STUB(ArchiveGetArchiveOffsetIndices) STUB(func_801D3538)
STUB(func_801D37CC) STUB(func_801D3F7C) STUB(func_801D4318) STUB(func_801D43B0)
/* TU-internal callees that are spies in a group (the runner weakens the TU copy) */
#if GRP == 4 || GRP == 5
STUB(func_80072F98) STUB(func_800734B8)
#endif
#if GRP == 4
STUB(func_80072D84) STUB(func_80073328) STUB(func_80074BA4) STUB(func_8007519C) STUB(func_80075534)
STUB(func_800704E8) STUB(func_80072480) STUB(func_80075D8C) STUB(func_8007625C) STUB(func_800763BC)
#endif

/* libgpu AddPrim: retail inlines addPrim(); the port's libgpu implements the same 24-bit link */
void AddPrim(void* ot, void* p)
{
    uint32_t* o = ot; uint32_t* q = p;
    q[0] = (q[0] & 0xFF000000u) | (o[0] & 0x00FFFFFFu);
    o[0] = (o[0] & 0xFF000000u) | ((uint32_t)(uintptr_t)p & 0x00FFFFFFu);
}

/* ------------------------------------------------------------------------- */
/* retail execution                                                          */
/* ------------------------------------------------------------------------- */
static PcPortMipsCpu* cur_cpu;
static uint32_t cov_lo, cov_hi; static uint32_t* cov;
static int mv_rd(void* u, uint32_t a, unsigned w, uint32_t* v)
{
    if (cov && cur_cpu && w == 4 && a == cur_cpu->pc && a >= cov_lo && a < cov_hi) cov[(a - cov_lo) / 4]++;
    return h_rd(u, a, w, v);
}
static int mv_bridge(void* u, PcPortMipsCpu* c, uint32_t t)
{
    (void)u;
    for (int i = 0; i < S_COUNT; i++) {
        if (spydef[i].addr == t && spy_active[i]) {
            uint64_t raw[13];
            for (unsigned k = 0; k < 13; k++) raw[k] = k < 4 ? c->gpr[4 + k] : (uint64_t)g32(c->gpr[29] + 4 * k);
            c->gpr[2] = spy_run(i, raw);
            if (term_requested) { term_requested = 0; c->gpr[31] = HALT_PC; }
            return 1;
        }
    }
    if (t >= OV_LO && t < 0x80058800u && !(t >= 0x80043BF0u && t < 0x80043D00u)) { /* SetSemiTrans / SetPolyG4 leaves */
        fprintf(stderr, "ORACLE FAIL retail called unlisted SLUS function %08x\n", t); exit(2);
    }
    return 0;
}
static uint32_t mv_retail_call(uint32_t entry, const uint32_t* args, unsigned nargs, uint32_t sp, uint64_t budget, const char* what)
{
    PcPortMipsBus bus = { .read = mv_rd, .write = h_wr, .bridge = mv_bridge };
    PcPortMipsCpu cpu; PcPortMipsCpuInit(&cpu, &bus);
    PcPortMipsCpu* saved = cur_cpu;
    for (unsigned i = 0; i < nargs; i++) {
        if (i < 4) cpu.gpr[4 + i] = args[i]; else put32(g_PsxRam + (sp & 0x1FFFFF) + 4 * i, args[i]);
    }
    cpu.gpr[28] = GP_VALUE; cpu.gpr[29] = sp; cpu.gpr[31] = HALT_PC;
    cur_cpu = &cpu;
    int rc = PcPortMipsRun(&cpu, entry, HALT_PC, budget);
    cur_cpu = saved;
    if (rc != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "%s FAIL retail oracle did not halt (rc=%d) pc=%08x: %s\n", what, rc, cpu.pc, cpu.error);
        exit(2);
    }
    return cpu.gpr[2];
}

/* ------------------------------------------------------------------------- */
/* disc model + callee effects                                               */
/* ------------------------------------------------------------------------- */
static uint32_t hash32(uint32_t x) { x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; x *= 0x846ca68bu; x ^= x >> 16; return x; }
static void model_hdr(unsigned sec, uint8_t* out /* 0x28 bytes */)
{
    uint16_t st, ty, si, sc; uint32_t fr;
    memset(out, 0, 0x28);
    if (sec < S.total) { st = S.st[sec]; ty = S.ty[sec]; si = S.si[sec]; sc = S.sc[sec]; fr = S.fr[sec]; }
    else {
        uint32_t h = hash32(sec * 77u + S.base);
        sc = (uint16_t)(3 + ((h >> 6) & 7)); si = (uint16_t)((h >> 3) % sc);
        st = (h & 1) ? 0x160 : (uint16_t)((h >> 4) & ~0x160u); ty = (uint16_t)(h >> 8); fr = (h >> 9) & 0x3FF;
    }
    memcpy(out + 0, &st, 2); memcpy(out + 2, &ty, 2); memcpy(out + 4, &si, 2); memcpy(out + 6, &sc, 2); memcpy(out + 8, &fr, 4);
}
static void build_model(void)
{
    unsigned t = 0, f = 1, nsc = 3 + rn(6), nf = 20 + rn(60);
    S.total = 0;
    while (t < 520 && f <= nf) {
        unsigned n = nsc + (rn(8) == 0 ? rn(3) : 0);
        for (unsigned i = 0; i < n && t < 560; i++, t++) {
            S.st[t] = 0x160; S.ty[t] = 0x8001; S.si[t] = (uint16_t)i; S.sc[t] = (uint16_t)n; S.fr[t] = f;
            if (rn(14) == 0) S.st[t] = (uint16_t)(rnd() & ~0x160u);
        }
        if (rn(3) == 0) {
            S.st[t] = (uint16_t)(rnd() & ~0x160u); S.ty[t] = (uint16_t)rnd(); S.si[t] = (uint16_t)rn(8); S.sc[t] = (uint16_t)(rn(8) + 1); S.fr[t] = rnd(); t++;
        }
        f++;
    }
    S.total = t;
}

static uint32_t effect(int id, Arg* a, unsigned n)
{
    unsigned k = S.calls[id]++;
    uint32_t r = S.rv[id][k < 8 ? k : 7];
    (void)n;
    switch (id) {
    case S_ControllerGetButtonState: return S.pad[k % S.padn];
    case S_GameCheckAndHandleSoftReset:
        if (S.calls[id] >= S.term_after) term_requested = 1;
        return 0;
    case S_FontDrawLetters:
        if (S.term_fdl && S.calls[id] >= S.term_fdl) term_requested = 1;
        return 0;
    case S_MainLoop: term_requested = 1; return 0;
    case S_PClseek:
        if (a[2].v == 2) return S.fsize;
        S.fpos = a[1].v; return a[1].v;
    case S_PCread: {
        uint8_t* d = a[1].host; unsigned len = a[2].v;
        if (S.fpos >= S.fsize) return 0;
        {
            uint8_t h[0x28]; unsigned off = len == 0x28 ? 8 : 0;
            model_hdr(S.fpos / S.ssz, h);
            memset(d, 0, len);
            memcpy(d + off, h, len - off < 0x20 ? len - off : 0x20);
        }
        return len;
    }
    case S_ArchiveReadFileFromCdSector: {
        uint8_t* d = a[1].host; uint8_t h[0x28];
        model_hdr(a[0].v - S.base, h);
        memset(d, 0, 0x800); memcpy(d, h, 0x28);
        return 0;
    }
    case S_ArchiveGetArchiveOffsetIndices:
        if (a[0].host) *(int32_t*)a[0].host = 0x11;
        if (a[1].host) *(int32_t*)a[1].host = 0x22;
        return 0;
    case S_ArchiveDecodeSector: return S.base ? S.base : (hash32(a[0].v) & 0xFFFFFF);
    case S_func_801D37CC: S.frame_cb = 1; return 0;
    case S_func_801D3F7C:
        /* the player module pumps: it reports the next frame through the registered callback */
        if (S.frame_cb) {
            uint32_t frame = (uint32_t)S.cb_frame, x = rn(300), y = S.cb_y[rn(2)];
            S.cb_frame += 1 + (int)rn(3);
            if (side == 0) {
                uint32_t args[3] = { frame, x, y };
                mv_retail_call(0x800768D8, args, 3, 0x801EF000u, 100000, "frame callback");
            } else {
                func_800768D8((u16)frame, (u16)x, (u16)y);
            }
        }
        return 0;
    default: return r;
    }
}

/* ------------------------------------------------------------------------- */
/* globals                                                                   */
/* ------------------------------------------------------------------------- */
#define G(v, addr) { #v, &v, addr, sizeof(v), 0 }
static const Glob mv_globs[] = {
    G(D_80076EEC, 0x80076EECu), G(D_80076EF0, 0x80076EF0u), G(D_80076EF4, 0x80076EF4u), G(D_80076EF8, 0x80076EF8u),
    G(D_80076F04, 0x80076F04u), G(D_80076F08, 0x80076F08u),
    G(D_80076F8C, 0x80076F8Cu), G(D_80076F9C, 0x80076F9Cu), G(D_80076FAC, 0x80076FACu), G(D_80076FBC, 0x80076FBCu),
    G(D_80076FCC, 0x80076FCCu), G(D_80076FDC, 0x80076FDCu), G(D_80076FEC, 0x80076FECu), G(D_80076FFC, 0x80076FFCu),
    G(D_8007700C, 0x8007700Cu), G(D_80077010, 0x80077010u), G(D_80077014, 0x80077014u), G(D_80077018, 0x80077018u),
    G(D_8007701C, 0x8007701Cu), G(D_80077020, 0x80077020u), G(D_80077024, 0x80077024u), G(D_80077028, 0x80077028u),
    G(D_80077118, 0x80077118u), G(D_8007711C, 0x8007711Cu),
    G(D_80077394, 0x80077394u), G(D_80077398, 0x80077398u), G(D_8007739C, 0x8007739Cu), G(D_800773A0, 0x800773A0u),
    G(D_800773A4, 0x800773A4u), G(D_800773A8, 0x800773A8u), G(D_800773AC, 0x800773ACu), G(D_800773B0, 0x800773B0u),
    G(D_800773B4, 0x800773B4u), G(D_800773B8, 0x800773B8u),
    G(D_80077438, 0x80077438u), G(D_8007743C, 0x8007743Cu), G(D_80077440, 0x80077440u), G(D_80077444, 0x80077444u),
    G(D_80077448, 0x80077448u), G(D_8007744C, 0x8007744Cu), G(D_80077450, 0x80077450u), G(D_80077454, 0x80077454u),
    G(D_8004FE44, 0x8004FE44u), G(D_8004FE45, 0x8004FE45u), G(D_8004FE46, 0x8004FE46u), G(D_8004FE47, 0x8004FE47u),
    G(D_8005A49C, 0x8005A49Cu), G(D_8005A4A4, 0x8005A4A4u), G(D_8005A4A8, 0x8005A4A8u), G(D_8005A4B4, 0x8005A4B4u),
    G(D_8005A4B8, 0x8005A4B8u), G(D_8005A4DC, 0x8005A4DCu), G(D_80062514, 0x80062514u),
    G(D_801D68B4, 0x801D68B4u), G(D_801E89D4, 0x801E89D4u), G(g_ArchiveTable, 0x8004FDF0u),
};
#define NMVG (sizeof(mv_globs) / sizeof(mv_globs[0]))

/* env: native <-> retail layout */
static void env_to_retail(const uint8_t* nat, uint8_t* r)
{
    for (unsigned i = 0; i < 2; i++) {
        const uint8_t* n = nat + i * NENV; uint8_t* d = r + i * RENV_STRIDE;
        memcpy(d, n, 0x70);
        for (unsigned k = 0; k < 32; k++) {
            unsigned long w8; uint32_t w;
            memcpy(&w8, n + MVH_ot + k * MVH_ULONG, sizeof(w8)); w = (uint32_t)w8; memcpy(d + 0x70 + 4 * k, &w, 4);
        }
        memcpy(d + 0xF0, n + MVH_box, MVH_POLY_SIZE);
        memcpy(d + 0x114, n + MVH_frame, MVH_POLY_SIZE);
    }
}
static uint8_t env_retail_snap[2 * RENV_STRIDE];
static uint32_t env_ptr_retail;
static uint8_t* env_nat(void) { return D_80077124; }
static void push_env(void)
{
    uint8_t buf[2 * RENV_STRIDE]; uint32_t pw;
    memset(buf, 0, sizeof(buf));
    env_to_retail(env_nat(), buf);
    memcpy(hp(RENV), buf, sizeof(buf));
    pw = n2v(D_80077120);
    memcpy(hp(0x80077120u), &pw, 4);
}
static void snap_env(void) { memcpy(env_retail_snap, hp(RENV), sizeof(env_retail_snap)); memcpy(&env_ptr_retail, hp(0x80077120u), 4); }
static const char* env_diff(void)
{
    uint8_t buf[2 * RENV_STRIDE];
    memset(buf, 0, sizeof(buf));
    env_to_retail(env_nat(), buf);
    for (unsigned i = 0; i < sizeof(buf); i++)
        if (buf[i] != env_retail_snap[i]) { static char m[96]; snprintf(m, sizeof m, "env byte env%u+0x%x native=%02x retail=%02x", i / RENV_STRIDE, i % RENV_STRIDE, buf[i], env_retail_snap[i]); return m; }
    if (n2v(D_80077120) != env_ptr_retail) return "D_80077120";
    return NULL;
}

/* ------------------------------------------------------------------------- */
/* case driver                                                               */
/* ------------------------------------------------------------------------- */
typedef struct Fx {
    const char* name; uint32_t entry, end; int hasret; unsigned ncase; uint64_t budget; unsigned nargs;
    void (*setup)(uint32_t id); uint32_t (*native)(void);
} Fx;
static uint32_t AA[6];
static uint8_t ram_init[0x200000], ram_pushed[0x200000];
static unsigned prints_left2 = 3;
static void rep(const Fx* f, unsigned id, const char* detail)
{
    if (!prints_left2) return;
    prints_left2--;
    fprintf(stderr, "MOVIE RETAIL DIVERGE fn=%s case=%u %s\n", f->name, id, detail);
    printf("MOVIE RETAIL DIVERGE fn=%s case=%u %s\n", f->name, id, detail);
}
static void reset_script(void)
{
    memset(&S, 0, sizeof(S));
    memset(stk_anchor, 0, sizeof(stk_anchor));
    term_requested = 0;
    S.padn = 1; S.term_after = 1000000; S.ssz = 0x800;
}
static void fill_globals_random(void)
{
    for (unsigned i = 0; i < NMVG; i++) {
        uint8_t* p = mv_globs[i].nat;
        for (unsigned k = 0; k < mv_globs[i].size; k++) p[k] = (uint8_t)rnd();
    }
    for (unsigned k = 0; k < 2 * NENV; k++) env_nat()[k] = (uint8_t)rnd();
    D_80077120 = env_nat() + (rn(2) ? NENV : 0);
}
static void rand_pads(unsigned n)
{
    static const uint32_t bits[] = { 0x10, 0x20, 0x40, 0x80, 0x100, 0x800, 0x1000, 0x2000, 0x4000, 0x8000, 0x4, 0x8 };
    S.padn = n;
    for (unsigned i = 0; i < n; i++) {
        uint32_t v;
        unsigned r = rn(6);
        if (r == 0) v = 0;
        else if (r <= 2) v = bits[rn(12)];
        else if (r == 3) v = bits[rn(12)] | bits[rn(12)];
        else v = i ? S.pad[i - 1] : bits[rn(12)];
        if (rn(9) == 0) v = rnd() & 0xFFFF;
        S.pad[i] = v;
    }
}

static unsigned cov_hit_total[16], cov_all_total[16];
static int mv_run_case(const Fx* f, uint32_t id)
{
    unsigned i;
    uint32_t v0r, v0n = 0;
    static char script_copy[sizeof(S)];
    uint64_t sr;
    seed_case(id * 2654435761u + 11);
    reset_script();
    memset(low, 0, LOW_SIZE);
    fill_globals_random();
    f->setup(id);
    memcpy(low_init, low, LOW_SIZE);
    memcpy(ram_init, g_PsxRam, sizeof(ram_init));
    glob_push(); push_env();
    memcpy(ram_pushed, g_PsxRam, sizeof(ram_pushed));
    memcpy(script_copy, &S, sizeof(S));
    sr = rng_s;
    nev[0] = 0; side = 0;
    v0r = mv_retail_call(f->entry, AA, f->nargs, SP_TOP, f->budget, f->name);
    memcpy(low_retail, low, LOW_SIZE);
    glob_snapshot(); snap_env();
    /* a guest byte written outside the listed globals/env/stack/low would be a gap in this harness */
    for (uint32_t a = OV_LO; a < OV_HI; a++) {
        uint32_t o = a & 0x1FFFFF; unsigned listed = 0;
        if (a >= STK_LO) break;
        if (g_PsxRam[o] == ram_pushed[o]) continue;
        if (a >= RENV && a < RENV + 2 * RENV_STRIDE) continue;
        if (a >= 0x80077120u && a < 0x80077124u) continue;
        for (unsigned g = 0; g < NMVG; g++)
            if (a >= mv_globs[g].g && a < mv_globs[g].g + mv_globs[g].size) { listed = 1; break; }
        if (!listed) { fprintf(stderr, "ORACLE FAIL retail wrote unlisted guest byte %08x (fn=%s case=%u)\n", a, f->name, id); exit(2); }
    }
    memcpy(g_PsxRam, ram_init, sizeof(ram_init));
    unsigned nret = nev[0];
    memcpy(&S, script_copy, sizeof(S)); rng_s = sr;
    memset(stk_anchor, 0, sizeof(stk_anchor));
    term_requested = 0;
    memcpy(low, low_init, LOW_SIZE);
    nev[1] = 0; side = 1;
    if (setjmp(term_jb) == 0) v0n = f->native();
    side = 0;

    for (i = 0; i < LOW_CMP; i++)
        if (low[i] != low_retail[i]) { char d[80]; snprintf(d, sizeof d, "memory at 0x%x native=%02x retail=%02x", i, low[i], low_retail[i]); rep(f, id, d); return 1; }
    const char* g = glob_diff();
    if (g) { char d[80]; snprintf(d, sizeof d, "global %s", g); rep(f, id, d); return 1; }
    g = env_diff();
    if (g) { rep(f, id, g); return 1; }
    if (f->hasret && v0n != v0r) { char d[80]; snprintf(d, sizeof d, "return native=%x retail=%x", v0n, v0r); rep(f, id, d); return 1; }
    if (nret != nev[1]) {
        char d[200]; snprintf(d, sizeof d, "callee-count native=%u retail=%u", nev[1], nret);
        for (unsigned q = 0; q < nret && q < nev[1]; q++)
            if (ev[0][q].id != ev[1][q].id) { snprintf(d, sizeof d, "callee-count native=%u retail=%u (first id mismatch at %u: native %s retail %s)", nev[1], nret, q, spydef[ev[1][q].id].name, spydef[ev[0][q].id].name); break; }
        rep(f, id, d); return 1;
    }
    for (i = 0; i < nret; i++) {
        if (memcmp(&ev[0][i], &ev[1][i], sizeof(MvEv))) {
            char d[200];
            snprintf(d, sizeof d, "callee %u native=%s retail=%s", i, spydef[ev[1][i].id].name, spydef[ev[0][i].id].name);
            if (ev[0][i].id == ev[1][i].id)
                for (unsigned k = 0; k < MAXV; k++)
                    if (ev[0][i].v[k] != ev[1][i].v[k]) { snprintf(d, sizeof d, "callee %u %s arg%u native=%x retail=%x", i, spydef[ev[0][i].id].name, k, ev[1][i].v[k], ev[0][i].v[k]); break; }
            rep(f, id, d); return 1;
        }
    }
    return 0;
}

/* ------------------------------------------------------------------------- */
/* setups                                                                    */
/* ------------------------------------------------------------------------- */
#define O_STR   0x0100u
#define O_POLY0 0x1000u
#define O_POLY1 0x1100u
#define O_OT    0x1200u
#define O_BTN   0x1300u
#define O_TABLE 0x7B00u
static void fill_low_random(void) { for (unsigned i = 0; i < LOW_CMP; i++) low[i] = (u8)rnd(); }
static s32 rs16(void) { return (rnd() & 3) ? (s32)rrange(-400, 400) : (s32)(s16)rnd_val(); }
static s32 rw(void) { return (rnd() & 3) ? rrange(-400, 400) : (s32)(s16)rnd_val(); }
static void rvall(int id, uint32_t v) { for (int i = 0; i < 8; i++) S.rv[id][i] = v; }

static void rand_colour_state(u8 a[4][4], u8 b[4][4], s32* step, s32* len)
{
    for (int i = 0; i < 4; i++) {
        for (int c = 0; c < 4; c++) { a[i][c] = (u8)rnd(); b[i][c] = (u8)rnd(); }
        len[i] = rn(3) ? rrange(1, 0x120) : (rn(4) ? 0x20 : 1);
        step[i] = rn(4) ? rrange(0, len[i] + 1) : (s32)rn(0x140);
        if (rn(5) == 0) step[i] = len[i];
    }
}
static s32 pick_type(int wide)
{
    unsigned r = rn(20);
    if (r < 8) return 0;
    if (r < 16) return 1;
    if (r < 18) return 2;
    return wide ? (s32)rrange(3, 9) : 2; /* the menu keeps the type in 0..2 (MovieMain wraps it); retail reads uninitialised registers outside */
}
#if GRP == 1
/* ---- group 1: leaf ---- */
static void st_box(uint32_t id)
{
    (void)id; fill_low_random();
    AA[0] = lo_addr(O_POLY0); AA[1] = lo_addr(O_POLY1);
    AA[2] = (uint32_t)rs16(); AA[3] = (uint32_t)rs16(); AA[4] = (uint32_t)rw(); AA[5] = (uint32_t)rw();
}
static void st_anim_a(uint32_t id)
{
    st_box(id);
    AA[0] = lo_addr(O_OT); AA[1] = lo_addr(O_POLY0);
    rand_colour_state(D_80076F8C, D_80076F9C, D_80076FAC, D_80076FBC);
}
static void st_anim_b(uint32_t id)
{
    st_box(id);
    AA[0] = lo_addr(O_OT); AA[1] = lo_addr(O_POLY0);
    rand_colour_state(D_80076FCC, D_80076FDC, D_80076FEC, D_80076FFC);
}
static uint32_t nat_72D84(void) { func_80072D84(lo(O_POLY0), lo(O_POLY1), (s16)AA[2], (s16)AA[3], (s32)AA[4], (s32)AA[5]); return 0; }
static uint32_t nat_73328(void) { func_80073328(lo(O_POLY0), lo(O_POLY1), (s16)AA[2], (s16)AA[3], (s32)AA[4], (s32)AA[5]); return 0; }
static uint32_t nat_72F98(void) { func_80072F98(lo(O_OT), lo(O_POLY0), (s16)AA[2], (s16)AA[3], (s32)AA[4], (s32)AA[5]); return 0; }
static uint32_t nat_734B8(void) { func_800734B8(lo(O_OT), lo(O_POLY0), (s16)AA[2], (s16)AA[3], (s32)AA[4], (s32)AA[5]); return 0; }
static void st_none(uint32_t id) { (void)id; fill_low_random(); }
static uint32_t nat_74AF0(void) { return (uint32_t)func_80074AF0(); }

#endif
#if GRP == 2
/* ---- group 2: sector search ---- */
static void st_sector(uint32_t id)
{
    (void)id; fill_low_random();
    D_80077448 = pick_type(0);
    D_8007711C = (s32)rn(48);
    build_model();
    S.ssz = D_80077448 == 1 ? 0x920u : 0x800u;
    S.base = 0x400 + rn(0x20000);
    S.fsize = S.total * S.ssz;
    rvall(S_ArchiveDecodeSizeAbsolute, rn(6) == 0 ? rn(48) : (uint32_t)D_8007711C + 1 + rn(12));
    rvall(S_func_8002C3D8, rn(2) ? 0u : (rn(2) ? 1u : 0xFFFFFFFFu));
    rvall(S_ArchiveGetFilePath, lo_addr(O_STR));
    rvall(S_PCopen, 3 + rn(5));
    rvall(S_ArchiveDecodeAlignedSize, rn(8) == 0 ? rn(0x2000) * 16 : S.total * S.ssz - (rn(4) == 0 ? rn(S.ssz) : 0));
    AA[0] = rn(8) == 0 ? (uint32_t)rrange(-1, 3) : (rn(10) == 0 ? rn(0x2000) : (uint32_t)rrange(2, 24 + (int)rn(60)));
}
static uint32_t nat_74BA4(void) { return (uint32_t)func_80074BA4((s32)AA[0]); }
static uint32_t nat_7519C(void) { return (uint32_t)func_8007519C(); }

#endif
#if GRP == 3
/* ---- group 3: playback ---- */
static void st_play(uint32_t id)
{
    int32_t y0;
    (void)id; fill_low_random();
    D_80077448 = pick_type(1);
    D_8007711C = (s32)rn(48);
    rvall(S_ArchiveDecodeSizeAbsolute, rn(5) == 0 ? rn(0x1FFFF) : (uint32_t)D_8007711C + 1 + rn(12));
    rvall(S_ArchiveDecodeSize, rn(4) == 0 ? 0x18u : 1 + rn(0x100000));
    D_80077454 = rn(3) == 0 ? (s32)rnd_val() : (s32)rn(2);
    switch (rn(5)) { case 0: D_800773A0 = -1; break; case 1: D_800773A0 = 0; break; case 2: D_800773A0 = (s32)rrange(1, 0xF0); break;
                     case 3: D_800773A0 = (s32)rrange(0xF1, 0x200); break; default: break; }
    D_8007739C = rn(3) ? (s32)rrange(1, 60) : (s32)rnd_val();
    D_80077438 = rn(3) == 0 ? 1 : (rn(5) == 0 ? (s32)rnd_val() : 0);
    D_80077028 = rn(3) == 0 ? (s32)rn(3) : D_80077028;
    S.cb_frame = rrange(0, 50);
    y0 = D_800773A0 > 0 ? (0xF0 - D_800773A0) / 2 : 0;
    S.cb_y[0] = (uint32_t)y0; S.cb_y[1] = rn(2) ? (uint32_t)(y0 + 0xF0) : rn(0x200);
    rand_pads(48); S.term_after = 3 + rn(40);
    AA[0] = rn(2) ? 0u : rn(256); AA[1] = rn(300); AA[2] = rn(300);
}
static uint32_t nat_763BC(void) { func_800763BC((u8)AA[0]); return 0; }
static uint32_t nat_7625C(void) { func_8007625C(); return 0; }
static uint32_t nat_76488(void) { return (uint32_t)func_80076488(); }
static void st_cb(uint32_t id)
{
    (void)id; fill_low_random();
    D_801D68B4 = rn(3) == 0 ? (s32)rnd() : (s32)rn(3);
    if (rn(4) == 0) D_801D68B4 = 1;
    D_80077024 = rn(2) ? (s32)rn(0x100) : (s32)rnd();
    D_8007739C = rn(3) ? (s32)rrange(0, 300) : (s32)rnd_val();
    AA[0] = rn(2) ? rn(300) : (rnd() & 0xFFFF);
    AA[1] = rnd() & 0xFFFF;
    AA[2] = rn(3) == 0 ? (uint32_t)(D_80077024 & 0xFFFF) : (rn(2) ? rn(0x200) : (rnd() & 0xFFFF));
    if (rn(3) == 0) D_80077438 = 0;
}
static uint32_t nat_768D8(void) { func_800768D8((u16)AA[0], (u16)AA[1], (u16)AA[2]); return 0; }
static void st_poll(uint32_t id) { (void)id; fill_low_random(); rand_pads(10); if (rn(2)) D_800773AC = (s32)S.pad[0]; if (rn(2)) D_80077028 = 0; if (rn(3) == 0) { D_80076F04 = (s32)rn(200); D_80076F08 = rn(2) ? 0x5A : (s32)rn(150); } }
static uint32_t nat_769A4(void) { func_800769A4(); return 0; }

#endif
#if GRP == 4
/* ---- group 4: MovieMain + menu pad ---- */
static void st_main(uint32_t id)
{
    (void)id; fill_low_random();
    switch (rn(10)) { case 0: D_8004FE44 = 0; break; case 1: D_8004FE44 = 1; break; case 2: D_8004FE44 = 0x81; break;
                      case 3: D_8004FE44 = (u8)rnd(); break; default: D_8004FE44 = 0xFF; break; }
    rvall(S_func_8002C3D8, rn(3) == 0 ? 0u : (rn(2) ? 1u : 0xFFFFFFFFu));
    S.rv[S_HeapAlloc][0] = 0x80100000u + (rn(0x100) << 4); S.rv[S_HeapAlloc][1] = 0x80140000u + (rn(0x100) << 4);
    for (int i = 2; i < 8; i++) S.rv[S_HeapAlloc][i] = 0x80160000u;
    rvall(S_ArchiveGetDiscNumber, 1 + rn(2));
    rand_pads(40);
    if (rn(3) == 0) S.pad[0] = rn(2) ? 0x100u : 0;
    D_80077118 = rn(12) == 0 ? rnd() : rn(14);
    S.term_after = 2 + rn(30);
    rvall(S_func_80074BA4, rn(4) == 0 ? 0xFFFFFFFFu : rn(400));
    rvall(S_func_8007519C, rn(3) == 0 ? 0xFFFFFFFFu : rn(0x2400));
    if (rn(4) == 0) { /* held right/left with the step-scale buttons at END FRAME then START FRAME: the 0x1FFF / 1 clamps */
        uint32_t sc = (rn(4) ? 0x90u : 0u) | (rn(2) ? 0x10u : 0u);
        uint32_t d1 = rn(2) ? 0x2000u : 0x8000u, d2 = rn(2) ? 0x2000u : 0x8000u;
        for (unsigned i = 1; i < 40; i++) S.pad[i] = 0;
        for (unsigned i = 1; i < 12; i += 2) S.pad[i] = d1 | sc;
        S.pad[12] = 0x1000;            /* cursor 3 -> 2 */
        for (unsigned i = 13; i < 30; i += 2) S.pad[i] = d2 | sc;
        D_80077118 = 3; D_8004FE44 = 0xFF; S.term_after = 25 + rn(10);
        if (rn(3) == 0) S.pad[12] = 0x4000; /* or down to 4 (MOVIE CHANNEL) and back */
    }
    if (rn(3) == 0) { /* menu-active random walk: circle / left-right (with step scale) / up-down edges */
        static const uint32_t m[] = { 0x20, 0x20, 0x2000, 0x8000, 0x2090, 0x8090, 0x2010, 0x8010, 0x1000, 0x4000, 0x2080, 0x8080 };
        for (unsigned i = 0; i < 40; i++) S.pad[i] = (i & 1) ? m[rn(12)] : 0;
        D_80077118 = 2 + rn(2); D_8004FE44 = 0xFF; S.term_after = 25 + rn(10);
    }
}
static uint32_t nat_main(void) { MovieMain(); return 0; }
static void st_pad(uint32_t id)
{
    (void)id; fill_low_random();
    rand_pads(10);
    AA[0] = rn(2) ? 0u : (uint32_t)rrange(-2, 3); AA[1] = rn(2) ? 0xD : rn(20); AA[2] = lo_addr(O_BTN);
    if (rn(2)) D_800773AC = (s32)S.pad[rn(S.padn)];
    if (rn(4) == 0) D_80076EF0 = 1 + (s32)rn(3);
}
static uint32_t nat_747AC(void) { return (uint32_t)func_800747AC((s32)AA[0], AA[1], (s32*)lo(O_BTN)); }

#endif
#if GRP == 5
/* ---- group 5: FAT CHECK ---- */
static void st_fat(uint32_t id)
{
    unsigned i;
    (void)id; fill_low_random();
    for (i = 0; i < 0x1260; i++) {
        uint8_t* e = low + O_TABLE + i * 7;
        for (unsigned k = 0; k < 7; k++) e[k] = (u8)rnd();
        if (rn(4) == 0) { e[3] = e[4] = e[5] = e[6] = 0; }
        else if (rn(3) == 0) { e[6] &= 0x7F; e[5] &= 0x0F; }
    }
    for (i = 0; i < 0x80; i++) low[O_STR + i] = (u8)(0x30 + rn(40));
    for (i = 0; i < 3; i++) low[O_STR + rn(0x40)] = '\\';
    low[O_STR + 0x40 + rn(0x20)] = 0;
    g_ArchiveTable = lo_addr(O_TABLE);
    rvall(S_ArchiveGetFilePath, rn(5) == 0 ? 0u : lo_addr(O_STR));
    rand_pads(48);
    S.term_fdl = 2 + rn(14);
}
static uint32_t nat_75D8C(void) { func_80075D8C(); return 0; }
static void st_size(uint32_t id) { st_fat(id); AA[0] = rn(0x1260); }
static uint32_t nat_75D4C(void) { return (uint32_t)func_80075D4C((s32)AA[0]); }

#endif
/* ------------------------------------------------------------------------- */
static const Fx fx_all[] = {
#if GRP == 1
    { "func_80072D84", 0x80072D84, 0x80072F98, 0, 4000, 200000, 6, st_box, nat_72D84 },
    { "func_80072F98", 0x80072F98, 0x80073328, 0, 4000, 200000, 6, st_anim_a, nat_72F98 },
    { "func_80073328", 0x80073328, 0x800734B8, 0, 4000, 200000, 6, st_box, nat_73328 },
    { "func_800734B8", 0x800734B8, 0x800737EC, 0, 4000, 200000, 6, st_anim_b, nat_734B8 },
    { "func_80074AF0", 0x80074AF0, 0x80074B58, 1, 2000, 200000, 0, st_none, nat_74AF0 },
#elif GRP == 2
    { "func_80074BA4", 0x80074BA4, 0x8007519C, 1, 6000, 20000000, 1, st_sector, nat_74BA4 },
    { "func_8007519C", 0x8007519C, 0x800753B8, 1, 4000, 20000000, 0, st_sector, nat_7519C },
#elif GRP == 3
    { "func_800763BC", 0x800763BC, 0x80076488, 0, 4000, 20000000, 1, st_play, nat_763BC },
    { "func_8007625C", 0x8007625C, 0x800763BC, 0, 4000, 20000000, 0, st_play, nat_7625C },
    { "func_80076488", 0x80076488, 0x800768D8, 1, 4000, 20000000, 0, st_play, nat_76488 },
    { "func_800768D8", 0x800768D8, 0x800769A4, 0, 3000, 200000, 3, st_cb, nat_768D8 },
    { "func_800769A4", 0x800769A4, 0x80076AF0, 0, 3000, 200000, 0, st_poll, nat_769A4 },
#elif GRP == 4
    { "MovieMain", 0x800737EC, 0x800747AC, 0, 1500, 80000000, 0, st_main, nat_main },
    { "func_800747AC", 0x800747AC, 0x80074AF0, 1, 4000, 200000, 3, st_pad, nat_747AC },
#elif GRP == 5
    { "func_80075D8C", 0x80075D8C, 0x8007625C, 0, 500, 80000000, 0, st_fat, nat_75D8C },
    { "func_80075D4C", 0x80075D4C, 0x80075D8C, 1, 2000, 200000, 1, st_size, nat_75D4C },
#endif
};

int main(int argc, char** argv)
{
    const char* only = argc > 1 ? argv[1] : NULL;
    const char* mpath = getenv("XENO_MOVIE"); if (!mpath) mpath = "disc/movie.bin";
    unsigned total_fail = 0, ran = 0;
    map_low();
    load_slus("MOVIE");
    {
        FILE* f = fopen(mpath, "rb");
        size_t n;
        if (!f) { fprintf(stderr, "MOVIE FAIL cannot open %s\n", mpath); return 2; }
        n = fread(g_PsxRam + (MOVIE_BASE & 0x1FFFFF), 1, MOVIE_SIZE + 16, f); fclose(f);
        if (n != MOVIE_SIZE) { fprintf(stderr, "MOVIE FAIL movie.bin size %zu\n", n); return 2; }
    }
    globs = mv_globs; nglobs = (unsigned)NMVG;
    for (int i = 0; i < S_COUNT; i++) {
        int tu = spydef[i].addr >= 0x80070000u && spydef[i].addr < 0x80077000u;
        spy_active[i] = GRP == 1 ? 0 : tu ? (GRP == 4 || (GRP == 5 && (i == S_func_80072F98 || i == S_func_800734B8))) : 1;
    }
    for (unsigned q = 0; q < sizeof(fx_all) / sizeof(fx_all[0]); q++) {
        const Fx* f = &fx_all[q];
        unsigned fails = 0, total = (f->end - f->entry) / 4, hit = 0;
        if (only && strcmp(only, f->name)) continue;
        cov_lo = f->entry; cov_hi = f->end; cov = calloc(total, 4);
        prints_left2 = 3;
        for (unsigned id = 0; id < f->ncase && !fails; id++) fails += (unsigned)mv_run_case(f, id);
        for (unsigned i = 0; i < total; i++) hit += cov[i] != 0;
        if (getenv("XENO_MOVIE_COV"))
            for (unsigned i = 0; i < total; i++) if (!cov[i]) fprintf(stderr, "  %s: uncovered %08x\n", f->name, f->entry + i * 4);
        if (fails) { total_fail++; printf("MOVIE RETAIL FAIL fn=%s\n", f->name); }
        else printf("MOVIE RETAIL PASS fn=%s cases=%u retail-instruction-coverage=%u/%u\n", f->name, f->ncase, hit, total);
        free(cov); cov = NULL; ran++;
    }
    if (!ran && !only) { fprintf(stderr, "MOVIE FAIL no function ran\n"); return 2; }
    fflush(stdout);
    return total_fail ? 1 : 0;
}
