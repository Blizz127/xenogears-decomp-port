void GamePartySyncStreamedData(void)
{
    unsigned int slot;

    if (!g_PartyIsWaitingForStreamData)
        return;

    GameWaitForCdData();
    for (slot = 0; slot < MAX_PARTY_MEMBERS; ++slot) {
        HeapUnpinBlock(g_PartyDataBuffers[slot]);
        if (g_GamePartyMembers[slot] == 255)
            continue;

        HeapUnpinBlock(g_PartyStreamDataPointers[slot]);
        LZSSDecompress(g_PartyStreamDataPointers[slot], g_PartyDataBuffers[slot]);
        HeapFree(g_PartyStreamDataPointers[slot]);
    }
    g_PartyIsWaitingForStreamData = 0;
}
