void KernelMenuMain(void)
{
    KernelMenuInitialize();
    g_KernelMenuIsRunning = 1;
    D_800592C8 = 0;

    do {
        unsigned int frame = (unsigned int)D_800592C4 + 1u;
        int page = (int)(frame & 1u);
        KernelMenuRenderEnvironment *selected = &g_KernelMenuRenderEnvironments[page];
        void *orderingTable = &selected->ot;

        D_800592C4 = (int)frame;
        D_800592C8 = page;
        g_KernelMenuCurRenderEnvironment = selected;

        TermPrim(orderingTable);
        FontDrawLetters(orderingTable);
        KernelMenuUpdate();
        AddPrim(orderingTable, &g_KernelMenuCurRenderEnvironment->cursor);
        DrawSync(0);
        VSync(0);
        PutDrawEnv(&g_KernelMenuCurRenderEnvironment->drawEnv);
        PutDispEnv(&g_KernelMenuCurRenderEnvironment->dispEnv);
        DrawOTag(orderingTable);
    } while (g_KernelMenuIsRunning || D_800592C8 == 0);

    DrawSync(0);
    MainLoop(0);
}
