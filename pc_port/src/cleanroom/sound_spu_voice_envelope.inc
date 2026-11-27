void SoundSetVoiceAdsrAttackModeAndRate(s32 voiceIndex, s32 rate, s32 mode)
{
    SpuUnion *const bank = *(SpuUnion *volatile *)&g_pSoundSpuRegisters;
    const u32 byteOffset = (u32)voiceIndex * 16u + 8u;

    if (byteOffset < sizeof(bank->raw)) {
        volatile u16 *const reg = &bank->raw[byteOffset / 2];
        const unsigned char lowByte = *(const volatile unsigned char *)reg;
        const u32 attackMode = ((u32)mode >> 2) & 1u;

        *reg = (u16)(lowByte + (u32)rate * 256u + attackMode * 32768u);
    }
}

void SoundSetVoiceAdsrDecayShift(s32 voiceIndex, s32 shift)
{
    SpuUnion *const bank = *(SpuUnion *volatile *)&g_pSoundSpuRegisters;
    const u32 byteOffset = (u32)voiceIndex * 16u + 8u;

    if (byteOffset < sizeof(bank->raw)) {
        volatile u16 *const reg = &bank->raw[byteOffset / 2];
        const u16 previous = *reg;

        *reg = (u16)((previous & 0xff0fu) + (u32)shift * 16u);
    }
}

void SoundSetVoiceAdsrSustainLevel(s32 voiceIndex, s32 level)
{
    SpuUnion *const bank = *(SpuUnion *volatile *)&g_pSoundSpuRegisters;
    const u32 byteOffset = (u32)voiceIndex * 16u + 8u;

    if (byteOffset < sizeof(bank->raw)) {
        volatile u16 *const reg = &bank->raw[byteOffset / 2];
        const u16 previous = *reg;

        *reg = (u16)((previous & 0xfff0u) + (u32)level);
    }
}

void SoundSetVoiceAdsrSustainRateAndDirection(s32 voiceIndex, s32 rate, s32 direction)
{
    SpuUnion *const bank = *(SpuUnion *volatile *)&g_pSoundSpuRegisters;
    const u32 byteOffset = (u32)voiceIndex * 16u + 10u;

    if (byteOffset < sizeof(bank->raw)) {
        volatile u16 *const reg = &bank->raw[byteOffset / 2];
        const u16 previous = *reg;
        const u32 directionBits = ((u32)direction >> 1) & 3u;

        *reg = (u16)((previous & 0x003fu) + (u32)rate * 64u
                     + directionBits * 16384u);
    }
}
