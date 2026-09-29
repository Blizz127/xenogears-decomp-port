#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "battle_mips_adapter.h"

extern void func_8001FBE4(void *, uint32_t, void *);
extern void func_800248D4(void *);
extern int32_t a9_helper_impl(void *, int32_t);

#define GUARD(name) void name(void) { fputs("SPRITE A9 FAIL unexpected callee " #name "\n", stderr); abort(); }
GUARD(AnimScriptTick) GUARD(HeapAlloc) GUARD(HeapChangeCurrentUser) GUARD(HeapFree)
GUARD(func_80039E60)
GUARD(func_8001EE68) GUARD(func_8001D4E8)
int32_t g_WorkListCurTimer; uint8_t D_8006BE10[32];
uint8_t g_PsxRam[0x300000];
GUARD(__wrap_ReadGeomOffset) GUARD(__wrap_ScaleMatrixL) GUARD(func_8001CE74)
GUARD(func_8001D2B0) GUARD(func_80022D44) GUARD(func_80023B84) GUARD(func_80023290)
GUARD(func_80023124) GUARD(func_8001FB30) GUARD(func_800245D8) GUARD(func_8002C3E8)
GUARD(func_8002C59C) GUARD(func_8002C8CC) GUARD(func_8002CB54) GUARD(func_8002CC10)
GUARD(func_8001F6B0) GUARD(func_800B2AEC) GUARD(ApplyMatrixSV) GUARD(MulMatrix0)
GUARD(func_800C11CC)
GUARD(ReadGeomOffset) GUARD(RotMatrix) GUARD(ScaleMatrix) GUARD(ScaleMatrixL)
GUARD(TransMatrix) GUARD(SetTransMatrix) GUARD(SetRotMatrix) GUARD(RotTransSV)
GUARD(rcos) GUARD(rsin)
void MTC2(unsigned int value, int reg) { (void)value; (void)reg; fputs("SPRITE A9 FAIL unexpected MTC2\n", stderr); abort(); }
unsigned int MFC2(int reg) { (void)reg; fputs("SPRITE A9 FAIL unexpected MFC2\n", stderr); abort(); }
int doCOP2(int command) { (void)command; fputs("SPRITE A9 FAIL unexpected doCOP2\n", stderr); abort(); }
uint32_t D_80018644, D_800592E4, D_8004FBB8[8], D_8006F99C[4], D_8006F9AC[4], D_800C3EB0[4];
int32_t D_80059198; uint8_t D_800591AD, D_800591B0, D_800591B3;
int16_t D_800592E8, D_800592EA; int g_cfg_pgxpTextureCorrection, g_WorkListCurTimer;

unsigned candidate_helper_calls;
uintptr_t candidate_helper_sprite;
int32_t candidate_helper_input;

int32_t func_80022CAC(void *sprite, int32_t value) {
    candidate_helper_calls++;
    candidate_helper_sprite = (uintptr_t)sprite;
    candidate_helper_input = value;
    return a9_helper_impl(sprite, value);
}

static uint8_t ram[0x200000];
static struct {
    uint8_t before[0x40];
    uint8_t sprite[0xC0];
    uint8_t after[0x40];
    uint8_t external_ops[4];
} fixture, initial, expected;
static PcPortMipsCpu *active_cpu;
static unsigned retail_handler_calls, retail_helper_calls, cases;
static uint32_t retail_helper_sprite, retail_helper_input;

static uint8_t *mapped(uint32_t address, unsigned width) {
    uintptr_t first = (uintptr_t)&fixture;
    if (address >= first && (uint64_t)address + width <= first + sizeof(fixture))
        return (uint8_t *)(uintptr_t)address;
    if (address >= 0x80000000u && (uint64_t)address + width <= 0x80200000u)
        return ram + (address & 0x1FFFFFu);
    return NULL;
}

static int read_bus(void *unused, uint32_t address, unsigned width, uint32_t *value) {
    uint8_t *p;
    unsigned i;
    (void)unused;
    if (address == 0x80021698u)
        retail_handler_calls++;
    if (address == 0x80022CACu) {
        retail_helper_calls++;
        retail_helper_sprite = active_cpu->gpr[4];
        retail_helper_input = active_cpu->gpr[5];
    }
    p = mapped(address, width);
    if (!p)
        return -1;
    *value = 0;
    for (i = 0; i < width; i++)
        *value |= (uint32_t)p[i] << (i * 8);
    return 0;
}

static int write_bus(void *unused, uint32_t address, unsigned width, uint32_t value) {
    uint8_t *p = mapped(address, width);
    unsigned i;
    (void)unused;
    if (!p)
        return -1;
    for (i = 0; i < width; i++)
        p[i] = (uint8_t)(value >> (i * 8));
    return 0;
}

static void put16(uint8_t *p, uint16_t value) {
    p[0] = (uint8_t)value; p[1] = (uint8_t)(value >> 8);
}

static void put32(uint8_t *p, uint32_t value) {
    p[0] = (uint8_t)value; p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16); p[3] = (uint8_t)(value >> 24);
}

static void compare(uint8_t operand, unsigned variant) {
    static const int16_t scales[] = {0, 1, -1, 0x7FFF, (int16_t)0x8000, 0x1000, -0x1000};
    static const uint16_t helper_scales[] = {0, 1, 0x3FF, 0x400, 0x7FFF, 0xFFFF};
    static const uint32_t positions[] = {0, 1, 0xFFFFu, 0x10000u, 0x7FFFFFFFu,
                                         0x80000000u, 0xFFFFFFFFu, 0x12345678u};
    static const uint32_t flags[] = {0, 4, 8, 0xFFFFFFFFu};
    uint8_t *sprite = fixture.sprite;
    uint8_t *aliases[] = {fixture.external_ops, sprite, sprite + 1, sprite + 0x2C,
                          sprite + 0x2D, sprite + 0x3A, sprite + 0x3B,
                          sprite + 0xAC, sprite + 0xAD};
    uint8_t *ops;
    PcPortMipsBus bus = {0};
    PcPortMipsCpu cpu;
    unsigned i, alias = variant % (sizeof(aliases) / sizeof(aliases[0]));
    uint32_t high = (variant & 1u) ? 0xFFFFFF00u : 0u;

    for (i = 0; i < sizeof(fixture); i++)
        ((uint8_t *)&fixture)[i] = (uint8_t)(i * 37u + variant * 53u + operand);
    put32(sprite, positions[variant % (sizeof(positions) / sizeof(positions[0]))]);
    put16(sprite + 0x2C, (uint16_t)scales[variant % (sizeof(scales) / sizeof(scales[0]))]);
    put16(sprite + 0x3A, helper_scales[variant % (sizeof(helper_scales) / sizeof(helper_scales[0]))]);
    put32(sprite + 0xAC, flags[variant % (sizeof(flags) / sizeof(flags[0]))]);
    ops = aliases[alias];
    ops[0] = operand;
    initial = fixture;

    bus.read = read_bus;
    bus.write = write_bus;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = (uint32_t)(uintptr_t)sprite;
    cpu.gpr[5] = high | 0xA9u;
    cpu.gpr[6] = (uint32_t)(uintptr_t)ops;
    cpu.gpr[29] = 0x801FFF00u;
    cpu.gpr[31] = 0xFFFFFFFCu;
    retail_handler_calls = retail_helper_calls = 0;
    retail_helper_sprite = retail_helper_input = 0;
    active_cpu = &cpu;
    if (PcPortMipsRun(&cpu, 0x8001FBE4u, 0xFFFFFFFCu, 1000) != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "A9 ORACLE ERROR case=%u pc=%08x %s\n", cases, cpu.pc, cpu.error);
        exit(2);
    }
    active_cpu = NULL;
    if (retail_handler_calls != 1 || retail_helper_calls != 1) {
        fprintf(stderr, "A9 ORACLE BOUNDARY ERROR case=%u handler=%u helper=%u\n",
                cases, retail_handler_calls, retail_helper_calls);
        exit(2);
    }
    expected = fixture;

    fixture = initial;
    sprite = fixture.sprite;
    aliases[0] = fixture.external_ops; aliases[1] = sprite; aliases[2] = sprite + 1;
    aliases[3] = sprite + 0x2C; aliases[4] = sprite + 0x2D;
    aliases[5] = sprite + 0x3A; aliases[6] = sprite + 0x3B;
    aliases[7] = sprite + 0xAC; aliases[8] = sprite + 0xAD;
    ops = aliases[alias];
    candidate_helper_calls = 0;
    candidate_helper_sprite = 0;
    candidate_helper_input = 0;
    func_8001FBE4(sprite, high | 0xA9u, ops);
    if (memcmp(&fixture, &expected, sizeof(fixture)) != 0 ||
        candidate_helper_calls != 1 ||
        candidate_helper_sprite != (uintptr_t)retail_helper_sprite ||
        (uint32_t)candidate_helper_input != retail_helper_input) {
        unsigned diff = 0;
        while (diff < sizeof(fixture) && ((uint8_t *)&fixture)[diff] ==
               ((uint8_t *)&expected)[diff]) diff++;
        fprintf(stderr, "SPRITE A9 DIFFERENTIAL FAIL case=%u operand=%02x variant=%u alias=%u "
                "diff=%u helper=%u ptr=%08x/%08x input=%08x/%08x\n",
                cases, operand, variant, alias, diff, candidate_helper_calls,
                (unsigned)candidate_helper_sprite, retail_helper_sprite,
                (uint32_t)candidate_helper_input, retail_helper_input);
        exit(1);
    }
    cases++;
}

static void verify_pc_advance(void) {
    uint8_t *sprite = fixture.sprite;
    uint8_t *script = fixture.external_ops;
    PcPortMipsBus bus = {0};
    PcPortMipsCpu cpu;

    memset(&fixture, 0, sizeof(fixture));
    script[0] = 0xA9;
    script[1] = 0x21;
    script[2] = 0x86; /* With negative +0x10, retail waits here. */
    put32(sprite + 0x10, 0x80000000u);
    put16(sprite + 0x2C, 0x1000);
    put16(sprite + 0x3A, 0);
    put32(sprite + 0x64, (uint32_t)(uintptr_t)script);
    initial = fixture;
    ram[0x591AD] = 0;
    bus.read = read_bus;
    bus.write = write_bus;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = (uint32_t)(uintptr_t)sprite;
    cpu.gpr[29] = 0x801FFF00u;
    cpu.gpr[31] = 0xFFFFFFFCu;
    retail_handler_calls = retail_helper_calls = 0;
    active_cpu = &cpu;
    if (PcPortMipsRun(&cpu, 0x800248D4u, 0xFFFFFFFCu, 3000) != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "A9 PC ORACLE ERROR pc=%08x %s\n", cpu.pc, cpu.error);
        exit(2);
    }
    active_cpu = NULL;
    if (retail_handler_calls != 1 || retail_helper_calls != 1 ||
        *(uint32_t *)(void *)(sprite + 0x64) != (uint32_t)(uintptr_t)script + 2u ||
        *(uint16_t *)(void *)(sprite + 0x9E) != 1) {
        fprintf(stderr, "SPRITE A9 RETAIL PC ADVANCE FAIL pcword=%08x expected=%08x timer=%u calls=%u/%u\n",
                *(uint32_t *)(void *)(sprite + 0x64),
                (uint32_t)(uintptr_t)script + 2u,
                *(uint16_t *)(void *)(sprite + 0x9E),
                retail_handler_calls, retail_helper_calls);
        exit(1);
    }
    expected = fixture;
    puts("SPRITE A9 RETAIL PC ADVANCE PASS stride=2 next=0x86 wait");

    fixture = initial;
    sprite = fixture.sprite;
    script = fixture.external_ops;
    D_800591AD = 0;
    g_WorkListCurTimer = 0;
    candidate_helper_calls = 0;
    candidate_helper_sprite = 0;
    candidate_helper_input = 0;
    func_800248D4(sprite);
    if (memcmp(&fixture, &expected, sizeof(fixture)) != 0 ||
        candidate_helper_calls != 1 ||
        candidate_helper_sprite != (uintptr_t)retail_helper_sprite ||
        (uint32_t)candidate_helper_input != retail_helper_input ||
        *(uint32_t *)(void *)(sprite + 0x64) != (uint32_t)(uintptr_t)script + 2u ||
        *(uint16_t *)(void *)(sprite + 0x9E) != 1) {
        unsigned diff = 0;
        while (diff < sizeof(fixture) && ((uint8_t *)&fixture)[diff] ==
               ((uint8_t *)&expected)[diff]) diff++;
        fprintf(stderr, "SPRITE A9 NATIVE OUTER FAIL diff=%u pcword=%08x expected=%08x "
                "timer=%u helper=%u ptr=%08x/%08x input=%08x/%08x\n",
                diff, *(uint32_t *)(void *)(sprite + 0x64),
                (uint32_t)(uintptr_t)script + 2u,
                *(uint16_t *)(void *)(sprite + 0x9E), candidate_helper_calls,
                (unsigned)candidate_helper_sprite, retail_helper_sprite,
                (uint32_t)candidate_helper_input, retail_helper_input);
        exit(1);
    }
    printf("SPRITE A9 NATIVE OUTER PASS actual func_800248D4 stride=2 "
           "next=0x86 wait x=%08x\n", *(uint32_t *)(void *)sprite);
}

int main(void) {
    FILE *image;
    unsigned operand, variant;
    assert((uintptr_t)&fixture + sizeof(fixture) <= UINT32_MAX);
    image = fopen("disc/SLUS_006.64", "rb");
    assert(image && !fseek(image, 0x800, SEEK_SET));
    assert(fread(ram + 0x10000, 1, sizeof(ram) - 0x10000, image) > 0x22000);
    assert(!fclose(image));
    verify_pc_advance();
    for (operand = 0; operand < 256; operand++)
        for (variant = 0; variant < 18; variant++)
            compare((uint8_t)operand, variant);
    printf("SPRITE A9 DIFFERENTIAL PASS cases=%u\n", cases);
    return 0;
}

/* C5 is outside this test: keep its newly linked dependency fail-closed. */
void func_80022CDC(void *sprite) { (void)sprite; __builtin_trap(); }
