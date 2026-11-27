void SoundSetVoiceKeyOff(u32 voiceFlags)
{
    SpuUnion *bank = *(SpuUnion *volatile *)&g_pSoundSpuRegisters;
    bank->raw[198] = (u16)voiceFlags;
    bank->raw[199] = (u16)(voiceFlags >> 16);
}

void SoundSetVoiceAdsrReleaseShiftAndMode(s32 voiceIndex, s32 releaseRate, s32 releaseModeBit2)
{
    SpuUnion *bank = *(SpuUnion *volatile *)&g_pSoundSpuRegisters;
    u32 offset = (u32)voiceIndex * 16u + 10u;
    if (offset >= sizeof bank->raw) {
        return;
    }

    volatile u16 *reg = &bank->raw[offset / 2u];
    u16 old = *reg;
    u32 mode = ((u32)releaseModeBit2 >> 2) * 32u;
    *reg = (u16)((old & 0xffc0u) + (u32)releaseRate + mode);
}
