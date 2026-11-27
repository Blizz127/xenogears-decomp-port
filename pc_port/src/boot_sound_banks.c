/*
 * Retail boot sound-bank closure from func_80019578
 * (SLUS_006.64 0x80019670-0x80019774 and 0x80019840-0x80019868).
 *
 * The native entry point replaces that function, so these four disc-backed
 * WDS loads must be replayed explicitly. They supply the title/menu sample
 * banks; omitting them leaves otherwise valid voices keyed at SPU address 0.
 */

#include "boot_sound_banks.h"

#include <limits.h>

#include "common.h"
#include "system/archive.h"
#include "system/sound.h"

extern s32 ArchiveReadFileToBuffer(s32 index, void *buffer, u32 arg2, u32 flags);
extern int PcPort_ArchiveCdDataSyncStatus(int mode);
extern void *HeapAlloc(u_int size, u_int flags);
extern u_int HeapFree(void *memory);
extern SoundWDSEntry *SoundLoadWdsFile(SoundWDSEntry *source, s32 mode);
extern void SoundFreeWdsEntry(SoundWDSEntry *entry);
extern int func_8003BDFC(s32 flags);

/* Host-width counterparts of the four retail pointer globals. */
void *D_80059560;
void *D_800595AC;
void *D_8006251C;
void *D_80062524;

int PcPort_LoadRetailBootSoundBanks(void)
{
    void *buffers[4];
    SoundWDSEntry *entries[4];
    int allocated = 0;
    int capacity = 0;
    int loaded = 0;

    if (!g_ArchiveHeader || !g_ArchiveTable)
        return 0;
    int directory = ArchiveSetIndex(0, 1);
    if (directory < 0 || directory > 16 * CD_SECTOR_SIZE / ARCHIVE_HEADER_ENTRY_SIZE - 5)
        return 0;

    /* Retail allocates all four source buffers before submitting file 2. */
    for (int file = 2; file <= 5; ++file) {
        int bytes = ArchiveDecodeSize(file);
        if (bytes <= 0 || bytes > INT_MAX - 3)
            goto free_buffers;
        capacity = (bytes + 3) & ~3;
        buffers[allocated] = HeapAlloc(capacity, 0);
        if (!buffers[allocated])
            goto free_buffers;
        ++allocated;
    }
    for (int file = 2; file < 5; ++file) {
        if (ArchiveReadFileToBuffer(file, buffers[file - 2], 0, 0) != 0)
            goto free_buffers;
    }
    if (PcPort_ArchiveSubmitBootWds5(buffers[3], capacity) != 0)
        goto free_buffers;
    if (PcPort_ArchiveCdDataSyncStatus(0) != 0)
        goto free_buffers;

    for (int i = 0; i < 4; ++i) {
        entries[i] = SoundLoadWdsFile(buffers[i], 0);
        if (!entries[i]) {
            /* The failed load can already have queued an SPU transfer. */
            func_8003BDFC(0x10);
            for (int j = 0; j < i; ++j)
                SoundFreeWdsEntry(entries[j]);
            goto free_buffers;
        }
    }
    D_80059560 = entries[1];
    D_800595AC = entries[3];
    func_8003BDFC(0x10);
    loaded = 1;

free_buffers:
    for (int i = 0; i < allocated; ++i)
        HeapFree(buffers[i]);
    return loaded;
}
