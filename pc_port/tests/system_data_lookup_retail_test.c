/* Compare native system-data readers with the supplied retail instructions.
 * Synthetic tables exercise the ABI; they are not replacement game content. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "battle_mips_adapter.h"
#include "psx_memory.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t D_8005A0E4[1024];
extern void *g_SystemDataEntries;
extern void *GetStringEntry(void *, int32_t);
extern void *func_80033784(int32_t, int32_t);
extern void *func_800339C8(int32_t, int32_t);
extern void func_80033ABC(uint16_t *);
extern void func_80033B34(uint16_t *, uint8_t *, int32_t);
extern int16_t func_80033BAC(uint8_t, uint8_t);
#define LOOKUP(name) extern void *name(int32_t)
LOOKUP(func_800337B8); LOOKUP(GetAccessoryName); LOOKUP(GetItemName);
LOOKUP(GetWeaponName); LOOKUP(func_80033878); LOOKUP(func_800338A8);
LOOKUP(func_800338D8); LOOKUP(func_80033908); LOOKUP(func_80033938);
LOOKUP(func_80033968); LOOKUP(func_80033998); LOOKUP(func_800339FC);
LOOKUP(func_80033A2C); LOOKUP(func_80033A5C); LOOKUP(func_80033A8C);

#define TABLE 0x80100000u
#define BUNDLE 0x80110000u
#define INPUT 0x80140000u
#define OUTPUT 0x80141000u
#define STACK 0x801ff000u
#define HALT 0xfffffffcu

static uint8_t *address_bytes(uint32_t address, unsigned width)
{
    uintptr_t base = (uintptr_t)g_PsxRam;
    if (address >= base && (uint64_t)address + width <= base + sizeof(g_PsxRam))
        return (uint8_t *)(uintptr_t)address;
    if (address >= 0x80000000u && (uint64_t)address + width <= 0x80200000u)
        return PSX_ADDR(address);
    return NULL;
}

static int bus_read(void *opaque, uint32_t address, unsigned width, uint32_t *value)
{
    (void)opaque;
    const uint8_t *p = address_bytes(address, width);
    if (!p) return -1;
    *value = 0;
    for (unsigned i = 0; i < width; i++) *value |= (uint32_t)p[i] << (8 * i);
    return 0;
}

static int bus_write(void *opaque, uint32_t address, unsigned width, uint32_t value)
{
    (void)opaque;
    uint8_t *p = address_bytes(address, width);
    if (!p) return -1;
    for (unsigned i = 0; i < width; i++) p[i] = value >> (8 * i);
    return 0;
}

static void word(uint32_t address, uint32_t value)
{ assert(bus_write(NULL, address, 4, value) == 0); }

static uint32_t retail(uint32_t entry, uint32_t a, uint32_t b, uint32_t c)
{
    PcPortMipsCpu cpu;
    PcPortMipsBus bus = {.read = bus_read, .write = bus_write};
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = a; cpu.gpr[5] = b; cpu.gpr[6] = c;
    cpu.gpr[29] = STACK; cpu.gpr[31] = HALT;
    if (PcPortMipsRun(&cpu, entry, HALT, 2000000) != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "SYSTEM LOOKUP retail %08x failed: %s\n", entry, cpu.error);
        assert(0);
    }
    return cpu.gpr[2];
}

static void equal_pointer(void *actual, uint32_t expected)
{
    if ((uintptr_t)actual != expected) {
        fprintf(stderr, "SYSTEM LOOKUP pointer mismatch native=%p retail=%08x\n", actual, expected);
        assert(0);
    }
}

int main(void)
{
    FILE *image = fopen("disc/SLUS_006.64", "rb");
    assert(image && (uintptr_t)g_PsxRam + sizeof(g_PsxRam) < UINT32_MAX);
    assert(fseek(image, 0x80033728u - 0x8000f800u, SEEK_SET) == 0);
    assert(fread(PSX_ADDR(0x80033728u), 1, 0x5a8, image) == 0x5a8);
    fclose(image);
    g_SystemDataEntries = PSX_ADDR(TABLE);
    word(0x80059360u, (uintptr_t)g_SystemDataEntries);
    for (unsigned i = 0; i < 256; i++)
        word(TABLE + i * 4, (uintptr_t)PSX_ADDR(BUNDLE + i * 0x400));
    for (unsigned i = 0; i < 0x20000 / 2; i++)
        ((uint16_t *)PSX_ADDR(BUNDLE))[i] = (uint16_t)(i * 4051u);

    static const struct { uint32_t pc; void *(*native)(int32_t); } lookups[] = {
        {0x800338d8u, func_800338D8}, /* Exact visible-run-06 crash first. */
        {0x800337b8u, func_800337B8}, {0x800337e8u, GetAccessoryName},
        {0x80033818u, GetItemName}, {0x80033848u, GetWeaponName},
        {0x80033878u, func_80033878}, {0x800338a8u, func_800338A8},
        {0x80033908u, func_80033908}, {0x80033938u, func_80033938},
        {0x80033968u, func_80033968}, {0x80033998u, func_80033998},
        {0x800339fcu, func_800339FC}, {0x80033a2cu, func_80033A2C},
        {0x80033a5cu, func_80033A5C}, {0x80033a8cu, func_80033A8C},
    };
    const int32_t indices[] = {-2, -1, 0, 1, 2, 7, 31, 255};
    uint8_t tables_before[0x400], bundles_before[0x20000];
    memcpy(tables_before, PSX_ADDR(TABLE), sizeof(tables_before));
    memcpy(bundles_before, PSX_ADDR(BUNDLE), sizeof(bundles_before));
    unsigned cases = 0;
    for (unsigned l = 0; l < sizeof(lookups) / sizeof(*lookups); l++) {
        fprintf(stderr, "checking system lookup %08x\n", lookups[l].pc);
        for (unsigned i = 0; i < sizeof(indices) / sizeof(*indices); i++, cases++)
            equal_pointer(lookups[l].native(indices[i]), retail(lookups[l].pc, indices[i], 0, 0));
    }
    for (unsigned table = 0; table < 54; table++) {
        for (unsigned i = 0; i < sizeof(indices) / sizeof(*indices); i++, cases += 3) {
            equal_pointer(func_80033784(table, indices[i]), retail(0x80033784u, table, indices[i], 0));
            equal_pointer(func_800339C8(table, indices[i]), retail(0x800339c8u, table, indices[i], 0));
            void *bundle = PSX_ADDR(BUNDLE + table * 0x400);
            equal_pointer(GetStringEntry(bundle, indices[i]), retail(0x80033728u, (uintptr_t)bundle, indices[i], 0));
        }
    }
    assert(memcmp(tables_before, PSX_ADDR(TABLE), sizeof(tables_before)) == 0);
    assert(memcmp(bundles_before, PSX_ADDR(BUNDLE), sizeof(bundles_before)) == 0);

    /* All 324 glyphs, including single-byte and double-byte entries. */
    uint8_t *glyphs = PSX_ADDR(BUNDLE);
    word(TABLE + 0x6c, (uintptr_t)glyphs);
    for (unsigned i = 0; i < 0x144; i++) {
        glyphs[i * 2] = i >> 8;
        glyphs[i * 2 + 1] = i;
    }
    for (unsigned a = 0; a < 3; a++)
        for (unsigned b = 0; b < 256; b++, cases++)
            assert((uint16_t)func_80033BAC(a, b) == (uint16_t)retail(0x80033bacu, a, b, 0));

    uint16_t *input = PSX_ADDR(INPUT);
    const unsigned lengths[] = {0, 1, 2, 7, 324};
    for (unsigned l = 0; l < sizeof(lengths) / sizeof(*lengths); l++) {
        unsigned count = lengths[l];
        for (unsigned i = 0; i < count; i++) input[i] = (i * 137) % 324;
        input[count] = 0xffff;
        memset(PSX_ADDR(0x8005a0e4u), 0xa5, 1024);
        memset(D_8005A0E4, 0xa5, sizeof(D_8005A0E4));
        retail(0x80033abcu, INPUT, 0, 0);
        func_80033ABC(input);
        assert(memcmp(D_8005A0E4, PSX_ADDR(0x8005a0e4u), 1024) == 0);
        memset(PSX_ADDR(OUTPUT), 0xa5, 1024);
        memset(D_8005A0E4, 0xa5, sizeof(D_8005A0E4));
        retail(0x80033b34u, INPUT, OUTPUT, count);
        func_80033B34(input, D_8005A0E4, count);
        assert(memcmp(D_8005A0E4, PSX_ADDR(OUTPUT), 1024) == 0);
        cases += 2;
    }
    printf("SYSTEM DATA LOOKUP RETAIL PASS cases=%u, pointers/read-only tables/glyphs/guards\n", cases);
    return 0;
}
