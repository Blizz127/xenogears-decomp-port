#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "battle_mips_adapter.h"

extern void func_8001FBE4(void *, uint32_t, void *);
/* Link guards only. B4 uses the real linked stack helper; another path cannot silently
 * turn this test into a comparison against replacement implementations. */
#define GUARD(name) void name(void) { fputs("SPRITE B4 FAIL unexpected callee " #name "\n", stderr); abort(); }
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
void MTC2(unsigned int value, int reg) { (void)value; (void)reg; fputs("SPRITE B4 FAIL unexpected callee MTC2\n", stderr); abort(); }
unsigned int MFC2(int reg) { (void)reg; fputs("SPRITE B4 FAIL unexpected callee MFC2\n", stderr); abort(); }
int doCOP2(int command) { (void)command; fputs("SPRITE B4 FAIL unexpected callee doCOP2\n", stderr); abort(); }
int32_t func_80023124(int32_t a, int32_t b)
{ (void)a; (void)b; fputs("SPRITE B4 FAIL unexpected angle helper\n", stderr); abort(); }
uint32_t D_80018644, D_800592E4;
int32_t D_80059198;
uint8_t D_800591AD, D_800591B0, D_800591B3;
int16_t D_800592E8, D_800592EA;
uint32_t D_8004FBB8[8], D_8006F99C[4], D_8006F9AC[4], D_800C3EB0[4];
int32_t g_WorkListCurTimer;
uint8_t D_8006BE10[32];
static uint8_t ram[0x200000];
static struct {
    uint32_t before[8], sprite[128], between[8], ops[2], after[8];
} fixture, initial, expected;
static unsigned cases, handler_entries, helper_entries;

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
    if (a == 0x80021480u) ++handler_entries;
    if (a == 0x80021ca0u) ++helper_entries;
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

/* The full signed cursor range addresses sprite bytes 0x0e..0x10d after
 * decrement. This controlled allocation includes that range and guards.
 * Alias inputs are physically coupled: a byte aliased with the cursor must
 * equal that cursor, so incompatible pairs are skipped explicitly. */
static void compare(unsigned cursor, unsigned byte, unsigned alias, unsigned variant)
{
    for (unsigned i = 0; i < sizeof(fixture); ++i)
        ((uint8_t *)&fixture)[i] = (uint8_t)(i * 37u + variant * 73u + byte);
    uint8_t *sprite = (uint8_t *)fixture.sprite;
    unsigned destination = (unsigned)(0x8e + (int8_t)(uint8_t)(cursor - 1));
    uint8_t *operands[] = {(uint8_t *)fixture.ops, sprite + 0x8c,
        sprite + destination, sprite + destination - 1, sprite + destination + 1};
    uint8_t *ops = operands[alias];
    if (ops == sprite + 0x8c && byte != cursor) return;
    sprite[0x8c] = (uint8_t)cursor;
    ops[0] = (uint8_t)byte;
    initial = fixture;
    handler_entries = helper_entries = 0;
    PcPortMipsBus bus = {.read = rd, .write = wr};
    PcPortMipsCpu cpu;
    PcPortMipsCpuInit(&cpu, &bus);
    const uint32_t opcodes[] = {0xb4u, 0x100b4u, 0xffffffb4u, 0xdeadbeb4u};
    uint32_t opcode = opcodes[variant];
    cpu.gpr[4] = (uint32_t)(uintptr_t)sprite;
    cpu.gpr[5] = opcode;
    cpu.gpr[6] = (uint32_t)(uintptr_t)ops;
    cpu.gpr[29] = 0x801fff00u;
    cpu.gpr[31] = 0xfffffffcu;
    int result = PcPortMipsRun(&cpu, 0x8001fbe4u, 0xfffffffcu, 1000);
    if (result != PC_PORT_MIPS_HALTED || handler_entries != 1 || helper_entries != 1) {
        fprintf(stderr, "SPRITE B4 FAIL oracle case=%u pc=%08x handler=%u helper=%u: %s\n",
                cases, cpu.pc, handler_entries, helper_entries, cpu.error);
        exit(2);
    }
    expected = fixture;
    fixture = initial;
    if (cases == 0) {
        printf("SPRITE B4 first retail case: cursor=%02x operand=%02x expected_cursor=%02x "
               "destination=%03x expected_byte=%02x\n", cursor, byte,
               ((uint8_t *)expected.sprite)[0x8c], destination,
               ((uint8_t *)expected.sprite)[destination]);
        fflush(stdout);
    }
    func_8001FBE4(sprite, opcode, ops);
    if (memcmp(&fixture, &expected, sizeof(fixture))) {
        unsigned first = 0;
        while (first < sizeof(fixture) && ((uint8_t *)&fixture)[first] == ((uint8_t *)&expected)[first]) ++first;
        fprintf(stderr, "SPRITE B4 FAIL case=%u cursor=%02x byte=%02x alias=%u opcode=%08x "
                "diff=%u cursor_result=%02x/%02x destination=%03x data=%02x/%02x\n",
                cases, cursor, byte, alias, opcode, first, sprite[0x8c],
                ((uint8_t *)expected.sprite)[0x8c], destination, sprite[destination],
                ((uint8_t *)expected.sprite)[destination]);
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
    /* Observed normal-opening input from retained stop RAM: sprite offset
     * 0x1cde24, operand offset 0x18cfa5, cursor 0x10 and operand 0x04. */
    compare(0x10, 0x04, 0, 0);
    for (unsigned cursor = 0; cursor < 256; ++cursor)
        for (unsigned byte = 0; byte < 256; ++byte)
            for (unsigned variant = 0; variant < 4; ++variant)
                compare(cursor, byte, 0, variant);
    unsigned alias_start = cases;
    for (unsigned alias = 1; alias < 5; ++alias)
        for (unsigned cursor = 0; cursor < 256; ++cursor)
            for (unsigned byte = 0; byte < 256; ++byte)
                for (unsigned variant = 0; variant < 4; ++variant)
                    compare(cursor, byte, alias, variant);
    assert(cases == 1046541 && cases - alias_start == 784396);
    printf("SPRITE B4 PASS %u cases: full native/retail dispatcher and real stack helpers, "
           "all operand bytes/cursors, signed cursor wrap, raw opcode upper bits, "
           "%u cursor/destination/adjacent aliases and complete fixture/guard memory\n",
           cases, cases - alias_start);
    puts("SPRITE B4 scope: actual retail instruction oracle; controlled low pointers and "
         "complete signed stack-address range; no unrelated helpers, register-state or visible-runtime claim.");
    return 0;
}

/* C5 is outside this test: keep its newly linked dependency fail-closed. */
void func_80022CDC(void *sprite) { (void)sprite; __builtin_trap(); }
