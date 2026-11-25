void func_80083994(void)
{
}

int func_80085F30(void)
{
    SoundWDSEntry *entry;

    if (ArchiveDataSync() != 0) {
        return -1;
    }

    entry = SoundLoadWdsFile(D_800B00E0, 0);
    D_8006251C = entry;
    D_80059560 = entry;
    func_8003BDFC(16);
    HeapFree(D_800B00E0);
    D_8004F364 = 1;
    D_8004F384 = 0;
    D_8004F368 = 0;
    return 0;
}

void func_80085FB8(void)
{
    void *buffer;

    ArchiveSetIndex(28, 0);
    buffer = HeapAlloc(ArchiveDecodeAlignedSize(3), 1);
    D_800B00E0 = buffer;
    ArchiveReadFileToBuffer(3, buffer, 0, 128);
    ArchiveSetIndex(4, 0);
    D_8004F364 = 128;
}

void func_80086024(void)
{
    if (D_8004F368 == 0) {
        SoundWDSEntry *entry = D_8006251C;
        D_8004F384 = 1;
        SoundFreeWdsEntry(entry);
        D_8004F368 = 1;
    }
    D_8004F364 = 0;
}
