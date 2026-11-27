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
static uint8_t refresh_code[0x7d0];
static unsigned calls;
static int bridge(void *p, PcPortMipsCpu *c, uint32_t t) {
    (void)p;
    (void)c;
    if (t != 0x80075168u)
        return 0;
    assert(calls++ == 0);
    return 0;
}
static void seed(uint8_t *code, unsigned k) {
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memcpy(PSX_ADDR(0x80075938u), code, 0xae0);
    memcpy(PSX_ADDR(0x80075168u), refresh_code, 0x7d0);
    assert(!wr(NULL, 0x8010853du, 1, k));
    assert(!wr(NULL, 0x800ccdc4u, 2, 25));
    assert(!wr(NULL, 0x800ccdc6u, 2, 100));
    assert(!wr(NULL, 0x800d2d28u, 4, 0x80150000u));
    assert(!wr(NULL, 0x800c3ea4u, 4, 0x80100000u));
    for (unsigned i = 0; i < 3; i++)
        assert(!wr(NULL, 0x800c3eb6u + 28 * i, 1, (k && i == 0) ? 0 : 0x7f));
    assert(!wr(NULL, 0x80150090u, 1, k == 2));
    assert(!wr(NULL, 0x800d2df0u, 2, 100));
    assert(!wr(NULL, 0x800d2e06u, 2, 25));
    assert(!wr(NULL, 0x801fefe0u, 4, 0xfffffff0u));
}
int main(int argc, char **argv) {
    assert(argc == 2);
    FILE *f = fopen(argv[1], "rb");
    assert(f);
    uint8_t code[0xae0];
    assert(!fseek(f, 0x5e48, SEEK_SET));
    assert(fread(code, 1, sizeof(code), f) == sizeof(code));
    assert(!fseek(f, 0x5678, SEEK_SET));
    assert(fread(refresh_code, 1, 0x7d0, f) == 0x7d0);
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
        assert(PcPortMipsRun(&c, 0x80075938u, 0x8000deacu, 5000) == PC_PORT_MIPS_HALTED);
        assert(calls == 1);
    }
    return 0;
}
