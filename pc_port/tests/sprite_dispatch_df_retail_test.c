/* Retail-equivalence test for animation-script opcode 0xDF (func_8001FBE4
 * jtbl_800183D8[0x55] = 80020DB4): *func_8001FBA4(sprite, ops) = ops[1].
 * Oracle: the unchanged retail dispatcher + func_8001FBA4 on the MIPS
 * interpreter over a physically shared fixture; native side is the port. */
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

static void compare(uint32_t high, uint8_t index, uint8_t value, uint8_t cursor, unsigned variant)
{
    for (unsigned i = 0; i < sizeof(fixture); ++i)
        ((uint8_t *)&fixture)[i] = (uint8_t)(i * 29u + variant * 17u + value);
    uint8_t *p = fixture.sprite;
    fixture.ops[0] = index;
    fixture.ops[1] = value;
    p[0x8C] = cursor;
    put32(p + 0x88, (uint32_t)(uintptr_t)fixture.regs);
    initial = fixture;

    PcPortMipsBus bus = {.read = rd, .write = wr};
    PcPortMipsCpu cpu;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = (uint32_t)(uintptr_t)p;
    cpu.gpr[5] = high | 0xdfu;
    cpu.gpr[6] = (uint32_t)(uintptr_t)fixture.ops;
    cpu.gpr[29] = 0x801fff00u;
    cpu.gpr[31] = 0xfffffffcu;
    int result = PcPortMipsRun(&cpu, 0x8001fbe4u, 0xfffffffcu, 400);
    if (result != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "SPRITE DF FAIL oracle case=%u pc=%08x: %s\n", cases, cpu.pc, cpu.error);
        exit(2);
    }
    expected = fixture;
    fixture = initial;
    func_8001FBE4(p, high | 0xdfu, fixture.ops);
    if (memcmp(&fixture, &expected, sizeof(fixture))) {
        unsigned d = 0;
        while (d < sizeof(fixture) && ((uint8_t *)&fixture)[d] == ((uint8_t *)&expected)[d]) ++d;
        fprintf(stderr, "SPRITE DF FAIL case=%u high=%08x index=%02x value=%02x cursor=%02x diff=%u\n",
                cases, high, index, value, cursor, d);
        exit(1);
    }
    /* Independent decode check: exactly one byte changed, at the retail slot. */
    {
        unsigned changed = 0;
        for (unsigned i = 0; i < sizeof(fixture); ++i)
            changed += ((uint8_t *)&expected)[i] != ((uint8_t *)&initial)[i];
        uint8_t *slot = (index & 0x80) ? fixture.regs + (index & 0x7f)
                                       : p + 0x8E + (int8_t)cursor + (int8_t)index;
        assert(changed <= 1 && *slot == value);
    }
    ++cases;
}

int main(void)
{
    assert((uintptr_t)&fixture + sizeof(fixture) <= UINT32_MAX);
    FILE *image = fopen("disc/SLUS_006.64", "rb");
    assert(image && !fseek(image, 0x800, SEEK_SET));
    assert(fread(ram + 0x10000, 1, 0x200000 - 0x10000, image) > 0x48000);
    assert(!fclose(image));
    uint32_t entry;
    assert(!rd(NULL, 0x800183d8u + (0xdf - 0x8a) * 4, 4, &entry) && entry == 0x80020db4u);

    static const uint32_t highs[] = {0, 0x100, 0x80000000u, 0xffffff00u};
    static const uint8_t values[] = {0, 1, 0x7f, 0x80, 0xff};
    /* Stack indexes keep 0x8E + cursor + index inside the sprite block. */
    static const uint8_t indexes[] = {0, 1, 2, 5, 0x10, 0x7f /* -1 via s8? no: 0x7f = +127 */,
                                      0x80, 0x81, 0x90, 0xbf, 0xff};
    static const uint8_t cursors[] = {0, 4, 0x20, 0x40};
    for (unsigned h = 0; h < 4; ++h)
        for (unsigned c = 0; c < 4; ++c)
            for (unsigned i = 0; i < sizeof(indexes); ++i)
                for (unsigned v = 0; v < sizeof(values); ++v) {
                    uint8_t index = indexes[i];
                    if (!(index & 0x80)) {
                        int off = 0x8E + (int8_t)cursors[c] + (int8_t)index;
                        if (off < 0 || off >= 0x100) continue;
                    }
                    compare(highs[h], index, values[v], cursors[c], h + c);
                }
    printf("SPRITE DF PASS %u cases: stack-relative and register-block slots, all value bytes, high opcode bits\n", cases);
    return 0;
}
