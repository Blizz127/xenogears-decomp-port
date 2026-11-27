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
    *v = 0;
    memcpy(v, PSX_ADDR(a), w);
    return 0;
}
static int wr(void *p, uint32_t a, unsigned w, uint32_t v) {
    (void)p;
    memcpy(PSX_ADDR(a), &v, w);
    return 0;
}
static unsigned calls;
static int bridge(void *p, PcPortMipsCpu *c, uint32_t t) {
    (void)p;
    if (t != 0x800bc2f0u && t != 0x800bc460u)
        return 0;
    assert(calls < 2);
    ++calls;
    c->gpr[2] = 0x1234;
    c->gpr[3] = 0x5678;
    return 1;
}
int main(void) {
    FILE *f = fopen("disc/battle.bin", "rb");
    assert(f);
    assert(!fseek(f, 0x4c914, SEEK_SET));
    assert(fread(PSX_ADDR(0x800bc404u), 1, 80, f) == 80);
    fclose(f);
    PcPortMipsBus bus = {.read = rd, .write = wr, .bridge = bridge};
    for (unsigned k = 0; k < 2; k++) {
        wr(NULL, 0x800c37c8u, 1, k);
        wr(NULL, 0x800c3cdcu, 2, 0xa55a);
        PcPortMipsCpu cpu;
        PcPortMipsCpuInit(&cpu, &bus);
        for (unsigned i = 1; i < 32; i++)
            cpu.gpr[i] = 0x31400000u + i;
        cpu.gpr[4] = 0x1234u;
        cpu.gpr[29] = 0x801ff000u;
        cpu.gpr[31] = 0x80080644u;
        calls = 0;
        assert(PcPortMipsRun(&cpu, 0x800bc404u, 0x80080644u, 1000) == PC_PORT_MIPS_HALTED);
        assert(calls == (k ? 0u : 2u));
    }
    return 0;
}
