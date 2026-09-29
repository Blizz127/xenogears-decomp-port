#ifndef PC_PORT_BATTLE_TARGET_SETUP_H
#define PC_PORT_BATTLE_TARGET_SETUP_H
#include "battle_mips_adapter.h"
/* Staged native BC2F0; not installed in runtime dispatch. The caller supplies
 * the real memory resolver (including shared globals and native task handles).
 * No pointer masking or host indirect calls occur here. invoke must complete
 * the callback synchronously, in the same memory domain, or return nonzero.
 * Returns -1 on failure, retaining prior writes: never retry via retail after
 * failure. Guest stack/register residue is not modeled by this API. */
int PcPortBattleTargetSetup(const PcPortMipsBus *memory, uint32_t mode,
                          int (*invoke)(void *, uint32_t, uint32_t),
                          void *callback_context);
/* Isolated guest-frame variant; NOT registered for runtime dispatch. CPU must
 * be separate from bus-addressable memory, with an aligned SP >= 0x18.
 * Preserves retail frame writes/reloads and callback-visible a0/s0/SP/RA.
 * Other caller-saved register residue, pipeline and instruction count are not
 * modeled. invoke executes the callback synchronously using this CPU's ABI
 * state and propagates its register effects, without changing the outer
 * instruction pipeline. Failures retain prior memory/register effects, with
 * no fallback/replay. Bus validity/access checks remain caller-owned. */
int PcPortBattleTargetSetupFrame(PcPortMipsCpu *cpu,
        int (*invoke)(void *, PcPortMipsCpu *, uint32_t), void *context);
#endif
