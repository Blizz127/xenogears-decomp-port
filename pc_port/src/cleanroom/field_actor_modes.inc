extern int fieldActorModeVsync(int) __asm__("Vsync");
extern void fieldActorModeScreenDimensions(void) __asm__("FieldSetScreenDimensions");
extern void fieldActorModeZero(void) __asm__("func_800AAC08");
extern void fieldActorModeTwo(void) __asm__("func_800AABD8");
extern void fieldActorModeOne(int, int, int, int) __asm__("func_800AAE4C");
extern void fieldActorModeThree(int, int, int, int) __asm__("func_800AADC8");
extern int16_t fieldActorModeFlag __asm__("D_800ADB54");

void func_80086E1C(void)
{
    int mode = fieldActorParameterGet(1);
    if (mode == 0) {
        RECT screen = {0, 0, 1280, 512};
        ClearImage(&screen, 0, 0, 0);
        DrawSync(0);
        fieldActorModeVsync(0);
        SetDefDrawEnv(&g_FieldRenderContexts[0].drawEnvs[0], 0, 0, 640, 224);
        SetDefDrawEnv(&g_FieldRenderContexts[1].drawEnvs[0], 0, 256, 640, 224);
        SetDefDispEnv(&g_FieldRenderContexts[0].dispEnv, 0, 256, 640, 224);
        SetDefDispEnv(&g_FieldRenderContexts[1].dispEnv, 0, 0, 640, 224);
        fieldActorModeScreenDimensions();
    } else if (mode == 1 || mode == 2) {
        *(volatile int16_t *)&fieldActorModeFlag = mode == 1;
    }
    fieldActorParameterAdvance(3);
}

void func_80086FD0(void)
{
    ActorData *actor = fieldActorParameterCurrent();
    const volatile uint8_t *script = *(void *volatile *)&g_FieldScriptVMCurScriptData;
    unsigned cursor = *fieldActorParameterCursor(actor);
    unsigned mode = script[cursor + 1u];
    switch (mode) {
    case 0:
        fieldActorModeZero();
        fieldActorParameterAdvance(2);
        break;
    case 2:
        fieldActorModeTwo();
        fieldActorParameterAdvance(2);
        break;
    case 1:
    case 3: {
        int index = fieldActorParameterGet(2);
        int first = fieldActorParameterGet(4);
        int second = fieldActorParameterGet(6);
        int third = fieldActorParameterGet(8);
        if (mode == 1)
            fieldActorModeOne(index, first, second, third);
        else
            fieldActorModeThree(index, first, second, third);
        fieldActorParameterAdvance(10);
        break;
    }
    default:
        break;
    }
}
