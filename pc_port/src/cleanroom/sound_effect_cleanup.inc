/* SPDX-License-Identifier: MIT */
void func_8003852C(void *entry)
{
    SoundFile *previous = NULL;
    SoundFile *file = g_SoundSedsLinkedList;
    while (file && file != entry) {
        previous = file;
        file = SOUND_PSX_TO_PTR(SoundFile, file->pNext);
    }
    if (!file) {
        SoundHandleError(16);
        return;
    }

    func_8003A094(file);
    DisableEvent(g_unk_SoundEvent);
    const SoundPsxAddress next = file->pNext;
    if (previous)
        previous->pNext = next;
    else
        g_SoundSedsLinkedList = SOUND_PSX_TO_PTR(SoundFile, next);
    file->pNext = 0;
    if (SoundValidateFile(file, UINT32_C(0x73646573), 0x0101)) {
        SoundHandleError(11);
        return;
    }
    EnableEvent(g_unk_SoundEvent);
}

void func_80039FF8(void)
{
    AudioManager *manager = SOUND_PSX_TO_PTR(AudioManager, D_800595D8);
    const s32 count = D_80059478;
    DisableEvent(g_unk_SoundEvent);
    if (manager) {
        for (s32 i = 0; i < count; ++i) {
            AudioElement *element = (AudioElement *)((uint8_t *)manager +
                offsetof(AudioManager, elements) + (size_t)i * sizeof(AudioElement));
            if (element->active_flag & 1u) {
                element->active_flag = 0;
                SoundReleaseVoiceFromChannel(&element->voice_data, element->voice_number);
            }
        }
        manager->unk_0x48 = 0;
    }
    EnableEvent(g_unk_SoundEvent);
}
