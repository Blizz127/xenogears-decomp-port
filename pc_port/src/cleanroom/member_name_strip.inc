void func_801C95A0(s32 character, s32 slot) {
    unsigned pair = (u8)character / 2u;
    unsigned position = (u8)slot / 2u;
    u8 *names = (u8 *)&g_GameState + pair * 40u;
    void *work = HeapAlloc(1014, 0);
    bzero(work, 1014);
    SystemRenderStringEntry(names, work, 36, 0);
    SystemRenderStringEntry(names + 20, work, 36, 1);

    RECT rectangle;
    u16 u, v;
    __builtin_memcpy(&u, &g_MemberChangeMenuCharTexcoordsU[position], sizeof(u));
    __builtin_memcpy(&v, &g_MemberChangeMenuCharTexcoordsV[position], sizeof(v));
    rectangle.x = (s16)(u + 384u);
    rectangle.y = (s16)v;
    rectangle.w = 40;
    rectangle.h = 13;
    LoadImage(&rectangle, work);
    DrawSync(0);
    HeapFree(work);
}
