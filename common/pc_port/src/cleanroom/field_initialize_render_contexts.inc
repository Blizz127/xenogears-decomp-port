void FieldInitializeRenderContexts(void)
{
    extern int Vsync(int mode);
    extern void FieldSetScreenDimensions(void);
    extern void func_8002DFF0(s32, s32);

    volatile DRAWENV *firstDraw = &g_FieldRenderContexts[0].drawEnvs[0];
    volatile DRAWENV *secondDraw = &g_FieldRenderContexts[1].drawEnvs[0];

    D_80059198 = 1;
    DrawSync(0);
    Vsync(0);
    InitGeom();
    SetGeomOffset(160, 112);

    SetDefDrawEnv(&g_FieldRenderContexts[0].drawEnvs[0], 0, 0, 320, 224);
    SetDefDrawEnv(&g_FieldRenderContexts[1].drawEnvs[0], 0, 256, 320, 224);
    SetDefDrawEnv(&g_FieldRenderContexts[0].drawEnvs[1], 0, 0, 320, 224);
    SetDefDrawEnv(&g_FieldRenderContexts[1].drawEnvs[1], 0, 256, 320, 224);
    SetDefDispEnv(&g_FieldRenderContexts[0].dispEnv, 0, 256, 320, 224);
    SetDefDispEnv(&g_FieldRenderContexts[1].dispEnv, 0, 0, 320, 224);

    FieldSetClipDimensions(0, 0, 320, 224);
    FieldSetScreenDimensions();

    firstDraw->r0 = 0;
    firstDraw->g0 = 0;
    firstDraw->b0 = 0;
    secondDraw->r0 = 0;
    secondDraw->g0 = 0;
    secondDraw->b0 = 0;
    firstDraw->dtd = 1;
    secondDraw->dtd = 1;

    Vsync(0);
    PutDispEnv(&g_FieldRenderContexts[1].dispEnv);
    PutDrawEnv(&g_FieldRenderContexts[1].drawEnvs[0]);
    func_8002DFF0(320, 240);
}
