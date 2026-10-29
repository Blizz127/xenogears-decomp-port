#ifndef XENO_PORT_PAD_CONFIRM_H
#define XENO_PORT_PAD_CONFIRM_H
/* input.pad_confirm = cross|circle: which physical controller face button
 * carries the game's confirm.  Read at start (PcPort_PadConfirmInit), and
 * changeable at run time (dev menu Options, "CONFIRM BUTTON"), where the
 * choice applies immediately and is saved to the user's config file. */
#ifdef __cplusplus
extern "C" {
#endif
/* `layoutCircle`: the retail layout's confirm is Circle (XENO_BUTTON_LAYOUT=jp). */
void PcPort_PadConfirmInit(int layoutCircle);
/* 1 when the physical Circle (east) button is the confirm. */
int PcPort_PadConfirmCircle(void);
/* Apply `circle` now; `persist` also writes input.pad_confirm to the config. */
void PcPort_PadConfirmSet(int circle, int persist);
void PcPort_PadConfirmToggle(void);
/* input.pad_fast_forward = off|on (default off; XENO_INPUT_PAD_FAST_FORWARD):
 * L3 toggles and R3 holds fast-forward, each only when pressed alone.  Read
 * by the pad code (psycross_pad_select.inl). */
int PcPort_PadFastForwardEnabled(void);
#ifdef __cplusplus
}
#endif
#endif
