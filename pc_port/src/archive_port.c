/*
 * archive_port.c - synchronous CD/archive read path for the PC port.
 *
 * On PSX, the game's archive layer streams files off the CD with an asynchronous
 * state machine: ArchiveReadFile() arms CdReadyCallback/CdSyncCallback +
 * CdControlF(CdlSetloc), and the registered callbacks (func_8002B084 /
 * func_8002BA58 / func_8002B2F0, all still raw MIPS) copy each delivered sector
 * into the destination buffer as the drive spools. ArchiveCdDataSync() then busy-
 * polls g_ArchiveCdDriveState until the callbacks drive it back to IDLE.
 *
 * PsyCross exposes the same libcd sector primitives the port needs
 * (CdControlB(CdlSetloc) seeks, CdRead queues one sector, and CdReadSync(1)
 * advances one sector). We exclude the game's src/.../system/archive.c from the
 * port build (see build_port.sh) and provide the retail archive state machine's
 * CD-facing part here. Ordinary files still use a blocking read; stream files
 * use one queued sector per func_80028B14 poll and the retail section ring.
 * The arithmetic-only archive helpers (ArchiveSetIndex, ArchiveDecode*,
 * ArchiveReadFileToBuffer/FromCdSector) still come from libarchive.c and call
 * straight into this function.
 *
 * The command callback is intentionally kept out of this TU: PsyCross's CD
 * queue has no PSX interrupt callback boundary for the archive's DMA handler.
 * The sector pump below is the equivalent boundary at the retail caller's
 * ArchiveCdDataSync/func_80028B14 polling boundary; it never reports completion
 * until a sector was actually consumed by CdReadSync.
 */

#include "common.h"
#include "system/archive.h"
#include "psyq/pc.h"
#include "psyq/libcd.h"
#include "psyq/libgpu.h"   /* RECT, LoadImage, DrawSync for the 0xBB VRAM uploader */
#include "psx_memory.h"
#include <stdint.h>    /* g_PsxRam / PSX_RAM_SIZE for PSX-layout queue detect */

/* Forward-declare the two host-libc calls used for the partial-sector bounce.
 * Pulling in <stdlib.h> here fails to compile: the game headers (via common.h)
 * leave the TU without int32_t, which glibc's <stdlib.h> then references. memcpy
 * is already declared through common.h (psyq/memory.h). */
extern void* malloc(unsigned long size);
extern void  free(void* ptr);
extern int   printf(const char* format, ...);
extern void* memcpy(void* dest, const void* src, unsigned long size);
extern void* memset(void* dest, int value, unsigned long size);

/* Set by ArchiveReadFileToBuffer/ArchiveReadFileFromCdSector (libarchive.c) just
 * before they call us: the absolute CD sector and the byte length to read. */
extern s32 g_ArchiveCurFileSector;
extern int ArchiveReadFileToBuffer(int entryIndex, void* pDestBuffer, int arg2, int flags);
#include "../include/xg_plat/mods.h"
/* xg_plat/disc.h itself pulls libc headers whose types clash with the
 * psyq ones this TU uses; declare the one call needed (weak, like the mods
 * hooks, so tests linking this TU alone still link). */
extern int xg_plat_disc_read_sectors(unsigned int lba, unsigned int count, void* out)
    __attribute__((weak));
#pragma weak xg_plat_mods_filter_asset
#pragma weak xg_plat_mods_assets_active
#pragma weak xg_plat_mods_lookup_asset

int ArchiveReadFile(u32 dbgEntryIndex, u8* pDestBuffer, s32 arg2, s32 flags);
extern int ArchiveDecodeSize(int entryIndex);
extern int ArchiveDecodeSector(int entryIndex);
extern int ArchiveDecodeAlignedSize(unsigned int entryIndex);
extern int ArchiveDataSync(void);
extern void* HeapAlloc(u_int size, u_int allocMode);
extern void ArchiveChangeStreamingFile(void* pStreamFile);
extern void ArchiveClearStreamFileSections(void);
extern void ArchiveCdDataSync(int mode);
extern u8 D_800B2394;
extern s16 D_8004FE10;
extern s16 D_8004FE24;

/* ArchiveCdDriveCommandHandler remains unresolved: PsyCross's
 * CdSyncCallback is unimplemented. The polling path below does not establish
 * equivalence for every retained caller of the retail command callback. */

typedef struct {
    void* streamFile;
    s32 nextSector;
    s32 sectorsLeft;
    s32 slot;
    s32 readPending;
    u16 producedId; /* DMA producer; D_8004FE24 remains the consumer cursor. */
    u8* replacement;  /* mod replacement served instead of the disc (xg_plat/mods.h) */
    s32 replacementSector;
} ArchivePortStreamReadState;

static ArchivePortStreamReadState s_ArchivePortStreamRead;

/* Streamed files: when mods replace assets, read the whole file from the
 * user's disc image up front, look it up by the SHA-256 of its original
 * bytes, and if a mod replaces it serve the stream's sectors from the
 * replacement (padded to whole sectors) instead of the CD.  Returns 1 when
 * a replacement is installed.  No-op without mods. */
static int ArchivePortStreamReplacement(s32 firstSector, s32 size, s32* pSectors)
{
    s32 nSectors = (size + (CD_SECTOR_SIZE - 1)) / CD_SECTOR_SIZE;
    u8* original;
    void* repl;
    unsigned long replSize = 0;

    if (!xg_plat_mods_assets_active || !xg_plat_mods_lookup_asset ||
        !xg_plat_disc_read_sectors || !xg_plat_mods_assets_active() || nSectors <= 0)
        return 0;
    original = (u8*)malloc((unsigned long)nSectors * CD_SECTOR_SIZE);
    if (original == NULL)
        return 0;
    if (xg_plat_disc_read_sectors((unsigned int)firstSector, (unsigned int)nSectors, original) != 0) {
        free(original);
        return 0;
    }
    repl = xg_plat_mods_lookup_asset(original, (unsigned long)size, (void*)&replSize);
    free(original);
    if (repl == NULL || replSize == 0) {
        free(repl);
        return 0;
    }
    nSectors = (s32)((replSize + (CD_SECTOR_SIZE - 1)) / CD_SECTOR_SIZE);
    s_ArchivePortStreamRead.replacement = (u8*)malloc((unsigned long)nSectors * CD_SECTOR_SIZE);
    if (s_ArchivePortStreamRead.replacement == NULL) {
        free(repl);
        return 0;
    }
    memset(s_ArchivePortStreamRead.replacement, 0, (unsigned long)nSectors * CD_SECTOR_SIZE);
    memcpy(s_ArchivePortStreamRead.replacement, repl, replSize);
    free(repl);
    s_ArchivePortStreamRead.replacementSector = 0;
    printf("[xg_plat] mods: streamed file at sector %d served from a replacement "
           "(%d sectors)\n", (int)firstSector, (int)nSectors);
    *pSectors = nSectors;
    g_ArchiveCurFileSize = (s32)replSize;
    return 1;
}

static u16 ArchivePsxQueueGetIndex(u8* pEntries, int index) {
    return *(u16*)(pEntries + index * 8);
}

static u32 ArchivePsxQueueGetData(u8* pEntries, int index) {
    return *(u32*)(pEntries + index * 8 + 4);
}

static void ArchivePsxQueueSetEntry(u8* pEntries, int index, u16 archiveIndex, u32 pData) {
    *(u16*)(pEntries + index * 8) = archiveIndex;
    *(u32*)(pEntries + index * 8 + 4) = pData;
}

static int ArchiveReadPsxStreamQueue(u8* pEntries, int arg1) {
    int count;
    int i;

    if (pEntries == NULL || ArchivePsxQueueGetIndex(pEntries, 0) == 0) {
        return -3;
    }

    for (count = 0; ArchivePsxQueueGetIndex(pEntries, count) != 0; count += 1) {
    }
    if (count == 0) {
        return -3;
    }

    for (i = 0; i < count - 1; i += 1) {
        int minIndex = i;
        int j;
        for (j = i + 1; j < count; j += 1) {
            if (ArchivePsxQueueGetIndex(pEntries, j) < ArchivePsxQueueGetIndex(pEntries, minIndex)) {
                minIndex = j;
            }
        }
        if (minIndex != i) {
            u16 tmpIndex = ArchivePsxQueueGetIndex(pEntries, i);
            u32 tmpData = ArchivePsxQueueGetData(pEntries, i);
            ArchivePsxQueueSetEntry(pEntries, i,
                                    ArchivePsxQueueGetIndex(pEntries, minIndex),
                                    ArchivePsxQueueGetData(pEntries, minIndex));
            ArchivePsxQueueSetEntry(pEntries, minIndex, tmpIndex, tmpData);
        }
    }

    ArchiveCdDataSync(0);
    D_8004FE18 = g_CurArchiveOffset;
    D_8004FDFC = 0;

    for (i = 0; i < count; i += 1) {
        u32 pData = ArchivePsxQueueGetData(pEntries, i);
        if (pData != 0) {
            void* host;
            /* Prefer emulated PSX virtual addresses; also accept truncated host
             * pointers that already land inside g_PsxRam (HeapAlloc). */
            if (pData >= 0x80000000u && pData < 0x80200000u)
                host = PSX_ADDR(pData);
            else
                host = (void*)(uintptr_t)pData;
            ArchiveReadFileToBuffer(ArchivePsxQueueGetIndex(pEntries, i),
                                    host,
                                    arg1,
                                    CdlModeSpeed);
            ArchiveCdDataSync(0);
        }
    }

    g_ArchiveCurFileSize = 0;
    D_8004FDFC = 0;
    g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_IDLE;
    return 0;
}

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

/* PsyCross executes Setloc/Pause synchronously and does not provide the PSX
 * command-completion interrupt used by ArchiveCdDriveCommandHandler.  Keep the
 * retail entry-index decoding and externally visible drive-state contract,
 * but close the SEEK -> SEEK_DONE -> IDLE sequence in this adapter once the
 * host command has actually completed. */
static void ArchivePortCdSeekOrPause(int entryIndex) {
    int ok;

    if (entryIndex > 0) {
        int sector = ArchiveDecodeSector(entryIndex);
        CdIntToPos(sector, &g_ArchiveCdCurLocation);
        g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_SEEK;
        ok = CdControlB(CdlSetloc, (u_char*)&g_ArchiveCdCurLocation, NULL);
        if (!ok) {
            g_ArchiveCdDriveError = ARCHIVE_CD_DRIVE_ERR_SEEK;
            g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_ERROR;
            return;
        }
    } else {
        g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_DONE;
        ok = CdControlB(CdlPause, NULL, NULL);
        if (!ok) {
            g_ArchiveCdDriveError = ARCHIVE_CD_DRIVE_ERR_GENERIC;
            g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_ERROR;
            return;
        }
    }

    g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_IDLE;
}

void ArchiveCdSeekOrPause(int entryIndex) {
    if (g_ArchiveDebugTable == 0 && ArchiveDataSync() == 0) {
        D_8004FE18 = g_CurArchiveOffset;
        ArchivePortCdSeekOrPause(entryIndex);
    }
}

void ArchiveCdSeekToFile(int entryIndex) {
    ArchivePortCdSeekOrPause(entryIndex);
}

extern void func_8002BF38(void);
extern s8* D_8004FE08;
extern s16 D_8004FE28;
extern s16 D_80059F24, D_80059F30;
extern u16 D_80059F28, D_80059F2C, D_80059F34, D_80059F38;
extern s32 D_80059F3C;
extern u16* D_80059F4C;
extern s32 D_80059F50;

/* Feed the streamed VRAM archive through the retail synchronous section
 * decoder func_8002BF38 (temp2.c) instead of the former heuristic parser.
 * The decoder consumes one 0x800 sector per call from the stream-file slot
 * ring (retail: CD callbacks fill free slots; here we do it synchronously in
 * order). Layout per retail asm 80029FD4-9FF4: slots at pStreamFile+4
 * (D_8004FE2C, set by our func_80029EB0), sector buffers at pStreamFile+0x24
 * (D_8004FE08). The header sector of each section stays claimed in its slot
 * until section end (the decoder reads the strip height table from it), so
 * sectors must be copied into ring slots rather than decoded in place. */
static void PcPortDrain0xBBToVram(u8* buf, s32 sizeBytes, void* pStreamFile) {
    u8* slots = (u8*)pStreamFile + 4;
    u8* buffers = (u8*)pStreamFile + 0x24;
    s32 slotCount = D_8004FE40;
    s32 off;
    u16 seqIn = 0;
    s32 fed = 0;

    /* Retail func_80029EB0 zeroes the decoder state and mode registers from
     * its (all-zero for the field stream) arguments: asm 8002A050-8002A094. */
    D_8004FE08 = (s8*)buffers;
    D_8004FE28 = 0;
    D_80059F24 = 0;
    D_80059F28 = 0;
    D_80059F2C = 0;
    D_80059F30 = 0;
    D_80059F34 = 0;
    D_80059F38 = 0;
    D_80059F3C = 0;
    D_80059F4C = NULL;
    D_80059F50 = 0;
    D_8004FDFC = 1;
    ArchiveClearStreamFileSections();

    for (off = 0; off + 0x800 <= sizeBytes && D_8004FDFC == 1; off += 0x800) {
        s32 j;
        for (j = 0; j < slotCount; j++) {
            if (*(u16*)(slots + j * 8) == 0) {
                break;
            }
        }
        if (j == slotCount) {
            printf("[field-0bb] STOP: slot ring stalled at sector %d\n", fed);
            return;
        }
        memcpy(buffers + j * 0x800, buf + off, 0x800);
        *(u16*)(slots + j * 8 + 0) = 1;
        *(u16*)(slots + j * 8 + 2) = seqIn;
        seqIn++;
        fed++;
        func_8002BF38();
    }
    printf("[field-0bb] retail decoder: %d sectors fed, sectionsLeft=%d stripsLeft=%d done=%d\n",
           fed, D_80059F3C, D_80059F50, D_8004FDFC == 0);
}

int func_80029EB0(s32 archiveIndex, void* pStreamFile, s32 arg2, s32 arg3, s32 arg4, s32 arg5, s32 arg6, s32 arg7, s32 arg8, s32 arg9) {
    int nStreamSectors;
    int result;
    u8* pReadBuffer;

    (void)arg3;
    (void)arg4;
    (void)arg5;
    (void)arg6;
    (void)arg7;
    (void)arg8;
    (void)arg9;

    if (pStreamFile == NULL) {
        return -4;
    }

    nStreamSectors = *(s32*)pStreamFile;
    if (nStreamSectors < 2) {
        return -4;
    }

    if (archiveIndex <= 0 || ArchiveDecodeSize(archiveIndex) <= 0) {
        return -3;
    }

    ArchiveCdDataSync(0);
    D_8004FE18 = g_CurArchiveOffset;
    ArchiveChangeStreamingFile(pStreamFile);

    g_ArchiveCurFileSector = ArchiveDecodeSector(archiveIndex);
    g_ArchiveCurFileSize = ArchiveDecodeAlignedSize(archiveIndex);
    D_8004FE2C = (ArchiveStreamFileSectionHeader*)((u8*)pStreamFile + 4);
    D_8004FE40 = nStreamSectors;
    D_8004FDFC = 1;
    D_8004FE38 = arg2 & 0xFFFF;
    D_8004FE34 = 0;
    ArchiveClearStreamFileSections();

    pReadBuffer = (u8*)malloc(g_ArchiveCurFileSize);
    if (pReadBuffer == NULL) {
        D_8004FDFC = 0;
        g_ArchiveCurFileSize = 0;
        g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_IDLE;
        return -1;
    }

    {
        /* ArchiveReadFile clears g_ArchiveCurFileSize on completion, so capture
         * the byte length now for the VRAM drain. */
        s32 vramUploadSize = g_ArchiveCurFileSize;

        result = ArchiveReadFile(archiveIndex, pReadBuffer, arg2, 0);

        /* Retail decodes every streamed sector through func_8002BF38 as CD
         * callbacks land (asm 8002A050-8002A094 arms the decoder state inside
         * this function); the port reads the file synchronously, so drain the
         * whole buffer through the same decoder here. This is what uploads the
         * per-map field textures ((mapNum<<1)+0xB9 archive) to VRAM. */
        if (result >= 0 && vramUploadSize > 0) {
            printf("[field-0bb] archive 0x%x: %d bytes read, draining to VRAM\n", archiveIndex, vramUploadSize);
            PcPortDrain0xBBToVram(pReadBuffer, vramUploadSize, pStreamFile);
        }
    }
    free(pReadBuffer);
    return result;
}

int func_80029AFC(StreamDataQueueEntry* pEntries, int arg1, int arg2) {
    int count;
    int i;
    uintptr_t entries_u;

    (void)arg2;

    if ((void*)pEntries == (void*)&D_800B2394) {
        return ArchiveReadPsxStreamQueue((u8*)pEntries, arg1);
    }

    /* World-map mode init (0x80071CDC) builds a retail 8-byte-stride queue in
     * emulated PSX RAM at 0x8009D3F8. Host sizeof(StreamDataQueueEntry) is not
     * 8, so walk that buffer with the PSX layout helper. */
    entries_u = (uintptr_t)pEntries;
    if (entries_u >= (uintptr_t)g_PsxRam &&
        entries_u < (uintptr_t)g_PsxRam + (uintptr_t)PSX_RAM_SIZE) {
        return ArchiveReadPsxStreamQueue((u8*)pEntries, arg1);
    }

    if (pEntries == NULL || pEntries[0].archiveIndex == 0) {
        return -3;
    }

    for (count = 0; pEntries[count].archiveIndex != 0; count += 1) {
    }
    if (count == 0) {
        return -3;
    }

    for (i = 0; i < count - 1; i += 1) {
        int minIndex = i;
        int j;
        for (j = i + 1; j < count; j += 1) {
            if ((u16)pEntries[j].archiveIndex < (u16)pEntries[minIndex].archiveIndex) {
                minIndex = j;
            }
        }
        if (minIndex != i) {
            StreamDataQueueEntry tmp = pEntries[i];
            pEntries[i] = pEntries[minIndex];
            pEntries[minIndex] = tmp;
        }
    }

    ArchiveCdDataSync(0);
    D_8004FE18 = g_CurArchiveOffset;
    D_8004FDFC = 0;

    for (i = 0; i < count; i += 1) {
        if (pEntries[i].pData != NULL) {
            ArchiveReadFileToBuffer((u16)pEntries[i].archiveIndex,
                                    pEntries[i].pData,
                                    arg1,
                                    CdlModeSpeed);
            ArchiveCdDataSync(0);
        }
    }

    g_ArchiveCurFileSize = 0;
    D_8004FDFC = 0;
    g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_IDLE;
    return 0;
}

/* The retail CD producer selects the next empty slot and lets the CD DMA
 * callback mark it ready without advancing the consumer cursor. The
 * PsyCross libcd queue has the same sector payload but exposes completion via
 * CdReadSync(1), so retain the retail two-phase boundary: one call queues a
 * sector and a later call consumes exactly that one sector.  This is important
 * for WDS banks and streamed field data; reading the complete file here would
 * change when the caller's callback observes each chunk and can overrun the
 * retail eight-slot window.
 */
void PcPort_ArchivePollTransport(void) {
    u8* streamFile = (u8*)g_ArchiveCurStreamFile;
    u8* table;
    u8* buffers;
    s32 slotCount;
    s32 i;

    if (streamFile == NULL || s_ArchivePortStreamRead.streamFile != streamFile ||
        s_ArchivePortStreamRead.sectorsLeft <= 0) {
        return;
    }

    table = streamFile + 4;
    buffers = streamFile + 0x24 + (*(s32*)streamFile * sizeof(ArchiveStreamFileSectionHeader));
    slotCount = *(s32*)streamFile;
    if (slotCount <= 0 || slotCount > ARCHIVE_MAX_SECTIONS) {
        return;
    }

    if (s_ArchivePortStreamRead.readPending) {
        int readStatus = CdReadSync(1, NULL);
        if (readStatus < 0) {
            g_ArchiveCdDriveError = ARCHIVE_CD_DRIVE_ERR_READ;
            g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_ERROR;
            s_ArchivePortStreamRead.readPending = 0;
            return;
        }

        if (readStatus > 0) return;

        /* CdReadSync(1) has consumed exactly one MODE2 user-data sector.  The
         * archive callback's externally visible state is state=3 and the
         * monotonically increasing sector id. */
        *(u16*)(table + s_ArchivePortStreamRead.slot * 8 + 0) = 3;
        *(u16*)(table + s_ArchivePortStreamRead.slot * 8 + 2) = s_ArchivePortStreamRead.producedId++;
        s_ArchivePortStreamRead.readPending = 0;
        s_ArchivePortStreamRead.nextSector += 1;
        s_ArchivePortStreamRead.sectorsLeft -= 1;
        if (s_ArchivePortStreamRead.sectorsLeft == 0) {
            g_ArchiveCurFileSize = 0;
            D_8004FDFC = 0;
            g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_IDLE;
        }
        return;
    }

    /* Match the retail circular scan beginning at D_8004FE28.  A full ring
     * means the consumer has not released a prior slot yet; report no data
     * and leave the request outstanding so the caller can poll again. */
    for (i = 0; i < slotCount; i += 1) {
        s32 slot = (D_8004FE28 + i) % slotCount;
        if (*(u16*)(table + slot * 8 + 0) == ARCHIVE_STREAM_FILE_NOT_LOADED) {
            s_ArchivePortStreamRead.slot = slot;
            D_8004FE28 = (s16)((slot + 1) % slotCount);
            break;
        }
    }
    if (i == slotCount) {
        return;
    }

    if (s_ArchivePortStreamRead.replacement != NULL) {
        /* Mod replacement: deliver the next sector at once, with the same
         * slot bookkeeping as a completed CD read. */
        memcpy(buffers + s_ArchivePortStreamRead.slot * CD_SECTOR_SIZE,
               s_ArchivePortStreamRead.replacement +
                   (unsigned long)s_ArchivePortStreamRead.replacementSector * CD_SECTOR_SIZE,
               CD_SECTOR_SIZE);
        s_ArchivePortStreamRead.replacementSector += 1;
        *(u16*)(table + s_ArchivePortStreamRead.slot * 8 + 0) = 3;
        *(u16*)(table + s_ArchivePortStreamRead.slot * 8 + 2) = s_ArchivePortStreamRead.producedId++;
        s_ArchivePortStreamRead.sectorsLeft -= 1;
        if (s_ArchivePortStreamRead.sectorsLeft == 0) {
            free(s_ArchivePortStreamRead.replacement);
            s_ArchivePortStreamRead.replacement = NULL;
            g_ArchiveCurFileSize = 0;
            D_8004FDFC = 0;
            g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_IDLE;
        }
        return;
    }

    /* The read starts at the archive sector captured by libarchive.c.  Each
     * subsequent one-sector queue advances PsyCross's image cursor naturally. */
    if (!s_ArchivePortStreamRead.readPending) {
        if (!CdRead(1,
                    (u_long*)(buffers + s_ArchivePortStreamRead.slot * CD_SECTOR_SIZE),
                    CdlModeSpeed)) {
            g_ArchiveCdDriveError = ARCHIVE_CD_DRIVE_ERR_READ;
            g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_ERROR;
            return;
        }
        s_ArchivePortStreamRead.readPending = 1;
    }
    return;
}

/* Polling the CD transport does not consume a ready sector. This matches the
 * retail CD interrupt / func_80028B14 split and lets ArchiveCdDataSync complete
 * a transfer while a menu has temporarily stopped the field consumer. */
s32 func_80028B14(void) {
    u8* stream = (u8*)g_ArchiveCurStreamFile;
    s32 count, i;
    if (!stream || s_ArchivePortStreamRead.streamFile != stream) return 0;
    if (s_ArchivePortStreamRead.sectorsLeft <= 0 &&
        s_ArchivePortStreamRead.producedId == (u16)D_8004FE24) return 0;
    count = *(s32*)stream;
    if (count <= 0 || count > ARCHIVE_MAX_SECTIONS) return 0;
    PcPort_ArchivePollTransport();
    for (i = 0; i < count; ++i) {
        u16* entry = (u16*)(stream + 4 + i * 8);
        if (entry[0] == 3 && entry[1] == (u16)D_8004FE24) {
            D_8004FE24 = (s16)((u16)D_8004FE24 + 1u);
            return (s32)(uintptr_t)(stream + 0x24 + count * 8 + i * CD_SECTOR_SIZE);
        }
    }
    return 0;
}

int ArchiveReadFile(u32 dbgEntryIndex, u8* pDestBuffer, s32 arg2, s32 flags) {
    int nSectors;
    CdlLOC loc;

    (void)dbgEntryIndex;
    (void)arg2;

    /* The normal stream mode is the 0x800-byte field/WDS path implemented by
     * func_80028B14 below.  0x200 selects the retail 2340-byte ADPCM sector
     * path; PsyCross's CdReadSync data primitive currently exposes only the
     * 2048-byte MODE2 payload, so accepting it here would silently truncate
     * the sector.  Keep that distinct mode fail-closed until its XA/DMA
     * payload boundary is implemented. */
    if (flags & 0x200) {
        return -4;
    }

    if (flags & CdlModeStream) {
        u8* streamFile = (u8*)pDestBuffer;
        s32 slotCount;

        if (streamFile == NULL || streamFile != (u8*)g_ArchiveCurStreamFile) {
            return -3;
        }
        slotCount = *(s32*)streamFile;
        if (slotCount <= 0 || slotCount > ARCHIVE_MAX_SECTIONS) {
            return -3;
        }

        ArchiveChangeStreamingFile(streamFile);
        D_8004FE08 = (s8*)(streamFile + 0x24 +
                            slotCount * sizeof(ArchiveStreamFileSectionHeader));
        D_8004FE2C = (ArchiveStreamFileSectionHeader*)(streamFile + 4);
        D_8004FE40 = slotCount;
        D_8004FE10 = 0;
        D_8004FE24 = 0;
        D_8004FE28 = 0;
        D_8004FE34 = 0;
        D_8004FDFC = 1;
        g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_READ_SECTOR;
        ArchiveClearStreamFileSections();

        s_ArchivePortStreamRead.streamFile = streamFile;
        s_ArchivePortStreamRead.nextSector = g_ArchiveCurFileSector;
        s_ArchivePortStreamRead.sectorsLeft =
            (g_ArchiveCurFileSize + (CD_SECTOR_SIZE - 1)) / CD_SECTOR_SIZE;
        s_ArchivePortStreamRead.producedId = 0;
        s_ArchivePortStreamRead.slot = 0;
        s_ArchivePortStreamRead.readPending = 0;
        free(s_ArchivePortStreamRead.replacement);
        s_ArchivePortStreamRead.replacement = NULL;
        if (ArchivePortStreamReplacement(g_ArchiveCurFileSector, g_ArchiveCurFileSize,
                                         &s_ArchivePortStreamRead.sectorsLeft))
            return 0; /* served from memory: no CD seek */

        if (s_ArchivePortStreamRead.sectorsLeft <= 0) {
            g_ArchiveCurFileSize = 0;
            D_8004FDFC = 0;
            g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_IDLE;
            return -4;
        }

        /* Seek is synchronous in PsyCross, but the data transfer remains
         * sector-granular and is completed only by func_80028B14 polling. */
        CdIntToPos(g_ArchiveCurFileSector, &loc);
        if (!CdControlB(CdlSetloc, (u_char*)&loc, NULL)) {
            s_ArchivePortStreamRead.sectorsLeft = 0;
            D_8004FDFC = 0;
            g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_ERROR;
            return -1;
        }
        return 0;
    }

    if (pDestBuffer == NULL || g_ArchiveCurFileSize <= 0) {
        g_ArchiveCurFileSize = 0;
        D_8004FDFC = 0;
        g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_IDLE;
        return 0;
    }

    /* Round the byte length up to whole 2048-byte data sectors. */
    nSectors = (g_ArchiveCurFileSize + (CD_SECTOR_SIZE - 1)) / CD_SECTOR_SIZE;

    /* Same sector addressing the hardware path uses (CdIntToPos on the raw
     * sector). CdControlB(CdlSetloc) seeks the image and selects RM_DATA so
     * CdReadSync extracts the 2048-byte data payload from each MODE2/2352 sector. */
    CdIntToPos(g_ArchiveCurFileSector, &loc);
    CdControlB(CdlSetloc, (u_char*)&loc, NULL);

    if (nSectors * CD_SECTOR_SIZE == (int)g_ArchiveCurFileSize) {
        /* File is a whole number of sectors: read straight into the buffer. */
        CdRead(nSectors, (u_long*)pDestBuffer, 0);
        CdReadSync(0, NULL);
    } else {
        /* Partial last sector. The game sizes buffers to ArchiveDecodeAlignedSize
         * (4-byte aligned, NOT sector-rounded), so pDestBuffer holds exactly
         * g_ArchiveCurFileSize bytes. CdRead always delivers whole sectors, so
         * reading directly would scribble up to 2047 bytes past the buffer into
         * the next heap block (on PSX the async sector-copy callbacks only write
         * the file's bytes). Bounce through a sector-rounded scratch buffer and
         * copy back just the file's bytes. */
        int nAlignedBytes = nSectors * CD_SECTOR_SIZE;
        u8* pBounce = (u8*)malloc(nAlignedBytes);
        if (pBounce == NULL) {
            return -1;
        }
        CdRead(nSectors, (u_long*)pBounce, 0);
        CdReadSync(0, NULL);
        memcpy(pDestBuffer, pBounce, g_ArchiveCurFileSize);
        free(pBounce);
    }

    /* Mods: hash-keyed asset replacement of the file just read
     * (xg_plat/mods.h).  Weak so tests that link this TU alone need no mods
     * layer; with no mods installed it only emits the asset_loaded event. */
    if (xg_plat_mods_filter_asset)
        (void)xg_plat_mods_filter_asset(pDestBuffer, (size_t)g_ArchiveCurFileSize,
                                        (size_t)g_ArchiveCurFileSize);

    /* Mark the transfer complete so ArchiveDataSync()/ArchiveCdDataSync() return
     * immediately (g_ArchiveCdDriveState IDLE, no in-flight file). */
    g_ArchiveCurFileSize = 0;
    D_8004FDFC = 0;
    g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_IDLE;
    return 0;
}

void ArchiveCdSetMode(u_char mode) {
    int i;

    g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_RESET_MODE;
    for (i = 0; i < 4; i += 1) {
        D_80059F18[i] = 0;
    }

    D_80059F18[0] = mode;
    CdControlF(CdlSetmode, D_80059F18);
    CdControlF(CdlPause, NULL);
    g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_IDLE;
}

/* Retail func_8002A498: stop the current archive stream on a channel.  The
 * debug-table close/retry path is retained verbatim; the normal CD path only
 * updates the stream state consumed by the movie/archive callers. */
void func_8002A498(int channel) {
    int i;

    D_8004FE34 = 1;
    D_8004FE38 = channel;
    if (g_ArchiveDebugTable) {
        g_ArchiveCurFileSize = 0;
        D_8004FDFC = 0;

        /* Close (debug) streaming file handle. */
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
