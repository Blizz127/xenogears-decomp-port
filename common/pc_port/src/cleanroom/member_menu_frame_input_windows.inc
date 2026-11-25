#include "system/controller.h"

extern int ControllerGetType(int controllerIndex);
extern void SoundMuteAllSpuChannels(void);
extern void SoundEnableAllSpuChannels(void);
extern s32 func_80036410(void);
extern int ControllerPopState(void);
extern void ControllerResetState(void);
extern void GameCheckAndHandleSoftReset(void);
extern int Vsync(int mode);

static int MemberChangeDecodePadSample(void)
{
    static const u_short directions[] = {0x2000, 0x4000, 0x8000, 0x1000};
    static const struct {
        u_short mask;
        u_char input;
    } releases[] = {
        {0x20, MENU_INPUT_CONFIRM}, {0x40, MENU_INPUT_BACK},
        {0x80, MENU_INPUT_SQUARE}, {0x10, MENU_INPUT_TRIANGLE},
        {4, MENU_INPUT_L1}, {8, MENU_INPUT_R1}, {0x800, 11}, {0x100, 12},
        {1, MENU_INPUT_IDLE}
    };

    u_short pressed = g_C1ButtonStatePressedOnce;
    for (unsigned i = 0; i < sizeof(directions) / sizeof(directions[0]); ++i) {
        if (pressed & directions[i])
            return (int)i;
    }

    u_short released = g_C1ButtonStateReleased;
    for (unsigned i = 0; i < sizeof(releases) / sizeof(releases[0]); ++i) {
        if (!(released & releases[i].mask))
            continue;
        if (releases[i].mask == 0x100)
            g_Menu->unk1E94 = !g_Menu->unk1E94;
        if (releases[i].mask == 1)
            ++g_Menu->unk1E95;
        return releases[i].input;
    }
    return -1;
}

void MemberChangeMenuPollInput(void)
{
    if (!ControllerGetType(0)) {
        SoundMuteAllSpuChannels();
        int saved = D_80059488;
        while (!ControllerGetType(0)) {
        }
        SoundEnableAllSpuChannels();
        D_80059488 = saved;
    }

    int input = MENU_INPUT_IDLE;
    if (func_80036410()) {
        ControllerResetState();
    } else {
        while (ControllerPopState()) {
            int selected = MemberChangeDecodePadSample();
            if (selected >= 0) {
                input = selected;
                break;
            }
        }
    }
    g_Menu->input = (u_char)input;
}

void MemberChangeMenuUpdateAndRender(void)
{
    MemberChangeMenuPollInput();
    GameCheckAndHandleSoftReset();

    SystemMenu* menu = g_Menu;
    GfxEnvironment* next = &menu->gfxEnvs[menu->pGfxEnv == &menu->gfxEnvs[0]];
    menu->pGfxEnv = next;
    menu->renderContext = !menu->renderContext;
    ClearOTagR(next->ot, 16);
    MemberChangeMenuRender();
    int destinationY = g_Menu->renderContext == 0 ? 224 : 0;

    DrawSync(0);
    Vsync(0);
    PutDrawEnv(&g_Menu->pGfxEnv->drawEnv);
    PutDispEnv(&g_Menu->pGfxEnv->dispEnv);
    MoveImage(&g_Menu->pSelectionMenu->unk1180, 0, destinationY);
    DrawOTag(&g_Menu->pGfxEnv->ot[15]);
}

static u_short MemberChangeGrowWindowDimension(u_short current, u_short target)
{
    unsigned next = (unsigned)current + 32;
    return next < target ? (u_short)next : target;
}

void MemberChangeMenuUpdateWindows(void)
{
    for (unsigned slot = 0; slot < MENU_MAX_NUM_WINDOWS; ++slot) {
        if (!g_Menu->pManager->unk27[slot])
            continue;
        MenuWindowParameters* params = g_Menu->windowParameters[slot];
        if (params->unk11)
            continue;

        u_short width = MemberChangeGrowWindowDimension(params->unk8, params->width);
        u_short height = MemberChangeGrowWindowDimension(params->unkA, params->height);
        params->unk8 = width;
        params->unkA = height;
        if (width == params->width && height == params->height)
            params->unk11 = 1;

        u_short x = (u_short)(params->x + params->width / 2 - width / 2);
        u_short y = (u_short)(params->y + params->height / 2 - height / 2);
        MemberChangeMenuSetWindow(params->index, x, y, width, height, params->unk12,
                                  params->zIndex, params->hasScrollBar);
    }
}
