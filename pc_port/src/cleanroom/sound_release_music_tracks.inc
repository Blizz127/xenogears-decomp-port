/* SPDX-License-Identifier: MIT */
static AudioElement *SoundReleaseTrackAt(AudioManager *manager, size_t index)
{
    /* SoundCalculateAudioManagerSize reserves the configured track count. */
    uint8_t *tracks = (uint8_t *)manager + offsetof(AudioManager, elements);
    return (AudioElement *)(tracks + index * sizeof(AudioElement));
}

static void SoundReleaseActiveTrack(AudioManager *manager, AudioElement *track)
{
    const unsigned channel_bit = (uint8_t)track->unk_0x06[0] & 31u;
    track->active_flag = 0;
    manager->unk_0x48 = (s32)((uint32_t)manager->unk_0x48 & ~(UINT32_C(1) << channel_bit));
    SoundReleaseVoiceFromChannel(&track->voice_data, track->voice_number);
}

void func_8003A14C(s32 sound_id)
{
    AudioManager *manager = SOUND_PSX_TO_PTR(AudioManager, D_800595D8);
    const s32 count = D_80059478;
    if (!manager || count <= 0)
        return;

    for (s32 index = 0; index < count; ++index) {
        AudioElement *track = SoundReleaseTrackAt(manager, (size_t)index);
        if (track->active_flag & 1u) {
            uint32_t id;
            memcpy(&id, track->unk_0x06 + 2, sizeof id);
            if (id == (uint32_t)sound_id)
                SoundReleaseActiveTrack(manager, track);
        }
    }
}

void func_8003A20C(s32 selector)
{
    AudioManager *manager = SOUND_PSX_TO_PTR(AudioManager, D_800595D8);
    const unsigned first = ((uint8_t)selector & 0xfeu) ^ 8u;
    /* Both members must belong to the allocation created by func_8003B148. */
    if (!manager || D_80059478 <= (s32)(first + 1u))
        return;

    for (unsigned index = first; index < first + 2u; ++index) {
        AudioElement *track = SoundReleaseTrackAt(manager, index);
        if (track->active_flag & 1u)
            SoundReleaseActiveTrack(manager, track);
    }
}
