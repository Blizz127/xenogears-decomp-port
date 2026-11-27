/* SPDX-License-Identifier: MIT */
void func_800386C4(s32 mode)
{
    uint16_t channel_mode = 0;
    if (mode >= 1 && mode <= 3)
        channel_mode = (uint16_t)((2u * (uint32_t)mode - 1u) << 8);

    g_SoundControlFlags = (uint16_t)((g_SoundControlFlags & ~0x0700u) | channel_mode);
    SoundApplyVolumeSettings();
    SpuSetReverbModeDepth(g_SoundReverbDepth.left, g_SoundReverbDepth.right);
    for (AudioManager *manager = g_SoundAudioManagerListHead; manager;
         manager = SOUND_PSX_TO_PTR(AudioManager, manager->next)) {
        unk_SoundSetFlagsOnActiveVoices(0x100, manager);
    }
    if (g_SoundControlFlags & 0x4000)
        SoundSetupCdMix(g_SoundVolumeController.unk_field2);

    uint8_t *echo = D_80059518;
    if (!echo || !(echo[0] & 1u))
        return;

    uint16_t level;
    memcpy(&level, echo + 0x12, sizeof level);
    const int separated = func_80038824();
    const uint16_t gain = (uint16_t)((uint32_t)level << (separated ? 7 : 6));
    const uint16_t channels[2][3] = {
        {1, gain, separated ? 0 : gain},
        {1, separated ? 0 : gain, gain}
    };
    memcpy(echo + 0x36, channels[0], sizeof channels[0]);
    memcpy(echo + 0x62, channels[1], sizeof channels[1]);
}
