/*
 * Archive code is split into two C files due to a compiler mismatch.
 * libarchive.c contains two functions which seemingly only compiles on GCC 2.6.X.
 * Meanwhile, ArchiveReadFile only seemingly compiles on GCC 2.7.2.
 * This suggests that archive.c is compiled on its own, maybe as a library.
*/

#include "common.h"
#ifndef XENO_PC_PORT
#include "system/memory.h"
#endif
#include "system/archive.h"
#include "psyq/pc.h"
#include "psyq/libcd.h"
#ifndef XENO_PC_PORT
s32 ArchiveDataSync();
extern s32 ArchiveCdDriveCommandHandler__a_s32[] asm("ArchiveCdDriveCommandHandler");
extern s32 D_8004FE18__a_s32[] asm("D_8004FE18");
extern s32 g_ArchiveCdCurLocation__a_s32[] asm("g_ArchiveCdCurLocation");
extern s32 g_ArchiveCdDriveState__a_s32[] asm("g_ArchiveCdDriveState");
extern s32 g_ArchiveDebugTable__a_s32[] asm("g_ArchiveDebugTable");
extern s32 g_CurArchiveOffset__a_s32[] asm("g_CurArchiveOffset");
void func_8002A2D0(s32 arg0);
#endif

#ifdef XENO_PC_PORT
#include <stdint.h>
#endif

extern s8* D_8004FE08;
extern s32 D_8004FE0C;
extern s32 D_8004FE10;
extern s16 D_8004FE24;
extern s16 D_8004FE26;
extern s16 D_8004FE28;
extern void* g_ArchiveCurStreamFile; // Current stream file struct
extern s32 D_80059F00;
extern u32 D_80059F0C;
extern s16 D_80059F60;
extern s32 D_8005A4DC;
extern void func_8002B084();
extern void func_8002B2F0();
extern void func_8002BA58();
extern void func_8002BA40(void);
extern void func_8002AC24(void);
extern void func_8002804C(s32, s32, s32, s32);
extern int ArchiveDecodeSizeAligned(int entryIndex);
extern s32 g_ArchiveCurFileSector;
extern s32 D_8004FE00;
extern s32 D_8004FE3C;
extern s32 D_80059EF8;
extern s32 D_80059F04;
extern u16 D_80059F24;
extern u16 D_80059F28;
extern u16 D_80059F2C;
extern u16 D_80059F30;
extern u16 D_80059F34;
extern u16 D_80059F38;
extern s32 D_80059F3C;
extern u16 D_80059F40;
extern u16 D_80059F44;
extern u16 D_80059F48;
extern s32 D_80059F4C;
extern s32 D_80059F50;
extern void func_8002B8B0(s32, s32);
extern void func_8002BF38(s32, s32);
extern void func_8002BB50(void);
extern void func_8002B5D0(void);

// Only matches on GCC 2.7.2
int ArchiveReadFile(u32 dbgEntryIndex, u8* pDestBuffer, s32 arg2, s32 flags) {
    void* pReadyCallback;
    int nStreamSectors;
    int nStreamSectors2; // The same as nStreamSectors, doesn't match unless it's two variables
    s32 hFile;
    int i;
    char* pFilePath;
    char* pFilePath2; // The same as nStreamSectors, doesn't match unless it's two variables
    u8* pCdMode;
    u32* pData;
    CdlLOC* pLoc;

    D_80059F0C = dbgEntryIndex;
    for (i = 2, pData = &D_80059F00; i >= 0; i--)
        *pData-- = 0;
    
    D_8004FDFC = 1;
    D_8004FE08 = pDestBuffer;
    D_8004FE38 = arg2 & 0xFFFF;
    D_8004FE10 = 0;
    D_8004FE0C = 0;
    D_8004FE34 = 0;
    D_8005A4DC = 0;
    pLoc = &g_ArchiveCdCurLocation;
    CdIntToPos(g_ArchiveCurFileSector, pLoc);
    
    if (flags & CdlModeStream) {
        ArchiveChangeStreamingFile(pDestBuffer);
        nStreamSectors = *(u32*)g_ArchiveCurStreamFile;
        if (nStreamSectors) {
            D_8004FE08 = g_ArchiveCurStreamFile + nStreamSectors * sizeof(ArchiveStreamFileSectionHeader) + STREAM_FILE_HEADER_SIZE;
            D_8004FE2C = g_ArchiveCurStreamFile + 4;
            D_8004FE40 = nStreamSectors;
            D_8004FE26 = 0;
            D_8004FE28 = 0;
            D_8004FE24 = 0;
            ArchiveClearStreamFileSections();
            
            if (g_ArchiveDebugTable) {
                pFilePath2 = ArchiveGetFilePath(dbgEntryIndex);
                
                for (i = 0; i < RETRY_COUNT; i++) {
                    D_8004FE4C = PCopen(pFilePath2, 0, 0);
                    if (D_8004FE4C == -1)
                        func_8002804C(i, 0xFF, 0, 0);
                    else
                        break;
                }

                return -(~D_8004FE4C == 0) & -3;
            }

            g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_READ_SECTOR;
            CdDataCallback(&func_8002BA58);
            CdSyncCallback(&ArchiveCdDriveCommandHandler);
            pReadyCallback = &func_8002B2F0;
            CdReadyCallback(pReadyCallback);
            D_8005A488 += 1;
            CdControlF(CdlSetloc, (void*)pLoc);
            return 0;
        }
        return -4;
    }
    
    if (flags & 0x200) {
        ArchiveChangeStreamingFile(pDestBuffer);
        nStreamSectors2 = *(u32*)g_ArchiveCurStreamFile;
        if (nStreamSectors2) {
            D_8004FE08 = g_ArchiveCurStreamFile + (nStreamSectors2 * sizeof(ArchiveStreamFileSectionHeader)) + STREAM_FILE_HEADER_SIZE;
            D_8004FE2C = g_ArchiveCurStreamFile + 4;
            D_8004FE40 = nStreamSectors2;
            D_80059F60 = 0;
            D_8004FE26 = 0;
            D_8004FE28 = 0;
            D_8004FE24 = 0;
            ArchiveClearStreamFileSections();
            
            pCdMode = &D_80059F1B;
            for (i = 3; i >= 0; i--)
                *pCdMode-- = 0;

            // CD Mode - double speed, 2340 byte size
            D_80059F18[0] = flags | CdlModeSpeed | CdlModeSize1;
            
            if (g_ArchiveDebugTable) {
                pFilePath2 = ArchiveGetFilePath(dbgEntryIndex);

                for (i = 0; i < RETRY_COUNT; i++) {
                    D_8004FE4C = PCopen(pFilePath2, 0, 0);
                    if (D_8004FE4C == -1)
                        func_8002804C(i, 0xFF, 0, 0);
                    else
                        break;
                }
                
                return -(~D_8004FE4C == 0) & -3;
            }
            return 0;
        }
        return -4;
    }
    
    if (g_ArchiveDebugTable) {
        pFilePath = ArchiveGetFilePath(dbgEntryIndex);

        for (i = 0; i < RETRY_COUNT; i++) {
            hFile = PCopen(pFilePath, 0, 0);
            if (hFile == -1)
                func_8002804C(i, 0xFF, 0, 0);
            else
                break;
        }

        if (hFile == -1) {
            return -4;
        }

        if (pDestBuffer != NULL) {
            for (i = 0; i < RETRY_COUNT; i++) {
                if (PCread(hFile, pDestBuffer, g_ArchiveCurFileSize) == 0)
                    func_8002804C(i, 0, 0xFF, 0);
                else
                    break;
            }
        }
        
        for (i = 0; i < RETRY_COUNT; i++) {
            if (PCclose(hFile) != 0) {
                func_8002804C(i, 0, 0, 0xFF);
            } else {
                g_ArchiveCurFileSize = 0;
                D_8004FDFC = 0;
                return 0;
            }
        }

        return -6;
    }
    
    g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_READ_SECTOR;
    CdDataCallback(NULL);
    CdSyncCallback(&ArchiveCdDriveCommandHandler);
    pReadyCallback = &func_8002B084;
    CdReadyCallback(pReadyCallback);
    D_8005A488 += 1;
    CdControlF(CdlSetloc, (void*)pLoc);
    return 0;
}

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/archive", func_80029AFC);
#else
/* Transcribed from asm/slus_006.64/nonmatchings/system/archive/func_80029AFC.s
 * (0x80029AFC-0x80029EAC). Count/sort 8-byte StreamDataQueueEntry records
 * until archiveIndex==0. Empty/NULL returns -3. Selection-sort by unsigned
 * index. Then ArchiveCdDataSync(0), snapshot g_CurArchiveOffset, zero
 * D_80059F00 and the two words below it. If first index or pData is 0:
 * ArchiveCdSeekToFile(arg1) and clear size/count. Else decode sector/size,
 * CdIntToPos. Debug table: PCopen/PCread each entry. Else arm CD
 * READ_SECTOR with func_8002BA40 / ArchiveCdDriveCommandHandler /
 * func_8002AC24 and CdControlF(CdlSetloc). Frame 0x40. */
int func_80029AFC(StreamDataQueueEntry* pEntries, int arg1, int arg2) {
    s32 count;
    s32 i;
    s32 j;
    s32 minIndex;
    u16 minVal;
    u16 tmpIndex;
    void* tmpData;
    u32* pData;
    s32 hFile;
    s32 retry;
    char* pPath;
    StreamDataQueueEntry* pCur;
    CdlLOC* pLoc;

    (void)arg2;

    if (pEntries == NULL) {
        return -3;
    }
    if (pEntries[0].archiveIndex == 0) {
        return -3;
    }

    count = 0;
    pCur = pEntries;
    do {
        pCur += 1;
        count += 1;
    } while (pCur->archiveIndex != 0);

    if (count - 1 > 0) {
        for (i = 0; i < count - 1; i++) {
            minIndex = i;
            minVal = (u16)pEntries[i].archiveIndex;
            for (j = i + 1; j < count; j++) {
                if ((u16)pEntries[j].archiveIndex < minVal) {
                    minIndex = j;
                    minVal = (u16)pEntries[j].archiveIndex;
                }
            }
#ifdef ARCHIVE_29AFC_MUTANT_SKIP_SORT
            (void)minIndex;
#else
            tmpIndex = (u16)pEntries[minIndex].archiveIndex;
            tmpData = pEntries[minIndex].pData;
            pEntries[minIndex].archiveIndex = pEntries[i].archiveIndex;
            pEntries[minIndex].pData = pEntries[i].pData;
            pEntries[i].archiveIndex = (s16)tmpIndex;
            pEntries[i].pData = tmpData;
#endif
        }
    }

    ArchiveCdDataSync(0);
    D_8004FE18 = g_CurArchiveOffset;
    for (i = 2, pData = (u32*)&D_80059F00; i >= 0; i--) {
        *pData-- = 0;
    }

    D_8004FE10 = 0;
#ifdef XENO_PC_PORT
    D_8004FE0C = (s32)(uintptr_t)pEntries;
#else
    D_8004FE0C = (s32)pEntries;
#endif
    D_8004FDFC = count;
    D_8004FE00 = count;
    D_8004FE08 = pEntries[0].pData;

    if (pEntries[0].archiveIndex == 0 || pEntries[0].pData == NULL) {
        ArchiveCdSeekToFile(arg1);
        g_ArchiveCurFileSize = 0;
        D_8004FDFC = 0;
        return 0;
    }

    D_80059F0C = (u16)pEntries[0].archiveIndex;
    g_ArchiveCurFileSector = ArchiveDecodeSector((u16)pEntries[0].archiveIndex);
    g_ArchiveCurFileSize = ArchiveDecodeSizeAligned((u16)pEntries[0].archiveIndex);
    D_8004FE38 = arg1 & 0xFFFF;
    D_8004FE3C = 0;
    D_8004FE34 = 0;
    D_8005A4DC = 0;
    pLoc = &g_ArchiveCdCurLocation;
    CdIntToPos(g_ArchiveCurFileSector, pLoc);

    if (g_ArchiveDebugTable != 0) {
        for (i = 0; i < count; i++) {
            D_80059F0C = (u16)pEntries[i].archiveIndex;
            pPath = ArchiveGetFilePath((u16)pEntries[i].archiveIndex);
            hFile = -1;
            for (retry = 0; retry < RETRY_COUNT; retry++) {
                hFile = PCopen(pPath, 0, 0);
                if (hFile != -1) {
                    break;
                }
                func_8002804C(retry, 0xFF, 0, 0);
            }
            if (hFile != -1 && pEntries[i].pData != NULL) {
                for (retry = 0; retry < RETRY_COUNT; retry++) {
                    if (PCread(hFile, (char*)pEntries[i].pData,
                               ArchiveDecodeSizeAligned(
                                   (u16)pEntries[i].archiveIndex)) != 0) {
                        break;
                    }
                    func_8002804C(retry, 0, 0xFF, 0);
                }
            }
            for (retry = 0; retry < RETRY_COUNT; retry++) {
                if (PCclose(hFile) == 0) {
                    break;
                }
                func_8002804C(retry, 0, 0, 0xFF);
            }
        }
        g_ArchiveCurFileSize = 0;
        D_8004FDFC = 0;
        return 0;
    }

    g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_READ_SECTOR;
    CdDataCallback(&func_8002BA40);
    CdSyncCallback(&ArchiveCdDriveCommandHandler);
    CdReadyCallback(&func_8002AC24);
    D_8005A488 += 1;
    CdControlF(CdlSetloc, (void*)pLoc);
    return 0;
}
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/archive", func_80029EB0);
#else
/* Transcribed from asm/slus_006.64/nonmatchings/system/archive/func_80029EB0.s
 * (0x80029EB0-0x8002A260). Arms the streaming-section decoder for one archive
 * file: ArchiveCdDataSync + snapshot, zero the stream words below D_80059F00,
 * ArchiveChangeStreamingFile, latch sector/size/buffer pointers, publish the
 * mode registers D_80059F24-F38 (from the trailing u16 arguments), then either
 * the debug-table path (PCopen retry, func_8002B8B0/func_8002BF38 pump until
 * D_8004FDFC drains, PCclose retry) or the CD path (READ_SECTOR callbacks,
 * CdControlF(CdlSetloc, &g_ArchiveCdCurLocation)). Frame 0x50. */
int func_80029EB0(s32 archiveIndex, s32* pStreamFile, s32 arg2, s32 arg3, u16 arg4, u16 arg5,
                  u16 arg6, u16 arg7, u16 arg8, u16 arg9)
{
    int nStreamSectors;
    s16 i;
    s32* pWord;
    char* pFilePath;
    CdlLOC* pLoc;

    (void)arg3;

    if (pStreamFile == NULL)
        return -4;

    nStreamSectors = *pStreamFile;
    if (nStreamSectors < 2)
        return -4;

    if (archiveIndex <= 0)
        return -3;

    if (ArchiveDecodeSize(archiveIndex) <= 0)
        return -3;

    ArchiveCdDataSync(0);
    D_8004FE18 = g_CurArchiveOffset;

    for (i = 0, pWord = &D_80059EF8; i < 3; i++)
        pWord[i] = 0;

    ArchiveChangeStreamingFile(pStreamFile);
    D_80059F0C = archiveIndex;
    g_ArchiveCurFileSector = ArchiveDecodeSector(archiveIndex);
    g_ArchiveCurFileSize = ArchiveDecodeAlignedSize(archiveIndex);
    D_8004FE08 = (s8*)pStreamFile + (nStreamSectors * 8) + STREAM_FILE_HEADER_SIZE;
    D_8004FE2C = (ArchiveStreamFileSectionHeader*)((u8*)pStreamFile + 4);
#ifdef ARCHIVE_29EB0_MUTANT_SWAP_MODE_XY
    D_80059F30 = arg8;
    D_80059F34 = arg7;
#else
    D_80059F30 = arg7;
    D_80059F34 = arg8;
#endif
    D_8004FDFC = 1;
    D_8004FE38 = arg2 & 0xFFFF;
    D_8004FE10 = 0;
    D_8004FE40 = nStreamSectors;
    D_8004FE26 = 0;
    D_8004FE28 = 0;
    D_8004FE0C = 0;
    D_8004FE34 = 0;
    D_8005A4DC = 0;
    D_80059F24 = arg4;
    D_80059F28 = arg5;
    D_80059F2C = arg6;
    D_80059F3C = 0;
    D_80059F40 = 0;
    D_80059F44 = 0;
    D_80059F48 = 0;
    D_80059F4C = 0;
    D_80059F50 = 0;
    D_80059F38 = arg9;
    ArchiveClearStreamFileSections();

    pLoc = &g_ArchiveCdCurLocation;
    CdIntToPos(g_ArchiveCurFileSector, pLoc);

    if (g_ArchiveDebugTable) {
        pFilePath = ArchiveGetFilePath(archiveIndex);

        for (i = 0; i < RETRY_COUNT; i++) {
            D_80059F04 = PCopen(pFilePath, 0, 0);
            if (D_80059F04 != -1)
                break;
            func_8002804C(i, 0xFF, 0, 0);
        }

        do {
            func_8002B8B0(0, 0);
            func_8002BF38(0, 0);
        } while (D_8004FDFC > 0);

        while (i = PCclose(D_80059F04)) {
            func_8002804C(i, 0, 0, 0xFF);
            if (i + 1 < RETRY_COUNT)
                continue;
            break;
        }

        if (i != 0)
            return -6;

        D_8004FDFC = 0;
        g_ArchiveCurFileSize = 0;
        return 0;
    }

    g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_READ_SECTOR;
    CdDataCallback(&func_8002BB50);
    CdSyncCallback(&ArchiveCdDriveCommandHandler);
    CdReadyCallback(&func_8002B5D0);
    D_8005A488 += 1;
    CdControlF(CdlSetloc, (u_char*)pLoc);
    return 0;
}
#endif

int* ArchiveAllocStreamFile(int numEntries, int allocMode) {
    int* pStreamFile;

    if (numEntries > 0) {
        pStreamFile = HeapAlloc((numEntries * (CD_SECTOR_SIZE + sizeof(ArchiveStreamFileSectionHeader))) + STREAM_FILE_HEADER_SIZE, allocMode);
        if (pStreamFile) {
            *pStreamFile = numEntries;
            ArchiveChangeStreamingFile(pStreamFile);
            ArchiveClearStreamFileSections();
            return pStreamFile;
        }
    }
    return NULL;
}

#ifndef XENO_PC_PORT
/* retail func_8002A2D0; the port names it ArchiveCdSeekOrPause */

#ifndef XENO_PC_PORT
void func_8002A2D0(s32 arg0) {
    s32 *var_a1;
    s32 var_a0;

    if ((g_ArchiveDebugTable__a_s32[0] == 0) && (ArchiveDataSync() == 0)) {
        D_8004FE18__a_s32[0] = g_CurArchiveOffset__a_s32[0];
        if (arg0 > 0) {
            CdIntToPos(ArchiveDecodeSector(arg0), g_ArchiveCdCurLocation__a_s32);
            g_ArchiveCdDriveState__a_s32[0] = 3;
            CdSyncCallback(ArchiveCdDriveCommandHandler__a_s32);
            var_a0 = 2;
            var_a1 = g_ArchiveCdCurLocation__a_s32;
        } else {
            g_ArchiveCdDriveState__a_s32[0] = 5;
            CdSyncCallback(ArchiveCdDriveCommandHandler__a_s32);
            var_a0 = 9;
            var_a1 = NULL;
        }
        CdControlF(var_a0, var_a1);
    }
}
#endif

#else
void ArchiveCdSeekOrPause(int entryIndex) {
    u_char nCommand;
    u_char* nParam;
    int nSector;
    CdlLOC* pCdLocation;

    if ((g_ArchiveDebugTable == 0) && (ArchiveDataSync() == 0)) {
        D_8004FE18 = g_CurArchiveOffset;
        if (entryIndex > 0) {
            pCdLocation = &g_ArchiveCdCurLocation;
            nSector = ArchiveDecodeSector(entryIndex);
            CdIntToPos(nSector, pCdLocation);
            g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_SEEK;
            CdSyncCallback(&ArchiveCdDriveCommandHandler);
            nCommand = CdlSetloc;
            nParam = (u_char*)pCdLocation;
        } else {
            g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_DONE;
            CdSyncCallback(&ArchiveCdDriveCommandHandler);
            nCommand = CdlPause;
            nParam = NULL;
        }

        CdControlF(nCommand, nParam);
    }
}
#endif

void ArchiveCdSeekToFile(int entryIndex) {
    u_char nCommand;
    u_char* nParam;
    int nSector;
    CdlLOC* pCdLocation;

    if (entryIndex > 0) {
        pCdLocation = &g_ArchiveCdCurLocation;
        nSector = ArchiveDecodeSector(entryIndex);
        CdIntToPos(nSector, pCdLocation);
        g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_SEEK;
        CdSyncCallback(&ArchiveCdDriveCommandHandler);
        nCommand = CdlSetloc;
        nParam = &g_ArchiveCdCurLocation;
    } else {
        g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_DONE;
        CdSyncCallback(&ArchiveCdDriveCommandHandler);
        nCommand = CdlPause;
        nParam = NULL;
    }

    CdControlF(nCommand, nParam);
}

void ArchiveCdSetMode(u_char mode) {
    int i;
    u_char* pParam;

    g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_RESET_MODE;
    CdSyncCallback(&ArchiveCdDriveCommandHandler);
    for (i = 3, pParam = &D_80059F1B; i >= 0; i -= 1) {
        *pParam-- = 0;
    }
    
    D_80059F18[0] = mode;
    CdControlF(CdlSetmode, &D_80059F18);
}

void func_8002A498(int channel) {
    int i;

    D_8004FE34 = 1;
    D_8004FE38 = channel;
    if (g_ArchiveDebugTable) {
        g_ArchiveCurFileSize = 0;
        D_8004FDFC = 0;

        // Close (debug) streaming file handle
        if (D_8004FE4C != -1) {
            while (i = PCclose(D_8004FE4C)) {
                if (i != 0) {
                    if (i + 1 < 4)
                        continue;
                }
                break;
            }
            D_8004FE4C = -1;
        }
    }
}

void ArchiveStreamFreeQueueData(StreamDataQueueEntry* pEntries) {
    StreamDataQueueEntry* pCurrent;
    void* pData;

    if ((unsigned short)pEntries->archiveIndex != 0) {
        pCurrent = pEntries;
        do {
            pData = pCurrent->pData;
            pCurrent++;
            if (pData) {
                HeapFree(pData);
            }
        } while ((unsigned short)pCurrent->archiveIndex != 0);
    }
}

#ifndef XENO_PC_PORT
/* retail func_8002A57C; the port names it ArchiveStreamAllocQueueData */
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/archive", func_8002A57C);
#else
StreamDataQueueEntry* ArchiveStreamAllocQueueData(int archiveIndex, StreamDataQueueEntry* pEntries) {
    int entryIndex;
    int numEntries;
    int i;
    s32 bHasAllocatedEntries;
    void* pBuffer;

    bHasAllocatedEntries = 0;
    numEntries = (short)ArchiveDecodeSizeAbsolute(archiveIndex);

    if (numEntries > 0) {
        if (pEntries == NULL) {
            pEntries = HeapAlloc((numEntries + 1) * sizeof(StreamDataQueueEntry), 0);
            bHasAllocatedEntries = 1;
            if (pEntries == NULL) {
                return NULL;
            }
        }

        for (i = 0; i < numEntries; i++) {
            entryIndex = archiveIndex + i + 1;
            pEntries[i].archiveIndex = entryIndex;

            pBuffer = HeapAlloc(ArchiveDecodeAlignedSize(entryIndex), 0);
            pEntries[i].pData = pBuffer;
            if (pBuffer == NULL) {
                ArchiveStreamFreeQueueData(pEntries);
                if (bHasAllocatedEntries > 0) {
                    HeapFree(pEntries);
                }
                pEntries = NULL;
                return 0;
            }
        }

        pEntries[numEntries].archiveIndex = 0;
        pEntries[numEntries].pData = NULL;
    } else {
        pEntries = NULL;
    }

    return pEntries;
}
#endif

void ArchiveCdDriveCommandHandler(u_char status, u_char* pResult) {
    char channel;

    switch (g_ArchiveCdDriveState) {
        case ARCHIVE_CD_DRIVE_READ_SECTOR:
            if (status == CdlComplete) {
                D_8005A48C += 1;
                g_ArchiveCdDriveState += 1;
                CdControlF(CdlReadN, NULL);
            } else {
                D_8005A490 += 1;
                D_80059F08 = CdReadyCallback(NULL);
                g_ArchiveCdDriveError = ARCHIVE_CD_DRIVE_ERR_READ;
                g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_ERROR;
                CdControlF(CdlNop, NULL);
            }
            break;

        case ARCHIVE_CD_DRIVE_READ_DONE:
            if (status == CdlComplete) {
                D_8005A48C += 1;
                CdSyncCallback(NULL);
                g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_IDLE;
                return;
            }
            D_8005A490 += 1;
            D_80059F08 = CdReadyCallback(NULL);
            g_ArchiveCdDriveError = ARCHIVE_CD_DRIVE_ERR_READ;
            g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_ERROR;
            CdControlF(CdlNop, NULL);
            break;

        case ARCHIVE_CD_DRIVE_SEEK:
            if (status == CdlComplete) {
                g_ArchiveCdDriveState += 1;
                CdControlF(CdlSeekL, NULL);
            } else {
                g_ArchiveCdDriveError = ARCHIVE_CD_DRIVE_ERR_SEEK;
                g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_ERROR;
                CdControlF(CdlNop, NULL);
            }
            break;
        
        case ARCHIVE_CD_DRIVE_SEEK_DONE:
            if (status == CdlComplete) {
                CdSyncCallback(NULL);
                g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_IDLE;
                return;
            }
            
            g_ArchiveCdDriveError = ARCHIVE_CD_DRIVE_ERR_SEEK;
            g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_ERROR;
            CdControlF(CdlNop, NULL);
            break;

        case ARCHIVE_CD_DRIVE_DONE:
            if (status == CdlComplete) {
                CdSyncCallback(NULL);
                g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_IDLE;
                return;
            }
            
            g_ArchiveCdDriveError = ARCHIVE_CD_DRIVE_ERR_GENERIC;
            g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_ERROR;
            CdControlF(CdlNop, NULL);
            break;

        case ARCHIVE_CD_DRIVE_READ_DATA_SECTOR:
            if (status == CdlComplete) {
                g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_READ_SECTOR;
                D_8005A488 += 1;
                D_8005A494 += 1;
                CdReadyCallback(D_80059F08);
                CdControlF(CdlSetloc, &g_ArchiveCdCurLocation);
            } else {
                g_ArchiveCdDriveError = ARCHIVE_CD_DRIVE_ERR_READ;
                D_8005A498 += 1;
                g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_ERROR;
                CdControlF(CdlNop, NULL);
            }
            break;
        
        case ARCHIVE_CD_DRIVE_HALT:
            if (status == CdlComplete) {
                g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_READ_DATA_SECTOR;
                D_8005A4A8 += 1;
                CdControlF(CdlPause, NULL);
            } else {
                g_ArchiveCdDriveError = ARCHIVE_CD_DRIVE_ERR_HALT;
                g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_ERROR;
                D_8005A4B4 += 1;
                CdControlF(CdlNop, NULL);
            }
            break;

        case ARCHIVE_CD_DRIVE_SET_ADPCM_PLAY_CHANNEL:
            if (status == CdlComplete) {
                g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_READ_ADPCM_SECTOR;
                channel = D_8004FE38;
                D_80059F15 = channel;
                D_80059F14[0] = 1;
                CdControlF(CdlSetfilter, &D_80059F14);
            } else {
                g_ArchiveCdDriveError = ARCHIVE_CD_DRIVE_ERR_ADPCM;
                g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_ERROR;
                CdControlF(CdlNop, NULL);
            }
            break;

        case ARCHIVE_CD_DRIVE_READ_ADPCM_SECTOR:
            if (status == CdlComplete) {
                g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_READ_SECTOR;
                D_8005A488 += 1;
                D_8005A494 += 1;
                CdReadyCallback(D_80059F08);
                CdControlF(CdlSetloc, &g_ArchiveCdCurLocation);
            } else {
                g_ArchiveCdDriveError = ARCHIVE_CD_DRIVE_ERR_ADPCM;
                g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_ERROR;
                D_8005A498 += 1;
                CdControlF(CdlNop, NULL);
            }
            break;

        case ARCHIVE_CD_DRIVE_RESET_MODE:
            if (status == CdlComplete) {
                g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_DONE;
                CdControlF(CdlPause, NULL);
            } else {
                g_ArchiveCdDriveError = ARCHIVE_CD_DRIVE_ERR_RESET_MODE;
                g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_ERROR;
                D_8005A4B4 += 1;
                CdControlF(CdlNop, NULL);
            }
            break;

        case ARCHIVE_CD_DRIVE_ERROR:
            if (status == CdlComplete && !(*pResult & CdlStatShellOpen)) {
                g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_HANDLE_ERROR;
                CdControlF(CdlGetTN, NULL);
            } else {
                g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_ERROR;
                CdControlF(CdlNop, NULL);
            }
            break;
        
        case ARCHIVE_CD_DRIVE_HANDLE_ERROR:
            // Error handler
            if (status == CdlComplete) {
                switch (g_ArchiveCdDriveError) {
                    case ARCHIVE_CD_DRIVE_ERR_SEEK:
                        // Set the target location and retry
                        g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_SEEK;
                        CdControlF(CdlSetloc, &g_ArchiveCdCurLocation);
                        break;
                    case ARCHIVE_CD_DRIVE_ERR_GENERIC:
                        g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_DONE;
                        CdControlF(CdlPause, NULL);
                        break;
                    case ARCHIVE_CD_DRIVE_ERR_READ:
                        // Pause the read and retry
                        g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_READ_DATA_SECTOR;
                        CdControlF(CdlPause, NULL);
                        break;
                    case ARCHIVE_CD_DRIVE_ERR_HALT:
                        g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_HALT;
                        CdControlF(CdlStop, NULL);
                        break;
                    case ARCHIVE_CD_DRIVE_ERR_ADPCM:
                        g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_SET_ADPCM_PLAY_CHANNEL;
                        CdControlF(CdlSetmode, &D_80059F18);
                        break;
                    case ARCHIVE_CD_DRIVE_ERR_RESET_MODE:
                        g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_RESET_MODE;
                        CdControlF(CdlSetmode, &D_80059F18);
                        break;
                    default:
                        return;
                }
            } else {
                g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_ERROR;
                CdControlF(CdlNop, NULL);
            }
            break;
        
        case ARCHIVE_CD_DRIVE_IDLE:
        default:
            return;
    }
}
