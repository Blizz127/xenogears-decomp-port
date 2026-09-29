#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "battle_mips_adapter.h"

extern void func_8001FBE4(void *, uint32_t, void *);
/* Link guards only. E5 uses the real pointer helper and RNG; other paths cannot silently
 * turn this test into a comparison against replacement implementations. */
#define GUARD(name) void name(void) { fputs("SPRITE E5 FAIL unexpected callee " #name "\n", stderr); abort(); }
GUARD(func_800B2AEC)
GUARD(func_80039E60)
/* Guest-RAM backing for the opcode 0xB0 selector read; never touched here. */
uint8_t g_PsxRam[0x300000];
GUARD(AnimScriptTick) GUARD(ApplyMatrixSV) GUARD(HeapAlloc)
GUARD(HeapChangeCurrentUser) GUARD(HeapFree) GUARD(MulMatrix0)
GUARD(__wrap_ReadGeomOffset) GUARD(RotMatrix) GUARD(ScaleMatrix) GUARD(__wrap_ScaleMatrixL)
GUARD(func_8001CE74) GUARD(func_8001D2B0) GUARD(func_8001D4E8) GUARD(func_8001EE68)
GUARD(func_8001F6B0) GUARD(func_8001FB30) GUARD(func_80022D44)
GUARD(func_80023B84) GUARD(func_800245D8) GUARD(func_8002C3E8)
GUARD(func_8002C59C) GUARD(func_8002C8CC) GUARD(func_8002CB54)
GUARD(func_8002CC10) GUARD(rcos) GUARD(rsin)
GUARD(func_80022CAC) GUARD(func_80023290) GUARD(TransMatrix) GUARD(SetTransMatrix)
GUARD(SetRotMatrix) GUARD(RotTransSV)
void MTC2(unsigned int value, int reg)
{ (void)value; (void)reg; fputs("SPRITE E5 FAIL unexpected callee MTC2\n", stderr); abort(); }
unsigned int MFC2(int reg)
{ (void)reg; fputs("SPRITE E5 FAIL unexpected callee MFC2\n", stderr); abort(); }
int doCOP2(int command)
{ (void)command; fputs("SPRITE E5 FAIL unexpected callee doCOP2\n", stderr); abort(); }
int32_t func_80023124(int32_t a, int32_t b)
{ (void)a; (void)b; fputs("SPRITE E5 FAIL unexpected angle helper\n", stderr); abort(); }
uint32_t D_80018644, D_800592E4;
int32_t D_80059198;
uint8_t D_800591AD, D_800591B0, D_800591B3;
int16_t D_800592E8, D_800592EA;
uint32_t D_8004FBB8[8], D_8006F99C[4], D_8006F9AC[4], D_800C3EB0[4];
int32_t g_WorkListCurTimer;
uint8_t D_8006BE10[32];
extern uint32_t g_RandomSeed;
extern int __real_rand(void);
static unsigned native_rand_calls;
int __wrap_rand(void) { ++native_rand_calls; return __real_rand(); }
static uint8_t ram[0x200000];
static struct {
    uint32_t before[8], sprite[128], between[8], target[64], ops[2], after[8];
} fixture, initial, expected;
static unsigned cases, handler_entries, pointer_entries, rand_entries;
static unsigned destination_operand0, destination_operand1, seed_operands, seed_destinations;
static uint32_t destination;
static PcPortMipsCpu *active_cpu;

static uint8_t *address(uint32_t a, unsigned width)
{
    uintptr_t seed = (uintptr_t)&g_RandomSeed;
    if (a >= seed && (uint64_t)a + width <= seed + 4)
        return (uint8_t *)(uintptr_t)a;
    if (a >= 0x8005a1fcu && (uint64_t)a + width <= 0x8005a200u)
        return (uint8_t *)&g_RandomSeed + (a - 0x8005a1fcu);
    uintptr_t first = (uintptr_t)&fixture;
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
    if (a == 0x80020c20u) ++handler_entries;
    if (a == 0x8001fba4u) ++pointer_entries;
    if (a == 0x8003fa38u) {
        ++rand_entries;
        if (active_cpu) destination = active_cpu->gpr[16];
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

/* The actual RNG seed is mapped at both its retail address and the same low
 * native pointer used by operand aliases. Its update can change either operand
 * byte between destination resolution and the retail post-rand range read.
 * All helpers execute their production/retail bodies, without RNG doubles. */
static void compare(unsigned index, unsigned range, unsigned cursor, uint32_t seed,
                    unsigned layout, unsigned alias, unsigned variant)
{
    for (unsigned i = 0; i < sizeof(fixture); ++i)
        ((uint8_t *)&fixture)[i] = (uint8_t)(i * 37u + variant * 73u + index + range);
    uint8_t *sprite = (uint8_t *)fixture.sprite;
    uint8_t *bases[] = {(uint8_t *)fixture.target, sprite + 0x100, (uint8_t *)&g_RandomSeed};
    uint8_t *base = bases[layout];
    uint32_t packed = (uint32_t)(uintptr_t)base;
    memcpy(sprite + 0x88, &packed, 4);
    sprite[0x8c] = (uint8_t)cursor;
    g_RandomSeed = seed;
    if (layout == 2 && index >= 0x84) return;
    uint8_t *planned = index & 0x80 ? base + (index & 0x7f)
                                 : sprite + 0x8e + (int8_t)(uint8_t)cursor + index;
    uint8_t *operands[] = {(uint8_t *)fixture.ops, sprite + 0x8b, sprite + 0x8c,
        planned, (uint8_t *)&g_RandomSeed, (uint8_t *)&g_RandomSeed + 1,
        (uint8_t *)&g_RandomSeed + 2};
    uint8_t *ops = operands[alias];
    if (alias < 4) {
        if (layout == 2 && alias == 3) return;
        ops[0] = (uint8_t)index;
        ops[1] = (uint8_t)range;
    }
    /* Avoid a deliberately malformed packed base when operand placement
     * overwrites its high byte. Local addressing does not read that base. */
    if (alias == 1 && (ops[0] & 0x80)) return;
    if (layout == 2 && ops[0] >= 0x84) return;
    initial = fixture;
    uint32_t initial_seed = g_RandomSeed;
    handler_entries = pointer_entries = rand_entries = 0;
    PcPortMipsBus bus = {.read = rd, .write = wr};
    PcPortMipsCpu cpu;
    PcPortMipsCpuInit(&cpu, &bus);
    active_cpu = &cpu;
    const uint32_t opcodes[] = {0xe5u, 0x100e5u, 0xffffffe5u, 0xdeadbee5u};
    uint32_t opcode = opcodes[variant & 3];
    cpu.gpr[4] = (uint32_t)(uintptr_t)sprite;
    cpu.gpr[5] = opcode;
    cpu.gpr[6] = (uint32_t)(uintptr_t)ops;
    cpu.gpr[29] = 0x801fff00u;
    cpu.gpr[31] = 0xfffffffcu;
    int result = PcPortMipsRun(&cpu, 0x8001fbe4u, 0xfffffffcu, 1000);
    if (result != PC_PORT_MIPS_HALTED || handler_entries != 1 || pointer_entries != 1 || rand_entries != 1) {
        fprintf(stderr, "SPRITE E5 FAIL oracle case=%u pc=%08x entries=%u/%u/%u: %s\n",
                cases, cpu.pc, handler_entries, pointer_entries, rand_entries, cpu.error);
        exit(2);
    }
    active_cpu = NULL;
    expected = fixture;
    uint32_t expected_seed = g_RandomSeed;
    uint8_t expected_byte = *address(destination, 1);
    fixture = initial;
    g_RandomSeed = initial_seed;
    if (cases == 0) {
        printf("SPRITE E5 first retail case: index=%02x range=%02x cursor=%02x seed=%08x "
               "expected_byte=%02x expected_seed=%08x\n", index, range, cursor, seed,
               expected_byte, expected_seed);
        fflush(stdout);
    }
    native_rand_calls = 0;
    func_8001FBE4(sprite, opcode, ops);
    if (memcmp(&fixture, &expected, sizeof(fixture)) || g_RandomSeed != expected_seed || native_rand_calls != 1) {
        unsigned first = 0;
        while (first < sizeof(fixture) && ((uint8_t *)&fixture)[first] == ((uint8_t *)&expected)[first]) ++first;
        fprintf(stderr, "SPRITE E5 FAIL case=%u index=%02x range=%02x cursor=%02x seed=%08x "
                "layout=%u alias=%u opcode=%08x diff=%u data=%02x/%02x next_seed=%08x/%08x rand_calls=%u\n",
                cases, index, range, cursor, seed, layout, alias, opcode, first,
                *address(destination, 1), expected_byte, g_RandomSeed, expected_seed, native_rand_calls);
        exit(1);
    }
    destination_operand0 += destination == (uintptr_t)ops;
    destination_operand1 += destination == (uintptr_t)(ops + 1);
    seed_operands += (uintptr_t)ops >= (uintptr_t)&g_RandomSeed &&
                     (uintptr_t)ops + 2 <= (uintptr_t)&g_RandomSeed + 4;
    seed_destinations += destination >= (uintptr_t)&g_RandomSeed &&
                        destination < (uintptr_t)&g_RandomSeed + 4;
    ++cases;
}

/* Select inputs using actual retail RNG executions, not a copied LCG formula.
 * This supplies every possible low random byte for the scaling cross-product. */
static void seeds_for_every_random_byte(uint32_t seeds[256])
{
    uint8_t seen[256] = {0};
    unsigned found = 0;
    for (uint32_t candidate = 0; candidate < 65536 && found < 256; ++candidate) {
        g_RandomSeed = candidate;
        PcPortMipsBus bus = {.read = rd, .write = wr};
        PcPortMipsCpu cpu;
        PcPortMipsCpuInit(&cpu, &bus);
        cpu.gpr[31] = 0xfffffffcu;
        assert(PcPortMipsRun(&cpu, 0x8003fa38u, 0xfffffffcu, 100) == PC_PORT_MIPS_HALTED);
        assert(cpu.gpr[2] <= 0x7fff);
        unsigned byte = cpu.gpr[2] & 255;
        if (!seen[byte]) { seen[byte] = 1; seeds[byte] = candidate; ++found; }
    }
    assert(found == 256);
}

int main(void)
{
    assert((uintptr_t)&fixture + sizeof(fixture) <= UINT32_MAX);
    assert((uintptr_t)&g_RandomSeed + 4 <= UINT32_MAX);
    FILE *image = fopen("disc/SLUS_006.64", "rb");
    assert(image && !fseek(image, 0x800, SEEK_SET));
    assert(fread(ram + 0x10000, 1, sizeof(ram) - 0x10000, image) > 0x30000);
    assert(!fclose(image));
    /* Observed opening operands/cursor, with a controlled initial RNG seed.
     * The stopped guest RAM seed alone does not establish the host seed. */
    compare(0, 0x12, 0x0f, 0, 0, 0, 0);
    const unsigned ranges[] = {0, 1, 2, 3, 127, 128, 254, 255};
    const unsigned cursors[] = {0, 1, 15, 127, 128, 129, 254, 255};
    const uint32_t edges[] = {0, 1, 0xffffffffu, 0x7fffffffu, 0x80000000u,
                              0x12345678u, 0x80000001u, 0xff0000ffu};
    for (unsigned index = 0; index < 256; ++index)
        for (unsigned cursor = 0; cursor < 256; ++cursor)
            for (unsigned r = 0; r < 8; ++r)
                for (unsigned s = 0; s < 8; ++s)
                    compare(index, ranges[r], cursor, edges[s],
                            (index + cursor + r + s) & 1, 0, (index ^ cursor ^ r ^ s) & 3);
    uint32_t seeds[256];
    seeds_for_every_random_byte(seeds);
    for (unsigned random_byte = 0; random_byte < 256; ++random_byte)
        for (unsigned range = 0; range < 256; ++range)
            for (unsigned external = 0; external < 2; ++external)
                compare(external ? 0x80 : 0, range, 15, seeds[random_byte],
                        0, 0, (random_byte ^ range) & 3);
    unsigned alias_start = cases;
    for (unsigned alias = 1; alias < 4; ++alias)
        for (unsigned index = 0; index < 256; ++index)
            for (unsigned r = 0; r < 8; ++r)
                for (unsigned c = 0; c < 8; ++c)
                    for (unsigned s = 0; s < 8; ++s)
                        for (unsigned layout = 0; layout < 2; ++layout)
                            compare(index, ranges[r], cursors[c], edges[s], layout,
                                    alias, (index ^ r ^ c ^ s) & 3);
    for (unsigned alias = 4; alias < 7; ++alias)
        for (unsigned s = 0; s < 264; ++s)
            for (unsigned c = 0; c < 8; ++c)
                for (unsigned layout = 0; layout < 2; ++layout)
                    for (unsigned variant = 0; variant < 4; ++variant)
                        compare(0, 0, cursors[c], s < 256 ? seeds[s] : edges[s - 256],
                                layout, alias, variant);
    for (unsigned index = 0x80; index < 0x84; ++index)
        for (unsigned range = 0; range < 256; ++range)
            for (unsigned s = 0; s < 256; ++s)
                compare(index, range, 15, seeds[s], 2, 0, (index ^ range ^ s) & 3);
    assert(cases == 5293569 && cases - alias_start == 968192);
    assert(destination_operand0 && destination_operand1 && seed_operands && seed_destinations);
    printf("SPRITE E5 PASS %u cases: native/retail dispatcher, pointer helper and RNG, "
           "all index/cursor pairs at 8 ranges and 8 seeds, all 256x256 random-byte/range pairs "
           "in both addressing modes, %u alias cases, complete fixture and RNG state\n",
           cases, cases - alias_start);
    printf("SPRITE E5 alias coverage: destination=operand0:%u operand1:%u seed-operands:%u seed-destination:%u\n",
           destination_operand0, destination_operand1, seed_operands, seed_destinations);
    puts("SPRITE E5 scope: controlled valid packed pointers, real helper/RNG and exact call count; "
         "32-bit seed boundaries, not all 2^32 seeds; no register-state or visible-runtime claim.");
    return 0;
}

/* C5 is outside this test: keep its newly linked dependency fail-closed. */
void func_80022CDC(void *sprite) { (void)sprite; __builtin_trap(); }
