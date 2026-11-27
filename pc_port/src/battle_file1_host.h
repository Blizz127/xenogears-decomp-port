#ifndef XENO_BATTLE_FILE1_HOST_H
#define XENO_BATTLE_FILE1_HOST_H

/* Contract between the lifted battle command module routines
 * (battle_file1_host.c, generated bodies) and the identity-gated dispatch in
 * battle_mips_runtime.c (battle_file1_gate.inc).  The adopted-body ABI, the
 * accessors and the exit-register record are battle_results_host.h's. */
#include "battle_results_host.h"

/* A lifted body's own `jal target` at guest address `site`, made exactly as
 * BattleResults_Jal makes it (production bridge, callee on a copy of the
 * dispatching CPU, fail-closed unwind) with the file-1 content identity
 * re-checked afterwards.  Only valid inside an adopted file-1 body. */
void BattleFile1_Jal(BattleResultsRegs *regs, uint32_t target, uint32_t site);

#endif
