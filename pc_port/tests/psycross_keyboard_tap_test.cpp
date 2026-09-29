#include <cstdio>
#include <cstring>

#include <SDL_scancode.h>

#include "PsyX/PsyX_public.h"
#include "pad/PsyX_pad.h"

extern const u_char* g_sdlKeyboardState;
extern u_short PsyX_Pad_UpdateKeyboardInput();

/* Added by the PC-port PsyCross patch.  A keydown must survive one pad update
 * even if its matching keyup was already drained from SDL's event queue. */
extern void PsyX_Pad_LatchKeyboardInput(int scancode);

PsyXKeyboardMapping g_cfg_keyboardMapping;
PsyXControllerMapping g_cfg_controllerMapping;
int g_padCommEnable = 1;
int g_activeKeyboardControllers = 1;

int main()
{
    static u_char keyboard[SDL_NUM_SCANCODES];
    struct TapCase {
        int scancode;
        u_short bit;
        const char* name;
    };
    const TapCase taps[] = {
        { SDL_SCANCODE_X,            0x8000, "Square" },
        { SDL_SCANCODE_Z,            0x2000, "Circle" },
        { SDL_SCANCODE_V,            0x1000, "Triangle" },
        { SDL_SCANCODE_C,            0x4000, "Cross" },
        { SDL_SCANCODE_LSHIFT,       0x0400, "L1" },
        { SDL_SCANCODE_LCTRL,        0x0100, "L2" },
        { SDL_SCANCODE_LEFTBRACKET,  0x0002, "L3" },
        { SDL_SCANCODE_RSHIFT,       0x0800, "R1" },
        { SDL_SCANCODE_RCTRL,        0x0200, "R2" },
        { SDL_SCANCODE_RIGHTBRACKET, 0x0004, "R3" },
        { SDL_SCANCODE_UP,           0x0010, "Up" },
        { SDL_SCANCODE_DOWN,         0x0040, "Down" },
        { SDL_SCANCODE_LEFT,         0x0080, "Left" },
        { SDL_SCANCODE_RIGHT,        0x0020, "Right" },
        { SDL_SCANCODE_SPACE,        0x0001, "Select" },
        { SDL_SCANCODE_RETURN,       0x0008, "Start" },
    };

    std::memset(&g_cfg_keyboardMapping, 0, sizeof(g_cfg_keyboardMapping));
    std::memset(keyboard, 0, sizeof(keyboard));
    g_cfg_keyboardMapping.kc_circle = SDL_SCANCODE_Z;
    g_cfg_keyboardMapping.kc_square = SDL_SCANCODE_X;
    g_cfg_keyboardMapping.kc_triangle = SDL_SCANCODE_V;
    g_cfg_keyboardMapping.kc_cross = SDL_SCANCODE_C;
    g_cfg_keyboardMapping.kc_l1 = SDL_SCANCODE_LSHIFT;
    g_cfg_keyboardMapping.kc_l2 = SDL_SCANCODE_LCTRL;
    g_cfg_keyboardMapping.kc_l3 = SDL_SCANCODE_LEFTBRACKET;
    g_cfg_keyboardMapping.kc_r1 = SDL_SCANCODE_RSHIFT;
    g_cfg_keyboardMapping.kc_r2 = SDL_SCANCODE_RCTRL;
    g_cfg_keyboardMapping.kc_r3 = SDL_SCANCODE_RIGHTBRACKET;
    g_cfg_keyboardMapping.kc_dpad_up = SDL_SCANCODE_UP;
    g_cfg_keyboardMapping.kc_dpad_down = SDL_SCANCODE_DOWN;
    g_cfg_keyboardMapping.kc_dpad_left = SDL_SCANCODE_LEFT;
    g_cfg_keyboardMapping.kc_dpad_right = SDL_SCANCODE_RIGHT;
    g_cfg_keyboardMapping.kc_select = SDL_SCANCODE_SPACE;
    g_cfg_keyboardMapping.kc_start = SDL_SCANCODE_RETURN;
    g_sdlKeyboardState = keyboard; /* final SDL state is already key-up */

    for (unsigned int i = 0; i < sizeof(taps) / sizeof(taps[0]); i++) {
        const u_short expected = (u_short)(0xFFFF & ~taps[i].bit);
        PsyX_Pad_LatchKeyboardInput(taps[i].scancode);
        const u_short first = PsyX_Pad_UpdateKeyboardInput();
        const u_short second = PsyX_Pad_UpdateKeyboardInput();

        if (first != expected) {
            std::fprintf(stderr, "queued %s tap was lost: got=%04x expected=%04x\n",
                         taps[i].name, first, expected);
            return 1;
        }
        if (second != 0xFFFF) {
            std::fprintf(stderr, "queued %s tap was not one-shot: second=%04x\n",
                         taps[i].name, second);
            return 1;
        }
    }

    /* Host shortcuts must not become game-pad input, whether SDL reports
     * a held key or a keydown latched before the next controller sample. */
    unsigned unmapped = 0;
    for (int scan = 0; scan < SDL_NUM_SCANCODES; ++scan) {
        bool mapped = false;
        for (const TapCase& tap : taps) mapped |= tap.scancode == scan;
        if (mapped) continue;
        keyboard[scan] = 1;
        PsyX_Pad_LatchKeyboardInput(scan);
        const u_short held = PsyX_Pad_UpdateKeyboardInput();
        keyboard[scan] = 0;
        const u_short released = PsyX_Pad_UpdateKeyboardInput();
        if (held != 0xFFFF || released != 0xFFFF) {
            std::fprintf(stderr, "unmapped scancode %d leaked: held=%04x released=%04x\n",
                         scan, held, released);
            return 1;
        }
        ++unmapped;
    }
    std::printf("PsyCross unmapped keyboard isolation: PASS (%u scancodes)\n", unmapped);
    std::puts("PsyCross queued keyboard tap: PASS");
    return 0;
}
