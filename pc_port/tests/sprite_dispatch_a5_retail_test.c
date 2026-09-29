#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "battle_mips_adapter.h"

extern void func_8001FBE4(void *, uint32_t, void *);

#define GUARD(name) void name(void) { fputs("SPRITE A5 FAIL unexpected callee " #name "\n", stderr); abort(); }
GUARD(func_800B2AEC)
GUARD(func_80039E60)
/* Guest-RAM backing for the opcode 0xB0 selector read; never touched here. */
uint8_t g_PsxRam[0x300000];
GUARD(AnimScriptTick) GUARD(HeapAlloc) GUARD(HeapChangeCurrentUser) GUARD(HeapFree)
GUARD(__wrap_ReadGeomOffset) GUARD(__wrap_ScaleMatrix) GUARD(__wrap_ScaleMatrixL)
GUARD(func_8001CE74) GUARD(func_8001D2B0) GUARD(func_8001D4E8) GUARD(func_8001EE68)
GUARD(func_8001F6B0) GUARD(func_8001FB30)
GUARD(func_80022D44) GUARD(func_80023B84) GUARD(func_80023290)
GUARD(func_800245D8) GUARD(func_8002C3E8) GUARD(func_8002C59C)
GUARD(func_8002C8CC) GUARD(func_8002CB54) GUARD(func_8002CC10)
GUARD(func_80023124)

uint32_t D_80018644, D_800592E4;
int32_t D_80059198;
uint8_t D_800591AD, D_800591B0, D_800591B3;
int16_t D_800592E8, D_800592EA;
uint32_t D_8004FBB8[8], D_8006F99C[4], D_8006F9AC[4], D_800C3EB0[4];
int32_t g_WorkListCurTimer;
uint8_t D_8006BE10[32];
int g_cfg_pgxpTextureCorrection;

extern int __real_rcos(int);
extern int __real_rsin(int);
extern void a5_velocity_impl(void *);
static unsigned native_helper_calls, native_rcos_calls, native_rsin_calls;
static unsigned native_trig_order[4], native_trig_count;
static int native_trig_args[4];

void func_80022974(void *sprite)
{
    ++native_helper_calls;
    a5_velocity_impl(sprite);
}

int __wrap_rcos(int angle)
{
    if (native_trig_count < 4) {
        native_trig_order[native_trig_count] = 1;
        native_trig_args[native_trig_count] = angle;
        ++native_trig_count;
    }
    ++native_rcos_calls;
    return __real_rcos(angle);
}

int __wrap_rsin(int angle)
{
    if (native_trig_count < 4) {
        native_trig_order[native_trig_count] = 2;
        native_trig_args[native_trig_count] = angle;
        ++native_trig_count;
    }
    ++native_rsin_calls;
    return __real_rsin(angle);
}

static uint8_t ram[0x200000];
static struct {
    uint8_t before[0x40];
    uint8_t sprite[0xc0];
    uint8_t ops[8];
    uint8_t after[0x40];
} fixture, initial, expected;
static unsigned cases;
static unsigned handler_entries, velocity_entries, rcos_entries, rsin_entries;
static unsigned retail_trig_order[4], retail_trig_count;
static int retail_trig_args[4];
static PcPortMipsCpu *active_cpu;

static uint8_t *address(uint32_t a, unsigned width)
{
    uintptr_t factor = (uintptr_t)&D_80059198;
    uintptr_t first = (uintptr_t)&fixture;
    if (a >= factor && (uint64_t)a + width <= factor + 4)
        return (uint8_t *)(uintptr_t)a;
    if (a >= 0x80059198u && (uint64_t)a + width <= 0x8005919cu)
        return (uint8_t *)&D_80059198 + (a - 0x80059198u);
    if (a >= first && (uint64_t)a + width <= first + sizeof(fixture))
        return (uint8_t *)(uintptr_t)a;
    if (a >= 0x80000000u && (uint64_t)a + width <= 0x80200000u)
        return ram + (a & 0x1fffffu);
    return NULL;
}

static int rd(void *unused, uint32_t a, unsigned width, uint32_t *value)
{
    (void)unused;
    uint8_t *p = address(a, width);
    if (!p) return -1;
    if (a == 0x80021884u) ++handler_entries;
    if (a == 0x80022974u) ++velocity_entries;
    if (a == 0x8003f8b0u) {
        ++rcos_entries;
        if (retail_trig_count < 4) {
            retail_trig_order[retail_trig_count] = 1;
            retail_trig_args[retail_trig_count] = active_cpu ? (int)active_cpu->gpr[4] : 0;
            ++retail_trig_count;
        }
    }
    if (a == 0x8003f8ccu) {
        ++rsin_entries;
        if (retail_trig_count < 4) {
            retail_trig_order[retail_trig_count] = 2;
            retail_trig_args[retail_trig_count] = active_cpu ? (int)active_cpu->gpr[4] : 0;
            ++retail_trig_count;
        }
    }
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

static void put16(uint8_t *p, uint16_t value) { memcpy(p, &value, 2); }
static void put32(uint8_t *p, uint32_t value) { memcpy(p, &value, 4); }
static uint32_t word(const uint8_t *p) { uint32_t value; memcpy(&value, p, 4); return value; }

static void compare(uint8_t operand, int16_t speed, uint16_t angle,
                    uint32_t radius, uint32_t flags, int32_t factor,
                    unsigned alias, unsigned variant)
{
    static const uint32_t positions[] = {
        0, 1, 0xffffffffu, 0x7fffffffu, 0x80000000u, 0x80000001u,
        0x12345678u, 0xff0000ffu
    };
    for (unsigned i = 0; i < sizeof(fixture); ++i)
        ((uint8_t *)&fixture)[i] = (uint8_t)(i * 37u + variant * 73u + operand);
    uint8_t *p = fixture.sprite;
    put32(p + 0x18, radius);
    put32(p + 0xAC, flags);
    put16(p + 0x82, (uint16_t)speed);
    put16(p + 0x32, angle);
    D_80059198 = factor;
    uint8_t *aliases[] = {
        fixture.ops, p + 0x18, p + 0x19, p + 0x82, p + 0x83,
        p + 0x32, p + 0x33, p + 0xAC,
        (uint8_t *)&D_80059198, (uint8_t *)&D_80059198 + 1,
        (uint8_t *)&D_80059198 + 2, (uint8_t *)&D_80059198 + 3
    };
    uint8_t *ops = aliases[alias % (sizeof(aliases) / sizeof(aliases[0]))];
    ops[0] = operand;
    put32(p + 0x0, positions[variant & 7]);
    put32(p + 0x8, positions[(variant + 3) & 7]);
    initial = fixture;
    uint32_t initial_factor = (uint32_t)D_80059198;
    uint32_t opcode = 0x000000a5u | (variant << 8);

    PcPortMipsBus bus = {.read = rd, .write = wr};
    PcPortMipsCpu cpu;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = (uint32_t)(uintptr_t)p;
    cpu.gpr[5] = opcode;
    cpu.gpr[6] = (uint32_t)(uintptr_t)ops;
    cpu.gpr[29] = 0x801fff00u;
    cpu.gpr[31] = 0xfffffffcu;
    handler_entries = velocity_entries = rcos_entries = rsin_entries = 0;
    retail_trig_count = 0;
    memset(retail_trig_order, 0, sizeof(retail_trig_order));
    memset(retail_trig_args, 0, sizeof(retail_trig_args));
    active_cpu = &cpu;
    int result = PcPortMipsRun(&cpu, 0x8001fbe4u, 0xfffffffcu, 1600);
    active_cpu = NULL;
    if (result != PC_PORT_MIPS_HALTED || handler_entries != 1 || velocity_entries != 1 ||
        rcos_entries != 1 || rsin_entries != 1 || retail_trig_count != 2) {
        fprintf(stderr, "SPRITE A5 FAIL oracle case=%u pc=%08x entries=%u/%u/%u/%u trig=%u: %s\n",
                cases, cpu.pc, handler_entries, velocity_entries, rcos_entries,
                rsin_entries, retail_trig_count, cpu.error);
        exit(2);
    }
    expected = fixture;
    uint32_t expected_factor = (uint32_t)D_80059198;

    fixture = initial;
    D_80059198 = (int32_t)initial_factor;
    native_helper_calls = native_rcos_calls = native_rsin_calls = 0;
    native_trig_count = 0;
    memset(native_trig_order, 0, sizeof(native_trig_order));
    memset(native_trig_args, 0, sizeof(native_trig_args));
    func_8001FBE4(p, opcode, ops);
    /* The retail symbol map names sine rcos and cosine rsin; the native
     * shim maps these to the conventional PsyCross names recorded above.
     * The native helper masks before the call; retail masks in each leaf. */
    int trig_mismatch = native_trig_count != retail_trig_count;
    for (unsigned i = 0; i < retail_trig_count; ++i)
        trig_mismatch |= native_trig_order[i] != 3u - retail_trig_order[i] ||
                         native_trig_args[i] != (retail_trig_args[i] & 0xfff);
    if (memcmp(&fixture, &expected, sizeof(fixture)) ||
        (uint32_t)D_80059198 != expected_factor || native_helper_calls != 1 ||
        native_rcos_calls != 1 || native_rsin_calls != 1 || trig_mismatch) {
        unsigned diff = 0;
        while (diff < sizeof(fixture) && ((uint8_t *)&fixture)[diff] == ((uint8_t *)&expected)[diff]) ++diff;
        fprintf(stderr, "SPRITE A5 FAIL case=%u operand=%02x speed=%d angle=%04x radius=%08x "
                "flags=%08x factor=%08x alias=%u variant=%u diff=%u pos=%08x,%08x/%08x,%08x "
                "vel=%08x,%08x/%08x,%08x factor=%08x/%08x helper=%u trig=%u,%u mismatch=%d\n",
                cases, operand, speed, angle, radius, flags, (uint32_t)factor, alias,
                variant, diff, word(p), word(p + 0x18), word(expected.sprite),
                word(expected.sprite + 0x18), word(p + 0x0c), word(p + 0x14),
                word(expected.sprite + 0x0c), word(expected.sprite + 0x14),
                (uint32_t)D_80059198, expected_factor, native_helper_calls,
                native_rcos_calls, native_rsin_calls, trig_mismatch);
        exit(1);
    }
    ++cases;
}

int main(void)
{
    assert((uintptr_t)&fixture + sizeof(fixture) <= UINT32_MAX);
    assert((uintptr_t)&D_80059198 + 4 <= UINT32_MAX);
    FILE *image = fopen("disc/SLUS_006.64", "rb");
    assert(image && !fseek(image, 0x800, SEEK_SET));
    assert(fread(ram + 0x10000, 1, sizeof(ram) - 0x10000, image) > 0x48000);
    assert(!fclose(image));

    const uint8_t operands[] = {0, 1, 2, 0x7f, 0x80, 0xfe, 0xff};
    const int16_t speed_edges[] = {0, 1, -1, 0x7ff, -0x800, 0x1000, -0x1000, 0x7fff, (int16_t)0x8000};
    const uint16_t angles[] = {0, 1, 0xfff, 0x1000, 0x1001, 0x7fff, 0x8000, 0xffff};
    const uint32_t radii[] = {0, 1, 0xffffffffu, 0x7fffffffu, 0x80000000u, 0x80000001u, 0x12345678u, 0xff0000ffu};
    const uint32_t factors[] = {0, 1, 0xffffffffu, 0x7fffffffu, 0x80000000u, 2, 0xfffffffeu};
    const uint32_t divisors[] = {0, 1, 2, 3, 0x7ffu, 0xfffu};

    compare(0x40, 0x1000, 0x3456, 0x00123456u, 0x00008000u, 0, 0, 0);
    if (getenv("SPRITE_A5_QUICK")) {
        compare(0xff, (int16_t)0x8000, 0xffff, 0x80000000u, 0, INT32_MAX, 0, 1);
        compare(0x80, -0x1000, 0x1001, 0xffffffffu, 0xfff << 7, INT32_MIN, 8, 2);
        compare(0x7f, -1, 0x0001, 1, 1u << 7, 1, 0, 3);
        printf("SPRITE A5 QUICK PASS %u cases\n", cases);
        return 0;
    }
    for (unsigned o = 0; o < sizeof(operands); ++o)
        for (unsigned s = 0; s < sizeof(speed_edges) / sizeof(speed_edges[0]); ++s)
            for (unsigned f = 0; f < sizeof(factors) / sizeof(factors[0]); ++f)
                compare(operands[o], speed_edges[s], angles[(o + s) & 7], radii[(o + f) & 7],
                        divisors[(o + s + f) % 6] << 7, (int32_t)factors[f], 0, (o ^ s ^ f) & 3);
    for (unsigned o = 0; o < 256; ++o)
        compare((uint8_t)o, 0x1800, (uint16_t)o, radii[o & 7], (o & 0xfffu) << 7,
                (int32_t)factors[o % 7], 0, o & 3);
    for (unsigned s = 0; s < 65536; ++s)
        compare(0xa5, (int16_t)s, angles[s & 7], radii[s & 7], (s & 0xfffu) << 7,
                (int32_t)factors[s % 7], 0, s & 3);
    for (unsigned angle = 0; angle < 4096; ++angle)
        compare((uint8_t)(angle ^ 0x5a), 0x1000, (uint16_t)angle, radii[angle & 7],
                divisors[angle % 6] << 7, (int32_t)factors[angle % 7], 0, angle & 3);
    for (unsigned divisor = 0; divisor < 4096; ++divisor)
        compare((uint8_t)divisor, speed_edges[divisor % 9], angles[divisor & 7], radii[divisor & 7],
                divisor << 7, (int32_t)factors[divisor % 7], 0, divisor & 3);
    for (unsigned alias = 1; alias < 12; ++alias)
        for (unsigned value = 0; value < 256; value += 17)
            compare((uint8_t)value, speed_edges[alias % 9], angles[value & 7], radii[(value + alias) & 7],
                    divisors[(value + alias) % 6] << 7, (int32_t)factors[alias % 7], alias, alias & 3);

    printf("SPRITE A5 PASS %u cases: retail/native A5 and 22974, full operand/p82/angle/divisor axes, "
           "global-factor overflow and DIV-zero edges, velocity side effects, whole fixture and aliases\n", cases);
    puts("SPRITE A5 scope: actual SLUS_006.64 dispatcher/A5/22974/trig instructions and native existing helper; "
         "finite independent census, no exhaustive Cartesian or rendering claim.");
    return 0;
}

/* C5 is outside this test: keep its newly linked dependency fail-closed. */
void func_80022CDC(void *sprite) { (void)sprite; __builtin_trap(); }
