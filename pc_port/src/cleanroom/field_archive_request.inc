int func_8001B484(int fileIndex, s32 secondKey)
{
    if (*(volatile s32 *)&D_8004F334 == secondKey &&
        *(volatile s32 *)&D_8004F330 == fileIndex) {
        return 0;
    }

    if (ArchiveDataSync() != 0) {
        return -1;
    }
    ArchiveCdDataSync(0);

    if (*(volatile s32 *)&D_8004F334 != -1) {
        HeapUnpinBlock(*(void *volatile *)&D_8005A4E0);
        HeapFree(*(void *volatile *)&D_8005A4E0);
    }

    func_8001B53C(fileIndex);
    *(volatile s32 *)&D_8004F334 = secondKey;
    *(volatile s32 *)&D_8004F330 = fileIndex;
    return -1;
}
