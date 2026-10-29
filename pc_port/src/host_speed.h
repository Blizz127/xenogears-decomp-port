#ifndef XENO_PC_PORT_HOST_SPEED_H
#define XENO_PC_PORT_HOST_SPEED_H

/* Host wall-clock controls. No emulated registers or script state are changed. */
#ifdef __cplusplus
extern "C" {
#endif
int PsyX_GetSpeedMultiplier(void);
void PsyX_SetSpeedMultiplier(int multiplier);
void PsyX_SetFastForwardHeld(int held);
/* As above, naming the source for the "speed:" log line (psycross_host_speed.inl). */
void PsyX_SetSpeedMultiplierFrom(int multiplier, const char* source);
void PsyX_SetFastForwardHeldFrom(int held, const char* source);
void PsyX_SetFastForwardSpeed(int multiplier);
#ifdef __cplusplus
}
#endif

#endif
