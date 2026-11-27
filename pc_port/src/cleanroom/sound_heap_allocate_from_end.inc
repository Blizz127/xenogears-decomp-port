void *func_80039024(s32 size)
{
    const u32 alignmentMask = 15u;
    u32 required = (((u32)size + alignmentMask) & ~alignmentMask) + sizeof(SoundHeapBlockHeader);
    SoundHeapBlockHeader *chosen = NULL;
    u32 upper = 0;

    DisableEvent(g_unk_SoundEvent);
    SoundHeapBlockHeader *block = g_SoundHeapHead;
    for (;;) {
        u32 successor = block->pNext;
        u32 limit = successor ? successor : g_SoundHeapEnd;
        if ((s32)(limit - block->pPrev) >= (s32)required) {
            chosen = block;
            upper = limit;
        }
        if (!successor)
            break;
        block = SOUND_PSX_TO_PTR(SoundHeapBlockHeader, successor);
    }
    if (!chosen)
        return NULL;

    u32 address = (upper - required + alignmentMask) & ~alignmentMask;
    u32 payload = address + sizeof(SoundHeapBlockHeader);
    SoundHeapBlockHeader *header = SOUND_PSX_TO_PTR(SoundHeapBlockHeader, address);
    header->pPrev = payload + (u32)size;
    header->pNext = 0;
    header->unk4 = 0;
    header->unk0 = 2;
    header->unk2 = 0;
    header->pNext = chosen->pNext;
    chosen->pNext = address;

    EnableEvent(g_unk_SoundEvent);
    void *allocation = SOUND_PSX_TO_PTR(void, payload);
    SoundHeapClearBlockMemory(allocation, size);
    return allocation;
}
