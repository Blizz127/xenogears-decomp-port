void GamePartySignalReinitialize(void)
{
    *(volatile int *)&g_GamePartySkinsInitialized = 0;
}

void GameWaitForCdData(void)
{
    while (ArchiveDataSync() != 0) {
    }
    ArchiveCdDataSync(0);
}

void func_8001B53C(int index)
{
    u32 file = (u32)index + 184u;
    u32 size = (u32)ArchiveDecodeAlignedSize(file);
    *(volatile int *)&D_8005A4C0 = (int)size;
    void *allocation = HeapAlloc(size, 1);
    *(void *volatile *)&D_8005A4E0 = allocation;
    HeapPinBlock(allocation);
    void *destination = *(void *volatile *)&D_8005A4E0;
    ArchiveReadFileToBuffer((int)file, destination, 0, 128);
}

void func_8001B5A8(void)
{
    if (*(const volatile int *)&g_GameHasLoadedWDS == 1) {
        SoundFreeWdsEntry(g_GameCurLoadedWDS);
        *(volatile int *)&g_GameHasLoadedWDS = 0;
    }
}

void func_8001B6BC(void)
{
}

void func_8001B94C(DRAWENV *env)
{
    volatile DRAWENV *output = env;
    output->isbg = 1;
    output->dtd = 1;
    output->r0 = 60;
    output->g0 = 120;
    output->b0 = 120;
}

void GamePartySyncSkinData(void)
{
    GameWaitForCdData();
    int waiting = *(const volatile int *)&g_PartyIsWaitingForStreamData;
    if (waiting == 1) {
        GamePartySyncStreamedData();
    }
    if (*(const volatile int *)&g_GamePartySkinsInitialized) {
        if (waiting != 1) {
            GamePartyStreamLoadSkinData(0);
        }
        return;
    }

    int map = *(const volatile int *)&g_GameSceneMapNum;
    *(volatile int *)&D_8004F320 = (map & 0xc000) != 0;
    int gears = *(const volatile int *)&D_8004F320;
    *(volatile int *)&g_PartyIsWaitingForStreamData = 0;
    int current = *(const volatile int *)&D_8004F31C;
    if (gears) {
        if (current == 2) {
            return;
        }
        GamePartyGearsInitializeSkins();
    } else {
        if (current == 1) {
            return;
        }
        GamePartyCharactersInitializeSkins();
    }
    *(volatile int *)&g_PartyIsWaitingForStreamData = 1;
}
