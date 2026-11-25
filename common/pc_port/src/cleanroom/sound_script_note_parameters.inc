u8 *SoundScriptSetOctave(u8 *script, AudioManager *manager, AudioElement *element)
{
    (void)manager;
    u8 octave = *(volatile u8 *)script;
    ((volatile AudioElement *)element)->octave = octave * 12;
    return script + 1;
}

u8 *SoundScriptFermata(u8 *script, AudioManager *manager, AudioElement *element)
{
    (void)manager;
    volatile AudioElement *voice = element;
    u16 flags = voice->active_flag;
    u8 duration = *(volatile u8 *)script;
    voice->active_flag = flags | 0x0100;
    voice->fermataDuration = duration;
    return script + 1;
}

u8 *SoundScriptSetAttackTime(u8 *script, AudioManager *manager, AudioElement *element)
{
    (void)manager;
    volatile AudioElement *voice = element;
    u16 flags = voice->voice_data.flags;
    u8 attack = *(volatile u8 *)script;
    voice->voice_data.flags = flags | 0x0010;
    voice->voice_data.unkAdsr4 = attack;
    return script + 1;
}

u8 *SoundScriptSetSustain(u8 *script, AudioManager *manager, AudioElement *element)
{
    (void)manager;
    volatile AudioElement *voice = element;
    u16 flags = voice->voice_data.flags;
    u8 sustain = *(volatile u8 *)script;
    voice->voice_data.flags = flags | 0x0040;
    voice->voice_data.unkAdsr5 = sustain;
    return script + 1;
}
