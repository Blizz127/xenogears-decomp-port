#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "battle_mips_adapter.h"
#include "psx_memory.h"
uint8_t g_PsxRam[PSX_RAM_SIZE];

extern void func_800B2AEC(void *, void *, void *, int32_t, int32_t, int32_t);
extern int32_t func_80021AD8(int32_t, int32_t);

#define BATTLE_BASE 0x8006faf0u
#define BATTLE_SIZE 0x53f80u
#define STACK_BASE 0x801ff000u

static uint8_t battle_ram[BATTLE_SIZE];
static uint8_t clamp_code[0x2c];
static uint8_t stack_ram[0x2000];

typedef struct {
    uint8_t guard0[0x40];
    uint8_t header[0x80];
    uint8_t descriptor[0x1000];
    uint8_t guard1[0x40];
    uint8_t arena0[0x4000];
    uint8_t guard2[0x40];
    uint8_t arena1[0x4000];
    uint8_t guard3[0x40];
} Fixture;

static Fixture fixture, initial, expected;
static unsigned clamp_calls;
static unsigned cases;

static uint8_t *address(uint32_t value, unsigned width)
{
    uintptr_t first = (uintptr_t)&fixture;
    if (value >= first && (uint64_t)value + width <= first + sizeof(fixture))
        return (uint8_t *)(uintptr_t)value;
    if (value >= 0x80021ad8u && (uint64_t)value + width <= 0x80021b04u)
        return clamp_code + (value - 0x80021ad8u);
    if (value >= BATTLE_BASE &&
        (uint64_t)value + width <= BATTLE_BASE + sizeof(battle_ram))
        return battle_ram + (value - BATTLE_BASE);
    if (value >= STACK_BASE &&
        (uint64_t)value + width <= STACK_BASE + sizeof(stack_ram))
        return stack_ram + (value - STACK_BASE);
    uintptr_t native_ram = (uintptr_t)g_PsxRam;
    if (value >= native_ram && (uint64_t)value + width <= native_ram + PSX_RAM_SIZE)
        return (uint8_t *)(uintptr_t)value;
    if (((value & 0xFFE00000u) == 0x80000000u ||
         (value & 0xFFE00000u) == 0xA0000000u) &&
        (value & 0x1FFFFFu) + width <= 0x200000u)
        return (uint8_t *)PSX_ADDR(value);
    return NULL;
}

static int read_memory(void *unused, uint32_t value, unsigned width,
                       uint32_t *result)
{
    (void)unused;
    uint8_t *p = address(value, width);
    if (p == NULL)
        return -1;
    *result = 0;
    for (unsigned i = 0; i < width; ++i)
        *result |= (uint32_t)p[i] << (i * 8);
    return 0;
}

static int write_memory(void *unused, uint32_t value, unsigned width,
                        uint32_t data)
{
    (void)unused;
    uint8_t *p = address(value, width);
    if (p == NULL)
        return -1;
    for (unsigned i = 0; i < width; ++i)
        p[i] = (uint8_t)(data >> (i * 8));
    return 0;
}

static int bridge(void *unused, PcPortMipsCpu *cpu, uint32_t target)
{
    (void)unused;
    if (target != 0x80021ad8u)
        return 0;
    (void)cpu;
    ++clamp_calls;
    return 0; /* Observe entry, then execute the actual SLUS clamp bytes. */
}

static void put32(uint8_t *p, uint32_t value) { memcpy(p, &value, 4); }

static unsigned clamp_count_for_key(unsigned key)
{
    switch (key) {
    case 0x100: case 0x108: case 0x110: case 0x118:
        return 0;
    case 0x000: case 0x008: case 0x004: case 0x00c:
    case 0x104: case 0x10c:
        return 3;
    case 0x010: case 0x014: case 0x114:
        return 9;
    case 0x018: case 0x01c: case 0x11c:
        return 12;
    default:
        return 0;
    }
}

static void load_battle(void)
{
    FILE *file = fopen("disc/battle.bin", "rb");
    assert(file != NULL);
    assert(fread(battle_ram, 1, sizeof(battle_ram), file) == sizeof(battle_ram));
    assert(!fclose(file));
    file = fopen("disc/SLUS_006.64", "rb");
    assert(file && !fseek(file, 0x80021ad8u - 0x8000f800u, SEEK_SET));
    assert(fread(clamp_code, 1, sizeof(clamp_code), file) == sizeof(clamp_code));
    assert(!fclose(file));
}

static void run_retail(int32_t red, int32_t green, int32_t blue,
                       void *header, void *buffer0, void *buffer1)
{
    PcPortMipsBus bus = {.read = read_memory, .write = write_memory,
                         .bridge = bridge};
    PcPortMipsCpu cpu;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = (uint32_t)(uintptr_t)header;
    cpu.gpr[5] = (uint32_t)(uintptr_t)buffer0;
    cpu.gpr[6] = (uint32_t)(uintptr_t)buffer1;
    cpu.gpr[7] = (uint32_t)red;
    cpu.gpr[29] = STACK_BASE + 0x1000u;
    cpu.gpr[31] = 0xfffffffcu;
    /* B2AEC's prologue subtracts 0x38 before loading its stack arguments
     * at 0x48/0x4c, so these ABI arguments are entry-SP + 0x10/0x14. */
    put32(address(cpu.gpr[29] + 0x10, 4), (uint32_t)green);
    put32(address(cpu.gpr[29] + 0x14, 4), (uint32_t)blue);
    int result = PcPortMipsRun(&cpu, 0x800b2aecu, 0xfffffffcu, 200000);
    if (result != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "B2AEC FAIL retail pc=%08x: %s\n", cpu.pc, cpu.error);
        exit(2);
    }
}

static void init_descriptor(uint8_t *d, unsigned key, unsigned source_stride,
                            unsigned output_stride, unsigned variant)
{
    memset(d, (int)(0x31u + variant * 17u), source_stride * 4u);
    d[0] = (uint8_t)output_stride;
    d[1] = (uint8_t)source_stride;
    /* Only d[2].bit0 participates in the key.  Exercise all other bits. */
    d[2] = (uint8_t)(((key & 0x100u) ? 0u : 1u) |
                     ((variant * 0x53u) & 0xfeu));
    d[3] = (uint8_t)(key & 0x1cu);
    /* d[3].bits[4:2] are the shape; all other bits are irrelevant. */
    d[3] |= (uint8_t)((variant * 0x97u) & (uint8_t)~0x1cu);
    for (unsigned i = 4; i < source_stride * 4u; ++i)
        d[i] = (uint8_t)(i * 29u + variant * 11u);
}

static void compare(unsigned key, unsigned count, unsigned source_stride,
                    unsigned output_stride, unsigned overlap, unsigned variant,
                    int32_t red, int32_t green, int32_t blue,
                    unsigned color_sample)
{
    memset(&fixture, (int)(0xa5u ^ variant), sizeof(fixture));
    uint8_t *header = fixture.header;
    uint8_t *descriptor = fixture.descriptor;
    put32(header + 0x10, (uint32_t)(descriptor - header));
    put32(header + 0x14, count);
    for (unsigned i = 0; i < count; ++i) {
        unsigned k = (i == 0) ? key : ((key + i * 4u) & 0x11cu);
        uint8_t *d = descriptor + i * (source_stride + 1u) * 4u;
        init_descriptor(d, k, source_stride, output_stride, variant + i);
    }
    uint8_t *buffer0 = fixture.arena0 + 0x100;
    uint8_t *buffer1 = fixture.arena1 + 0x180;
    if (overlap == 1 || overlap == 2 || overlap == 3)
        buffer1 = fixture.arena0 + 0x100 + overlap * 7u;
    else if (overlap == 4)
        buffer0 = descriptor - 4;
    else if (overlap == 5)
        buffer1 = descriptor;
    else if (overlap == 6) {
        buffer0 = descriptor - 4;
        buffer1 = descriptor;
    }
    for (unsigned i = 0; i < sizeof(fixture.arena0); ++i)
        fixture.arena0[i] = (uint8_t)(i * 13u + variant);
    for (unsigned i = 0; i < sizeof(fixture.arena1); ++i)
        fixture.arena1[i] = (uint8_t)(i * 19u + variant * 3u);
    /* Reinstall metadata after arena patterns and give every color byte a
     * finite edge value while retaining descriptor control bytes. */
    put32(header + 0x10, (uint32_t)(descriptor - header));
    put32(header + 0x14, count);
    for (unsigned i = 0; i < count; ++i) {
        unsigned k = (i == 0) ? key : ((key + i * 4u) & 0x11cu);
        uint8_t *d = descriptor + i * (source_stride + 1u) * 4u;
        init_descriptor(d, k, source_stride, output_stride, variant + i);
        for (unsigned j = 4; j < source_stride * 4u; ++j)
            d[j] = (uint8_t)(color_sample + j * 17u + i * 3u);
    }
    initial = fixture;
    clamp_calls = 0;
    run_retail(red, green, blue, header, buffer0, buffer1);
    if (count == 1 && clamp_calls != clamp_count_for_key(key)) {
        fprintf(stderr, "B2AEC FAIL clamp count case=%u key=%03x got=%u expected=%u\n",
                cases, key, clamp_calls, clamp_count_for_key(key));
        exit(1);
    }
    expected = fixture;
    fixture = initial;
    func_800B2AEC(header, buffer0, buffer1, red, green, blue);
    if (memcmp(&fixture, &expected, sizeof(fixture)) != 0) {
        unsigned diff = 0;
        while (diff < sizeof(fixture) &&
               ((uint8_t *)&fixture)[diff] == ((uint8_t *)&expected)[diff])
            ++diff;
        fprintf(stderr, "B2AEC FAIL case=%u key=%03x count=%u strides=%u/%u "
                "overlap=%u deltas=%d/%d/%d color=%u diff=%u\n", cases,
                key, count, source_stride, output_stride, overlap, red, green,
                blue, color_sample, diff);
        for (unsigned i = diff > 8 ? diff - 8 : 0; i < diff + 8; ++i)
            fprintf(stderr, " %u:%02x/%02x", i,
                    ((uint8_t *)&fixture)[i], ((uint8_t *)&expected)[i]);
        fputc('\n', stderr);
        exit(1);
    }
    ++cases;
}

/* The Aquasol F2 path keeps a native model pointer but serializes both
 * packet buffers as KSEG addresses. Cover each independent argument domain,
 * including cached/uncached guest aliases, against the same retail routine. */
static void compare_pointer_domains(void)
{
    Fixture *backing = (Fixture *)PSX_ADDR(0x80100000u);
    const unsigned keys[] = {0, 8, 0x10, 0x18, 4, 0xc, 0x14, 0x1c,
                            0x104, 0x10c, 0x114, 0x11c};
    for (unsigned alias = 0; alias != 2; ++alias)
        for (unsigned domains = 0; domains != 8; ++domains)
            for (unsigned k = 0; k != sizeof(keys)/sizeof(keys[0]); ++k) {
                memset(backing, 0xA5, sizeof(*backing));
                put32(backing->header + 0x10,
                      (uint32_t)(backing->descriptor - backing->header));
                put32(backing->header + 0x14, 2);
                init_descriptor(backing->descriptor, keys[k], 9, 11, k);
                init_descriptor(backing->descriptor + 40, keys[k], 9, 11, k + 1);
                void *args[] = {backing->header, backing->arena0, backing->arena1};
                for (unsigned a = 0; a != 3; ++a)
                    if (domains & (1u << a))
                        args[a] = (void *)(uintptr_t)((alias ? 0xA0000000u : 0x80000000u) |
                            (uint32_t)((uint8_t *)args[a] - g_PsxRam));
                initial = *backing;
                run_retail(-7, -7, -7, args[0], args[1], args[2]);
                expected = *backing;
                *backing = initial;
                func_800B2AEC(args[0], args[1], args[2], -7, -7, -7);
                if (memcmp(backing, &expected, sizeof(*backing))) {
                    fprintf(stderr, "B2AEC FAIL case=%u pointer domains=%u alias=%u key=%x\n",
                            cases, domains, alias, keys[k]);
                    exit(1);
                }
                ++cases;
            }
}

int main(void)
{
    assert((uintptr_t)&fixture + sizeof(fixture) <= UINT32_MAX);
    load_battle();
    compare_pointer_domains();

    /* First case is the explicit scratch-empty RED when production is absent. */
    compare(0, 1, 2, 2, 0, 0, 0, 0, 0, 0);
    compare(0, 0, 0, 0, 0, 1, 32767, -32768, 0, 255);
    const unsigned keys[] = {0, 8, 0x10, 0x18, 4, 0xc, 0x14, 0x1c,
                             0x104, 0x10c, 0x114, 0x11c, 0x100, 0x108,
                             0x110, 0x118};
    const int32_t deltas[] = {-32768, -256, -1, 0, 1, 255, 32767};
    for (unsigned k = 0; k < sizeof(keys) / sizeof(keys[0]); ++k)
        for (unsigned sample = 0; sample < 256; ++sample)
            compare(keys[k], 1, 9, 11, sample & 3u, sample,
                    deltas[sample % 7], deltas[(sample + 2) % 7],
                    deltas[(sample + 4) % 7], sample);
    for (unsigned k = 0; k < sizeof(keys) / sizeof(keys[0]); ++k) {
        compare(keys[k], 3, 255, 255, 9, k, -32768, 32767, -1, 255);
        for (unsigned overlap = 0; overlap <= 6; ++overlap)
            compare(keys[k], 3, 9, 11, overlap, k, 1, -1, 7, 127);
    }
    compare(0, 2, 0, 0, 0, 9, -32768, 32767, -1, 0);
    compare(0x10, 2, 1, 1, 4, 10, 255, -256, 1, 255);
    compare(0x114, 2, 2, 1, 5, 11, 1, -1, 32767, 127);
    compare(0x11c, 2, 3, 3, 6, 12, -32768, 0, 32767, 128);
    printf("B2AEC PASS %u cases: all keys, irrelevant key bits, all color bytes, "
           "signed deltas, zero/multiple records, edge strides, overlaps and guards\n", cases);
    puts("B2AEC scope: actual battle.bin leaf and actual SLUS clamp instructions; "
         "finite census, no exhaustive descriptor universe claim");
    return 0;
}
