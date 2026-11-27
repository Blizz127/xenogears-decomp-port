#include <stdint.h>

typedef uint16_t ArchiveDirectoryWord __attribute__((aligned(1), __may_alias__));

static int archiveSignedSizeWord(uint32_t word);

int ArchiveGetCurFileSize(void)
{
    return *(const volatile int *)&g_ArchiveCurFileSize;
}

int func_80028548(int directoryIndex, int entryIndex)
{
    uint32_t base = *(const volatile uint32_t *)&g_ArchiveHeader;
    uint32_t entry = (uint32_t)directoryIndex + (uint32_t)entryIndex;
    uint32_t address = base + entry * 2u;
    unsigned value = *(const volatile ArchiveDirectoryWord *)(uintptr_t)address;
    uint32_t offset = *(const volatile uint32_t *)&g_CurArchiveOffset;
    return archiveSignedSizeWord(value - offset);
}

int ArchiveGetArchiveOffsetIndices(int *alignedIndex, int *remainder)
{
    uint32_t base = *(const volatile uint32_t *)&g_ArchiveHeader;
    unsigned match = 0;
    for (unsigned index = 0; index < 64; ++index) {
        uint32_t wanted = *(const volatile uint32_t *)&g_CurArchiveOffset + 1u;
        uint32_t address = base + 2u * index;
        unsigned entry = *(const volatile ArchiveDirectoryWord *)(uintptr_t)address;
        if (entry == wanted) {
            match = index;
            break;
        }
    }
    *(volatile int *)alignedIndex = (int)(match & ~3u);
    *(volatile int *)remainder = (int)(match & 3u);
    return archiveSignedSizeWord(*(const volatile uint32_t *)&g_CurArchiveOffset);
}

void *ArchiveChangeStreamingFile(void *next)
{
    void *volatile *slot = &g_ArchiveCurStreamFile;
    void *previous = *slot;
    *slot = next;
    return previous;
}

char *ArchiveGetFilePath(int entryIndex)
{
    uint32_t base = *(const volatile uint32_t *)&g_ArchiveDebugTable;
    if (base == 0)
        return (char *)0;
    uint32_t offset = *(const volatile uint32_t *)&g_CurArchiveOffset;
    uint32_t row = offset + (uint32_t)entryIndex - 1u;
    return (char *)(uintptr_t)(base + 64u * row);
}

int func_80028A18(int entryIndex)
{
    uint32_t offset = (uint32_t)*(const volatile int *)&D_8004FE18;
    uint32_t base = *(const volatile uint32_t *)&g_ArchiveTable;
    uint32_t row = offset + (uint32_t)entryIndex - 1u;
    const volatile uint8_t *record = (const volatile uint8_t *)(uintptr_t)(base + 7u * row);
    unsigned high = record[2];
    unsigned middle = record[1];
    unsigned low = record[0];
    return (int)((high << 16) | (middle << 8) | low);
}
