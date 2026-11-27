extern void *LZSSHeapDecompress(void *, int);
extern void *HeapAlloc(u_int, u_int);
extern u_int HeapFree(void *);
extern void func_8002DD20(u32 *);
extern void SoundAddSedsEntry(SoundFile *);

/* SPDX-License-Identifier: MIT */
static SystemMenu *memberChangeResourceMenu(void)
{
    SystemMenu *volatile *active_menu = &g_Menu;
    return *active_menu;
}

/* SPDX-License-Identifier: MIT */
static MenuUnk2 *memberChangeResourceCard(void)
{
    SystemMenu *volatile *active_menu = &g_Menu;
    MenuUnk2 *volatile *card = &(*active_menu)->unk32C;
    return *card;
}

void MemberChangeMenuLoadResources(void)
{
    typedef u32 ArchiveEntryWord __attribute__((__may_alias__));
    void *const archive = *(void *volatile *)&D_8005945C;
    const volatile ArchiveEntryWord *const entries = archive;
    void *volatile *const soundBuffer = &D_8006259C;

    ResolveArchiveEntryPointers(archive);
    void *const cardData = LZSSHeapDecompress((void *)(uintptr_t)entries[1], 1);
    OpenTIM(cardData);
    ReadTIM(&memberChangeResourceCard()->tim);
    ((volatile MenuUnk2 *)memberChangeResourceCard())->unk4B94 = 0x53;
    ((volatile MenuUnk2 *)memberChangeResourceCard())->unk4B95 = 0x43;
    ((volatile MenuUnk2 *)memberChangeResourceCard())->unk4B96 = 0x11;
    ((volatile MenuUnk2 *)memberChangeResourceCard())->unk4B97 = 0x01;
    bzero(memberChangeResourceCard()->unk4B98, 0x5c);

    const void *const colors = ((volatile MenuUnk2 *)memberChangeResourceCard())->tim.caddr;
    memmove(memberChangeResourceCard()->unk4BF4, colors, 0x20);
    const void *const pixels = ((volatile MenuUnk2 *)memberChangeResourceCard())->tim.paddr;
    memmove(memberChangeResourceCard()->unk4C14, pixels, 0x80);
    HeapFree(cardData);

    void *const temporary = LZSSHeapDecompress((void *)(uintptr_t)entries[2], 1);
    func_8002DD20(temporary);
    HeapFree(temporary);
    void *const resource3 = LZSSHeapDecompress((void *)(uintptr_t)entries[3], 0);
    ((volatile SystemMenu *)memberChangeResourceMenu())->unk2DC = resource3;
    void *const resource4 = LZSSHeapDecompress((void *)(uintptr_t)entries[4], 0);
    ((volatile SystemMenu *)memberChangeResourceMenu())->unk2E0 = resource4;

    if (*(const volatile u8 *)&g_MenuDebugEnabled) {
        ArchiveSetIndex(16, 2);
        const u_int size = (u_int)ArchiveDecodeAlignedSize(5);
        *soundBuffer = HeapAlloc(size, 0);
        ArchiveReadFileToBuffer(5, *soundBuffer, 0, 128);
        ArchiveCdDataSync(0);
        ArchiveSetIndex(16, 0);
        SoundAddSedsEntry(*soundBuffer);
    }
    volatile SystemMenu *const menu = memberChangeResourceMenu();
    SoundFile *const sound = *soundBuffer;
    menu->unk2E4 = sound;
    HeapFree(archive);
}
