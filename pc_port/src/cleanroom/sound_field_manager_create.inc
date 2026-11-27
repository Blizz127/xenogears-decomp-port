/* SPDX-License-Identifier: MIT */
#include <stdint.h>
#include <string.h>

AudioManager *func_80039850(SoundFile *file)
{
    const s16 error = (s16)SoundUnkDebug0(file);
    if (error != 0) {
        SoundHandleError(error);
        return NULL;
    }

    const u8 *layout = (const u8 *)&file->sedId;
    const u32 base_size = (u32)SoundCalculateAudioManagerSize(layout[0]);
    const u32 extra_size = layout[1] ? 0x180u : 0u;
    AudioManager *manager = SoundHeapAllocate(base_size + extra_size);
    if (manager == NULL) {
        SoundHandleError(30);
        return NULL;
    }

    manager->unk_0x8 = (s32)SOUND_PTR_TO_PSX(file);
    if (layout[1])
        func_8003B0AC(manager, file);
    func_8003B22C(manager);
    func_8003B424(manager);
    memset(manager->unk_0x4c, 0, sizeof manager->unk_0x4c);
    SoundAddAudioManagerToList(manager);
    return manager;
}

void func_800399D4(AudioManager *manager)
{
    if (manager == NULL)
        return;

    if ((u16)manager->unk_Flags & 0x8000u)
        func_80039C4C(manager);
    SoundFile *file = (SoundFile *)(intptr_t)manager->unk_0x8;
    if ((u16)SoundUnkDebug0(file) != 0) {
        SoundHandleError(10);
        return;
    }
    if (SoundRemoveAudioManagerFromList(manager) != 0) {
        SoundHandleError(5);
        return;
    }
    func_8003B930(manager);
    if (((u16)manager->unk_Flags & 0x4000u) == 0)
        SoundHeapFree(manager);
}
