static unsigned int partyQueueOptional(unsigned int index, int file,
                                       void *volatile *slot, volatile int *flag,
                                       int successValue)
{
    u32 size = (u32)ArchiveDecodeAlignedSize(file);
    void *data = HeapAlloc(size, 1);
    *slot = data;
    volatile StreamDataQueueEntry *entry = &g_PartyStreamDataQueue[index];
    entry->pData = data;
    if (data) {
        HeapPinBlock(data);
        entry->archiveIndex = (short)file;
        *flag = successValue;
        ++index;
    }
    return index;
}

void GamePartyStreamLoadSkinData(int mode)
{
    HeapToggleErrorHandler(1);
    if (*(const volatile int *)&g_PartyIsWaitingForStreamData == 1) {
        GamePartySyncStreamedData();
    }
    GameWaitForCdData();

    const volatile u32 *members = (const volatile u32 *)g_GamePartyMemberSkins;
    void *volatile *owned = g_PartyStreamDataPointers;
    unsigned int count = 0;
    for (unsigned int i = 0; i < 3; ++i) {
        u32 skin = members[i];
        if (skin == 255u) {
            continue;
        }
        volatile StreamDataQueueEntry *entry = &g_PartyStreamDataQueue[count];
        entry->archiveIndex = (short)(u_short)(skin + 5u);
        u32 file = members[i] + 5u;
        u32 size = (u32)ArchiveDecodeAlignedSize(file);
        void *data = HeapAlloc(size, 1);
        owned[count] = data;
        entry->pData = data;
        if (!data) {
            for (unsigned int j = 0; j < count; ++j) {
                HeapUnpinBlock(owned[j]);
                HeapFree(owned[j]);
            }
            HeapToggleErrorHandler(0);
            return;
        }
        HeapPinBlock(owned[count]);
        ++count;
    }
    if (mode != 0) {
        count = partyQueueOptional(count, 0xa7, &D_8005A4A0, &D_8004F344, 1);
        count = partyQueueOptional(count, 0xa8, &D_8005A4BC, &D_8004F32C, 0);
    }
    volatile StreamDataQueueEntry *end = &g_PartyStreamDataQueue[count];
    end->pData = NULL;
    end->archiveIndex = 0;
    func_80029AFC(g_PartyStreamDataQueue, 0, 0);
    *(volatile int *)&g_PartyIsWaitingForStreamData = 1;
    HeapToggleErrorHandler(0);
}
