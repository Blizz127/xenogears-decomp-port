/* Retail-equivalence test for animation-script opcode 0xA8 (jtbl_800183D8
 * [0x1E] = 8002176C): sprite+0x32 += (s8)ops[0] << 4, then func_80022974.
 * Oracle: retail dispatcher on the MIPS interpreter with the velocity
 * rebuild bridged (call counted, no body); native side calls a recorder. */
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "battle_mips_adapter.h"

extern void func_8001FBE4(void *, uint32_t, void *);
static unsigned retail_calls;

static uint8_t ram[0x200000];
static struct { uint8_t before[0x40]; uint8_t sprite[0x100]; uint8_t ops[8]; uint8_t after[0x40]; } fixture, initial, expected;
static unsigned cases;
static uint8_t *address(uint32_t a, unsigned w)
{
    uintptr_t f = (uintptr_t)&fixture;
    if (a >= f && (uint64_t)a + w <= f + sizeof(fixture)) return (uint8_t *)(uintptr_t)a;
    if (a >= 0x80000000u && (uint64_t)a + w <= 0x80200000u) return ram + (a & 0x1fffffu);
    return NULL;
}
static int rd(void *u, uint32_t a, unsigned w, uint32_t *v) { uint8_t *p = address(a, w); (void)u; if (!p) return -1; *v = 0; for (unsigned i = 0; i < w; ++i) *v |= (uint32_t)p[i] << (i * 8); return 0; }
static int wr(void *u, uint32_t a, unsigned w, uint32_t v) { uint8_t *p = address(a, w); (void)u; if (!p) return -1; for (unsigned i = 0; i < w; ++i) p[i] = (uint8_t)(v >> (i * 8)); return 0; }
/* Count entries into the retail velocity rebuild but let it run as guest
 * code (it and its trig leaves are plain main-exe code in ram). */
static int bridge(void *u, PcPortMipsCpu *cpu, uint32_t target)
{
    (void)u;
    if (target == 0x80022974u) { ++retail_calls; assert(cpu->gpr[4] == (uint32_t)(uintptr_t)fixture.sprite); }
    return 0;
}
/* The native func_80022974 (same TU as the handler) calls the port's
 * rcos/rsin; back them with the retail leaves run on the interpreter so both
 * sides use identical trig.  Retail 8003F8CC is the port's rcos, 8003F8B0
 * its rsin (see the libgte shim note in sprite_dispatch_a5). */
static int guest_trig(uint32_t entry, int angle)
{
    PcPortMipsBus bus = {.read = rd, .write = wr};
    PcPortMipsCpu cpu; PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = (uint32_t)angle; cpu.gpr[29] = 0x801ffe00u; cpu.gpr[31] = 0xfffffffcu;
    int r = PcPortMipsRun(&cpu, entry, 0xfffffffcu, 200);
    assert(r == PC_PORT_MIPS_HALTED);
    return (int)cpu.gpr[2];
}
int rcos(int angle) { return guest_trig(0x8003f8ccu, angle); }
int rsin(int angle) { return guest_trig(0x8003f8b0u, angle); }
static void compare(uint32_t high, uint8_t op, uint16_t angle, unsigned variant)
{
    for (unsigned i = 0; i < sizeof(fixture); ++i) ((uint8_t *)&fixture)[i] = (uint8_t)(i * 31u + variant * 11u + op);
    uint8_t *p = fixture.sprite; fixture.ops[0] = op; memcpy(p + 0x32, &angle, 2);
    initial = fixture;
    PcPortMipsBus bus = {.read = rd, .write = wr, .bridge = bridge};
    PcPortMipsCpu cpu; PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = (uint32_t)(uintptr_t)p; cpu.gpr[5] = high | 0xa8u; cpu.gpr[6] = (uint32_t)(uintptr_t)fixture.ops;
    cpu.gpr[29] = 0x801fff00u; cpu.gpr[31] = 0xfffffffcu; retail_calls = 0;
    if (PcPortMipsRun(&cpu, 0x8001fbe4u, 0xfffffffcu, 400) != PC_PORT_MIPS_HALTED || retail_calls != 1) {
        fprintf(stderr, "SPRITE A8 FAIL oracle case=%u pc=%08x calls=%u: %s\n", cases, cpu.pc, retail_calls, cpu.error); exit(2);
    }
    expected = fixture; fixture = initial;
    func_8001FBE4(p, high | 0xa8u, fixture.ops);
    uint16_t got; memcpy(&got, p + 0x32, 2);
    if (memcmp(&fixture, &expected, sizeof(fixture)) ||
        got != (uint16_t)(angle + (uint16_t)((int32_t)(int8_t)op * 16))) {
        unsigned d = 0; while (d < sizeof(fixture) && ((uint8_t *)&fixture)[d] == ((uint8_t *)&expected)[d]) ++d;
        fprintf(stderr, "SPRITE A8 FAIL case=%u high=%08x op=%02x angle=%04x got=%04x diff=%u\n", cases, high, op, angle, got, d); exit(1);
    }
    ++cases;
}
int main(void)
{
    assert((uintptr_t)&fixture + sizeof(fixture) <= UINT32_MAX);
    FILE *image = fopen("disc/SLUS_006.64", "rb"); assert(image && !fseek(image, 0x800, SEEK_SET));
    assert(fread(ram + 0x10000, 1, 0x200000 - 0x10000, image) > 0x48000); assert(!fclose(image));
    uint32_t entry; assert(!rd(NULL, 0x800183d8u + (0xa8 - 0x8a) * 4, 4, &entry) && entry == 0x8002176cu);
    static const uint32_t highs[] = {0, 0x100, 0x80000000u, 0xffffff00u};
    static const uint16_t angles[] = {0, 1, 0x7ff, 0x800, 0xfff, 0x1000, 0x7fff, 0x8000, 0xffff, 0xfff0};
    for (unsigned h = 0; h < 4; ++h) for (unsigned a = 0; a < 10; ++a) for (unsigned op = 0; op < 256; ++op)
        compare(highs[h], (uint8_t)op, angles[a], h + a);
    printf("SPRITE A8 PASS %u cases: all operand bytes x 10 angles x high bits, delay-slot store before the velocity call\n", cases);
    return 0;
}
