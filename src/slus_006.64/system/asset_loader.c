#include "common.h"
#include "system/archive.h"
#include "system/memory.h"

extern int ArchiveDecodeSizeAbsolute(int index);
extern int ArchiveDecodeAlignedSize(int index);
extern int func_80029AFC(StreamDataQueueEntry* entries, int arg1, int arg2);
extern u8* D_800658C8;

#ifdef XENO_PC_PORT
#include "../../../pc_port/src/psx_memory.h"
/* Keep the queue in its retail RAM location and eight-byte layout. The native
 * archive adapter already accepts guest-RAM queues, translates their pointers
 * and consumes them synchronously. A host pointer must not occupy two words. */
typedef struct {
    s16 archiveIndex;
    u16 pad;
    u32 pData;
} AssetQueueEntry;
_Static_assert(sizeof(AssetQueueEntry) == 8, "retail archive queue stride");
#define ASSET_QUEUE ((AssetQueueEntry*)PSX_ADDR(0x8005A1DCu))
#define QUEUE_ADDRESS(p) PsxMemory_GuestAddr(p)
#define QUEUE_BUFFER(p) ((u8*)PSX_ADDR(p))
#else
extern StreamDataQueueEntry D_8005A1DC[3];
#define ASSET_QUEUE D_8005A1DC
#define QUEUE_ADDRESS(p) (p)
#define QUEUE_BUFFER(p) ((u8*)(p))
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/asset_loader", func_800379D8);
#else
/* Retail 800379D8..80037B88. Loads the selected pair from archive 0xC/3,
 * retaining both pinned allocations and exposing the second file past its
 * four-byte header. No replacement data or success-on-failure path is added. */
s32 func_800379D8(s32 index, s32 variant,
                  u8** first, u8** second, u8** third)
{
    s32 savedDirectory, savedEntry;
    s32 result = 0;
    s32 count;
    u8* firstData;
    u8* secondData;

    ArchiveGetArchiveOffsetIndices(&savedDirectory, &savedEntry);
    ArchiveSetIndex(0xC, 3);
    HeapChangeCurrentUser(4, NULL);
    count = (s16)ArchiveDecodeSizeAbsolute(5) / 2;
    if (index >= count) {
        result = -1;
        *first = NULL;
        *second = NULL;
        *third = NULL;
    } else {
        /* SLL/ADDU/ADDIU are modulo 32 bits, including signed input cases. */
        s32 firstFile = (s32)((u32)index * 2u + 6u);
        s32 secondFile = (s32)((u32)index * 2u + 7u + (u32)variant);
        secondData = HeapAlloc(ArchiveDecodeAlignedSize(secondFile), 1);
        HeapPinBlock((HeapBlock*)secondData);
        firstData = HeapAlloc(ArchiveDecodeAlignedSize(firstFile), 1);
        HeapPinBlock((HeapBlock*)firstData);

        ASSET_QUEUE[0].archiveIndex = (s16)firstFile;
        ASSET_QUEUE[0].pData = QUEUE_ADDRESS(firstData);
        ASSET_QUEUE[1].archiveIndex = (s16)secondFile;
        ASSET_QUEUE[1].pData = QUEUE_ADDRESS(secondData);
        ASSET_QUEUE[2].archiveIndex = 0;
        ASSET_QUEUE[2].pData = 0;
        func_80029AFC((StreamDataQueueEntry*)ASSET_QUEUE, 0, 0);

        *first = firstData;
        *second = NULL;
        /* Retail reloads queue[1] after the archive call can sort the queue. */
        *third = QUEUE_BUFFER(ASSET_QUEUE[1].pData) + 4;
        D_800658C8 = QUEUE_BUFFER(ASSET_QUEUE[1].pData) + 4;
    }
    ArchiveSetIndex(savedDirectory, savedEntry);
    return result;
}
#endif
