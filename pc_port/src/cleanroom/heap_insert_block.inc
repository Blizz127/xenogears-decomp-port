void *HeapInsertAlloc(HeapBlock *payload, u_int size)
{
    u32 address = (u32)(uintptr_t)payload;
    volatile HeapHeaderWord *header = (volatile HeapHeaderWord *)((uintptr_t)payload - sizeof(HeapBlock));
    u32 next = header[0];
    u32 room = next - address - (u32)sizeof(HeapBlock);
    if (size + 2u * (u32)sizeof(HeapBlock) >= room) {
        return NULL;
    }

    u32 splitAddress = address + size;
    volatile HeapHeaderWord *split = (volatile HeapHeaderWord *)(uintptr_t)splitAddress;
    (void)split[1];
    *(volatile u32 *)&g_HeapNeedsConsolidation = 1;
    split[0] = next;
    split[1] = 0x84000000u;
    header[0] = splitAddress + (u32)sizeof(HeapBlock);
    return (void *)header;
}
