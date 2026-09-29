/* Retail E9/EA/EB model-axis deltas and dirty flag; whole-fixture oracle. */
#include "battle_mips_adapter.h"
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static uint8_t *address(uint32_t a, unsigned width) {
    uintptr_t first = (uintptr_t)&fixture;
    if (a >= first && (uint64_t)a + width <= first + sizeof(fixture))
        return (uint8_t *)(uintptr_t)a;
    if (a >= 0x80000000u && (uint64_t)a + width <= 0x80200000u)
        return ram + (a & 0x1fffffu);
    return NULL;
}
static int rd(void *u, uint32_t a, unsigned w, uint32_t *v) {
    uint8_t *p = address(a, w);
    (void)u;
    if (!p)
        return -1;
    *v = 0;
    for (unsigned i = 0; i < w; ++i)
        *v |= (uint32_t)p[i] << (i * 8);
    return 0;
}
static int wr(void *u, uint32_t a, unsigned w, uint32_t v) {
    uint8_t *p = address(a, w);
    (void)u;
    if (!p)
        return -1;
    for (unsigned i = 0; i < w; ++i)
        p[i] = (uint8_t)(v >> (i * 8));
    return 0;
}
static void put32(uint8_t *p, uint32_t v) { memcpy(p, &v, 4); }

static void compare(unsigned opcode, unsigned imm, unsigned variant,
                    uint32_t high) {
    for (unsigned i = 0; i < sizeof(fixture); i++)
        ((uint8_t *)&fixture)[i] = (uint8_t)(i * 31 + imm);
    uint8_t *p = fixture.sprite;
    unsigned off = 6 + 2 * (opcode - 0xe9);
    uint8_t *target = variant == 0   ? NULL
                      : variant == 2 ? p + 0x3c - off
                      : variant == 4 ? p + 0x3e - off
                                     : fixture.regs;
    put32(p + 0x20, (uintptr_t)target);
    uint8_t *ops = variant == 3 ? target + off : fixture.ops;
    ops[0] = imm;
    ops[1] = imm >> 8;
    initial = fixture;
    PcPortMipsBus bus = {.read = rd, .write = wr};
    PcPortMipsCpu cpu;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = (uintptr_t)p;
    cpu.gpr[5] = high | opcode;
    cpu.gpr[6] = (uintptr_t)ops;
    cpu.gpr[29] = 0x801fff00;
    cpu.gpr[31] = 0xfffffffc;
    if (PcPortMipsRun(&cpu, 0x8001fbe4, 0xfffffffc, 400) !=
        PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "AXIS oracle %s\n", cpu.error);
        exit(2);
    }
    expected = fixture;
    fixture = initial;
    func_8001FBE4(p, high | opcode, ops);
    if (memcmp(&fixture, &expected, sizeof(fixture))) {
        fprintf(stderr,
                "SPRITE AXIS FAIL case=%u op=%x imm=%x variant=%u high=%x\n",
                cases, opcode, imm, variant, high);
        exit(1);
    }
    cases++;
}
int main(void) {
    assert((uintptr_t)&fixture + sizeof(fixture) <= UINT32_MAX);
    FILE *f = fopen("disc/SLUS_006.64", "rb");
    assert(f && !fseek(f, 0x800, SEEK_SET));
    assert(fread(ram + 0x10000, 1, 0x1f0000, f) > 0x48000);
    assert(!fclose(f));
    for (unsigned op = 0xe9; op <= 0xeb; op++) {
        uint32_t entry;
        assert(!rd(NULL, 0x800183d8 + (op - 0x8a) * 4, 4, &entry) &&
               entry == 0x80021374 + (op - 0xe9) * 0x34);
        for (unsigned imm = 0; imm < 65536; imm++)
            for (unsigned v = 0; v < 5; v++)
                compare(op, imm, v, 0);
        const uint32_t highs[] = {0x100, 0x80000000u, 0xffffff00u};
        for (unsigned h = 0; h < 3; h++)
            for (unsigned imm = 0; imm < 65536; imm += 127)
                compare(op, imm, 1, highs[h]);
    }
    printf("SPRITE AXIS PASS %u cases\n", cases);
    return 0;
}
