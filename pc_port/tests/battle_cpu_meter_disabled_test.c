/* Production adapter + meter mechanism test. No gameplay/coverage claim. */
#define _POSIX_C_SOURCE 200809L
#include "battle_mips_adapter.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct BattleMipsRuntime { int unused; } BattleMipsRuntime;
#define BATTLE_BASE 0x80070000u
#define BATTLE_END 0x800C0000u
static unsigned reads;
static int target_is_guest_code(uint32_t target)
{
    return target >= 0x1000u && target < 0x100cu;
}
static const uint8_t *resolve_memory(BattleMipsRuntime *runtime,
                                     uint32_t address, unsigned width, int write)
{
    (void)runtime; (void)address; (void)width; (void)write;
    return NULL; /* No guest JAL/JALR in this fixture. */
}
static uint32_t load_le(const uint8_t *p, unsigned width)
{
    uint32_t value = 0;
    unsigned i;
    for (i = 0; i < width; ++i) value |= (uint32_t)p[i] << (8u * i);
    return value;
}
#include "battle_cpu_fallback_meter.inc"

static int read_word(void *opaque, uint32_t address, unsigned width, uint32_t *value)
{
    static const uint32_t code[] = {0x24020007u, 0x03e00008u, 0u};
    (void)opaque;
    reads++;
    if (width != 4 || !target_is_guest_code(address)) return -1;
    *value = code[(address - 0x1000u) / 4u];
    return 0;
}
static int metered_bridge(void *opaque, PcPortMipsCpu *cpu, uint32_t target)
{
    CpuMeterBridgeState state;
    int result = target == 0x2000u;
    cpu_meter_bridge_enter(opaque, cpu, target, &state);
    if (result) {
        cpu_meter_note_kind(CPU_METER_KIND_RESULTS);
        cpu->gpr[2] = 17;
    }
    cpu_meter_bridge_leave(cpu, target, result, &state);
    return result;
}
int main(void)
{
    PcPortMipsCpu cpu;
    BattleMipsRuntime runtime = {0};
    PcPortMipsBus bus = {0};
    unsigned disabled;
    bus.opaque = &runtime;
    bus.read = read_word;
    bus.bridge = metered_bridge;
    assert(setenv("XENO_CPU_FALLBACK_STATS", "1", 1) == 0);
    for (disabled = 0; disabled < 2; ++disabled) {
        assert(setenv("XENO_DISABLE_CPU_INTERPRETER", disabled ? "1" : "0", 1) == 0);
        PcPortMipsCpuInit(&cpu, &bus);
        cpu.gpr[31] = 0xfffffffcu;
        reads = 0;
        cpu_meter_begin_battle();
        cpu_meter_mark_entry(&cpu, 0x1000u);
        assert(PcPortMipsRun(&cpu, 0x1000u, 0xfffffffcu, 20) ==
               (disabled ? PC_PORT_MIPS_INTERPRETER_DISABLED : PC_PORT_MIPS_HALTED));
        assert(cpu.steps == (disabled ? 0u : 3u));
        assert(reads == cpu.steps);
        assert(g_CpuFallbackMeter.interpreted_instructions == cpu.steps);
        assert(g_CpuFallbackMeter.interpreted_calls == (disabled ? 0u : 1u));
        assert(g_CpuFallbackMeter.distinct_targets == (disabled ? 0u : 1u));
        assert(g_CpuFallbackMeter.disabled_dispatches == disabled);
        cpu_meter_end_battle(disabled ? "stopped" : "returned");
    }
    /* Native bridge work still counts when the CPU is disabled. */
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[31] = 0x1000u;
    reads = 0;
    cpu_meter_begin_battle();
    cpu_meter_mark_entry(&cpu, 0x2000u);
    assert(PcPortMipsRun(&cpu, 0x2000u, 0xfffffffcu, 20) == PC_PORT_MIPS_INTERPRETER_DISABLED);
    assert(cpu.gpr[2] == 17 && cpu.steps == 0 && reads == 0);
    assert(g_CpuFallbackMeter.native_results_calls == 1);
    assert(g_CpuFallbackMeter.disabled_dispatches == 1);
    assert(g_CpuFallbackMeter.interpreted_instructions == 0);
    assert(g_CpuFallbackMeter.interpreted_calls == 0);
    cpu_meter_end_battle("stopped");
    puts("CPU METER disabled-dispatch accounting PASS; gameplay not tested");
    return 0;
}
