/* SPDX-License-Identifier: MIT */
static AudioElement *SoundGroupElement(AudioManager *manager, s32 index)
{
    return (AudioElement *)((uint8_t *)manager + offsetof(AudioManager, elements)
                          + (size_t)index * sizeof(AudioElement));
}

s32 func_8003A65C(s32 sound_id, s32 group_size)
{
    AudioManager *manager = SOUND_PSX_TO_PTR(AudioManager, D_800595D8);
    const s32 channel_count = D_80059478;
    if (!manager || group_size <= 0 || group_size > 32)
        return -1;

    for (s32 i = 0; i < channel_count; ++i) {
        AudioElement *element = SoundGroupElement(manager, i);
        uint32_t id;
        memcpy(&id, element->unk_0x06 + 2, sizeof id);
        if (!(element->active_flag & 1u) || id != (uint32_t)sound_id)
            continue;
        element->active_flag = 0;
        manager->unk_0x48 &= (int32_t)~(UINT32_C(1) << ((uint8_t)element->unk_0x06[0] & 31u));
        SoundReleaseVoiceFromChannel(&element->voice_data, element->voice_number);
    }

    const int32_t start = (int32_t)((uint32_t)D_80059478 - (uint32_t)group_size - 2u);
    const uint32_t lower = (uint32_t)manager->elementCount - (uint32_t)D_80059544;
    manager = SOUND_PSX_TO_PTR(AudioManager, D_800595D8);
    if (!manager)
        return -1;

    int32_t selected = start;
    uint32_t oldest = UINT32_MAX;
    const uint32_t group_mask = UINT32_MAX >> (32u - (uint32_t)group_size);
    for (int32_t i = start; i >= 0 && (uint32_t)i + (uint32_t)group_size - 1u >= lower; i -= group_size) {
        if (!((uint32_t)manager->unk_0x48 & (group_mask << ((uint32_t)i & 31u))))
            return i;

        const AudioElement *element = SoundGroupElement(manager, i);
        uint32_t serial;
        memcpy(&serial, element->unk_0x06 + 6, sizeof serial);
        if ((uint8_t)element->unk_0x06[1] <= 32u && serial < oldest) {
            selected = i;
            oldest = serial;
        }
    }
    return selected;
}
