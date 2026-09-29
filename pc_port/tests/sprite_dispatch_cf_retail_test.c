/* Retail-equivalence test for animation-script opcode 0xCF (jtbl_800183D8
 * [0x45] = 8002008C): model part angle set/add with the 800215A4 flag tail.
 * Oracle: retail dispatcher on the MIPS interpreter over a shared fixture. */
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "battle_mips_adapter.h"

extern void func_8001FBE4(void *, uint32_t, void *);
static uint8_t ram[0x200000];
static struct { uint8_t before[0x40]; uint8_t sprite[0x100]; uint8_t ops[8]; uint8_t model[0x40]; uint8_t table[8 * 8]; uint8_t after[0x40]; } fixture, initial, expected;
static unsigned cases;
static uint8_t *address(uint32_t a, unsigned w)
{
    uintptr_t f = (uintptr_t)&fixture;
    if (a >= f && (uint64_t)a + w <= f + sizeof(fixture)) return (uint8_t *)(uintptr_t)a;
    if (a >= 0x80000000u && (uint64_t)a + w <= 0x80200000u) return ram + (a & 0x1fffffu);
    return NULL;
}
static int rd(void *u, uint32_t a, unsigned w, uint32_t *v) { uint8_t *p = address(a, w); (void)u; if (!p) return -1; *v = 0; for (unsigned i = 0; i < w; ++i) *v |= (uint32_t)p[i] << (i * 8); return 0; }
static int wr(void *u, uint32_t a, unsigned w, uint32_t v) { uint8_t *p = address(a, w); (void)u; if (!p) return -1; for (unsigned i = 0; i < w; ++i) p[i] = (uint8_t)(v >> (i * 8)); return 0; }
static void put32(uint8_t *p, uint32_t v) { memcpy(p, &v, 4); }
static void compare(uint32_t high, uint16_t v, uint32_t flagsAC, int nullModel, int nullTable, unsigned variant)
{
    for (unsigned i = 0; i < sizeof(fixture); ++i) ((uint8_t *)&fixture)[i] = (uint8_t)(i * 37u + variant * 13u + v);
    uint8_t *p = fixture.sprite; fixture.ops[0] = (uint8_t)v; fixture.ops[1] = (uint8_t)(v >> 8);
    put32(p + 0x20, nullModel ? 0u : (uint32_t)(uintptr_t)fixture.model);
    put32(p + 0xAC, flagsAC);
    put32(fixture.model + 0x34, nullTable ? 0u : (uint32_t)(uintptr_t)fixture.table);
    initial = fixture;
    PcPortMipsBus bus = {.read = rd, .write = wr};
    PcPortMipsCpu cpu; PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = (uint32_t)(uintptr_t)p; cpu.gpr[5] = high | 0xcfu; cpu.gpr[6] = (uint32_t)(uintptr_t)fixture.ops;
    cpu.gpr[29] = 0x801fff00u; cpu.gpr[31] = 0xfffffffcu;
    if (PcPortMipsRun(&cpu, 0x8001fbe4u, 0xfffffffcu, 400) != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "SPRITE CF FAIL oracle case=%u pc=%08x: %s\n", cases, cpu.pc, cpu.error); exit(2);
    }
    expected = fixture; fixture = initial;
    func_8001FBE4(p, high | 0xcfu, fixture.ops);
    if (memcmp(&fixture, &expected, sizeof(fixture))) {
        unsigned d = 0; while (d < sizeof(fixture) && ((uint8_t *)&fixture)[d] == ((uint8_t *)&expected)[d]) ++d;
        fprintf(stderr, "SPRITE CF FAIL case=%u high=%08x v=%04x flagsAC=%08x nullModel=%d nullTable=%d diff=%u\n", cases, high, v, flagsAC, nullModel, nullTable, d); exit(1);
    }
    ++cases;
}
int main(void)
{
    assert((uintptr_t)&fixture + sizeof(fixture) <= UINT32_MAX);
    FILE *image = fopen("disc/SLUS_006.64", "rb"); assert(image && !fseek(image, 0x800, SEEK_SET));
    assert(fread(ram + 0x10000, 1, 0x200000 - 0x10000, image) > 0x48000); assert(!fclose(image));
    uint32_t entry; assert(!rd(NULL, 0x800183d8u + (0xcf - 0x8a) * 4, 4, &entry) && entry == 0x8002008cu);
    static const uint32_t highs[] = {0, 0x100, 0x80000000u, 0xffffff00u};
    static const uint16_t amounts[] = {0, 1, 0x7f, 0x80, 0xff, 0x100, 0x1ff};
    static const uint32_t flags[] = {0, 4, 0xfffffffbu, 0xffffffffu};
    for (unsigned h = 0; h < 4; ++h) for (unsigned f = 0; f < 4; ++f) for (unsigned idx = 0; idx < 8; ++idx) for (unsigned set = 0; set < 2; ++set)
        for (unsigned a = 0; a < 7; ++a) for (unsigned nm = 0; nm < 2; ++nm) for (unsigned nt = 0; nt < 2; ++nt) {
            uint16_t v = (uint16_t)(amounts[a] | (idx << 9) | (set << 12) | ((h & 1) ? 0x8000u : 0u));
            compare(highs[h], v, flags[f], (int)nm, (int)nt, h * 4 + f + idx);
        }
    printf("SPRITE CF PASS %u cases: amount/index/set-add/negate/sign, null model and table, flag tail\n", cases);
    return 0;
}
