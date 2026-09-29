#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "battle_mips_adapter.h"
#include "psx/gtereg.h"

extern void func_8001FBE4(void *, uint32_t, void *);
extern unsigned int MFC2(int), CFC2(int);
extern void MTC2(unsigned int, int), CTC2(unsigned int, int);
extern int doCOP2(int);

/* C4 owns RotMatrix and ApplyMatrixLV through the real PsyCross GTE backend.
 * These guards cover unrelated dispatcher paths pulled into the same native
 * translation unit; they must never become replacement implementations. */
#define GUARD(name) void name(void) { fputs("SPRITE C4 FAIL unexpected callee " #name "\n", stderr); abort(); }
GUARD(func_800B2AEC)
GUARD(func_80039E60)
/* Guest-RAM backing for the opcode 0xB0 selector read; never touched here. */
uint8_t g_PsxRam[0x300000];
GUARD(AnimScriptTick) GUARD(__wrap_ApplyMatrixSV) GUARD(HeapAlloc)
GUARD(HeapChangeCurrentUser) GUARD(HeapFree) GUARD(__wrap_MulMatrix0)
GUARD(__wrap_ReadGeomOffset) GUARD(__wrap_ScaleMatrix) GUARD(__wrap_ScaleMatrixL)
GUARD(func_8001CE74) GUARD(func_8001D2B0) GUARD(func_8001D4E8) GUARD(func_8001EE68)
GUARD(func_8001F6B0) GUARD(func_8001FB30) GUARD(func_80022D44)
GUARD(func_80022CAC) GUARD(func_80023290) GUARD(func_80023B84) GUARD(func_800245D8)
GUARD(func_8002C3E8) GUARD(func_8002C59C) GUARD(func_8002C8CC)
GUARD(func_8002CB54) GUARD(func_8002CC10)

int32_t func_80023124(int32_t a, int32_t b)
{ (void)a; (void)b; fputs("SPRITE C4 FAIL unexpected angle helper\n", stderr); abort(); }

uint32_t D_80018644, D_800592E4;
int32_t D_80059198;
uint8_t D_800591AD, D_800591B0, D_800591B3;
int16_t D_800592E8, D_800592EA;
uint32_t D_8004FBB8[8], D_8006F99C[4], D_8006F9AC[4], D_800C3EB0[4];
/* Opcode 0xA7 timer (s32) and opcode 0xBD packed block; both unreached here. */
int32_t g_WorkListCurTimer;
uint8_t D_8006BE10[32];
int g_cfg_pgxpTextureCorrection;

extern uint32_t g_RandomSeed;
extern int __real_rand(void);
static unsigned native_rand_calls;
int __wrap_rand(void) { ++native_rand_calls; return __real_rand(); }

static uint8_t ram[0x200000];
static struct {
    uint32_t before[8];
    uint8_t sprite[0x80];
    uint8_t between[8];
    uint8_t ops[4];
    uint8_t after[8];
} fixture, initial, expected;
static unsigned cases;
static unsigned handler_entries, rand_entries, vector_entries;
static unsigned rot_entries, apply_entries;
static PcPortMipsCpu *active_cpu;

/* C4's meaningful GTE post-state is the rotation matrix loaded by SetRotMatrix,
 * IR1..IR3 and MAC1..MAC3 produced by the two ApplyMatrixLV passes, and FLAG.
 * The remaining data/control registers are unrelated or unread by this opcode
 * and are deliberately excluded from the oracle comparison. */
typedef struct {
    uint32_t matrix[5];
    uint32_t ir[3];
    uint32_t mac[3];
    uint32_t flag;
} C4GteSnapshot;

static C4GteSnapshot expected_gte, native_gte;

static void save_c4_gte(C4GteSnapshot *snapshot)
{
    memcpy(snapshot->matrix, &gteRegs.CP2C.r[0], sizeof(snapshot->matrix));
    for (unsigned i = 0; i < 3; ++i) {
        snapshot->ir[i] = gteRegs.CP2D.r[9 + i];
        snapshot->mac[i] = gteRegs.CP2D.r[25 + i];
    }
    snapshot->flag = gteRegs.CP2C.r[31];
}

static uint8_t *address(uint32_t a, unsigned width)
{
    uintptr_t seed = (uintptr_t)&g_RandomSeed;
    uintptr_t first = (uintptr_t)&fixture;
    if (a >= seed && (uint64_t)a + width <= seed + 4)
        return (uint8_t *)(uintptr_t)a;
    if (a >= 0x8005a1fcu && (uint64_t)a + width <= 0x8005a200u)
        return (uint8_t *)&g_RandomSeed + (a - 0x8005a1fcu);
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
    handler_entries += a == 0x800215b8u;
    rand_entries += a == 0x8003fa38u;
    vector_entries += a == 0x80021b04u;
    rot_entries += a == 0x8003f738u;
    apply_entries += a == 0x8004947cu;
    (void)active_cpu;
    *value = 0;
    for (unsigned i = 0; i < width; ++i)
        *value |= (uint32_t)p[i] << (i * 8);
    return 0;
}

static int wr(void *unused, uint32_t a, unsigned width, uint32_t value)
{
    (void)unused;
    uint8_t *p = address(a, width);
    if (!p) return -1;
    for (unsigned i = 0; i < width; ++i)
        p[i] = (uint8_t)(value >> (i * 8));
    return 0;
}

static uint32_t c2read(void *unused, int control, unsigned reg)
{
    (void)unused;
    return control ? CFC2((int)reg) : MFC2((int)reg);
}

static void c2write(void *unused, int control, unsigned reg, uint32_t value)
{
    (void)unused;
    if (control) CTC2(value, (int)reg); else MTC2(value, (int)reg);
}

static int c2command(void *unused, uint32_t value)
{
    (void)unused;
    doCOP2((int)value);
    return 0;
}

static void put32(uint8_t *p, uint32_t value) { memcpy(p, &value, 4); }

static void run_retail(uint8_t *sprite, uint8_t *ops)
{
    PcPortMipsBus bus = {.read = rd, .write = wr, .cop2_read = c2read,
                         .cop2_write = c2write, .cop2_command = c2command};
    PcPortMipsCpu cpu;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = (uint32_t)(uintptr_t)sprite;
    cpu.gpr[5] = 0x000000c4u;
    cpu.gpr[6] = (uint32_t)(uintptr_t)ops;
    cpu.gpr[29] = 0x801fff00u;
    cpu.gpr[31] = 0xfffffffcu;
    active_cpu = &cpu;
    int result = PcPortMipsRun(&cpu, 0x8001fbe4u, 0xfffffffcu, 4000);
    active_cpu = NULL;
    if (result != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "SPRITE C4 FAIL retail pc=%08x: %s\n", cpu.pc, cpu.error);
        exit(2);
    }
}

static void compare(uint8_t range, uint32_t seed,
                    int32_t velocity_x, int32_t velocity_y, int32_t velocity_z,
                    unsigned alias, unsigned variant)
{
    for (unsigned i = 0; i < sizeof(fixture); ++i)
        ((uint8_t *)&fixture)[i] = (uint8_t)(i * 37u + variant * 73u + range);
    uint8_t *sprite = fixture.sprite;
    put32(sprite + 0x0c, (uint32_t)velocity_x);
    put32(sprite + 0x10, (uint32_t)velocity_y);
    put32(sprite + 0x14, (uint32_t)velocity_z);
    uint8_t *operands[] = {
        fixture.ops, sprite + 0x0c, sprite + 0x0d, sprite + 0x10,
        sprite + 0x11, sprite + 0x14, sprite + 0x15,
        (uint8_t *)&g_RandomSeed, (uint8_t *)&g_RandomSeed + 1,
        (uint8_t *)&g_RandomSeed + 2, (uint8_t *)&g_RandomSeed + 3
    };
    uint8_t *ops = operands[alias];
    g_RandomSeed = seed;
    ops[0] = range;
    initial = fixture;
    uint32_t initial_seed = g_RandomSeed;

    memset(&gteRegs, 0, sizeof(gteRegs));
    handler_entries = rand_entries = vector_entries = 0;
    rot_entries = apply_entries = 0;
    run_retail(sprite, ops);
    if (handler_entries != 1 || rand_entries != 1 || vector_entries != 1 ||
        rot_entries != 1 || apply_entries != 1) {
        fprintf(stderr, "SPRITE C4 FAIL oracle case=%u entries=%u/%u/%u/%u/%u\n",
                cases, handler_entries, rand_entries, vector_entries,
                rot_entries, apply_entries);
        exit(2);
    }
    expected = fixture;
    uint32_t expected_seed = g_RandomSeed;
    save_c4_gte(&expected_gte);

    fixture = initial;
    g_RandomSeed = initial_seed;
    memset(&gteRegs, 0, sizeof(gteRegs));
    native_rand_calls = 0;
    func_8001FBE4(sprite, 0x000000c4u, ops);
    save_c4_gte(&native_gte);
    if (memcmp(&fixture, &expected, sizeof(fixture)) ||
        g_RandomSeed != expected_seed || native_rand_calls != 1 ||
        memcmp(&native_gte, &expected_gte, sizeof(native_gte))) {
        unsigned first = 0;
        while (first < sizeof(fixture) &&
               ((uint8_t *)&fixture)[first] == ((uint8_t *)&expected)[first])
            ++first;
        fprintf(stderr, "SPRITE C4 FAIL case=%u range=%02x seed=%08x "
                "vx=%08x vy=%08x vz=%08x alias=%u variant=%u diff=%u "
                "seed=%08x/%08x rand_calls=%u\n", cases, range, seed,
                (uint32_t)velocity_x, (uint32_t)velocity_y, (uint32_t)velocity_z,
                alias, variant, first, g_RandomSeed, expected_seed,
                native_rand_calls);
        for (unsigned i = 0; i < 5; ++i)
            fprintf(stderr, " gte_matrix%u=%08x/%08x", i,
                    native_gte.matrix[i], expected_gte.matrix[i]);
        for (unsigned i = 0; i < 3; ++i)
            fprintf(stderr, " gte_ir%u=%08x/%08x gte_mac%u=%08x/%08x", i + 1,
                    native_gte.ir[i], expected_gte.ir[i], i + 1,
                    native_gte.mac[i], expected_gte.mac[i]);
        fprintf(stderr, " gte_flag=%08x/%08x\n", native_gte.flag,
                expected_gte.flag);
        exit(1);
    }
    ++cases;
}

static void select_random_seeds(uint32_t seeds[256])
{
    uint8_t seen[256] = {0};
    unsigned found = 0;
    PcPortMipsBus bus = {.read = rd, .write = wr};
    for (uint32_t candidate = 0; candidate < 65536u && found < 256; ++candidate) {
        g_RandomSeed = candidate;
        PcPortMipsCpu cpu;
        PcPortMipsCpuInit(&cpu, &bus);
        cpu.gpr[31] = 0xfffffffcu;
        assert(PcPortMipsRun(&cpu, 0x8003fa38u, 0xfffffffcu, 100) == PC_PORT_MIPS_HALTED);
        unsigned value = cpu.gpr[2] & 0xffu;
        if (!seen[value]) {
            seen[value] = 1;
            seeds[value] = candidate;
            ++found;
        }
    }
    assert(found == 256);
}

int main(void)
{
    assert((uintptr_t)&fixture + sizeof(fixture) <= UINT32_MAX);
    assert((uintptr_t)&g_RandomSeed + 4 <= UINT32_MAX);
    FILE *image = fopen("disc/SLUS_006.64", "rb");
    assert(image && !fseek(image, 0x800, SEEK_SET));
    assert(fread(ram + 0x10000, 1, sizeof(ram) - 0x10000, image) > 0x48000);
    assert(!fclose(image));

    /* First case makes the current unsupported-command RED explicit. */
    compare(0x40, 1, 0, 0, 0, 0, 0);

    const uint8_t ranges[] = {0, 1, 2, 3, 0x7f, 0x80, 0xfe, 0xff};
    const int32_t edges[] = {0, 1, -1, 32767, -32768, 0x40000000,
                             (int32_t)0xc0000000u, INT32_MAX, INT32_MIN};
    for (unsigned x = 0; x < sizeof(edges) / sizeof(edges[0]); ++x)
        for (unsigned y = 0; y < sizeof(edges) / sizeof(edges[0]); ++y)
            for (unsigned z = 0; z < sizeof(edges) / sizeof(edges[0]); ++z)
                for (unsigned r = 0; r < sizeof(ranges); ++r)
                    compare(ranges[r], 0x12340000u + x * 0x101u + y * 17u + z,
                            edges[x], edges[y], edges[z], 0, (x + y + z + r) & 3);

    uint32_t random_seeds[256];
    select_random_seeds(random_seeds);
    for (unsigned random = 0; random < 256; ++random)
        for (unsigned range = 0; range < 256; ++range)
            compare((uint8_t)range, random_seeds[random], 0x12345678,
                    (int32_t)0x87654321u, -0x40000000, 0,
                    (random + range) & 3);

    const unsigned aliases = 11;
    for (unsigned alias = 1; alias < aliases; ++alias)
        for (unsigned range = 0; range < 256; range += 17)
            compare((uint8_t)range, 0xabcdef01u + alias, INT32_MIN,
                    INT32_MAX, (int32_t)0x80000001u, alias, alias & 3);

    printf("SPRITE C4 PASS %u cases: retail/native dispatcher, first-RNG ordering, "
           "unsigned range, signed velocity edges, matrix/GTE output, complete fixture "
           "and seed/operand aliases\n", cases);
    puts("SPRITE C4 GTE scope: matrix CP2C.r[0..4], IR1..IR3, MAC1..MAC3, FLAG "
         "compared; unrelated/unread GTE registers excluded");
    puts("SPRITE C4 scope: actual retail instructions and native PsyCross GTE backend; "
         "finite velocity/range/seed/alias census; no exhaustive 32-bit input claim.");
    return 0;
}

/* C5 is outside this test: keep its newly linked dependency fail-closed. */
void func_80022CDC(void *sprite) { (void)sprite; __builtin_trap(); }
