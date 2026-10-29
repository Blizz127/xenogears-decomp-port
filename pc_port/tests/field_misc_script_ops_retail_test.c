/* Retail differential for the hand-written field script-opcode handlers in
 * src/field/main/misc.c (XENO_PC_PORT arms).  The production misc.c TU is
 * linked unmodified.  For every case both sides start from byte-identical
 * guest memory:
 *   retail: the pinned disc/field.bin bytes executed on PcPortMipsRun;
 *   native: the port C body, with the guest buffers identity-mapped at their
 *           guest addresses and the globals mirrored into byte regions that
 *           alias the retail symbols (asm .set).
 * Callees (script-argument decoders, particle-bank init, the patch writer) are
 * capturing spies on both sides; call order, arguments and return feed are
 * compared along with every byte of every touched window and the callee-saved
 * registers.  No retail bytes are stored in the tree. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include "battle_mips_adapter.h"

#define FIELD_BASE 0x8006faf0u
#define FIELD_SIZE 260862u
#define STACK 0x801FF000u
#define HALT 0xFFFFFFFCu

/* ---- guest windows (byte-identical at start, compared at end) ---------- */
#define BUF_BASE 0x80120000u
#define BUF_SIZE 0x60000u
#define SCRIPT 0x80120000u    /* g_FieldScriptVMCurScriptData */
#define CURACT 0x80130000u    /* g_FieldScriptVMCurActor (ActorData) */
#define GSTATE 0x80140000u    /* g_pGameState */
#define FACTORS 0x80150000u   /* g_FieldActors, stride 0x5C */
#define ADATA 0x80160000u     /* per-actor ActorData, stride 0x200 */
#define SDATA 0x80170000u     /* per-actor sprite data, stride 0x200 */
#define NACT 16u

#define P_GSTATE 0x8005A39Cu
#define P_CURACT 0x800B0078u
#define P_SCRIPT 0x800ADC00u
#define P_FACTORS 0x800AFB10u

/* Native-visible globals live in byte regions; retail symbol names alias
 * into them so the production TU sees the same layout as the guest. */
#define RS_BASE 0x800AF980u
#define RS_SIZE 0x100u
#define RA_BASE 0x800ADB00u
#define RA_SIZE 0x100u
#define RB_BASE 0x800AFC00u
#define RB_SIZE 0x100u
#define RC_BASE 0x800B0080u
#define RC_SIZE 0x80u
#define RP_BASE 0x800B02CCu
#define RP_SIZE 0x3C0u
#define RD_BASE 0x800B2200u
#define RD_SIZE 0x200u
#define RE_BASE 0x800C3A00u
#define RE_SIZE 0x80u
#define RF_BASE 0x8005A440u
#define RF_SIZE 0x40u
#define STR2(x) #x
#define STR(x) STR2(x)
#define REGION(id) uint8_t id[id##_SIZE] __attribute__((aligned(16), used))
#define ALIAS(sym, reg, addr) \
    __asm__(".globl " #sym "\n.set " #sym ", " #reg "+(" STR(addr) "-" STR(reg##_BASE) ")")
REGION(RS); REGION(RA); REGION(RB); REGION(RC); REGION(RP); REGION(RD); REGION(RE); REGION(RF);
ALIAS(D_800ADB70, RA, 0x800ADB70); ALIAS(D_800ADB74, RA, 0x800ADB74);
ALIAS(D_800ADB80, RA, 0x800ADB80); ALIAS(D_800ADBDC, RA, 0x800ADBDC);
ALIAS(D_800ADBFC, RA, 0x800ADBFC);
ALIAS(g_FieldScriptMaxInstructionCount, RB, 0x800AFC7C);
ALIAS(D_800B00C0, RC, 0x800B00C0);
ALIAS(g_FieldDefaultParticleBanks, RP, 0x800B02CC);
ALIAS(D_800B221C, RD, 0x800B221C); ALIAS(g_PlayerActorIndex, RD, 0x800B226C);
ALIAS(D_800B22E2, RD, 0x800B22E2); ALIAS(D_800B2324, RD, 0x800B2324);
ALIAS(D_800B2326, RD, 0x800B2326); ALIAS(D_800B2328, RD, 0x800B2328);
ALIAS(D_800B233E, RD, 0x800B233E); ALIAS(D_800B234E, RD, 0x800B234E);
ALIAS(D_800B2374, RD, 0x800B2374); ALIAS(D_800B2378, RD, 0x800B2378);
ALIAS(D_800B237C, RD, 0x800B237C); ALIAS(D_800B2380, RD, 0x800B2380);
ALIAS(D_800B2384, RD, 0x800B2384);
ALIAS(D_800C3A20, RE, 0x800C3A20); ALIAS(D_800C3A22, RE, 0x800C3A22);
ALIAS(D_800C3A24, RE, 0x800C3A24); ALIAS(D_800C3A26, RE, 0x800C3A26);
ALIAS(D_800C3A28, RE, 0x800C3A28); ALIAS(D_800C3A2A, RE, 0x800C3A2A);
ALIAS(D_800C3A2C, RE, 0x800C3A2C); ALIAS(D_800C3A2E, RE, 0x800C3A2E);
ALIAS(D_800C3A30, RE, 0x800C3A30); ALIAS(D_800C3A32, RE, 0x800C3A32);
ALIAS(D_800C3A34, RE, 0x800C3A34); ALIAS(D_800C3A36, RE, 0x800C3A36);
ALIAS(D_800C3A38, RE, 0x800C3A38); ALIAS(D_800C3A3A, RE, 0x800C3A3A);
ALIAS(D_8005A444, RF, 0x8005A444);
ALIAS(g_Scene, RS, 0x800AF990);
/* Pointer globals: host pointers natively (identity-mapped guest addresses),
 * u32 guest words in the retail image. */
uintptr_t g_pGameState, g_FieldScriptVMCurActor, g_FieldScriptVMCurScriptData, g_FieldActors;

typedef struct { uint32_t base, size; uint8_t* nat; } Region;
static const Region regions[] = {
    { RS_BASE, RS_SIZE, RS }, { RA_BASE, RA_SIZE, RA }, { RB_BASE, RB_SIZE, RB }, { RC_BASE, RC_SIZE, RC },
    { RP_BASE, RP_SIZE, RP }, { RD_BASE, RD_SIZE, RD }, { RE_BASE, RE_SIZE, RE },
    { RF_BASE, RF_SIZE, RF },
};
#define NREG (sizeof(regions) / sizeof(regions[0]))

/* ---- machinery ---------------------------------------------------------- */
static uint8_t ram[0x200000], *natbuf;
static uint8_t field_image[FIELD_SIZE];
static unsigned cases, checks;
static const char* cur_name;
static PcPortMipsCpu* active_cpu;
static uint32_t cov_lo, cov_hi;
static uint8_t cov[4096];

static void fail(const char* why) {
    fprintf(stderr, "FIELD MISC OPS FAIL fn=%s case=%u %s\n", cur_name, cases, why);
    printf("FIELD MISC OPS FAIL fn=%s case=%u %s\n", cur_name, cases, why);
    exit(1);
}
static void require(int ok, const char* why) { checks++; if (!ok) fail(why); }
static uint8_t* gp(uint32_t a) { return ram + (a & 0x1FFFFFu); }
static uint32_t r32(uint32_t a) { uint32_t v; memcpy(&v, gp(a), 4); return v; }
static void w32(uint32_t a, uint32_t v) { memcpy(gp(a), &v, 4); }
static void w16(uint32_t a, uint16_t v) { memcpy(gp(a), &v, 2); }

static uint64_t rng_s = 0x9E3779B97F4A7C15ull;
static uint32_t rnd(void) {
    rng_s ^= rng_s << 13; rng_s ^= rng_s >> 7; rng_s ^= rng_s << 17;
    return (uint32_t)(rng_s >> 16);
}
static uint32_t rnd_range(uint32_t n) { return rnd() % n; }
static int32_t rnd_val(void) {
    static const int32_t spec[] = { 0, 1, -1, 2, 3, 0x7F, 0x80, 0xFF, 0x100, 0x7FFF, 0x8000, 0xFFFF,
                                    0x10000, 0x7FFFFFFF, (int32_t)0x80000000, 0x12345, -0x8000 };
    return (rnd() & 1) ? spec[rnd_range(sizeof(spec) / sizeof(spec[0]))] : (int32_t)rnd();
}

/* ---- boundary spies ----------------------------------------------------- */
enum { S_ARG1 = 1, S_ARG2, S_ARG3, S_ARG4, S_ARG5, S_VMARG, S_VMACTOR, S_PBANK, S_PATCH, NSPY };
typedef struct { uint32_t id, a0, a1; } Event;
static struct { Event ev[16]; unsigned n; } trace;
static int32_t feed[16];
static void spy(uint32_t id, uint32_t a0, uint32_t a1) {
    require(trace.n < 16, "spy count");
    trace.ev[trace.n++] = (Event){ id, a0, a1 };
}
static int32_t spy_ret(uint32_t id, uint32_t a0, uint32_t a1) {
    unsigned k = trace.n;
    spy(id, a0, a1);
    return feed[k];
}
int FieldScriptArgument1(int i, int m) { return spy_ret(S_ARG1, (uint32_t)i, (uint32_t)m); }
int FieldScriptArgument2(int i, int m) { return spy_ret(S_ARG2, (uint32_t)i, (uint32_t)m); }
int FieldScriptArgument3(int i, int m) { return spy_ret(S_ARG3, (uint32_t)i, (uint32_t)m); }
int FieldScriptArgument4(int i, int m) { return spy_ret(S_ARG4, (uint32_t)i, (uint32_t)m); }
int FieldScriptArgument5(int i, int m) { return spy_ret(S_ARG5, (uint32_t)i, (uint32_t)m); }
int FieldScriptVMGetArgument(int i) { return spy_ret(S_VMARG, (uint32_t)i, 0); }
int FieldScriptVMGetActorIndex(int i) { return spy_ret(S_VMACTOR, (uint32_t)i, 0); }
void FieldInitializeDefaultParticleBanks(int i) { spy(S_PBANK, (uint32_t)i, 0); }
void func_8008D2E0(int a, int b) { spy(S_PATCH, (uint32_t)a, (uint32_t)b); }

typedef struct { uint32_t addr, id, nargs, ret; } Target;
static const Target targets[] = {
    { 0x8009CF78, S_ARG1, 2, 1 }, { 0x8009CFBC, S_ARG2, 2, 1 }, { 0x8009D000, S_ARG3, 2, 1 },
    { 0x8009D044, S_ARG4, 2, 1 }, { 0x8009D088, S_ARG5, 2, 1 }, { 0x800ACDEC, S_VMARG, 1, 1 },
    { 0x8009CDB4, S_VMACTOR, 1, 1 }, { 0x800A94A4, S_PBANK, 1, 0 }, { 0x8008D2E0, S_PATCH, 2, 0 },
};

static int read_bus(void* o, uint32_t a, unsigned width, uint32_t* v) {
    (void)o;
    if (a < 0x80000000u || (uint64_t)a + width > 0x80200000u) return -1;
    *v = 0;
    for (unsigned i = 0; i < width; i++) *v |= (uint32_t)gp(a)[i] << (8 * i);
    if (active_cpu && width == 4 && a == active_cpu->pc && a >= cov_lo && a < cov_hi) cov[(a - cov_lo) / 4]++;
    return 0;
}
static int in_window(uint32_t a, unsigned w) {
    if (a >= 0x801FE000u && (uint64_t)a + w <= 0x80200000u) return 1;
    if (a >= BUF_BASE && (uint64_t)a + w <= BUF_BASE + BUF_SIZE) return 1;
    for (unsigned i = 0; i < NREG; i++)
        if (a >= regions[i].base && (uint64_t)a + w <= (uint64_t)regions[i].base + regions[i].size) return 1;
    return 0;
}
static int write_bus(void* o, uint32_t a, unsigned width, uint32_t v) {
    (void)o;
    if (!in_window(a, width)) { fprintf(stderr, "retail stray write %08x w=%u\n", a, width); return -1; }
    for (unsigned i = 0; i < width; i++) gp(a)[i] = (uint8_t)(v >> (8 * i));
    return 0;
}
static int bridge(void* o, PcPortMipsCpu* c, uint32_t t) {
    (void)o;
    for (unsigned i = 0; i < sizeof(targets) / sizeof(targets[0]); i++) {
        if (targets[i].addr != t) continue;
        unsigned k = trace.n;
        spy(targets[i].id, c->gpr[4], targets[i].nargs > 1 ? c->gpr[5] : 0);
        if (targets[i].ret) c->gpr[2] = (uint32_t)feed[k];
        return 1;
    }
    return 0;
}

/* ---- native entry wrappers --------------------------------------------- */
extern void FieldActorCopyPlacement(int, int);
extern void func_80087C34(void), func_80087E98(void), func_80088674(void), func_80088790(void);
extern void func_80088C1C(void), func_80089DCC(void), func_8008AEC8(void);
extern void func_8008DA04(int, int);
extern void func_8008EA58(void), func_8008EC30(void);
extern void func_8008764C(void), func_8008F90C(void);

typedef struct { uint32_t a0, a1; } Args;
static void n_copy(Args a) { FieldActorCopyPlacement((int)a.a0, (int)a.a1); }
static void n_87C34(Args a) { (void)a; func_80087C34(); }
static void n_87E98(Args a) { (void)a; func_80087E98(); }
static void n_88674(Args a) { (void)a; func_80088674(); }
static void n_88790(Args a) { (void)a; func_80088790(); }
static void n_88C1C(Args a) { (void)a; func_80088C1C(); }
static void n_89DCC(Args a) { (void)a; func_80089DCC(); }
static void n_8AEC8(Args a) { (void)a; func_8008AEC8(); }
static void n_8DA04(Args a) { func_8008DA04((int)a.a0, (int)a.a1); }
static void n_8EA58(Args a) { (void)a; func_8008EA58(); }
static void n_8EC30(Args a) { (void)a; func_8008EC30(); }
static void n_8764C(Args a) { (void)a; func_8008764C(); }
static void n_8F90C(Args a) { (void)a; func_8008F90C(); }

/* ---- per-function case generators (write the guest image `ram`) -------- */
static void feed_fill(void) { for (unsigned i = 0; i < 16; i++) feed[i] = rnd_val(); }
static void set_ip(void) { w16(CURACT + 0xCC, (uint16_t)rnd_range(0x7000)); }
static int32_t small(int32_t lo, int32_t hi) { return lo + (int32_t)rnd_range((uint32_t)(hi - lo + 1)); }

static void g_copy(Args* a) {
    a->a0 = rnd_range(NACT); a->a1 = (rnd() & 3) ? rnd_range(NACT) : a->a0;
}
static void g_plain(Args* a) { (void)a; set_ip(); }
static void g_87E98(Args* a) {
    (void)a; set_ip();
    w32(0x800ADBFC, rnd_range(NACT + 1));
    w32(0x8005A444, (rnd() & 1) ? rnd_range(NACT) : (uint32_t)rnd_val());
    feed[0] = (rnd() % 5 == 0) ? 0xFF : (rnd() & 1) ? (int32_t)r32(0x8005A444) : small(0, NACT - 1);
    if ((feed[0] < 0 || feed[0] > (int32_t)(NACT - 1)) && feed[0] != 0xFF) feed[0] &= 0xF;
}
static void g_88674(Args* a) {
    (void)a; set_ip();
    static const int32_t pick[] = { 0, 1, 2, 3, 4, 0xFF, -1, 0x10, 0x20, 0x30, 0x103 };
    feed[0] = (rnd() & 1) ? 0xFF : (int32_t)small(0, 7);
    feed[1] = (rnd() & 3) ? feed[0] : rnd_val();
    feed[2] = (rnd() & 3) ? pick[rnd_range(11)] : rnd_val();
    feed[3] = rnd_val(); feed[4] = rnd_val();
    /* FieldInitializeDefaultParticleBanks gets the clamped first argument */
}
static void g_88790(Args* a) {
    (void)a; set_ip();
    static const int32_t pick[] = { 0, 1, 2, 3, 4, 0xFF, -1, 0x10, 0x20, 0x30, 0x103 };
    feed[0] = (rnd() & 1) ? 0xFF : (int32_t)small(0, 7);
    feed[1] = (rnd() & 3) ? pick[rnd_range(11)] : rnd_val();
    feed[2] = rnd_val(); feed[3] = rnd_val();
}
static void g_88C1C(Args* a) {
    (void)a; set_ip();
    w32(0x800B2384, rnd_range(8)); /* bankIndex, stride 0x78, 8 banks */
}
static void g_89DCC(Args* a) {
    (void)a; set_ip();
    feed[0] = small(-4, 15);
    feed[4] = (rnd() % 3 == 0) ? 0xFF : small(0, 40);
}
static void g_8AEC8(Args* a) {
    (void)a; set_ip();
    feed[0] = small(-3, 30); /* record index is operand 1 */
}
static void g_8DA04(Args* a) {
    set_ip(); a->a0 = (uint32_t)rnd_val(); a->a1 = (uint32_t)rnd_val();
}
static void g_8EA58(Args* a) {
    (void)a; set_ip();
    if (rnd() & 1) w32(0x800ADBDC, 0);
}
static void g_8EC30(Args* a) {
    (void)a; set_ip();
    if (rnd() % 4 == 0) w32(0x800ADBDC, 0);
    if (rnd() % 3) feed[3] = (feed[3] & ~0xFF) | small(0, 5) | (rnd() & 0xF0);
}

static void g_8764C(Args* a) {
    (void)a; set_ip();
    for (unsigned i = 0; i < 2; i++) feed[i] = (rnd() % 3 == 0) ? 9 + (int32_t)rnd_range(2) : small(0, 20);
}
static void g_8F90C(Args* a) {
    (void)a; set_ip();
    for (unsigned i = 0; i < 3; i++) feed[i] = (rnd() % 4 == 0) ? 0 : rnd_val(); /* full range: retail wraps */
    feed[3] = (rnd() % 5 == 0) ? 0 : (rnd() % 4 == 0) ? -1 : (rnd() % 8 == 0) ? rnd_val() : small(-300, 300);
    w32(0x800AF990u + 0xA0, (uint32_t)rnd_val());
    w32(0x800AF990u + 0xA4, (uint32_t)rnd_val());
}

typedef struct {
    const char* name; uint32_t entry; unsigned insns;
    void (*native)(Args); void (*gen)(Args*); unsigned ncases;
    unsigned nregs_ok; /* 1 when a0/a1 are real arguments */
} Fn;
static Fn fns[] = {
    { "FieldActorCopyPlacement", 0x8008D380, 124, n_copy, g_copy, 1200, 1 },
    { "func_80087C34", 0x80087C34, 63, n_87C34, g_plain, 1200, 0 },
    { "func_80087E98", 0x80087E98, 67, n_87E98, g_87E98, 1200, 0 },
    { "func_80088674", 0x80088674, 71, n_88674, g_88674, 1200, 0 },
    { "func_80088790", 0x80088790, 69, n_88790, g_88790, 1200, 0 },
    { "func_80088C1C", 0x80088C1C, 55, n_88C1C, g_88C1C, 1200, 0 },
    { "func_80089DCC", 0x80089DCC, 83, n_89DCC, g_89DCC, 1200, 0 },
    { "func_8008AEC8", 0x8008AEC8, 68, n_8AEC8, g_8AEC8, 1200, 0 },
    { "func_8008DA04", 0x8008DA04, 62, n_8DA04, g_8DA04, 1200, 1 },
    { "func_8008EA58", 0x8008EA58, 118, n_8EA58, g_8EA58, 1200, 0 },
    { "func_8008EC30", 0x8008EC30, 121, n_8EC30, g_8EC30, 2000, 0 },
    { "func_8008764C", 0x8008764C, 109, n_8764C, g_8764C, 1200, 0 },
    { "func_8008F90C", 0x8008F90C, 75, n_8F90C, g_8F90C, 1500, 0 },
};

/* ---- one case ------------------------------------------------------------ */
static void fill_windows(void) {
    unsigned mode = rnd() & 3;
    uint8_t pat = (uint8_t)rnd();
    uint32_t i;
    for (i = 0; i < BUF_SIZE; i += 4) {
        uint32_t v = mode == 0 ? rnd() : mode == 1 ? 0u : mode == 2 ? 0xFFFFFFFFu : (uint32_t)pat * 0x01010101u;
        /* sparse random bytes on top so masks / flags vary */
        if (mode != 0 && (rnd() & 7) == 0) v = rnd();
        w32(BUF_BASE + i, v);
    }
    for (unsigned r = 0; r < NREG; r++)
        for (i = 0; i < regions[r].size; i += 4)
            w32(regions[r].base + i, mode == 0 ? rnd() : mode == 1 ? 0u : mode == 2 ? 0xFFFFFFFFu : (uint32_t)pat * 0x01010101u);
    for (unsigned k = 0; k < NACT; k++) {
        w32(FACTORS + k * 0x5C + 0x4C, ADATA + k * 0x200);
        w32(FACTORS + k * 0x5C + 0x04, SDATA + k * 0x200);
    }
    w32(P_GSTATE, GSTATE); w32(P_CURACT, CURACT); w32(P_SCRIPT, SCRIPT); w32(P_FACTORS, FACTORS);
}
static void mirror_in(void) {
    memcpy(natbuf, gp(BUF_BASE), BUF_SIZE);
    for (unsigned r = 0; r < NREG; r++) memcpy(regions[r].nat, gp(regions[r].base), regions[r].size);
    g_pGameState = GSTATE; g_FieldScriptVMCurActor = CURACT;
    g_FieldScriptVMCurScriptData = SCRIPT; g_FieldActors = FACTORS;
}

typedef struct { Event ev[16]; unsigned n; } Trace;

/* SpriteData is a host-layout struct natively (64-bit pointers: field_0x84 sits
 * at SPR84_HOST, the retail offset is 0x84).  The host-layout offset is checked
 * against the production headers by the runner (-DSPR84_HOST=...).  Swapping the
 * two u16 slots in the native copy of every sprite block lets one identical
 * guest image drive both sides; swapping back restores retail offsets. */
static void sprite_swap(void) {
    for (unsigned k = 0; k < NACT; k++) {
        uint8_t* b = natbuf + (SDATA - BUF_BASE) + k * 0x200;
        uint8_t t[2];
        memcpy(t, b + 0x84, 2); memcpy(b + 0x84, b + SPR84_HOST, 2); memcpy(b + SPR84_HOST, t, 2);
    }
}

static void run_case(Fn* f, unsigned idx) {
    Args a = { 0, 0 };
    cur_name = f->name;
    memset(ram, 0, sizeof(ram));
    memcpy(gp(FIELD_BASE), field_image, FIELD_SIZE);
    fill_windows(); /* windows overlay the (data part of the) overlay image */
    memset(&trace, 0, sizeof(trace));
    feed_fill();
    f->gen(&a);
    /* snapshot for restoring identical start state */
    static uint8_t start[0x200000];
    memcpy(start, ram, sizeof(start));
    static int32_t feed0[16];
    memcpy(feed0, feed, sizeof(feed));

    /* retail */
    trace.n = 0;
    PcPortMipsBus bus = { 0 }; PcPortMipsCpu cpu;
    bus.read = read_bus; bus.write = write_bus; bus.bridge = bridge;
    PcPortMipsCpuInit(&cpu, &bus);
    for (int r = 16; r < 24; r++) cpu.gpr[r] = 0xC0DE0000u + (uint32_t)r;
    cpu.gpr[28] = 0xC0DE001Cu; cpu.gpr[30] = 0xC0DE001Eu;
    cpu.gpr[29] = STACK; cpu.gpr[31] = HALT; cpu.gpr[4] = a.a0; cpu.gpr[5] = a.a1;
    cov_lo = f->entry; cov_hi = f->entry + f->insns * 4;
    active_cpu = &cpu;
    int rc = PcPortMipsRun(&cpu, f->entry, HALT, 5000);
    active_cpu = NULL;
    if (rc != PC_PORT_MIPS_HALTED) fprintf(stderr, "oracle rc=%d %s\n", rc, cpu.error);
    require(rc == PC_PORT_MIPS_HALTED, "retail run halted");
    require(cpu.gpr[29] == STACK, "retail stack restored");
    for (int r = 16; r < 24; r++) require(cpu.gpr[r] == 0xC0DE0000u + (uint32_t)r, "retail callee-saved reg");
    require(cpu.gpr[28] == 0xC0DE001Cu && cpu.gpr[30] == 0xC0DE001Eu, "retail gp/fp preserved");
    Trace rt; memcpy(&rt, &trace, sizeof(rt));
    static uint8_t after[0x200000];
    memcpy(after, ram, sizeof(after));

    /* native on the identical start state */
    memcpy(ram, start, sizeof(start));
    memcpy(feed, feed0, sizeof(feed));
    memset(&trace, 0, sizeof(trace));
    mirror_in();
    sprite_swap();
    f->native(a);
    sprite_swap();
    Trace nt; memcpy(&nt, &trace, sizeof(nt));

    require(nt.n == rt.n, "spy call count");
    require(memcmp(nt.ev, rt.ev, sizeof(Event) * nt.n) == 0, "spy call trace (order/arguments)");
    require(g_pGameState == GSTATE && g_FieldScriptVMCurActor == CURACT &&
            g_FieldScriptVMCurScriptData == SCRIPT && g_FieldActors == FACTORS, "pointer globals unchanged");
    if (memcmp(natbuf, after + (BUF_BASE & 0x1FFFFF), BUF_SIZE) != 0) {
        unsigned nd = 0;
        for (uint32_t i = 0; i < BUF_SIZE && nd < 6; i++)
            if (natbuf[i] != after[(BUF_BASE & 0x1FFFFF) + i]) {
                fprintf(stderr, "first diff %08x native=%02x retail=%02x start=%02x\n", BUF_BASE + i,
                        natbuf[i], after[(BUF_BASE & 0x1FFFFF) + i], start[(BUF_BASE & 0x1FFFFF) + i]);
                nd++;
            }
        fail("guest buffer window");
    }
    for (unsigned r = 0; r < NREG; r++) {
        if (memcmp(regions[r].nat, after + (regions[r].base & 0x1FFFFF), regions[r].size) != 0) {
            for (uint32_t i = 0; i < regions[r].size; i++)
                if (regions[r].nat[i] != after[(regions[r].base & 0x1FFFFF) + i]) {
                    fprintf(stderr, "first diff %08x native=%02x retail=%02x\n", regions[r].base + i,
                            regions[r].nat[i], after[(regions[r].base & 0x1FFFFF) + i]);
                    break;
                }
            fail("global region bytes");
        }
    }
    (void)idx;
    cases++;
}

int main(int argc, char** argv) {
    require(argc >= 2, "field image argument");
    FILE* fp = fopen(argv[1], "rb"); require(fp != NULL, "open field image");
    require(fread(field_image, 1, FIELD_SIZE, fp) == FIELD_SIZE && fgetc(fp) == EOF, "field image size");
    fclose(fp);
    natbuf = mmap((void*)(uintptr_t)BUF_BASE, BUF_SIZE, PROT_READ | PROT_WRITE,
                  MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
    require(natbuf == (void*)(uintptr_t)BUF_BASE, "identity map of guest buffers");
    const char* only = argc >= 3 ? argv[2] : NULL;
    unsigned total_fn = 0;
    for (unsigned i = 0; i < sizeof(fns) / sizeof(fns[0]); i++) {
        Fn* f = &fns[i];
        if (only && strcmp(only, f->name) != 0) continue;
        memset(cov, 0, sizeof(cov));
        unsigned before = cases;
        for (unsigned c = 0; c < f->ncases; c++) run_case(f, c);
        unsigned seen = 0, missing = 0;
        for (unsigned k = 0; k < f->insns; k++) { if (cov[k]) seen++; else missing++; }
        printf("FIELD MISC OPS %-24s PASS cases=%u instructions=%u/%u\n", f->name, cases - before, seen, f->insns);
        if (missing) {
            printf("  uncovered:");
            for (unsigned k = 0; k < f->insns; k++) if (!cov[k]) printf(" %08x", f->entry + 4 * k);
            printf("\n");
        }
        total_fn++;
    }
    printf("FIELD MISC OPS PASS functions=%u cases=%u checks=%u\n", total_fn, cases, checks);
    return 0;
}
