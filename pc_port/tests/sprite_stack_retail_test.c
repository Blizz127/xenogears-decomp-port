#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "battle_mips_adapter.h"

/* Real animation_scripts.c helpers against pinned SLUS leaf functions.
 * The wider arena maps every signed-index access, including index aliases;
 * this is helper memory/return proof, not real-game allocation validity. */
/* Match the generic bridge's full MIPS-sized return-register contract. */
extern uint32_t AnimScriptStackPopU8(void *);
extern int32_t AnimScriptStackPopU16(void *);
extern int32_t AnimScriptStackPopU24(void *);
extern void AnimScriptStackPushU8(void *, uint8_t);
extern void AnimScriptStackPushU16(void *, uint16_t);
extern void AnimScriptStackPushU24(void *, int32_t);
extern void *func_8001FBA4(void *, uint8_t *);
static uint8_t ram[0x200000];
static struct Fixture {
    uint8_t prefix[0x100], sprite[0x240];
    _Alignas(256) uint8_t fields[0x200];
    uint8_t selector, tail[31];
} fixture, initial, expected;
static const struct Helper {const char *name; uint32_t entry; unsigned width;} helpers[] = {
    {"pop8", 0x80021c20, 1}, {"pop16", 0x80021c3c, 2},
    {"pop24", 0x80021c6c, 3}, {"push8", 0x80021ca0, 1},
    {"push16", 0x80021cc4, 2}, {"push24", 0x80021cf8, 3},
    {"GetArg", 0x8001fba4, 0}
};
static unsigned cases, failures;
static uint32_t pointer(const void *p) { return (uint32_t)(uintptr_t)p; }
static void put32(void *p, uint32_t v) { memcpy(p, &v, 4); }
static uint8_t *address(uint32_t a, unsigned width)
{
    uintptr_t first = (uintptr_t)&fixture;
    if (a >= first && (uint64_t)a + width <= first + sizeof(fixture))
        return (uint8_t *)(uintptr_t)a;
    if (a >= 0x80000000u && (uint64_t)a + width <= 0x80200000u)
        return ram + (a & 0x1fffffu);
    return NULL;
}
static int rd(void *unused, uint32_t a, unsigned width, uint32_t *value)
{
    (void)unused; uint8_t *p = address(a, width);
    if (!p) return -1;
    *value = 0;
    for (unsigned i = 0; i < width; ++i) *value |= (uint32_t)p[i] << (8 * i);
    return 0;
}
static int wr(void *unused, uint32_t a, unsigned width, uint32_t value)
{
    (void)unused; uint8_t *p = address(a, width);
    /* Instruction RAM is read-only; the complete writable map is fixture. */
    if (!p || a >= 0x80000000u) return -1;
    for (unsigned i = 0; i < width; ++i) p[i] = (uint8_t)(value >> (8 * i));
    return 0;
}
static void prepare(unsigned index, unsigned seed)
{
    for (unsigned i = 0; i < sizeof(fixture); ++i)
        ((uint8_t *)&fixture)[i] = (uint8_t)(i * 37u + (i >> 4) + seed * 73u);
    put32(fixture.sprite + 0x88, pointer(fixture.fields));
    /* Keep the pre-fix host-width field pointer valid but distinct, so RED
     * exposes wrong return addresses without relying on an invalid pointer. */
    uintptr_t wrong_field = (uintptr_t)(fixture.fields + 0x80);
    memcpy(fixture.sprite + 0xc0, &wrong_field, sizeof(wrong_field));
    fixture.sprite[0x8c] = (uint8_t)index;
    fixture.sprite[0xc8] = (uint8_t)(index ^ 0x5a);
}
static uint64_t native(unsigned op, uint32_t value, uint8_t *selector)
{
    void *sprite = fixture.sprite;
    switch (op) {
    case 0: return AnimScriptStackPopU8(sprite);
    case 1: return (uint32_t)AnimScriptStackPopU16(sprite);
    case 2: return (uint32_t)AnimScriptStackPopU24(sprite);
    case 3: AnimScriptStackPushU8(sprite, (uint8_t)value); return 0;
    case 4: AnimScriptStackPushU16(sprite, (uint16_t)value); return 0;
    case 5: AnimScriptStackPushU24(sprite, (int32_t)value); return 0;
    case 6: return (uintptr_t)func_8001FBA4(sprite, selector);
    default: assert(0); return 0;
    }
}
static int compare(unsigned op, uint32_t value, uint8_t *selector, const char *name)
{
    unsigned index = fixture.sprite[0x8c];
    initial = fixture;
    PcPortMipsBus bus = {.read = rd, .write = wr};
    PcPortMipsCpu cpu;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = pointer(fixture.sprite);
    cpu.gpr[5] = op == 6 ? pointer(selector) : value;
    cpu.gpr[29] = 0x801ff000;
    cpu.gpr[31] = 0xfffffffcu;
    if (PcPortMipsRun(&cpu, helpers[op].entry, 0xfffffffcu, 100) != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "SPRITE STACK FAIL oracle %s %s\n", helpers[op].name, cpu.error);
        assert(0);
    }
    expected = fixture;
    fixture = initial;
    uint64_t result = native(op, value, selector);
    int bad = 0;
    if ((op < 3 || op == 6) && result != cpu.gpr[2]) {
        if (failures < 12)
            fprintf(stderr, "SPRITE STACK FAIL %s %s case=%u index=%02x value=%08x return native=%08" PRIx64 " retail=%08x\n",
                    name, helpers[op].name, cases, index, value, result, cpu.gpr[2]);
        bad = 1;
    }
    if (memcmp(&fixture, &expected, sizeof(fixture))) {
        if (failures < 12) {
            for (unsigned i = 0; i < sizeof(fixture); ++i) {
                unsigned a = ((uint8_t *)&fixture)[i], b = ((uint8_t *)&expected)[i];
                if (a == b) continue;
                fprintf(stderr, "SPRITE STACK FAIL %s %s case=%u index=%02x value=%08x sprite-offset=%+d native=%02x retail=%02x\n",
                        name, helpers[op].name, cases, index, value, (int)i - (int)offsetof(struct Fixture, sprite), a, b);
                break;
            }
        }
        bad = 1;
    }
    failures += bad;
    ++cases;
    return !bad;
}
static unsigned byte_census(void)
{
    unsigned coupled = 0;
    for (unsigned op = 0; op < 6; ++op) {
        for (unsigned index = 0; index < 256; ++index) {
            for (unsigned channel = 0; channel < helpers[op].width; ++channel) {
                for (unsigned byte = 0; byte < 256; ++byte) {
                    prepare(index, op * 3 + channel);
                    uint32_t value = (0x967ac351u & ~(0xffu << (8 * channel))) |
                                     (byte << (8 * channel));
                    if (op < 3) {
                        uint8_t *target = fixture.sprite + 0x8e + (int8_t)index + channel;
                        /* The index byte cannot independently hold a different
                         * payload byte. Visit each realizable coupled case. */
                        if (target == fixture.sprite + 0x8c) {
                            if (byte != index) continue;
                            ++coupled;
                        }
                        *target = (uint8_t)byte;
                    }
                    assert(fixture.sprite[0x8c] == index);
                    compare(op, value, NULL, "all indices / independent value byte");
                }
            }
        }
    }
    assert(coupled == 6);
    return coupled;
}
static void argument_census(void)
{
    assert((pointer(fixture.fields) & 0xff) == 0);
    for (unsigned index = 0; index < 256; ++index) {
        for (unsigned selector = 0; selector < 256; ++selector) {
            prepare(index, 0);
            fixture.selector = (uint8_t)selector;
            compare(6, 0, &fixture.selector, "GetArg all index / selector modes");

            prepare(index, 1);
            put32(fixture.sprite + 0x88, pointer(fixture.fields + selector));
            assert(fixture.sprite[0x88] == selector);
            compare(6, 0, fixture.sprite + 0x88, "GetArg selector aliases packed field pointer");
        }
        prepare(index, 2);
        compare(6, 0, fixture.sprite + 0x8c, "GetArg selector aliases stack index");
    }
}
static void mixed_sequences(void)
{
    static const unsigned sequences[][6] = {
        {5, 2}, {3, 4, 2}, {5, 0, 1}, {4, 5, 2, 1},
        {2, 5}, {3, 3, 3, 0, 0, 0}
    };
    static const unsigned lengths[] = {2, 3, 3, 4, 2, 6};
    static const uint32_t values[] = {0x0072c285, 0x00ff8001, 0x800000ff, 0xff007fff};
    for (unsigned index = 0; index < 256; ++index) {
        for (unsigned sequence = 0; sequence < 6; ++sequence) {
            for (unsigned seed = 0; seed < 4; ++seed) {
                prepare(index, seed);
                for (unsigned step = 0; step < lengths[sequence]; ++step)
                    compare(sequences[sequence][step], values[seed] ^ (step * 0x12345u),
                            NULL, "mixed push/pop sequence");
            }
        }
    }
}
int main(void)
{
    assert((uintptr_t)&fixture + sizeof(fixture) <= UINT32_MAX);
    FILE *disc = fopen("disc/SLUS_006.64", "rb");
    assert(disc && !fseek(disc, 0x800, SEEK_SET));
    size_t n = fread(ram + 0x10000, 1, sizeof(ram) - 0x10000, disc);
    assert(n >= 0x11d3c && !fclose(disc));
    for (unsigned op = 0; op < 7; ++op) {
        prepare(8, op);
        fixture.selector = 3;
        compare(op, 0x72c285, &fixture.selector, "representative");
    }
    prepare(8, 7); fixture.selector = 0x83;
    compare(6, 0, &fixture.selector, "field argument pointer");
    prepare(8, 8);
    fixture.sprite[0xc8] = 8;
    fixture.sprite[0x96] = fixture.sprite[0xd2] = 0x85;
    fixture.sprite[0x97] = fixture.sprite[0xd3] = 0xc2;
    compare(1, 0, NULL, "PopU16 full-register sign extension, mirrored old/new fields");
    if (failures) {
        fprintf(stderr, "SPRITE STACK FAIL %u/%u representative cases\n", failures, cases);
        return 1;
    }
    unsigned start = cases;
    unsigned coupled = byte_census();
    unsigned byte_cases = cases - start;
    start = cases;
    argument_census();
    unsigned arg_cases = cases - start;
    start = cases;
    mixed_sequences();
    unsigned sequence_steps = cases - start;
    if (failures) {
        fprintf(stderr, "SPRITE STACK FAIL %u/%u complete-memory/return cases\n", failures, cases);
        return 1;
    }
    assert(byte_cases == 784902 && arg_cases == 131328 && sequence_steps == 20480);
    printf("SPRITE STACK PASS %u complete-memory/return cases: %u byte-census cases "
           "(%u independent, %u index-coupled), %u argument cases, %u mixed-sequence steps, 9 representatives\n",
           cases, byte_cases, byte_cases - coupled, coupled, arg_cases, sequence_steps);
    puts("SPRITE STACK scope: all 256 index bytes; each payload byte varied independently, "
         "fixed companion bytes; both argument modes and selected aliases. "
         "Not the Cartesian 24-bit value space or visible-runtime proof.");
    return 0;
}
