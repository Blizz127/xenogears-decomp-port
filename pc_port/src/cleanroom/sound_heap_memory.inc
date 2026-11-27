void SoundHeapInitialize(void *startAddress, unsigned int size)
{
    uintptr_t address = (uintptr_t)startAddress;
    u32 capacity = (u32)size & ~15u;
    if (address & 15u) {
        address = (address + 15u) & ~(uintptr_t)15u;
        capacity -= 16u;
    }

    SoundHeapBlockHeader *const head = (SoundHeapBlockHeader *)address;
    volatile SoundHeapBlockHeader *const header = head;
    const u32 addressWord = SOUND_PTR_TO_PSX(head);

    *(volatile u32 *)&g_SoundHeapEnd = addressWord + capacity;
    header->unk0 = 0x8000u;
    *(SoundHeapBlockHeader *volatile *)&g_SoundHeapHead = head;
    *(volatile u32 *)&g_SoundHeapSize = capacity;
    header->unk2 = 0;
    header->unk4 = 0;
    header->pPrev = addressWord + 16u;
    header->pNext = 0;
}

/* Native byte buffers can start on any alignment and contain any object type. */
typedef u32 SoundHeapTransferWord __attribute__((__aligned__(1), __may_alias__));

void SoundHeapSetBlockMemory(void *destination, void *source, int size)
{
    volatile unsigned char *out = destination;
    const volatile unsigned char *in = source;

    while (size >= 16) {
        const u32 second = *(const volatile SoundHeapTransferWord *)(in + 4);
        const u32 third = *(const volatile SoundHeapTransferWord *)(in + 8);
        const u32 fourth = *(const volatile SoundHeapTransferWord *)(in + 12);
        const u32 first = *(const volatile SoundHeapTransferWord *)in;

        *(volatile SoundHeapTransferWord *)out = first;
        *(volatile SoundHeapTransferWord *)(out + 4) = second;
        *(volatile SoundHeapTransferWord *)(out + 8) = third;
        *(volatile SoundHeapTransferWord *)(out + 12) = fourth;
        in += 16;
        out += 16;
        size -= 16;
    }
    while (size >= 4) {
        const u32 word = *(const volatile SoundHeapTransferWord *)in;

        *(volatile SoundHeapTransferWord *)out = word;
        in += 4;
        out += 4;
        size -= 4;
    }
    while (size > 0) {
        *out++ = *in++;
        --size;
    }
}

void SoundHeapClearBlockMemory(void *destination, s32 size)
{
    volatile unsigned char *out = destination;

    while (size >= 16) {
        *(volatile SoundHeapTransferWord *)(out + 12) = 0;
        *(volatile SoundHeapTransferWord *)(out + 8) = 0;
        *(volatile SoundHeapTransferWord *)(out + 4) = 0;
        *(volatile SoundHeapTransferWord *)out = 0;
        out += 16;
        size -= 16;
    }
    while (size >= 4) {
        *(volatile SoundHeapTransferWord *)out = 0;
        out += 4;
        size -= 4;
    }
    while (size > 0) {
        *out++ = 0;
        --size;
    }
}

SoundSpuMemoryBlock *SoundSpuMemoryFindBlock(s32 targetAddress)
{
    const volatile SoundSpuMemoryBlock *const head = &g_SoundSpuMemoryBlocks[0];

    if (head->spuAddress == targetAddress) {
        return &g_SoundSpuMemoryBlocks[0];
    }
    (void)head->nextBlockIndex;
    return NULL;
}
