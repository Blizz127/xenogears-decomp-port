/*
 * Differential certificate for the native port bodies of the battle
 * Attack-accept target chain in src/battle/main35_port.c:
 *   func_80084A7C (selection) -> func_800841E0 (list) -> func_80083FF4 (rule).
 *
 * CPU side: the retail bytes of 0x80083FF4..0x80084B40 (disc/battle.bin) run
 * on the MIPS adapter.  Native side: the three port bodies.  Both start from
 * identical guest RAM and identical argument registers.  The chain has no
 * callee outside itself, so no spies: each entry point (all three, and the
 * callees through their callers) runs real code on both sides.  After both
 * runs the return value (for the two functions that return one), all of
 * guest RAM outside the CPU stack window, and the CPU's callee-saved
 * registers must match.
 *
 * Coverage gate: every retail instruction of the three functions must be
 * fetched by some case (the bus bridge sees every fetch).  A fetch outside
 * the three function ranges fails the case.
 *
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

u32 func_80083FF4(u32 arg0, u32 arg1);
u32 func_800841E0(u32 arg0);
void func_80084A7C(u32 arg0);

u8 g_PsxRam[PSX_RAM_SIZE];
u8 g_PsxScratchpad[4096];

#define FAIL(...)                                      \
    do {                                               \
        fprintf(stderr, "TLIST FAIL: " __VA_ARGS__);   \
        fputc('\n', stderr);                           \
        exit(1);                                       \
    } while (0)

enum {
    kBlobAddr = 0x80083FF4u,
    kBlobBytes = 0xB4Cu, /* through 0x80084B40 */
    kStackTop = 0x801FF000u,
    kStackLo = 0x801F0000u, /* CPU stack window, excluded from the RAM compare */
    kCtx = 0x80150000u,
    kMatrix = 0x80160000u,
    kFillLo = 0x800C0000u,
    kFillHi = 0x800E4000u,
    kList = 0x800C3E90u,
    kCtxPtr = 0x800C3EACu,
    kCount = 0x800D3274u,
    kGroup = 0x800C3EB4u,
    kBlocked = 0x800C3EB7u,
    kRank = 0x800CCD34u,
    kStatus = 0x800CCD64u,
    kAltStatus = 0x800CCE08u,
    kPresent = 0x800D2DCCu,
    kAlt = 0x800D32A1u,
    kMatrixPtr = 0x800D3364u
};
typedef struct { u32 lo, hi; } Range;
static const Range kFn[3] = {
    { 0x80083FF4u, 0x80084108u }, /* func_80083FF4 */
    { 0x800841E0u, 0x80084548u }, /* func_800841E0 */
    { 0x80084A7Cu, 0x80084B40u }, /* func_80084A7C */
};

static u8* gp(u32 a) { return g_PsxRam + (a & 0x1FFFFFu); }
static void p32(u32 a, u32 v) { memcpy(gp(a), &v, 4); }
static void p16(u32 a, u16 v) { memcpy(gp(a), &v, 2); }

static u32 s_rng = 0x2468ACEu;
static u32 rnd(void)
{
    s_rng ^= s_rng << 13;
    s_rng ^= s_rng >> 17;
    s_rng ^= s_rng << 5;
    return s_rng;
}

/* ---- CPU side ------------------------------------------------------------- */
static int bus_read(void* u, u32 a, unsigned w, u32* v)
{
    unsigned i;

    (void)u;
    if (a < 0x80000000u || (uint64_t)a + w > 0x80200000u) {
        return -1;
    }
    *v = 0;
    for (i = 0; i < w; ++i) {
        *v |= (u32)gp(a)[i] << (i * 8);
    }
    return 0;
}

static int bus_write(void* u, u32 a, unsigned w, u32 v)
{
    unsigned i;

    (void)u;
    if (a < 0x80000000u || (uint64_t)a + w > 0x80200000u) {
        return -1;
    }
    for (i = 0; i < w; ++i) {
        gp(a)[i] = (u8)(v >> (i * 8));
    }
    return 0;
}

static unsigned char s_cover[3][0x400];
static unsigned s_calls[3];

static int bus_bridge(void* u, PcPortMipsCpu* c, u32 t)
{
    unsigned f;

    (void)u;
    (void)c;
    for (f = 0; f < 3; ++f) {
        if (t >= kFn[f].lo && t < kFn[f].hi) {
            s_cover[f][(t - kFn[f].lo) / 4] = 1;
            if (t == kFn[f].lo) {
                s_calls[f]++;
            }
            return 0;
        }
    }
    return -1; /* a fetch outside the chain fails the case */
}

static u8 s_base[PSX_RAM_SIZE], s_cpuRam[PSX_RAM_SIZE];
static u8 s_code[kBlobBytes];

static void load_retail(void)
{
    const char* path = getenv("TLIST_BATTLE_BIN") ? getenv("TLIST_BATTLE_BIN")
                                                  : "disc/battle.bin";
    FILE* f = fopen(path, "rb");
    u32 w0, w1, w2;

    if (f == NULL || fseek(f, (long)(kBlobAddr - 0x8006FAF0u), SEEK_SET) != 0 ||
        fread(s_code, 1, kBlobBytes, f) != (size_t)kBlobBytes) {
        FAIL("cannot read the retail chain from battle.bin");
    }
    fclose(f);
    memcpy(&w0, s_code, 4);                      /* 80083FF4 andi a1,a1,0xFF */
    memcpy(&w1, s_code + (0x800841E0u - kBlobAddr), 4); /* addiu sp,sp,-0x50 */
    memcpy(&w2, s_code + (0x80084A7Cu - kBlobAddr), 4); /* addiu sp,sp,-0x28 */
    if (w0 != 0x30A500FFu || w1 != 0x27BDFFB0u || w2 != 0x27BDFFD8u) {
        FAIL("retail chain bytes are not where expected (%08x %08x %08x)", w0, w1, w2);
    }
}

/* ---- state construction ---------------------------------------------------- */
typedef struct {
    unsigned p_present;  /* out of 16 */
    unsigned p_blocked;
    unsigned p_alt;
    unsigned p_matrix;   /* probability a matrix byte is nonzero */
    unsigned p_status;   /* probability of a blocking status bit */
    unsigned group_span; /* groups drawn from [0, group_span) mostly */
    unsigned rank_span;  /* ranks drawn from [0, rank_span) mostly */
    int ctx_alias;       /* 0 none, 1 row target inside the list, 2 +0x2E8 inside */
} Profile;

static u8 chance(unsigned p16) { return (rnd() & 15u) < p16; }

/* A status halfword: blocking when any of 0xC001 is set (each bit alone,
 * together, or random), otherwise a word with only the other bits. */
static u16 status_word(const Profile* pr)
{
    static const u16 kBlock[6] = { 0x0001u, 0x4000u, 0x8000u, 0xC000u, 0xC001u, 0x8001u };

    if (chance(pr->p_status)) {
        return (rnd() & 3u) ? kBlock[rnd() % 6u] : (u16)(rnd() | 0x1u);
    }
    return (u16)(rnd() & 0x3FFEu);
}

static void build_ram(const Profile* pr, u32 actor)
{
    unsigned t;
    u32 ctx = kCtx;

    for (t = kFillLo; t < kFillHi; t += 4) {
        p32(t, rnd());
    }
    for (t = 0; t < 256; ++t) {
        *gp(kPresent + t) = chance(pr->p_present) ? (u8)(1 + (rnd() % 5u)) : 0;
        *gp(kBlocked + t * 0x1Cu) = chance(pr->p_blocked) ? (u8)(1 + (rnd() % 3u)) : 0;
        *gp(kAlt + t * 8u) = chance(pr->p_alt) ? (u8)(1 + (rnd() % 3u)) : 0;
        *gp(kGroup + t * 0x1Cu) = (rnd() % 8u) ? (u8)(rnd() % pr->group_span) : (u8)rnd();
        p16(kRank + t * 0x170u, (rnd() % 8u) ? (u16)(rnd() % pr->rank_span) : (u16)rnd());
        p16(kStatus + t * 0x170u, status_word(pr));
        p16(kAltStatus + t * 0x170u, status_word(pr));
    }
    for (t = 0; t < 0x5000; ++t) {
        *gp(kMatrix + t) = chance(pr->p_matrix) ? (u8)(1 + (rnd() % 255u)) : 0;
    }
    p32(kMatrixPtr, kMatrix);
    if (pr->ctx_alias == 1) {
        ctx = kList + (rnd() % 12u) - 0x3Cu - (actor & 0xFFu) * 0x40u;
    } else if (pr->ctx_alias == 2) {
        ctx = kList + (rnd() % 12u) - 0x2E8u;
    }
    p32(kCtxPtr, ctx);
    if (pr->ctx_alias == 0) {
        for (t = 0; t < 256; ++t) {
            u32 r = rnd();
            *gp(ctx + t * 0x40u + 0x3Cu) = (r & 3u) ? (u8)(r >> 8) % 13u : (u8)(r >> 8);
        }
        *gp(ctx + 0x2E8u) = (u8)rnd();
    }
    memcpy(gp(kBlobAddr), s_code, kBlobBytes);
}

static Profile pick_profile(void)
{
    Profile p;
    static const unsigned kPresent[4] = { 16, 14, 12, 8 };

    p.p_present = kPresent[rnd() & 3u];
    p.p_blocked = (rnd() & 1u) ? 0u : (rnd() % 5u);
    p.p_alt = (rnd() & 1u) ? 0u : (rnd() % 8u);
    p.p_matrix = (rnd() & 1u) ? 0u : (rnd() % 4u);
    p.p_status = (rnd() & 1u) ? 0u : (rnd() % 6u);
    p.group_span = 1u + (rnd() % 4u);
    p.rank_span = 1u + (rnd() % 6u);
    p.ctx_alias = 0;
    return p;
}

static u32 pick_id(int wide)
{
    u32 r = rnd();
    u32 low = (!wide || (r & 3u)) ? (r >> 8) % 11u : (r >> 8) & 0xFFu;

    return (rnd() & 1u) ? low : (low | (rnd() & 0xFFFFFF00u)); /* masked on entry */
}

static unsigned s_checks;

static void run_case(const char* name, int fn, u32 a0, u32 a1)
{
    PcPortMipsCpu cpu;
    PcPortMipsBus bus;
    u32 i, saved[32], ret_cpu, ret_port = 0;
    int rc;
    u32 entry = kFn[fn].lo;

    memset(&bus, 0, sizeof(bus));
    bus.read = bus_read;
    bus.write = bus_write;
    bus.bridge = bus_bridge;
    memcpy(s_base, g_PsxRam, sizeof(s_base));

    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = a0;
    cpu.gpr[5] = a1;
    cpu.gpr[29] = kStackTop;
    cpu.gpr[31] = 0xFFFFFFFCu;
    for (i = 16; i <= 23; ++i) {
        cpu.gpr[i] = 0x51000000u + i;
    }
    cpu.gpr[30] = 0x5100001Eu;
    memcpy(saved, cpu.gpr, sizeof(saved));
    rc = PcPortMipsRun(&cpu, entry, 0xFFFFFFFCu, 400000);
    if (rc != PC_PORT_MIPS_HALTED) {
        FAIL("%s: retail CPU rc=%d (%s)", name, rc, cpu.error);
    }
    if (cpu.gpr[29] != kStackTop) {
        FAIL("%s: retail sp not restored", name);
    }
    for (i = 16; i <= 23; ++i) {
        if (cpu.gpr[i] != saved[i]) FAIL("%s: retail clobbered s%u", name, i - 16);
    }
    if (cpu.gpr[30] != saved[30]) FAIL("%s: retail clobbered fp", name);
    ret_cpu = cpu.gpr[2];
    memcpy(s_cpuRam, g_PsxRam, sizeof(s_cpuRam));

    memcpy(g_PsxRam, s_base, sizeof(s_base));
    switch (fn) {
    case 0: ret_port = func_80083FF4(a0, a1); break;
    case 1: ret_port = func_800841E0(a0); break;
    default: func_80084A7C(a0); break;
    }
    if (fn == 0 && ret_cpu != ret_port) {
        FAIL("%s: func_80083FF4 returned retail=%u port=%u", name, ret_cpu, ret_port);
    }
    if (fn == 1 && ret_cpu != ret_port) {
        FAIL("%s: func_800841E0 returned retail=%u port=%u", name, ret_cpu, ret_port);
    }
    /* Whole-RAM compare, minus the CPU's stack window. */
    {
        u32 lo = kStackLo & 0x1FFFFFu, hi = lo + 0x10000u;

        if (memcmp(s_cpuRam, g_PsxRam, lo) != 0 ||
            memcmp(s_cpuRam + hi, g_PsxRam + hi, PSX_RAM_SIZE - hi) != 0) {
            for (i = 0; i < PSX_RAM_SIZE; ++i) {
                if (i >= lo && i < hi) continue;
                if (s_cpuRam[i] != g_PsxRam[i]) {
                    FAIL("%s: RAM differs at 0x%08x retail=%02x port=%02x (a0=%08x a1=%08x)",
                         name, 0x80000000u + i, s_cpuRam[i], g_PsxRam[i], a0, a1);
                }
            }
        }
    }
    s_checks++;
}

int main(void)
{
    unsigned n, f, missing = 0, covered = 0, total = 0;
    char name[80];
    static const unsigned kCases[3] = { 5000, 4000, 4000 };

    load_retail();
    for (f = 0; f < 3; ++f) {
        for (n = 0; n < kCases[f]; ++n) {
            Profile pr = pick_profile();
            u32 a0 = pick_id(f == 0 ? 1 : (rnd() % 16u == 0));
            u32 a1 = pick_id(1);

            if (f == 2 && (rnd() % 4u) == 0) {
                pr.ctx_alias = 1 + (int)(rnd() & 1u);
            }
            if (f != 0 && (rnd() % 8u) == 0) {
                a0 = (a0 & ~0xFFu) | (rnd() % 3u); /* side < 3 half */
            }
            memset(g_PsxRam, 0, sizeof(g_PsxRam));
            build_ram(&pr, a0);
            snprintf(name, sizeof(name), "fn%u #%u a0=%08x a1=%08x", f, n, a0, a1);
            run_case(name, (int)f, a0, a1);
        }
    }

    /* Coverage gate: every instruction of all three retail functions. */
    for (f = 0; f < 3; ++f) {
        u32 i, words = (kFn[f].hi - kFn[f].lo) / 4;

        for (i = 0; i < words; ++i) {
            total++;
            if (s_cover[f][i]) {
                covered++;
            } else {
                fprintf(stderr, "TLIST COVERAGE: instruction 0x%08x never ran\n",
                        kFn[f].lo + i * 4u);
                missing++;
            }
        }
        if (s_calls[f] == 0) {
            fprintf(stderr, "TLIST COVERAGE: fn %u never entered\n", f);
            missing++;
        }
    }
    if (missing != 0) {
        FAIL("coverage gate: %u item(s) missing", missing);
    }
    printf("TLIST coverage: %u/%u retail instructions across 3 functions\n", covered, total);
    printf("TLIST checks: %u cases (retail CPU result + whole-RAM identical)\n", s_checks);
    return 0;
}
