#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "battle_mips_adapter.h"

extern void func_8001FBE4(void *, uint32_t, void *);

/* C0 must reach only its retail leaves.  These guards catch accidental
 * success through another dispatch path when the whole switch is linked. */
#define GUARD(name) void name(void) { fputs("SPRITE C0 FAIL unexpected callee " #name "\n", stderr); abort(); }
GUARD(func_800B2AEC)
GUARD(func_80039E60)
GUARD(func_8001D4E8)
int32_t g_WorkListCurTimer; uint8_t D_8006BE10[32];
/* Guest-RAM backing for the opcode 0xB0 selector read; never touched here. */
uint8_t g_PsxRam[0x300000];
GUARD(AnimScriptTick) GUARD(__wrap_ApplyMatrixSV) GUARD(HeapAlloc)
GUARD(HeapChangeCurrentUser) GUARD(HeapFree) GUARD(__wrap_MulMatrix0)
GUARD(__wrap_ReadGeomOffset) GUARD(__wrap_ScaleMatrix) GUARD(__wrap_ScaleMatrixL)
GUARD(func_8001CE74) GUARD(func_8001D2B0) GUARD(func_8001EE68)
GUARD(func_8001F6B0) GUARD(func_8001FB30) GUARD(func_80022D44)
GUARD(func_80023B84) GUARD(func_800245D8) GUARD(func_8002C3E8)
GUARD(func_8002C59C) GUARD(func_8002C8CC) GUARD(func_8002CB54)
GUARD(func_8002CC10) GUARD(func_80023290)
GUARD(func_80023124)

uint32_t D_80018644, D_800592E4;
int32_t D_80059198;
uint8_t D_800591AD, D_800591B0, D_800591B3;
int16_t D_800592E8, D_800592EA;
uint32_t D_8004FBB8[8], D_8006F99C[4], D_8006F9AC[4], D_800C3EB0[4];
int g_cfg_pgxpTextureCorrection;

extern uint32_t g_RandomSeed;
extern int __real_rand(void);
extern int __real_rcos(int);
extern int __real_rsin(int);
extern int32_t __real_func_80022CAC(void *, int32_t);

static unsigned native_rand_calls, native_rcos_calls, native_rsin_calls;
static unsigned native_helper_calls;
static uint8_t native_events[8], retail_events[8];
static unsigned native_event_count, retail_event_count;
static uint32_t native_helper_args[2][2], retail_helper_args[2][2];

/* Event order uses 1=rand, 2=trig, 3=slow helper.  The source-level shim
 * intentionally swaps the conventional PsyCross names to retail semantics,
 * so the two trig exports are counted separately while order remains generic. */
int __wrap_rand(void)
{
    if (native_event_count < sizeof(native_events)) native_events[native_event_count++] = 1;
    ++native_rand_calls;
    return __real_rand();
}

int __wrap_rcos(int angle)
{
    if (native_event_count < sizeof(native_events)) native_events[native_event_count++] = 2;
    ++native_rcos_calls;
    return __real_rcos(angle);
}

int __wrap_rsin(int angle)
{
    if (native_event_count < sizeof(native_events)) native_events[native_event_count++] = 2;
    ++native_rsin_calls;
    return __real_rsin(angle);
}

int32_t __wrap_func_80022CAC(void *sprite, int32_t value)
{
    if (native_event_count < sizeof(native_events)) native_events[native_event_count++] = 3;
    if (native_helper_calls < 2) {
        native_helper_args[native_helper_calls][0] = (uint32_t)(uintptr_t)sprite;
        native_helper_args[native_helper_calls][1] = (uint32_t)value;
    }
    ++native_helper_calls;
    return __real_func_80022CAC(sprite, value);
}

static uint8_t ram[0x200000];
static struct {
    uint8_t before[0x40];
    uint8_t sprite[0xc0];
    uint8_t ops[8];
    uint8_t after[0x40];
} fixture, initial, expected;
static unsigned cases;
static unsigned handler_entries, rand_entries, rcos_entries, rsin_entries, helper_entries;
static PcPortMipsCpu *active_cpu;

static uint8_t *address(uint32_t a, unsigned width)
{
    uintptr_t seed = (uintptr_t)&g_RandomSeed;
    uintptr_t first = (uintptr_t)&fixture;
    if (a >= seed && (uint64_t)a + width <= seed + 4)
        return (uint8_t *)(uintptr_t)a;
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
    (void)unused;
    uint8_t *p = address(a, width);
    if (!p) return -1;
    if (a == 0x80020158u) ++handler_entries;
    if (a == 0x8003fa38u) {
        ++rand_entries;
        if (retail_event_count < sizeof(retail_events)) retail_events[retail_event_count++] = 1;
    }
    if (a == 0x8003f8ccu) {
        ++rcos_entries;
        if (retail_event_count < sizeof(retail_events)) retail_events[retail_event_count++] = 2;
    }
    if (a == 0x8003f8b0u) {
        ++rsin_entries;
        if (retail_event_count < sizeof(retail_events)) retail_events[retail_event_count++] = 2;
    }
    if (a == 0x80022cacu) {
        ++helper_entries;
        if (retail_event_count < sizeof(retail_events)) retail_events[retail_event_count++] = 3;
        if (helper_entries <= 2 && active_cpu != NULL) {
            retail_helper_args[helper_entries - 1][0] = active_cpu->gpr[4];
            retail_helper_args[helper_entries - 1][1] = active_cpu->gpr[5];
        }
    }
    *value = 0;
    for (unsigned i = 0; i < width; ++i) *value |= (uint32_t)p[i] << (i * 8);
    return 0;
}

static int wr(void *unused, uint32_t a, unsigned width, uint32_t value)
{
    (void)unused;
    uint8_t *p = address(a, width);
    if (!p) return -1;
    for (unsigned i = 0; i < width; ++i) p[i] = (uint8_t)(value >> (i * 8));
    return 0;
}

static void put16(uint8_t *p, uint16_t value) { memcpy(p, &value, 2); }
static void put32(uint8_t *p, uint32_t value) { memcpy(p, &value, 4); }
static uint32_t word(const uint8_t *p) { uint32_t value; memcpy(&value, p, 4); return value; }

static void compare(uint8_t operand, int16_t scale, uint16_t timer,
                    uint32_t seed, unsigned position, unsigned alias,
                    unsigned variant)
{
    static const uint32_t positions[] = {
        0, 1, 0xffffffffu, 0x7fffffffu, 0x80000000u,
        0x00010000u, 0xffff0000u, 0x12345678u
    };
    for (unsigned i = 0; i < sizeof(fixture); ++i)
        ((uint8_t *)&fixture)[i] = (uint8_t)(i * 37u + variant * 73u + operand);
    uint8_t *p = fixture.sprite;
    put32(p + 0, positions[position & 7]);
    put32(p + 8, positions[(position + 3) & 7]);
    put16(p + 0x2c, (uint16_t)scale);
    put16(p + 0x3a, timer);
    g_RandomSeed = seed;
    uint8_t *aliases[] = {
        fixture.ops, p + 0, p + 1, p + 8, p + 9,
        p + 0x2c, p + 0x2d, p + 0x3a, p + 0x3b,
        (uint8_t *)&g_RandomSeed, (uint8_t *)&g_RandomSeed + 1,
        (uint8_t *)&g_RandomSeed + 2, (uint8_t *)&g_RandomSeed + 3
    };
    uint8_t *ops = aliases[alias % (sizeof(aliases) / sizeof(aliases[0]))];
    ops[0] = operand;
    initial = fixture;
    uint32_t initial_seed = g_RandomSeed;

    PcPortMipsBus bus = {.read = rd, .write = wr};
    PcPortMipsCpu cpu;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = (uint32_t)(uintptr_t)p;
    uint32_t opcode = 0x000000c0u | (variant << 8);
    cpu.gpr[5] = opcode;
    cpu.gpr[6] = (uint32_t)(uintptr_t)ops;
    cpu.gpr[29] = 0x801fff00u;
    cpu.gpr[31] = 0xfffffffcu;
    handler_entries = rand_entries = rcos_entries = rsin_entries = helper_entries = 0;
    retail_event_count = 0;
    memset(retail_events, 0, sizeof(retail_events));
    memset(retail_helper_args, 0, sizeof(retail_helper_args));
    active_cpu = &cpu;
    int result = PcPortMipsRun(&cpu, 0x8001fbe4u, 0xfffffffcu, 1200);
    active_cpu = NULL;
    if (result != PC_PORT_MIPS_HALTED || handler_entries != 1 || rand_entries != 2 ||
        rcos_entries != 1 || rsin_entries != 1 || helper_entries != 2 ||
        retail_event_count != 6) {
        fprintf(stderr, "SPRITE C0 FAIL oracle case=%u pc=%08x entries=%u/%u/%u/%u/%u events=%u: %s\n",
                cases, cpu.pc, handler_entries, rand_entries, rcos_entries,
                rsin_entries, helper_entries, retail_event_count, cpu.error);
        exit(2);
    }
    static const uint8_t expected_events[] = {1, 1, 2, 3, 2, 3};
    if (memcmp(retail_events, expected_events, sizeof(expected_events)) != 0) {
        fputs("SPRITE C0 FAIL oracle call order", stderr); exit(2);
    }
    expected = fixture;
    uint32_t expected_seed = g_RandomSeed;

    fixture = initial;
    g_RandomSeed = initial_seed;
    native_rand_calls = native_rcos_calls = native_rsin_calls = native_helper_calls = 0;
    native_event_count = 0;
    memset(native_events, 0, sizeof(native_events));
    memset(native_helper_args, 0, sizeof(native_helper_args));
    func_8001FBE4(p, opcode, ops);
    if (memcmp(&fixture, &expected, sizeof(fixture)) || g_RandomSeed != expected_seed ||
        native_rand_calls != 2 || native_rcos_calls != 1 || native_rsin_calls != 1 ||
        native_helper_calls != 2 || native_event_count != 6 ||
        memcmp(native_events, expected_events, sizeof(expected_events)) != 0 ||
        memcmp(native_helper_args, retail_helper_args, sizeof(native_helper_args)) != 0) {
        unsigned diff = 0;
        while (diff < sizeof(fixture) && ((uint8_t *)&fixture)[diff] == ((uint8_t *)&expected)[diff]) ++diff;
        fprintf(stderr, "SPRITE C0 FAIL case=%u operand=%02x scale=%d timer=%04x seed=%08x "
                "position=%u alias=%u variant=%u diff=%u xyz=%08x,%08x/%08x,%08x "
                "next_seed=%08x/%08x calls=%u,%u,%u,%u helpers=%08x,%08x/%08x,%08x\n",
                cases, operand, scale, timer, seed, position, alias, variant, diff,
                word(p), word(p + 8), word(expected.sprite), word(expected.sprite + 8),
                g_RandomSeed, expected_seed, native_rand_calls, native_rcos_calls,
                native_rsin_calls, native_helper_calls, native_helper_args[0][1],
                native_helper_args[1][1], retail_helper_args[0][1], retail_helper_args[1][1]);
        exit(1);
    }
    ++cases;
}

static void select_random_seeds(uint32_t seeds[256])
{
    uint8_t seen[256] = {0};
    unsigned found = 0;
    PcPortMipsBus bus = {.read = rd, .write = wr};
    for (uint32_t candidate = 0; candidate < 65536u && found < 256; ++candidate) {
        g_RandomSeed = candidate;
        PcPortMipsCpu cpu;
        PcPortMipsCpuInit(&cpu, &bus);
        cpu.gpr[31] = 0xfffffffcu;
        assert(PcPortMipsRun(&cpu, 0x8003fa38u, 0xfffffffcu, 100) == PC_PORT_MIPS_HALTED);
        unsigned value = cpu.gpr[2] & 0xffu;
        if (!seen[value]) { seen[value] = 1; seeds[value] = candidate; ++found; }
    }
    assert(found == 256);
}

int main(void)
{
    assert((uintptr_t)&fixture + sizeof(fixture) <= UINT32_MAX);
    assert((uintptr_t)&g_RandomSeed + 4 <= UINT32_MAX);
    FILE *image = fopen("disc/SLUS_006.64", "rb");
    assert(image && !fseek(image, 0x800, SEEK_SET));
    assert(fread(ram + 0x10000, 1, sizeof(ram) - 0x10000, image) > 0x48000);
    assert(!fclose(image));

    const uint8_t operands[] = {0, 1, 2, 0x7f, 0x80, 0xfe, 0xff};
    const int16_t scales[] = {0, 1, -1, 0x7ff, -0x800, 0x1000, -0x1000, 0x7fff, (int16_t)0x8000};
    const uint16_t timers[] = {0, 1, 2, 0x3ff, 0x400, 0x401, 0x7fff, 0xffff};
    const uint32_t seeds[] = {0, 1, 0xffffffffu, 0x80000000u, 0x12345678u, 0xabcdef01u, 0x7fffffffu, 0x00ff00ffu};

    compare(0x40, 0x1000, 0, 1, 0, 0, 0);
    if (getenv("SPRITE_C0_QUICK")) {
        compare(0xff, (int16_t)0x8000, 0xffff, 0x87654321u, 4, 0, 1);
        compare(0x80, -0x1800, 0x400, 0x13579bdfu, 7, 5, 2);
        printf("SPRITE C0 QUICK PASS %u cases\n", cases);
        return 0;
    }
    for (unsigned o = 0; o < sizeof(operands); ++o)
        for (unsigned s = 0; s < sizeof(scales) / sizeof(scales[0]); ++s)
            for (unsigned t = 0; t < sizeof(timers) / sizeof(timers[0]); ++t)
                for (unsigned r = 0; r < sizeof(seeds) / sizeof(seeds[0]); ++r)
                    compare(operands[o], scales[s], timers[t], seeds[r], (o + s + t + r) & 7, 0,
                            (o ^ s ^ t ^ r) & 3);
    for (unsigned o = 0; o < 256; ++o)
        compare((uint8_t)o, 0x1800, 0x400, 0x13579bdfu, o & 7, 0, o & 3);
    for (unsigned s = 0; s < 65536; ++s)
        compare(0xa5, (int16_t)s, 0x401, 0x2468ace0u + s, s & 7, 0, s & 3);
    for (unsigned t = 0; t < 65536; ++t)
        compare(0x7f, (int16_t)0xe800, (uint16_t)t, 0xdead0000u + t, t & 7, 0, t & 3);

    uint32_t random_seeds[256];
    select_random_seeds(random_seeds);
    for (unsigned r = 0; r < 256; ++r)
        compare((uint8_t)(r ^ 0x5a), (int16_t)((r & 1) ? -0x1800 : 0x1800),
                (uint16_t)(r * 257u), random_seeds[r], (r >> 5) & 7, 0, r & 3);
    for (unsigned alias = 1; alias < 13; ++alias)
        for (unsigned value = 0; value < 256; value += 17)
            compare((uint8_t)value, (int16_t)((alias & 1) ? 0x1000 : -0x1000),
                    (uint16_t)(alias * 0x111u), 0xabcdef01u + alias,
                    (value >> 4) & 7, alias, alias & 3);

    printf("SPRITE C0 PASS %u cases: retail/native dispatcher, two RNG calls, rcos/rsin and helper argument order, "
           "full operand/scale/timer axes, position wrap edges, whole fixture, seed and operand aliases\n", cases);
    puts("SPRITE C0 scope: actual SLUS_006.64 C0/RNG/trig/0x80022CAC instructions and native extracted helper; "
         "finite independent census, no exhaustive 32-bit Cartesian claim or rendering claim.");
    return 0;
}

/* C5 is outside this test: keep its newly linked dependency fail-closed. */
void func_80022CDC(void *sprite) { (void)sprite; __builtin_trap(); }
