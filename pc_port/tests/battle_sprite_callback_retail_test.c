#define _GNU_SOURCE
#define PcPortMipsRun SpriteCallbackTestMipsRun
#include "../src/battle_mips_runtime.c"
#undef PcPortMipsRun

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int PcPortMipsRun(PcPortMipsCpu *, uint32_t, uint32_t, uint64_t);
extern void func_80021BF8(void *, int32_t);

int SpriteCallbackTestMipsRun(PcPortMipsCpu *cpu, uint32_t entry,
                              uint32_t halt, uint64_t limit)
{
    return PcPortMipsRun(cpu, entry, halt, limit);
}

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];
int g_GPUDisabledState;
/* Runtime diagnostics/pad-pump hooks referenced by battle_mips_runtime.c.
 * Inert here: this path presents no frames and reads no pads. */
unsigned g_PcPortPresentedFrames;
void PcPort_PadVblankPump(void) {}

/* The callback fixture never reaches graphics. These are fail-fast leaves for
 * unrelated runtime bridge branches retained by the production source. */
char PsyX_BeginScene(void) { abort(); }
void ClearSplits(void) { abort(); }
void DrawAllSplits(void) { abort(); }
void ParsePrimitivesLinkedList(u_long *p, int s) { (void)p; (void)s; abort(); }
unsigned MFC2(int r) { (void)r; abort(); }
unsigned CFC2(int r) { (void)r; abort(); }
void MTC2(unsigned v, int r) { (void)v; (void)r; abort(); }
void CTC2(unsigned v, int r) { (void)v; (void)r; abort(); }
int doCOP2(int op) { (void)op; abort(); }

#define SLUS_BASE 0x8000f800u
#define SETTER_PC 0x80021bf8u
#define CALLBACK_PC 0x801e9400u
#define SPRITE_PC 0x80180000u
#define STACK_PC 0x801ff000u
#define HALT_PC 0xfffffffcu

static uint8_t host_sprite[0x100];
static void native_callback(void);

static void load_actual_setter(void)
{
    FILE *file = fopen("disc/SLUS_006.64", "rb");
    assert(file != NULL);
    assert(!fseek(file, (long)(SETTER_PC - SLUS_BASE), SEEK_SET));
    assert(fread(PSX_ADDR(SETTER_PC), 1, 8, file) == 8);
    assert(!fclose(file));
    /* The same pinned retail setter is a deliberately simple callback body.
     * Relocate it to a controlled guest-RAM callback address so the dispatcher
     * executes real MIPS instructions; this does not claim opening-code
     * identity for that address. */
    memcpy(PSX_ADDR(CALLBACK_PC), PSX_ADDR(SETTER_PC), 8);
}

static int oracle_bridge(void *opaque, PcPortMipsCpu *cpu, uint32_t target)
{
    (void)opaque;
    (void)cpu;
    /* For the direct retail setter oracle, fetch 80021BF8 from SLUS bytes.
     * The production bridge handles this address only in the separate native
     * boundary check below. */
    if (target >= SETTER_PC && target < SETTER_PC + 8u)
        return 0;
    return runtime_bridge(opaque, cpu, target);
}

static int run_retail_setter(uint32_t callback)
{
    PcPortMipsBus bus = {.opaque = &g_BattleRuntime,
                         .read = runtime_read,
                         .write = runtime_write,
                         .bridge = oracle_bridge};
    PcPortMipsCpu cpu;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = SPRITE_PC;
    cpu.gpr[5] = callback;
    cpu.gpr[29] = STACK_PC;
    cpu.gpr[31] = HALT_PC;
    return PcPortMipsRun(&cpu, SETTER_PC, HALT_PC, 64);
}

static int run_native_bridge(uint32_t callback, uint32_t *stored)
{
    PcPortMipsCpu cpu;
    PcPortMipsCpuInit(&cpu, NULL);
    cpu.gpr[4] = SPRITE_PC;
    cpu.gpr[5] = callback;
    if (runtime_bridge_call(&g_BattleRuntime, &cpu, SETTER_PC) != 1)
        return -1;
    /* a0 is a guest sprite address, so the production bridge must translate
     * it to the emulated-RAM host object before calling the native setter. */
    memcpy(stored, PSX_ADDR(SPRITE_PC) + 0x68, sizeof(*stored));
    return 0;
}

static int check_direct_setter(uint32_t callback)
{
    uint32_t expected;
    memset(PSX_ADDR(SPRITE_PC), 0xa5, 0x100);
    expected = callback;
    if (run_retail_setter(callback) != PC_PORT_MIPS_HALTED)
        return 1;
    if (memcmp(PSX_ADDR(SPRITE_PC) + 0x68, &expected, sizeof(expected)) != 0)
        return 2;
    return 0;
}

static int check_bridge_and_callback(uint32_t callback)
{
    uint32_t stored;
    uint32_t guest_roundtrip;
    uint32_t expected = callback;
    memset(PSX_ADDR(SPRITE_PC), 0x5a, 0x100);
    if (run_native_bridge(callback, &stored) != 0)
        return 3;
    if (stored != expected) {
        fprintf(stderr,
                "CALLBACK RED classification omitted: guest callback %08x "
                "stored as %08x\n", callback, stored);
        return 4;
    }
    guest_roundtrip = stored;
    /* Null and an already-packed native callback word must also survive the
     * same production bridge call. */
    if (run_native_bridge(0, &stored) != 0 || stored != 0)
        return 5;
    uint32_t native_word = (uint32_t)(uintptr_t)native_callback;
    if (run_native_bridge(native_word, &stored) != 0 ||
        stored != native_word)
        return 5;
    memset(PSX_ADDR(SPRITE_PC), 0, 0x100);
    *(uint32_t *)(PSX_ADDR(SPRITE_PC) + 0x68) = 0xdeadbeefu;
    if (run_guest_callback(&g_BattleRuntime, guest_roundtrip,
                           PSX_ADDR(SPRITE_PC)) != 0)
        return 6;
    if (*(uint32_t *)(PSX_ADDR(SPRITE_PC) + 0x68) != 0)
        return 7;
    return 0;
}

static void native_callback(void) { }

int main(void)
{
    const uint32_t guest_callback = CALLBACK_PC;
    uint32_t native_callback_word = (uint32_t)(uintptr_t)native_callback;
    uint32_t stored;
    int rc;

    assert((uintptr_t)g_PsxRam + PSX_RAM_SIZE <= UINTPTR_MAX);
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    load_actual_setter();
    initialize_runtime(&g_BattleRuntime);

    rc = check_direct_setter(guest_callback);
    if (rc != 0) {
        fprintf(stderr, "CALLBACK FAIL retail setter oracle rc=%d\n", rc);
        return 1;
    }
    rc = check_bridge_and_callback(guest_callback);
    if (rc != 0)
        return 1;

    memset(host_sprite, 0, sizeof(host_sprite));
    func_80021BF8(host_sprite, 0);
    if (*(uint32_t *)(host_sprite + 0x68) != 0)
        return 1;
    func_80021BF8(host_sprite, (int32_t)native_callback_word);
    memcpy(&stored, host_sprite + 0x68, sizeof(stored));
    if (stored != native_callback_word) {
        fprintf(stderr, "CALLBACK FAIL native callback round-trip\n");
        return 1;
    }
    if (!is_callback_argument("func_80021BF8", 1) ||
        is_callback_argument("func_80021BF8", 0)) {
        fprintf(stderr, "CALLBACK FAIL production classification\n");
        return 1;
    }
    puts("CALLBACK PASS retail setter, guest callback preservation, data translation, "
         "zero/native callback and relocated retail callback execution");
    return 0;
}
