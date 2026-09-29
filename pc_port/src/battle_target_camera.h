#ifndef PC_PORT_BATTLE_TARGET_CAMERA_H
#define PC_PORT_BATTLE_TARGET_CAMERA_H
#include "battle_mips_adapter.h"
/* Staged BC460 native computation and public state writes, not registered in
 * runtime dispatch. The passive bus resolves every guest/shared/native handle;
 * no raw pointer is cast or masked by this API. Input tables/vectors must remain
 * stable during the call (apart from its public writes) and must not alias
 * native locals or SDK/GTE backing storage.
 * Guest frame/register residue and fault-instruction timing are not modeled;
 * unused raw VZ0 padding differs, but register-visible VZ0 is preserved.
 * Returns -1 on a bus/API failure, retaining preceding public writes and GTE
 * effects. Never fall back to retail or replay after failure. Native SDK fault
 * policy (including VectorNormal overflow abort) remains unchanged. */
int PcPortBattleUpdateTargetCamera(const PcPortMipsBus *memory,uint32_t mask);
#endif
