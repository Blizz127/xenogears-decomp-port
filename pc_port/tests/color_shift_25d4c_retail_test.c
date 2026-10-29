/* func_80025D4C (retail 80025D4C..80025FA8, the handwritten GTE loop that
 * temp1f.c keeps as INCLUDE_ASM) against its port body in
 * pc_port/src/retail_leaf_adapters.c.  The retail bytes from the user's
 * disc/SLUS_006.64 run in the test-only interpreter (battle_mips_adapter.c);
 * the native body runs on the same fixture.  After every case, the three
 * arrays and the whole PsyCross GTE register file must agree.
 *
 * Cases cover:
 *  - the five o32 stack arguments (red, green, blue, kind, factor at
 *    sp+0x10..0x20) and counts 0..16;
 *  - kinds 0..3, with random t2..t4 at entry (the kinds that set them never
 *    read them first);
 *  - kinds outside 0..3, with t2..t4 = 0 at entry (the port body's start);
 *  - offsets with bit 15 set and past 16 bits: the s16 clamps, and the
 *    carries that the final OR lets spill into the next field;
 *  - the colour-blend test's factor list, and zero / 0x8000 source pixels;
 *  - separate, pairwise-aliased and fully aliased arrays.  The game's one
 *    call (800A6CE8) passes the same array as source, target and destination.
 * Both sides drive the same PsyCross GTE backend, so this proves the port body
 * reproduces the retail instruction stream's effects, not the hardware's. */
#include <stdio.h>
#include <string.h>
#define uid_t psyq_test_uid_t
#define gid_t psyq_test_gid_t
#include "common.h"
#undef uid_t
#undef gid_t
extern void func_80025D4C(s32 count, u16* source, u16* target, u16* destination, s32 red,
                          s32 green, s32 blue, s32 kind, s32 factor) __attribute__((weak));
#include "psyq/libgte.h"
extern void MTC2(unsigned int, int);
extern unsigned int MFC2(int);
extern unsigned int CFC2(int);
extern void CTC2(unsigned int, int);
extern int doCOP2(int);
#include "battle_mips_adapter.h"
#include "psx/gtereg.h"

#define RETAIL 0x80025d4cu
#define RETAIL_FILE_OFFSET 0x1654c
#define RETAIL_SIZE 0x25c
#define STACK 0x801ff000u
#define HALT 0xfffffffcu

static u8 ram[0x200000];
static struct { u16 a[24], b[24], c[24]; } fixture, initial, expected;

static u8* address(u32 a, unsigned w)
{
    uintptr_t lo = (uintptr_t)&fixture;
    if (a >= 0x80000000u && (uint64_t)a + w <= 0x80200000u) return ram + (a & 0x1fffff);
    if (a >= lo && (uint64_t)a + w <= lo + sizeof(fixture)) return (u8*)(uintptr_t)a;
    return NULL;
}
static int rd(void* u, u32 a, unsigned w, u32* v)
{
    u8* p = address(a, w);
    (void)u;
    if (!p) return -1;
    *v = 0;
    for (unsigned i = 0; i < w; ++i) *v |= (u32)p[i] << (i * 8);
    return 0;
}
static int wr(void* u, u32 a, unsigned w, u32 v)
{
    u8* p = address(a, w);
    (void)u;
    if (!p) return -1;
    for (unsigned i = 0; i < w; ++i) p[i] = (u8)(v >> (i * 8));
    return 0;
}
static u32 copRead(void* u, int control, unsigned reg) { (void)u; return control ? CFC2(reg) : MFC2(reg); }
static void copWrite(void* u, int control, unsigned reg, u32 value)
{
    (void)u;
    if (control) CTC2(value, reg); else MTC2(value, reg);
}
static int copCommand(void* u, u32 instruction) { (void)u; doCOP2(instruction & 0x1ffffffu); return 0; }

static u32 seed = 0x2545f491u;
static u32 next(void) { seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5; return seed; }

static const s32 kFactors[] = {(-2147483647 - 1), -32768, -1, 0, 1, 16, 31, 32, 33, 0x7fffffff};
static const s32 kOffsets[] = {-0x10003, -0x8000, -40, -31, -1, 0, 1, 5, 31, 32, 40,
                               0x7fff, 0x8000, 0x10005};
static const s32 kKinds[] = {0, 1, 2, 3, -1, 4, 0x7fffffff, (-2147483647 - 1)};
#define N(x) (sizeof(x) / sizeof((x)[0]))

typedef struct { u32 count; s32 red, green, blue, kind, factor; unsigned alias; s32 sweep; } Case;

static void pointers(unsigned alias, u16** source, u16** target, u16** destination)
{
    *source = fixture.a; *target = fixture.b; *destination = fixture.c;
    switch (alias) {
    case 1: *destination = fixture.a; break;                       /* dest == source */
    case 2: *destination = fixture.b; break;                       /* dest == target */
    case 3: *target = fixture.a; *destination = fixture.a; break;  /* all three: 800A6CE8 */
    case 4: *target = fixture.a; break;                            /* source == target */
    default: break;
    }
}

static void gte_seed(u32 rgbc, u32 ir, u32 flag)
{
    memset(&gteRegs, 0, sizeof(gteRegs));
    MTC2(rgbc, 6);
    MTC2(ir, 9);
    MTC2(ir >> 8, 10);
    MTC2(ir >> 16, 11);
    CTC2(flag, 31);
}

static unsigned failures;

static int run_case(const Case* k, unsigned number)
{
    u16 *source, *target, *destination;
    u32 rgbc = next(), ir = next(), flag = next() & 0x7ffff000u, t2 = next(), t3 = next(), t4 = next();
    GTERegisters expectedGte;
    PcPortMipsBus bus = {.read = rd, .write = wr, .cop2_read = copRead, .cop2_write = copWrite,
                         .cop2_command = copCommand};
    PcPortMipsCpu cpu;
    int rc;

    if (k->kind < 0 || k->kind > 3) t2 = t3 = t4 = 0;
    for (unsigned i = 0; i < 24; ++i) {
        fixture.a[i] = (u16)next(); fixture.b[i] = (u16)next(); fixture.c[i] = (u16)next();
    }
    if (k->sweep >= 0) {
        for (unsigned i = 0; i < 16; ++i) fixture.a[i] = (u16)(k->sweep + i);
    } else {
        fixture.a[number % 7] = 0;          /* no store, GPF still runs */
        fixture.a[(number % 5) + 8] = 0x8000;
    }
    initial = fixture;
    pointers(k->alias, &source, &target, &destination);

    gte_seed(rgbc, ir, flag);
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = k->count;
    cpu.gpr[5] = (u32)(uintptr_t)source;
    cpu.gpr[6] = (u32)(uintptr_t)target;
    cpu.gpr[7] = (u32)(uintptr_t)destination;
    cpu.gpr[10] = t2; cpu.gpr[11] = t3; cpu.gpr[12] = t4;
    cpu.gpr[29] = STACK;
    cpu.gpr[31] = HALT;
    wr(NULL, STACK + 0x10, 4, (u32)k->red);
    wr(NULL, STACK + 0x14, 4, (u32)k->green);
    wr(NULL, STACK + 0x18, 4, (u32)k->blue);
    wr(NULL, STACK + 0x1c, 4, (u32)k->kind);
    wr(NULL, STACK + 0x20, 4, (u32)k->factor);
    rc = PcPortMipsRun(&cpu, RETAIL, HALT, 100000);
    if (rc != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "COLOR SHIFT FAIL oracle %s\n", cpu.error);
        return -1;
    }
    expected = fixture;
    expectedGte = gteRegs;

    fixture = initial;
    gte_seed(rgbc, ir, flag);
    func_80025D4C((s32)k->count, source, target, destination, k->red, k->green, k->blue, k->kind,
                  k->factor);
    if (memcmp(&fixture, &expected, sizeof(fixture)) != 0 ||
        memcmp(&gteRegs, &expectedGte, sizeof(gteRegs)) != 0) {
        unsigned at = 0;
        const u16* got = (const u16*)&fixture;
        const u16* want = (const u16*)&expected;
        while (at < sizeof(fixture) / 2 && got[at] == want[at]) ++at;
        if (failures++ < 8) {
            fprintf(stderr, "COLOR SHIFT FAIL case=%u count=%u kind=%d factor=%d red=%d green=%d "
                    "blue=%d alias=%u ", number, k->count, k->kind, k->factor, k->red, k->green,
                    k->blue, k->alias);
            if (at < sizeof(fixture) / 2)
                fprintf(stderr, "first array word %u: native=%04x retail=%04x\n", at, got[at], want[at]);
            else
                fprintf(stderr, "arrays equal, GTE register file differs\n");
        }
        return 1;
    }
    return 0;
}

int main(void)
{
    FILE* f = fopen("disc/SLUS_006.64", "rb");
    unsigned cases = 0;
    Case k;

    if (!func_80025D4C) { fputs("COLOR SHIFT FAIL missing native owner\n", stderr); return 1; }
    if (!f || fseek(f, RETAIL_FILE_OFFSET, SEEK_SET) != 0 ||
        fread(ram + (RETAIL & 0x1fffff), 1, RETAIL_SIZE, f) != RETAIL_SIZE) {
        fputs("COLOR SHIFT FAIL cannot read retail bytes from disc/SLUS_006.64\n", stderr);
        return 1;
    }
    fclose(f);

    /* Every kind x factor x aliasing, with rotated offset triples, counts 0/1/16. */
    for (unsigned ki = 0; ki < N(kKinds); ++ki)
        for (unsigned fi = 0; fi < N(kFactors); ++fi)
            for (unsigned alias = 0; alias < 5; ++alias)
                for (unsigned o = 0; o < N(kOffsets); ++o)
                    for (unsigned c = 0; c < 3; ++c) {
                        k.count = c == 0 ? 0 : c == 1 ? 1 : 16;
                        k.red = kOffsets[o];
                        k.green = kOffsets[(o * 5 + 3) % N(kOffsets)];
                        k.blue = kOffsets[(o * 9 + 1) % N(kOffsets)];
                        k.kind = kKinds[ki];
                        k.factor = kFactors[fi];
                        k.alias = alias;
                        k.sweep = -1;
                        if (run_case(&k, cases++) < 0) return 1;
                    }
    /* Every source pixel value (16 per call) for each kind 0..3. */
    for (unsigned ki = 0; ki < 4; ++ki)
        for (u32 base = 0; base < 0x10000; base += 16) {
            k.count = 16;
            k.red = (s32)(base >> 12) - 8;
            k.green = (s32)((base >> 8) & 15) - 4;
            k.blue = (s32)(next() % 9) - 4;
            k.kind = (s32)ki;
            k.factor = (s32)((base >> 4) % 34);
            k.alias = 0;
            k.sweep = (s32)base;
            if (run_case(&k, cases++) < 0) return 1;
        }
    /* Random draws over the same lists and over whole 32-bit words. */
    for (unsigned n = 0; n < 60000; ++n) {
        u32 r = next();
        k.count = next() % 17;
        k.red = (r & 1) ? (s32)next() : kOffsets[next() % N(kOffsets)];
        k.green = (r & 2) ? (s32)(next() % 64) - 32 : kOffsets[next() % N(kOffsets)];
        k.blue = (r & 4) ? (s32)(next() % 64) - 32 : kOffsets[next() % N(kOffsets)];
        k.kind = (r & 8) ? (s32)(next() % 4) : kKinds[next() % N(kKinds)];
        k.factor = (r & 16) ? (s32)(next() % 40) - 4 : kFactors[next() % N(kFactors)];
        k.alias = next() % 5;
        k.sweep = -1;
        if (run_case(&k, cases++) < 0) return 1;
    }
    if (failures) {
        fprintf(stderr, "COLOR SHIFT FAIL %u of %u cases\n", failures, cases);
        return 1;
    }
    puts("COLOR SHIFT shared PsyCross backend, not independent hardware validation");
    printf("COLOR SHIFT PASS %u cases: retail body vs port body, arrays and GTE state\n", cases);
    return 0;
}
