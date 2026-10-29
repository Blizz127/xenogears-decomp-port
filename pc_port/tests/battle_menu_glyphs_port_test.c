/*
 * Differential certificate for the native port bodies of battle
 * func_8008D598 (command-menu glyph builder) and func_8008D328 (its fade),
 * src/battle/main49_port.c.
 *
 * CPU side: the retail bytes of the whole disc/battle.bin overlay (incl. the
 * static D_800C3000 / D_800C2F4C menu tables) and the retail SDK
 * SetSemiTrans/SetShadeTex (0x80043BFC..0x80043C4C, disc/SLUS_006.64) run on
 * the MIPS adapter.  Native side: the port bodies linked against the real
 * PsyCross SetSemiTrans/SetShadeTex (USE_EXTENDED_PRIM_POINTERS=0).
 *
 * func_80076A10 (the glyph layout, native in production in main4.c over
 * func_8002675C) is NOT covered here: both sides call the same deterministic
 * fake instead (return 0..3 from a hash of its arguments, pseudo-random bytes
 * over ret * 0x50 prim bytes, every call's arguments logged).  The logs, the
 * return value, whole guest RAM and the host buffers must match.
 *
 * Coverage gate: every instruction of retail 0x8008D328..0x8008DC34 must be
 * executed on the CPU side across the suite (the bridge sees each fetch).
 * The 8008D598 arguments carry dirty high bits on the CPU side so the retail
 * masks are exercised.  Retail bytes are only read from disc/.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "battle_mips_adapter.h"

extern s32 func_8008D598(u8 actor, u8 menu, u8 fade);
extern void func_8008D328(void);

u8 g_PsxRam[PSX_RAM_SIZE];
u8 g_PsxScratchpad[4096];

enum {
    kFnLo = 0x8008D328u,
    kFnHi = 0x8008DC34u,
    kFade = 0x8008D328u,
    kBuild = 0x8008D598u,
    kFakeAddr = 0x80076A10u,
    kLeafAddr = 0x80043BFCu,
    kLeafBytes = 0x50,
    kBinBase = 0x8006FAF0u,
    kGlobBlk = 0x800D2D28u,  /* D_800D2D28 */
    kGlobPool = 0x800C3EA4u, /* D_800C3EA4 */
    kGlobAct = 0x800C3EACu,  /* D_800C3EAC */
    kGlobBuf = 0x800CCB34u,  /* D_800CCB34 */
    kMenuTbl = 0x800C3000u,  /* D_800C3000 */
    kListTbl = 0x800C2F4Cu,  /* D_800C2F4C */
    kGuestBlk = 0x80110000u,
    kGuestPool = 0x80120000u,
    kGuestAct = 0x80140000u,
    kGuestSyn = 0x80160000u,
    kBlkSize = 0x400,
    kPoolSize = 0xB000,
    kActSize = 0x4400,
    kSynSize = 0x400,
    kStackLo = 0x801FE000u,
    kSynMenu = 0xE0,
    kSynList0 = 0xA0,
    kSynList1 = 0xA1,
    kMaxLog = 512
};
static u8 s_hostBlk[kBlkSize] __attribute__((aligned(16)));
static u8 s_hostPool[kPoolSize] __attribute__((aligned(16)));
static u8 s_hostAct[kActSize] __attribute__((aligned(16)));
static u8 s_snapshot[PSX_RAM_SIZE];

static unsigned s_checks;
static unsigned s_pcHit[(kFnHi - kFnLo) / 4];
static unsigned s_recordPc; /* count PCs only on the CPU side */

#define FAIL(...)                                          \
    do {                                                   \
        fprintf(stderr, "MENUGLYPHS FAIL: " __VA_ARGS__);  \
        fputc('\n', stderr);                               \
        exit(1);                                           \
    } while (0)

/* ---- the shared fake func_80076A10 ------------------------------------- */
typedef struct {
    u32 idx;
    u32 polys;
    s32 x;
    s32 y;
    s32 ret;
} Call;
static Call s_log[kMaxLog];
static unsigned s_nlog;
static int s_fakeMode; /* 0 = hash 0..3, 1 = always 1, 2 = always 0, 3 = always 2 */
static unsigned s_retHit[8];

static u32 mix(u32 h)
{
    h ^= h >> 16;
    h *= 0x7FEB352Du;
    h ^= h >> 15;
    h *= 0x846CA68Bu;
    h ^= h >> 16;
    return h;
}

static u32 norm_ptr(const u8* p)
{
    if (p >= g_PsxRam && p < g_PsxRam + PSX_RAM_SIZE) {
        return 0x80000000u + (u32)(p - g_PsxRam);
    }
    return (u32)(uintptr_t)p;
}

static s32 fake_ret(u32 idx, s32 x, s32 y)
{
    u32 h = mix(idx * 0x9E3779B1u ^ (u32)x * 0x85EBCA6Bu ^ (u32)y * 0xC2B2AE35u);

    switch (s_fakeMode) {
    case 1: return 1;
    case 2: return 0;
    case 3: return 2;
    default: return (s32)(h & 3u);
    }
}

static s32 fake_draw(u32 idx, u8* polys, s16 x, s16 y)
{
    s32 ret = fake_ret(idx, x, y);
    s32 i;

    if (s_nlog >= kMaxLog) {
        FAIL("fake func_80076A10 called too many times");
    }
    s_log[s_nlog++] = (Call){ idx, norm_ptr(polys), x, y, ret };
    if (ret < 8) {
        s_retHit[ret]++;
    }
    for (i = 0; i < ret * 0x50; ++i) {
        u32 h = mix((u32)i * 2654435761u ^ idx * 131u ^ (u32)x * 17u ^ (u32)y);

        polys[i] = (u8)h;
        if ((i % 0x28) == 4 && (h & 0x300u) == 0) {
            polys[i] = 0; /* r0 == 0 about 1 in 4 */
        } else if ((i % 0x28) == 4 && polys[i] == 0) {
            polys[i] = 1;
        }
    }
    return ret;
}

/* The native side links this as the production symbol. */
s32 func_80076A10(s32 index, u8* polys, s16 x, s16 y)
{
    return fake_draw((u32)index, polys, x, y);
}

/* ---- CPU bus ------------------------------------------------------------ */
static u8* bus_ptr(u32 a, unsigned w)
{
    if (a >= 0x80000000u && (uint64_t)a + w <= 0x80000000u + 0x200000u) {
        return g_PsxRam + (a & 0x1FFFFFu);
    }
#define HOSTBUF(b)                                                         \
    if ((uintptr_t)a >= (uintptr_t)(b) &&                                  \
        (uint64_t)a + w <= (uintptr_t)(b) + sizeof(b)) {                   \
        return (u8*)(uintptr_t)a;                                          \
    }
    HOSTBUF(s_hostBlk)
    HOSTBUF(s_hostPool)
    HOSTBUF(s_hostAct)
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

static int bus_bridge(void* u, PcPortMipsCpu* c, u32 t)
{
    (void)u;
    if (t >= kFnLo && t < kFnHi) {
        if (s_recordPc) {
            s_pcHit[(t - kFnLo) / 4]++;
        }
        return 0;
    }
    if (t >= kLeafAddr && t < kLeafAddr + kLeafBytes) {
        return 0; /* retail SetSemiTrans / SetShadeTex */
    }
    if (t == kFakeAddr) {
        u32 a0 = c->gpr[4], a1 = c->gpr[5], a2 = c->gpr[6], a3 = c->gpr[7];
        s32 ret = fake_ret(a0, (s16)a2, (s16)a3);
        u8* p = bus_ptr(a1, ret > 0 ? (unsigned)ret * 0x50u : 1u);
        unsigned i;

        if (p == NULL) {
            FAIL("retail passed an unmapped prim pointer 0x%08X to func_80076A10", a1);
        }
        (void)fake_draw(a0, p, (s16)a2, (s16)a3);
        c->gpr[2] = (u32)ret;
        /* a real callee clobbers the caller-saved registers */
        c->gpr[3] = 0xDEAD0003u;
        for (i = 4; i <= 7; ++i) c->gpr[i] = 0xDEAD0000u + i;
        for (i = 8; i <= 15; ++i) c->gpr[i] = 0xDEAD0000u + i;
        c->gpr[24] = 0xDEAD0018u;
        c->gpr[25] = 0xDEAD0019u;
        return 1;
    }
    return -1; /* anything else is unloaded code: never run it silently */
}

static void load_retail(void)
{
    const char* bb = getenv("MG_BATTLE_BIN") ? getenv("MG_BATTLE_BIN") : "disc/battle.bin";
    const char* sl = getenv("MG_SLUS") ? getenv("MG_SLUS") : "disc/SLUS_006.64";
    FILE* f = fopen(bb, "rb");
    long n;

    if (f == NULL) {
        FAIL("cannot open battle.bin");
    }
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n <= 0 || kBinBase + (u32)n > 0x800D0000u ||
        fread(g_PsxRam + (kBinBase & 0x1FFFFFu), 1, (size_t)n, f) != (size_t)n) {
        FAIL("cannot read battle.bin");
    }
    fclose(f);
    f = fopen(sl, "rb");
    if (f == NULL || fseek(f, (long)(kLeafAddr - 0x8000F800u), SEEK_SET) != 0 ||
        fread(g_PsxRam + (kLeafAddr & 0x1FFFFFu), 1, kLeafBytes, f) != (size_t)kLeafBytes) {
        FAIL("cannot read retail SetSemiTrans/SetShadeTex from SLUS_006.64");
    }
    fclose(f);
}

/* ---- deterministic input generation ------------------------------------- */
static u32 s_rng = 1;
static u32 rnd(void)
{
    s_rng ^= s_rng << 13;
    s_rng ^= s_rng >> 17;
    s_rng ^= s_rng << 5;
    return s_rng;
}

typedef struct {
    u32 blk, pool, act;   /* values of D_800D2D28 / D_800C3EA4 / D_800C3EAC */
    u32 buf;              /* D_800CCB34 */
    u8 actor, menu, fade;
    int direct;           /* enter at func_8008D328 instead */
    int fakeMode;
    int cnt0, cnt1;       /* +0xD0/+0xD4 override when != -1 */
    int a3;               /* blk[0xA3] override when >= 0 */
    int act2e0;           /* act[0x2E0] override when >= 0 */
    int poolzero;         /* zero the whole pool */
    u32 synth;            /* != 0: synthetic menu/list tables from this seed */
} Setup;

static void put32(u32 a, u32 v) { memcpy(g_PsxRam + (a & 0x1FFFFFu), &v, 4); }
static void put16(u32 a, u32 v) { u16 h = (u16)v; memcpy(g_PsxRam + (a & 0x1FFFFFu), &h, 2); }

/* Synthetic menu 0xE0 -> groups/lists, lists 0xA0/0xA1: random entries
 * covering every remap/tint branch. */
static void build_synth(u32 seed)
{
    u32 save = s_rng;
    u32 cur = kGuestSyn + 0x20;
    u32 l, e, nent;
    u8 grp[2];
    static const u8 kGrp[3] = { 0, 1, 0xFF };

    s_rng = seed * 2246822519u + 3u;
    grp[0] = kGrp[rnd() % 3];
    grp[1] = (rnd() % 4 == 0) ? 0xFF : kGrp[rnd() % 2];
    if (seed % 5 == 0) {
        grp[0] = (u8)(rnd() % 2);
    }
    g_PsxRam[(kGuestSyn + 0) & 0x1FFFFFu] = grp[0];
    g_PsxRam[(kGuestSyn + 1) & 0x1FFFFFu] = grp[1];
    g_PsxRam[(kGuestSyn + 2) & 0x1FFFFFu] = kSynList0;
    g_PsxRam[(kGuestSyn + 3) & 0x1FFFFFu] = (seed % 3 == 0) ? kSynList0 : kSynList1;
    put32(kMenuTbl + kSynMenu * 4u, kGuestSyn);
    for (l = 0; l < 2; ++l) {
        put32(kListTbl + (kSynList0 + l) * 4u, cur);
        nent = rnd() % 7;
        if (seed % 11 == 0 && l == 0) {
            nent = 0;
        }
        for (e = 0; e < nent; ++e) {
            u32 code, attr;

            switch (rnd() % 4) {
            case 0: code = 0x4000u | (rnd() & 0x3Fu) << 2 | (rnd() & 3u); break;
            case 1: code = (rnd() & 0xFFu) | 0x4000u; break;
            case 2: code = rnd() % 0x100u; break;
            default: code = rnd() % 0x3F00u; break;
            }
            switch (rnd() % 5) {
            case 0: attr = rnd() % 0x2000u; break;
            case 1: attr = 0x2000u | (rnd() % 0x1000u); break;
            case 2: attr = (rnd() % 0x2000u) & ~0xFFu; break;
            case 3: attr = 0x2000u | ((rnd() % 0x10u) << 4) | (rnd() % 0x10u); break;
            default: attr = rnd() & 0xFFFFu; break;
            }
            put16(cur + 0, code);
            put16(cur + 2, rnd() & 0xFFFFu);
            put16(cur + 4, rnd() & 0xFFFFu);
            put16(cur + 6, attr);
            cur += 8;
        }
        put16(cur, 0xFFFF);
        cur += 8;
    }
    s_rng = save;
}

static void fill(u8* p, unsigned n, unsigned flavour)
{
    unsigned i;

    for (i = 0; i < n; ++i) {
        u32 r = rnd();

        if (flavour == 1) { /* actor records: plenty of 0 / 0xFF */
            p[i] = (r % 100) < 40 ? 0 : (r % 100) < 65 ? 0xFF : (u8)(r >> 8);
        } else {
            p[i] = (u8)(r | 1u);
        }
    }
    if (flavour == 2) { /* prim pool: some zero bytes (r0 == 0) */
        for (i = 0; i < n; ++i) {
            if ((rnd() & 3u) == 0) {
                p[i] = 0;
            }
        }
    }
}

static void apply(const Setup* s)
{
    memcpy(g_PsxRam, s_snapshot, PSX_RAM_SIZE);
    put32(kGlobBlk, s->blk);
    put32(kGlobPool, s->pool);
    put32(kGlobAct, s->act);
    put32(kGlobBuf, s->buf);
    fill(s_hostBlk, kBlkSize, 0);
    fill(g_PsxRam + (kGuestBlk & 0x1FFFFFu), kBlkSize, 0);
    fill(s_hostPool, kPoolSize, 2);
    fill(g_PsxRam + (kGuestPool & 0x1FFFFFu), kPoolSize, 2);
    fill(s_hostAct, kActSize, 1);
    fill(g_PsxRam + (kGuestAct & 0x1FFFFFu), kActSize, 1);
    if (s->poolzero) {
        memset(s_hostPool, 0, kPoolSize);
        memset(g_PsxRam + (kGuestPool & 0x1FFFFFu), 0, kPoolSize);
    }
    if (s->cnt0 != -1) {
        u32 v0 = (u32)s->cnt0, v1 = (u32)s->cnt1;

        memcpy(g_PsxRam + ((kGuestBlk + 0xD0) & 0x1FFFFFu), &v0, 4);
        memcpy(g_PsxRam + ((kGuestBlk + 0xD4) & 0x1FFFFFu), &v1, 4);
        memcpy(s_hostBlk + 0xD0, &v0, 4);
        memcpy(s_hostBlk + 0xD4, &v1, 4);
    }
    if (s->a3 >= 0) {
        g_PsxRam[(kGuestBlk + 0xA3) & 0x1FFFFFu] = (u8)s->a3;
        s_hostBlk[0xA3] = (u8)s->a3;
    }
    if (s->act2e0 >= 0) {
        g_PsxRam[(kGuestAct + 0x2E0) & 0x1FFFFFu] = (u8)s->act2e0;
        s_hostAct[0x2E0] = (u8)s->act2e0;
    }
    if (s->synth != 0) {
        build_synth(s->synth);
    }
}

typedef struct {
    u8 ram[PSX_RAM_SIZE];
    u8 blk[kBlkSize];
    u8 pool[kPoolSize];
    u8 act[kActSize];
    Call log[kMaxLog];
    unsigned nlog;
    u32 ret;
} State;
static State s_exp;

static void capture(State* st, u32 ret)
{
    memcpy(st->ram, g_PsxRam, PSX_RAM_SIZE);
    memcpy(st->blk, s_hostBlk, kBlkSize);
    memcpy(st->pool, s_hostPool, kPoolSize);
    memcpy(st->act, s_hostAct, kActSize);
    memcpy(st->log, s_log, sizeof(s_log));
    st->nlog = s_nlog;
    st->ret = ret;
}

static void run_case(const char* name, const Setup* s)
{
    PcPortMipsCpu cpu;
    PcPortMipsBus bus;
    u32 i, saved[32];
    int rc;
    u32 seed = s_rng;
    State* got = (State*)malloc(sizeof(State));
    u32 ret;

    memset(&bus, 0, sizeof(bus));
    bus.read = bus_read;
    bus.write = bus_write;
    bus.bridge = bus_bridge;
    s_fakeMode = s->fakeMode;

    /* ---- retail CPU ---- */
    apply(s);
    s_nlog = 0;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[29] = 0x801FF000u;
    cpu.gpr[31] = 0xFFFFFFFCu;
    for (i = 16; i <= 23; ++i) {
        cpu.gpr[i] = 0x51000000u + i;
    }
    cpu.gpr[30] = 0x5100001Eu;
    /* dirty high bits above the byte arguments: retail masks them */
    cpu.gpr[4] = (rnd() & 0xFFFFFF00u) | s->actor;
    cpu.gpr[5] = (rnd() & 0xFFFFFF00u) | s->menu;
    cpu.gpr[6] = (s->fade ? ((rnd() & 0xFFFFFF00u) | s->fade) : (rnd() & 0xFFFFFF00u));
    memcpy(saved, cpu.gpr, sizeof(saved));
    s_recordPc = 1;
    rc = PcPortMipsRun(&cpu, s->direct ? kFade : kBuild, 0xFFFFFFFCu, 3000000);
    s_recordPc = 0;
    if (rc != PC_PORT_MIPS_HALTED) {
        FAIL("%s: retail CPU rc=%d (%s)", name, rc, cpu.error);
    }
    if (cpu.gpr[29] != 0x801FF000u) {
        FAIL("%s: retail sp not restored", name);
    }
    for (i = 16; i <= 23; ++i) {
        if (cpu.gpr[i] != saved[i]) {
            FAIL("%s: retail clobbered s%u", name, i - 16);
        }
    }
    if (cpu.gpr[30] != saved[30]) {
        FAIL("%s: retail clobbered fp", name);
    }
    capture(&s_exp, cpu.gpr[2]);

    /* ---- native port body, same state (rng replay) ---- */
    s_rng = seed;
    apply(s);
    s_nlog = 0;
    if (s->direct) {
        func_8008D328();
        ret = s_exp.ret; /* no return value */
    } else {
        ret = (u32)func_8008D598(s->actor, s->menu, s->fade);
    }
    capture(got, ret);

    if (got->nlog != s_exp.nlog) {
        FAIL("%s: func_80076A10 called %u times native vs %u retail", name, got->nlog, s_exp.nlog);
    }
    for (i = 0; i < s_exp.nlog; ++i) {
        if (memcmp(&got->log[i], &s_exp.log[i], sizeof(Call)) != 0) {
            FAIL("%s: func_80076A10 call %u differs: retail(%X,%X,%d,%d) native(%X,%X,%d,%d)",
                 name, i, s_exp.log[i].idx, s_exp.log[i].polys, s_exp.log[i].x, s_exp.log[i].y,
                 got->log[i].idx, got->log[i].polys, got->log[i].x, got->log[i].y);
        }
    }
    if (got->ret != s_exp.ret) {
        FAIL("%s: return retail=0x%08X native=0x%08X", name, s_exp.ret, got->ret);
    }
    for (i = 0; i < PSX_RAM_SIZE; ++i) {
        if (0x80000000u + i >= kStackLo) {
            break;
        }
        if (s_exp.ram[i] != got->ram[i]) {
            FAIL("%s: RAM 0x%08X retail=%02X native=%02X", name, 0x80000000u + i,
                 s_exp.ram[i], got->ram[i]);
        }
    }
    for (i = 0; i < kBlkSize; ++i) {
        if (s_exp.blk[i] != got->blk[i]) {
            FAIL("%s: host blk +0x%X retail=%02X native=%02X", name, i, s_exp.blk[i], got->blk[i]);
        }
    }
    for (i = 0; i < kPoolSize; ++i) {
        if (s_exp.pool[i] != got->pool[i]) {
            FAIL("%s: host pool +0x%X retail=%02X native=%02X", name, i, s_exp.pool[i], got->pool[i]);
        }
    }
    for (i = 0; i < kActSize; ++i) {
        if (s_exp.act[i] != got->act[i]) {
            FAIL("%s: host act +0x%X retail=%02X native=%02X", name, i, s_exp.act[i], got->act[i]);
        }
    }
    free(got);
    s_checks++;
}

static Setup base_setup(u32 kind)
{
    Setup s;

    memset(&s, 0, sizeof(s));
    s.blk = (kind & 1) ? (u32)(uintptr_t)s_hostBlk : kGuestBlk;
    s.pool = (kind & 2) ? (u32)(uintptr_t)s_hostPool : kGuestPool;
    s.act = (kind & 4) ? (u32)(uintptr_t)s_hostAct : kGuestAct;
    s.cnt0 = s.cnt1 = -1;
    s.a3 = s.act2e0 = -1;
    return s;
}

int main(void)
{
    Setup s;
    unsigned menu, ai, b, seed, kind, ci;
    static const u8 kActors[] = { 0, 1, 2, 3, 0xFF };
    static const int kCnt[][2] = { { 0, 0 }, { 1, 0 }, { 0, 2 }, { 3, 2 }, { 5, 5 }, { 1, 1 },
                                   { (int)0x80000000u, 0 }, { 0x40000000, 0x40000000 },
                                   { 0, (int)0x80000000u }, { 4, 0 } };
    static const int kA3[] = { 0, 1, 3, 17 };
    static const int k2e0[] = { 0, 1, 3, 0xF, 0x55 };
    unsigned i;

    load_retail();
    memcpy(s_snapshot, g_PsxRam, PSX_RAM_SIZE);
    s_rng = 0xC0FFEE01u;

    /* 1. every real menu x actors x buffer, production shape and memory kinds */
    for (menu = 0; menu < 26; ++menu) {
        for (ai = 0; ai < sizeof(kActors); ++ai) {
            for (b = 0; b < 2; ++b) {
                kind = (menu + ai + b) % 8;
                s = base_setup(kind);
                s.buf = b;
                s.actor = kActors[ai];
                s.menu = (u8)menu;
                run_case("real menu", &s);
            }
        }
    }

    /* 2. fakeModes over a few real menus (ret 0 / 1 / 2 each) */
    for (ci = 0; ci < 3; ++ci) {
        for (menu = 0; menu < 26; menu += 3) {
            s = base_setup(menu % 8);
            s.fakeMode = (int)(1 + (menu / 3 + ci) % 3);
            s.buf = menu & 1;
            s.actor = (u8)(menu % 4);
            s.menu = (u8)menu;
            run_case("real menu fake modes", &s);
        }
    }

    /* 3. fade through 8008D598 and directly at 8008D328 */
    for (ci = 0; ci < sizeof(kCnt) / sizeof(kCnt[0]); ++ci) {
        for (i = 0; i < sizeof(kA3) / sizeof(kA3[0]); ++i) {
            for (b = 0; b < 3; ++b) {
                s = base_setup((ci + i + b) % 8);
                s.fade = (u8)(b == 1 ? 0x80 : 1);
                s.direct = (b == 2);
                s.cnt0 = kCnt[ci][0];
                s.cnt1 = kCnt[ci][1];
                s.a3 = kA3[i];
                s.act2e0 = k2e0[(ci + i) % 5];
                s.actor = (u8)(ci & 3);
                s.menu = (u8)ci;
                s.poolzero = (ci == 9 && b == 0); /* all r0 zero: +0xCB cleared */
                run_case("fade", &s);
            }
        }
    }
    s = base_setup(0);
    s.fade = 1;
    s.cnt0 = 2;
    s.cnt1 = 2;
    s.a3 = 0;
    s.poolzero = 1;
    run_case("fade all-zero pool", &s);
    s.poolzero = 0;
    s.direct = 1;
    s.act2e0 = 0xFF;
    run_case("fade act 0xFF", &s);

    /* 4. synthetic menus: every remap / tint branch, all memory kinds */
    for (seed = 1; seed <= 600; ++seed) {
        s_rng = seed * 2654435761u + 11u;
        s = base_setup(seed % 8);
        s.synth = seed;
        s.menu = kSynMenu;
        s.actor = kActors[rnd() % sizeof(kActors)];
        s.buf = rnd() & 1u;
        if (seed % 13 == 0) {
            s.buf = 2;
        }
        s.fakeMode = (seed % 4 == 3) ? (int)(1 + seed % 3) : 0;
        if (seed % 17 == 0) {
            s.fade = 1;
            s.cnt0 = (int)(rnd() % 4);
            s.cnt1 = (int)(rnd() % 4);
            s.a3 = (int)(rnd() % 3);
        }
        run_case("synthetic menu", &s);
    }

    /* Coverage gate: every retail instruction executed on the CPU side. */
    {
        unsigned miss = 0;

        for (i = 0; i < (kFnHi - kFnLo) / 4; ++i) {
            if (s_pcHit[i] == 0) {
                fprintf(stderr, "MENUGLYPHS uncovered retail PC 0x%08X\n", kFnLo + i * 4);
                miss++;
            }
        }
        if (miss != 0) {
            FAIL("%u retail instructions never executed", miss);
        }
        if (s_retHit[0] == 0 || s_retHit[1] == 0 || s_retHit[2] == 0 || s_retHit[3] == 0) {
            FAIL("func_80076A10 fake returns 0/1/2/3 not all seen");
        }
    }
    printf("MENUGLYPHS PORT PASS checks=%u (instr coverage %u/%u, fake ret 0:%u 1:%u 2:%u 3:%u)\n",
           s_checks, (unsigned)((kFnHi - kFnLo) / 4), (unsigned)((kFnHi - kFnLo) / 4),
           s_retHit[0], s_retHit[1], s_retHit[2], s_retHit[3]);
    return 0;
}
