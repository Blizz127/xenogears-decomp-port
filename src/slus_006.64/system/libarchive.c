#include "common.h"
#include "system/archive.h"
#include "psyq/pc.h"
#include "psyq/libcd.h"
#ifdef XENO_PC_PORT
#include <stdint.h>
#endif

// Temp
extern int func_800286CC(void);

extern s32 g_ArchiveCurFileSector;

void ArchiveInit(u32 pArchiveTable, u32 pHeaderTable, u32 pDebugTable) {
    D_8005A488 = 0;
    D_8005A48C = 0;
    D_8005A490 = 0;
    D_8005A494 = 0;
    D_8005A498 = 0;
    D_8005A49C = 0;
    D_8005A4A4 = 0;
    D_8005A4A8 = 0;
    D_8005A4B4 = 0;
    
    if ( (pDebugTable == NULL) || (pDebugTable == -1) ) {
        while (CdInit() == 0);
        CdSetDebug(0);
        CdDataCallback(0);
        CdSyncCallback(NULL);
        CdReadyCallback(NULL);
        CdControl(7, 0, &D_80059F1C);
        ArchiveCdSetMode(0xA0);
        ArchiveCdDataSync(0);
        Vsync(3);
    } else {
        PCinit();
    }

    if (pDebugTable != -1) {
        g_ArchiveDebugTable = pDebugTable;
    } else {
        g_ArchiveDebugTable = NULL;
    }
    
    g_ArchiveTable = pArchiveTable;
    g_ArchiveHeader = pHeaderTable;
    g_CurArchiveOffset = 0;
    D_8004FDFC = 0;
    g_ArchiveCurFileSize = 0;
    g_ArchiveCdDriveState = 0;
    D_8004FE4C = -1;
    
    if (!pDebugTable) {
        ArchiveReadFileFromCdSector(0x18, pArchiveTable, (0x10 * CD_SECTOR_SIZE), 0, 0);
        ArchiveCdDataSync(0);
        ArchiveReadFileFromCdSector(0x28, g_ArchiveHeader, 0x7A, 0, 0);
        ArchiveCdDataSync(0);
    }
}

void ArchiveReset(void) {
    func_8002A498(0);
    ArchiveCdDataSync(0);
    if (g_ArchiveDebugTable == 0) {
        while (CdControlB(9, 0, &D_80059F1C) == 0);
        ArchiveCdSetMode(0xA0);
        ArchiveCdDataSync(0);
        Vsync(3);
    }
    CdDataCallback(0);
    CdSyncCallback(NULL);
    CdReadyCallback(NULL);
    D_8004FDFC = 0;
    g_ArchiveCurFileSize = 0;
    g_ArchiveCdDriveState = 0;
}

int ArchiveSetIndex(int directoryIndex, int entryIndex) {
    s32 nOffset;

    nOffset = ((u_short*)g_ArchiveHeader)[directoryIndex + entryIndex] - 1;
    g_CurArchiveOffset = nOffset;
    if (nOffset < 0) {
        g_CurArchiveOffset = 0;
        return -1;
    }
    return nOffset;
}

int ArchiveGetArchiveOffsetIndices(int* alignedIndex, int* remainder) {
    int nAligned;
    int nIndex;
    u_short* pArchiveHeaderEntry;

    pArchiveHeaderEntry = g_ArchiveHeader;
    nIndex = 0;
    
    while(nIndex < ARCHIVE_MAX_SECTIONS) {
        if (*pArchiveHeaderEntry == g_CurArchiveOffset + 1) {
            // Round down to 4-byte boundary
            nAligned = (nIndex / 4) * 4;
            
            *alignedIndex = nAligned;
            *remainder = nIndex - nAligned;
            break;
        }
        
        nIndex += 1;
        pArchiveHeaderEntry += 1;
    }
    
    if (nIndex == ARCHIVE_MAX_SECTIONS) {
        *alignedIndex = 0;
        *remainder = 0;
    }
    
    return g_CurArchiveOffset;
}

unsigned short ArchiveGetDiscNumber(void) {
    return *((unsigned short*)(g_ArchiveHeader + 0x78));
}

int func_80028548(int directoryIndex, int entryIndex) {
    int nOffset;
    nOffset = ((u_short*)g_ArchiveHeader)[directoryIndex + entryIndex];
    return nOffset - g_CurArchiveOffset;
}

// Only matches on GCC 2.6.X
char* ArchiveReadHddFile(char* pFilePath, int* pFileSize) {
    int nFileSize;
    int hFile;
    int i;
    int nBytesRead;
    char* pFileBuffer;

    for (i = 0; i < RETRY_COUNT; i++) {
        hFile = PCopen(pFilePath, 0, O_RDONLY);
        if (hFile != -1)
            break;
    }

    if (hFile == -1) {
        pFileBuffer = NULL;
        goto error;
    }

    nFileSize = PClseek(hFile, 0, SEEK_END);
    if (pFileSize)
        *pFileSize = nFileSize;
    PClseek(hFile, 0, SEEK_SET);
    
    pFileBuffer = (char*) HeapAlloc(nFileSize, 0x0);
    nBytesRead = 0;
    if (pFileBuffer) {
        for (i = 0; i < RETRY_COUNT; i++) {
            nBytesRead = PCread(hFile, pFileBuffer, nFileSize);
            if (nBytesRead != 0)
                break;
        }
    } 
    
    if (nBytesRead == 0) {
        if (pFileBuffer)
            HeapFree(pFileBuffer);
        pFileBuffer = NULL;
    }

    i = 0;
    while (PCclose(hFile) != 0) {
        i += 1;
        if (i >= RETRY_COUNT) {
            if (pFileBuffer)
                HeapFree(pFileBuffer);
            pFileBuffer = NULL;
            break;
        }        
    }

    error:
    return pFileBuffer;
}

int ArchiveGetCurFileSize(void) {
    return g_ArchiveCurFileSize;
}

int ArchiveDataSync(void) {
    int nResult;

    nResult = D_8004FDFC;
    if (nResult == 0) {
        if (g_ArchiveDebugTable == NULL && CdDataSync(1)) {
            return 1;
        }
        if (g_ArchiveCdDriveState != 0) {
            return 1;
        }
    }
    return nResult;
}

int ArchiveDecodeSize(int entryIndex) {
    char* pFilepath;
    int hArchiveFile;
    int nFileSize;
    u8* pArchiveEntry;
    u32 nOffset;
    
    if (g_ArchiveDebugTable) {
        pFilepath = ArchiveGetFilePath(entryIndex);
        hArchiveFile = PCopen(pFilepath, O_RDONLY, 0);
        nFileSize = PClseek(hArchiveFile, 0, SEEK_END);
        PCclose(hArchiveFile);
        if (nFileSize > 0) {
            goto exit;
        }
    }
    
    nOffset = (entryIndex + g_CurArchiveOffset - 1) * ARCHIVE_HEADER_ENTRY_SIZE;
    pArchiveEntry = nOffset + g_ArchiveTable;
    nFileSize = (
        (pArchiveEntry[6] << 0x18) + 
        (pArchiveEntry[5] << 0x10) + 
        (pArchiveEntry[4] << 0x8) + 
        pArchiveEntry[3]
    );

    exit:
    return nFileSize;
}

// Uses D_8004FE18 as archive offset
int ArchiveDecodeSizeAligned(int entryIndex) {
    char* pFilepath;
    int hArchiveFile;
    int nFileSize;
    u8* pArchiveEntry;
    u32 nOffset;
    
    if (g_ArchiveDebugTable) {
        pFilepath = ArchiveGetFilePath(entryIndex);
        hArchiveFile = PCopen(pFilepath, O_RDONLY, 0);
        nFileSize = PClseek(hArchiveFile, 0, SEEK_END);
        PCclose(hArchiveFile);
        if (nFileSize > 0) {
            goto exit;
        }
    }
    
    nOffset = (entryIndex + D_8004FE18 - 1) * ARCHIVE_HEADER_ENTRY_SIZE;
    pArchiveEntry = nOffset + g_ArchiveTable;
    nFileSize = (
        (pArchiveEntry[6] << 0x18) + 
        (pArchiveEntry[5] << 0x10) + 
        (pArchiveEntry[4] << 0x8) + 
        pArchiveEntry[3]
    );

    exit:
    nFileSize = ((nFileSize + 3) / 4) * 4;
    return nFileSize;
}

// Only matches on GCC 2.6.X
int ArchiveDecodeAlignedSize(unsigned int entryIndex) {
    s32 nSize;
    s32 nAlignedSize;
    
    nSize = ArchiveDecodeSize(entryIndex);
    nAlignedSize = ((nSize + 3 ) / 4);
    return nAlignedSize * 4;
}

int ArchiveDecodeSizeAbsolute(int entryIndex) {
    int nFileSize;
    u8* pArchiveEntry;
    u32 nOffset;
    
    nOffset = (entryIndex + g_CurArchiveOffset - 1) * ARCHIVE_HEADER_ENTRY_SIZE;
    pArchiveEntry = nOffset + g_ArchiveTable;
    nFileSize = (
        (pArchiveEntry[6] << 0x18) + 
        (pArchiveEntry[5] << 0x10) + 
        (pArchiveEntry[4] << 0x8) + 
        pArchiveEntry[3]
    );

    if (nFileSize >= 0)
        nFileSize = 0;
    else
        nFileSize = (s16) (nFileSize * -1);

    return nFileSize;
}

char* ArchiveGetFilePath(int entryIndex) {
    unsigned int nOffset;
    
    if (g_ArchiveDebugTable) {
        nOffset = (entryIndex + g_CurArchiveOffset - 1) * ARCHIVE_ENTRY_NAME_BUFFER_SIZE;
        return nOffset + g_ArchiveDebugTable;
    }

    return NULL;
}

int ArchiveDecodeSector(int entryIndex) {
    u_char* pArchiveEntry;
    unsigned int nOffset;

    nOffset = (entryIndex + g_CurArchiveOffset - 1) * ARCHIVE_HEADER_ENTRY_SIZE;
    pArchiveEntry = nOffset + g_ArchiveTable;
    return (pArchiveEntry[2] << 0x10) + (pArchiveEntry[1] << 0x8) + pArchiveEntry[0];
}

// Same as ArchiveDecodeSector, but uses D_8004FE18 as current archive offset
int func_80028A18(int entryIndex) {
    u_char* pArchiveEntry;
    unsigned int nOffset;

    nOffset = (entryIndex + D_8004FE18 - 1) * ARCHIVE_HEADER_ENTRY_SIZE;
    pArchiveEntry = nOffset + g_ArchiveTable;
    return (pArchiveEntry[2] << 0x10) + (pArchiveEntry[1] << 0x8) + pArchiveEntry[0];
}



// Possible start of archive2.c

void ArchiveCdDataSync(int mode) {
    int nResult;

    if (mode == 0) {
        do {
#ifdef XENO_PC_PORT
            /* Retail CD interrupts progress independently of this loop.
             * PsyCross needs a game-thread transport service point. */
            extern void PcPort_ArchivePollTransport(void);
            PcPort_ArchivePollTransport();
#endif
            nResult = ArchiveDataSync();
        } while (0 < nResult);
    }
    ArchiveDataSync();
}

void* ArchiveChangeStreamingFile(void* pStreamFile) {
    void* pPrevStreamFile;

    pPrevStreamFile = g_ArchiveCurStreamFile;
    g_ArchiveCurStreamFile = pStreamFile;
    return pPrevStreamFile;
}

int ArchiveClearStreamFileSections(void) {
    int nEntries;
    int i;
    void* pStreamData;
    ArchiveStreamFileSectionHeader* pCurSectionHeader;
    ArchiveStreamFileSectionHeader* pSectionHeaderStart;

    pStreamData = g_ArchiveCurStreamFile;
    if (pStreamData == NULL) {
        return -1;
    }
    
    i = 0; 
    nEntries = *(int*)pStreamData;
    pSectionHeaderStart = (ArchiveStreamFileSectionHeader*)(pStreamData + 4);
    
    pCurSectionHeader = pSectionHeaderStart;
    while (i < nEntries) {
        pCurSectionHeader[i].state = 0;
        pCurSectionHeader[i].id = 0;
        pCurSectionHeader[i].size = 0;
        pCurSectionHeader[i].unk2 = 0;
        i++;
    }
    pSectionHeaderStart->size = nEntries;
    
    return nEntries;
}

extern s32 D_8004FE0C;
extern s32 D_8004FE10;
extern u16 D_8004FE24;
extern u32 D_80059F0C;
extern void func_8002804C(s32, s32, s32, s32);

/* Transcribed from asm/slus_006.64/nonmatchings/system/libarchive/func_80028B14.s
 * (0x80028B14-0x80028E60). Streaming sector poll: reports the 0x800-byte sector
 * buffer for a free ring slot, or 0 while nothing is ready.
 *   streams NULL -> 0; buffers = pStreamFile + nStreamSectors*8 + 0x24.
 *   CD (g_ArchiveDebugTable == 0): scan the D_8004FE2C ring for slot
 *   state==3 && id==D_8004FE24, bump D_8004FE24 and report that slot buffer;
 *   a scan that runs past D_8004FE40 entries reports 0.
 *   debug table: hFile -1 or g_ArchiveCurFileSize <= 0 -> 0; otherwise rotate
 *   D_8004FE10 over the D_8004FE40 ring slots until a NOT_LOADED slot is found
 *   (a full ring whose scanned slot is loaded reports 0), mark it state 3 and
 *   PCread 0x800 into it with three func_8002804C(...,0,0xFF,0) retries;
 *   a short file (size - 0x800 <= 0) closes the handle with three
 *   func_8002804C(...,0,0,0xFF) retries, D_8004FE4C = -1, then queues the next
 *   D_8004FE0C entry (PCopen retries, ArchiveDecodeSizeAligned into
 *   g_ArchiveCurFileSize, D_8004FDFC--); an empty/absent next entry clears
 *   g_ArchiveCurFileSize and D_8004FDFC. Frame 0x38. */
#ifdef XENO_PC_PORT
/* The native port keeps its own streaming poll in pc_port/src/archive_port.c:
 * the retail CD-DMA arm has no host equivalent and the host queue must stay
 * one-sector-per-call (CdReadSync). Mark this matching definition weak so the
 * port owner wins the link; the matching build is unaffected. Retire the port
 * owner when the matching poll can drive the host CD queue and the streamed
 * field smoke passes with it. */
__attribute__((weak))
#endif
#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/libarchive", func_80028B14);
#else
s32 func_80028B14(void) {
    u8* pStreamFile;
    u8* pBuffers;
    u8* pSector;
    u8* pHeader;
    s32 nSectors;
    s32 i;
    s32 index;
    s32 nextIndex;
    char* pFilePath;

    pStreamFile = (u8*)g_ArchiveCurStreamFile;
    if (pStreamFile == NULL) {
        return 0;
    }

    nSectors = *(s32*)pStreamFile;
    pBuffers = pStreamFile + (nSectors * 8) + STREAM_FILE_HEADER_SIZE;

    if (g_ArchiveDebugTable == 0) {
        for (i = 0; i < nSectors; i++) {
            if (*(u16*)(pStreamFile + 4 + (i << 3)) == 3 &&
                *(u16*)(pStreamFile + 4 + (i << 3) + 2) == D_8004FE24) {
                break;
            }
        }
        if (i == D_8004FE40) {
            return 0;
        }
        D_8004FE24 += 1;
        return (s32)(uintptr_t)(pBuffers + (i << 11));
    }

    if (D_8004FE4C == -1) {
        return 0;
    }

    if (g_ArchiveCurFileSize <= 0) {
        return 0;
    }

    index = 0;
    if (D_8004FE40 > 0) {
        i = 0;
        do {
            index = D_8004FE10;
            pHeader = (u8*)D_8004FE2C + (index << 3);
            D_8004FE10 = index + 1;
            if (D_8004FE10 >= D_8004FE40) {
                D_8004FE10 = 0;
            }
            i++;
            if (*(u16*)pHeader == ARCHIVE_STREAM_FILE_NOT_LOADED) {
                break;
            }
        } while (i < D_8004FE40);
    } else {
        pHeader = pStreamFile + 4;
    }

    if (*(u16*)pHeader != 0) {
        return 0;
    }

#ifdef LIBARCHIVE_28B14_MUTANT_MARK_STATE2
    *(u16*)pHeader = 2;
#else
    *(u16*)pHeader = 3;
#endif
    pSector = pBuffers + (index << 11);

    i = 0;
    while (PCread(D_8004FE4C, (char*)pSector, CD_SECTOR_SIZE) == 0) {
        func_8002804C(i, 0, 0xFF, 0);
        i++;
        if (i >= RETRY_COUNT) {
            return 0;
        }
    }

    g_ArchiveCurFileSize -= CD_SECTOR_SIZE;
    if (g_ArchiveCurFileSize > 0) {
        return (s32)(uintptr_t)pSector;
    }
    g_ArchiveCurFileSize = 0;

    for (i = 0; i < RETRY_COUNT; i++) {
        if (PCclose(D_8004FE4C) == 0) {
            break;
        }
        func_8002804C(i, 0, 0, 0xFF);
    }

    D_8004FE4C = -1;
    if (D_8004FE0C == 0) {
        D_8004FDFC = 0;
        return (s32)(uintptr_t)pSector;
    }

    index = D_8004FE10 + 1;
    pHeader = (u8*)D_8004FE0C + (index << 3);
    nextIndex = *(u16*)pHeader;
    D_8004FE10 = index;
    D_80059F0C = nextIndex;

    if (nextIndex <= 0 || *(s32*)(pHeader + 4) == 0) {
        g_ArchiveCurFileSize = 0;
        D_8004FDFC = 0;
        return (s32)(uintptr_t)pSector;
    }

    pFilePath = ArchiveGetFilePath(nextIndex);

    for (i = 0; i < RETRY_COUNT; i++) {
        D_8004FE4C = PCopen(pFilePath, 0, 0);
        if (D_8004FE4C != -1) {
            break;
        }
        func_8002804C(i, 0xFF, 0, 0);
    }

    g_ArchiveCurFileSize = ArchiveDecodeSizeAligned(nextIndex);
    D_8004FDFC -= 1;
    return (s32)(uintptr_t)pSector;
}
#endif

int func_80028E60(int sectionIndex, int maxEntries, int targetState) {
    int i;
    ArchiveStreamFileSectionHeader* pSectionHeader;
    int nSections;
    
    i = 0;
    while (i < maxEntries) {
        pSectionHeader = D_8004FE2C + sectionIndex;
        nSections = D_8004FE40;

        while (1) {
            if (pSectionHeader->state == targetState) {
                sectionIndex++;
                pSectionHeader++;
                
                if (nSections < sectionIndex)
                    return 1;
                
                i++;
                
                if (i >= maxEntries)
                    return 0;
                
                continue;
            }
            return 1;
        }
    }

    return 0;
}

void ArchiveConsolidateStreamFileEntry(int sectionIndex) {
    short nSize;
    int nNextIndex;
    ArchiveStreamFileSectionHeader* pSectionHeader;
    ArchiveStreamFileSectionHeader* pNextSectionHeader;

    pSectionHeader = D_8004FE2C + sectionIndex;
    nSize = pSectionHeader->size;
    nNextIndex = sectionIndex + nSize;
    
    if (nNextIndex < D_8004FE40) {
        pNextSectionHeader = D_8004FE2C + nNextIndex;
        if (pNextSectionHeader->state == ARCHIVE_STREAM_FILE_NOT_LOADED) {
            pSectionHeader->size += pNextSectionHeader->size;
        }
    }
}

extern s8* D_8004FE08;
extern u16 D_8004FE26;
extern u8* D_80059F54;
extern u8* D_80059F58;
extern s16 D_80059F5C;
extern s16 D_80059F60;
extern u16 D_8005A4B8;
extern u8 D_800596F8[];

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/libarchive", func_80028F30);
#else
/* Output slots retain the retail 32-bit address ABI. Native callers must
 * receive them in u32 locals and widen explicitly, not pass void**. Port
 * archive buffers are allocated in the existing low-address heap. */
/* Transcribed from asm/slus_006.64/nonmatchings/system/libarchive/func_80028F30.s
 * (0x80028F30-0x8002945C, frame 0x50). Debug-table streamed frame fetch used by
 * the movie player: returns 0 and publishes *ppSection / *ppData for the oldest
 * completed ring slot, or 1 when no frame is complete.
 *   stream NULL -> 1 (out-params untouched).
 *   Debug section fetch (g_ArchiveDebugTable != 0, hFile != -1,
 *   g_ArchiveCurFileSize > 0):
 *     D_80059F60 == 0 (new section): walk D_8004FE2C from index 0 adding
 *     slot sizes until a NOT_LOADED slot or D_8004FE40 is reached; no room ->
 *     no data. In CD mode bit 3 of D_80059F18[0], first PCread 8 bytes into
 *     D_800596F8 and bail to a 0x918-byte seek when its first byte is 1.
 *     Otherwise PCread 0x20 into D_8004FE08 + slot*0x800 (saved to D_80059F54),
 *     latch D_80059F5C = u16 +6 and D_8005A4B8 = u16 +8 of that header, and
 *     when the slot's size (+4) is smaller than D_80059F5C seek back
 *     (-0x28 CD / -0x20 otherwise) and report no data. Else mark the slot
 *     (state 3, id D_8004FE26), split it when the remainder is >= 3
 *     (size = D_80059F5C + 1, zero the +8 sub-field at D_80059F5C*8 and store
 *     the remainder at +0xC, then ArchiveConsolidateStreamFileEntry), PCread
 *     the 0x7E0 payload into D_80059F58 = sector + D_80059F5C*0x20, bump
 *     D_8004FE26, optional 0x118 seek, D_8004FE10 = slot, D_80059F60++ and
 *     g_ArchiveCurFileSize -= 0x800.
 *     D_80059F60 != 0 (next sub-frame): same 8-byte CD probe, then mark slot
 *     D_8004FE10 + 1 (state 3, id D_8004FE26), PCread 0x20 into
 *     D_80059F54 + D_80059F60*0x20 and PCread 0x7E0 into
 *     D_80059F58 + D_80059F60*0x7E0, optional 0x118 seek, D_80059F60++ and
 *     g_ArchiveCurFileSize -= 0x800, wrapping D_80059F60 to 0 once it reaches
 *     D_80059F5C.
 *   Both paths (and every CD/no-file/empty path) then clear *ppData and scan
 *   the ring for state 3 + id D_8004FE24: no match -> 1; else *ppData = sector,
 *   *ppSection = sector + u16(+6)*0x20, and when
 *   func_80028E60(slot, u16(+6), 3) == 0, D_8004FE24 += u16(+6) and return 0. */
int func_80028F30(u32* ppSection, u32* ppData) {
    u8* pStreamFile;
    u8* pHeaders;
    u8* pBuffers;
    u8* pSector;
    u8* pDst;
    u8* pSlot;
    s32 nSectors;
    s32 i;
    s32 index;
    s32 size;
    s32 frame;

    pStreamFile = (u8*)g_ArchiveCurStreamFile;
    if (pStreamFile == NULL) {
        return 1;
    }

    nSectors = *(s32*)pStreamFile;
    pHeaders = pStreamFile + 4;
    pBuffers = pStreamFile + (nSectors * 8) + STREAM_FILE_HEADER_SIZE;

    if (g_ArchiveDebugTable != 0 && D_8004FE4C != -1 && g_ArchiveCurFileSize > 0) {
        if (D_80059F60 == 0) {
            index = 0;
            if (D_8004FE40 > 0) {
                do {
                    pSlot = (u8*)D_8004FE2C + (index << 3);
                    if (*(u16*)pSlot == 0) {
                        break;
                    }
                    index += *(u16*)(pSlot + 4);
                } while (index < D_8004FE40);
            }
            if (index >= D_8004FE40) {
                goto no_data;
            }

            if (D_80059F18[0] & 8) {
                pDst = D_800596F8;
                PCread(D_8004FE4C, (char*)pDst, 8);
                if (*(u8*)pDst == 1) {
                    goto seek_918;
                }
            }

            pDst = (u8*)(D_8004FE08 + (index << 11));
            D_80059F54 = pDst;
            PCread(D_8004FE4C, (char*)pDst, 0x20);
            D_80059F5C = *(u16*)(pDst + 6);
            D_8005A4B8 = *(u16*)(pDst + 8);
            if (*(u16*)(pSlot + 4) < (s16)D_80059F5C) {
                if (D_80059F18[0] & 8) {
                    PClseek(D_8004FE4C, -0x28, 1);
                } else {
                    PClseek(D_8004FE4C, -0x20, 1);
                }
                goto no_data;
            }

            frame = (s16)D_80059F5C;
            *(u16*)pSlot = 3;
            *(u16*)(pSlot + 2) = D_8004FE26;
            size = *(u16*)(pSlot + 4) - frame;
            if (size >= 3) {
                *(u16*)(pSlot + 4) = frame + 1;
                *(u16*)(pSlot + 8 + (frame << 3)) = 0;
                *(u16*)(pSlot + 0xC + (frame << 3)) = size - 1;
                ArchiveConsolidateStreamFileEntry(frame + 1);
            }

#ifdef LIBARCHIVE_28F30_MUTANT_PAYLOAD_SHIFT4
            D_80059F58 = (u8*)(D_8004FE08 + (index << 11)) + (frame << 4);
#else
            D_80059F58 = (u8*)(D_8004FE08 + (index << 11)) + (frame << 5);
#endif
            D_8004FE26 = D_8004FE26 + 1;
            PCread(D_8004FE4C, (char*)D_80059F58, 0x7E0);
            if (D_80059F18[0] & 8) {
                PClseek(D_8004FE4C, 0x118, 1);
            }

            D_8004FE10 = index;
            D_80059F60 = D_80059F60 + 1;
            g_ArchiveCurFileSize -= CD_SECTOR_SIZE;
            goto no_data;
        }

        if (D_80059F18[0] & 8) {
            pDst = D_800596F8;
            PCread(D_8004FE4C, (char*)pDst, 8);
            if (*(u8*)pDst == 1) {
                goto seek_918;
            }
        }

        index = D_8004FE10 + 1;
        pSlot = (u8*)D_8004FE2C + (index << 3);
        D_8004FE10 = index;
        *(u16*)pSlot = 3;
        *(u16*)(pSlot + 2) = D_8004FE26;
        D_8004FE26 = D_8004FE26 + 1;
        PCread(D_8004FE4C, (char*)(D_80059F54 + (D_80059F60 << 5)), 0x20);
        PCread(D_8004FE4C, (char*)(D_80059F58 + (D_80059F60 * 0x7E0)), 0x7E0);
        if (D_80059F18[0] & 8) {
            PClseek(D_8004FE4C, 0x118, 1);
        }

        g_ArchiveCurFileSize -= CD_SECTOR_SIZE;
        D_80059F60 = D_80059F60 + 1;
        if ((s16)D_80059F60 >= (s16)D_80059F5C) {
            D_80059F60 = 0;
        }
        goto no_data;

    seek_918:
        PClseek(D_8004FE4C, 0x918, 1);
    }

no_data:
    *ppData = 0;

    for (i = 0; i < nSectors; i++) {
        if (*(u16*)(pHeaders + (i << 3)) == 3 &&
            *(u16*)(pHeaders + (i << 3) + 2) == D_8004FE24) {
            break;
        }
    }
    if (i == D_8004FE40) {
        return 1;
    }

    pSector = pBuffers + (i << 11);
    *ppData = (u32)(uintptr_t)pSector;
    *ppSection = (u32)(uintptr_t)(pSector + (*(u16*)(pSector + 6) << 5));
    if (func_80028E60(i, *(u16*)(pSector + 6), 3) != 0) {
        return 1;
    }
    D_8004FE24 += *(u16*)(pSector + 6);
    return 0;
}
#endif

s32 func_8002945C(u8* pSlot) {
    void* pStreamFile = g_ArchiveCurStreamFile;
    u8* pTable;
    s32 slotIdx;
    u16 result;

    if (pStreamFile == NULL) return 0xFFFF;
    if (pSlot == NULL) return 0;
    pTable = (u8*)pStreamFile + 4;
    {
        /* retail: unsigned offset of the slot from the data area */
        u8* pData = (u8*)pStreamFile + *(s32*)pStreamFile * 8 + 0x24;
        slotIdx = (u32)(pSlot - pData) >> 11;
    }
    result = *(u16*)(pTable + slotIdx * 8);
    *(u16*)(pTable + slotIdx * 8) = 0;
    return result;
}

extern void* g_ArchiveCurStreamFile;

s32 func_800294B4(void* pSlot) {
    void* pStreamFile = g_ArchiveCurStreamFile;
    u8* pTable;
    s32 slotIdx, count, i;
    u16 result;
    if (pStreamFile == NULL) return 0xFFFF;
    if (pSlot == NULL) return 0;
    pTable = (u8*)pStreamFile + 4;
    {
        /* retail: unsigned offset of the slot from the data area */
        u8* pData = (u8*)pStreamFile + *(s32*)pStreamFile * 8 + 0x24;
        slotIdx = (u32)((u8*)pSlot - pData) >> 11;
    }
    result = *(u16*)(pTable + slotIdx * 8);
    count = *(u16*)((u8*)pSlot + 6);
    for (i = count; i > 0; i--) {
        *(u16*)(pTable + (slotIdx + i) * 8 - 8) = 0;
    }
    /* takes the section index (retail passes slotIdx, not the slot pointer) */
    ArchiveConsolidateStreamFileEntry(slotIdx);
    return result;
}

s32 ArchiveReadFileFromCdSector(s32 sector, void* pDestBuffer, s32 fileSize, s32 arg3, u32 flags) {
    if (g_ArchiveDebugTable) {
        return -1;
    }
    
    ArchiveCdDataSync(0);
    g_ArchiveCurFileSector = sector;
    g_ArchiveCurFileSize = fileSize;
    return ArchiveReadFile(0, pDestBuffer, arg3, flags);
}

s32 ArchiveReadFileToBuffer(s32 index, void* pBuffer, u32 arg2, u32 flags) {
    if ((index <= 0) || (ArchiveDecodeSize(index) <= 0) || (pBuffer == NULL)) {
        return -3;
    }
    ArchiveCdDataSync(0);
    D_8004FE18 = g_CurArchiveOffset;
    g_ArchiveCurFileSector = ArchiveDecodeSector(index);
    g_ArchiveCurFileSize = ArchiveDecodeAlignedSize(index);
    return ArchiveReadFile(index, pBuffer, arg2, flags);
}
