#define _GNU_SOURCE
#define PcPortMipsRun NativeCallbackMapTestMipsRun
#include "../src/battle_mips_runtime.c"
#undef PcPortMipsRun

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int PcPortMipsRun(PcPortMipsCpu *, uint32_t, uint32_t, uint64_t);

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];
int g_GPUDisabledState;
/* Runtime diagnostics/pad-pump hooks referenced by battle_mips_runtime.c.
 * Inert here: this path presents no frames and reads no pads. */
unsigned g_PcPortPresentedFrames;
void PcPort_PadVblankPump(void) {}

char PsyX_BeginScene(void) { abort(); }
void ClearSplits(void) { abort(); }
void DrawAllSplits(void) { abort(); }
void ParsePrimitivesLinkedList(u_long *p, int s) { (void)p; (void)s; abort(); }
unsigned MFC2(int r) { (void)r; abort(); }
unsigned CFC2(int r) { (void)r; abort(); }
void MTC2(unsigned v, int r) { (void)v; (void)r; abort(); }
void CTC2(unsigned v, int r) { (void)v; (void)r; abort(); }
int doCOP2(int op) { (void)op; abort(); }

static unsigned callback_calls;
static unsigned callback_kind;
static void *callback_argument;
extern int reject_generated_stub;

/* Exported spy name is resolved by the generated retail map. Its body only
 * observes the ABI boundary; it does not stand in for retail timer logic. */
void func_80022DF4(void *task)
{
    ++callback_calls;
    callback_kind = 1;
    callback_argument = task;
}

void func_80022E8C(void *task)
{
    ++callback_calls;
    callback_kind = 2;
    callback_argument = task;
}

void func_80022EB8(void *task)
{
    ++callback_calls;
    callback_kind = 3;
    callback_argument = task;
}

int NativeCallbackMapTestMipsRun(PcPortMipsCpu *cpu, uint32_t entry,
                                 uint32_t halt, uint64_t limit)
{
    return PcPortMipsRun(cpu, entry, halt, limit);
}

static int call_host_timer(uint32_t target, uint32_t guest_data)
{
    PcPortMipsCpu cpu;
    PcPortMipsCpuInit(&cpu, NULL);
    cpu.gpr[4] = guest_data;
    cpu.gpr[31] = 0xfffffffcu;
    return runtime_bridge_call(&g_BattleRuntime, &cpu, target);
}

int main(void)
{
    const struct {
        uint32_t address;
        void *host;
        const char *name;
    } timers[] = {
        {0x80022df4u, (void *)func_80022DF4, "func_80022DF4"},
        {0x80022e8cu, (void *)func_80022E8C, "func_80022E8C"},
        {0x80022eb8u, (void *)func_80022EB8, "func_80022EB8"},
    };
    const ResolvedFunction *resolved;
    const uint32_t guest_data = 0x80180000u;

    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    initialize_runtime(&g_BattleRuntime);
    for (unsigned i = 0; i < sizeof(timers) / sizeof(timers[0]); ++i) {
        resolved = find_function(&g_BattleRuntime, timers[i].address);
        if (resolved == NULL || resolved->host != timers[i].host ||
            strcmp(resolved->name, timers[i].name) != 0) {
            fprintf(stderr,
                    "NATIVE CALLBACK RED missing map entry for %08x (%s)\n",
                    timers[i].address, timers[i].name);
            return 1;
        }
    }
    for (unsigned i = 0; i < sizeof(timers) / sizeof(timers[0]); ++i) {
        if ((uintptr_t)timers[i].host > UINT32_MAX) {
        fprintf(stderr, "NATIVE CALLBACK INFRA host spy is above 32-bit ABI\n");
        return 2;
        }
    }

    reject_generated_stub = 0;
    for (unsigned i = 0; i < sizeof(timers) / sizeof(timers[0]); ++i) {
        callback_calls = 0;
        callback_kind = 0;
        callback_argument = NULL;
        if (call_host_timer((uint32_t)(uintptr_t)timers[i].host,
                            guest_data) != 1 || callback_calls != 1 ||
            callback_kind != i + 1 ||
            callback_argument != PSX_ADDR(guest_data)) {
            fprintf(stderr,
                    "NATIVE CALLBACK FAIL translated timer ABI %s arg=%p\n",
                    timers[i].name, callback_argument);
            return 1;
        }
    }

    callback_calls = 0;
    callback_kind = 0;
    if (call_host_timer(0x0050f00du, guest_data) != -1 || callback_calls != 0) {
        fprintf(stderr, "NATIVE CALLBACK FAIL unknown host target accepted\n");
        return 1;
    }

    reject_generated_stub = 1;
    callback_calls = 0;
    if (call_host_timer((uint32_t)(uintptr_t)func_80022DF4,
                        guest_data) != -1 || callback_calls != 0) {
        fprintf(stderr, "NATIVE CALLBACK FAIL generated stub was dispatched\n");
        return 1;
    }
    puts("NATIVE CALLBACK PASS map entry, translated guest data, unknown-host rejection, "
         "and generated-stub rejection; boundary-only spy");
    return 0;
}
