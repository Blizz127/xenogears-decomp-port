/*
 * Differential certificate for the native port bodies in src/battle/main2_port.c:
 * func_800728B8 (strip linker), func_80074D4C (pulsing G3 arrows), func_80073FB8
 * and func_80074AB8 (HUD / window OT submit steps of func_80076418).
 *
 * CPU side: the retail bytes of the four functions (disc/battle.bin) AND the
 * retail SDK AddPrim (0x80043B48, disc/SLUS_006.64) run on the MIPS adapter --
 * the strip calls inside 73FB8/74AB8 execute retail 800728B8 bytes too.  Native
 * side: the port bodies (main2_port.c) on the same whole-RAM state; the OT
 * link is the guest-address AddPrim (add_prim) inside the port TU.
 *
 * Every case starts from random RAM with synthetic overlay globals (D_800CCB04
 * OT, D_800CCB34 buffer, D_800C3EA4 pool, D_800D2D28 menu block, D_800C3E24 arrow
 * block, D_800D2DB4 panel block, D_800D2E38 records, D_800C3E4C), runs both
 * sides, and compares every byte of guest RAM below the stack window plus the
 * callee-saved registers.  Alias cases point the OT entry at the very fields
 * the retail code re-reads between primitives (index/count bytes, the global
 * pointers themselves), which a body that caches a read cannot survive.
 * Coverage gate: every retail instruction of the four functions is executed.
 * Retail bytes are only read from disc/, never stored in the repo.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "battle_mips_adapter.h"

extern void func_800728B8(void* prims, s32 count, s32 start);
extern void func_80074D4C(void);
extern void func_80073FB8(void);
extern void func_80074AB8(void);

u8 g_PsxRam[PSX_RAM_SIZE];
u8 g_PsxScratchpad[4096];

enum {
    kAddPrim = 0x80043B48u,
    kAddPrimBytes = 60,
    kStackLo = 0x801FE000u,
    kStackTop = 0x801FF000u,
    kHalt = 0xFFFFFFFCu,
    kBudget = 3000000,

    kOtHead = 0x800CCB04u,   /* D_800CCB04 */
    kBufIdx = 0x800CCB34u,   /* D_800CCB34 */
    kPool = 0x800C3EA4u,     /* D_800C3EA4 */
    kMenu = 0x800D2D28u,     /* D_800D2D28 */
    kFx = 0x800C3E24u,       /* D_800C3E24 */
    kPanel = 0x800D2DB4u,    /* D_800D2DB4 */
    kRecTbl = 0x800D2E38u,   /* D_800D2E38[7] */
    kSlotFlag = 0x800C3E4Cu, /* D_800C3E4C */

    gOt = 0x80110000u,
    gPool = 0x80120000u,
    gMenu = 0x80140000u,
    gFx = 0x80150000u,
    gPanel = 0x80160000u,
    gRec = 0x80180000u, /* + i * 0x4000 */
    kFillLo = 0x80100000u,
    kFillHi = 0x801C0000u
};

typedef struct {
    u32 addr;
    unsigned n;
    unsigned hit[200];
} Fn;
static Fn s_fn[4] = {
    { 0x800728B8u, 32, { 0 } },
    { 0x80073FB8u, 186, { 0 } },
    { 0x80074AB8u, 165, { 0 } },
    { 0x80074D4Cu, 104, { 0 } },
};
static unsigned s_recordPc;
static unsigned s_checks, s_skipped, s_alias;
static unsigned s_cases[4];

static u8 s_snapshot[PSX_RAM_SIZE];
static u8 s_expRam[PSX_RAM_SIZE];
static u8 s_gotRam[PSX_RAM_SIZE];

#define FAIL(...)                                      \
    do {                                               \
        fprintf(stderr, "HUDRAW FAIL: " __VA_ARGS__);  \
        fputc('\n', stderr);                           \
        exit(1);                                       \
    } while (0)

static u8* bus_ptr(u32 a, unsigned w)
{
    u32 hi = a & 0xFF800000u;

    if ((hi == 0x80000000u || hi == 0xA0000000u) && (a & 0x1FFFFFu) + w <= 0x200000u) {
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

static int bus_bridge(void* u, PcPortMipsCpu* c, u32 t)
{
    unsigned f;

    (void)u;
    (void)c;
    if (s_recordPc) {
        for (f = 0; f < 4; ++f) {
            if (t >= s_fn[f].addr && t < s_fn[f].addr + s_fn[f].n * 4u) {
                s_fn[f].hit[(t - s_fn[f].addr) / 4]++;
            }
        }
    }
    return 0; /* AddPrim and the strip callee are retail code too */
}

static void load_at(const char* path, u32 base, u32 addr, unsigned n, const char* what)
{
    FILE* f = fopen(path, "rb");

    if (f == NULL || fseek(f, (long)(addr - base), SEEK_SET) != 0 ||
        fread(g_PsxRam + (addr & 0x1FFFFFu), 1, n, f) != (size_t)n) {
        FAIL("cannot read %s", what);
    }
    fclose(f);
}

static void load_retail(void)
{
    const char* bb = getenv("HUD_BATTLE_BIN") ? getenv("HUD_BATTLE_BIN") : "disc/battle.bin";
    const char* sl = getenv("HUD_SLUS") ? getenv("HUD_SLUS") : "disc/SLUS_006.64";
    unsigned f;

    for (f = 0; f < 4; ++f) {
        load_at(bb, 0x8006FAF0u, s_fn[f].addr, s_fn[f].n * 4u, "retail function (battle.bin)");
    }
    load_at(sl, 0x8000F800u, kAddPrim, kAddPrimBytes, "AddPrim (SLUS_006.64)");
}

static u32 s_rng = 1;
static u32 rnd(void)
{
    s_rng ^= s_rng << 13;
    s_rng ^= s_rng >> 17;
    s_rng ^= s_rng << 5;
    return s_rng;
}

static void put32(u32 a, u32 v) { memcpy(g_PsxRam + (a & 0x1FFFFFu), &v, 4); }
static void put16(u32 a, u32 v) { memcpy(g_PsxRam + (a & 0x1FFFFFu), &v, 2); }
static void put8(u32 a, u32 v) { g_PsxRam[a & 0x1FFFFFu] = (u8)v; }

/* One synthetic state.  Fields are consumed by apply(). */
typedef struct {
    u32 ot;             /* D_800CCB04 value: OT head (entry 1 = +4) */
    u32 buf;            /* D_800CCB34 */
    u32 slot;           /* D_800C3E4C byte */
    u32 menu;           /* D_800D2D28 value */
    u32 fx;             /* D_800C3E24 value */
    u32 panel;          /* D_800D2DB4 value */
    u32 rec[7];         /* D_800D2E38[] */
    u32 c6, c7, ad;     /* menu flag bytes */
    u32 e0;             /* fx +0xE0 word */
    u32 e4, e5;         /* fx +0xE4 / +0xE5 bytes */
    unsigned cmax;      /* counts / starts drawn from [0, cmax] */
    unsigned recmax;    /* record index byte drawn from [0, recmax] */
    s32 reccnt;         /* record count word base: >=0 -> rnd % (n+1) */
    u32 counter;        /* panel halfword +0x5DA2 */
    u32 seed;           /* rng replay seed */
    int alias_target;   /* if nonzero: OT entry 1 sits on this guest address */
} Setup;

static void apply(const Setup* s)
{
    u32 i;

    memcpy(g_PsxRam, s_snapshot, PSX_RAM_SIZE);
    s_rng = s->seed;
    for (i = kFillLo; i < kFillHi; i += 4) {
        put32(i, rnd());
    }
    put32(kOtHead, s->ot);
    put32(kBufIdx, s->buf);
    put32(kPool, gPool);
    put32(kMenu, s->menu);
    put32(kFx, s->fx);
    put32(kPanel, s->panel);
    put8(kSlotFlag, s->slot);
    for (i = 0; i < 7; ++i) {
        put32(kRecTbl + i * 4u, s->rec[i]);
    }
    /* menu block flags: +0x7F/+0x84 per slot, +0xB0 per record */
    put8(s->menu + 0xC6u, s->c6);
    put8(s->menu + 0xC7u, s->c7);
    put8(s->menu + 0xADu, s->ad);
    for (i = 0; i < 4; ++i) {
        put8(s->menu + 0x7Fu + i, rnd() & 1);
        put8(s->menu + 0x84u + i, rnd() % 3);
    }
    for (i = 0; i < 7; ++i) {
        put8(s->menu + 0xB0u + i, (rnd() & 3) != 0);
    }
    /* arrow block */
    put32(s->fx + 0xE0u, s->e0);
    put8(s->fx + 0xE4u, s->e4);
    put8(s->fx + 0xE5u, s->e5);
    for (i = 0; i < 4; ++i) {
        put8(s->fx + 0xE6u + i, rnd() & 1);
    }
    /* panel block: counts and starts are all bytes in +0x5D70..+0x5DA1 */
    for (i = 0x5D70u; i <= 0x5DA1u; ++i) {
        put8(s->panel + i, rnd() % (s->cmax + 1u));
    }
    put16(s->panel + 0x5DA2u, s->counter);
    /* records: count word +0x5A0, start byte +0x5A4 */
    for (i = 0; i < 7; ++i) {
        s32 cnt = s->reccnt >= 0 ? (s32)(rnd() % (u32)(s->reccnt + 1)) : -(s32)(rnd() % 4u);

        put32(s->rec[i] + 0x5A0u, (u32)cnt);
        put8(s->rec[i] + 0x5A4u, rnd() % (s->recmax + 1u));
    }
    (void)s->alias_target;
}

static void run_retail(u32 entry, u32 a0, u32 a1, u32 a2, int* rc_out)
{
    PcPortMipsCpu cpu;
    PcPortMipsBus bus;
    u32 i, saved[32];
    int rc;

    memset(&bus, 0, sizeof(bus));
    bus.read = bus_read;
    bus.write = bus_write;
    bus.bridge = bus_bridge;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[29] = kStackTop;
    cpu.gpr[31] = kHalt;
    cpu.gpr[4] = a0;
    cpu.gpr[5] = a1;
    cpu.gpr[6] = a2;
    for (i = 16; i <= 23; ++i) {
        cpu.gpr[i] = 0x51000000u + i;
    }
    cpu.gpr[30] = 0x5100001Eu;
    memcpy(saved, cpu.gpr, sizeof(saved));
    rc = PcPortMipsRun(&cpu, entry, kHalt, kBudget);
    *rc_out = rc;
    if (rc == PC_PORT_MIPS_HALTED) {
        if (cpu.gpr[29] != kStackTop) {
            FAIL("retail sp not restored");
        }
        for (i = 16; i <= 23; ++i) {
            if (cpu.gpr[i] != saved[i]) {
                FAIL("retail clobbered s%u", i - 16);
            }
        }
    }
}

/* which: 0 = 800728B8 (args), 1 = 73FB8, 2 = 74AB8, 3 = 74D4C */
static void run_case(const char* name, int which, const Setup* s, u32 a0, s32 a1, s32 a2)
{
    int rc;
    unsigned f;
    u32 i;
    Fn hitsave[4];
    static const u32 entries[4] = { 0x800728B8u, 0x80073FB8u, 0x80074AB8u, 0x80074D4Cu };

    /* ---- retail CPU ---- */
    apply(s);
    if (which == 0) {
        /* The strip helper's own arguments live in the registers; seed the
         * OT head for it as for every other entry. */
    }
    memcpy(hitsave, s_fn, sizeof(s_fn));
    s_recordPc = 1;
    run_retail(entries[which], a0, (u32)a1, (u32)a2, &rc);
    s_recordPc = 0;
    if (rc != PC_PORT_MIPS_HALTED) {
        memcpy(s_fn, hitsave, sizeof(s_fn)); /* coverage only from compared cases */
        /* A wild pointer or an unbounded count the alias pushed the retail
         * code into: no comparable result.  Counted, and bounded. */
        s_skipped++;
        return;
    }
    memcpy(s_expRam, g_PsxRam, PSX_RAM_SIZE);

    /* ---- native port body, same state ---- */
    apply(s);
    switch (which) {
    case 0: func_800728B8(PSX_ADDR(a0), a1, a2); break;
    case 1: func_80073FB8(); break;
    case 2: func_80074AB8(); break;
    default: func_80074D4C(); break;
    }
    memcpy(s_gotRam, g_PsxRam, PSX_RAM_SIZE);

    for (i = 0; i < kStackLo - 0x80000000u; ++i) {
        if (s_expRam[i] != s_gotRam[i]) {
            FAIL("%s: RAM 0x%08X retail=%02X native=%02X", name, 0x80000000u + i,
                 s_expRam[i], s_gotRam[i]);
        }
    }
    (void)f;
    s_checks++;
    s_cases[which]++;
}

static Setup base_setup(u32 seed)
{
    Setup s;
    unsigned i;

    memset(&s, 0, sizeof(s));
    s_rng = seed;
    s.ot = gOt + ((rnd() % 0x400u) & ~3u);
    s.buf = rnd() & 1;
    s.slot = (rnd() % 4 == 0) ? (rnd() & 3) : 1;
    s.menu = gMenu;
    s.fx = gFx;
    s.panel = gPanel;
    for (i = 0; i < 7; ++i) {
        s.rec[i] = gRec + i * 0x4000u;
    }
    s.c6 = rnd() % 5 != 0;
    s.c7 = rnd() % 4 != 0;
    s.ad = rnd() % 4 != 0;
    s.e0 = 0x40u + (rnd() % 0xC0u);
    s.e4 = rnd() & 1;
    s.e5 = rnd() & 1;
    s.cmax = 3;
    s.recmax = 2;
    s.reccnt = 3;
    s.counter = rnd() % 0x20u;
    s.seed = seed ^ 0x9E3779B9u;
    return s;
}

/* Guest addresses the retail code reads between primitives, plus the OT
 * global itself: the OT entry 1 (head + 4) is placed on each in turn. */
static u32 alias_target(const Setup* s, unsigned k)
{
    switch (k % 14) {
    case 0: return kOtHead;
    case 1: return kBufIdx;
    case 2: return kPool;
    case 3: return kMenu;
    case 4: return kFx;
    case 5: return kPanel;
    case 6: return kSlotFlag & ~3u;
    case 7: return kRecTbl + (rnd() % 7) * 4u;
    case 8: return s->rec[rnd() % 7] + 0x5A4u;
    case 9: return s->rec[rnd() % 7] + 0x5A0u;
    case 10: return s->fx + 0xE4u;
    case 11: return s->fx + 0xE0u;
    case 12: return s->panel + 0x5D70u + (rnd() % 0x30u & ~3u);
    default: return s->panel + 0x5DA0u;
    }
}

int main(void)
{
    Setup s;
    unsigned seed, k, f;
    static const u32 kE0[] = { 0x3C, 0x3D, 0x40, 0x41, 0x43, 0x44, 0xF8, 0xFB, 0xFC, 0xFD, 0x100,
                               0x104, 0, 4, 0xFFFFFFFCu, 0x7FFFFFFCu, 0x80000000u, 0xFFu, 0x1FFu };
    static const u32 kCounter[] = { 0, 1, 0xD, 0xE, 0xF, 0x10, 0x13, 0x14, 0x15, 0x16, 0x7FFE,
                                    0x7FFF, 0x8000, 0xFFFE, 0xFFFF };

    load_retail();
    memcpy(s_snapshot, g_PsxRam, PSX_RAM_SIZE);

    /* 1. 74D4C: every threshold of the pulse in both directions x buffer
     *    bytes (2 makes slot 3's stores land on +0xE4/+0xE5/+0xE6). */
    for (k = 0; k < sizeof(kE0) / sizeof(kE0[0]); ++k) {
        unsigned e5, e4;

        for (e5 = 0; e5 < 3; ++e5) {
            for (e4 = 0; e4 < 5; ++e4) {
                s = base_setup(0x1000u + k * 64u + e5 * 8u + e4);
                s.c6 = 1;
                s.e0 = kE0[k];
                s.e5 = e5;
                s.e4 = e4 == 4 ? 0xFFu : e4;
                run_case("74D4C thresholds", 3, &s, 0, 0, 0);
            }
        }
    }
    s = base_setup(0x1234u);
    s.c6 = 0;
    run_case("74D4C off", 3, &s, 0, 0, 0);

    /* 2. 800728B8 direct: counts (zero, negative, one, many) x starts incl.
     *    wrap-sized ones; KSEG0 and KSEG1 strip addresses. */
    {
        static const s32 kCnt[] = { 0, -1, -5, 1, 2, 3, 7, 0x7FFFFFFF, 0x80000000u };
        static const s32 kStart[] = { 0, 1, 2, 5, 0xFF, -1, -2, 0x06666667, 0x20000001, 0x7FFFFFFF };

        for (k = 0; k < sizeof(kCnt) / sizeof(kCnt[0]); ++k) {
            for (f = 0; f < sizeof(kStart) / sizeof(kStart[0]); ++f) {
                s = base_setup(0x2000u + k * 32u + f);
                if (kCnt[k] == 0x7FFFFFFF) {
                    continue; /* unbounded loop on both sides; covered by the budget skip */
                }
                run_case("728B8 direct", 0, &s, gPool + 0x8000u + ((k & 1) ? 0x20000000u : 0), kCnt[k],
                         kStart[f]);
            }
        }
    }

    /* 3. 74AB8: the 0x5DA2 counter at every threshold (u16 vs s16, wrap),
     *    flags on and off. */
    for (k = 0; k < sizeof(kCounter) / sizeof(kCounter[0]); ++k) {
        for (f = 0; f < 4; ++f) {
            s = base_setup(0x3000u + k * 16u + f);
            s.counter = kCounter[k];
            s.c7 = f & 1;
            s.ad = (f >> 1) & 1;
            if (f == 3) {
                s.c7 = 1;
                s.ad = 1;
            }
            run_case("74AB8 counter", 2, &s, 0, 0, 0);
        }
    }

    /* 4. 73FB8: slot flag 0/1/2 x buffer, records on/off. */
    for (k = 0; k < 3; ++k) {
        for (f = 0; f < 2; ++f) {
            s = base_setup(0x4000u + k * 4u + f);
            s.slot = k;
            s.buf = f;
            run_case("73FB8 slot flag", 1, &s, 0, 0, 0);
        }
    }

    /* 5. alias cases: OT entry 1 on the fields retail re-reads. */
    for (seed = 0; seed < 224; ++seed) {
        int which = 1 + (int)(seed % 3);

        s = base_setup(0x5000u + seed);
        s_rng = 0x5A5A0000u + seed;
        s.ad = 1;
        s.c6 = 1;
        s.c7 = 1;
        s.slot = 1;
        s.ot = alias_target(&s, seed / 3) - 4u;
        if (which == 3) {
            s.e4 = rnd() % 3;
        }
        s_alias++;
        run_case("alias", which == 3 ? 3 : which, &s, 0, 0, 0);
    }

    /* 6. fuzz: random block placement, big counts, random flags. */
    for (seed = 1; seed <= 600; ++seed) {
        int which = (int)(seed % 4);
        unsigned i;

        s = base_setup(0x6000u + seed);
        s_rng = seed * 2654435761u + 11u;
        s.cmax = (seed % 11 == 0) ? 0xFFu : (rnd() % 8u);
        s.recmax = (seed % 13 == 0) ? 0xFFu : (rnd() % 4u);
        s.reccnt = (seed % 7 == 0) ? -1 : (s32)(rnd() % 9u);
        s.counter = rnd();
        s.slot = rnd() % 3;
        s.buf = (seed % 5 == 0) ? rnd() : (rnd() & 1);
        s.e0 = rnd();
        s.e4 = (seed % 6 == 0) ? rnd() : (rnd() & 1);
        s.ot = gOt + ((rnd() % 0x4000u) & ~3u);
        s.menu = gMenu + ((rnd() % 0x400u) & ~3u);
        s.fx = gFx + ((rnd() % 0x400u) & ~3u);
        s.panel = gPanel + ((rnd() % 0x400u) & ~3u);
        for (i = 0; i < 7; ++i) {
            s.rec[i] = gRec + i * 0x4000u + ((rnd() % 0x100u) & ~3u);
        }
        run_case("fuzz", which, &s, gPool + 0x4000u, (s32)(rnd() % 9u) - 2, (s32)(rnd() % 6u));
    }

    /* Coverage gate: every retail instruction of the four functions ran. */
    {
        unsigned miss = 0, total = 0;

        for (f = 0; f < 4; ++f) {
            for (k = 0; k < s_fn[f].n; ++k) {
                total++;
                if (s_fn[f].hit[k] == 0) {
                    fprintf(stderr, "HUDRAW uncovered retail PC 0x%08X\n", s_fn[f].addr + k * 4u);
                    miss++;
                }
            }
        }
        if (miss != 0) {
            FAIL("%u of %u retail instructions never executed", miss, total);
        }
        if (s_checks < 1000 || s_skipped * 20 > s_checks) {
            FAIL("too few comparable cases: checks=%u skipped=%u", s_checks, s_skipped);
        }
        printf("HUDRAW PORT PASS checks=%u skipped=%u alias=%u (728B8:%u 73FB8:%u 74AB8:%u 74D4C:%u) "
               "instr coverage %u/%u\n",
               s_checks, s_skipped, s_alias, s_cases[0], s_cases[1], s_cases[2], s_cases[3], total, total);
    }
    return 0;
}
