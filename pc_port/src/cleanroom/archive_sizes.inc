/* Independently authored from Disc 1 behavior and the supplied PC ABI. */
#include <stdint.h>

static uint32_t archiveReadSizeField(int entryIndex, uint32_t archiveOffset)
{
    uint32_t row = (uint32_t)entryIndex + archiveOffset - 1u;
    uint32_t address = g_ArchiveTable + row * 7u + 3u;
    const uint8_t *field = (const uint8_t *)(uintptr_t)address;

    return (uint32_t)field[0] | ((uint32_t)field[1] << 8) |
           ((uint32_t)field[2] << 16) | ((uint32_t)field[3] << 24);
}

static int archiveSignedSizeWord(uint32_t word)
{
    if (word > INT32_MAX) {
        return -1 - (int)(UINT32_MAX - word);
    }
    return (int)word;
}

static int archiveDebugFileSize(int entryIndex)
{
    if (!g_ArchiveDebugTable) {
        return 0;
    }

    char *path = ArchiveGetFilePath(entryIndex);
    int fd = PCopen(path, 0, 0);
    int size = PClseek(fd, 0, 2);
    PCclose(fd);
    return size;
}

int ArchiveDecodeSize(int entryIndex)
{
    int debugSize = archiveDebugFileSize(entryIndex);
    if (debugSize > 0) {
        return debugSize;
    }

    return archiveSignedSizeWord(archiveReadSizeField(entryIndex, g_CurArchiveOffset));
}

int ArchiveDecodeSizeAligned(int entryIndex)
{
    int debugSize = archiveDebugFileSize(entryIndex);
    uint32_t size = debugSize > 0 ? (uint32_t)debugSize :
                    archiveReadSizeField(entryIndex, (uint32_t)D_8004FE18);
    int biased = archiveSignedSizeWord(size + 3u);
    return (biased / 4) * 4;
}

int ArchiveDecodeSizeAbsolute(int entryIndex)
{
    uint32_t size = archiveReadSizeField(entryIndex, g_CurArchiveOffset);
    if (size <= INT32_MAX) {
        return 0;
    }

    int magnitudeLow = (int)((0u - size) & UINT16_MAX);
    return magnitudeLow >= 0x8000 ? magnitudeLow - 0x10000 : magnitudeLow;
}
