/* SPDX-License-Identifier: MIT */
static int SoundEnvelopeSlotAvailable(unsigned slot)
{
    if (slot >= 16 || !soundCallableMethods[slot])
        return 0;
#if defined(XENO_SOUND_INCLUDE_RESIDENT_HYBRID) && !defined(XENO_NATIVE_ONLY)
    return PcPort_SoundResidentHybridEnabled();
#else
    return 1;
#endif
}

SoundPsxAddress PcPort_SoundEnvelopeMethodKey(unsigned slot)
{
    if (!SoundEnvelopeSlotAvailable(slot))
        return 0;

    uintptr_t native = (uintptr_t)soundCallableMethods[slot];
    if (native <= UINT32_MAX)
        return (SoundPsxAddress)native;
    return SoundCallbackAddress(soundEnvelopeWords + slot * 4);
}

static int SoundEnvelopeSlotForWord(SoundPsxAddress word)
{
    if (!word)
        return -1;

    int match = -1;
    for (unsigned slot = 0; slot < 16; ++slot) {
        if (!SoundEnvelopeSlotAvailable(slot))
            continue;
        uintptr_t native = (uintptr_t)soundCallableMethods[slot];
        uint32_t guest = SoundCallbackAddress(soundEnvelopeWords + slot * 4);
        if (word != guest && (native > UINT32_MAX || word != (SoundPsxAddress)native))
            continue;
        if (match >= 0 && soundCallableMethods[match] != soundCallableMethods[slot])
            return -1;
        match = (int)slot;
    }
    return match;
}

int PcPort_CanSoundEnvelopeWord(SoundPsxAddress word)
{
    return SoundEnvelopeSlotForWord(word) >= 0;
}

int PcPort_TrySoundEnvelopeWord(SoundPsxAddress word, SoundEnvelope *envelope, s32 *value)
{
    int slot = SoundEnvelopeSlotForWord(word);
    return slot >= 0 && PcPort_TrySoundEnvelopeMethod((unsigned)slot, envelope, value);
}
