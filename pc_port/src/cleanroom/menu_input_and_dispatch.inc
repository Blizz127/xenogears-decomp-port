extern int PcPort_RoboMenuFieldVisitPending(void);

void func_8001BEEC(void)
{
    volatile SystemMenu *view = g_Menu;
    view->translation.vz = 0x800;
    view->unk228 = 0x800;
    view->rotation.vz = 0;
    view->rotation.vy = 0;
    view->rotation.vx = 0;
    view->translation.vy = 0;
    view->translation.vx = 0;
    view->unk21C = 0;
    view->unk21A = 0;
    view->unk218 = 0;
    view->unk224 = 0;
    view->unk220 = 0;
    view->unk2E8 = 1;
    view->transitionEffectState = 0;
}

void MenuProcessControllerInput(void)
{
    static const unsigned short buttons[] = {
        0x2000, 0x4000, 0x8000, 0x1000, 0x0020, 0x0100, 0x0004, 0x0001
    };
    unsigned char input = 8;
    if (func_80036410()) {
        ControllerResetState();
    } else {
        while (ControllerPopState()) {
            unsigned short pressed = g_C1ButtonStatePressedOnce;
            unsigned int choice = 0;
            while (choice < sizeof buttons / sizeof buttons[0] && !(pressed & buttons[choice]))
                ++choice;
            if (choice == sizeof buttons / sizeof buttons[0])
                continue;

            volatile SystemMenu *state = g_Menu;
            if (choice < 5) {
                input = (unsigned char)choice;
            } else if (choice == 5) {
                state->unk1E94 = state->unk1E94 == 0;
                input = 12;
            } else if (choice == 6) {
                if (state->unk1E95)
                    --state->unk1E95;
            } else {
                ++state->unk1E95;
            }
            break;
        }
    }
    g_Menu->input = input;
}

void MenuMain(void)
{
    SystemMenu *allocated = HeapAlloc(sizeof(SystemMenu), 0);
    g_Menu = allocated;
    bzero((unsigned char *)allocated, sizeof(SystemMenu));
    g_Menu->input = 8;
    HeapChangeCurrentUser(2, 0);

    SystemMenu *active = g_Menu;
    volatile SystemMenu *menu = active;
    menu->pGfxEnv = &active->gfxEnvs[1];
    menu->unk1E94 = 0;
    menu->unk1E95 = 1;
    menu->unk2D8 = 0;
    menu->shouldDrawMenu = 0;
    MenuInitializeGfxEnvironments();
    if (g_MenuDebugEnabled) {
        volatile SystemMenu *debugMenu = g_Menu;
        debugMenu->gfxEnvs[0].drawEnv.isbg = 1;
        debugMenu->gfxEnvs[1].drawEnv.isbg = 1;
    }

    func_8001BEEC();
    Vsync(0);
    PutDrawEnv(&g_Menu->gfxEnvs[0].drawEnv);
    PutDrawEnv(&g_Menu->gfxEnvs[1].drawEnv);
    PutDispEnv(&g_Menu->gfxEnvs[0].dispEnv);
    PutDispEnv(&g_Menu->gfxEnvs[1].dispEnv);
    SetDispMask(1);
    MenuExecute();
    if (!PcPort_RoboMenuFieldVisitPending())
        g_MenuDebugEnabled = 1;
}
