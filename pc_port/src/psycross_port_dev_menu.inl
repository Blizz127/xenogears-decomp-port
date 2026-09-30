/* SDL host input for the opt-in Port Dev Menu. A/B are physical controller
 * buttons, independent of the game's USA/Japanese confirm mapping. */
#include "port_dev_menu.h"
static int PsyX_PortDevMenuHandleEvent(SDL_Event *event)
{
    static SDL_JoystickID comboDevice = -1;
    static int leftClick, rightClick, comboHeld, stickDirection;
    int open = PcPort_DevMenuOpen();
    if (!PcPort_DevMenuEnabled()) return 0;
    if (event->type == SDL_KEYDOWN || event->type == SDL_KEYUP) {
        int pressed = event->type == SDL_KEYDOWN;
        int key = event->key.keysym.scancode;
        if (pressed && event->key.repeat) return open;
        /* The key left of 1: unused by the game's keyboard map and by every
         * PsyCross/host hotkey (F7/F8 stay quick save/load). */
        if (key == SDL_SCANCODE_GRAVE)
            return PcPort_DevMenuKey(PC_DEV_OPEN, pressed);
        if (!open) return 0;
        if (key == SDL_SCANCODE_UP) PcPort_DevMenuKey(PC_DEV_UP, pressed);
        else if (key == SDL_SCANCODE_DOWN) PcPort_DevMenuKey(PC_DEV_DOWN, pressed);
        else if (key == SDL_SCANCODE_RETURN || key == SDL_SCANCODE_Z) PcPort_DevMenuKey(PC_DEV_ACCEPT, pressed);
        else if (key == SDL_SCANCODE_ESCAPE || key == SDL_SCANCODE_C) PcPort_DevMenuKey(PC_DEV_BACK, pressed);
        return 1;
    }
    if (event->type == SDL_CONTROLLERBUTTONDOWN || event->type == SDL_CONTROLLERBUTTONUP) {
        int pressed = event->type == SDL_CONTROLLERBUTTONDOWN;
        int button = event->cbutton.button;
        if (comboDevice != event->cbutton.which) {
            comboDevice = event->cbutton.which; leftClick = rightClick = comboHeld = 0;
        }
        if (button == SDL_CONTROLLER_BUTTON_LEFTSTICK) leftClick = pressed;
        if (button == SDL_CONTROLLER_BUTTON_RIGHTSTICK) rightClick = pressed;
        if (leftClick && rightClick && !comboHeld) {
            PcPort_DevMenuKey(PC_DEV_OPEN, 1); comboHeld = 1; return 1;
        }
        if (!leftClick || !rightClick) comboHeld = 0;
        if (!open) return button == SDL_CONTROLLER_BUTTON_LEFTSTICK || button == SDL_CONTROLLER_BUTTON_RIGHTSTICK;
        if (button == SDL_CONTROLLER_BUTTON_DPAD_UP) PcPort_DevMenuKey(PC_DEV_UP, pressed);
        else if (button == SDL_CONTROLLER_BUTTON_DPAD_DOWN) PcPort_DevMenuKey(PC_DEV_DOWN, pressed);
        else if (button == SDL_CONTROLLER_BUTTON_A) PcPort_DevMenuKey(PC_DEV_ACCEPT, pressed);
        else if (button == SDL_CONTROLLER_BUTTON_B) PcPort_DevMenuKey(PC_DEV_BACK, pressed);
        return 1;
    }
    if (open && event->type == SDL_CONTROLLERAXISMOTION) {
        if (event->caxis.axis == SDL_CONTROLLER_AXIS_LEFTY) {
            int dir = event->caxis.value < -16000 ? -1 : event->caxis.value > 16000 ? 1 : 0;
            if (dir && dir != stickDirection) PcPort_DevMenuKey(dir < 0 ? PC_DEV_UP : PC_DEV_DOWN, 1);
            stickDirection = dir;
        }
        return 1;
    }
    if (event->type == SDL_WINDOWEVENT && event->window.event == SDL_WINDOWEVENT_FOCUS_LOST)
        leftClick = rightClick = comboHeld = stickDirection = 0;
    return open && (event->type == SDL_MOUSEBUTTONDOWN || event->type == SDL_MOUSEBUTTONUP || event->type == SDL_MOUSEWHEEL);
}

static void PsyX_PortDevMenuDraw()
{
    GLboolean scissorEnabled, mask[4];
    GLint scissor[4];
    GLfloat clear[4];
    int top, i, rows;
    if (!PcPort_DevMenuOpen()) return;
    scissorEnabled = glIsEnabled(GL_SCISSOR_TEST);
    glGetBooleanv(GL_COLOR_WRITEMASK, mask);
    glGetIntegerv(GL_SCISSOR_BOX, scissor);
    glGetFloatv(GL_COLOR_CLEAR_VALUE, clear);
    glEnable(GL_SCISSOR_TEST); glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    top = g_windowHeight - 30;
    rows = PcPort_DevMenuRows();
    PsyX_HostToolbarClearRect(8, top - 28 * (rows + 3), g_windowWidth - 16, 28 * (rows + 4), 0.035f, 0.05f, 0.09f);
    PsyX_HostToolbarDrawText(PcPort_DevMenuTitle(), 20, top);
    for (i = 0; i < rows; i++) {
        int y = top - 28 * (i + 1);
        if (i == PcPort_DevMenuCursor()) PsyX_HostToolbarClearRect(16, y - 4, g_windowWidth - 32, 24, 0.15f, 0.30f, 0.40f);
        PsyX_HostToolbarDrawText(PcPort_DevMenuRow(i), 24, y);
    }
    PsyX_HostToolbarDrawText("DPAD OR STICK  A PICK  B BACK", 20, top - 28 * (rows + 1));
    PsyX_HostToolbarDrawText("KEY LEFT OF 1 OR L3 AND R3 CLOSE", 20, top - 28 * (rows + 2));
    PsyX_HostToolbarDrawText(PcPort_DevMenuStatus(), 20, top - 28 * (rows + 3));
    glClearColor(clear[0], clear[1], clear[2], clear[3]);
    glColorMask(mask[0], mask[1], mask[2], mask[3]);
    glScissor(scissor[0], scissor[1], scissor[2], scissor[3]);
    if (!scissorEnabled) glDisable(GL_SCISSOR_TEST);
}
