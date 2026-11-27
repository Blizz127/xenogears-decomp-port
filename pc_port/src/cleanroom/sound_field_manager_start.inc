#include <string.h>

void func_80039A80(AudioManager *manager, s32 level, s32 steps)
{
    if (!PcPort_AdmitSoundSequence())
        return;
    if (!manager) {
        SoundHandleError(5);
        return;
    }

    manager->unk_Flags = (s16)((u16)manager->unk_Flags & 0x7fffu);
    if ((s16)SoundUnkDebug0((void *)(intptr_t)manager->unk_0x8)) {
        SoundHandleError(10);
        return;
    }
    if ((u16)manager->unk_Flags & 0x8000u)
        func_80039C4C(manager);

    DisableEvent(g_unk_SoundEvent);
    func_8003B22C(manager);
    func_8003B424(manager);
    manager->unk_Interpolator_0x70.currentValue = 0;
    func_8003A89C(manager, level, steps);
    manager->unk_Flags = (s16)((u16)manager->unk_Flags | 0x8000u);
    EnableEvent(g_unk_SoundEvent);
}

void func_80039B68(AudioManager *manager, s32 level, s32 steps)
{
    if (!PcPort_AdmitSoundSequence())
        return;
    if (!manager) {
        SoundHandleError(5);
        return;
    }

    unsigned count = manager->elementCount;
    if (!count) {
        manager->unk_Flags = (s16)((u16)manager->unk_Flags & 0x7fffu);
        return;
    }
    for (unsigned i = 0; i < count; ++i) {
        AudioElement *element = &manager->elements[i];
        unsigned bankId = (u16)element->unk_0x24 >> 8;
        SoundWDSEntry *bank = SoundFindWdsEntry((int)bankId);
        if (!bank) {
            manager->unk_Flags = (s16)((u16)manager->unk_Flags & 0x7fffu);
            return;
        }

        element->unk2C = SOUND_PTR_TO_PSX(bank);
        const u8 *instrument = (const u8 *)(bank + 1) + 16u * element->unk_0x26;
        u32 start;
        u16 loop;
        memcpy(&start, instrument, sizeof(start));
        memcpy(&loop, instrument + 4, sizeof(loop));
        SoundVoiceData *voice = &element->voice_data;
        voice->startAddress = (u32)bank->spuMemoryAddress + 8u * start;
        voice->loopAddress = 8u * (start + loop);
        voice->flags = 0xffffu;
    }

    manager->unk_Flags = (s16)((u16)manager->unk_Flags | 0x0100u);
    manager->unk_Interpolator_0x70.currentValue = 0;
    func_8003A89C(manager, level, steps);
}
