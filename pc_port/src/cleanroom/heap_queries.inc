static u32 heapNextPayloadWord(const void *payload)
{
    const volatile HeapHeaderWord *next =
        (const volatile HeapHeaderWord *)((uintptr_t)payload - sizeof(HeapBlock));
    return *next;
}

u_int HeapWalkUntilEnd(void)
{
    void *payload = *(void *volatile *)&g_Heap;
    while (heapWordOwner(*heapMetadataWord(payload)) != 1u) {
        payload = (void *)(uintptr_t)heapNextPayloadWord(payload);
    }
    return 0;
}

static u32 heapFreeSpace(int largest)
{
    void *payload = *(void *volatile *)&g_Heap;
    u32 result = largest ? (u32)sizeof(HeapBlock) : 0u;

    while (heapWordOwner(*heapMetadataWord(payload)) != 1u) {
        if (heapWordOwner(*heapMetadataWord(payload)) == 0u) {
            u32 bytes = heapNextPayloadWord(payload) - (u32)(uintptr_t)payload - (u32)sizeof(HeapBlock);
            if (!largest) {
                result += bytes;
            } else if (bytes > result) {
                result = bytes;
            }
        }
        payload = (void *)(uintptr_t)heapNextPayloadWord(payload);
    }
    return largest ? result - (u32)sizeof(HeapBlock) : result;
}

u_int HeapGetTotalFreeSize(void)
{
    return heapFreeSpace(0);
}

u_int HeapGetLargestFreeBlockSize(void)
{
    return heapFreeSpace(1);
}
