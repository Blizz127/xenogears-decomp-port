u8 *SoundScriptSaveOctaveAndIP(u8 *script, AudioManager *manager, AudioElement *element) {
    (void)manager;
    u8 octave = *(const volatile u8 *)&element->octave;
    volatile AudioElement *state = element;
    state->savedScriptIP = SOUND_PTR_TO_PSX(script);
    state->savedOctave = octave;
    return script;
}

void SoundTickInterpolator(AudioInterpolator *interpolator) {
    volatile AudioInterpolator *state = interpolator;
    u16 remaining = (u16)((u16)state->counter - 1u);
    state->counter = (s16)remaining;
    if (remaining == 0) {
        state->currentValue = (s32)((u32)(u16)state->targetValue << 16);
    } else {
        u32 value = (u32)state->currentValue;
        u32 delta = (u32)state->stepIncrement;
        state->currentValue = (s32)(value + delta);
    }
}
