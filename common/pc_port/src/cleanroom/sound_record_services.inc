int SoundFileComputeChecksum(SoundFile *file)
{
    typedef u32 ChecksumWord __attribute__((__may_alias__));
    const u32 length = ((const volatile SoundFile *)file)->unk8;
    u32 words = (length + 3u) / 4u;
    uintptr_t address = (uintptr_t)file;
    u32 sum = 0;

    while (words) {
        sum += *(const volatile ChecksumWord *)address;
        address += sizeof(ChecksumWord);
        --words;
    }
    return sum <= 0x7fffffffu ? (int)sum : -1 - (int)~sum;
}

SoundWDSEntry *SoundFindWdsEntry(int targetID)
{
    SoundWDSEntry *node = *(SoundWDSEntry *volatile *)&g_SoundWdsLinkedList;

    while (node) {
        const volatile SoundWDSEntry *const fields = node;
        if ((int)fields->id == targetID) {
            return node;
        }
        node = SOUND_PSX_TO_PTR(SoundWDSEntry, fields->pNext);
    }
    return NULL;
}

u32 SoundSpuMemoryAllocateWDS(SoundWDSEntry *file, int mode)
{
    const volatile SoundWDSEntry *const fields = file;
    const int automatic = mode == 0 ? fields->spuMemoryAddress == 0 : mode == -1;
    const s32 size = *(const volatile s32 *)&fields->adpcmDataSize;

    if (automatic) {
        const s32 option = fields->unk1E;
        return SoundSpuMemoryAllocateBlock(size, option);
    }
    const s32 address = fields->spuMemoryAddress;
    const s32 option = fields->unk1E;
    return SoundSpuMemoryAllocateBlockAtAddress(size, address, option);
}

void SoundAddAudioManagerToList(AudioManager *manager)
{
    const volatile u32 *const event = &g_unk_SoundEvent;
    AudioManager *volatile *const head = &g_SoundAudioManagerListHead;

    DisableEvent((long)*event);
    const u32 enableToken = *event;
    AudioManager *const previous = *head;
    *head = manager;
    ((volatile AudioManager *)manager)->next = SOUND_PTR_TO_PSX(previous);
    EnableEvent((long)enableToken);
}

void SoundCopyAudioManagerData(AudioManager *dest, AudioManager *src)
{
    volatile AudioManager *const fields = dest;
    const s32 count = fields->elementCount;
    const SoundPsxAddress next = fields->next;
    const SoundPsxAddress managerLink = fields->unk_Manager_0x4;
    const s32 size = SoundCalculateAudioManagerSize(count);

    SoundHeapSetBlockMemory(dest, src, size);
    fields->next = next;
    fields->unk_Manager_0x4 = managerLink;
}
