/* The native-only build must not fetch instructions, even when runtime
 * configuration explicitly requests interpretation. Native calls still run. */
#define _POSIX_C_SOURCE 200809L
#include "battle_mips_adapter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned reads, writes, native_calls;

static int read_word(void *opaque, uint32_t address, unsigned width, uint32_t *value)
{
    (void)opaque; (void)address; (void)width;
    reads++;
    *value = 0;
    return 0;
}

static int write_word(void *opaque, uint32_t address, unsigned width, uint32_t value)
{
    (void)opaque; (void)address; (void)width; (void)value;
    writes++;
    return 0;
}

static int bridge(void *opaque, PcPortMipsCpu *cpu, uint32_t target)
{
    (void)opaque;
    if (target != 0x80010000u)
        return 0;
    native_calls++;
    cpu->gpr[2] = cpu->gpr[4] + 7u;
    return 1;
}

#define CHECK(condition, what) do { if (!(condition)) { \
    fprintf(stderr, "NATIVE ONLY FAIL %s\n", what); return 1; \
} } while (0)

int main(void)
{
    CHECK(setenv("XENO_CPU_EXECUTION_STATS", "1", 1) == 0, "statistics environment");
    PcPortMipsBus bus = { .read = read_word, .write = write_word, .bridge = bridge };
    const char *values[] = {NULL, "0", "1"};
    for (unsigned i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
        if (values[i]) {
            CHECK(setenv("XENO_DISABLE_CPU_INTERPRETER", values[i], 1) == 0, "control environment");
        } else {
            CHECK(unsetenv("XENO_DISABLE_CPU_INTERPRETER") == 0, "clear control environment");
        }
        CHECK(PcPortMipsInterpreterDisabled(), "runtime configuration enabled interpreter");
        PcPortMipsCpu cpu;
        PcPortMipsCpuInit(&cpu, &bus);
        cpu.gpr[4] = 35;
        cpu.gpr[31] = 0xfffffffcu;
        CHECK(PcPortMipsRun(&cpu, 0x80010000u, 0xfffffffcu, 8) == PC_PORT_MIPS_HALTED,
              "native bridge did not return");
        CHECK(cpu.gpr[2] == 42 && cpu.steps == 0, "native result/steps");
        CHECK(PcPortMipsRun(&cpu, 0x80020000u, 0xfffffffcu, 8) == PC_PORT_MIPS_INTERPRETER_DISABLED,
              "unconverted target was not refused");
        CHECK(strstr(cpu.error, "0x80020000") && cpu.steps == 0, "refusal target/steps");
    }
    PcPortMipsExecutionStats stats = PcPortMipsExecutionStatsGet();
    CHECK(reads == 0 && writes == 0 && native_calls == 3, "instruction bus touched");
    CHECK(stats.entries == 6 && stats.native_bridges == 3 && stats.interpreted_instructions == 0 &&
          stats.disabled_refusals == 3 && stats.unresolved_bridges == 0, "execution counters");
    puts("NATIVE ONLY PASS unset/0/1 controls, native calls=3, instruction fetches=0, interpreted instructions=0");
    return 0;
}
