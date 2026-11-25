void KernelMenuUpdate(void)
{
    char version[12];
    volatile int *choice = &g_KernelMenuCurChoice;
    const volatile unsigned short *pressed = &g_C1ButtonStatePressedOnce;
    POLY_F3 *cursor;
    unsigned int row;

    if (*pressed & 0x1000u) {
        *choice = (int)((unsigned int)*choice - 1u);
        if (*choice < 0)
            *choice = 5;
    }
    if (*pressed & 0x4000u) {
        *choice = (int)((unsigned int)*choice + 1u);
        if (*choice >= 6)
            *choice = 0;
    }
    if (g_C1ButtonStateReleased & 0x20u) {
        ChangeGameState((int)((unsigned int)*choice + 1u));
        g_KernelMenuIsRunning = 0;
    }

    Sprintf(version, &D_800182B0, (unsigned char)D_80059484,
           (unsigned char)D_80059420, (unsigned char)D_80059418);
    FontPrintf(&D_800182C0, version, D_80010000 ? &D_800182E8 : &D_800182F0);
    FontPrintf(&D_800182F8);

    row = (unsigned int)*choice * 8u;
    cursor = &g_KernelMenuCurRenderEnvironment->cursor;
    cursor->x0 = 32;
    cursor->y0 = (short)(row + 40u);
    cursor->x1 = 39;
    cursor->y1 = (short)(row + 44u);
    cursor->x2 = 32;
    cursor->y2 = (short)(row + 48u);
}
