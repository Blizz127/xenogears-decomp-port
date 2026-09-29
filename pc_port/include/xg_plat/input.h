/* xg_plat/input.h -- input interface: keyboard bindings for the pad.
 * Backend: PsyCross keyboard mapping (src/plat/xg_plat_psycross.c). */
#ifndef XG_PLAT_INPUT_H
#define XG_PLAT_INPUT_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    XG_PLAT_BTN_SQUARE, XG_PLAT_BTN_CIRCLE, XG_PLAT_BTN_TRIANGLE, XG_PLAT_BTN_CROSS,
    XG_PLAT_BTN_L1, XG_PLAT_BTN_L2, XG_PLAT_BTN_R1, XG_PLAT_BTN_R2,
    XG_PLAT_BTN_START, XG_PLAT_BTN_SELECT,
    XG_PLAT_BTN_LEFT, XG_PLAT_BTN_RIGHT, XG_PLAT_BTN_UP, XG_PLAT_BTN_DOWN,
    XG_PLAT_BTN_COUNT
} XgPlatButton;

/* Name used in config files ("circle", "l1", "up", ...), or NULL. */
const char* xg_plat_input_button_name(XgPlatButton button);
/* Bind a keyboard key by its SDL key name ("Z", "Left Shift", ...).
 * Returns 1 when the key name is known. */
int xg_plat_input_bind_key(XgPlatButton button, const char* key_name);

/* SDL key name currently bound to `button` ("Z", "Return", ...), or NULL. */
const char* xg_plat_input_key_name(XgPlatButton button);

/* The game's pad path.  The game reads its two 0x22-byte controller buffers
 * (g_C1Buffer: status, id, buttons[2] active-low, analog[4]), which on PSX
 * the BIOS fills every vblank.  The port registers them once (ControllerInit
 * -> attach_pad) and refreshes them once per frame (the VSync shim -> poll);
 * both arrive here by link-time routing (build_port.sh XG_PLAT_WRAP_SYMS),
 * and the backend fills the buffers from the keyboard bindings above and any
 * game controllers. */
void xg_plat_input_attach_pad(int slot, unsigned char* buffer); /* slot 0/1 */
void xg_plat_input_poll(void);

/* Current pad state from the attached buffers: pad 1 in the low 16 bits,
 * pad 2 in the high 16, PSX button bit layout, bit set = pressed.  0 for a
 * slot with no buffer or no controller.  (The game itself never calls
 * PadRead; this is for plugins and tools.) */
unsigned int xg_plat_input_read_pads(void);

#ifdef __cplusplus
}
#endif

#endif
