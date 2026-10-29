/*
 * Differential certificate for the native port body of battle func_800B89FC
 * ("open the battle command window", src/battle/mainc104_port.c).
 *
 * CPU side: the retail bytes of 0x800B89FC (disc/battle.bin) run on the MIPS
 * adapter.  Native side: the port body.  Both start from identical guest RAM
 * and identical arguments.  Every callee -- battle-overlay functions and the
 * main-executable services -- is replaced by a recording spy on BOTH sides:
 * on the CPU the adapter bridge intercepts the retail call target, on the
 * native side the spy is linked in place of the callee.  Both spies run the
 * same logic (spy_do): they log the call id, its arguments and a snapshot of
 * the globals the body orders against (so store-vs-call order is checked, not
 * just the final memory), apply the same optional state perturbation (to catch
 * a port that caches what retail re-reads), and return the same controlled
 * value.  After both runs the call logs and all of guest RAM (outside the CPU
 * stack window) must be identical, as must the CPU's callee-saved registers.
 *
 * Coverage gate: every instruction of the retail function except the dead
 * tail of the `break 1` spin must be fetched at least once by some case.
 * The `break` case is checked separately: retail stops on `break`, the port
 * aborts (caught here) after the same first callee.
 *
 * Retail bytes are only read from disc/, never stored in the repo.
 */
#include <setjmp.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "battle_mips_adapter.h"
#include "battle_host_guest.h"

void func_800B89FC(s32 resume, s32 index, s32 column, s32 target);

u8 g_PsxRam[PSX_RAM_SIZE];
u8 g_PsxScratchpad[4096];

#define FAIL(...)                                   \
    do {                                            \
        fprintf(stderr, "B89FC FAIL: " __VA_ARGS__); \
        fputc('\n', stderr);                        \
        exit(1);                                    \
    } while (0)

enum {
    kRetailAddr = 0x800B89FCu,
    kRetailBytes = 0x308,
    kStackTop = 0x801FF000u,
    kStackLo = 0x801F0000u, /* CPU stack window, excluded from the RAM compare */
    kTask = 0x80140000u,
    kTask2 = 0x80140200u,
    kSprA = 0x80141000u,
    kSprB = 0x80141400u,
    kSprC = 0x80141800u,
    kHeapBuf = 0x80150000u,
    kTable = 0x800CCB3Cu, /* D_800C3EB0 + 0x8C8C */
    kRows = 0x800C3EB0u
};
static u8 s_soundEntry[16]; /* SoundLoadWdsFile result: raw -no-pie address */
static u8 s_hostWord[16];   /* a raw host address word for func_8001EE68 */

/* ---- guest RAM helpers --------------------------------------------------- */
static u8* gp(u32 a) { return g_PsxRam + (a & 0x1FFFFFu); }
static u32 g32(u32 a) { u32 v; memcpy(&v, gp(a), 4); return v; }
static u16 g16(u32 a) { u16 v; memcpy(&v, gp(a), 2); return v; }
static void p32(u32 a, u32 v) { memcpy(gp(a), &v, 4); }

static u32 s_rng = 0x1234567u;
static u32 rnd(void)
{
    s_rng ^= s_rng << 13;
    s_rng ^= s_rng >> 17;
    s_rng ^= s_rng << 5;
    return s_rng;
}

/* ---- callee spies --------------------------------------------------------- */
enum {
    S_BC3F8, S_BE790, S_BED4C, S_BEFF4, S_BF3E8, S_C0F70, S_BC454, S_EE68,
    S_BF2B8, S_B8354, S_ASETIDX, S_ADEC, S_HALLOC, S_AREAD, S_SOUND, S_3BDFC,
    S_HFREE, S_B8D04, S_BFBA0, S_BF0B4, S_BEE2C, S_21BF8, S_BF0C4, S_COUNT
};
static const struct { const char* name; u32 addr; unsigned nargs; } kSpy[S_COUNT] = {
    [S_BC3F8] = { "func_800BC3F8", 0x800BC3F8u, 1 },
    [S_BE790] = { "func_800BE790", 0x800BE790u, 0 },
    [S_BED4C] = { "func_800BED4C", 0x800BED4Cu, 0 },
    [S_BEFF4] = { "func_800BEFF4", 0x800BEFF4u, 1 },
    [S_BF3E8] = { "func_800BF3E8", 0x800BF3E8u, 1 },
    [S_C0F70] = { "func_800C0F70", 0x800C0F70u, 0 },
    [S_BC454] = { "func_800BC454", 0x800BC454u, 1 },
    [S_EE68] = { "func_8001EE68", 0x8001EE68u, 1 },
    [S_BF2B8] = { "func_800BF2B8", 0x800BF2B8u, 1 },
    [S_B8354] = { "func_800B8354", 0x800B8354u, 0 },
    [S_ASETIDX] = { "ArchiveSetIndex", 0x80028470u, 2 },
    [S_ADEC] = { "ArchiveDecodeAlignedSize", 0x800288ECu, 1 },
    [S_HALLOC] = { "HeapAlloc", 0x80031BDCu, 2 },
    [S_AREAD] = { "ArchiveReadFileToBuffer", 0x800295D8u, 4 },
    [S_SOUND] = { "SoundLoadWdsFile", 0x80037FD8u, 2 },
    [S_3BDFC] = { "func_8003BDFC", 0x8003BDFCu, 1 },
    [S_HFREE] = { "HeapFree", 0x800320E8u, 1 },
    [S_B8D04] = { "func_800B8D04", 0x800B8D04u, 0 },
    [S_BFBA0] = { "func_800BFBA0", 0x800BFBA0u, 0 },
    [S_BF0B4] = { "func_800BF0B4", 0x800BF0B4u, 1 },
    [S_BEE2C] = { "func_800BEE2C", 0x800BEE2Cu, 3 },
    [S_21BF8] = { "func_80021BF8", 0x80021BF8u, 2 },
    [S_BF0C4] = { "func_800BF0C4", 0x800BF0C4u, 1 },
};

typedef struct {
    u32 id;
    u32 a[4];
    u32 snap[14];
} Call;
enum { kMaxCalls = 96 };
static Call s_log[kMaxCalls];
static unsigned s_nlog;

/* Scenario the spies act on (identical for both sides). */
typedef struct {
    u32 idx;
    u32 ee68;       /* func_8001EE68 result */
    unsigned wait_n; /* func_8003BDFC returns nonzero this many times */
    u32 wait_val;   /* the nonzero value (low 16 bits may be zero) */
    u32 sound_ret;
    int flip_id;    /* spy whose first call rewrites row[idx]+8 */
    u8 flip_val;
    int step_id;    /* spy whose first call rewrites D_800C360C */
    u32 step_val;
    int swap_mode;  /* at func_800BF3E8: 1 task+4 := SprB; 2 D_800C3610 := Task2 */
} Scenario;
static Scenario sc;
static unsigned s_waits;
static int s_flipped, s_stepped;

static u32 spy_do(int id, u32 a0, u32 a1, u32 a2, u32 a3)
{
    Call* c;
    u32 task, ret = 0, args[4] = { a0, a1, a2, a3 };
    unsigned i;

    if (s_nlog >= kMaxCalls) {
        FAIL("call log overflow");
    }
    c = &s_log[s_nlog++];
    memset(c, 0, sizeof(*c));
    c->id = (u32)id;
    for (i = 0; i < kSpy[id].nargs; ++i) {
        c->a[i] = args[i];
    }
    task = g32(0x800C3610u);
    c->snap[0] = task;
    c->snap[1] = g16(0x800C3608u);
    c->snap[2] = *gp(0x800C3624u);
    c->snap[3] = *gp(0x800C3620u);
    c->snap[4] = *gp(0x800C3622u);
    c->snap[5] = g16(0x800C3DF0u);
    c->snap[6] = g32(0x800C3A6Cu);
    c->snap[7] = g32(0x800C3618u);
    c->snap[8] = g32(0x800C361Cu);
    c->snap[9] = g32(0x800C360Cu);
    c->snap[10] = g32(task + 0x20);
    c->snap[11] = g32(task + 0x40);
    c->snap[12] = g32(task + 4);
    c->snap[13] = g32(task + 0x24);

    /* Perturbations (retail re-reads these; a port that cached them differs). */
    if (sc.flip_id == id && !s_flipped) {
        s_flipped = 1;
        *gp(kRows + sc.idx * 28u + 8) = sc.flip_val;
    }
    if (sc.step_id == id && !s_stepped) {
        s_stepped = 1;
        p32(0x800C360Cu, sc.step_val);
    }

    switch (id) {
    case S_BED4C:
        ret = kTask;
        break;
    case S_BEFF4: {
        u32 t = g32(0x800C3610u);

        p32(t + 0x24, a0);
        p32(t + 4, g32(kTable + a0 * 4u));
        ret = g32(t + 4);
        break;
    }
    case S_BF3E8:
        if (sc.swap_mode == 1) {
            p32(g32(0x800C3610u) + 4, kSprB);
        } else if (sc.swap_mode == 2) {
            p32(kTask2 + 4, kSprB);
            p32(0x800C3610u, kTask2);
        }
        break;
    case S_EE68:
        ret = sc.ee68;
        break;
    case S_ADEC:
        ret = 0x1234u;
        break;
    case S_HALLOC:
        ret = kHeapBuf;
        break;
    case S_SOUND:
        ret = sc.sound_ret;
        break;
    case S_3BDFC:
        ret = s_waits < sc.wait_n ? sc.wait_val : 0;
        s_waits++;
        break;
    default:
        break;
    }
    return ret;
}

/* ---- native-side spies: the symbols the port TU calls --------------------- */
#define NORM(p) BG_ADDR(p)

void func_800BC3F8(u32 v) { spy_do(S_BC3F8, v, 0, 0, 0); }
void func_800BE790(void) { spy_do(S_BE790, 0, 0, 0, 0); }
u32 func_800BED4C(void) { return spy_do(S_BED4C, 0, 0, 0, 0); }
void func_800BEFF4(u32 slot) { spy_do(S_BEFF4, slot, 0, 0, 0); }
void func_800BF3E8(u8* spr) { spy_do(S_BF3E8, NORM(spr), 0, 0, 0); }
void func_800C0F70(void) { spy_do(S_C0F70, 0, 0, 0, 0); }
void func_800BC454(u16 v) { spy_do(S_BC454, v, 0, 0, 0); }
int func_8001EE68(u8* p) { return (int)spy_do(S_EE68, NORM(p), 0, 0, 0); }
void func_800BF2B8(u8* spr) { spy_do(S_BF2B8, NORM(spr), 0, 0, 0); }
void func_800B8354(void) { spy_do(S_B8354, 0, 0, 0, 0); }
int ArchiveSetIndex(int d, int e) { return (int)spy_do(S_ASETIDX, (u32)d, (u32)e, 0, 0); }
int ArchiveDecodeAlignedSize(unsigned int e) { return (int)spy_do(S_ADEC, e, 0, 0, 0); }
void* HeapAlloc(unsigned int size, unsigned int flags)
{
    spy_do(S_HALLOC, size, flags, 0, 0);
    return gp(kHeapBuf);
}
int ArchiveReadFileToBuffer(int i, void* b, unsigned int a2, unsigned int fl)
{
    return (int)spy_do(S_AREAD, (u32)i, NORM(b), a2, fl);
}
void* SoundLoadWdsFile(void* p, int mode)
{
    return (void*)(uintptr_t)spy_do(S_SOUND, NORM(p), (u32)mode, 0, 0);
}
s16 func_8003BDFC(s32 flags) { return (s16)spy_do(S_3BDFC, (u32)flags, 0, 0, 0); }
unsigned int HeapFree(void* p) { return spy_do(S_HFREE, NORM(p), 0, 0, 0); }
void func_800B8D04(void) { spy_do(S_B8D04, 0, 0, 0, 0); }
void func_800BFBA0(void) { spy_do(S_BFBA0, 0, 0, 0, 0); }
void func_800BF0B4(u32 v) { spy_do(S_BF0B4, v, 0, 0, 0); }
void func_800BEE2C(u32 a, u32 b, u32 c) { spy_do(S_BEE2C, a, b, c, 0); }
void func_80021BF8(void* p, s32 cb) { spy_do(S_21BF8, NORM(p), (u32)cb, 0, 0); }
void func_800BF0C4(u8* spr) { spy_do(S_BF0C4, NORM(spr), 0, 0, 0); }

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

static unsigned char s_cover[kRetailBytes / 4];

static int bus_bridge(void* u, PcPortMipsCpu* c, u32 t)
{
    int id;

    (void)u;
    if (t >= kRetailAddr && t < kRetailAddr + kRetailBytes) {
        s_cover[(t - kRetailAddr) / 4] = 1;
        return 0;
    }
    for (id = 0; id < S_COUNT; ++id) {
        if (kSpy[id].addr == t) {
            c->gpr[2] = spy_do(id, c->gpr[4], c->gpr[5], c->gpr[6], c->gpr[7]);
            return 1;
        }
    }
    return -1; /* an unexpected call target fails the case */
}

static u8 s_base[PSX_RAM_SIZE], s_cpuRam[PSX_RAM_SIZE];
static u8 s_code[kRetailBytes];

static void load_retail(void)
{
    const char* path = getenv("B89FC_BATTLE_BIN") ? getenv("B89FC_BATTLE_BIN")
                                                  : "disc/battle.bin";
    FILE* f = fopen(path, "rb");

    if (f == NULL || fseek(f, (long)(kRetailAddr - 0x8006FAF0u), SEEK_SET) != 0 ||
        fread(s_code, 1, kRetailBytes, f) != (size_t)kRetailBytes) {
        FAIL("cannot read retail func_800B89FC from battle.bin");
    }
    fclose(f);
}

typedef struct {
    s32 resume, column, target;
    u32 idx;
    u8 row8;
    u32 gfx;         /* D_800C3618 */
    int slot_mode;   /* D_800C361C relative to the sprite's packed slot */
    u8 bank_loaded;  /* D_800C3622 */
    int task_busy;   /* D_800C3610 != 0 at entry */
} Case;

static const u32 kSprs[3] = { kSprA, kSprB, kSprC };

static u32 slot_of(u32 spr)
{
    return ((g32(spr + 0xAC) & 3u) << 2) | (g32(spr + 0xA8) >> 30);
}

/* Build the case's initial guest RAM into g_PsxRam. */
static void build_ram(const Case* k)
{
    unsigned i;
    u32 spr, slot;

    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memcpy(gp(kRetailAddr), s_code, kRetailBytes);
    for (i = 0x800C3000u; i < 0x800C5000u; i += 4) {
        p32(i, rnd());
    }
    for (i = 0x80140000u; i < 0x80144000u; i += 4) {
        p32(i, rnd());
    }
    for (i = 0; i < 64; ++i) {
        p32(kTable + i * 4u, kSprs[i % 3]);
    }
    for (i = 0; i < 3; ++i) {
        u32 pp = 0x80142000u + i * 0x100u, w = 0x80143000u + i * 0x100u;

        p32(kSprs[i] + 0x24, pp);
        p32(pp, w);
        if (i == 2) {
            memset(s_hostWord, 0x5A, sizeof(s_hostWord));
            p32(pp, (u32)(uintptr_t)s_hostWord); /* raw host-address word */
        }
    }
    p32(0x800C3610u, k->task_busy ? kTask2 : 0);
    *gp(kRows + k->idx * 28u + 8) = k->row8;
    p32(0x800C3618u, k->gfx);
    *gp(0x800C3622u) = k->bank_loaded;
    p32(0x800C360Cu, rnd() % 10u);
    p32(0x800C3A6Cu, rnd() & 1u ? rnd() : 0u);
    /* D_800C361C relative to the sprite the compare will see. */
    spr = sc.swap_mode ? kSprB : g32(kTable + k->idx * 4u);
    slot = slot_of(spr);
    switch (k->slot_mode) {
    case 0: p32(0x800C361Cu, slot); break;                 /* match */
    case 1: p32(0x800C361Cu, slot ^ 4u); break;            /* differs in the AC bits */
    case 2: p32(0x800C361Cu, slot ^ 1u); break;            /* differs in the A8 bits */
    case 3: p32(0x800C361Cu, slot | 0x100u); break;        /* high bits set */
    default: p32(0x800C361Cu, rnd()); break;
    }
}

static unsigned s_checks;

static void compare_logs(const char* name, const Call* a, unsigned na,
                         const Call* b, unsigned nb)
{
    unsigned i, j;

    if (na != nb) {
        FAIL("%s: call count retail=%u port=%u", name, na, nb);
    }
    for (i = 0; i < na; ++i) {
        if (memcmp(&a[i], &b[i], sizeof(Call)) != 0) {
            fprintf(stderr, "B89FC FAIL: %s: call #%u differs: retail %s port %s\n",
                    name, i, kSpy[a[i].id].name, kSpy[b[i].id].name);
            for (j = 0; j < 4; ++j) {
                fprintf(stderr, "  a%u retail=%08x port=%08x\n", j, a[i].a[j], b[i].a[j]);
            }
            for (j = 0; j < 14; ++j) {
                fprintf(stderr, "  snap%u retail=%08x port=%08x\n", j, a[i].snap[j],
                        b[i].snap[j]);
            }
            exit(1);
        }
    }
}

static unsigned s_hit[S_COUNT];

static void run_case(const char* name, const Case* k)
{
    static Call retailLog[kMaxCalls];
    PcPortMipsCpu cpu;
    PcPortMipsBus bus;
    u32 i, saved[32];
    unsigned nr, off;
    int rc;

    memset(&bus, 0, sizeof(bus));
    bus.read = bus_read;
    bus.write = bus_write;
    bus.bridge = bus_bridge;

    sc.idx = k->idx;
    build_ram(k);
    memcpy(s_base, g_PsxRam, sizeof(s_base));

    /* ---- retail CPU ---- */
    s_nlog = 0; s_waits = 0; s_flipped = 0; s_stepped = 0;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = (u32)k->resume;
    cpu.gpr[5] = k->idx;
    cpu.gpr[6] = (u32)k->column;
    cpu.gpr[7] = (u32)k->target;
    cpu.gpr[29] = kStackTop;
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
    if (cpu.gpr[29] != kStackTop) {
        FAIL("%s: retail sp not restored", name);
    }
    for (i = 16; i <= 23; ++i) {
        if (cpu.gpr[i] != saved[i]) FAIL("%s: retail clobbered s%u", name, i - 16);
    }
    if (cpu.gpr[30] != saved[30]) FAIL("%s: retail clobbered fp", name);
    nr = s_nlog;
    memcpy(retailLog, s_log, sizeof(Call) * nr);
    memcpy(s_cpuRam, g_PsxRam, sizeof(s_cpuRam));
    for (off = 0; off < nr; ++off) {
        s_hit[retailLog[off].id]++;
    }

    /* ---- native port body on the identical state ---- */
    memcpy(g_PsxRam, s_base, sizeof(s_base));
    s_nlog = 0; s_waits = 0; s_flipped = 0; s_stepped = 0;
    func_800B89FC(k->resume, (s32)k->idx, k->column, k->target);

    compare_logs(name, retailLog, nr, s_log, s_nlog);
    /* Whole-RAM compare, minus the CPU's stack window. */
    memset(s_cpuRam + (kStackLo & 0x1FFFFFu), 0, 0x10000);
    memset(g_PsxRam + (kStackLo & 0x1FFFFFu), 0, 0x10000);
    for (i = 0; i < PSX_RAM_SIZE; ++i) {
        if (s_cpuRam[i] != g_PsxRam[i]) {
            FAIL("%s: RAM differs at 0x%08x retail=%02x port=%02x", name,
                 0x80000000u + i, s_cpuRam[i], g_PsxRam[i]);
        }
    }
    s_checks++;
}

static sigjmp_buf s_jb;
static void on_abort(int sig) { (void)sig; siglongjmp(s_jb, 1); }

/* D_800C3610 != 0: retail spins on `break 1`, the port aborts. */
static void run_break(void)
{
    PcPortMipsCpu cpu;
    PcPortMipsBus bus;
    Call retailLog[kMaxCalls];
    unsigned nr;
    struct sigaction sa, old;
    Case k = { .resume = 1, .column = 5, .target = 6, .idx = 2, .row8 = 0,
               .task_busy = 1 };

    memset(&bus, 0, sizeof(bus));
    bus.read = bus_read;
    bus.write = bus_write;
    bus.bridge = bus_bridge;
    memset(&sc, 0, sizeof(sc));
    sc.flip_id = sc.step_id = -1;
    sc.idx = k.idx;
    build_ram(&k);
    memcpy(s_base, g_PsxRam, sizeof(s_base));
    s_nlog = 0;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = 1; cpu.gpr[5] = 2; cpu.gpr[6] = 5; cpu.gpr[7] = 6;
    cpu.gpr[29] = kStackTop;
    cpu.gpr[31] = 0xFFFFFFFCu;
    if (PcPortMipsRun(&cpu, kRetailAddr, 0xFFFFFFFCu, 4000) !=
            PC_PORT_MIPS_UNSUPPORTED || strstr(cpu.error, "break") == NULL) {
        FAIL("break case: retail did not stop on break (%s)", cpu.error);
    }
    nr = s_nlog;
    memcpy(retailLog, s_log, sizeof(Call) * nr);
    memcpy(g_PsxRam, s_base, sizeof(s_base));
    s_nlog = 0;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_abort;
    sigaction(SIGABRT, &sa, &old);
    if (sigsetjmp(s_jb, 1) == 0) {
        func_800B89FC(1, 2, 5, 6);
        FAIL("break case: port body did not abort");
    }
    sigaction(SIGABRT, &old, NULL);
    compare_logs("break", retailLog, nr, s_log, s_nlog);
    if (nr != 1 || retailLog[0].id != S_BC3F8) {
        FAIL("break case: expected exactly func_800BC3F8 before the spin");
    }
    s_checks++;
}

static const u32 kIdx[] = { 0, 1, 5, 15, 16, 17, 31, 33 };
static const u32 kWait[] = { 0x1234u, 0x8000u, 0x10000u, 0xFFFFu, 0x80000000u };

static void scenario_reset(u32 ee68, unsigned wait_n, u32 wait_val)
{
    memset(&sc, 0, sizeof(sc));
    sc.flip_id = sc.step_id = -1;
    sc.ee68 = ee68;
    sc.wait_n = wait_n;
    sc.wait_val = wait_val;
    sc.sound_ret = (rnd() & 3u) ? (u32)(uintptr_t)s_soundEntry : 0u;
}

int main(void)
{
    unsigned a, b, c, d, e, n;
    char name[96];
    Case k;

    load_retail();
    run_break();

    /* Structured sweep: every index x resume x row byte x EE68 x gfx/slot. */
    for (a = 0; a < sizeof(kIdx) / sizeof(kIdx[0]); ++a) {
        for (b = 0; b < 3; ++b) {                       /* resume 0, 1, 0x100 */
            for (c = 0; c < 3; ++c) {                   /* row8 0, 1, 0x80 */
                for (d = 0; d < 3; ++d) {               /* EE68 0, 1, 0x80 */
                    for (e = 0; e < 6; ++e) {           /* gfx/slot/bank mix */
                        memset(&k, 0, sizeof(k));
                        k.idx = kIdx[a];
                        k.resume = b == 0 ? 0 : (b == 1 ? 1 : 0x100);
                        k.column = (s32)rnd();
                        k.target = (s32)rnd();
                        k.row8 = c == 0 ? 0 : (c == 1 ? 1 : 0x80);
                        k.gfx = (e == 0) ? 0u : kHeapBuf;
                        k.slot_mode = (e <= 1) ? 0 : (int)(e - 2) % 4;
                        k.bank_loaded = (e & 1) ? (u8)(1 + (rnd() & 0x7F)) : 0;
                        scenario_reset(d == 0 ? 0u : (d == 1 ? 1u : 0x80u),
                                       e % 4, kWait[(a + e) % 5]);
                        snprintf(name, sizeof(name),
                                 "sweep idx=%u res=%d row8=%u ee68=%u e=%u",
                                 k.idx, k.resume, k.row8, sc.ee68, e);
                        run_case(name, &k);
                    }
                }
            }
        }
    }

    /* Perturbation sweep: spies rewrite what retail re-reads. */
    for (n = 0; n < 4000; ++n) {
        memset(&k, 0, sizeof(k));
        k.idx = kIdx[rnd() % 8];
        k.resume = (rnd() & 1) ? (s32)(rnd() % 3) : 0;
        k.column = (s32)rnd();
        k.target = (s32)rnd();
        k.row8 = (rnd() & 1) ? (u8)(rnd() & 0xFF) : 0;
        k.gfx = (rnd() & 1) ? kHeapBuf : 0;
        k.slot_mode = (int)(rnd() % 5);
        k.bank_loaded = (rnd() & 1) ? (u8)rnd() : 0;
        scenario_reset((rnd() & 1) ? (rnd() | 1u) : 0u, rnd() % 5,
                       kWait[rnd() % 5]);
        sc.flip_id = (int)(rnd() % (S_COUNT + 1)) - 1;
        sc.flip_val = (u8)(rnd() & 1 ? (rnd() & 0xFF) : 0);
        if (rnd() & 1) {
            sc.step_id = (int)(rnd() % S_COUNT);
            sc.step_val = rnd() % 10u;
        }
        sc.swap_mode = (int)(rnd() % 3);
        snprintf(name, sizeof(name), "perturb #%u idx=%u", n, k.idx);
        run_case(name, &k);
    }

    /* Coverage gate: every retail instruction except the dead tail of the
     * break spin (j + delay nop at 0x800B8A48/0x800B8A4C) was fetched. */
    {
        unsigned i, missing = 0, covered = 0;

        for (i = 0; i < kRetailBytes / 4; ++i) {
            u32 pc = kRetailAddr + i * 4u;

            if (pc == 0x800B8A48u || pc == 0x800B8A4Cu) {
                continue;
            }
            if (s_cover[i]) {
                covered++;
            } else {
                fprintf(stderr, "B89FC COVERAGE: instruction 0x%08x never ran\n", pc);
                missing++;
            }
        }
        for (i = 0; i < S_COUNT; ++i) {
            if (s_hit[i] == 0) {
                fprintf(stderr, "B89FC COVERAGE: callee %s never called\n", kSpy[i].name);
                missing++;
            }
        }
        if (missing != 0) {
            FAIL("coverage gate: %u item(s) missing", missing);
        }
        printf("B89FC coverage: %u/%u retail instructions, all %d callees hit\n",
               covered, kRetailBytes / 4 - 2, S_COUNT);
    }
    printf("B89FC checks: %u cases (retail CPU log + whole-RAM identical)\n", s_checks);
    return 0;
}
