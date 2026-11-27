void KernelMenuInitialize(void)
{
    KernelMenuRenderEnvironment *pages = g_KernelMenuRenderEnvironments;
    int page;

    HeapChangeCurrentUser(6, 0);
    FontLoadFont(8, 16, 368, 480, 1000, 1, 960, 256, 960, 511, 0);

    for (page = 0; page < 2; ++page)
        SetDefDrawEnv(&pages[page].drawEnv, 0, page * 240, 320, 224);
    for (page = 0; page < 2; ++page)
        SetDefDispEnv(&pages[page].dispEnv, 0, (1 - page) * 240, 320, 224);
    for (page = 0; page < 2; ++page)
        KernelMenuInitializeCursor(page);
}
