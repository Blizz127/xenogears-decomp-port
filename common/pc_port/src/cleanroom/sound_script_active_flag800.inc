u8 *SoundScriptSetActiveFlag800(u8 *script, AudioManager *manager, AudioElement *element)
{
    (void)manager;
    ((volatile AudioElement *)element)->active_flag |= 0x0800;
    return script;
}

u8 *SoundScriptClearActiveFlag800(u8 *script, AudioManager *manager, AudioElement *element)
{
    (void)manager;
    ((volatile AudioElement *)element)->active_flag &= 0xf7ff;
    return script;
}
