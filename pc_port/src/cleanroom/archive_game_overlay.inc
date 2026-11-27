/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Blizz127 and contributors
 *
 * Complete an overlay read before publishing its bytes to the game heap.
 * The last LZSS group can extend past the archive member into sector padding.
 */
#include "game_state_overlay.h"
#include "lzss_bounds.h"
#include "archive_ready.h"

extern int memcmp(const void *, const void *, size_t);

typedef struct {
    int sector;
    u8 *payload;
    int delivered;
    int command;
} ArchiveOverlayTransfer;

/* File callbacks are dispatched by the game thread from the two SDK poll
 * calls below. The pointer also guards nested overlay loads from asset hooks. */
static ArchiveOverlayTransfer *archiveOverlayActive;

static void archiveOverlayReady(u_char interrupt, u_char *response)
{
    (void)response;
    ArchiveOverlayTransfer *transfer = archiveOverlayActive;
    if (transfer && interrupt == CdlDataReady)
        transfer->delivered = PcPort_ArchiveGetWdsSector(transfer->sector, transfer->payload);
}

static void archiveOverlayCommand(u_char interrupt, u_char *response)
{
    (void)response;
    ArchiveOverlayTransfer *transfer = archiveOverlayActive;
    if (!transfer)
        return;
    if (interrupt == CdlComplete) {
        if (transfer->command == CdlSetloc &&
            PsyX_CdStartFileReadN(archiveOverlayReady, g_PcPortArchiveFileRegistration, archiveOverlayCommand)) {
            transfer->command = CdlReadN;
            return;
        }
        if (transfer->command == CdlReadN) {
            transfer->command = 0;
            return;
        }
    }
    transfer->command = -1;
}

static int archiveOverlayReadSectors(ArchiveOverlayTransfer *transfer, int sectors, u8 *staging)
{
    CdlLOC position;
    if (!CdIntToPos(transfer->sector, &position))
        return -1;
    transfer->command = CdlSetloc;
    int accepted = PsyX_CdStartTimedFileSetloc(&position, archiveOverlayReady, archiveOverlayCommand,
                                               &g_PcPortArchiveFileRegistration, PcPort_ArchiveClockUsec);
    if (accepted == -1)
        return PsyX_CdReadFileLogical(&position, sectors, staging) == 1 ? 0 : -1;
    if (accepted != 1)
        return ARCHIVE_READ_RETRY;

    uint64_t ticket = g_PcPortArchiveFileRegistration;
    while (transfer->command > 0) {
        int result = PsyX_CdPollSyncCallback(archiveOverlayCommand);
        if (result < 0) {
            transfer->command = -1;
            break;
        }
        if (!result)
            PcPort_ArchiveWaitForQueueTick();
    }

    int copied = 0;
    if (transfer->command == 0) {
        uint64_t start = PcPort_ArchiveClockUsec();
        int rate = PsyX_CdDataSectorRate();
        while (rate > 0 && copied < sectors && ticket == g_PcPortArchiveFileRegistration) {
            uint64_t due = (uint64_t)(copied + 1) * 1000000u / (unsigned int)rate;
            if (PcPort_ArchiveClockUsec() - start < due) {
                PcPort_ArchiveWaitForQueueTick();
                continue;
            }
            transfer->payload = staging + (size_t)copied * CD_SECTOR_SIZE;
            transfer->delivered = 0;
            if (PsyX_CdPollFileReadN(transfer->sector, archiveOverlayReady, ticket) != 1 ||
                !transfer->delivered)
                break;
            ++transfer->sector;
            ++copied;
        }
    }
    int released = PsyX_CdEndFileReadN(archiveOverlayReady, ticket, copied == sectors);
    if (g_PcPortArchiveFileRegistration == ticket)
        g_PcPortArchiveFileRegistration = 0;
    return released && copied == sectors ? 0 : -1;
}

static int archiveOverlayPublish(const PcPortOverlayLayout *layout, const u8 *original, u8 *destination,
                                 unsigned int capacity, unsigned int *sourceSize)
{
    size_t logical = (size_t)layout->logicalSize;
    size_t source = (size_t)layout->sourceSize;
    /* Usually the heap can hold both lengths. A mounted disc may instead
     * contain a short packed stream inside a larger, padded archive member.
     * Hash that entire member even when the caller reserved only its prefix. */
    u8 *member = capacity >= logical ? destination : malloc(logical);
    if (!member)
        return -1;
    memcpy(member, original, logical);
    size_t filtered = xg_plat_mods_filter_asset ? xg_plat_mods_filter_asset(member, logical, logical) : logical;
    size_t retained = filtered;
    if (filtered == logical && memcmp(member, original, logical) == 0) {
        /* A mounted or modified file can supply its own complete stream.
         * Append the physical tail only when it completes the expected image. */
        size_t extent;
        uint32_t declared = (uint32_t)original[0] | (uint32_t)original[1] << 8 |
                            (uint32_t)original[2] << 16 | (uint32_t)original[3] << 24;
        if (declared == (unsigned int)layout->imageSize &&
            PcPort_LzssStreamExtent(original, source, declared, &extent) && extent == source)
            retained = source;
    }
    int ok = filtered > 0 && filtered <= logical && retained <= capacity;
    if (ok) {
        if (member != destination)
            memcpy(destination, member, retained < logical ? retained : logical);
        if (retained > logical)
            memcpy(destination + logical, original + logical, retained - logical);
        *sourceSize = (unsigned int)retained;
    }
    if (member != destination)
        free(member);
    return ok ? 0 : -1;
}

int PcPort_ArchiveReadGameOverlay(unsigned int state, void *destination, unsigned int capacity,
                                  unsigned int *sourceSize)
{
    if (archiveOverlayActive || g_PcPortArchiveFileRegistration || g_PcPortArchiveCancelFilteredQueue ||
        g_PcPortArchiveQueueService || g_PcPortArchiveModeService || PcPort_ArchiveBootWdsOwned() ||
        D_8004FDFC || g_ArchiveCdDriveState != ARCHIVE_CD_DRIVE_IDLE)
        return ARCHIVE_READ_RETRY;
    if (!destination || !sourceSize || !g_ArchiveHeader || !g_ArchiveTable || g_ArchiveDebugTable)
        return -1;
    const PcPortOverlayLayout *layout = PcPort_GetOverlayLayout(state);
    if (!layout || layout->file <= 0 || layout->logicalSize < 4 || layout->sourceSize < 4 ||
        layout->imageSize <= 0 || (unsigned int)layout->sourceSize > capacity)
        return -1;

    /* ArchiveSetIndex(0,1) selects a one-based directory entry. Reject a
     * stale archive selection before indexing into the 16-sector FAT. */
    const u8 *directory = (const u8 *)(uintptr_t)g_ArchiveHeader;
    unsigned int first = (unsigned int)directory[2] | (unsigned int)directory[3] << 8;
    unsigned int rows = 16u * CD_SECTOR_SIZE / ARCHIVE_HEADER_ENTRY_SIZE;
    if (!first || first - 1u != g_CurArchiveOffset || g_CurArchiveOffset >= rows ||
        (unsigned int)layout->file > rows - g_CurArchiveOffset ||
        ArchiveDecodeAlignedSize((unsigned int)layout->file) != layout->logicalSize)
        return -1;

    int sectors = 1 + (layout->logicalSize - 1) / CD_SECTOR_SIZE;
    if ((size_t)layout->sourceSize > (size_t)sectors * CD_SECTOR_SIZE)
        return -1;
    int sector = ArchiveDecodeSector(layout->file);
    if (sector < 0 || sector > 100 * 60 * 75 - 150 - sectors)
        return -1;
    if (ArchiveDataSync() != 0)
        return ARCHIVE_READ_RETRY;

    u8 *staging = malloc((size_t)sectors * CD_SECTOR_SIZE);
    if (!staging)
        return -1;
    ArchiveOverlayTransfer transfer = { .sector = sector };
    archiveOverlayActive = &transfer;
    int result = archiveOverlayReadSectors(&transfer, sectors, staging);
    if (result == 0) {
        D_8004FE18 = g_CurArchiveOffset;
        D_80059F0C = (unsigned int)layout->file;
        g_ArchiveCurFileSector = sector + sectors;
        g_ArchiveCdDriveError = 0;
        result = archiveOverlayPublish(layout, staging, destination, capacity, sourceSize);
    } else if (result != ARCHIVE_READ_RETRY) {
        g_ArchiveCdDriveError = ARCHIVE_CD_DRIVE_ERR_READ;
    }
    archiveOverlayActive = NULL;
    free(staging);
    return result;
}
