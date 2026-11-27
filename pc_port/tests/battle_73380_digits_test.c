/* Exercise player-supplied guest code with synthetic state and mocked child services. */
#include "battle_mips_adapter.h"
#include "psx_memory.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
uint8_t g_PsxRam[PSX_RAM_SIZE], g_PsxScratchpad[4096];
static int rd(void *p, uint32_t a, unsigned w, uint32_t *v) {
    (void)p;
    uint32_t o = a & 0x1fffffu;
    if (o + w > PSX_RAM_SIZE)
        return -1;
    *v = 0;
    memcpy(v, g_PsxRam + o, w);
    return 0;
}
static int wr(void *p, uint32_t a, unsigned w, uint32_t v) {
    (void)p;
    uint32_t o = a & 0x1fffffu;
    if (o + w > PSX_RAM_SIZE)
        return -1;
    memcpy(g_PsxRam + o, &v, w);
    return 0;
}
static unsigned calls;
static int bridge(void *p, PcPortMipsCpu *c, uint32_t t) {
    (void)p;
    if (t != 0x8008aaa0u && t != 0x80076a10u)
        return 0;
    assert(calls < 8);
    ++calls;
    if (t == 0x8008aaa0u) {
        return 0;
    }
    c->gpr[2] = 1;
    return 1;
}
static uint8_t digits_code[172];
static void seed(const uint8_t *code, unsigned k) {
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memcpy(PSX_ADDR(0x80073380u), code, 440);
    memcpy(PSX_ADDR(0x8008aaa0u), digits_code, 172);
    *(uint32_t *)PSX_ADDR(0x800c3ea4u) = 0x800d6000u;
    *(uint32_t *)PSX_ADDR(0x800d2d28u) = 0x800d5000u;
    *(uint8_t *)PSX_ADDR(0x800de53du) = k ? 1 : 0;
    *(uint16_t *)PSX_ADDR(0x800ccdc4u) = 0x120;
    *(uint8_t *)PSX_ADDR(0x800d32a0u) = 23;
    *(uint16_t *)PSX_ADDR(0x800c3254u) = 40;
    *(uint8_t *)PSX_ADDR(0x800ccb34u) = 9;
}
int main(int argc, char **argv) {
    assert(argc == 2);
    FILE *f = fopen(argv[1], "rb");
    assert(f);
    uint8_t code[440];
    assert(!fseek(f, 0x3890, SEEK_SET));
    assert(fread(code, 1, 440, f) == 440);
    assert(!fseek(f, 0x1afb0, SEEK_SET));
    assert(fread(digits_code, 1, 172, f) == 172);
    fclose(f);
    PcPortMipsBus bus = {.read = rd, .write = wr, .bridge = bridge};
    for (unsigned k = 0; k < 2; k++) {
        PcPortMipsCpu c;
        seed(code, k);
        calls = 0;
        PcPortMipsCpuInit(&c, &bus);
        for (unsigned i = 1; i < 32; i++)
            c.gpr[i] = 0x31400000u + i;
        c.gpr[4] = 0;
        c.gpr[29] = 0x801ff000;
        c.gpr[31] = 0x8000deacu;
        assert(PcPortMipsRun(&c, 0x80073380u, 0x8000deacu, 5000) == PC_PORT_MIPS_HALTED);
        assert(calls >= 2 && calls <= 5);
    }
    return 0;
}
