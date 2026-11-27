/* Exercise player-supplied guest code with synthetic state and mocked child services. */
/* Test-only retail instruction oracle, synthetic actor state and spies. */
#include <assert.h>
#include <stdint.h>
static uint32_t fault_addr;
#include "battle_mips_adapter.h"
#include "psx_memory.h"
#include <stdio.h>
#include <string.h>
uint8_t g_PsxRam[PSX_RAM_SIZE], g_PsxScratchpad[4096];
static unsigned count;
static uint32_t invoke(uint32_t target) {
    assert(count < 4);
    ++count;
    return target == 0x80072f38u ? 0x1234u : 0;
}
static uint8_t *resolve(uint32_t a, unsigned w) {
    uintptr_t b = (uintptr_t)g_PsxRam;
    if ((uintptr_t)a >= b && (uintptr_t)a + w <= b + 0x200000)
        return (uint8_t *)(uintptr_t)a;
    unsigned o = a & 0x1fffff;
    if (o + w > 0x200000)
        return NULL;
    return g_PsxRam + o;
}
static int readmem(void *p, uint32_t a, unsigned w, uint32_t *v) {
    (void)p;
    if (a == fault_addr)
        return -1;
    uint8_t *q = resolve(a, w);
    if (!q)
        return -1;
    *v = 0;
    memcpy(v, q, w);
    return 0;
}
static int writemem(void *p, uint32_t a, unsigned w, uint32_t v) {
    (void)p;
    uint8_t *q = resolve(a, w);
    if (!q)
        return -1;
    memcpy(q, &v, w);
    return 0;
}
static int bridge(void *p, PcPortMipsCpu *c, uint32_t t) {
    (void)p;
    if (t == 0x80072a9cu || t == 0x80072da8u || t == 0x80072f38u || t == 0x80073380u) {
        c->gpr[2] = invoke(t);
        c->gpr[3] = 0;
        return 1;
    }
    return 0;
}
static void seed(unsigned k, const uint8_t *code) {
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memcpy(PSX_ADDR(0x800742a0u), code, 260);
    uint8_t *record = PSX_ADDR(0x800d5000u);
    assert((uintptr_t)record <= UINT32_MAX);
    *(uint32_t *)PSX_ADDR(0x800d2d28u) = k == 3 ? (uint32_t)(uintptr_t)record : 0x800d5000u;
    record[0x7c] = k ? 1 : 0;
    *(uint32_t *)(record + 0xe0) = 0x11111111;
    *(uint32_t *)(record + 0xec) = 0x22222222;
    *(uint8_t *)PSX_ADDR(0x800c3eb6u) = k == 1 ? 0x7f : 0;
    *(uint8_t *)PSX_ADDR(0x800d32a1u) = k == 3 ? 7 : 0;
    *(uint8_t *)PSX_ADDR(0x800ccb34u) = 5;
}
int main(int argc, char **argv) {
    FILE *f;
    uint8_t code[260];
    assert(argc == 2);
    f = fopen(argv[1], "rb");
    assert(f);
    assert(!fseek(f, 0x47b0, SEEK_SET));
    assert(fread(code, 1, 260, f) == 260);
    fclose(f);
    for (unsigned k = 0; k < 4; k++) {
        PcPortMipsCpu c;
        PcPortMipsBus bus = {.read = readmem, .write = writemem, .bridge = bridge};
        seed(k, code);
        count = 0;
        PcPortMipsCpuInit(&c, &bus);
        for (unsigned i = 1; i < 32; i++)
            c.gpr[i] = 0x31400000u + i;
        c.gpr[29] = 0x801ff000;
        c.gpr[31] = 0x8000deacu;
        assert(PcPortMipsRun(&c, 0x800742a0u, 0x8000deacu, 5000) == PC_PORT_MIPS_HALTED);
    }
    for (unsigned k = 0; k < 4; k++) {
        PcPortMipsCpu c;
        PcPortMipsBus bus = {.read = readmem, .write = writemem, .bridge = bridge};
        seed(0, code);
        PcPortMipsCpuInit(&c, &bus);
        for (unsigned i = 1; i < 32; i++)
            c.gpr[i] = 0x31400000u + i;
        c.gpr[29] = 0x801ff000;
        c.gpr[31] = 0x8000deacu;
        fault_addr = 0x801fefe0u + 0x1cu - k * 4;
        assert(PcPortMipsRun(&c, 0x800742a0u, 0x8000deacu, 5000) == PC_PORT_MIPS_FAULT);
    }
    return 0;
}
