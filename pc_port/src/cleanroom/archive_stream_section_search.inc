#include <stdint.h>

static int archiveSignedSizeWord(uint32_t word);

static volatile ArchiveStreamFileSectionHeader *archiveStreamSectionAt(
    ArchiveStreamFileSectionHeader *base, uint32_t index)
{
    /* Keep the native base, including its upper address bits. The guest
       section displacement wraps at 32 bits and can designate earlier rows. */
    intptr_t delta = archiveSignedSizeWord(index * 8u);
    return (volatile ArchiveStreamFileSectionHeader *)((uintptr_t)base + (uintptr_t)delta);
}

int func_80028E60(int sectionIndex, int maxEntries, int targetState)
{
    if (maxEntries <= 0)
        return 0;
    ArchiveStreamFileSectionHeader *table =
        *(ArchiveStreamFileSectionHeader *volatile *)&D_8004FE2C;
    int limit = *(const volatile int *)&D_8004FE40;
    uint32_t index = (uint32_t)sectionIndex;
    for (int remaining = maxEntries; remaining; --remaining) {
        if ((int)archiveStreamSectionAt(table, index)->state != targetState)
            return 1;
        ++index;
        if (archiveSignedSizeWord(index) > limit)
            return 1;
    }
    return 0;
}

void ArchiveConsolidateStreamFileEntry(int sectionIndex)
{
    ArchiveStreamFileSectionHeader *table =
        *(ArchiveStreamFileSectionHeader *volatile *)&D_8004FE2C;
    uint32_t index = (uint32_t)sectionIndex;
    volatile ArchiveStreamFileSectionHeader *section = archiveStreamSectionAt(table, index);
    unsigned size = section->size;
    int stride = (int)size;
    if (size >= 0x8000u)
        stride -= 0x10000;
    uint32_t nextIndex = index + (uint32_t)stride;
    int limit = *(const volatile int *)&D_8004FE40;
    if (archiveSignedSizeWord(nextIndex) < limit) {
        const volatile ArchiveStreamFileSectionHeader *next = archiveStreamSectionAt(table, nextIndex);
        if (next->state == 0)
            section->size = (uint16_t)(size + next->size);
    }
}
