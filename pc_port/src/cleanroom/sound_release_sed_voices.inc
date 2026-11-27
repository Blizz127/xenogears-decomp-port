/* SPDX-License-Identifier: MIT */
#include <stdint.h>
#include <string.h>

void func_8003A094(SoundFile *file)
{
    AudioManager *manager = SOUND_PSX_TO_PTR(AudioManager, D_800595D8);
    const s32 count = D_80059478;
    if (!manager || !file)
        return;

    const u16 bank = file->sedId;
    for (s32 index = 0; index < count; ++index) {
        AudioElement *element = &manager->elements[index];
        if (!(element->active_flag & 1u))
            continue;

        u16 element_bank;
        memcpy(&element_bank, (const uint8_t *)element + 0x0a, sizeof element_bank);
        if (element_bank == bank) {
            const unsigned bit = (u8)element->unk_0x06[0] & 31u;
            element->active_flag = 0;
            manager->unk_0x48 = (s32)((u32)manager->unk_0x48 & ~(1u << bit));
            SoundReleaseVoiceFromChannel(&element->voice_data, element->voice_number);
        }
    }
}
