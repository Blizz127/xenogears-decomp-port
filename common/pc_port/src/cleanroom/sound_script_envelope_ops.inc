/* The byte cursor can point into the voice record. Sample each operand at the
 * point where the sequencer consumes it, and preserve ordered byte stores. */
static u8 sound_script_envelope_operand(const volatile u8 *script,
                                        volatile AudioElement *element, u16 mask)
{
    u16 old_flags = element->voice_data.flags;
    u8 value = *script;
    element->voice_data.flags = (u16)(old_flags | mask);
    return value;
}

u8 *SoundScriptSetAttackMode(u8 *script, AudioManager *manager, AudioElement *element)
{
    volatile AudioElement *state = element;
    u8 value = sound_script_envelope_operand(script, state, 0x0010u);

    (void)manager;
    state->voice_data.unkAdsr1 = value;
    return script + 1;
}

u8 *SoundScriptSetDecayTime(u8 *script, AudioManager *manager, AudioElement *element)
{
    volatile AudioElement *state = element;
    u8 value = sound_script_envelope_operand(script, state, 0x0020u);

    (void)manager;
    state->voice_data.adsrDR = value;
    return script + 1;
}

u8 *SoundScriptSetRelease(u8 *script, AudioManager *manager, AudioElement *element)
{
    volatile AudioElement *state = element;
    u8 value = sound_script_envelope_operand(script, state, 0x0080u);

    (void)manager;
    state->unk_0x28 = value;
    state->voice_data.unkAdsr6 = value;
    return script + 1;
}

u8 *SoundScriptSetReleaseMode(u8 *script, AudioManager *manager, AudioElement *element)
{
    volatile AudioElement *state = element;
    u8 value = sound_script_envelope_operand(script, state, 0x0080u);

    (void)manager;
    state->voice_data.unkAdsr3 = value;
    return script + 1;
}

u8 *SoundScriptSetSustainLevel(u8 *script, AudioManager *manager, AudioElement *element)
{
    volatile AudioElement *state = element;
    u8 value = sound_script_envelope_operand(script, state, 0x0100u);

    (void)manager;
    state->voice_data.adsrSR = value;
    return script + 1;
}

u8 *SoundScriptSetSustainMode(u8 *script, AudioManager *manager, AudioElement *element)
{
    volatile AudioElement *state = element;
    u8 value = sound_script_envelope_operand(script, state, 0x0040u);

    (void)manager;
    state->voice_data.unkAdsr2 = value;
    return script + 1;
}

u8 *SoundScriptSetAdsrDRAndSR(u8 *script, AudioManager *manager, AudioElement *element)
{
    volatile AudioElement *state = element;
    const volatile u8 *operands = script;
    u8 decay = operands[0];
    u16 flags;
    u8 sustain;

    (void)manager;
    state->voice_data.adsrDR = decay;
    flags = state->voice_data.flags;
    sustain = operands[1];
    state->voice_data.flags = (u16)(flags | 0x0120u);
    state->voice_data.adsrSR = sustain;
    return script + 2;
}
