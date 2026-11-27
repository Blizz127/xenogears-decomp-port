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
    if (t != 0x80025fa8u && t != 0x80043bfcu)
        return 0;
    assert(calls < 32);
    ++calls;
    c->gpr[2] = 1;
    return 1;
}
static void seed(uint8_t *code, unsigned k) {
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memcpy(PSX_ADDR(0x80076710u), code, 0x2d8);
    assert(!wr(NULL, 0x800d2d28u, 4, 0x80100000u));
    assert(!wr(NULL, 0x800c3ea4u, 4, 0x80150000u));
    assert(!wr(NULL, 0x800d2f5cu, 4, 0x80140000u));
    assert(!wr(NULL, 0x800ccb34u, 4, 0));
    assert(!wr(NULL, 0x801000a9u, 1, k == 0 ? 0 : k == 1 ? 1 : 6));
    assert(!wr(NULL, 0x801000abu, 1, k == 2 ? 2 : 1));
    assert(!wr(NULL, 0x80100034u, 4, 100));
    assert(!wr(NULL, 0x8010003cu, 4, 16));
    assert(!wr(NULL, 0x80100044u, 4, 28));
    assert(!wr(NULL, 0x8010004cu, 4, 152));
    assert(!wr(NULL, 0x80100054u, 4, 256));
    assert(!wr(NULL, 0x8010005cu, 4, 256));
    assert(!wr(NULL, 0x80100104u, 2, 0x800));
}
int main(int argc, char **argv) {
    assert(argc == 2);
    FILE *f = fopen(argv[1], "rb");
    assert(f);
    uint8_t code[0x2d8];
    assert(!fseek(f, 0x6c20, SEEK_SET));
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
        c.gpr[4] = k;
        c.gpr[29] = 0x801ff000;
        c.gpr[31] = 0x8000deacu;
        c.hi = 0x24681357u;
        c.lo = 0xabcdef01u;
        assert(PcPortMipsRun(&c, 0x80076710u, 0x8000deacu, 5000) == PC_PORT_MIPS_HALTED);
        assert(calls >= 2u);
    }
    uint32_t faults[] = {0x80100054u, 0x8010006cu, 0x8010005cu, 0x801fefb8u, 0x801feff0u};
    for (unsigned k = 0; k < 5; k++) {
        PcPortMipsCpu c;
        read_fault = write_fault = 0;
        seed(code, 1);
        calls = 0;
        PcPortMipsCpuInit(&c, &bus);
        for (unsigned i = 1; i < 32; i++)
            c.gpr[i] = 0x31400000u + i;
        c.gpr[4] = 1;
        c.gpr[29] = 0x801ff000;
        c.gpr[31] = 0x8000deacu;
        c.hi = 0x24681357u;
        c.lo = 0xabcdef01u;
        if (k == 3)
            write_fault = faults[k];
        else
            read_fault = faults[k];
        assert(PcPortMipsRun(&c, 0x80076710u, 0x8000deacu, 5000) == PC_PORT_MIPS_FAULT);
    }
    return 0;
}
