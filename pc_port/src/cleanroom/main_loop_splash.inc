static void splashPresentBrightness(SPRT *sprite, u_char level)
{
    sprite->r0 = level;
    sprite->g0 = level;
    sprite->b0 = level;
    DrawPrim(sprite);
    Vsync(0);
}

void GameShowSplashScreen(void)
{
    HeapSetCurrentUser(6);
    void *logo = LZSSHeapDecompress((void *)&g_PublishedByLogoCompressed, 1);
    uintptr_t address = (uintptr_t)logo;
    RECT upload = {0, 240, 16, 1};
    LoadImage(&upload, (u_long *)(address + 20u));
    upload = (RECT){640, 0, 64, 48};
    LoadImage(&upload, (u_long *)(address + 64u));

    SPRT sprite;
#if USE_EXTENDED_PRIM_POINTERS
    setlen(&sprite, 4);
#else
    /* The compact GPU tag stores its length after three untouched link bytes. */
    ((volatile u_char *)&sprite.tag)[3] = 4;
#endif
    sprite.code = 0x64;
    sprite.x0 = 32;
    sprite.y0 = 88;
    sprite.u0 = 0;
    sprite.v0 = 0;
    sprite.w = 256;
    sprite.h = 48;
    sprite.clut = GetClut(0, 240);

    DRAWENV draw;
    DISPENV display;
    SetDefDrawEnv(&draw, 0, 0, 320, 224);
    SetDefDispEnv(&display, 0, 0, 320, 224);
    PutDrawEnv(&draw);
    PutDispEnv(&display);
    DrawSync(0);

    for (int level = 0; level < 128; level += 8) {
        splashPresentBrightness(&sprite, (u_char)level);
    }
    for (int frame = 0; frame < 110; ++frame) {
        Vsync(0);
    }
    for (int level = 128; level >= 0; level -= 8) {
        splashPresentBrightness(&sprite, (u_char)level);
    }
    HeapFree(logo);
}
