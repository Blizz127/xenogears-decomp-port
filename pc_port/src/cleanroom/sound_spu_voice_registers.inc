void SoundSetVoiceKeyOn(u32 voiceFlags)
{
    SpuUnion *const bank = *(SpuUnion *volatile *)&g_pSoundSpuRegisters;

    bank->raw[0x188 / 2] = (u16)voiceFlags;
    bank->raw[0x18a / 2] = (u16)(voiceFlags >> 16);
}

void SoundSetReverbVoices(u32 voiceFlags)
{
    SpuUnion *const bank = *(SpuUnion *volatile *)&g_pSoundSpuRegisters;

    bank->raw[0x198 / 2] = (u16)voiceFlags;
    bank->raw[0x19a / 2] = (u16)(voiceFlags >> 16);
}

void SoundSetVoiceVolume(s32 voiceIndex, s32 volL, s32 volR)
{
    SpuUnion *const bank = *(SpuUnion *volatile *)&g_pSoundSpuRegisters;
    const u32 byteOffset = (u32)voiceIndex * 16u;

    if (byteOffset >= sizeof(bank->raw)) {
        return;
    }
    bank->raw[byteOffset / 2] = (u16)volL;
    bank->raw[byteOffset / 2 + 1] = (u16)volR;
}

void SoundSetVoicePitch(s32 voiceIndex, s32 pitch)
{
    SpuUnion *const bank = *(SpuUnion *volatile *)&g_pSoundSpuRegisters;
    const u32 byteOffset = (u32)voiceIndex * 16u + 4u;

    if (byteOffset < sizeof(bank->raw)) {
        bank->raw[byteOffset / 2] = (u16)pitch;
    }
}

void SoundSetVoiceStartAddress(s32 voiceIndex, s32 addr)
{
    SpuUnion *const bank = *(SpuUnion *volatile *)&g_pSoundSpuRegisters;
    const u32 byteOffset = (u32)voiceIndex * 16u + 6u;

    if (byteOffset < sizeof(bank->raw)) {
        bank->raw[byteOffset / 2] = (u16)((u32)addr >> 3);
    }
}
