/* The dispatch table always supplies three host pointers, including for
 * direct-entry compatibility symbols that historically have integer slots. */
static u8 *sound_resident_sequence(const SoundResidentEntry *entry, u8 *stream,
                                   AudioManager *manager, AudioElement *element)
{
    const uintptr_t args[3] = { (uintptr_t)stream, (uintptr_t)manager, (uintptr_t)element };
    return (u8 *)sound_resident_run(entry, args);
}

u8 *PcPort_SoundSequenceD2D0(u8 *stream, AudioManager *manager, AudioElement *element)
{
    static const SoundResidentEntry entry = { 0x8003d2d0u, 48, { 1, 0, 97 }, 7, 4, 4, 0 };
    return sound_resident_sequence(&entry, stream, manager, element);
}

u8 *PcPort_SoundSequenceD300(u8 *stream, AudioManager *manager, AudioElement *element)
{
    static const SoundResidentEntry entry = { 0x8003d300u, 40, { 0, 16, 2 }, 7, 4, 4, 0 };
    return sound_resident_sequence(&entry, stream, manager, element);
}

u8 *PcPort_SoundSequenceD79C(u8 *stream, AudioManager *manager, AudioElement *element)
{
    static const SoundResidentEntry entry = { 0x8003d79cu, 44, { 1, 0, 112 }, 7, 4, 4, 0 };
    return sound_resident_sequence(&entry, stream, manager, element);
}

u8 *PcPort_SoundSequenceE160(u8 *stream, AudioManager *manager, AudioElement *element)
{
    static const SoundResidentEntry entry = { 0x8003e160u, 32, { 0, 0, 312 }, 7, 4, 4, 0 };
    return sound_resident_sequence(&entry, stream, manager, element);
}

s32 PcPort_SoundEnvelopeF190(SoundEnvelope *envelope)
{
    static const SoundResidentEntry entry = { 0x8003f190u, 20, { 32, 0, 0 }, 1, 1, 0, 0 };
    const uintptr_t args[3] = { (uintptr_t)envelope, 0, 0 };
    return (s32)sound_resident_run(&entry, args);
}
