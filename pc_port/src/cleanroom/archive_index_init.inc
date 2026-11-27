#include <stdint.h>

void ArchiveInit(u32 tableAddress, u32 headerAddress, u32 mode)
{
    D_8005A488 = 0;
    D_8005A48C = 0;
    D_8005A490 = 0;
    D_8005A494 = 0;
    D_8005A498 = 0;
    D_8005A49C = 0;
    D_8005A4A4 = 0;
    D_8005A4A8 = 0;
    D_8005A4B4 = 0;

    if (mode == 0 || mode == UINT32_MAX) {
        while (CdInit() == 0) {
            /* The drive can take several attempts to initialize. */
        }
        CdSetDebug(0);
        CdDataCallback(0);
        CdSyncCallback(0);
        CdReadyCallback(0);
        CdControl(7, 0, (unsigned char *)&D_80059F1C);
        ArchiveCdSetMode(0xa0);
        ArchiveCdDataSync(0);
        Vsync(3);
    } else {
        PCinit();
    }

    g_ArchiveDebugTable = (mode == UINT32_MAX) ? 0 : mode;
    g_ArchiveTable = tableAddress;
    g_ArchiveHeader = headerAddress;
    g_CurArchiveOffset = 0;
    D_8004FDFC = 0;
    g_ArchiveCurFileSize = 0;
    g_ArchiveCdDriveState = 0;
    D_8004FE4C = -1;

    if (mode == 0) {
        ArchiveReadFileFromCdSector(24, (void *)(uintptr_t)tableAddress, 32768, 0, 0);
        ArchiveCdDataSync(0);
        ArchiveReadFileFromCdSector(40, (void *)(uintptr_t)g_ArchiveHeader, 122, 0, 0);
        ArchiveCdDataSync(0);
    }
}

int ArchiveSetIndex(int directoryIndex, int entryIndex)
{
    uint32_t index = (uint32_t)directoryIndex + (uint32_t)entryIndex;
    uint32_t address = g_ArchiveHeader + index * UINT32_C(2);
    const unsigned char *entry = (const unsigned char *)(uintptr_t)address;
    unsigned int value = (unsigned int)entry[0] | ((unsigned int)entry[1] << 8);

    if (value == 0) {
        g_CurArchiveOffset = 0;
        return -1;
    }

    g_CurArchiveOffset = value - 1;
    return (int)(value - 1);
}
