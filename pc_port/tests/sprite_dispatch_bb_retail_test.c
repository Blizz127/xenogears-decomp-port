#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "battle_mips_adapter.h"

extern void func_8001FBE4(void *, uint32_t, void *);
/* Link guards only. BB has no callees; another dispatch path cannot silently
 * turn this test into a comparison against replacement implementations. */
#define GUARD(name) void name(void) { fputs("SPRITE BB FAIL unexpected callee " #name "\n", stderr); abort(); }
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
void MTC2(unsigned int value, int reg) { (void)value; (void)reg; fputs("SPRITE BB FAIL unexpected callee MTC2\n", stderr); abort(); }
unsigned int MFC2(int reg) { (void)reg; fputs("SPRITE BB FAIL unexpected callee MFC2\n", stderr); abort(); }
int doCOP2(int command) { (void)command; fputs("SPRITE BB FAIL unexpected callee doCOP2\n", stderr); abort(); }
int32_t func_80023124(int32_t a, int32_t b)
{ (void)a; (void)b; fputs("SPRITE BB FAIL unexpected angle helper\n", stderr); abort(); }
uint32_t D_80018644, D_800592E4;
int32_t D_80059198;
uint8_t D_800591AD, D_800591B0, D_800591B3;
int16_t D_800592E8, D_800592EA;
uint32_t D_8004FBB8[8], D_8006F99C[4], D_8006F9AC[4], D_800C3EB0[4];
int32_t g_WorkListCurTimer;
uint8_t D_8006BE10[32];
static uint8_t ram[0x200000];
static struct {
    uint32_t before[8], sprite[64], between[8], ops[2], after[8];
} fixture, initial, expected;
static unsigned cases, handler_entries;

static uint16_t half(const uint8_t *p)
{
    return (uint16_t)((unsigned)p[0] | (unsigned)p[1] << 8);
}

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
    if (a == 0x80020e14u) ++handler_entries;
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

/* For destination-byte aliases, the operand and that byte of the old halfword
 * are the same physical input. Adjacent aliases preserve the full independent
 * operand domain; the complete fixture, including guards, is compared. */
static void compare(unsigned old, unsigned byte, unsigned alias, unsigned variant)
{
    for (unsigned i = 0; i < sizeof(fixture); ++i)
        ((uint8_t *)&fixture)[i] = (uint8_t)(i * 37u + variant * 73u + byte);
    uint8_t *sprite = (uint8_t *)fixture.sprite;
    uint8_t *operands[] = {(uint8_t *)fixture.ops, sprite + 0x30, sprite + 0x31,
                          sprite + 0x2f, sprite + 0x32};
    uint8_t *ops = operands[alias];
    sprite[0x30] = (uint8_t)old;
    sprite[0x31] = (uint8_t)(old >> 8);
    if (alias == 1 || alias == 2) assert(*ops == byte);
    ops[0] = (uint8_t)byte;
    initial = fixture;
    handler_entries = 0;
    PcPortMipsBus bus = {.read = rd, .write = wr};
    PcPortMipsCpu cpu;
    PcPortMipsCpuInit(&cpu, &bus);
    const uint32_t opcodes[] = {0xbbu, 0x100bbu, 0xffffffbbu, 0xdeadbebbu};
    uint32_t opcode = opcodes[variant];
    cpu.gpr[4] = (uint32_t)(uintptr_t)sprite;
    cpu.gpr[5] = opcode;
    cpu.gpr[6] = (uint32_t)(uintptr_t)ops;
    cpu.gpr[29] = 0x801fff00u;
    cpu.gpr[31] = 0xfffffffcu;
    int result = PcPortMipsRun(&cpu, 0x8001fbe4u, 0xfffffffcu, 1000);
    if (result != PC_PORT_MIPS_HALTED || handler_entries != 1) {
        fprintf(stderr, "SPRITE BB FAIL oracle case=%u pc=%08x handler=%u: %s\n",
                cases, cpu.pc, handler_entries, cpu.error);
        exit(2);
    }
    expected = fixture;
    fixture = initial;
    if (cases == 0) {
        printf("SPRITE BB first retail case: old_halfword=%04x operand=%02x expected_halfword=%04x\n",
               old, byte, half((uint8_t *)expected.sprite + 0x30));
        fflush(stdout);
    }
    func_8001FBE4(sprite, opcode, ops);
    if (memcmp(&fixture, &expected, sizeof(fixture))) {
        unsigned first = 0;
        while (first < sizeof(fixture) && ((uint8_t *)&fixture)[first] == ((uint8_t *)&expected)[first]) ++first;
        fprintf(stderr, "SPRITE BB FAIL case=%u old=%04x byte=%02x alias=%u opcode=%08x "
                "diff=%u result=%04x/%04x\n", cases, old, byte, alias, opcode, first,
                half(sprite + 0x30), half((uint8_t *)expected.sprite + 0x30));
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
    /* Captured normal-opening sprite offset 0x1ce0d8 and operand offset
     * 0x18cfaf: halfword +0x30 is zero, signed operand 0xea is -22. */
    compare(0, 0xea, 0, 0);
    for (unsigned old = 0; old < 65536; ++old)
        for (unsigned byte = 0; byte < 256; ++byte)
            compare(old, byte, 0, (old ^ byte) & 3);
    unsigned alias_start = cases;
    for (unsigned old = 0; old < 65536; ++old)
        for (unsigned variant = 0; variant < 4; ++variant) {
            compare(old, old & 255, 1, variant);
            compare(old, old >> 8, 2, variant);
        }
    const unsigned edges[] = {0,1,0x7e,0x7f,0x80,0x81,0xff,0x100,
                              0x7fff,0x8000,0x8001,0xff00,0xff7f,0xff80,0xfffe,0xffff};
    for (unsigned alias = 3; alias < 5; ++alias)
        for (unsigned edge = 0; edge < 16; ++edge)
            for (unsigned byte = 0; byte < 256; ++byte)
                for (unsigned variant = 0; variant < 4; ++variant)
                    compare(edges[edge], byte, alias, variant);
    assert(cases == 17334273 && cases - alias_start == 557056);
    printf("SPRITE BB PASS %u cases: full native/retail dispatcher, complete 65536x256 "
           "halfword/operand domain, signed delta and halfword wrapping, opcode upper bits, "
           "%u destination/adjacent aliases and complete fixture/guard memory\n",
           cases, cases - alias_start);
    puts("SPRITE BB scope: actual retail instruction oracle; controlled low pointers, "
         "no dependency calls, register-state or visible-runtime claim.");
    return 0;
}

/* C5 is outside this test: keep its newly linked dependency fail-closed. */
void func_80022CDC(void *sprite) { (void)sprite; __builtin_trap(); }
