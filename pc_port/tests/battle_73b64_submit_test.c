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
static unsigned calls;
static int bridge(void *p, PcPortMipsCpu *c, uint32_t t) {
    (void)p;
    if (t != 0x80043b48u && t != 0x800728b8u)
        return 0;
    assert(calls < 48);
    ++calls;
    c->gpr[2] = 0x1234;
    return 1;
}
static void seed(uint8_t *code, unsigned k) {
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memcpy(PSX_ADDR(0x80073b64u), code, 0x324);
    assert(!wr(NULL, 0x800d2d28u, 4, 0x80150000u));
    assert(!wr(NULL, 0x800c3ea4u, 4, 0x80100000u));
    assert(!wr(NULL, 0x800ccb04u, 4, 0x80190000u));
    assert(!wr(NULL, 0x80150097u, 1, k == 2 ? 1 : 0));
    for (unsigned i = 0; i < 3; i++) {
        assert(!wr(NULL, 0x800c3eb6u + 28 * i, 1, k == 0 ? 0x7f : (k == 1 && i != 0 ? 0x7f : 0)));
        assert(!wr(NULL, 0x801500ccu + i, 1, k == 2));
        assert(!wr(NULL, 0x8010853du + 0x1e4 * i, 1, k == 2 ? 1 : 0));
    }
}
int main(int argc, char **argv) {
    assert(argc == 2);
    FILE *f = fopen(argv[1], "rb");
    assert(f);
    uint8_t code[0x324];
    assert(!fseek(f, 0x4074, SEEK_SET));
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
        c.gpr[29] = 0x801ff000;
        c.gpr[31] = 0x8000deacu;
        c.hi = 0x24681357u;
        c.lo = 0xabcdef01u;
        assert(PcPortMipsRun(&c, 0x80073b64u, 0x8000deacu, 5000) == PC_PORT_MIPS_HALTED);
        assert(calls > 0);
    }
    uint32_t targets[] = {0x80150098u, 0x80150084u, 0x801feffcu};
    for (unsigned k = 0; k < 3; k++) {
        PcPortMipsCpu c;
        fault_addr = 0;
        seed(code, 2);
        calls = 0;
        PcPortMipsCpuInit(&c, &bus);
        for (unsigned i = 1; i < 32; i++)
            c.gpr[i] = 0x31400000u + i;
        c.gpr[29] = 0x801ff000;
        c.gpr[31] = 0x8000deacu;
        c.hi = 0x24681357u;
        c.lo = 0xabcdef01u;
        fault_addr = targets[k];
        assert(PcPortMipsRun(&c, 0x80073b64u, 0x8000deacu, 5000) == PC_PORT_MIPS_FAULT);
    }
    return 0;
}
