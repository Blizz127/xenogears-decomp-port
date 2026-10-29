/*
 * Differential certificate for the native port body of battle func_80077698
 * (Attack-ring arrow builder, src/battle/main7_port.c).
 *
 * CPU side: the retail bytes of 0x80077698 (disc/battle.bin) AND the retail
 * SDK bzero (0x8003F8E8) / SetPolyG3 (0x80043C74) (disc/SLUS_006.64) run on the
 * MIPS adapter.  Native side: the port body linked against the port's
 * SetPolyG3 (the one-line compat body in pc_port/src/psyq_compat_leaf.inc,
 * defined in production by retail_leaf_adapters.c; PsyCross only declares it;
 * USE_EXTENDED_PRIM_POINTERS=0) and libc bzero.
 *
 * Per case both sides start from the same whole-RAM snapshot with the same
 * synthetic state (block pointer, buffer index, flag pointer, random block
 * contents); every byte of guest RAM outside the stack window, the host-side
 * block/flag buffers and the callee-saved registers/sp are compared.  Retail
 * bytes are only read from disc/, never stored in the repo.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "battle_mips_adapter.h"

extern void func_80077698(void);

u8 g_PsxRam[PSX_RAM_SIZE];
u8 g_PsxScratchpad[4096];

enum {
    kRetailAddr = 0x80077698u,
    kRetailBytes = 186 * 4,
    kBzeroAddr = 0x8003F8E8u,
    kBzeroBytes = 0x30,
    kG3Addr = 0x80043C74u,
    kG3Bytes = 0x14,
    kPtrBlock = 0x800C3E24u,  /* D_800C3E24 */
    kBufIdx = 0x800CCB34u,    /* D_800CCB34 */
    kPtrFlag = 0x800D2D28u,   /* D_800D2D28 */
    kGuestBlk = 0x80110000u,
    kGuestFlag = 0x80130000u,
    kStackLo = 0x801FE000u,
    kHostSize = 0x400
};
static u8 s_hostBlk[kHostSize] __attribute__((aligned(16)));
static u8 s_hostFlag[kHostSize] __attribute__((aligned(16)));
static u8 s_snapshot[PSX_RAM_SIZE];

static unsigned s_checks;
static unsigned s_armHit[4][2]; /* [arrow i][half j] seen in retail output */

static u8* bus_ptr(u32 a, unsigned w)
{
    if (a >= 0x80000000u && (uint64_t)a + w <= 0x80000000u + 0x200000u) {
        return g_PsxRam + (a & 0x1FFFFFu);
    }
    if ((uintptr_t)a >= (uintptr_t)s_hostBlk &&
        (uint64_t)a + w <= (uintptr_t)s_hostBlk + sizeof(s_hostBlk)) {
        return (u8*)(uintptr_t)a;
    }
    if ((uintptr_t)a >= (uintptr_t)s_hostFlag &&
        (uint64_t)a + w <= (uintptr_t)s_hostFlag + sizeof(s_hostFlag)) {
        return (u8*)(uintptr_t)a;
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
    (void)u;
    (void)c;
    (void)t;
    return 0; /* bzero and SetPolyG3 are retail code too */
}

#define FAIL(...)                                    \
    do {                                             \
        fprintf(stderr, "G3ARROWS FAIL: " __VA_ARGS__); \
        fputc('\n', stderr);                         \
        exit(1);                                     \
    } while (0)

static void load_at(const char* path, u32 base, u32 addr, unsigned n,
                    const char* what)
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
    const char* bb = getenv("G3_BATTLE_BIN") ? getenv("G3_BATTLE_BIN") : "disc/battle.bin";
    const char* sl = getenv("G3_SLUS") ? getenv("G3_SLUS") : "disc/SLUS_006.64";

    load_at(bb, 0x8006FAF0u, kRetailAddr, kRetailBytes, "func_80077698 (battle.bin)");
    load_at(sl, 0x8000F800u, kBzeroAddr, kBzeroBytes, "bzero (SLUS_006.64)");
    load_at(sl, 0x8000F800u, kG3Addr, kG3Bytes, "SetPolyG3 (SLUS_006.64)");
}

static u32 s_rng = 1;
static u32 rnd(void)
{
    s_rng ^= s_rng << 13;
    s_rng ^= s_rng >> 17;
    s_rng ^= s_rng << 5;
    return s_rng;
}

typedef struct {
    u32 blk;         /* value of D_800C3E24 */
    u32 flag;        /* value of D_800D2D28 */
    u8 idx;          /* D_800CCB34 */
    int zero_fill;   /* block pre-state 0x00 instead of random */
} Setup;

static void put32(u32 a, u32 v) { memcpy(g_PsxRam + (a & 0x1FFFFFu), &v, 4); }

static void apply(const Setup* s)
{
    unsigned i;

    memcpy(g_PsxRam, s_snapshot, PSX_RAM_SIZE);
    put32(kPtrBlock, s->blk);
    put32(kPtrFlag, s->flag);
    g_PsxRam[kBufIdx & 0x1FFFFFu] = s->idx;
    /* Random (non-zero) pre-state around and inside the block and flag
     * targets, so bzero/overrun/misplaced stores are all visible. */
    for (i = 0; i < kHostSize; ++i) {
        s_hostBlk[i] = s->zero_fill ? 0 : (u8)(rnd() | 1);
        s_hostFlag[i] = (u8)(rnd() | 1);
    }
    for (i = 0; i < 0x4800; ++i) {
        g_PsxRam[((kGuestBlk - 0x200) & 0x1FFFFFu) + i] = s->zero_fill ? 0 : (u8)(rnd() | 1);
        g_PsxRam[((kGuestFlag - 0x200) & 0x1FFFFFu) + i] = (u8)(rnd() | 1);
    }
}

typedef struct {
    u8 ram[PSX_RAM_SIZE];
    u8 blk[kHostSize];
    u8 flag[kHostSize];
} State;
static State s_exp;

static void capture(State* st)
{
    memcpy(st->ram, g_PsxRam, PSX_RAM_SIZE);
    memcpy(st->blk, s_hostBlk, kHostSize);
    memcpy(st->flag, s_hostFlag, kHostSize);
}

/* Expected vertex bytes (independent of both sides) of arrow i, for the
 * non-vacuity/coverage check of the retail result. */
static const u16 kV[4][6] = {
    { 0xC0, 0x70, 0xB0, 0x78, 0xB0, 0x68 },
    { 0xA0, 0x90, 0x98, 0x80, 0xA8, 0x80 },
    { 0x80, 0x70, 0x90, 0x68, 0x90, 0x78 },
    { 0xA0, 0x50, 0x98, 0x60, 0xA8, 0x60 },
};

static void note_coverage(const State* st, const Setup* s, const char* name)
{
    const u8* blk;
    unsigned i, j;
    u32 flag_at = s->flag + 0xC6u;

    /* The flag store lands inside the arrows (fuzz alias): the table no
     * longer describes the block, the differential compare still applies. */
    if (flag_at >= s->blk && flag_at < s->blk + 0xE0u) {
        return;
    }
    if (s->blk >= 0x80000000u) {
        blk = st->ram + (s->blk & 0x1FFFFFu);
    } else {
        blk = st->blk + (s->blk - (u32)(uintptr_t)s_hostBlk);
    }
    for (i = 0; i < 4; ++i) {
        for (j = 0; j < 2; ++j) {
            const u8* p = blk + (i * 2 + j) * 0x1C;
            const u16* v = (const u16*)(p + 8);
            u16 got[6] = { v[0], v[1], v[4], v[5], v[8], v[9] };

            if (memcmp(got, kV[i], sizeof(got)) != 0 || p[3] != 6 || p[7] != 0x30 ||
                p[4] != 0xFF || p[0xC] != 0x40 || p[0x14] != 0x40) {
                FAIL("%s: retail arrow %u/%u is not the expected primitive", name, i, j);
            }
            s_armHit[i][j]++;
        }
    }
}

static void run_case(const char* name, const Setup* s)
{
    PcPortMipsCpu cpu;
    PcPortMipsBus bus;
    u32 i, saved[32];
    int rc;
    u32 seed = s_rng;
    State* got = (State*)malloc(sizeof(State));

    memset(&bus, 0, sizeof(bus));
    bus.read = bus_read;
    bus.write = bus_write;
    bus.bridge = bus_bridge;

    /* ---- retail CPU ---- */
    apply(s);
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[29] = 0x801FF000u;
    cpu.gpr[31] = 0xFFFFFFFCu;
    for (i = 16; i <= 23; ++i) {
        cpu.gpr[i] = 0x51000000u + i;
    }
    cpu.gpr[30] = 0x5100001Eu;
    memcpy(saved, cpu.gpr, sizeof(saved));
    rc = PcPortMipsRun(&cpu, kRetailAddr, 0xFFFFFFFCu, 400000);
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
    capture(&s_exp);
    note_coverage(&s_exp, s, name);

    /* ---- native port body, same state (rng replay) ---- */
    s_rng = seed;
    apply(s);
    func_80077698();
    capture(got);

    for (i = 0; i < PSX_RAM_SIZE; ++i) {
        if (0x80000000u + i >= kStackLo) {
            break;
        }
        if (s_exp.ram[i] != got->ram[i]) {
            FAIL("%s: RAM 0x%08X retail=%02X native=%02X", name, 0x80000000u + i,
                 s_exp.ram[i], got->ram[i]);
        }
    }
    for (i = 0; i < kHostSize; ++i) {
        if (s_exp.blk[i] != got->blk[i]) {
            FAIL("%s: host block +0x%X retail=%02X native=%02X", name, i,
                 s_exp.blk[i], got->blk[i]);
        }
        if (s_exp.flag[i] != got->flag[i]) {
            FAIL("%s: host flag buf +0x%X retail=%02X native=%02X", name, i,
                 s_exp.flag[i], got->flag[i]);
        }
    }
    free(got);
    s_checks++;
}

int main(void)
{
    Setup s;
    unsigned seed;
    static const u32 kFlagAlias[] = { 0x1E, 0x1F, 0x20, 0x21, 0x2A }; /* +0xC6 -> 0xE4,0xE5,0xE6,0xE7,0xF0 */

    load_retail();
    memcpy(s_snapshot, g_PsxRam, PSX_RAM_SIZE);

    /* 1. production shape: guest block + guest flag block. */
    s_rng = 0xA77B0001u;
    s = (Setup){ kGuestBlk, kGuestFlag, 0, 0 };
    run_case("guest/guest idx0", &s);
    s.idx = 1;
    run_case("guest/guest idx1", &s);
    s.idx = 0xFF;
    run_case("guest/guest idx0xFF", &s);
    s.zero_fill = 1;
    run_case("guest/guest zero-fill", &s);

    /* 2. host-address block and/or flag block (native allocators). */
    s = (Setup){ (u32)(uintptr_t)s_hostBlk + 0x20, (u32)(uintptr_t)s_hostFlag, 1, 0 };
    run_case("host/host", &s);
    s = (Setup){ kGuestBlk, (u32)(uintptr_t)s_hostFlag + 0x10, 0, 0 };
    run_case("guest/host", &s);
    s = (Setup){ (u32)(uintptr_t)s_hostBlk + 0x40, kGuestFlag, 1, 0 };
    run_case("host/guest", &s);

    /* 3. the flag store aliases the block's own tail: write order decides. */
    for (seed = 0; seed < sizeof(kFlagAlias) / sizeof(kFlagAlias[0]); ++seed) {
        s = (Setup){ kGuestBlk, kGuestBlk + kFlagAlias[seed], (u8)(2 + seed), 0 };
        run_case("flag aliases block (guest)", &s);
        s = (Setup){ (u32)(uintptr_t)s_hostBlk + 0x40,
                     (u32)(uintptr_t)s_hostBlk + 0x40 + kFlagAlias[seed], (u8)(2 + seed), 0 };
        run_case("flag aliases block (host)", &s);
    }

    /* 4. block covering the buffer-index global D_800CCB34 (offset 0xB4):
     *    bzero clears it before the read. */
    s = (Setup){ kBufIdx - 0xB4, kGuestFlag, 0x5A, 0 };
    run_case("block covers D_800CCB34", &s);
    s = (Setup){ kBufIdx - 0xE4, kGuestFlag, 0x5A, 0 };  /* idx byte lands on +0xE4 */
    run_case("block +0xE4 is D_800CCB34", &s);

    /* 5. fuzz: random 4-aligned guest/host block + flag, random idx. */
    for (seed = 1; seed <= 200; ++seed) {
        s_rng = seed * 2654435761u + 7u;
        s.idx = (u8)rnd();
        s.zero_fill = (seed % 7) == 0;
        if (seed % 3 == 0) {
            s.blk = (u32)(uintptr_t)s_hostBlk + ((rnd() % 0x100) & ~3u);
        } else {
            s.blk = kGuestBlk + ((rnd() % 0x4000) & ~3u);
        }
        if (seed % 5 == 0) {
            s.flag = (u32)(uintptr_t)s_hostFlag + (rnd() % 0x100);
        } else if (seed % 5 == 1) {
            s.flag = s.blk + (rnd() % 0x100);
        } else {
            s.flag = kGuestFlag + (rnd() % 0x4000);
        }
        run_case("fuzz", &s);
    }

    /* Coverage gate: all 4 switch arms x both halves executed on the retail
     * CPU (checked against the independent vertex table), in >= 100 cases. */
    {
        unsigned i, j;

        for (i = 0; i < 4; ++i) {
            for (j = 0; j < 2; ++j) {
                if (s_armHit[i][j] < 100) {
                    FAIL("arrow %u half %u executed only %u times", i, j, s_armHit[i][j]);
                }
            }
        }
    }
    printf("G3ARROWS PORT PASS checks=%u (4 arrows x 2 halves, retail CPU incl. retail bzero/SetPolyG3)\n",
           s_checks);
    return 0;
}
