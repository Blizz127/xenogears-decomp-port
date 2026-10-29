/*
 * Differential certificate for the native port body of battle func_800B1720
 * (TMD-style primitive list -> GPU packets, src/battle/main77.c port arm).
 *
 * CPU side: the retail bytes of 0x800B1720 (disc/battle.bin) AND of the retail
 * SetSemiTrans/SetShadeTex (0x80043BFC..0x80043C4C, disc/SLUS_006.64) run on
 * the MIPS adapter, so the packet code byte is set by retail code, not a stub.
 * Native side: the port body linked against the real PsyCross SetShadeTex /
 * SetSemiTrans (USE_EXTENDED_PRIM_POINTERS=0, as in production).
 *
 * Per case both sides start from identical input and 0xA5-filled output; the
 * whole input region (the source tpage is rewritten in place), the whole
 * output region and the callee-saved registers/sp are compared.  Retail
 * bytes are only read from disc/, never stored in the repo.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "battle_mips_adapter.h"

extern void func_800B1720(u8* tmd, u8* prim, s32 abr, s32 tge);

u8 g_PsxRam[PSX_RAM_SIZE];
u8 g_PsxScratchpad[4096];

enum {
    kRetailAddr = 0x800B1720u,
    kRetailBytes = 480 * 4,
    kLeafAddr = 0x80043BFCu,
    kLeafBytes = 0x50,
    kGuestIn = 0x80100000u,
    kGuestOut = 0x80120000u,
    kRegion = 0x1000
};
/* Host-side buffers live at static (-no-pie, < 4 GiB) addresses: the CPU bus
 * maps those u32 addresses straight to the buffers. */
static u8 s_hostIn[kRegion] __attribute__((aligned(16)));
static u8 s_hostOut[kRegion] __attribute__((aligned(16)));

static unsigned s_checks;
static unsigned s_kindHit[2][0x120]; /* [abr != 0][type] seen on the CPU */

static u8* bus_ptr(u32 a, unsigned w)
{
    if (a >= 0x80000000u && (uint64_t)a + w <= 0x80000000u + 0x200000u) {
        return g_PsxRam + (a & 0x1FFFFFu);
    }
    if ((uintptr_t)a >= (uintptr_t)s_hostIn &&
        (uint64_t)a + w <= (uintptr_t)s_hostIn + sizeof(s_hostIn)) {
        return (u8*)(uintptr_t)a;
    }
    if ((uintptr_t)a >= (uintptr_t)s_hostOut &&
        (uint64_t)a + w <= (uintptr_t)s_hostOut + sizeof(s_hostOut)) {
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
    return 0; /* everything, SetShadeTex/SetSemiTrans included, is retail code */
}

#define FAIL(...)                                  \
    do {                                           \
        fprintf(stderr, "B1720 FAIL: " __VA_ARGS__); \
        fputc('\n', stderr);                       \
        exit(1);                                   \
    } while (0)

static void load_retail(void)
{
    FILE* f = fopen(getenv("B1720_BATTLE_BIN") ? getenv("B1720_BATTLE_BIN")
                                               : "disc/battle.bin", "rb");

    if (f == NULL || fseek(f, (long)(kRetailAddr - 0x8006FAF0u), SEEK_SET) != 0 ||
        fread(g_PsxRam + (kRetailAddr & 0x1FFFFFu), 1, kRetailBytes, f) !=
            (size_t)kRetailBytes) {
        FAIL("cannot read retail func_800B1720 from battle.bin");
    }
    fclose(f);
    f = fopen(getenv("B1720_SLUS") ? getenv("B1720_SLUS") : "disc/SLUS_006.64",
              "rb");
    if (f == NULL || fseek(f, (long)(kLeafAddr - 0x8000F800u), SEEK_SET) != 0 ||
        fread(g_PsxRam + (kLeafAddr & 0x1FFFFFu), 1, kLeafBytes, f) !=
            (size_t)kLeafBytes) {
        FAIL("cannot read retail SetSemiTrans/SetShadeTex from SLUS_006.64");
    }
    fclose(f);
}

/* ---- deterministic input generation ------------------------------------ */
static u32 s_rng = 1;
static u32 rnd(void)
{
    s_rng ^= s_rng << 13;
    s_rng ^= s_rng >> 17;
    s_rng ^= s_rng << 5;
    return s_rng;
}

/* Realistic output length (tag len = words after the tag) per mode&0x1C. */
static u8 olen_for(unsigned mode)
{
    static const u8 k[8] = { 4, 7, 5, 9, 6, 9, 8, 12 };
    return k[(mode >> 2) & 7];
}

typedef struct {
    u8 buf[kRegion];
    unsigned size;  /* bytes of packet data after the 0x18-byte header */
    u32 count;
    unsigned base;  /* offset of the TMD header inside buf */
} List;

/* Header: +0x10 = offset of the first packet (0x18), +0x14 = count.  Packet:
 * byte0 olen, byte1 ilen (words after the header word), byte2 flags (bit 0 =
 * lit), byte3 mode; the rest is random (colours, uv, clut, tpage). */
static void build_list(List* l, const unsigned* modes, const unsigned* flags,
                       unsigned n, int fuzz_len)
{
    unsigned i, off = 0x18, j;

    for (j = 0; j < sizeof(l->buf); ++j) {
        l->buf[j] = (u8)rnd();
    }
    memset(l->buf, 0, 0x18);
    for (i = 0; i < n; ++i) {
        unsigned mode = modes[i];
        unsigned ilen = 9 + ((rnd() >> 4) & 3);       /* 40..52 bytes: covers +0x22 */
        unsigned olen = olen_for(mode);

        if (fuzz_len) {
            olen = 3 + (rnd() >> 8) % 12;
            ilen = 4 + (rnd() >> 8) % 9;
        }
        l->buf[off + 0] = (u8)olen;
        l->buf[off + 1] = (u8)ilen;
        l->buf[off + 2] = (u8)((flags[i] & 1) | (fuzz_len ? (rnd() & 0xFE) : 0));
        l->buf[off + 3] = (u8)(mode | (fuzz_len ? (rnd() & 0xE3) : 0));
        off += (ilen + 1) * 4;
        if (off + 0x40 > sizeof(l->buf)) {
            FAIL("list too large");
        }
    }
    l->size = off - 0x18;
    l->count = n;
    l->base = 0;
    ((u32*)l->buf)[4] = 0x18;
    ((u32*)l->buf)[5] = n;
}

typedef struct {
    u32 in_addr;  /* u32 address handed to both sides */
    u32 out_addr;
    u8* in_host;
    u8* out_host;
    unsigned in_off; /* tmd header offset inside in_host */
} Place;

static void place_reset(Place* pl, const List* l)
{
    memcpy(pl->in_host, l->buf, kRegion);
    memset(pl->out_host, 0xA5, kRegion);
}

static void run_case(const char* name, const List* l, Place* pl, s32 abr,
                     s32 tge)
{
    u8 exp_in[kRegion], exp_out[kRegion];
    PcPortMipsCpu cpu;
    PcPortMipsBus bus;
    u32 i, saved[32];
    int rc;
    unsigned t;

    memset(&bus, 0, sizeof(bus));
    bus.read = bus_read;
    bus.write = bus_write;
    bus.bridge = bus_bridge;

    /* ---- retail CPU ---- */
    place_reset(pl, l);
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = pl->in_addr;
    cpu.gpr[5] = pl->out_addr;
    cpu.gpr[6] = (u32)abr;
    cpu.gpr[7] = (u32)tge;
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
    memcpy(exp_in, pl->in_host, kRegion);
    memcpy(exp_out, pl->out_host, kRegion);
    /* Coverage bookkeeping from the (pre-run) input list (header at base). */
    {
        const u8* h = l->buf + l->base;
        const u8* q = h + *(const u32*)(h + 0x10);
        u32 n = *(const u32*)(h + 0x14);

        for (i = 0; i < n; ++i) {
            t = (q[3] & 0x1Cu) | (((q[2] ^ 1u) & 1u) << 8);
            s_kindHit[abr != 0][t]++;
            q += (q[1] + 1u) * 4u;
        }
    }

    /* ---- native port body on identical (restored) state ---- */
    place_reset(pl, l);
    func_800B1720(pl->in_host + pl->in_off, pl->out_host, abr, tge);

    if (memcmp(exp_out, pl->out_host, kRegion) != 0) {
        for (i = 0; i < kRegion; ++i) {
            if (exp_out[i] != pl->out_host[i]) {
                FAIL("%s abr=%d tge=%d: output byte +0x%X retail=%02X native=%02X",
                     name, abr, tge, i, exp_out[i], pl->out_host[i]);
            }
        }
    }
    if (memcmp(exp_in, pl->in_host, kRegion) != 0) {
        for (i = 0; i < kRegion; ++i) {
            if (exp_in[i] != pl->in_host[i]) {
                FAIL("%s abr=%d tge=%d: input byte +0x%X retail=%02X native=%02X",
                     name, abr, tge, i, exp_in[i], pl->in_host[i]);
            }
        }
    }
    /* Non-vacuity: the retail run wrote output iff the list was non-empty. */
    {
        unsigned changed = 0;
        for (i = 0; i < kRegion; ++i) {
            changed += exp_out[i] != 0xA5;
        }
        if ((*(const u32*)(l->buf + l->base + 0x14) != 0) != (changed != 0)) {
            FAIL("%s: retail wrote %u output bytes for count=%u", name, changed,
                 l->count);
        }
    }
    s_checks++;
}

static const unsigned kModes16[16] = { 0x00, 0x04, 0x08, 0x0C, 0x10, 0x14, 0x18,
                                       0x1C, 0x00, 0x04, 0x08, 0x0C, 0x10, 0x14,
                                       0x18, 0x1C };
static const unsigned kFlags16[16] = { 1, 1, 1, 1, 1, 1, 1, 1,
                                       0, 0, 0, 0, 0, 0, 0, 0 };

static void arg_matrix(const char* name, const List* l, Place* pl)
{
    static const s32 abrs[] = { 0, 1, 2, 3, 4, 5, (s32)0xFFFFFFFF, 0x7FFFFFFF,
                                (s32)0x80000000 };
    static const s32 tges[] = { 1, 0, 0x100, 0xFF, 0x1FF, -1 };
    unsigned a, g;

    for (a = 0; a < sizeof(abrs) / sizeof(abrs[0]); ++a) {
        for (g = 0; g < sizeof(tges) / sizeof(tges[0]); ++g) {
            run_case(name, l, pl, abrs[a], tges[g]);
        }
    }
}

int main(void)
{
    static List list;
    static Place guest_to_host, host_to_guest, guest_to_guest, overlap, real_host,
                 real_guest;
    unsigned modes[24], flags[24], n, k, seed;

    load_retail();

    guest_to_host = (Place){ kGuestIn, (u32)(uintptr_t)s_hostOut,
                             g_PsxRam + (kGuestIn & 0x1FFFFFu), s_hostOut, 0 };
    host_to_guest = (Place){ (u32)(uintptr_t)s_hostIn, kGuestOut, s_hostIn,
                             g_PsxRam + (kGuestOut & 0x1FFFFFu), 0 };
    guest_to_guest = (Place){ kGuestIn, kGuestOut,
                              g_PsxRam + (kGuestIn & 0x1FFFFFu),
                              g_PsxRam + (kGuestOut & 0x1FFFFFu), 0 };

    /* 1. canonical list: all 16 type values, realistic olen, production
     *    (0,1) plus the whole abr/tge matrix. */
    s_rng = 0x1720B1B1u;
    build_list(&list, kModes16, kFlags16, 16, 0);
    run_case("canon guest->host (0,1)", &list, &guest_to_host, 0, 1);
    run_case("canon host->guest (0,1)", &list, &host_to_guest, 0, 1);
    run_case("canon guest->guest (0,1)", &list, &guest_to_guest, 0, 1);
    arg_matrix("canon guest->host", &list, &guest_to_host);
    arg_matrix("canon host->guest", &list, &host_to_guest);

    /* 1b. the REAL input of func_800BD098: D_8001C76C + 0xC (retail
     *     SLUS_006.64, 0x170 bytes) as the native raw host copy, output a
     *     HeapAlloc-shaped block in guest RAM, called (0,1) exactly as the
     *     caller does; plus (0,0), (2,1) and the same data in guest RAM. */
    {
        FILE* f = fopen(getenv("B1720_SLUS"), "rb");

        memset(&list, 0, sizeof(list));
        if (f == NULL || fseek(f, (long)(0x8001C76Cu - 0x8000F800u), SEEK_SET) != 0 ||
            fread(list.buf, 1, 0x170, f) != 0x170) {
            FAIL("cannot read D_8001C76C from SLUS_006.64");
        }
        fclose(f);
        list.base = 0xC;
        if (*(u32*)(list.buf + 0xC + 0x14) == 0 || *(u32*)(list.buf + 0xC + 0x14) > 64) {
            FAIL("unexpected D_8001C76C header (count %u)", *(u32*)(list.buf + 0xC + 0x14));
        }
        real_host = (Place){ (u32)(uintptr_t)s_hostIn + 0xC, kGuestOut, s_hostIn,
                             g_PsxRam + (kGuestOut & 0x1FFFFFu), 0xC };
        real_guest = (Place){ kGuestIn + 0xC, kGuestOut, g_PsxRam + (kGuestIn & 0x1FFFFFu),
                              g_PsxRam + (kGuestOut & 0x1FFFFFu), 0xC };
        run_case("real D_8001C76C host->guest (0,1)", &list, &real_host, 0, 1);
        run_case("real D_8001C76C host->guest (0,0)", &list, &real_host, 0, 0);
        run_case("real D_8001C76C host->guest (2,1)", &list, &real_host, 2, 1);
        run_case("real D_8001C76C guest->guest (0,1)", &list, &real_guest, 0, 1);
        printf("real input: %u packets from D_8001C76C+0xC\n",
               *(u32*)(list.buf + 0xC + 0x14));
    }

    /* 2. count == 0 (reads +0x10/+0x14 only, writes nothing). */
    build_list(&list, kModes16, kFlags16, 0, 0);
    run_case("empty (0,1)", &list, &guest_to_host, 0, 1);
    run_case("empty (3,1)", &list, &host_to_guest, 3, 1);

    /* 3. each type alone (single-packet lists, both abr states). */
    for (k = 0; k < 16; ++k) {
        modes[0] = kModes16[k];
        flags[0] = kFlags16[k];
        build_list(&list, modes, flags, 1, 0);
        run_case("single (0,1)", &list, &guest_to_host, 0, 1);
        run_case("single (2,1)", &list, &host_to_guest, 2, 1);
        run_case("single (4,0)", &list, &guest_to_guest, 4, 0);
    }

    /* 4. fuzz: random type order, random olen/ilen (so prim and packet strides
     *    differ), random flag/mode upper bits, random abr/tge. */
    for (seed = 1; seed <= 120; ++seed) {
        static const s32 abrs[] = { 0, 1, 2, 3, 4, (s32)0xFFFFFFFF };
        static const s32 tges[] = { 0, 1, 0x100, 0xFF, 0x1FF };
        Place* pl;

        s_rng = seed * 2654435761u + 7u;
        n = 1 + rnd() % 20;
        for (k = 0; k < n; ++k) {
            modes[k] = (rnd() & 0x1C);
            flags[k] = rnd();
        }
        build_list(&list, modes, flags, n, seed & 1);
        pl = seed % 3 == 0 ? &guest_to_host
                           : seed % 3 == 1 ? &host_to_guest : &guest_to_guest;
        run_case("fuzz", &list, pl, abrs[rnd() % 6], tges[rnd() % 5]);
    }

    /* 5. in-place overlap: the output buffer starts inside the packet list,
     *    so the order of each byte read/write matters (both sides must
     *    interleave exactly as retail's lbu/sb do). */
    for (seed = 1; seed <= 40; ++seed) {
        s_rng = seed * 40503u + 99u;
        n = 2 + rnd() % 10;
        for (k = 0; k < n; ++k) {
            modes[k] = (rnd() & 0x1C);
            flags[k] = rnd();
        }
        build_list(&list, modes, flags, n, seed & 1);
        overlap = (Place){ kGuestIn, kGuestIn + 0x20 + 4 * (rnd() % 24),
                           g_PsxRam + (kGuestIn & 0x1FFFFFu),
                           g_PsxRam + (kGuestIn & 0x1FFFFFu), 0 };
        /* One shared region: out_host == in_host, so place_reset's fill would
         * wipe the list; build it by hand for this case. */
        {
            u8 exp[kRegion];
            PcPortMipsCpu cpu;
            PcPortMipsBus bus;
            int rc;
            s32 abr = (s32)(rnd() % 5), tge = (s32)(rnd() & 1);
            u8* base = overlap.in_host;

            memset(&bus, 0, sizeof(bus));
            bus.read = bus_read;
            bus.write = bus_write;
            bus.bridge = bus_bridge;
            memcpy(base, list.buf, kRegion);
            PcPortMipsCpuInit(&cpu, &bus);
            cpu.gpr[4] = overlap.in_addr;
            cpu.gpr[5] = overlap.out_addr;
            cpu.gpr[6] = (u32)abr;
            cpu.gpr[7] = (u32)tge;
            cpu.gpr[29] = 0x801FF000u;
            cpu.gpr[31] = 0xFFFFFFFCu;
            rc = PcPortMipsRun(&cpu, kRetailAddr, 0xFFFFFFFCu, 400000);
            if (rc != PC_PORT_MIPS_HALTED) {
                FAIL("overlap: retail CPU rc=%d (%s)", rc, cpu.error);
            }
            memcpy(exp, base, kRegion);
            memcpy(base, list.buf, kRegion);
            func_800B1720(overlap.in_host, base + (overlap.out_addr - kGuestIn),
                          abr, tge);
            if (memcmp(exp, base, kRegion) != 0) {
                unsigned i;
                for (i = 0; i < kRegion; ++i) {
                    if (exp[i] != base[i]) {
                        FAIL("overlap seed %u: byte +0x%X retail=%02X native=%02X",
                             seed, i, exp[i], base[i]);
                    }
                }
            }
            s_checks++;
        }
    }

    /* Coverage gate: every type value was executed with abr == 0, and every
     * textured type (the TSB rewrite) with abr != 0. */
    {
        static const unsigned all[16] = { 0x00, 0x04, 0x08, 0x0C, 0x10, 0x14,
                                          0x18, 0x1C, 0x100, 0x104, 0x108,
                                          0x10C, 0x110, 0x114, 0x118, 0x11C };
        unsigned i;

        for (i = 0; i < 16; ++i) {
            if (s_kindHit[0][all[i]] == 0 || s_kindHit[1][all[i]] == 0) {
                FAIL("type 0x%X not executed for abr==0 (%u) / abr!=0 (%u)",
                     all[i], s_kindHit[0][all[i]], s_kindHit[1][all[i]]);
            }
        }
    }
    printf("B1720 PORT PASS checks=%u (all 16 type values x abr==0/!=0 executed on retail CPU)\n",
           s_checks);
    return 0;
}
