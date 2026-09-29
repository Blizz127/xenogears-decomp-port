#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "battle_mips_adapter.h"

extern void func_8001FBE4(void *, uint32_t, void *);
/* Link guards only. A1 has no callees; another dispatch path cannot silently
 * turn this test into a comparison against replacement implementations. */
#define GUARD(name) void name(void) { fputs("SPRITE A1 FAIL unexpected callee " #name "\n", stderr); abort(); }
GUARD(func_800B2AEC)
GUARD(func_80039E60)
/* Guest-RAM backing for the opcode 0xB0 selector read; never touched here. */
uint8_t g_PsxRam[0x300000];
GUARD(AnimScriptTick) GUARD(ApplyMatrixSV) GUARD(HeapAlloc)
GUARD(HeapChangeCurrentUser) GUARD(HeapFree) GUARD(MulMatrix0)
GUARD(ReadGeomOffset) GUARD(RotMatrix) GUARD(ScaleMatrix) GUARD(ScaleMatrixL)
GUARD(func_8001CE74) GUARD(func_8001D2B0) GUARD(func_8001D4E8) GUARD(func_8001EE68)
GUARD(func_8001F6B0) GUARD(func_8001FB30) GUARD(func_80022D44)
GUARD(func_80023B84) GUARD(func_800245D8) GUARD(func_8002C3E8)
GUARD(func_8002C59C) GUARD(func_8002C8CC) GUARD(func_8002CB54)
GUARD(func_8002CC10) GUARD(rcos) GUARD(rsin)
GUARD(func_80022CAC) GUARD(func_80023290) GUARD(TransMatrix) GUARD(SetTransMatrix)
GUARD(SetRotMatrix) GUARD(RotTransSV)
void MTC2(unsigned int value, int reg) { (void)value; (void)reg; fputs("SPRITE A1 FAIL unexpected callee MTC2\n", stderr); abort(); }
unsigned int MFC2(int reg) { (void)reg; fputs("SPRITE A1 FAIL unexpected callee MFC2\n", stderr); abort(); }
int doCOP2(int command) { (void)command; fputs("SPRITE A1 FAIL unexpected callee doCOP2\n", stderr); abort(); }
int32_t func_80023124(int32_t a, int32_t b)
{ (void)a; (void)b; fputs("SPRITE A1 FAIL unexpected angle helper\n", stderr); abort(); }
uint32_t D_80018644, D_800592E4;
int32_t D_80059198;
uint8_t D_800591AD, D_800591B0, D_800591B3;
int16_t D_800592E8, D_800592EA;
uint32_t D_8004FBB8[8], D_8006F99C[4], D_8006F9AC[4], D_800C3EB0[4];
int32_t g_WorkListCurTimer;
uint8_t D_8006BE10[32];
static uint8_t ram[0x200000];
static struct {
    uint32_t before[8], sprite[64], between[8], state[8], ops[2], after[8];
} fixture, initial, expected;
static unsigned cases;

static uint32_t word(const uint8_t *p)
{
    uint32_t value;
    memcpy(&value, p, 4);
    return value;
}

static void put_word(uint8_t *p, uint32_t value) { memcpy(p, &value, 4); }
static void put_half(uint8_t *p, uint16_t value) { memcpy(p, &value, 2); }

static uint8_t *address(uint32_t a, unsigned width)
{
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

/* Layouts alias the override word with old vy, divisor flags, gate flags, or
 * the signed scale word. Operand locations include those inputs and vy bytes.
 * Writing an operand may physically couple an input byte with those fields;
 * the resulting single memory image is passed unchanged to both executions. */
static void compare(unsigned byte, uint32_t factor, int16_t scale, unsigned divisor,
                    unsigned gate, uint32_t override, uint32_t old_vy,
                    unsigned layout, unsigned alias, unsigned variant)
{
    for (unsigned i = 0; i < sizeof(fixture); ++i)
        ((uint8_t *)&fixture)[i] = (uint8_t)(i * 37u + variant * 73u + byte);
    uint8_t *sprite = (uint8_t *)fixture.sprite;
    uint8_t *states[] = {(uint8_t *)fixture.state, sprite + 0x10,
                         sprite + 0xac, sprite + 0xa8, sprite + 0x80};
    uint8_t *state = states[layout];
    put_word(sprite + 0x10, old_vy);
    put_word(sprite + 0x7c, (uint32_t)(uintptr_t)state);
    put_half(sprite + 0x82, (uint16_t)scale);
    put_word(sprite + 0xa8, ((variant & 1) ? 0xfffffffeu : 0x2001f800u) | gate);
    put_word(sprite + 0xac, (divisor << 7) | ((variant & 1) ? 0xfff8007fu : 0x20u));
    fixture.state[0] = override;
    uint8_t *operands[] = {(uint8_t *)fixture.ops, sprite + 0x10, sprite + 0x11,
        sprite + 0x12, sprite + 0x13, sprite + 0x82, sprite + 0x83,
        sprite + 0xa8, sprite + 0xac, sprite + 0xad,
        state, state + 1, state + 2, state + 3};
    uint8_t *ops = operands[alias];
    ops[0] = (uint8_t)byte;
    initial = fixture;
    D_80059198 = (int32_t)factor;
    assert(!wr(NULL, 0x80059198u, 4, factor));
    PcPortMipsBus bus = {.read = rd, .write = wr};
    PcPortMipsCpu cpu;
    PcPortMipsCpuInit(&cpu, &bus);
    uint32_t opcode = variant & 2 ? 0xdeadbea1u : 0xa1u;
    cpu.gpr[4] = (uint32_t)(uintptr_t)sprite;
    cpu.gpr[5] = opcode;
    cpu.gpr[6] = (uint32_t)(uintptr_t)ops;
    cpu.gpr[29] = 0x801fff00;
    cpu.gpr[31] = 0xfffffffcu;
    int result = PcPortMipsRun(&cpu, 0x8001fbe4u, 0xfffffffcu, 1000);
    if (result != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "SPRITE A1 FAIL oracle case=%u pc=%08x: %s\n", cases, cpu.pc, cpu.error);
        exit(2);
    }
    expected = fixture;
    fixture = initial;
    if (cases == 0) {
        printf("SPRITE A1 first retail case: operand=00 factor=0 scale=8192 divisor=256 "
               "old_vy=-126336 expected_vy=%d\n", (int32_t)word((uint8_t *)expected.sprite + 0x10));
        fflush(stdout);
    }
    func_8001FBE4(sprite, opcode, ops);
    if (memcmp(&fixture, &expected, sizeof(fixture)) || (uint32_t)D_80059198 != factor) {
        unsigned first = 0;
        while (first < sizeof(fixture) && ((uint8_t *)&fixture)[first] == ((uint8_t *)&expected)[first]) ++first;
        fprintf(stderr, "SPRITE A1 FAIL case=%u byte=%02x factor=%08x scale=%d divisor=%u "
                "gate=%u override=%08x layout=%u alias=%u variant=%u diff=%u vy=%08x/%08x\n",
                cases, byte, factor, scale, divisor, gate, override, layout, alias, variant,
                first, word(sprite + 0x10), word((uint8_t *)expected.sprite + 0x10));
        exit(1);
    }
    ++cases;
}

int main(void)
{
    assert((uintptr_t)&fixture + sizeof(fixture) <= UINT32_MAX);
    FILE *image = fopen("disc/SLUS_006.64", "rb");
    assert(image && !fseek(image, 0x800, SEEK_SET));
    assert(fread(ram + 0x10000, 1, sizeof(ram) - 0x10000, image) > 0x12000);
    assert(!fclose(image));
    compare(0, 0, 8192, 256, 0, 0, (uint32_t)-126336, 0, 0, 0);
    const uint32_t factors[] = {0, 1, 0xffffffffu, 0x7fffffffu, 0x80000000u, 0x40000000u, 0x40000001u, 0x12345678u};
    const int16_t scales[] = {0, 1, -1, 4095, -4095, 8192, 32767, -32768};
    const unsigned divisors[] = {0, 1, 2, 3, 7, 2047, 2048, 4095};
    for (unsigned byte = 0; byte < 256; ++byte)
        for (unsigned f = 0; f < 8; ++f)
            for (unsigned s = 0; s < 8; ++s)
                for (unsigned d = 0; d < 8; ++d)
                    for (unsigned gate = 0; gate < 2; ++gate)
                        compare(byte, factors[f], scales[s], divisors[d], gate, 0,
                                0xaabbccddu, 0, 0, (byte + f + s + d) & 3);
    const uint32_t states[] = {0, 1, 0x7fffffu, 0x800000u, 0x80000000u, 0xffffffffu, 0x12345678u, 0xfffe1280u};
    for (unsigned divisor = 0; divisor < 4096; ++divisor)
        for (unsigned state = 0; state < 8; ++state)
            compare((divisor + state * 31u) & 255, factors[state], scales[state], divisor,
                    1, states[state], 0x12345678u, 0, 0, state & 3);
    unsigned alias_start = cases;
    for (unsigned layout = 0; layout < 5; ++layout)
        for (unsigned alias = 0; alias < 14; ++alias)
            for (unsigned byte = 0; byte < 256; ++byte)
                for (unsigned variant = 0; variant < 4; ++variant)
                    compare(byte, factors[variant * 2], scales[variant * 2 + 1],
                            divisors[variant * 2], variant & 1, states[variant * 2],
                            states[variant * 2 + 1], layout, alias, variant);
    assert(cases == 366593 && cases - alias_start == 71680);
    printf("SPRITE A1 PASS %u cases: full native/retail dispatcher, signed bytes, wrapping factors/products, "
           "signed scale and rounding, override/fallback, all 4096 divisors including zero, "
           "%u aligned alias cases and complete fixture/guard memory\n", cases, cases - alias_start);
    puts("SPRITE A1 scope: actual retail instruction oracle; selected valid packed-pointer aliases, "
         "no dependency calls, no register-state or visible-runtime claim.");
    return 0;
}

/* C5 is outside this test: keep its newly linked dependency fail-closed. */
void func_80022CDC(void *sprite) { (void)sprite; __builtin_trap(); }
