void SoundSetVoiceLoopAddress(s32 voiceIndex, s32 address)
{
    SpuUnion *const bank = *(SpuUnion *volatile *)&g_pSoundSpuRegisters;
    const u32 offset = (u32)voiceIndex * 16u + 14u;

    if (offset < sizeof(bank->raw)) {
        volatile u16 *const loop = &bank->raw[offset / 2u];
        *loop = (u16)((u32)address >> 3);
        if ((u32)voiceIndex < 32u) {
            const u16 storedAddress = *loop;
            SpuSetVoiceLoopStartAddr(voiceIndex, (u32)storedAddress * 8u);
        }
    }
}
