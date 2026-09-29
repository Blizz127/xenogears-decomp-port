/* Execute the supplied field initializer and its UV/GPU dependencies, then
 * compare the whole particle and guards with the production native owners.
 * No renderer, replacement assets, or scene state is involved in this test. */
#include "common.h"
#include "field/particles.h"
#include "battle_mips_adapter.h"
#include "psx_memory.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

u8 g_PsxRam[PSX_RAM_SIZE];
extern void FieldInitializeParticlePrimitive(void *, s32, s32);

/* Model the current generated zero-data fallback only when the production
 * data TU has no strong owner. The data gate must reject these placeholders. */
#define MISSING_TABLE(name, count) u16 name[count] __attribute__((weak, aligned(8)))
MISSING_TABLE(D_800AF27C, 16); MISSING_TABLE(D_800AF27E, 16);
MISSING_TABLE(D_800AF280, 16); MISSING_TABLE(D_800AF282, 16);
MISSING_TABLE(D_800AF284, 16); MISSING_TABLE(D_800AF286, 16);
MISSING_TABLE(D_800AF288, 16); MISSING_TABLE(D_800AF28A, 16);
MISSING_TABLE(D_800AF28C, 16); MISSING_TABLE(D_800AF28E, 16);
MISSING_TABLE(D_800AF290, 16); MISSING_TABLE(D_800AF292, 482);
u8 D_800AF474[32] __attribute__((weak, aligned(8)));
#undef MISSING_TABLE

#define SHAPES 0x800af27cu
#define OUTPUT 0x80180020u
#define STACK 0x801ff000u
#define HALT 0xfffffffcu

static u8 *address_bytes(u32 address, unsigned width)
{
    if (address >= 0x80000000u && (uint64_t)address + width <= 0x80200000u)
        return PSX_ADDR(address);
    return NULL;
}
static int read_bus(void *context, u32 address, unsigned width, u32 *value)
{
    (void)context;
    const u8 *p = address_bytes(address, width);
    if (!p) return -1;
    *value = 0;
    for (unsigned i = 0; i < width; i++) *value |= (u32)p[i] << (i * 8);
    return 0;
}
static int write_bus(void *context, u32 address, unsigned width, u32 value)
{
    (void)context;
    u8 *p = address_bytes(address, width);
    if (!p) return -1;
    if (!((address >= OUTPUT && address + width <= OUTPUT + 0xc0) ||
          (address >= STACK - 0x100 && address + width <= STACK))) return -1;
    for (unsigned i = 0; i < width; i++) p[i] = value >> (i * 8);
    return 0;
}
static void load_slice(const char *path, u32 base, u32 address, size_t size)
{
    FILE *file = fopen(path, "rb");
    assert(file && fseek(file, address - base, SEEK_SET) == 0);
    assert(fread(PSX_ADDR(address), 1, size, file) == size);
    assert(fclose(file) == 0);
}

static int check_data(void)
{
    const u16 *labels[] = {D_800AF27C, D_800AF27E, D_800AF280, D_800AF282,
        D_800AF284, D_800AF286, D_800AF288, D_800AF28A, D_800AF28C,
        D_800AF28E, D_800AF290, D_800AF292};
    for (unsigned i = 0; i < 12; i++) {
        if ((uintptr_t)labels[i] != (uintptr_t)labels[0] + i * 2) {
            fprintf(stderr, "PARTICLE PRIMITIVE missing contiguous data: label %u\n", i);
            return 0;
        }
    }
    if (memcmp(labels[0], PSX_ADDR(SHAPES), 21 * 24) != 0 ||
        memcmp(D_800AF474, PSX_ADDR(0x800af474u), 8) != 0) {
        fprintf(stderr, "PARTICLE PRIMITIVE production data differs from retail\n");
        return 0;
    }
    return 1;
}

static int compare_initializer(unsigned shape, unsigned abr, unsigned fill)
{
    u8 initial[0x100], expected[0x100];
    for (unsigned i = 0; i < sizeof(initial); i++) initial[i] = fill + i * 37;
    memcpy(PSX_ADDR(OUTPUT - 0x20), initial, sizeof(initial));
    memset(PSX_ADDR(STACK - 0x120), 0xa5, 0x140);
    PcPortMipsCpu cpu;
    PcPortMipsBus bus = {.read = read_bus, .write = write_bus};
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = OUTPUT; cpu.gpr[5] = shape; cpu.gpr[6] = abr;
    cpu.gpr[29] = STACK; cpu.gpr[31] = HALT;
    for (unsigned i = 16; i <= 23; i++) cpu.gpr[i] = 0xa5010000u + i;
    if (PcPortMipsRun(&cpu, 0x800a8eacu, HALT, 1000) != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "PARTICLE PRIMITIVE retail failed: %s\n", cpu.error);
        return 0;
    }
    assert(cpu.gpr[29] == STACK);
    for (unsigned i = 16; i <= 23; i++) assert(cpu.gpr[i] == 0xa5010000u + i);
    memcpy(expected, PSX_ADDR(OUTPUT - 0x20), sizeof(expected));

    memcpy(PSX_ADDR(OUTPUT - 0x20), initial, sizeof(initial));
    FieldInitializeParticlePrimitive(PSX_ADDR(OUTPUT), shape, abr);
    const u8 *actual = PSX_ADDR(OUTPUT - 0x20);
    if (memcmp(actual, expected, sizeof(expected)) != 0) {
        fprintf(stderr, "PARTICLE PRIMITIVE shape=%u abr=%u fill=%u: polygons=%s vertices=%s\n",
            shape, abr, fill,
            memcmp(actual + 0x70, expected + 0x70, 0x50) ? "DIFFER" : "equal",
            memcmp(actual + 0xc0, expected + 0xc0, 0x20) ? "DIFFER" : "equal");
        for (unsigned i = 0; i < sizeof(expected); i++) {
            if (actual[i] != expected[i]) {
                fprintf(stderr, "  particle %+d native=%02x retail=%02x\n",
                        (int)i - 0x20, actual[i], expected[i]);
                break;
            }
        }
        return 0;
    }
    return 1;
}

int main(void)
{
    _Static_assert(sizeof(ParticlePrimitive) == 0xc0, "particle size");
    _Static_assert(sizeof(POLY_FT4) == 0x28, "packet size");
    load_slice("disc/field.bin", 0x8006faf0u, 0x800a8eacu, 0x208);
    load_slice("disc/field.bin", 0x8006faf0u, 0x8007a44cu, 0x178);
    load_slice("disc/field.bin", 0x8006faf0u, SHAPES, 0x200);
    load_slice("disc/SLUS_006.64", 0x8000f800u, 0x80043a1cu, 0x2a8);
    if (!check_data()) return 1;
    unsigned cases = 0;
    for (unsigned fill = 0; fill < 256; fill++) {
        for (unsigned shape = 0; shape < 21; shape++) {
            for (unsigned abr = 0; abr < 4; abr++) {
                if (!compare_initializer(shape, abr, fill)) return 1;
                cases++;
            }
        }
    }
    assert(check_data());
    printf("FIELD PARTICLE PRIMITIVE RETAIL PASS cases=%u shapes=21 abr=4 fills=256 data=512 bytes\n", cases);
    return 0;
}
