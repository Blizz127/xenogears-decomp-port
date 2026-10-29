#ifndef XENO_BATTLE_MIPS_ADAPTER_H
#define XENO_BATTLE_MIPS_ADAPTER_H

#include <stddef.h>
#include <stdint.h>

typedef struct PcPortMipsCpu PcPortMipsCpu;

typedef struct PcPortMipsBus {
    void *opaque;
    int (*read)(void *opaque, uint32_t address, unsigned width,
                uint32_t *value);
    int (*write)(void *opaque, uint32_t address, unsigned width,
                 uint32_t value);

    /* Called before fetching an instruction at target. Return 1 when target
     * was serviced as a native call, 0 when it is guest code, and -1 on an
     * unresolved boundary. A serviced call writes its result to v0/v1; the
     * core resumes at the guest ra. */
    int (*bridge)(void *opaque, PcPortMipsCpu *cpu, uint32_t target);

    uint32_t (*cop2_read)(void *opaque, int control, unsigned reg);
    void (*cop2_write)(void *opaque, int control, unsigned reg,
                       uint32_t value);
    int (*cop2_command)(void *opaque, uint32_t instruction);
} PcPortMipsBus;

struct PcPortMipsCpu {
    uint32_t gpr[32];
    uint32_t hi;
    uint32_t lo;
    uint32_t cp0[32];
    uint32_t pc;
    uint32_t next_pc;
    uint64_t steps;

    uint32_t load_value;
    uint32_t load_mask;
    uint8_t load_reg;
    uint8_t load_valid;

    PcPortMipsBus bus;
    char error[192];
};

enum {
    PC_PORT_MIPS_HALTED = 0,
    PC_PORT_MIPS_FAULT = -1,
    PC_PORT_MIPS_BUDGET = -2,
    PC_PORT_MIPS_UNSUPPORTED = -3,
    PC_PORT_MIPS_UNRESOLVED_CALL = -4,
    PC_PORT_MIPS_INTERPRETER_DISABLED = -5
};

void PcPortMipsCpuInit(PcPortMipsCpu *cpu, const PcPortMipsBus *bus);
/* In hybrid builds, diagnostic acceptance mode, off by default. In builds
 * with XENO_NATIVE_ONLY the instruction engine is excluded and this always
 * returns true. Native bridges remain allowed; missing bodies are refused
 * before fetch/execute. Neither mode establishes complete game conversion. */
int PcPortMipsInterpreterDisabled(void);
/* Optional process-wide adapter counters. Counts execution attempts, not game
 * coverage. Initialize at startup so a run without adapter calls reports zero. */
typedef struct PcPortMipsExecutionStats {
    uint64_t entries;
    uint64_t native_bridges;
    uint64_t interpreted_instructions;
    uint64_t disabled_refusals;
    uint64_t unresolved_bridges;
} PcPortMipsExecutionStats;
void PcPortMipsExecutionStatsInit(void);
PcPortMipsExecutionStats PcPortMipsExecutionStatsGet(void);
int PcPortMipsRun(PcPortMipsCpu *cpu, uint32_t entry, uint32_t halt_pc,
                  uint64_t max_steps);

#endif
