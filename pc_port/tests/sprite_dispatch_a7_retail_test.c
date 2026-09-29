/* A7 differential test: actual retail dispatcher, timer word, complete sprite
 * fixture. */
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
extern int32_t g_WorkListCurTimer;

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

static void compare(uint32_t high, unsigned operand, unsigned scale,
                    unsigned alias) {
    for (unsigned i = 0; i < sizeof(fixture); i++)
        ((uint8_t *)&fixture)[i] = (uint8_t)(i * 29 + scale);
    uint8_t *p = fixture.sprite;
    put32(p + 0xAC, (scale << 7) | 0xFFF8007Fu);
    uint16_t delay = (uint16_t)(scale * 37u + operand * 257u);
    memcpy(p + 0x9E, &delay, 2);
    uint8_t *ops = alias ? p + alias : fixture.ops;
    ops[0] = operand;
    uint32_t timer = 0xa5879132u ^ scale;
    put32(ram + 0x59428, timer);
    initial = fixture;
    PcPortMipsBus bus = {.read = rd, .write = wr};
    PcPortMipsCpu cpu;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = (uintptr_t)p;
    cpu.gpr[5] = high | 0xA7;
    cpu.gpr[6] = (uintptr_t)ops;
    cpu.gpr[29] = 0x801fff00;
    cpu.gpr[31] = 0xfffffffc;
    if (PcPortMipsRun(&cpu, 0x8001fbe4, 0xfffffffc, 400) !=
        PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "A7 oracle %s\n", cpu.error);
        exit(2);
    }
    expected = fixture;
    uint32_t expected_timer;
    memcpy(&expected_timer, ram + 0x59428, 4);
    fixture = initial;
    g_WorkListCurTimer = (int32_t)timer;
    func_8001FBE4(p, high | 0xA7, ops);
    if (memcmp(&fixture, &expected, sizeof(fixture)) ||
        (uint32_t)g_WorkListCurTimer != expected_timer) {
        fprintf(stderr,
                "SPRITE A7 FAIL case=%u operand=%u scale=%u alias=%u high=%x\n",
                cases, operand, scale, alias, high);
        exit(1);
    }
    cases++;
}
int main(void) {
    assert((uintptr_t)&fixture + sizeof(fixture) <= UINT32_MAX);
    FILE *image = fopen("disc/SLUS_006.64", "rb");
    assert(image && !fseek(image, 0x800, SEEK_SET));
    assert(fread(ram + 0x10000, 1, 0x200000 - 0x10000, image) > 0x48000);
    assert(!fclose(image));
    uint32_t entry;
    assert(!rd(NULL, 0x800183d8 + (0xa7 - 0x8a) * 4, 4, &entry) &&
           entry == 0x8001fdf0);
    for (unsigned scale = 0; scale < 4096; scale++)
        for (unsigned op = 0; op < 256; op++)
            compare(0, op, scale, 0);
    const unsigned aliases[] = {0x9d, 0x9e, 0x9f, 0xac, 0xad, 0xae, 0xaf};
    const unsigned scales[] = {0, 1, 255, 256, 4095};
    const uint32_t highs[] = {0x100, 0x80000000u, 0xffffff00u};
    for (unsigned h = 0; h < 3; h++)
        for (unsigned a = 0; a < 7; a++)
            for (unsigned sc = 0; sc < 5; sc++)
                for (unsigned op = 0; op < 256; op++)
                    compare(highs[h], op, scales[sc], aliases[a]);
    printf("SPRITE A7 PASS %u cases\n", cases);
    return 0;
}
