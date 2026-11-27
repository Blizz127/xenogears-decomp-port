/* SPDX-License-Identifier: MIT */
void SoundEnableAllSpuChannels(void)
{
    for (unsigned channel = 0; channel < NUM_VOICES; ++channel) {
        SoundVoiceData *voice = g_SoundChannels[channel];
        if (voice)
            voice->flags |= 0x01f5u;
    }
    g_SoundControlFlags = (s16)((u16)g_SoundControlFlags & ~0x40u);
}

void func_80039DB8(s32 packed_id)
{
    if (!((u16)g_SoundControlFlags & 0x0800u))
        return;
    if (!PcPort_AdmitSoundSequence())
        return;
    const u16 selection = (u16)(((u32)D_80059478 - 2u) | 0x8000u);
    D_80059404 = 2;
    func_8003B644((s16)selection, packed_id, 0x6000, 0x4000);
}
