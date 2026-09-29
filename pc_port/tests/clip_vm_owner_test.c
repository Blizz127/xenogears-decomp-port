#include <stdio.h>
#include <string.h>
#ifndef OBJECT_OVERLAY_SOURCE
#define OBJECT_OVERLAY_SOURCE "../src/field_object_overlay.c"
#endif
#include OBJECT_OVERLAY_SOURCE
#include "battle_mips_adapter.h"
extern void func_801E7378(s32 value) __attribute__((weak));
extern u32 D_801E85CC __attribute__((weak));

/* Test only the E39F0 working-state wrapper. The prelude and instruction
 * handlers are controlled boundaries; their retail suites are separate. */
static struct {
    u32 original[80], selected[80], pool[8];
    u16 script[8];
} fixture, initial, expected;
static u8 ram[0x200000];
static unsigned dispatches;
static int failures;

int PcPort_FieldClipPrelude(u8* obj, int32_t ticks, uint32_t* stream)
{
    assert(obj == (u8*)fixture.original && ticks == 7);
    *stream = (u32)(uintptr_t)fixture.script;
    return 1;
}

int PcPort_FieldClipControlStep(PcPortFieldClipControl* state)
{
    assert(state->pool == (u8*)fixture.pool);
    assert(state->limit == 3 && state->ticks == 7);
    assert(state->origin == (u8*)fixture.original);
    if (dispatches == 0) return 0;
    assert(state->object == (u8*)fixture.selected);
    state->running = 0;
    return 1;
}

int PcPort_FieldClipDataStep(PcPortFieldClipControl* state)
{
    assert(dispatches++ == 0);
    assert(state->object == (u8*)fixture.original);
    state->object = (u8*)fixture.selected;
    state->stream += 2;
    return 1;
}

static u8* address(u32 a, unsigned w)
{
    if (a >= 0x80000000u && (uint64_t)a + w <= 0x80200000u)
        return ram + (a & 0x1fffff);
    uintptr_t low = (uintptr_t)&fixture;
    if (a >= low && (uint64_t)a + w <= low + sizeof(fixture))
        return (u8*)(uintptr_t)a;
    return NULL;
}
static int rd(void* unused, u32 a, unsigned w, u32* value)
{
    (void)unused;
    u8* p = address(a, w);
    if (!p) return -1;
    *value = 0;
    for (unsigned i = 0; i < w; ++i) *value |= (u32)p[i] << (8 * i);
    return 0;
}
static int wr(void* unused, u32 a, unsigned w, u32 value)
{
    (void)unused;
    u8* p = address(a, w);
    if (!p) return -1;
    for (unsigned i = 0; i < w; ++i) p[i] = (u8)(value >> (8 * i));
    return 0;
}

int main(void)
{
    FILE* disc = fopen("disc/disc1.bin", "rb");
    assert(disc);
    for (unsigned i = 0; i < 25; ++i) {
        assert(fseek(disc, (231361 + i) * 2352L + 24, SEEK_SET) == 0);
        assert(fread(ram + 0x1dc000 + i * 2048, 1, 2048, disc) == 2048);
    }
    assert(fclose(disc) == 0);
    if (!func_801E7378 || &D_801E85CC == NULL) {
        fputs("CLIP VM OWNER FAIL missing retail blend-mode setter/data\n", stderr);
        return 1;
    }
    PcPortMipsBus bus = {.read = rd, .write = wr};
    for (u32 value = 0; value < 65536; ++value) {
        PcPortMipsCpu setter;
        PcPortMipsCpuInit(&setter, &bus);
        setter.gpr[4] = value | 0xa5a50000u;
        setter.gpr[31] = 0xfffffffcu;
        assert(wr(NULL, 0x801e85cc, 4, 0xdeadbeef) == 0);
        assert(PcPortMipsRun(&setter, 0x801e7378, 0xfffffffcu, 10) == PC_PORT_MIPS_HALTED);
        u32 want;
        assert(rd(NULL, 0x801e85cc, 4, &want) == 0);
        D_801E85CC = 0xdeadbeef;
        func_801E7378((s32)(value | 0xa5a50000u));
        if (D_801E85CC != want) {
            fputs("CLIP VM OWNER FAIL retail blend-mode setter\n", stderr);
            return 1;
        }
    }
    memset(&fixture, 0xa5, sizeof(fixture));
    fixture.original[4] = (u32)(uintptr_t)fixture.script;
    fixture.script[0] = 0x011f;
    fixture.script[1] = 0;
    initial = fixture;
    PcPortMipsCpu cpu;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[19] = (u32)(uintptr_t)(fixture.script + 1);
    cpu.gpr[20] = (u32)(uintptr_t)fixture.selected;
    cpu.gpr[29] = 0x801ff000;
    /* Real E5974..E59A0 exit: stop the dispatcher and omit postprocessing. */
    assert(wr(NULL, cpu.gpr[29] + 0xe8, 4, 0) == 0);
    assert(wr(NULL, cpu.gpr[29] + 0xf0, 4, 0) == 0);
    assert(PcPortMipsRun(&cpu, 0x801e5974, 0x801e59a0, 40) == PC_PORT_MIPS_HALTED);
    expected = fixture;
    fixture = initial;
    func_801E39F0((u8*)fixture.original, fixture.pool, 3, 7, 0);
    if (memcmp(&fixture, &expected, sizeof(fixture)) != 0) {
        fputs("CLIP VM OWNER FAIL stream committed to original instead of selected object\n", stderr);
        failures++;
    }
    if (failures) return 1;
    puts("CLIP VM OWNER PASS 65536 mode values, entry identity and retail exit destination; controlled prelude/handlers");
    return 0;
}
