/* The field headers can spell size_t as a 32-bit guest type. These two
 * services receive the native ABI's byte count on both host platforms. */
#ifdef _WIN32
extern void *ShopResourceZero(void *bytes, __SIZE_TYPE__ length) __asm__("bzero");
#else
extern void ShopResourceZero(void *bytes, __SIZE_TYPE__ length) __asm__("bzero");
#endif
extern void *ShopResourceMove(void *dest, const void *src, __SIZE_TYPE__ length) __asm__("memmove");

u32 *PcPort_LoadCommonShopResources(void)
{
    u32 *table = (u32 *)D_8005945C;
    ResolveArchiveEntryPointers(table);
    void *icon = LZSSHeapDecompress((void *)(uintptr_t)table[1], 1);
    OpenTIM(icon);
    ReadTIM(&g_Menu->unk32C->tim);
    MenuUnk2 *work = g_Menu->unk32C;
    for (int i = 0; i < 13; ++i)
        work->unk4F80[0x4e + i] = D_801C5000[i];
    work->unk4B94 = 0x53;
    work->unk4B95 = 0x43;
    work->unk4B96 = 0x11;
    work->unk4B97 = 1;
    /* Keep the three observable buffer services callable at every optimization
     * level. Each service may select a new current work object. */
    __typeof__(&ShopResourceZero) volatile clearBytes = &ShopResourceZero;
    __typeof__(&ShopResourceMove) volatile moveBytes = &ShopResourceMove;
    clearBytes(work->unk4B98, 0x5c);
    work = g_Menu->unk32C;
    moveBytes(work->unk4BF4, work->tim.caddr, 0x20);
    work = g_Menu->unk32C;
    moveBytes(work->unk4C14, work->tim.paddr, 0x80);
    HeapFree(icon);

    void *menuResource = LZSSHeapDecompress((void *)(uintptr_t)table[2], 1);
    func_8002DD20(menuResource);
    HeapFree(menuResource);
    void *atlas = LZSSHeapDecompress((void *)(uintptr_t)table[3], 0);
    g_Menu->unk2DC = atlas;
    void *secondAtlas = LZSSHeapDecompress((void *)(uintptr_t)table[4], 0);
    g_Menu->unk2E0 = secondAtlas;

    s32 coordinates[3][6];
    for (int slot = 0; slot < 3; ++slot) {
        s32 *row = coordinates[slot];
        func_80026338(g_Menu->unk2DC, 0x14b + slot,
                      (u32 *)&row[0], &row[1], &row[2], &row[3], &row[4], &row[5]);
    }
    coordinates[1][4] = (s32)((u32)coordinates[1][4] + 12u);

    u8 *portraits = LZSSHeapDecompress((void *)(uintptr_t)table[5], 1);
    TIM_IMAGE portrait;
    for (int slot = 0; slot < 3; ++slot) {
        u8 id = g_Menu->pManager->currentCharacterIDs[slot];
        if (id == 0xff)
            continue;
        OpenTIM((u_long *)(portraits + (u32)id * 0xb20u));
        ReadTIM(&portrait);
        const s32 *row = coordinates[slot];
        portrait.crect->x = (s16)row[2];
        portrait.crect->y = (s16)row[3];
        portrait.prect->x = (s16)row[4];
        portrait.prect->y = (s16)row[5];
        LoadImage(portrait.crect, (u_long *)portrait.caddr);
        LoadImage(portrait.prect, (u_long *)portrait.paddr);
    }
    DrawSync(0);
    HeapFree(portraits);

    if (g_MenuDebugEnabled) {
        ArchiveSetIndex(0x10, 2);
        u32 size = (u32)ArchiveDecodeAlignedSize(5);
        D_8006259C = HeapAlloc(size, 0);
        ArchiveReadFileToBuffer(5, D_8006259C, 0, 0x80);
        ArchiveCdDataSync(0);
        ArchiveSetIndex(0x10, 0);
        SoundAddSedsEntry(D_8006259C);
    }
    g_Menu->unk2E4 = D_8006259C;
    return table;
}
