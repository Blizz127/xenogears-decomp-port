/* SPDX-License-Identifier: MIT */
void SoundSetupCdMix(s32 level)
{
    const uint8_t attenuation = (g_SoundControlFlags & 0x700u) ? 0 : ((uint32_t)level >> 1);
    g_SoundVolumeController.unk_field2 = (int16_t)level;
    g_SoundCdRomAttenuation = (CdlATV){attenuation, attenuation, attenuation, attenuation};
    CdMix(&g_SoundCdRomAttenuation);
}

void SoundTransferWdsPart(u8 *data, s32 requested)
{
    if (g_SoundWdsRemainingBytes == 0)
        return;

    const int32_t amount = requested < g_SoundWdsRemainingBytes ? requested : g_SoundWdsRemainingBytes;
    SoundQueueSpuWriteCommand((uint32_t)g_SoundWdsCurSpuAddress, data, (uint32_t)amount, NULL);
    g_SoundWdsCurSpuAddress = (int32_t)((uint32_t)g_SoundWdsCurSpuAddress + (uint32_t)amount);
    g_SoundWdsRemainingBytes = (int32_t)((uint32_t)g_SoundWdsRemainingBytes - (uint32_t)amount);
}
