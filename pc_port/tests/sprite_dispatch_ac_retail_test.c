#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "battle_mips_adapter.h"

extern void func_8001FBE4(void *, uint32_t, void *);
#define GUARD(name) void name(void) { fputs("SPRITE AC FAIL unexpected callee " #name "\n", stderr); abort(); }
GUARD(func_800B2AEC)
GUARD(func_80039E60)
/* Guest-RAM backing for the opcode 0xB0 selector read; never touched here. */
uint8_t g_PsxRam[0x300000];
GUARD(AnimScriptTick) GUARD(HeapAlloc)
GUARD(HeapChangeCurrentUser) GUARD(HeapFree)
GUARD(__wrap_ReadGeomOffset) GUARD(__wrap_ScaleMatrixL)
GUARD(func_8001CE74) GUARD(func_8001D2B0) GUARD(func_8001D4E8)
GUARD(func_8001EE68) GUARD(func_8001F6B0) GUARD(func_8001FB30)
GUARD(func_80022D44) GUARD(func_80023B84)
GUARD(func_800245D8) GUARD(func_8002C3E8) GUARD(func_8002C59C)
GUARD(func_8002C8CC) GUARD(func_8002CB54) GUARD(func_8002CC10)
GUARD(func_80023124) GUARD(func_80023290)
void func_80022CAC(void *p, int32_t v) { (void)p; (void)v; fputs("SPRITE AC FAIL unexpected callee func_80022CAC\n", stderr); abort(); }
int g_cfg_pgxpTextureCorrection;

uint32_t D_80018644, D_800592E4;
int32_t D_80059198;
uint8_t D_800591AD, D_800591B0, D_800591B3;
int16_t D_800592E8, D_800592EA;
uint32_t D_8004FBB8[8], D_8006F99C[4], D_8006F9AC[4], D_800C3EB0[4];
/* Opcode 0xA7 timer (s32) and opcode 0xBD packed block; both unreached here. */
int32_t g_WorkListCurTimer;
uint8_t D_8006BE10[32];
extern uint32_t g_RandomSeed;
extern int __real_rand(void);
static unsigned native_rand_calls;
int __wrap_rand(void) { ++native_rand_calls; return __real_rand(); }

static uint8_t ram[0x200000];
static struct { uint8_t sprite[0xC0], ops[8], tail[0x20]; } fixture, initial, expected;
static unsigned cases;
static unsigned handler_entries, rand_entries, angle_entries, velocity_entries;

static uint8_t *address(uint32_t a, unsigned width)
{
    uintptr_t seed = (uintptr_t)&g_RandomSeed, first = (uintptr_t)&fixture;
    if (a >= seed && (uint64_t)a + width <= seed + 4) return (uint8_t *)(uintptr_t)a;
    if (a >= 0x8005a1fcu && (uint64_t)a + width <= 0x8005a200u)
        return (uint8_t *)&g_RandomSeed + (a - 0x8005a1fcu);
    if (a >= first && (uint64_t)a + width <= first + sizeof(fixture))
        return (uint8_t *)(uintptr_t)a;
    if (a >= 0x80000000u && (uint64_t)a + width <= 0x80200000u)
        return ram + (a & 0x1fffffu);
    return NULL;
}
static int rd(void *unused, uint32_t a, unsigned width, uint32_t *value)
{
    (void)unused; uint8_t *p = address(a, width); if (!p) return -1;
    handler_entries += a == 0x80021644u; rand_entries += a == 0x8003fa38u;
    angle_entries += a == 0x80021fe0u; velocity_entries += a == 0x80022974u;
    *value = 0; for (unsigned i = 0; i < width; ++i) *value |= (uint32_t)p[i] << (i * 8);
    return 0;
}
static int wr(void *unused, uint32_t a, unsigned width, uint32_t value)
{
    (void)unused; uint8_t *p = address(a, width); if (!p) return -1;
    for (unsigned i = 0; i < width; ++i) p[i] = (uint8_t)(value >> (i * 8));
    return 0;
}
static void put32(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value; p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16); p[3] = (uint8_t)(value >> 24);
}

static void compare(uint8_t range, uint16_t angle, uint32_t radius,
                    uint32_t flags, uint32_t seed, unsigned alias, unsigned variant)
{
    for (unsigned i = 0; i < sizeof(fixture); ++i)
        ((uint8_t *)&fixture)[i] = (uint8_t)(i * 37u + variant * 73u + range);
    put32(fixture.sprite + 0x18, radius); put32(fixture.sprite + 0xAC, flags);
    fixture.sprite[0x32] = (uint8_t)angle; fixture.sprite[0x33] = (uint8_t)(angle >> 8);
    fixture.ops[0] = range; g_RandomSeed = seed;
    uint8_t *aliases[] = {fixture.ops, fixture.sprite + 0x00, fixture.sprite + 0x18,
                          fixture.sprite + 0x32, fixture.sprite + 0xAC,
                          (uint8_t *)&g_RandomSeed, (uint8_t *)&g_RandomSeed + 1,
                          (uint8_t *)&g_RandomSeed + 2, (uint8_t *)&g_RandomSeed + 3};
    uint8_t *ops = aliases[alias];
    if (alias < 5) ops[0] = range;
    else ((uint8_t *)&g_RandomSeed)[alias - 5] = range;
    initial = fixture;
    uint32_t initial_seed = g_RandomSeed;
    PcPortMipsBus bus = {.read = rd, .write = wr}; PcPortMipsCpu cpu;
    PcPortMipsCpuInit(&cpu, &bus);
    const uint32_t opcodes[] = {0xacu, 0x100acu, 0xffffffacu, 0xdeadbeacu};
    uint32_t opcode = opcodes[variant & 3];
    cpu.gpr[4] = (uint32_t)(uintptr_t)fixture.sprite; cpu.gpr[5] = opcode;
    cpu.gpr[6] = (uint32_t)(uintptr_t)ops; cpu.gpr[29] = 0x801fff00u;
    cpu.gpr[31] = 0xfffffffcu; handler_entries = rand_entries = angle_entries = velocity_entries = 0;
    int result = PcPortMipsRun(&cpu, 0x8001fbe4u, 0xfffffffcu, 1000);
    if (result != PC_PORT_MIPS_HALTED || handler_entries != 1 || rand_entries != 1 ||
        angle_entries != 1 || velocity_entries != 1) {
        fprintf(stderr, "SPRITE AC FAIL oracle case=%u pc=%08x entries=%u/%u/%u/%u: %s\n",
                cases, cpu.pc, handler_entries, rand_entries, angle_entries, velocity_entries, cpu.error);
        exit(2);
    }
    expected = fixture; uint32_t expected_seed = g_RandomSeed;
    fixture = initial; g_RandomSeed = initial_seed; native_rand_calls = 0;
    if (cases == 0) {
        printf("SPRITE AC first retail case: range=%02x angle=%04x radius=%08x flags=%08x seed=%08x expected_seed=%08x\n",
               range, angle, radius, flags, seed, expected_seed); fflush(stdout);
    }
    func_8001FBE4(fixture.sprite, opcode, ops);
    if (memcmp(&fixture, &expected, sizeof(fixture)) || g_RandomSeed != expected_seed || native_rand_calls != 1) {
        unsigned diff = 0; while (diff < sizeof(fixture) && ((uint8_t *)&fixture)[diff] == ((uint8_t *)&expected)[diff]) ++diff;
        fprintf(stderr, "SPRITE AC FAIL case=%u range=%02x angle=%04x radius=%08x flags=%08x seed=%08x diff=%u seed=%08x/%08x rand_calls=%u\n",
                cases, range, angle, radius, flags, seed, diff, g_RandomSeed, expected_seed, native_rand_calls);
        exit(1);
    }
    ++cases;
}

/* Select every possible low random byte by executing the retail RNG itself.
 * This keeps the AC cross-product independent of a copied host LCG formula. */
static void seeds_for_random_byte(uint32_t seeds[256])
{
    uint8_t seen[256] = {0};
    unsigned found = 0;
    PcPortMipsBus bus = {.read = rd, .write = wr};
    for (uint32_t candidate = 0; candidate < 65536 && found < 256; ++candidate) {
        g_RandomSeed = candidate;
        PcPortMipsCpu cpu;
        PcPortMipsCpuInit(&cpu, &bus);
        cpu.gpr[31] = 0xfffffffcu;
        assert(PcPortMipsRun(&cpu, 0x8003fa38u, 0xfffffffcu, 100) == PC_PORT_MIPS_HALTED);
        unsigned byte = cpu.gpr[2] & 0xffu;
        if (!seen[byte]) { seen[byte] = 1; seeds[byte] = candidate; ++found; }
    }
    assert(found == 256);
}

int main(void)
{
    assert((uintptr_t)&fixture + sizeof(fixture) <= UINT32_MAX);
    assert((uintptr_t)&g_RandomSeed + 4 <= UINT32_MAX);
    FILE *image = fopen("disc/SLUS_006.64", "rb"); assert(image && !fseek(image, 0x800, SEEK_SET));
    assert(fread(ram + 0x10000, 1, sizeof(ram) - 0x10000, image) > 0x30000); assert(!fclose(image));
    compare(0x12, 0x3456, 0x00123456u, 0x00008000u, 0, 0, 0);
    puts("SPRITE AC baseline fixture passed through retail oracle");

    static const uint32_t radius_edges[] = {
        0, 1, 0xffffffffu, 0x7fffffffu, 0x80000000u, 0x80000001u,
        0x12345678u, 0xff0000ffu
    };
    static const uint16_t angle_edges[] = {
        0, 1, 2, 0xfffu, 0x1000u, 0x1001u, 0x7fffu, 0x8000u,
        0x8001u, 0xffffu
    };
    static const unsigned divisor_edges[] = {0, 1, 2, 3, 0x7ff, 0xffe, 0xfff};
    uint32_t seeds[256];
    seeds_for_random_byte(seeds);

    if (getenv("SPRITE_AC_QUICK") != NULL) {
        for (unsigned random_byte = 0; random_byte < 256; ++random_byte)
            for (unsigned edge = 0; edge < sizeof(angle_edges) / sizeof(angle_edges[0]); ++edge)
                compare((uint8_t)random_byte, angle_edges[edge], radius_edges[random_byte & 7],
                        divisor_edges[random_byte % (sizeof(divisor_edges) / sizeof(divisor_edges[0]))] << 7,
                        seeds[random_byte], 0, (random_byte ^ edge) & 3);
        for (unsigned alias = 1; alias < 9; ++alias)
            for (unsigned range = 0; range < 256; range += 17)
                compare((uint8_t)range, angle_edges[range % (sizeof(angle_edges) / sizeof(angle_edges[0]))],
                        radius_edges[range & 7], (range & 0xfffu) << 7, seeds[range], alias,
                        (alias ^ range) & 3);
        puts("SPRITE AC QUICK PASS");
        return 0;
    }

    /* Every random byte and unsigned range, with signed-angle/radius and
     * divisor boundary variants. */
    for (unsigned random_byte = 0; random_byte < 256; ++random_byte)
        for (unsigned range = 0; range < 256; ++range)
            for (unsigned edge = 0; edge < sizeof(angle_edges) / sizeof(angle_edges[0]); ++edge)
                for (unsigned divisor = 0; divisor < sizeof(divisor_edges) / sizeof(divisor_edges[0]); ++divisor)
                    compare((uint8_t)range, angle_edges[edge], radius_edges[(random_byte + range + edge) & 7],
                            divisor_edges[divisor] << 7, seeds[random_byte], 0,
                            (random_byte ^ range ^ edge ^ divisor) & 3);

    /* All 4096 masked angle values and all non-zero divisor values. */
    for (unsigned angle = 0; angle < 4096; ++angle)
        compare(0, (uint16_t)angle, radius_edges[angle & 7],
                divisor_edges[angle % (sizeof(divisor_edges) / sizeof(divisor_edges[0]))] << 7,
                seeds[angle & 0xff], 0, angle & 3);
    for (unsigned divisor = 0; divisor < 4096; ++divisor)
        compare((uint8_t)divisor, angle_edges[divisor % (sizeof(angle_edges) / sizeof(angle_edges[0]))],
                radius_edges[divisor & 7], divisor << 7, seeds[divisor & 0xff], 0, divisor & 3);

    /* Operand bytes may alias sprite state or any live RNG-seed byte.  The
     * range read is deliberately after rand, so seed aliases exercise the
     * updated seed rather than a copied pre-call value. */
    for (unsigned alias = 1; alias < 9; ++alias)
        for (unsigned range = 0; range < 256; ++range)
            for (unsigned angle = 0; angle < sizeof(angle_edges) / sizeof(angle_edges[0]); ++angle)
                compare((uint8_t)range, angle_edges[angle], radius_edges[(range + angle) & 7],
                        (range & 0xfffu) << 7, seeds[range], alias,
                        (alias ^ range ^ angle) & 3);

    printf("SPRITE AC PASS %u cases: retail AC RNG/range cross-product, masked angles, "
           "radius/divisor boundaries, operand and seed aliases; whole fixture and RNG state\n", cases);
    return 0;
}

/* C5 is outside this test: keep its newly linked dependency fail-closed. */
void func_80022CDC(void *sprite) { (void)sprite; __builtin_trap(); }
