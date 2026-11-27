/* Exercise player-supplied guest code with synthetic state and mocked child services. */
#include "battle_mips_adapter.h"
#include "psx_memory.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
uint8_t g_PsxRam[PSX_RAM_SIZE], g_PsxScratchpad[4096];
static uint32_t read_fault, write_fault;
static int rd(void *p, uint32_t a, unsigned w, uint32_t *v) {
    (void)p;
    if (a == read_fault)
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
    if (a == write_fault)
        return -1;
    uint32_t o = a & 0x1fffffu;
    if (o + w > PSX_RAM_SIZE)
        return -1;
    memcpy(g_PsxRam + o, &v, w);
    return 0;
}
static unsigned calls;
static int bridge(void *p, PcPortMipsCpu *c, uint32_t t) {
    (void)p;
    if (t != 0x80043b48u)
        return 0;
    assert(calls < 8);
    ++calls;
    c->gpr[2] = 0x1234;
    return 1;
}
static void seed(uint8_t *code, unsigned k) {
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memcpy(PSX_ADDR(0x8004b18cu), code, 0x198);
    uint8_t *m = PSX_ADDR(0x80100000u);
    for (unsigned j = 0; j < 32; j++)
        m[j] = (uint8_t)(0x30u + j * 7u);
    unsigned index = k == 0 ? 0 : k == 1 ? 1024 : 1;
    assert(!wr(NULL, 0x800523f0u + 4 * index, 4,
               k == 0   ? 0x10000000u
               : k == 1 ? 0x00001000u
                        : 0x0fff0006u));
}
int main(int argc, char **argv) {
    assert(argc == 2);
    FILE *f = fopen(argv[1], "rb");
    assert(f);
    uint8_t code[0x198];
    assert(!fseek(f, 0x3b98c, SEEK_SET));
    assert(fread(code, 1, sizeof(code), f) == sizeof(code));
    fclose(f);
    PcPortMipsBus bus = {.read = rd, .write = wr, .bridge = bridge};
    for (unsigned k = 0; k < 3; k++) {
        PcPortMipsCpu c;
        seed(code, k);
        calls = 0;
        PcPortMipsCpuInit(&c, &bus);
        for (unsigned i = 1; i < 32; i++)
            c.gpr[i] = 0x31400000u + i;
        c.gpr[4] = k == 0 ? 0u : k == 1 ? 1024u : 0xffffffffu;
        c.gpr[5] = 0x80100000u;
        c.gpr[29] = 0x801ff000;
        c.gpr[31] = 0x8000deacu;
        c.hi = 0x24681357u;
        c.lo = 0xabcdef01u;
        assert(PcPortMipsRun(&c, 0x8004b18cu, 0x8000deacu, 5000) == PC_PORT_MIPS_HALTED);
        assert(calls == 0);
    }
    uint32_t faults[] = {0x800533f0u, 0x80100000u, 0x80100006u, 0x80100002u,
                         0x80100008u, 0x80100000u, 0x8010000au};
    for (unsigned k = 0; k < 7; k++) {
        PcPortMipsCpu c;
        read_fault = write_fault = 0;
        seed(code, 1);
        PcPortMipsCpuInit(&c, &bus);
        for (unsigned i = 1; i < 32; i++)
            c.gpr[i] = 0x31400000u + i;
        c.gpr[4] = 1024u;
        c.gpr[5] = 0x80100000u;
        c.gpr[29] = 0x801ff000;
        c.gpr[31] = 0x8000deacu;
        c.hi = 0x24681357u;
        c.lo = 0xabcdef01u;
        if (k >= 5)
            write_fault = faults[k];
        else
            read_fault = faults[k];
        assert(PcPortMipsRun(&c, 0x8004b18cu, 0x8000deacu, 5000) == PC_PORT_MIPS_FAULT);
    }
    return 0;
}
