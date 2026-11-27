/* SPDX-License-Identifier: MIT */
void func_8003E900(void)
{
    SpuUnion *const voicePage = g_pSoundSpuRegisters;
    u32 pitchModulation = 0;
    u32 noise = 0;
    u32 reverb = 0;
    u16 dirtyChannels = 0;

    for (s32 channel = 0; channel < NUM_VOICES; ++channel) {
        SoundVoiceData *voice = g_SoundChannels[channel];
        if (!voice)
            continue;

        const u32 bit = 1u << channel;
        const u16 dirty = voice->flags;
        volatile u16 *const regs = &voicePage->raw[channel * 8];

        if (dirty & 1u) {
            regs[0] = (u16)voice->volume.left;
            regs[1] = (u16)voice->volume.right;
        }
        if (dirty & 4u)
            regs[2] = voice->pitch;
        if (dirty & 8u) {
            regs[3] = (u16)(voice->startAddress >> 3);
            SoundSetVoiceLoopAddress(channel, voice->loopAddress);
        }

        /* ADSR parameters merge with the word left by the preceding update.
         * A byte outside its normal field width can carry into adjacent bits. */
        if (dirty & 0x10u) {
            u16 low = *(volatile const u8 *)&regs[4];
            u32 attackMode = (voice->unkAdsr1 & 4u) << 13;
            regs[4] = (u16)(low + (voice->unkAdsr4 << 8) + attackMode);
        }
        if (dirty & 0x20u)
            regs[4] = (u16)((regs[4] & 0xff0fu) + (voice->adsrDR << 4));
        if (dirty & 0x40u) {
            u32 direction = (voice->unkAdsr2 & 6u) << 13;
            regs[5] = (u16)((regs[5] & 0x003fu) + (voice->unkAdsr5 << 6) + direction);
        }
        if (dirty & 0x80u) {
            u32 releaseMode = (voice->unkAdsr3 >> 2) << 5;
            regs[5] = (u16)((regs[5] & 0xffc0u) + voice->unkAdsr6 + releaseMode);
        }
        if (dirty & 0x100u)
            regs[4] = (u16)((regs[4] & 0xfff0u) + voice->adsrSR);

        const u16 modes = voice->modeFlags;
        if (modes & 0x10u)
            pitchModulation |= bit;
        if (modes & 0x20u)
            noise |= bit;
        if (modes & 0x40u)
            reverb |= bit;
        dirtyChannels |= dirty;
        if (dirty)
            voice->flags = 0;
    }

    if (dirtyChannels & 0x1000u) {
        g_pSoundSpuRegisters->raw[0x190 / 2] = (u16)pitchModulation;
        g_pSoundSpuRegisters->raw[0x192 / 2] = (u16)(pitchModulation >> 16);
    }
    if (dirtyChannels & 0x2000u) {
        g_pSoundSpuRegisters->raw[0x194 / 2] = (u16)noise;
        g_pSoundSpuRegisters->raw[0x196 / 2] = (u16)(noise >> 16);
    }
    if (dirtyChannels & 0x4000u) {
        g_pSoundSpuRegisters->raw[0x198 / 2] = (u16)reverb;
        g_pSoundSpuRegisters->raw[0x19a / 2] = (u16)(reverb >> 16);
    }
    if (g_SoundKeyOnFlags) {
        g_pSoundSpuRegisters->raw[0x188 / 2] = (u16)g_SoundKeyOnFlags;
        g_pSoundSpuRegisters->raw[0x18a / 2] = (u16)(g_SoundKeyOnFlags >> 16);
        g_SoundKeyOnFlags = 0;
    }
}
