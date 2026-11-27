/* Guest archive lengths are word-aligned. The host CD backend writes whole
 * sectors, so it only receives a private sector-rounded destination. */
typedef struct {
    u8 *staging;
    u8 *destination;
    int bytes;
} ArchivePortStagedFile;

static ArchivePortStagedFile s_bootWdsFile;
static int s_bootWdsOwned;
static int s_bootWdsPublishing;
static int s_bootWdsResult;

static int archiveStartStagedFile(ArchivePortStagedFile *file, int sector,
                                  u8 *destination, int bytes)
{
    CdlLOC position;
    if (!destination || bytes <= 0 || sector < 0)
        return ARCHIVE_CD_DRIVE_ERR_READ;
    int sectors = 1 + (bytes - 1) / CD_SECTOR_SIZE;
    /* CdIntToPos represents LBA + 150 as a two-decimal-digit MSF. */
    if (sector > 100 * 60 * 75 - 150 - sectors)
        return ARCHIVE_CD_DRIVE_ERR_SEEK;
    u8 *staging = malloc((size_t)sectors * CD_SECTOR_SIZE);
    if (!staging)
        return ARCHIVE_CD_DRIVE_ERR_READ;
    CdIntToPos(sector, &position);
    if (!CdControlB(CdlSetloc, (u_char *)&position, NULL)) {
        free(staging);
        return ARCHIVE_CD_DRIVE_ERR_SEEK;
    }
    if (!CdRead(sectors, (u_long *)staging, 0)) {
        free(staging);
        return ARCHIVE_CD_DRIVE_ERR_READ;
    }
    file->staging = staging;
    file->destination = destination;
    file->bytes = bytes;
    return 0;
}

static int archiveFinishStagedFile(ArchivePortStagedFile *file, int mode)
{
    int status;
    do {
        status = CdReadSync(mode, NULL);
    } while (!mode && status > 0);
    if (status > 0)
        return 1;
    if (!status)
        memcpy(file->destination, file->staging, (size_t)file->bytes);
    free(file->staging);
    file->staging = NULL;
    return status < 0 ? -1 : 0;
}

static void archiveCompleteStagedFile(int error, u8 *destination, int bytes)
{
    g_ArchiveCdDriveError = error;
    g_ArchiveCurFileSize = 0;
    D_8004FDFC = 0;
    g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_IDLE;
    if (!error && xg_plat_mods_filter_asset)
        (void)xg_plat_mods_filter_asset(destination, (size_t)bytes, (size_t)bytes);
}

/* Called on the game thread. A nonblocking poll can finish the read; keep
 * that result for the boot caller's subsequent blocking join. */
int PcPort_ArchiveBootWdsFence(int mode, int *result)
{
    if (!s_bootWdsOwned || s_bootWdsPublishing)
        return 0;
    if (s_bootWdsFile.staging) {
        int status = archiveFinishStagedFile(&s_bootWdsFile, mode);
        if (status > 0) {
            *result = 1;
            return 1;
        }
        s_bootWdsResult = status;
        s_bootWdsPublishing = 1;
        archiveCompleteStagedFile(status ? ARCHIVE_CD_DRIVE_ERR_READ : 0,
                                  s_bootWdsFile.destination, s_bootWdsFile.bytes);
        s_bootWdsFile = (ArchivePortStagedFile){0};
        s_bootWdsPublishing = 0;
    }
    *result = s_bootWdsResult;
    if (!mode)
        s_bootWdsOwned = 0;
    return 1;
}

int PcPort_ArchiveBootWdsOwned(void)
{
    return s_bootWdsOwned;
}

int PcPort_ArchiveSubmitBootWds5(void *destination, int capacity)
{
    if (s_bootWdsOwned || D_8004FDFC || g_ArchiveCdDriveState != ARCHIVE_CD_DRIVE_IDLE ||
        ArchiveDataSync() != 0)
        return ARCHIVE_READ_RETRY;
    if (!destination || capacity <= 0 || !g_ArchiveHeader || !g_ArchiveTable ||
        g_ArchiveDebugTable || g_CurArchiveOffset > 16u * CD_SECTOR_SIZE / ARCHIVE_HEADER_ENTRY_SIZE - 5 ||
        ((const u16 *)(uintptr_t)g_ArchiveHeader)[1] != g_CurArchiveOffset + 1u)
        return -1;
    int bytes = ArchiveDecodeSize(5);
    if (bytes <= 0 || bytes > INT32_MAX - 3 || ((bytes + 3) & ~3) > capacity)
        return -1;
    bytes = (bytes + 3) & ~3;
    int sector = ArchiveDecodeSector(5);
    int error = archiveStartStagedFile(&s_bootWdsFile, sector, destination, bytes);
    if (error) {
        g_ArchiveCdDriveError = error;
        return -1;
    }
    s_bootWdsOwned = 1;
    s_bootWdsResult = 0;
    D_8004FE18 = g_CurArchiveOffset;
    D_80059F0C = 5;
    g_ArchiveCurFileSector = sector;
    g_ArchiveCurFileSize = bytes;
    D_8004FDFC = 1;
    return 0;
}
