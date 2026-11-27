/* The sequence dispatch table passes complete host pointers for all three arguments. */
extern void func_8003E5BC(s32 instrument, AudioElement *element);

u8 *PcPort_SoundSequenceD298(u8 *stream, AudioManager *manager, AudioElement *element)
{
    (void)manager;
    func_8003E5BC(*stream, element);
    return stream + 1;
}

/* Direct callers use the legacy sign-extended element argument. */
u8 *func_8003D298(u8 *stream, s32 unused, s32 element_address)
{
    (void)unused;
    return PcPort_SoundSequenceD298(stream, NULL, (AudioElement *)(intptr_t)element_address);
}
