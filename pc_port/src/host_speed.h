#ifndef XENO_PC_PORT_HOST_SPEED_H
#define XENO_PC_PORT_HOST_SPEED_H

/* Host wall-clock controls. No emulated registers or script state are changed. */
#ifdef __cplusplus
extern "C" {
#endif
int PsyX_GetSpeedMultiplier(void);
void PsyX_SetSpeedMultiplier(int multiplier);
void PsyX_SetFastForwardHeld(int held);
void PsyX_SetFastForwardSpeed(int multiplier);
#ifdef __cplusplus
}
#endif

#endif
