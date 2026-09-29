#ifndef PC_PORT_BATTLE_TARGET_LIST_RAM_H
#define PC_PORT_BATTLE_TARGET_LIST_RAM_H
#include "battle_mips_adapter.h"
/* Isolated, NOT dispatch-enabled. Stable writable RAM and a separate CPU
 * object are caller-owned. Updates guest list/count/frame, v0, sp and saved
 * registers. Does not model caller-saved register residue or instruction count.
 * Returns -1 on an invalid accessed address, preserving partial writes; never
 * replay guest code after failure. Code identity/ABI/adoption remain external
 * gates, including the list's unresolved retail instruction match. */
int PcPortBattleTargetListRam(uint8_t *ram, size_t size, PcPortMipsCpu *cpu);
#endif
