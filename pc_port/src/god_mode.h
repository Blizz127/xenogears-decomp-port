#ifndef XENO_PC_PORT_GOD_MODE_H
#define XENO_PC_PORT_GOD_MODE_H
#include "battle_mips_adapter.h"
int PcPort_GodModeEnabled(void);
void PcPort_GodModeToggle(void);
void PcPort_GodModeBeforeGuest(PcPortMipsCpu* cpu, uint32_t target);
/* Random-encounter switch for route testing.  Retail's field keeps encounters
 * enabled while g_FieldControl.isRandomEncountersEnabled == 0, so turning them
 * off is a matter of suppressing the two places the field rolls for one.  The
 * flag is host-side only: no guest word is written, so the matching build and
 * the retail instruction stream are untouched. */
int PcPort_RandomBattlesEnabled(void);
void PcPort_RandomBattlesToggle(void);
#endif
