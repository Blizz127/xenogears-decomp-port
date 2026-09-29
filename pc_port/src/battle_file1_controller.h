#ifndef XENO_BATTLE_FILE1_CONTROLLER_INTERNAL_H
#define XENO_BATTLE_FILE1_CONTROLLER_INTERNAL_H

/* Private to battle_mips_runtime.c. The payload hash is checked at the
 * ordinary archive load boundary, before the module initializer can run. */
typedef struct BattleFile1Identity {
    uint64_t generation;
    int verified;
    int archive20_selected;
} BattleFile1Identity;

static void file1_invalidate(BattleMipsRuntime *runtime);
static int file1_identity_current(BattleMipsRuntime *runtime);
static int file1_try_controller(BattleMipsRuntime *runtime, PcPortMipsCpu *cpu);
static void file1_before_archive(BattleMipsRuntime *runtime, PcPortMipsCpu *cpu,
                                 uint32_t target, int *candidate);
static void file1_after_archive(BattleMipsRuntime *runtime, PcPortMipsCpu *cpu,
                                uint32_t target, uintptr_t result, int candidate);
#endif
