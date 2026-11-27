/* Exercise player-supplied guest code with synthetic state and mocked child services. */
#include <assert.h>
#include <stdint.h>
static uint32_t fault_addr;
#include "battle_mips_adapter.h"
#include "psx_memory.h"
#include <stdio.h>
#include <string.h>
uint8_t g_PsxRam[PSX_RAM_SIZE], g_PsxScratchpad[4096];
static int rd(void *p, uint32_t a, unsigned w, uint32_t *v) {
    (void)p;
    if (a == fault_addr)
        return -1;
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
static void seed(uint8_t *code, unsigned k) {
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memcpy(PSX_ADDR(0x80075168u), code, 0x7d0);
    assert(!wr(NULL, 0x800c3ea4u, 4, 0x80100000u));
    assert(!wr(NULL, 0x8010853du, 1, k));
    assert(!wr(NULL, 0x800ccdc4u, 2, 25));
    assert(!wr(NULL, 0x800ccdc6u, 2, 100));
}
int main(int argc, char **argv) {
    assert(argc == 2);
    FILE *f = fopen(argv[1], "rb");
    assert(f);
    uint8_t code[0x7d0];
    assert(!fseek(f, 0x5678, SEEK_SET));
    assert(fread(code, 1, 0x7d0, f) == 0x7d0);
    fclose(f);
    uint32_t values[] = {0u, 1u, 2u};
    PcPortMipsBus bus = {.read = rd, .write = wr};
    for (unsigned k = 0; k < 3; k++) {
        PcPortMipsCpu c;
        seed(code, k);
        PcPortMipsCpuInit(&c, &bus);
        for (unsigned i = 1; i < 32; i++)
            c.gpr[i] = 0x31400000u + i;
        c.gpr[4] = values[k];
        c.gpr[29] = 0x801ff000;
        c.gpr[31] = 0x8000deacu;
        c.hi = 0x24681357u;
        c.lo = 0xabcdef01u;
        assert(PcPortMipsRun(&c, 0x80075168u, 0x8000deacu, 5000) == PC_PORT_MIPS_HALTED);
    }
    {
        PcPortMipsCpu c;
        fault_addr = 0;
        seed(code, 1);
        PcPortMipsCpuInit(&c, &bus);
        for (unsigned i = 1; i < 32; i++)
            c.gpr[i] = 0x31400000u + i;
        c.gpr[29] = 0x801ff000;
        c.gpr[31] = 0x8000deacu;
        c.hi = 0x24681357u;
        c.lo = 0xabcdef01u;
        fault_addr = 0x800d32a0u;
        assert(PcPortMipsRun(&c, 0x80075168u, 0x8000deacu, 5000) == PC_PORT_MIPS_FAULT);
    }
    return 0;
}
