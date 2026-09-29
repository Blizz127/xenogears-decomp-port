#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "battle_mips_adapter.h"

extern void func_8001FBE4(void *, uint32_t, void *);
extern int32_t aa_helper_impl(void *, int32_t);

#define GUARD(name) void name(void) { fputs("SPRITE AA FAIL unexpected callee " #name "\n", stderr); abort(); }
GUARD(AnimScriptTick) GUARD(HeapAlloc) GUARD(HeapChangeCurrentUser) GUARD(HeapFree)
GUARD(func_80039E60)
GUARD(func_8001EE68) GUARD(func_8001D4E8)
int32_t g_WorkListCurTimer; uint8_t D_8006BE10[32];
/* Guest-RAM backing for the opcode 0xB0 selector read; never touched here. */
uint8_t g_PsxRam[0x300000];
GUARD(__wrap_ReadGeomOffset) GUARD(__wrap_ScaleMatrixL) GUARD(func_8001CE74)
GUARD(func_8001D2B0) GUARD(func_80022D44) GUARD(func_80023B84) GUARD(func_80023290)
GUARD(func_80023124) GUARD(func_8001FB30) GUARD(func_800245D8) GUARD(func_8002C3E8)
GUARD(func_8002C59C) GUARD(func_8002C8CC) GUARD(func_8002CB54) GUARD(func_8002CC10)
GUARD(func_8001F6B0) GUARD(func_800B2AEC) GUARD(ApplyMatrixSV) GUARD(MulMatrix0)
GUARD(ReadGeomOffset) GUARD(RotMatrix) GUARD(ScaleMatrix) GUARD(ScaleMatrixL)
GUARD(TransMatrix) GUARD(SetTransMatrix) GUARD(SetRotMatrix) GUARD(RotTransSV)
GUARD(rcos) GUARD(rsin)
void MTC2(unsigned int value, int reg) { (void)value; (void)reg; fputs("SPRITE AA FAIL unexpected MTC2\n", stderr); abort(); }
unsigned int MFC2(int reg) { (void)reg; fputs("SPRITE AA FAIL unexpected MFC2\n", stderr); abort(); }
int doCOP2(int command) { (void)command; fputs("SPRITE AA FAIL unexpected doCOP2\n", stderr); abort(); }
uint32_t D_80018644, D_800592E4, D_8004FBB8[8], D_8006F99C[4], D_8006F9AC[4], D_800C3EB0[4];
int32_t D_80059198; uint8_t D_800591AD, D_800591B0, D_800591B3;
int16_t D_800592E8, D_800592EA; int g_cfg_pgxpTextureCorrection;
extern uint32_t g_RandomSeed; extern int __real_rand(void);
int __wrap_rand(void) { return __real_rand(); }

static uint8_t ram[0x200000];
static struct { uint8_t pre[0x200], sprite[0xc0], ops[4], base[0x80]; } fixture, initial, expected;
static unsigned cases, retail_helper_entries, native_helper_entries;
static uint32_t retail_helper_args[2], native_helper_args[2];
static uint32_t retail_entry_pos, native_entry_pos;

int32_t func_80022CAC(void *sprite, int32_t value)
{
    ++native_helper_entries;
    native_helper_args[0] = (uint32_t)(uintptr_t)sprite;
    native_helper_args[1] = (uint32_t)value;
    memcpy(&native_entry_pos, (uint8_t *)sprite + 4, 4);
    return aa_helper_impl(sprite, value);
}

static uint8_t *address(uint32_t a, unsigned w)
{
    uintptr_t first = (uintptr_t)&fixture;
    if (a >= first && (uint64_t)a + w <= first + sizeof(fixture)) return (uint8_t *)(uintptr_t)a;
    if (a >= 0x80000000u && (uint64_t)a + w <= 0x80200000u) return ram + (a & 0x1fffffu);
    return NULL;
}
static int rd(void *u, uint32_t a, unsigned w, uint32_t *v)
{
    (void)u; uint8_t *p = address(a, w); if (!p) return -1;
    *v = 0; for (unsigned i = 0; i < w; ++i) *v |= (uint32_t)p[i] << (8 * i); return 0;
}
static int wr(void *u, uint32_t a, unsigned w, uint32_t v)
{
    (void)u; uint8_t *p = address(a, w); if (!p) return -1;
    for (unsigned i = 0; i < w; ++i) p[i] = (uint8_t)(v >> (8 * i)); return 0;
}
static int bridge(void *u, PcPortMipsCpu *c, uint32_t target)
{
    (void)u; if (target != 0x80022cacu) return 0;
    ++retail_helper_entries; retail_helper_args[0] = c->gpr[4]; retail_helper_args[1] = c->gpr[5];
    uint8_t *entry = address(c->gpr[4] + 4, 4); if (!entry) return -1; memcpy(&retail_entry_pos, entry, 4);
    return 0;
}
static void put16(uint8_t *p, uint16_t v) { memcpy(p, &v, 2); }
static void put32(uint8_t *p, uint32_t v) { memcpy(p, &v, 4); }

static void compare(uint8_t operand, int16_t scale, uint16_t timer, uint32_t position, unsigned alias)
{
    for (unsigned i = 0; i < sizeof(fixture); ++i) ((uint8_t *)&fixture)[i] = (uint8_t)(i * 37u + operand * 13u + (uint16_t)scale);
    uint8_t *p = fixture.sprite;
    uint8_t *source = alias == 0 ? fixture.ops : (alias == 1 ? p + 0x2c : (alias == 2 ? p + 0x3a : p + 0x04));
    put32(p + 0x04, position); put16(p + 0x2c, (uint16_t)scale); put16(p + 0x3a, timer);
    fixture.ops[0] = operand;
    if (alias == 1) put16(p + 0x2c, operand);
    if (alias == 2) put16(p + 0x3a, operand);
    initial = fixture; retail_helper_entries = native_helper_entries = 0; retail_entry_pos = native_entry_pos = 0;
    memset(retail_helper_args, 0, sizeof(retail_helper_args)); memset(native_helper_args, 0, sizeof(native_helper_args));
    PcPortMipsBus bus = {.read = rd, .write = wr, .bridge = bridge}; PcPortMipsCpu c; PcPortMipsCpuInit(&c, &bus);
    c.gpr[4] = (uint32_t)(uintptr_t)p; c.gpr[5] = 0xaau; c.gpr[6] = (uint32_t)(uintptr_t)source;
    c.gpr[29] = 0x801fff00u; c.gpr[31] = 0xfffffffcu;
    if (PcPortMipsRun(&c, 0x8001fbe4u, 0xfffffffcu, 4096) != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "SPRITE AA oracle failure operand=%02x scale=%04x timer=%04x alias=%u: %s\n", operand, (uint16_t)scale, timer, alias, c.error); exit(2);
    }
    expected = fixture; fixture = initial; func_8001FBE4(p, 0xaau, source);
    if (memcmp(&fixture, &expected, sizeof(fixture)) || retail_helper_entries != 1 || native_helper_entries != 1 || memcmp(retail_helper_args, native_helper_args, sizeof(retail_helper_args)) || retail_entry_pos != native_entry_pos) {
        fprintf(stderr, "SPRITE AA FAIL case=%u operand=%02x scale=%04x timer=%04x alias=%u helper=%u/%u args=%08x/%08x\n", cases, operand, (uint16_t)scale, timer, alias, retail_helper_entries, native_helper_entries, retail_helper_args[1], native_helper_args[1]); exit(1);
    }
    ++cases;
}

int main(void)
{
    assert((uintptr_t)&fixture + sizeof(fixture) <= UINT32_MAX);
    FILE *f = fopen("disc/SLUS_006.64", "rb"); assert(f && !fseek(f, 0x800, SEEK_SET));
    assert(fread(ram + 0x10000, 1, sizeof(ram) - 0x10000, f) > 0x48000); assert(!fclose(f));
    if (getenv("SPRITE_AA_QUICK")) {
        unsigned operand = getenv("SPRITE_AA_OPERAND") ? strtoul(getenv("SPRITE_AA_OPERAND"), 0, 0) : 1;
        unsigned scale = getenv("SPRITE_AA_SCALE") ? strtoul(getenv("SPRITE_AA_SCALE"), 0, 0) : 0x1000;
        unsigned timer = getenv("SPRITE_AA_TIMER") ? strtoul(getenv("SPRITE_AA_TIMER"), 0, 0) : 0;
        unsigned alias = getenv("SPRITE_AA_ALIAS") ? strtoul(getenv("SPRITE_AA_ALIAS"), 0, 0) : 0;
        unsigned position = getenv("SPRITE_AA_POSITION") ? strtoul(getenv("SPRITE_AA_POSITION"), 0, 0) : 0x00120000;
        compare((uint8_t)operand, (int16_t)(uint16_t)scale, (uint16_t)timer, position, alias); printf("SPRITE AA QUICK PASS %u cases\n", cases); return 0;
    }
    static const uint16_t edge_scales[] = {0, 1, 0x7ff, 0x800, 0xfff, 0x1000, 0x1fff, 0x4000, 0x7fff, 0x8000, 0x8001, 0xbfff, 0xc000, 0xf000, 0xffff};
    static const uint16_t edge_timers[] = {0, 1, 2, 0x3ff, 0x400, 0x401, 0x7fff, 0x8000, 0xffff};
    static const uint32_t edge_positions[] = {0, 1, 0xffff, 0x00010000, 0x00120000, 0x7fff0000, 0x80000000, 0xffffffff};
    for (unsigned alias = 0; alias < 4; ++alias)
        for (unsigned operand = 0; operand < 256; ++operand) compare((uint8_t)operand, 0x1000, 0, 0x00120000, alias);
    for (unsigned scale = 0; scale < 65536; ++scale) compare(0x7f, (int16_t)scale, 0x400, 0x00120000, 0);
    for (unsigned timer = 0; timer < 65536; ++timer) compare(0x80, (int16_t)0x1800, (uint16_t)timer, 0x00120000, 0);
    for (unsigned i = 0; i < sizeof(edge_scales) / sizeof(edge_scales[0]); ++i)
        for (unsigned j = 0; j < sizeof(edge_timers) / sizeof(edge_timers[0]); ++j)
            for (unsigned operand = 0; operand < 16; ++operand)
                for (unsigned k = 0; k < sizeof(edge_positions) / sizeof(edge_positions[0]); ++k)
                    for (unsigned alias = 0; alias < 4; ++alias) compare((uint8_t)(operand * 17u), (int16_t)edge_scales[i], edge_timers[j], edge_positions[k], alias);
    printf("SPRITE AA PASS %u cases: full operand/scale/timer domains, signed fixed-point rounding, helper ordering, and operand aliases\n", cases); return 0;
}

/* C5 is outside this test: keep its newly linked dependency fail-closed. */
void func_80022CDC(void *sprite) { (void)sprite; __builtin_trap(); }
