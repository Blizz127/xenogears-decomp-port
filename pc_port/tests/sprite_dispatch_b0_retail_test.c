/* Retail-equivalence test for animation-script opcodes 0xB0 and 0xB9
 * (func_8001FBE4 jtbl_800183D8[0x26] = 8001FCFC, [0x2F] = 8001FCF0): play
 * the operand byte as an SFX on a sound bank. 0xB0 uses the sound module's
 * selected bank at D_8005919C, 0xB9 the sprite's own bank at +0x50; both
 * share the 8001FD04 tail (null bank -> no call; else
 * func_80039E60(operand | bankId16 << 16)).
 *
 * The oracle is the unchanged retail dispatcher run on the MIPS interpreter
 * with the SFX API call bridged (a0 captured). The native side captures the
 * same call. Both see the same guest RAM (g_PsxRam is the interpreter RAM),
 * so the selector slot, guest-address banks and native-pointer banks are
 * physically shared inputs. */
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "battle_mips_adapter.h"
#include "psx_memory.h"

extern void func_8001FBE4(void *, uint32_t, void *);

uint8_t g_PsxRam[PSX_RAM_SIZE];
#define ram g_PsxRam

static unsigned native_calls, retail_calls;
static uint32_t native_packed, retail_packed;

void func_80039E60(int32_t packed)
{
    ++native_calls;
    native_packed = (uint32_t)packed;
}

static struct {
    uint8_t before[0x40];
    uint8_t sprite[0xc0];
    uint8_t ops[8];
    uint8_t banks[4][0x40];
    uint8_t after[0x40];
} fixture, initial, expected;
static unsigned cases;

static uint8_t *address(uint32_t a, unsigned width)
{
    uintptr_t first = (uintptr_t)&fixture;
    if (a >= first && (uint64_t)a + width <= first + sizeof(fixture))
        return (uint8_t *)(uintptr_t)a;
    if ((a < 0x200000u || (a & 0xffe00000u) == 0x80000000u ||
         (a & 0xffe00000u) == 0xa0000000u) && (a & 0x1fffffu) + width <= 0x200000u)
        return ram + (a & 0x1fffffu);
    return NULL;
}

static int rd(void *unused, uint32_t a, unsigned width, uint32_t *value)
{
    (void)unused;
    uint8_t *p = address(a, width);
    if (!p) return -1;
    *value = 0;
    for (unsigned i = 0; i < width; ++i) *value |= (uint32_t)p[i] << (i * 8);
    return 0;
}

static int wr(void *unused, uint32_t a, unsigned width, uint32_t value)
{
    (void)unused;
    uint8_t *p = address(a, width);
    if (!p) return -1;
    for (unsigned i = 0; i < width; ++i) p[i] = (uint8_t)(value >> (i * 8));
    return 0;
}

static int bridge(void *unused, PcPortMipsCpu *cpu, uint32_t target)
{
    (void)unused;
    if (target == 0x80039e60u) {
        ++retail_calls;
        retail_packed = cpu->gpr[4];
        cpu->gpr[2] = 0;
        return 1;
    }
    return 0;
}

static void put32(uint8_t *p, uint32_t value) { memcpy(p, &value, 4); }
static void put16(uint8_t *p, uint16_t value) { memcpy(p, &value, 2); }

/* scheme: 0 native pointer, 1 kseg0, 2 kuseg, 3 kseg1, 4 null. */
static uint32_t bank_reference(unsigned scheme, unsigned bank)
{
    static const uint32_t guest_base[] = {0, 0x80100000u, 0x00100000u, 0xa0100000u};
    if (scheme == 4) return 0;
    if (scheme == 0) return (uint32_t)(uintptr_t)fixture.banks[bank];
    return guest_base[scheme] + 0x1000u + bank * 0x40u;
}

static uint8_t *bank_storage(unsigned scheme, unsigned bank)
{
    if (scheme == 0) return fixture.banks[bank];
    return ram + 0x101000u + bank * 0x40u;
}

static void compare(unsigned opcode, uint32_t high, uint8_t operand,
                    unsigned scheme, uint16_t id, unsigned other_scheme)
{
    for (unsigned i = 0; i < sizeof(fixture); ++i)
        ((uint8_t *)&fixture)[i] = (uint8_t)(i * 41u + operand * 7u + scheme);
    memset(ram + 0x101000u, 0x5a, 0x100);
    uint8_t *p = fixture.sprite;
    fixture.ops[0] = operand;

    /* The bank the opcode must use carries `id`; the other source (the
     * selector for 0xB9, the sprite for 0xB0) carries a different bank with
     * a different id so a mixed-up source is observable. */
    uint32_t used = bank_reference(scheme, 0);
    uint32_t other = bank_reference(other_scheme, 1);
    if (scheme != 4) put16(bank_storage(scheme, 0) + 0x14, id);
    if (other_scheme != 4) put16(bank_storage(other_scheme, 1) + 0x14, (uint16_t)~id);
    if (opcode == 0xb0) {
        put32(ram + 0x5919cu, used);
        put32(p + 0x50, other);
    } else {
        put32(p + 0x50, used);
        put32(ram + 0x5919cu, other);
    }
    initial = fixture;
    uint8_t initial_ram[0x104];
    memcpy(initial_ram, ram + 0x101000u, 0x100);
    memcpy(initial_ram + 0x100, ram + 0x5919cu, 4);

    PcPortMipsBus bus = {.read = rd, .write = wr, .bridge = bridge};
    PcPortMipsCpu cpu;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = (uint32_t)(uintptr_t)p;
    cpu.gpr[5] = high | opcode;
    cpu.gpr[6] = (uint32_t)(uintptr_t)fixture.ops;
    cpu.gpr[29] = 0x801fff00u;
    cpu.gpr[31] = 0xfffffffcu;
    retail_calls = 0;
    retail_packed = 0xdeadbeefu;
    int result = PcPortMipsRun(&cpu, 0x8001fbe4u, 0xfffffffcu, 400);
    unsigned want_calls = scheme == 4 ? 0u : 1u;
    if (result != PC_PORT_MIPS_HALTED || retail_calls != want_calls ||
        (want_calls && retail_packed != ((uint32_t)operand | ((uint32_t)id << 16)))) {
        fprintf(stderr, "SPRITE B0 FAIL oracle case=%u opcode=%02x pc=%08x calls=%u packed=%08x: %s\n",
                cases, opcode, cpu.pc, retail_calls, retail_packed, cpu.error);
        exit(2);
    }
    expected = fixture;
    uint8_t expected_ram[0x104];
    memcpy(expected_ram, ram + 0x101000u, 0x100);
    memcpy(expected_ram + 0x100, ram + 0x5919cu, 4);

    fixture = initial;
    memcpy(ram + 0x101000u, initial_ram, 0x100);
    memcpy(ram + 0x5919cu, initial_ram + 0x100, 4);
    native_calls = 0;
    native_packed = 0xdeadbeefu;
    func_8001FBE4(p, high | opcode, fixture.ops);
    uint8_t after_ram[0x104];
    memcpy(after_ram, ram + 0x101000u, 0x100);
    memcpy(after_ram + 0x100, ram + 0x5919cu, 4);
    if (memcmp(&fixture, &expected, sizeof(fixture)) ||
        memcmp(after_ram, expected_ram, sizeof(after_ram)) ||
        native_calls != retail_calls ||
        (want_calls && native_packed != retail_packed)) {
        unsigned fdiff = 0, rdiff = 0;
        while (fdiff < sizeof(fixture) &&
               ((uint8_t *)&fixture)[fdiff] == ((uint8_t *)&expected)[fdiff]) ++fdiff;
        while (rdiff < sizeof(after_ram) && after_ram[rdiff] == expected_ram[rdiff]) ++rdiff;
        fprintf(stderr, "SPRITE B0 FAIL case=%u opcode=%02x high=%08x operand=%02x scheme=%u "
                "id=%04x other=%u calls=%u/%u packed=%08x/%08x fixture_diff=%u ram_diff=%u\n",
                cases, opcode, high, operand, scheme, id, other_scheme,
                native_calls, retail_calls, native_packed, retail_packed, fdiff, rdiff);
        if (fdiff < sizeof(fixture))
            fprintf(stderr, "  fixture[%u]: initial=%02x expected(after oracle)=%02x after(native)=%02x\n",
                    fdiff, ((uint8_t *)&initial)[fdiff], ((uint8_t *)&expected)[fdiff],
                    ((uint8_t *)&fixture)[fdiff]);
        exit(1);
    }
    ++cases;
}

int main(void)
{
    assert((uintptr_t)&fixture + sizeof(fixture) <= UINT32_MAX);
    FILE *image = fopen("disc/SLUS_006.64", "rb");
    assert(image && !fseek(image, 0x800, SEEK_SET));
    assert(fread(ram + 0x10000, 1, 0x200000 - 0x10000, image) > 0x48000);
    assert(!fclose(image));
    uint32_t entry;
    assert(!rd(NULL, 0x800183d8u + (0xb0 - 0x8a) * 4, 4, &entry) && entry == 0x8001fcfcu);
    assert(!rd(NULL, 0x800183d8u + (0xb9 - 0x8a) * 4, 4, &entry) && entry == 0x8001fcf0u);

    static const uint8_t operands[] = {0, 1, 2, 0x7f, 0x80, 0xfe, 0xff};
    static const uint16_t ids[] = {0, 1, 0x1234, 0x7fff, 0x8000, 0xffff};
    static const uint32_t highs[] = {0, 0x100, 0x80000000u, 0xffffff00u};
    for (unsigned oi = 0; oi < 2; ++oi)
        for (unsigned h = 0; h < 4; ++h)
            for (unsigned o = 0; o < sizeof(operands); ++o)
                for (unsigned scheme = 0; scheme < 5; ++scheme)
                    for (unsigned i = 0; i < sizeof(ids) / sizeof(ids[0]); ++i)
                        for (unsigned other = 0; other < 5; ++other)
                            compare(oi ? 0xb9 : 0xb0, highs[h], operands[o],
                                    scheme, ids[i], other);
    printf("SPRITE B0 PASS %u cases: 0xB0 selector bank and 0xB9 sprite bank, "
           "native/kseg0/kuseg/kseg1 aliases, null-bank no-op, packed id, no writes\n",
           cases);
    return 0;
}
