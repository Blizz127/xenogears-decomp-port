#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "battle_mips_adapter.h"

extern void func_8001FBE4(void *, uint32_t, void *);
extern void ba_helper_impl(uint8_t *, int32_t);
extern void ba_leaf_impl(void *);

/* The BA lane intentionally links the verbatim native helper and leaf through
 * thin wrappers. They record the helper ABI and 8001F6B0 entry state, then run
 * the real leaf body; this still makes no claim about displayed rendering. */
static unsigned native_helper_entries;
static uint32_t native_helper_sprite;
static uint32_t native_helper_value;
static unsigned native_leaf_entries;
static uint32_t native_leaf_sprite;
static uint8_t native_leaf_state[0xC0];

void func_80023290(uint8_t *sprite, int32_t value)
{
    ++native_helper_entries;
    native_helper_sprite = (uint32_t)(uintptr_t)sprite;
    native_helper_value = (uint32_t)value;
    ba_helper_impl(sprite, value);
}

void func_8001F6B0(void *sprite)
{
    ++native_leaf_entries;
    native_leaf_sprite = (uint32_t)(uintptr_t)sprite;
    memcpy(native_leaf_state, sprite, sizeof(native_leaf_state));
    ba_leaf_impl(sprite);
}

#define GUARD(name) \
    void name(void) { \
        fputs("SPRITE BA FAIL unexpected callee " #name "\n", stderr); \
        abort(); \
    }
GUARD(func_800B2AEC)
GUARD(func_80039E60)
/* Guest-RAM backing for the opcode 0xB0 selector read; never touched here. */
uint8_t g_PsxRam[0x300000];
GUARD(AnimScriptTick) GUARD(HeapAlloc) GUARD(HeapChangeCurrentUser)
GUARD(HeapFree) GUARD(__wrap_ReadGeomOffset) GUARD(__wrap_ScaleMatrixL)
GUARD(func_8001CE74) GUARD(func_8001D2B0) GUARD(func_8001D4E8) GUARD(func_8001EE68)
GUARD(func_80022D44) GUARD(func_80023B84)
GUARD(func_800245D8) GUARD(func_8002C3E8) GUARD(func_8002C59C)
GUARD(func_8002C8CC) GUARD(func_8002CB54) GUARD(func_8002CC10)
GUARD(func_80022CAC) GUARD(func_80023124) GUARD(func_8001FB30)

uint32_t D_80018644, D_800592E4;
int32_t D_80059198;
uint8_t D_800591AD, D_800591B0, D_800591B3;
int16_t D_800592E8, D_800592EA;
uint32_t D_8004FBB8[8], D_8006F99C[4], D_8006F9AC[4], D_800C3EB0[4];
int32_t g_WorkListCurTimer;
uint8_t D_8006BE10[32];
int g_cfg_pgxpTextureCorrection;

extern uint32_t g_RandomSeed;
extern int __real_rand(void);
static unsigned native_rand_calls;
int __wrap_rand(void)
{
    ++native_rand_calls;
    return __real_rand();
}

static uint8_t ram[0x200000];
static struct {
    uint8_t before[0x20];
    uint8_t sprite[0xC0];
    uint8_t after[0x20];
    uint8_t ops[8];
    uint8_t base[0x80];
    uint8_t prim[0x18 * 64];
} fixture, initial, expected;
static uint8_t expected_leaf_state[0xC0];
static unsigned cases;
static unsigned retail_handler_entries;
static unsigned retail_helper_entries;
static unsigned retail_leaf_entries;
static uint32_t retail_helper_sprite;
static uint32_t retail_helper_value;
static uint32_t retail_leaf_sprite;
static PcPortMipsCpu *active_cpu;

static uint8_t *address(uint32_t address_value, unsigned width)
{
    uintptr_t seed = (uintptr_t)&g_RandomSeed;
    uintptr_t first = (uintptr_t)&fixture;

    if (address_value >= seed &&
        (uint64_t)address_value + width <= seed + 4)
        return (uint8_t *)(uintptr_t)address_value;
    if (address_value >= 0x8005a1fcu &&
        (uint64_t)address_value + width <= 0x8005a200u)
        return (uint8_t *)&g_RandomSeed + (address_value - 0x8005a1fcu);
    if (address_value >= first &&
        (uint64_t)address_value + width <= first + sizeof(fixture))
        return (uint8_t *)(uintptr_t)address_value;
    if (address_value >= 0x80000000u &&
        (uint64_t)address_value + width <= 0x80200000u)
        return ram + (address_value & 0x1fffffu);
    return NULL;
}

static int read_memory(void *unused, uint32_t address_value, unsigned width,
                       uint32_t *value)
{
    (void)unused;
    uint8_t *p = address(address_value, width);
    if (p == NULL)
        return -1;
    retail_handler_entries += address_value == 0x80020f38u;
    *value = 0;
    for (unsigned i = 0; i < width; ++i)
        *value |= (uint32_t)p[i] << (i * 8);
    return 0;
}

static int write_memory(void *unused, uint32_t address_value, unsigned width,
                        uint32_t value)
{
    (void)unused;
    uint8_t *p = address(address_value, width);
    if (p == NULL)
        return -1;
    for (unsigned i = 0; i < width; ++i)
        p[i] = (uint8_t)(value >> (i * 8));
    return 0;
}

static int bridge(void *unused, PcPortMipsCpu *cpu, uint32_t target)
{
    (void)unused;
    if (target == 0x80023290u) {
        ++retail_helper_entries;
        retail_helper_sprite = cpu->gpr[4];
        retail_helper_value = cpu->gpr[5];
        return 0;
    }
    if (target == 0x8001f6b0u) {
        uint8_t *sprite = address(cpu->gpr[4], sizeof(expected_leaf_state));
        if (sprite == NULL)
            return -1;
        ++retail_leaf_entries;
        retail_leaf_sprite = cpu->gpr[4];
        memcpy(expected_leaf_state, sprite, sizeof(expected_leaf_state));
        return 0;
    }
    return 0;
}

static void put32(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

static uint32_t get32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void run_retail(uint8_t *sprite, uint8_t *ops)
{
    PcPortMipsBus bus = {
        .read = read_memory,
        .write = write_memory,
        .bridge = bridge,
    };
    PcPortMipsCpu cpu;

    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = (uint32_t)(uintptr_t)sprite;
    cpu.gpr[5] = 0xBAu;
    cpu.gpr[6] = (uint32_t)(uintptr_t)ops;
    cpu.gpr[29] = 0x801fff00u;
    cpu.gpr[31] = 0xfffffffcu;
    active_cpu = &cpu;
    int result = PcPortMipsRun(&cpu, 0x8001fbe4u, 0xfffffffcu, 2000);
    active_cpu = NULL;
    if (result != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "SPRITE BA FAIL retail pc=%08x: %s\n",
                cpu.pc, cpu.error);
        exit(2);
    }
}

static void compare(unsigned operand_value, unsigned requested_type,
                    unsigned alias, unsigned variant)
{
    static const uint32_t flags3c[] = {
        0x00000000u, 0xffffffffu, 0xa5a55a5au, 0x800000e1u,
    };
    static const uint32_t flags40[] = {
        0x00000000u, 0xffffffffu, 0x13579bdfu, 0x80010001u,
    };
    uint8_t *sprite;
    uint8_t *ops;
    uint8_t *aliases[23];
    uint8_t input_value;
    unsigned actual_type;
    unsigned expect_leaf;

    for (unsigned i = 0; i < sizeof(fixture); ++i)
        ((uint8_t *)&fixture)[i] = (uint8_t)(i * 37u + variant * 73u + operand_value);
    sprite = fixture.sprite;
    put32(sprite + 0x38, 0x2468ace0u ^ flags3c[variant & 3]);
    put32(sprite + 0x3c, flags3c[variant & 3]);
    put32(sprite + 0x40, (flags40[variant & 3] & ~(0xFu << 13)) |
                         (requested_type << 13));
    put32(sprite + 0x24, (uint32_t)(uintptr_t)fixture.base);
    put32(sprite + 0x20, (uint32_t)(uintptr_t)fixture.base);
    put32(fixture.base + 0x30, (uint32_t)(uintptr_t)fixture.prim);
    ((uint8_t *)sprite)[0x40] = (uint8_t)(((operand_value + alias + variant) & 63u) << 2);
    put32(sprite + 0x40, get32(sprite + 0x40) | (requested_type << 13));
    aliases[0] = fixture.ops;
    aliases[1] = sprite + 0x00;
    aliases[2] = sprite + 0x01;
    aliases[3] = sprite + 0x02;
    aliases[4] = sprite + 0x2a;
    aliases[5] = sprite + 0x2b;
    aliases[6] = sprite + 0x2c;
    aliases[7] = sprite + 0x2d;
    aliases[8] = sprite + 0x38;
    aliases[9] = sprite + 0x39;
    aliases[10] = sprite + 0x3a;
    aliases[11] = sprite + 0x3b;
    aliases[12] = sprite + 0x3c;
    aliases[13] = sprite + 0x3d;
    aliases[14] = sprite + 0x3e;
    aliases[15] = sprite + 0x3f;
    aliases[16] = sprite + 0x40;
    aliases[17] = sprite + 0x41;
    aliases[18] = sprite + 0x42;
    aliases[19] = sprite + 0x43;
    aliases[20] = sprite + 0xa8;
    aliases[21] = sprite + 0xac;
    aliases[22] = sprite + 0xbf;
    ops = aliases[alias];
    ops[0] = (uint8_t)operand_value;
    input_value = ops[0];
    initial = fixture;

    retail_handler_entries = 0;
    retail_helper_entries = 0;
    retail_leaf_entries = 0;
    retail_helper_sprite = 0;
    retail_helper_value = 0;
    retail_leaf_sprite = 0;
    run_retail(sprite, ops);
    actual_type = (get32(sprite + 0x40) >> 13) & 0xFu;
    expect_leaf = actual_type != 8 && actual_type != 9;
    if (retail_handler_entries != 1 || retail_helper_entries != 1 ||
        retail_leaf_entries != expect_leaf ||
        retail_helper_sprite != (uint32_t)(uintptr_t)sprite ||
        retail_helper_value != input_value ||
        (expect_leaf && retail_leaf_sprite != (uint32_t)(uintptr_t)sprite)) {
        fprintf(stderr,
                "SPRITE BA FAIL oracle case=%u type=%u alias=%u entries=%u/%u/%u "
                "args=%08x/%08x expected=%08x/%08x\n", cases, actual_type,
                alias, retail_handler_entries, retail_helper_entries,
                retail_leaf_entries, retail_helper_sprite, retail_helper_value,
                (uint32_t)(uintptr_t)sprite, (uint32_t)input_value);
        exit(2);
    }
    expected = fixture;

    fixture = initial;
    native_helper_entries = 0;
    native_helper_sprite = 0;
    native_helper_value = 0;
    native_leaf_entries = 0;
    native_leaf_sprite = 0;
    memset(native_leaf_state, 0, sizeof(native_leaf_state));
    native_rand_calls = 0;
    func_8001FBE4(sprite, 0xBAu, ops);
    if (memcmp(&fixture, &expected, sizeof(fixture)) ||
        native_helper_entries != 1 ||
        native_helper_sprite != (uint32_t)(uintptr_t)sprite ||
        native_helper_value != retail_helper_value ||
        native_leaf_entries != expect_leaf ||
        (expect_leaf && native_leaf_sprite != (uint32_t)(uintptr_t)sprite) ||
        (expect_leaf && memcmp(native_leaf_state, expected_leaf_state,
                                sizeof(native_leaf_state))) ||
        native_rand_calls != 0) {
        unsigned diff = 0;
        while (diff < sizeof(fixture) &&
               ((uint8_t *)&fixture)[diff] == ((uint8_t *)&expected)[diff])
            ++diff;
        fprintf(stderr,
                "SPRITE BA FAIL case=%u operand=%02x type=%u alias=%u variant=%u "
                "diff=%u helper=%u/%u args=%08x/%08x leaf=%u/%u rand=%u\n",
                cases, input_value, actual_type, alias, variant, diff,
                native_helper_entries, retail_helper_entries,
                native_helper_value, retail_helper_value, native_leaf_entries,
                retail_leaf_entries, native_rand_calls);
        exit(1);
    }
    ++cases;
}

int main(void)
{
    const char *quick = getenv("SPRITE_BA_QUICK");
    assert((uintptr_t)&fixture + sizeof(fixture) <= UINT32_MAX);
    FILE *image = fopen("disc/SLUS_006.64", "rb");
    assert(image && !fseek(image, 0x800, SEEK_SET));
    assert(fread(ram + 0x10000, 1, sizeof(ram) - 0x10000, image) > 0x48000);
    assert(!fclose(image));

    if (quick != NULL) {
        unsigned type = 0;
        const char *type_text = getenv("SPRITE_BA_QUICK_TYPE");
        if (type_text != NULL)
            type = (unsigned)strtoul(type_text, NULL, 0) & 15u;
        compare(0xe7, type, 0, 0);
        printf("SPRITE BA QUICK PASS %u cases\n", cases);
        return 0;
    }
    for (unsigned operand = 0; operand < 256; ++operand)
        for (unsigned type = 0; type < 16; ++type)
            for (unsigned alias = 0; alias < 23; ++alias)
                compare(operand, type, alias,
                        (operand ^ type ^ alias) & 3u);
    printf("SPRITE BA PASS %u cases: all 256 operands, types 0..15, finite flag "
           "neighbors, sprite operand aliases, helper/leaf call order\n", cases);
    puts("SPRITE BA scope: actual retail/native dispatcher, verbatim helper and "
         "leaf; 8001F6B0 entry is observed, so no displayed-rendering claim.");
    return 0;
}

/* C5 is outside this test: keep its newly linked dependency fail-closed. */
void func_80022CDC(void *sprite) { (void)sprite; __builtin_trap(); }
