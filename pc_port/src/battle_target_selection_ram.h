#ifndef PC_PORT_BATTLE_TARGET_SELECTION_RAM_H
#define PC_PORT_BATTLE_TARGET_SELECTION_RAM_H
#include "battle_mips_adapter.h"
/* Isolated composition, NOT runtime dispatch. Stable writable RAM and separate
 * CPU object are caller-owned. Defined outputs: RAM, sp, s0-s7 and ra.
 * Caller-saved residue (including v0) and instruction count are NOT modeled.
 * On invalid access returns -1, retaining prior writes; never replay guest
 * execution after failure. Identity/ABI and exact-list gates remain pending. */
int PcPortBattleTargetSelectionRam(uint8_t *ram, size_t size, PcPortMipsCpu *cpu);
#endif
