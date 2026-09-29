#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "battle_mips_adapter.h"

extern void func_8001FBE4(void *, uint32_t, void *);
extern void ba_leaf_impl(void *);
static unsigned native_leaf_entries;
static uint32_t native_leaf_arg;
static uint8_t native_leaf_entry[0xc0];
void func_8001F6B0(void *sprite)
{
    ++native_leaf_entries;
    native_leaf_arg = (uint32_t)(uintptr_t)sprite;
    memcpy(native_leaf_entry, sprite, sizeof(native_leaf_entry));
    ba_leaf_impl(sprite);
}

#define GUARD(name) void name(void) { fputs("SPRITE 91 FAIL unexpected callee " #name "\n", stderr); abort(); }
GUARD(func_800B2AEC)
GUARD(func_80039E60)
/* Guest-RAM backing for the opcode 0xB0 selector read; never touched here. */
uint8_t g_PsxRam[0x300000];
GUARD(AnimScriptTick) GUARD(HeapAlloc) GUARD(HeapChangeCurrentUser)
GUARD(HeapFree) GUARD(__wrap_ReadGeomOffset) GUARD(__wrap_ScaleMatrixL)
GUARD(func_8001CE74) GUARD(func_8001D2B0) GUARD(func_8001D4E8) GUARD(func_8001EE68) GUARD(func_80022D44)
GUARD(func_80023B84) GUARD(func_80022CAC) GUARD(func_80023124)
GUARD(func_80023290)
GUARD(func_8001FB30) GUARD(func_800245D8) GUARD(func_8002C3E8)
GUARD(func_8002C59C) GUARD(func_8002C8CC) GUARD(func_8002CB54)
GUARD(func_8002CC10)
uint32_t D_80018644, D_800592E4, D_8004FBB8[8], D_8006F99C[4], D_8006F9AC[4], D_800C3EB0[4];
int32_t g_WorkListCurTimer;
uint8_t D_8006BE10[32];
int32_t D_80059198; uint8_t D_800591AD, D_800591B0, D_800591B3;
int16_t D_800592E8, D_800592EA; int g_cfg_pgxpTextureCorrection;
extern uint32_t g_RandomSeed; extern int __real_rand(void);
int __wrap_rand(void) { return __real_rand(); }

static uint8_t ram[0x200000];
static struct { uint8_t before[0x20], sprite[0xc0], after[0x20], base[0x80], prim[0x18 * 64]; } fixture, initial, expected;
static uint8_t retail_leaf_entry[0xc0];
static unsigned cases, handler_entries, common_entries, leaf_entries;
static uint32_t retail_leaf_arg;

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
    handler_entries += a == 0x80020de8u;
    *v = 0; for (unsigned i = 0; i < w; ++i) *v |= (uint32_t)p[i] << (i * 8); return 0;
}
static int wr(void *u, uint32_t a, unsigned w, uint32_t v)
{
    (void)u; uint8_t *p = address(a, w); if (!p) return -1;
    for (unsigned i = 0; i < w; ++i) p[i] = (uint8_t)(v >> (i * 8)); return 0;
}
static int bridge(void *u, PcPortMipsCpu *cpu, uint32_t target)
{
    (void)u;
    if (target == 0x80020e04u) { ++common_entries; return 0; }
    if (target == 0x8001f6b0u) {
        uint8_t *p = address(cpu->gpr[4], sizeof(retail_leaf_entry)); if (!p) return -1;
        ++leaf_entries; retail_leaf_arg = cpu->gpr[4]; memcpy(retail_leaf_entry, p, sizeof(retail_leaf_entry)); return 0;
    }
    return 0;
}
static void put32(uint8_t *p, uint32_t v) { memcpy(p, &v, 4); }

static void compare(unsigned color, unsigned mode, unsigned frames)
{
    for (unsigned i = 0; i < sizeof(fixture); ++i) ((uint8_t *)&fixture)[i] = (uint8_t)(i * 37u + color);
    uint8_t *p = fixture.sprite;
    put32(p + 0x28, color * 0x01010101u);
    put32(p + 0x3c, (mode & 3u) | 0x000000a0u);
    p[0x2b] = 0xff;
    p[0x40] = (uint8_t)((frames & 63u) << 2);
    put32(p + 0x20, (uint32_t)(uintptr_t)fixture.base);
    put32(p + 0x24, (uint32_t)(uintptr_t)fixture.base);
    put32(fixture.base + 0x30, (uint32_t)(uintptr_t)fixture.prim);
    initial = fixture;
    handler_entries = common_entries = leaf_entries = 0; retail_leaf_arg = 0;
    PcPortMipsBus bus = {.read = rd, .write = wr, .bridge = bridge}; PcPortMipsCpu cpu;
    PcPortMipsCpuInit(&cpu, &bus); cpu.gpr[4] = (uint32_t)(uintptr_t)p; cpu.gpr[5] = 0x91; cpu.gpr[6] = 0;
    cpu.gpr[29] = 0x801fff00u; cpu.gpr[31] = 0xfffffffcu;
    if (PcPortMipsRun(&cpu, 0x8001fbe4u, 0xfffffffcu, 2000) != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "SPRITE 91 FAIL retail pc=%08x: %s\n", cpu.pc, cpu.error); exit(2);
    }
    if (handler_entries != 1 || common_entries != 1 || leaf_entries != 1 || retail_leaf_arg != (uint32_t)(uintptr_t)p) {
        fprintf(stderr, "SPRITE 91 FAIL oracle entries=%u/%u/%u\n", handler_entries, common_entries, leaf_entries); exit(2);
    }
    expected = fixture;
    fixture = initial; native_leaf_entries = 0; native_leaf_arg = 0; memset(native_leaf_entry, 0, sizeof(native_leaf_entry));
    func_8001FBE4(p, 0x91, NULL);
    if (memcmp(&fixture, &expected, sizeof(fixture)) ||
        native_leaf_entries != 1 || native_leaf_arg != (uint32_t)(uintptr_t)p ||
        memcmp(native_leaf_entry, retail_leaf_entry, sizeof(native_leaf_entry))) {
        fprintf(stderr, "SPRITE 91 FAIL case=%u color=%02x mode=%u frames=%u diff=%u leaf=%u/%u\n",
                cases, color, mode, frames, memcmp(&fixture, &expected, sizeof(fixture)), native_leaf_entries, leaf_entries); exit(1);
    }
    assert(p[0x2b] == (uint8_t)(initial.sprite[0x2b] & ~1u));
    ++cases;
}

int main(void)
{
    const char *quick = getenv("SPRITE_91_QUICK");
    assert((uintptr_t)&fixture + sizeof(fixture) <= UINT32_MAX);
    FILE *f = fopen("disc/SLUS_006.64", "rb"); assert(f && !fseek(f, 0x800, SEEK_SET));
    assert(fread(ram + 0x10000, 1, sizeof(ram) - 0x10000, f) > 0x48000); assert(!fclose(f));
    if (quick != NULL) {
        compare(0xa5, 1, 63);
        printf("SPRITE 91 QUICK PASS %u cases\n", cases);
        return 0;
    }
    for (unsigned color = 0; color < 256; ++color)
        for (unsigned mode = 0; mode < 4; ++mode)
            for (unsigned frames = 0; frames < 64; ++frames)
                compare(color, mode, frames);
    printf("SPRITE 91 PASS %u cases: all color bytes, modes 0..3, frame counts 0..63, "
           "NULL operand, actual leaf primitive color/tpage effects\n", cases);
    puts("SPRITE 91 scope: actual retail/native dispatcher and leaf with entry-state "
         "observation; no displayed-rendering claim.");
    return 0;
}

/* C5 is outside this test: keep its newly linked dependency fail-closed. */
void func_80022CDC(void *sprite) { (void)sprite; __builtin_trap(); }
