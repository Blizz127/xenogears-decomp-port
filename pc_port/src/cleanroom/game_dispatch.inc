static void gameInstallStateOverlay(MainGameState *selected)
{
    if (g_CurGameState == 3u) {
        fprintf(stderr, "[xeno-port] ChangeGameState(3) → MainLoop overlay load archive=0x0F hasOverlay=1\n");
    }
    ClearMemory(selected->pMemStart, selected->pHeapStart);
    void *image = LoadGameStateOverlay(g_CurGameState);
    if (g_CurGameState == 3u) {
        fprintf(stderr, "[xeno-port] LoadGameStateOverlay(3) returned %p\n", image);
    }
    if (!image) {
        fprintf(stderr, "[xeno-port] unable to enter game state %u: required overlay unavailable\n", g_CurGameState);
        exit(EXIT_FAILURE);
    }
    if (PcPort_GetOverlayLayout(g_CurGameState)) {
        uintptr_t start = (uintptr_t)g_MainGameStateOverlayBuffer;
        uintptr_t end = (uintptr_t)selected->pHeapStart;
        if (start != (uintptr_t)PSX_ADDR(0x8006faf0u) || end <= start ||
            end > (uintptr_t)PSX_ADDR(0x801fffffu) ||
            PcPort_DecodeCachedGameOverlay(g_CurGameState, g_MainGameStateOverlayBuffer, (unsigned)(end - start)) != 0) {
            fprintf(stderr, "[xeno-port] invalid overlay stream for state %u\n", g_CurGameState);
            exit(EXIT_FAILURE);
        }
    } else {
        LZSSDecompress(image, g_MainGameStateOverlayBuffer);
    }
    DrawSync(0);
    Vsync(0);
    EnterCriticalSection();
    DrawSync(0);
    Vsync(0);
    FlushCache();
    ExitCriticalSection();
}

void MainLoop(int reason)
{
    if (reason != 0) GameHandleError((unsigned int)reason, 0);
    for (;;) {
        unsigned int requested = g_CurGameState;
        PcPort_BattlingUnavailableGuard(requested, g_MainGameStates[requested].pFnMain != NULL);
        MainGameState *selected = &g_MainGameStates[g_CurGameState];
        ResetGraph(1);
        DrawSyncCallback(NULL);
        func_800363F0(NULL);
        DrawSync(0);
        Vsync(2);
        HeapRelocate((unsigned char *)selected->pHeapStart + 2048);
        HeapResetUser();
        if (selected->hasOverlay) gameInstallStateOverlay(selected);
        func_80019548();
        HeapRelocate((unsigned char *)selected->pHeapStart + 4);
        HeapReset();
        ControllerResetState();
        ChangeGameState(0);
        selected->pFnMain();
    }
}
