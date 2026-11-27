/* Exercise player-supplied guest code with synthetic state and mocked child services. */
#include "battle_mips_adapter.h"
#include "psx_memory.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
uint8_t g_PsxRam[PSX_RAM_SIZE], g_PsxScratchpad[4096];
static uint32_t fault_addr;
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
static unsigned calls;
static int bridge(void *p, PcPortMipsCpu *c, uint32_t t) {
    (void)p;
    if (t != 0x80099cf0u)
        return 0;
    assert(calls < 8);
    ++calls;
    c->gpr[2] = 0x1234;
    return 1;
}
static void seed(uint8_t *code, unsigned k) {
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memcpy(PSX_ADDR(0x80099890u), code, 0x460);
    uint32_t base = 0x800ccce8u + (k == 4 ? 3u * 0x170u : 0u);
    if (k == 1 || k == 2 || k == 4) {
        assert(!wr(NULL, base + 0x7c, 2, 0x1000));
        assert(!wr(NULL, base + 0x80, 2, 0x1800));
        assert(!wr(NULL, base + 0x84, 2, 0xf800));
        assert(!wr(NULL, base + 0x88, 2, 0xd000));
        assert(!wr(NULL, base + 0x8c, 2, 0xffff));
        for (unsigned j = 1; j <= 13; j++)
            assert(!wr(NULL, base + 0x15c + j, 1, k == 2 ? 2 : 1));
    }
    if (k == 3)
        assert(!wr(NULL, base + 0x15a, 1, 0x80));
}
int main(int argc, char **argv) {
    assert(argc == 2);
    FILE *f = fopen(argv[1], "rb");
    assert(f);
    uint8_t code[0x460];
    assert(!fseek(f, 0x29da0, SEEK_SET));
    assert(fread(code, 1, sizeof(code), f) == sizeof(code));
    fclose(f);
    PcPortMipsBus bus = {.read = rd, .write = wr, .bridge = bridge};
    for (unsigned k = 0; k < 5; k++) {
        if (k == 3)
            continue;
        PcPortMipsCpu c;
        seed(code, k);
        calls = 0;
        PcPortMipsCpuInit(&c, &bus);
        for (unsigned i = 1; i < 32; i++)
            c.gpr[i] = 0x31400000u + i;
        c.gpr[4] = k == 4 ? 3 : 0;
        c.gpr[29] = 0x801ff000;
        c.gpr[31] = 0x8000deacu;
        c.hi = 0x24681357u;
        c.lo = 0xabcdef01u;
        assert(PcPortMipsRun(&c, 0x80099890u, 0x8000deacu, 5000) == PC_PORT_MIPS_HALTED);
        assert(calls == 0);
    }
    uint32_t faults[] = {0x800cce42u, 0x800ccd64u, 0x800ccd68u, 0x800ccd62u, 0x801feff8u};
    for (unsigned k = 0; k < 5; k++) {
        PcPortMipsCpu c;
        fault_addr = 0;
        seed(code, 1);
        calls = 0;
        PcPortMipsCpuInit(&c, &bus);
        for (unsigned i = 1; i < 32; i++)
            c.gpr[i] = 0x31400000u + i;
        c.gpr[4] = 0;
        c.gpr[29] = 0x801ff000;
        c.gpr[31] = 0x8000deacu;
        c.hi = 0x24681357u;
        c.lo = 0xabcdef01u;
        fault_addr = faults[k];
        assert(PcPortMipsRun(&c, 0x80099890u, 0x8000deacu, 5000) == PC_PORT_MIPS_FAULT);
    }
    return 0;
}
