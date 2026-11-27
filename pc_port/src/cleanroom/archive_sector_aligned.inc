#include <stdint.h>

static int archiveSignedSizeWord(uint32_t word);

int ArchiveDecodeAlignedSize(unsigned int entryIndex)
{
    int size = ArchiveDecodeSize(archiveSignedSizeWord(entryIndex));
    int biased = archiveSignedSizeWord((uint32_t)size + 3u);
    return biased - biased % 4;
}

int ArchiveDecodeSector(int entryIndex)
{
    uint32_t offset = *(const volatile uint32_t *)&g_CurArchiveOffset;
    uint32_t table = *(const volatile uint32_t *)&g_ArchiveTable;
    uint32_t row = offset + (uint32_t)entryIndex - 1u;
    uint32_t address = table + 7u * row;
    const volatile uint8_t *record = (const volatile uint8_t *)(uintptr_t)address;

    unsigned int high = record[2];
    unsigned int middle = record[1];
    unsigned int low = record[0];
    return (int)((high << 16) | (middle << 8) | low);
}
