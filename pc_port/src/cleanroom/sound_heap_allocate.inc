void *SoundHeapAllocate(u32 requestedSize)
{
    const volatile u32 *const event = &g_unk_SoundEvent;
    const u32 required = ((requestedSize + 15u) & ~15u) + 16u;

    DisableEvent((long)*event);
    volatile SoundHeapBlockHeader *node =
        *(SoundHeapBlockHeader *volatile *)&g_SoundHeapHead;
    for (;;) {
        if (node->pNext) {
            const u32 next = node->pNext;
            const u32 usedEnd = node->pPrev;

            if (next - usedEnd >= required) {
                break;
            }
            node = SOUND_PSX_TO_PTR(SoundHeapBlockHeader, next);
        } else {
            const u32 heapEnd = *(const volatile u32 *)&g_SoundHeapEnd;
            const u32 usedEnd = node->pPrev;

            if (heapEnd - usedEnd < required) {
                return NULL;
            }
            break;
        }
    }

    const u32 headerAddress = (node->pPrev + 15u) & ~15u;
    volatile SoundHeapBlockHeader *const header =
        SOUND_PSX_TO_PTR(SoundHeapBlockHeader, headerAddress);
    const u32 payloadAddress = headerAddress + 16u;
    void *const payload = SOUND_PSX_TO_PTR(void, payloadAddress);
    const u32 selectedEvent = *event;

    header->pPrev = payloadAddress + requestedSize;
    header->pNext = 0;
    header->unk4 = 0;
    header->unk0 = 2;
    header->unk2 = 0;
    header->pNext = node->pNext;
    node->pNext = headerAddress;

    EnableEvent((long)selectedEvent);
    const s32 clearSize = requestedSize <= 0x7fffffffu
        ? (s32)requestedSize : -1 - (s32)~requestedSize;
    SoundHeapClearBlockMemory(payload, clearSize);
    return payload;
}
