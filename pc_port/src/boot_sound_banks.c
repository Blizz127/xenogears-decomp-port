/*
 * Retail boot sound-bank closure from func_80019578
 * (SLUS_006.64 0x80019670-0x80019774 and 0x80019840-0x80019868).
 *
 * The native entry point replaces that function, so these four disc-backed
 * WDS loads must be replayed explicitly. They supply the title/menu sample
 * banks; omitting them leaves otherwise valid voices keyed at SPU address 0.
 */

#include "boot_sound_banks.h"

#include "common.h"
#include "system/sound.h"

extern int ArchiveSetIndex(int directoryIndex, int entryIndex);
extern int ArchiveDecodeSize(int entryIndex);
extern void ArchiveReadFileToBuffer(int fileIndex, void *buffer,
                                    int arg2, int arg3);
extern int ArchiveCdDataSync(int mode);
extern void *HeapAlloc(int size, int flags);
extern void HeapFree(void *memory);
extern SoundWDSEntry *SoundLoadWdsFile(SoundWDSEntry *source, s32 mode);
extern s16 func_8003BDFC(s32 flags);

/* Host-width counterparts of the four retail pointer globals. */
void *D_80059560;
void *D_800595AC;
void *D_8006251C;
void *D_80062524;

void PcPort_LoadRetailBootSoundBanks(void)
{
    void *buffers[4];
    SoundWDSEntry *entries[4];
    int file;

    ArchiveSetIndex(0, 1);
    for (file = 2; file <= 5; file++) {
        buffers[file - 2] = HeapAlloc(ArchiveDecodeSize(file), 0);
        ArchiveReadFileToBuffer(file, buffers[file - 2], 0, 0);
    }
    ArchiveCdDataSync(0);

    for (file = 0; file < 4; file++) {
        entries[file] = SoundLoadWdsFile((SoundWDSEntry *)buffers[file], 0);
    }
    D_80059560 = entries[1];
    D_800595AC = entries[3];

    func_8003BDFC(0x10);
    for (file = 0; file < 4; file++) {
        HeapFree(buffers[file]);
    }
}
