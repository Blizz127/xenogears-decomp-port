static void gamePrintDiagnosticFrame(unsigned code, unsigned origin, unsigned frame)
{
    FontPrintf((char *)&D_800181C8, (int32_t)code);
    FontPrintf((char *)&D_800181DC, (unsigned int)origin);
    FontPrintf((char *)&D_800181E8, (int32_t)D_8004F2BC);
    FontPrintf((char *)&D_800181F4, (int32_t)frame);
    FontPrintf((char *)&D_80018200);
    FontPrintf((char *)&D_8001821C);
    if (code & 0x80u) {
        uint32_t guestString;
        memcpy(&guestString, PSX_ADDR(0x8004f0c0u + code * 4u), sizeof guestString);
        FontPrintf((char *)&D_80018220, (char *)PsxMemory_WordPtr(guestString));
    }
    if (code == 0x82u || code == 0x85u) {
        mem_addr site;
        u_int amount;
        HeapGetAllocInformation(&site, &amount);
        FontPrintf((char *)&D_80018224, (unsigned int)site);
        if (code == 0x82u) FontPrintf((char *)&D_80018238, (int32_t)amount, (unsigned int)amount);
        else FontPrintf((char *)&D_80018258, (void *)(uintptr_t)(uint32_t)amount);
    }
}

void GameHandleError(unsigned int code, unsigned int origin)
{
    PcPort_ReportGameError(code, origin);
    u32 mode = (u32)D_80010000;
    if (mode == 0 || mode == UINT32_MAX) {
        RECT screen = {0, 0, 640, 480};
        ClearImage(&screen, 255, 0, 0);
        for (;;) {
        }
    }

    DRAWENV draw[2];
    DISPENV display[2];
    HeapDumpToFile((char *)&D_800181B8);
    HeapForceFreeAllBlocks();
    for (unsigned i = 0; i < 2; ++i) {
        SetDefDrawEnv(&draw[i], 0, 240 * (int)i, 384, 240);
        SetDefDispEnv(&display[i], 0, 240 * (int)(1 - i), 384, 240);
    }
    DrawSyncCallback(NULL);
    func_800363F0(NULL);
    FontLoadFont(16, 16, 288, 240, 500, 0, 960, 256, 960, 511, NULL);
    for (unsigned i = 0; i < 2; ++i) {
        draw[i].isbg = 1;
        draw[i].r0 = draw[i].g0 = draw[i].b0 = 0;
    }
    ++D_8004F2BC;
    SetDispMask(1);
    for (unsigned int frame = 0;; ++frame) {
        unsigned int which = (frame & 1u) ^ 1u;
        PutDrawEnv(&draw[which]);
        PutDispEnv(&display[which]);
        FontDrawLetters(NULL);
        gamePrintDiagnosticFrame(code, origin, frame);
        Vsync(0);
    }
}
