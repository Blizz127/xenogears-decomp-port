extern int SoundFileComputeChecksum(SoundFile *file);
extern void SoundReleaseAllVoices(AudioManager *manager);

static void SoundApplyActiveElementFlags(AudioManager *manager, u16 flags, unsigned int fieldOffset)
{
    unsigned int count = ((const volatile AudioManager *)manager)->elementCount;
    for (unsigned int index = 0; index < count; ++index) {
        /* Allocations carry the requested number of elements after the fixed header. */
        u8 *record = (u8 *)manager + __builtin_offsetof(AudioManager, elements) + index * sizeof(AudioElement);
        if (((const volatile AudioElement *)record)->active_flag != 0) {
            volatile u16 *field = (volatile u16 *)(record + fieldOffset);
            *field |= flags;
        }
    }
}

void unk_SoundSetFlagsOnActiveVoices(u16 flags, AudioManager *manager)
{
    SoundApplyActiveElementFlags(manager, flags, __builtin_offsetof(AudioElement, status_flags));
}

void SoundSetFlagsOnActiveVoices(AudioManager *manager, s32 flags)
{
    SoundApplyActiveElementFlags(manager, (u16)flags, __builtin_offsetof(AudioElement, voice_data.flags));
}

int SoundValidateFile(SoundFile *file, u32 signature, u16 version)
{
    const volatile SoundFile *record = file;
    if (record->magic != signature)
        return SOUND_ERR_INVALID_SIGNATURE;
    if (SoundFileComputeChecksum(file) != 0)
        return SOUND_ERR_INVALID_CHECKSUM;
    return record->unkC == version ? SOUND_STATUS_OK : SOUND_ERR_UNK_0X4;
}

void func_80039CC4(void)
{
    AudioManager *manager = *(AudioManager *volatile *)&g_SoundAudioManagerListHead;
    while (manager != NULL) {
        volatile AudioManager *record = manager;
        u16 flags = (u16)record->unk_Flags;
        if ((flags & 1u) != 0) {
            record->unk_Flags = (s16)(flags & 0x7fffu);
            SoundReleaseAllVoices(manager);
        }
        manager = SOUND_PSX_TO_PTR(AudioManager, record->next);
    }
}
