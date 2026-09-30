/* Differential oracle: full retail sprite dispatcher and variable resolver. */
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "battle_mips_adapter.h"

extern void func_8001FBE4(void *, uint32_t, void *);

static uint8_t ram[0x200000];
static struct {
    uint8_t before[0x40];
    uint8_t sprite[0x100];
    uint8_t ops[8];
    uint8_t regs[0x80];
    uint8_t after[0x40];
} fixture, initial, expected;
static unsigned cases;

static uint8_t *address(uint32_t a, unsigned width)
{
    uintptr_t first = (uintptr_t)&fixture;
    if (a >= first && (uint64_t)a + width <= first + sizeof(fixture))
        return (uint8_t *)(uintptr_t)a;
    if (a >= 0x80000000u && (uint64_t)a + width <= 0x80200000u)
        return ram + (a & 0x1fffffu);
    return NULL;
}
static int rd(void *u, uint32_t a, unsigned w, uint32_t *v)
{
    uint8_t *p = address(a, w); (void)u;
    if (!p) return -1;
    *v = 0; for (unsigned i = 0; i < w; ++i) *v |= (uint32_t)p[i] << (i * 8);
    return 0;
}
static int wr(void *u, uint32_t a, unsigned w, uint32_t v)
{
    uint8_t *p = address(a, w); (void)u;
    if (!p) return -1;
    for (unsigned i = 0; i < w; ++i) p[i] = (uint8_t)(v >> (i * 8));
    return 0;
}
static void put32(uint8_t *p, uint32_t v) { memcpy(p, &v, 4); }

static void compare(uint8_t opcode, uint8_t a, uint8_t b, unsigned variant)
{
    memset(&fixture, 0xA5, sizeof(fixture));
    uint8_t *p = fixture.sprite;
    p[0x8C] = 0;
    put32(p + 0x88, (uint32_t)(uintptr_t)fixture.regs);
    uint8_t *slot = variant & 1 ? p + 0x92 : fixture.regs;
    fixture.ops[0] = variant & 1 ? 4 : 0x80;
    int binary = opcode == 0xD0 || opcode == 0xD1 || opcode == 0xD2 ||
                 opcode == 0xD3 || opcode == 0xD5 || opcode == 0xDD || opcode == 0xDE;
    fixture.ops[1] = binary ? 0x81 : b;
    slot[0] = a;
    slot[1] = b;
    fixture.regs[1] = b;
    if (binary && variant == 2) fixture.ops[1] = fixture.ops[0];
    uint8_t *ops = fixture.ops;
    if (!binary && variant == 2) { ops = slot - 1; ops[0] = 0x80; ops[1] = a; }
    if (!binary && variant == 3) { ops = slot; ops[0] = 4; ops[1] = b; }
    initial = fixture;
    PcPortMipsBus bus = {.read = rd, .write = wr};
    PcPortMipsCpu cpu;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = (uint32_t)(uintptr_t)p;
    cpu.gpr[5] = 0xABCDEF00u | opcode;
    cpu.gpr[6] = (uint32_t)(uintptr_t)ops;
    cpu.gpr[29] = 0x801fff00u;
    cpu.gpr[31] = 0xfffffffcu;
    int result = PcPortMipsRun(&cpu, 0x8001fbe4u, 0xfffffffcu, 400);
    if (result != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "SPRITE ARITH FAIL oracle pc=%08x: %s\n", cpu.pc, cpu.error); exit(2);
    }
    expected = fixture;
    fixture = initial;
    func_8001FBE4(p, 0xABCDEF00u | opcode, ops);
    if (memcmp(&fixture, &expected, sizeof(fixture))) {
        fprintf(stderr, "SPRITE ARITH FAIL case=%u op=%02x a=%02x b=%02x variant=%u\n", cases, opcode, a, b, variant);
        exit(1);
    }
    ++cases;
}
int main(void)
{
    assert((uintptr_t)&fixture + sizeof(fixture) <= UINT32_MAX);
    FILE *image = fopen("disc/SLUS_006.64", "rb");
    assert(image && !fseek(image, 0x800, SEEK_SET));
    assert(fread(ram + 0x10000, 1, sizeof(ram) - 0x10000, image) > 0x48000);
    assert(!fclose(image));
    static const uint8_t opcodes[] = {0xD0,0xD1,0xD2,0xD3,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE};
    static const uint32_t entries[] = {0x80020D68,0x80020BB0,0x80020BE8,0x80020D68,0x80020BE8,0x80020C50,0x80020C74,0x80020C9C,0x80020CC4,0x80020CE8,0x80020D0C,0x80020D34,0x80020D68,0x80020D68};
    for (unsigned o = 0; o < sizeof(opcodes); ++o) {
        uint32_t entry;
        assert(!rd(NULL, 0x800183D8u + (opcodes[o]-0x8A)*4, 4, &entry) && entry == entries[o]);
        for (unsigned a = 0; a < 256; ++a)
            for (unsigned b = 0; b < 256; ++b)
                compare(opcodes[o], a, b, 0);
        for (unsigned variant = 1; variant <= 3; ++variant)
            for (unsigned a = 0; a < 256; ++a)
                for (unsigned b = 0; b < 256; b += 17)
                    compare(opcodes[o], a, b, variant);
    }
    printf("SPRITE ARITH PASS %u cases: 14 retail table entries, exhaustive byte pairs, zero divisors, shift masks, aliases\n", cases);
}
