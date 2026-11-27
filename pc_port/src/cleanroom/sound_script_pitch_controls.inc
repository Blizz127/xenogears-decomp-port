u8 *SoundScriptRaiseOctave(u8 *script, AudioManager *manager, AudioElement *element)
{
    (void)manager;
    ((volatile AudioElement *)element)->octave += 12u;
    return script;
}

u8 *SoundScriptLowerOctave(u8 *script, AudioManager *manager, AudioElement *element)
{
    (void)manager;
    ((volatile AudioElement *)element)->octave -= 12u;
    return script;
}

u8 *SoundScriptSetUnk74(u8 *script, AudioManager *manager, AudioElement *element)
{
    (void)manager;
    volatile AudioElement *voice = element;
    u8 operand = *(volatile u8 *)script;
    u16 status = voice->status_flags;
    voice->unk_0x74 = (u16)(operand * 256u);
    voice->status_flags = status | 0x0100;
    return script + 1;
}

u8 *SoundScriptAddUnk6E(u8 *script, AudioManager *manager, AudioElement *element)
{
    (void)manager;
    volatile AudioElement *voice = element;
    u8 operand = *(volatile u8 *)script;
    s16 parameter = voice->unk_0x6E;
    u16 status = voice->status_flags;

    int delta = operand < 128 ? (int)operand : (int)operand - 256;
    u16 result = (u16)((s32)parameter + delta * 32);
    voice->unk_0x6E = (s16)(result <= 0x7fff ? (s32)result : (s32)result - 65536);
    voice->status_flags = status | 0x0200;
    return script + 1;
}
