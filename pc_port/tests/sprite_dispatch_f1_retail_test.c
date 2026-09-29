#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "battle_mips_adapter.h"

extern void func_8001FBE4(void *, uint32_t, void *);
extern void f1_leaf_impl(void *);

static unsigned native_leaf_entries;
static uint32_t native_leaf_arg;
static uint8_t native_leaf_entry[0xc0];

/* This wrapper records the production callback ABI, then executes the exact
 * rendering.c leaf extracted by the runner. It is not a replacement leaf. */
void func_8001F6B0(void *sprite)
{
    ++native_leaf_entries;
    native_leaf_arg = (uint32_t)(uintptr_t)sprite;
    memcpy(native_leaf_entry, sprite, sizeof(native_leaf_entry));
    f1_leaf_impl(sprite);
}

#define GUARD(name) \
    void name(void) { \
        fputs("SPRITE F1 FAIL unexpected callee " #name "\n", stderr); \
        abort(); \
    }
GUARD(func_800B2AEC)
GUARD(func_80039E60)
/* Guest-RAM backing for the opcode 0xB0 selector read; never touched here. */
uint8_t g_PsxRam[0x300000];
GUARD(AnimScriptTick) GUARD(HeapAlloc) GUARD(HeapChangeCurrentUser)
GUARD(HeapFree) GUARD(__wrap_ReadGeomOffset) GUARD(__wrap_ScaleMatrixL)
GUARD(func_8001CE74) GUARD(func_8001D2B0) GUARD(func_8001D4E8) GUARD(func_8001EE68)
GUARD(func_8001FB30) GUARD(func_80022D44) GUARD(func_80022CAC)
GUARD(func_80023B84) GUARD(func_800245D8) GUARD(func_8002C3E8)
GUARD(func_8002C59C) GUARD(func_8002C8CC) GUARD(func_8002CB54)
GUARD(func_8002CC10) GUARD(func_80023124) GUARD(func_80023290)

uint32_t D_80018644, D_800592E4;
int32_t D_80059198;
uint8_t D_800591AD, D_800591B0, D_800591B3;
int16_t D_800592E8, D_800592EA;
uint32_t D_8004FBB8[8], D_8006F99C[4], D_8006F9AC[4], D_800C3EB0[4];
/* Opcode 0xA7 timer (s32) and opcode 0xBD packed block; both unreached here. */
int32_t g_WorkListCurTimer;
uint8_t D_8006BE10[32];
int g_cfg_pgxpTextureCorrection;

static uint8_t ram[0x200000];
static struct {
    uint8_t before[0x20];
    uint8_t sprite[0xc0];
    uint8_t after[0x20];
    uint8_t ops[4];
    uint8_t base[0x80];
    uint8_t prim[0x18 * 64];
} fixture, initial, expected;
static uint8_t retail_leaf_entry[0xc0];
static unsigned cases;
static unsigned retail_handler_entries, retail_leaf_entries;
static uint32_t retail_leaf_arg;

typedef struct {
    uint32_t address;
    unsigned width;
} TraceEntry;
static TraceEntry trace[32];
static unsigned trace_count;
static int trace_handler;

static uint8_t *address(uint32_t value, unsigned width)
{
    uintptr_t first = (uintptr_t)&fixture;
    if (value >= first && (uint64_t)value + width <= first + sizeof(fixture))
        return (uint8_t *)(uintptr_t)value;
    if (value >= 0x80000000u &&
        (uint64_t)value + width <= 0x80200000u)
        return ram + (value & 0x1fffffu);
    return NULL;
}

static int read_memory(void *unused, uint32_t value, unsigned width,
                       uint32_t *result)
{
    (void)unused;
    uint8_t *p = address(value, width);
    if (p == NULL)
        return -1;
    retail_handler_entries += value == 0x80020f4cu;
    if (value == 0x80020f4cu) trace_handler = 1;
    if (value == 0x8001f6b0u || value == 0x80021ab8u) trace_handler = 0;
    if (trace_handler && (uintptr_t)p >= (uintptr_t)&fixture &&
        (uintptr_t)p < (uintptr_t)&fixture + sizeof(fixture) &&
        trace_count < 32) {
        trace[trace_count].address = value;
        trace[trace_count].width = width;
        ++trace_count;
    }
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
    if (target != 0x8001f6b0u)
        return 0;
    uint8_t *sprite = address(cpu->gpr[4], sizeof(retail_leaf_entry));
    if (sprite == NULL)
        return -1;
    ++retail_leaf_entries;
    retail_leaf_arg = cpu->gpr[4];
    memcpy(retail_leaf_entry, sprite, sizeof(retail_leaf_entry));
    trace_handler = 0;
    return 0; /* Observe entry, then execute the actual retail leaf. */
}

static void put32(uint8_t *p, uint32_t value)
{
    memcpy(p, &value, sizeof(value));
}

static uint32_t get32(const uint8_t *p)
{
    uint32_t value;
    memcpy(&value, p, sizeof(value));
    return value;
}

static void put16(uint8_t *p, uint16_t value)
{
    memcpy(p, &value, sizeof(value));
}

static void run_retail(uint8_t *sprite, uint8_t *operands)
{
    PcPortMipsBus bus = {
        .read = read_memory,
        .write = write_memory,
        .bridge = bridge,
    };
    PcPortMipsCpu cpu;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = (uint32_t)(uintptr_t)sprite;
    cpu.gpr[5] = 0xf1u;
    cpu.gpr[6] = (uint32_t)(uintptr_t)operands;
    cpu.gpr[29] = 0x801fff00u;
    cpu.gpr[31] = 0xfffffffcu;
    int result = PcPortMipsRun(&cpu, 0x8001fbe4u, 0xfffffffcu, 2000);
    if (result != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "SPRITE F1 FAIL retail pc=%08x: %s\n",
                cpu.pc, cpu.error);
        exit(2);
    }
}

static void expect_trace(uint8_t *sprite, uint8_t *operands, unsigned mode)
{
    TraceEntry expected_trace[10];
    unsigned count = 0;
    expected_trace[count++] = (TraceEntry){(uint32_t)(uintptr_t)(operands + 0), 1};
    expected_trace[count++] = (TraceEntry){(uint32_t)(uintptr_t)(sprite + 0x20), 4};
    expected_trace[count++] = (TraceEntry){(uint32_t)(uintptr_t)(operands + 1), 1};
    expected_trace[count++] = (TraceEntry){(uint32_t)(uintptr_t)(sprite + 0x3c), 4};
    expected_trace[count++] = (TraceEntry){(uint32_t)(uintptr_t)(operands + 2), 1};
    if (mode == 2) {
        expected_trace[count++] = (TraceEntry){(uint32_t)(uintptr_t)(operands + 0), 1};
        expected_trace[count++] = (TraceEntry){(uint32_t)(uintptr_t)(operands + 1), 1};
        expected_trace[count++] = (TraceEntry){(uint32_t)(uintptr_t)(operands + 2), 1};
    }
    expected_trace[count++] = (TraceEntry){(uint32_t)(uintptr_t)(sprite + 0x3c), 4};
    if (trace_count != count) {
        fprintf(stderr, "SPRITE F1 FAIL oracle trace count=%u/%u\n",
                trace_count, count);
        for (unsigned i = 0; i < trace_count; ++i)
            fprintf(stderr, " trace[%u]=%08x/%u\n", i,
                    trace[i].address, trace[i].width);
        exit(2);
    }
    for (unsigned i = 0; i < count; ++i) {
        if (trace[i].address != expected_trace[i].address ||
            trace[i].width != expected_trace[i].width) {
            fprintf(stderr, "SPRITE F1 FAIL oracle trace[%u]=%08x/%u expected=%08x/%u\n",
                    i, trace[i].address, trace[i].width,
                    expected_trace[i].address, expected_trace[i].width);
            exit(2);
        }
    }
}

static void compare(unsigned a, unsigned b, unsigned c, unsigned mode,
                    unsigned alias, unsigned variant)
{
    static const unsigned pointer_aliases[] = {10, 11, 12, 13, 14, 15, 16, 17, 24, 25, 26, 27};
    uint8_t *aliases[29];
    unsigned pointer_alias = 0;

    for (unsigned i = 0; i < sizeof(fixture); ++i)
        ((uint8_t *)&fixture)[i] = (uint8_t)(i * 37u + variant * 73u + a + b + c);
    uint8_t *sprite = fixture.sprite;
    uint8_t *base = (variant & 0x100u) ? sprite : fixture.base;
    put32(sprite + 0x20, (uint32_t)(uintptr_t)base);
    put32(sprite + 0x24, (uint32_t)(uintptr_t)fixture.base);
    put32(base + 0x30, (uint32_t)(uintptr_t)fixture.prim);
    put32(sprite + 0x28, 0xA1B2C300u | ((variant * 17u) & 0xffu));
    put32(sprite + 0x3c, (mode & 3u) | 0x800000a0u);
    sprite[0x40] = (uint8_t)((variant & 7u) << 2);
    put16(base + 0x38, 0x1234u + variant);
    put16(base + 0x3a, 0x5678u + variant);
    if (base != sprite) put16(base + 0x3c, 0x9abcu + variant);
    for (unsigned i = 0; i < 64; ++i) {
        put16(fixture.prim + i * 0x18 + 0x0a, (uint16_t)(0xF000u + i * 0x43u));
        put32(fixture.prim + i * 0x18 + 0x10, 0xdead0000u + i);
    }

    aliases[0] = fixture.ops;
    aliases[1] = sprite + 0x00; aliases[2] = sprite + 0x01; aliases[3] = sprite + 0x02;
    aliases[4] = sprite + 0x28; aliases[5] = sprite + 0x29; aliases[6] = sprite + 0x2a;
    aliases[7] = sprite + 0x3c; aliases[8] = sprite + 0x3d; aliases[9] = sprite + 0x3e;
    aliases[10] = sprite + 0x20; aliases[11] = sprite + 0x21;
    aliases[12] = sprite + 0x22; aliases[13] = sprite + 0x23;
    aliases[14] = sprite + 0x24; aliases[15] = sprite + 0x25;
    aliases[16] = sprite + 0x26; aliases[17] = sprite + 0x27;
    aliases[18] = base + 0x38; aliases[19] = base + 0x39;
    aliases[20] = base + 0x3a; aliases[21] = base + 0x3b;
    aliases[22] = base + 0x3c; aliases[23] = base + 0x3d;
    aliases[24] = base + 0x30; aliases[25] = base + 0x31;
    aliases[26] = base + 0x32; aliases[27] = base + 0x33;
    aliases[28] = sprite + 0x3f;
    for (unsigned i = 0; i < sizeof(pointer_aliases) / sizeof(pointer_aliases[0]); ++i)
        pointer_alias |= alias == pointer_aliases[i];
    uint8_t *operands = aliases[alias];
    if (!pointer_alias) {
        operands[0] = (uint8_t)a;
        operands[1] = (uint8_t)b;
        operands[2] = (uint8_t)c;
    }
    unsigned actual_mode = get32(sprite + 0x3c) & 3u;
    initial = fixture;
    retail_handler_entries = retail_leaf_entries = 0;
    retail_leaf_arg = 0;
    trace_count = 0;
    trace_handler = 0;
    run_retail(sprite, operands);
    expect_trace(sprite, operands, actual_mode);
    unsigned expect_leaf = (get32(sprite + 0x3c) & 3u) == 1u;
    if (retail_handler_entries == 0 || retail_leaf_entries != expect_leaf ||
        (expect_leaf && retail_leaf_arg != (uint32_t)(uintptr_t)sprite)) {
        fprintf(stderr, "SPRITE F1 FAIL oracle case=%u mode=%u alias=%u entries=%u/%u\n",
                cases, actual_mode, alias, retail_handler_entries, retail_leaf_entries);
        exit(2);
    }
    expected = fixture;

    fixture = initial;
    native_leaf_entries = 0;
    native_leaf_arg = 0;
    memset(native_leaf_entry, 0, sizeof(native_leaf_entry));
    func_8001FBE4(sprite, 0xf1u, operands);
    if (memcmp(&fixture, &expected, sizeof(fixture)) ||
        native_leaf_entries != expect_leaf ||
        (expect_leaf && native_leaf_arg != (uint32_t)(uintptr_t)sprite) ||
        (expect_leaf && memcmp(native_leaf_entry, retail_leaf_entry,
                               sizeof(native_leaf_entry)))) {
        unsigned diff = 0;
        while (diff < sizeof(fixture) &&
               ((uint8_t *)&fixture)[diff] == ((uint8_t *)&expected)[diff])
            ++diff;
        fprintf(stderr, "SPRITE F1 FAIL case=%u bytes=%02x/%02x/%02x mode=%u alias=%u "
                "diff=%u leaf=%u/%u args=%08x/%08x\n", cases, a, b, c,
                actual_mode, alias, diff, native_leaf_entries, retail_leaf_entries,
                native_leaf_arg, retail_leaf_arg);
        exit(1);
    }
    ++cases;
}

int main(void)
{
    assert((uintptr_t)&fixture + sizeof(fixture) <= UINT32_MAX);
    FILE *image = fopen("disc/SLUS_006.64", "rb");
    assert(image && !fseek(image, 0x800, SEEK_SET));
    assert(fread(ram + 0x10000, 1, sizeof(ram) - 0x10000, image) > 0x48000);
    assert(!fclose(image));

    /* The first case is intentionally the one-case semantic RED gate. */
    compare(0x00, 0x00, 0x00, 0, 0, 0);

    const unsigned values[] = {0, 1, 2, 0x7f, 0x80, 0xfe, 0xff};
    for (unsigned mode = 0; mode < 4; ++mode)
        for (unsigned alias = 0; alias < 29; ++alias)
            for (unsigned variant = 0; variant < 8; ++variant)
                for (unsigned i = 0; i < sizeof(values) / sizeof(values[0]); ++i)
                    compare(values[i], values[(i + alias) % 7],
                            values[(i + variant + 2) % 7], mode, alias, variant);
    /* Complete per-channel byte domains without a redundant 16M RGB cube. */
    for (unsigned mode = 0; mode < 4; ++mode)
        for (unsigned channel = 0; channel < 3; ++channel)
            for (unsigned value = 0; value < 256; ++value) {
                unsigned rgb[3] = {0x25, 0xa0, 0x5a};
                rgb[channel] = value;
                compare(rgb[0], rgb[1], rgb[2], mode, 0, 7);
            }
    /* A model base may alias the sprite: its third color halfword then
     * changes sprite+3C and must control the final leaf-call decision. */
    const unsigned overlapping_aliases[] = {0, 4, 5, 6, 18, 19, 20, 21, 22, 23};
    for (unsigned mode = 0; mode < 4; ++mode)
        for (unsigned i = 0; i < sizeof(overlapping_aliases) / sizeof(overlapping_aliases[0]); ++i)
            for (unsigned value = 0; value < 256; ++value)
                compare(0x63, 0x25, value, mode, overlapping_aliases[i], 0x107);
    assert(cases == 19809);
    printf("SPRITE F1 PASS %u cases: retail/native F1 order, mode-2 sequential stores, "
           "mode reload after overlapping base stores, actual leaf, all channel bytes, "
           "finite RGB edges and aliases\n", cases);
    puts("SPRITE F1 scope: exact retail dispatcher and extracted rendering leaf; "
         "no exhaustive 16M RGB cross product claim");
    return 0;
}

/* C5 is outside this test: keep its newly linked dependency fail-closed. */
void func_80022CDC(void *sprite) { (void)sprite; __builtin_trap(); }
