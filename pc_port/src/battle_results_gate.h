#ifndef XENO_BATTLE_RESULTS_GATE_INTERNAL_H
#define XENO_BATTLE_RESULTS_GATE_INTERNAL_H

/* Private to battle_mips_runtime.c: identity gate and dispatch for the
 * adopted battle RESULTS module routines (battle_results_gate.inc). */

/* Immutable code/rodata of the results module: 0x801DE000..0x801E44C0.
 * The module image is 0x6500 bytes; 0x801E44C0.. is mutable module data
 * (0x801E44C0 is a one-shot flag byte stored at 0x801E0A34; 0x801E44C4..
 * 0x801E44FF are pointers/state words written by the module), so it is not
 * part of the identity. */
#define RESULTS_IMMUTABLE_BYTES 0x64c0u

typedef struct BattleResultsIdentity {
    uint64_t generation;   /* bumped by every invalidation */
    uint64_t native_calls; /* adopted bodies run (diagnostics and tests) */
    int state;             /* RESULTS_UNKNOWN / _VERIFIED / _REJECTED */
    /* The exact bytes the verdict in `state` was computed from. */
    uint8_t snapshot[RESULTS_IMMUTABLE_BYTES];
} BattleResultsIdentity;

static void results_invalidate(BattleMipsRuntime *runtime);
static void results_note_store(BattleMipsRuntime *runtime, const uint8_t *p,
                               unsigned width);
static void results_note_archive(BattleMipsRuntime *runtime, uint32_t target);
static int results_try_host(BattleMipsRuntime *runtime, PcPortMipsCpu *cpu,
                            uint32_t target);
#endif
