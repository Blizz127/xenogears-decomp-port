#ifndef XENO_CONTROLLER_VBLANK_H
#define XENO_CONTROLLER_VBLANK_H

/* Native callbacks stay function pointers; guest code addresses require a
 * separate translation at the guest/host boundary before registration. */
void func_8003634C(void);
void func_800363E0(int enabled);
void func_800363F0(void (*callback)(void));

/* Retail channel-4 registration. The native implementation and service entry
 * must be called only on the game thread, never PsyCross's interrupt thread.
 * Service consumes elapsed counter ticks once, coalescing a masked interval
 * into one pending IRQ. It does not present frames or sample historical input.
 * Registration does not reset the interrupt clock. Reset is a boot/lifecycle
 * operation, not an alternative spelling of callback registration. */
void func_8004B7D0(void (*callback)(void));
void PcPort_ServiceVblank(void);
void PcPort_ResetVblankService(void);
int PcPort_MaskVblank(void);
void PcPort_UnmaskVblank(void);
int PcPort_GetServicedVblankCount(void);

#endif
