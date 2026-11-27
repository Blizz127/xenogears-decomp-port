#include <stdint.h>

static int archiveSignedSizeWord(uint32_t word);
extern int archiveSubmitRead(u32 entry, u8 *destination, s32 argument, s32 flags)
    __asm__("ArchiveReadFile");
extern int archiveSubmitAlignedSize(unsigned int entry) __asm__("ArchiveDecodeAlignedSize");

int ArchiveGetDiscNumber(void)
{
    uint32_t header = *(const volatile u32 *)&g_ArchiveHeader;
    uint32_t address = header + 0x78u;
    typedef uint16_t DiscNumberWord __attribute__((__may_alias__, __aligned__(1)));
    return *(const volatile DiscNumberWord *)(uintptr_t)address;
}

static int archiveReadOwned(void)
{
    void (*volatile *cancel)(void) = &g_PcPortArchiveCancelFilteredQueue;
    return *cancel != 0 ||
           *(const volatile uint64_t *)&g_PcPortArchiveFileRegistration != 0;
}

s32 ArchiveReadFileFromCdSector(s32 sector, void *destination, s32 bytes,
                                s32 argument, u32 flags)
{
    if (archiveReadOwned())
        return -2;
    if (*(const volatile u32 *)&g_ArchiveDebugTable)
        return -1;

    (void)ArchiveCdDataSync(0);
    *(volatile s32 *)&g_ArchiveCurFileSector = sector;
    *(volatile int *)&g_ArchiveCurFileSize = bytes;
    return archiveSubmitRead(0, destination, argument, archiveSignedSizeWord(flags));
}

s32 ArchiveReadFileToBuffer(s32 entry, void *destination, u32 argument, u32 flags)
{
    if (archiveReadOwned())
        return -2;
    if (entry <= 0)
        return -3;
    int size = ArchiveDecodeSize(entry);
    if (size <= 0 || destination == 0)
        return -3;

    (void)ArchiveCdDataSync(0);
    uint32_t offset = *(const volatile u32 *)&g_CurArchiveOffset;
    *(volatile int *)&D_8004FE18 = archiveSignedSizeWord(offset);
    *(volatile s32 *)&g_ArchiveCurFileSector = ArchiveDecodeSector(entry);
    *(volatile int *)&g_ArchiveCurFileSize = archiveSubmitAlignedSize((unsigned int)entry);
    return archiveSubmitRead((u32)entry, destination, archiveSignedSizeWord(argument),
                             archiveSignedSizeWord(flags));
}
