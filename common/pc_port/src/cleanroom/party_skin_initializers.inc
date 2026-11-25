/* Build the asynchronous archive request list for the current party. */
static void request_party_skins(int gear_mode)
{
    unsigned int kept = 0;
    unsigned int requests = 0;
    unsigned int slot;

    g_pGameState = &g_GameState;
    for (slot = 0; slot < MAX_PARTY_MEMBERS; ++slot) {
        unsigned int member = g_GameState.partyMembers[slot];
        if (member != 255u)
            g_GamePartyMembers[kept++] = member;
    }
    while (kept < MAX_PARTY_MEMBERS)
        g_GamePartyMembers[kept++] = 255;

    for (slot = 0; slot < MAX_PARTY_MEMBERS; ++slot) {
        int member = g_GamePartyMembers[slot];
        unsigned int skin, file;
        StreamDataQueueEntry *entry;
        void *buffer;

        if (member == 255)
            continue;
        skin = (unsigned int)member;
        if (gear_mode) {
            int gear = GameCharacterGetGearID(member);
            skin = gear == 255 ? 16u : 16u + (unsigned int)gear;
        }
        file = skin + 5u;
        entry = &g_PartyStreamDataQueue[requests];
        g_GamePartyMemberSkins[slot] = (int)skin;
        entry->archiveIndex = (short)file;
        buffer = HeapAlloc(ArchiveDecodeAlignedSize(file), 0);
        entry->pData = buffer;
        g_PartyStreamDataPointers[requests++] = buffer;
        HeapPinBlock(buffer);
    }

    g_PartyStreamDataQueue[requests].archiveIndex = 0;
    g_PartyStreamDataQueue[requests].pData = 0;
    func_80029AFC(g_PartyStreamDataQueue, 0, 0);
    D_8004F31C = gear_mode ? 2 : 1;
}

void GamePartyCharactersInitializeSkins(void)
{
    request_party_skins(0);
}

void GamePartyGearsInitializeSkins(void)
{
    request_party_skins(1);
}
