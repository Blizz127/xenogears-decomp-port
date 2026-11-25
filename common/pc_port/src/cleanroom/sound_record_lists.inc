void SoundAddSedsEntry(SoundFile *record)
{
    SoundFile *volatile *const head = &g_SoundSedsLinkedList;
    const volatile u32 *const event = &g_unk_SoundEvent;

    if (!record) {
        SoundHandleError(1);
        return;
    }
    if (!(*(const volatile u16 *)&g_SoundControlFlags & 0x80u)) {
        const volatile SoundFile *node = *head;
        if (node) {
            const u16 id = ((const volatile SoundFile *)record)->sedId;
            do {
                if (node->sedId == id) {
                    SoundHandleError(0x15);
                    return;
                }
                node = SOUND_PSX_TO_PTR(SoundFile, node->pNext);
            } while (node);
        }
    }

    const u16 status = (u16)SoundValidateFile(record, 0x73646573u, 0x0101u);
    if (status) {
        SoundHandleError(status & 0x8000u ? (s32)status - 65536 : (s32)status);
        return;
    }

    DisableEvent((long)*event);
    if (*head) {
        volatile SoundFile *tail = *head;
        while (tail->pNext) {
            tail = SOUND_PSX_TO_PTR(SoundFile, tail->pNext);
        }
        tail->pNext = SOUND_PTR_TO_PSX(record);
    } else {
        *head = record;
    }
    const u32 enableToken = *event;
    ((volatile SoundFile *)record)->pNext = 0;
    EnableEvent((long)enableToken);
}

void SoundFreeWdsEntry(SoundWDSEntry *target)
{
    SoundWDSEntry *volatile *const head = &g_SoundWdsLinkedList;
    const volatile u32 *const event = &g_unk_SoundEvent;
    volatile SoundWDSEntry *previous = NULL;
    volatile SoundWDSEntry *node = *head;

    while (node && node != target) {
        previous = node;
        node = SOUND_PSX_TO_PTR(SoundWDSEntry, node->pNext);
    }
    if (!node) {
        SoundHandleError(0x11);
        return;
    }

    DisableEvent((long)*event);
    const SoundPsxAddress next = node->pNext;
    if (previous) {
        previous->pNext = next;
    } else {
        *head = SOUND_PSX_TO_PTR(SoundWDSEntry, next);
    }
    EnableEvent((long)*event);

    const int freed = SoundSpuMemoryFreeBlock(node->spuMemoryAddress);
    if (node->spuMemoryAddress != freed) {
        SoundHandleError(0x24);
    }
    SoundHeapFree(target);
}

s32 SoundRemoveAudioManagerFromList(AudioManager *target)
{
    AudioManager *volatile *const head = &g_SoundAudioManagerListHead;
    volatile AudioManager *previous = NULL;
    volatile AudioManager *node = *head;

    while (node && node != target) {
        previous = node;
        node = SOUND_PSX_TO_PTR(AudioManager, node->next);
    }
    if (!node) {
        SoundHandleError(0x0f);
        return -1;
    }

    volatile u16 *const flags = (volatile u16 *)&node->unk_Flags;
    const u16 state = *flags;
    if (state & 0x8000u) {
        *flags = (u16)(state & 0x7fffu);
        SoundReleaseAllVoices(target);
    }
    const SoundPsxAddress next = node->next;
    if (previous) {
        previous->next = next;
    } else {
        *head = SOUND_PSX_TO_PTR(AudioManager, next);
    }
    return 0;
}
