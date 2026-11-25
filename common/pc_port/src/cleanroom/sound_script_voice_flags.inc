u8 *SoundScriptPercussionOff(u8 *script, AudioManager *manager, AudioElement *element)
{
    volatile AudioElement *state = element;
    (void)manager;
    state->active_flag &= (u16)~0x0010u;
    return script;
}

u8 *SoundScriptClearUnk04Bit1(u8 *script, AudioManager *manager, AudioElement *element)
{
    volatile AudioElement *state = element;
    (void)manager;
    state->unk_0x04 &= (u16)~0x0001u;
    return script;
}

u8 *SoundScriptToggleUnk04Bit2(u8 *script, AudioManager *manager, AudioElement *element)
{
    volatile AudioElement *state = element;
    (void)manager;
    state->unk_0x04 ^= 0x0002u;
    return script;
}

/* Both voice halfwords are sampled before either update becomes visible. */
static void sound_script_voice_mode(volatile AudioElement *state, u16 flags_to_set,
                                    u16 mode_to_clear, u16 mode_to_set)
{
    u16 flags = state->voice_data.flags;
    u16 mode = state->voice_data.modeFlags;
    state->voice_data.flags = (u16)(flags | flags_to_set);
    state->voice_data.modeFlags = (u16)((mode & (u16)~mode_to_clear) | mode_to_set);
}

u8 *SoundScriptSetVoiceFlags2000AndMode(u8 *script, AudioManager *manager, AudioElement *element)
{
    (void)manager;
    sound_script_voice_mode(element, 0x2000u, 0, 0x0020u);
    return script;
}

u8 *SoundScriptSetVoiceFlags2000ClearMode(u8 *script, AudioManager *manager, AudioElement *element)
{
    (void)manager;
    sound_script_voice_mode(element, 0x2000u, 0x0020u, 0);
    return script;
}

u8 *SoundScriptSetVoiceFlags4000ClearMode(u8 *script, AudioManager *manager, AudioElement *element)
{
    (void)manager;
    sound_script_voice_mode(element, 0x4000u, 0x0040u, 0);
    return script;
}

u8 *SoundScriptSetUnk62(u8 *script, AudioManager *manager, AudioElement *element)
{
    volatile AudioElement *state = element;
    u8 input = *(const volatile u8 *)script;
    (void)manager;
    state->unk_0x62 = (u16)input;
    return script + 1;
}

u8 *SoundScriptSetUnk6E(u8 *script, AudioManager *manager, AudioElement *element)
{
    volatile AudioElement *state = element;
    int input = *(const volatile u8 *)script;
    u16 status = state->status_flags;
    int signed_input = input < 128 ? input : input - 256;
    (void)manager;
    state->unk_0x6E = (s16)(signed_input * 32);
    state->status_flags = (u16)(status | 0x0200u);
    return script + 1;
}
