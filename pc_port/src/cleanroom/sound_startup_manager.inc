/* SPDX-License-Identifier: MIT */
AudioManager *func_8003B148(s32 requested)
{
    const s32 channels = requested & ~1;
    D_80059478 = channels;
    const u32 bytes = (u32)SoundCalculateAudioManagerSize(channels);
    AudioManager *manager = SoundHeapAllocate(bytes);
    if (!manager) {
        SoundHandleError(30);
        return NULL;
    }

    func_8003B32C(manager);
    if (channels <= 0) {
        SoundHeapFree(manager);
        SoundHandleError(30);
        return NULL;
    }
    uint8_t *voice = (uint8_t *)manager + offsetof(AudioManager, elements);
    for (s32 index = 0; index < channels; ++index, voice += sizeof(AudioElement)) {
        const uint16_t inactive = 0;
        memcpy(voice, &inactive, sizeof inactive);
        voice[6] = (uint8_t)index;
        voice[offsetof(AudioElement, voice_number)] = (uint8_t)(24u - (uint32_t)channels + (uint32_t)index);
    }
    SoundAddAudioManagerToList(manager);
    return manager;
}
