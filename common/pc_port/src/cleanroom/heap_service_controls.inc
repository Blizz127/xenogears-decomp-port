extern void func_8003747C(void *);

static uintptr_t heapAlignedAddress(const void *address)
{
    return (uintptr_t)address & ~(uintptr_t)3u;
}

static void heapClearSymbols(void)
{
    *(u8 *volatile *)&g_SymbolData = NULL;
    *(void *volatile *)&g_SymbolDataEndAddress = NULL;
}

void HeapReset(void)
{
    func_8003747C(NULL);
    *(volatile u16 *)&g_HeapCurContentType = 32;
    *(volatile u16 *)&g_HeapCurUser = 10;
    heapClearSymbols();
}

void HeapInit(void *start, void *end)
{
    uintptr_t low = heapAlignedAddress(start);
    uintptr_t high = heapAlignedAddress(end);
    volatile HeapHeaderWord *firstLink = (volatile HeapHeaderWord *)low;
    volatile HeapHeaderWord *firstMeta = (volatile HeapHeaderWord *)(low + 4u);
    volatile HeapHeaderWord *lastLink = (volatile HeapHeaderWord *)(high - 8u);
    volatile HeapHeaderWord *lastMeta = (volatile HeapHeaderWord *)(high - 4u);
    u32 preserved = *firstMeta & 0x021fffffu;

    *(void *volatile *)&g_Heap = (void *)(low + 8u);
    *(volatile u16 *)&g_HeapCurContentType = 32;
    *firstLink = (u32)high;
    *(volatile u32 *)&g_HeapNeedsConsolidation = 0;
    heapClearSymbols();
    *firstMeta = preserved | 0x84000000u;

    preserved = *lastMeta & 0x021fffffu;
    *(volatile u16 *)&g_HeapCurUser = 10;
    *lastLink = (u32)high;
    *lastMeta = preserved | 0x80200000u;
    *(HeapDelayedFreeBlock *volatile *)&g_HeapDelayedFreeBlocksHead.pNext = NULL;
    HeapReset();
}

void HeapRelocate(void *start)
{
    HeapFreeAllBlocks();
    if (*(const volatile u32 *)&g_HeapNeedsConsolidation) {
        HeapConsolidate();
    }

    uintptr_t low = heapAlignedAddress(start);
    volatile HeapHeaderWord *firstMeta = (volatile HeapHeaderWord *)(low + 4u);
    u32 preserved = *firstMeta & 0x021fffffu;
    void *previous = *(void *volatile *)&g_Heap;
    *(void *volatile *)&g_Heap = (void *)(low + 8u);
    u32 next = *(const volatile HeapHeaderWord *)((uintptr_t)previous - 8u);
    *firstMeta = preserved | 0x84000000u;
    *(volatile HeapHeaderWord *)low = next;
    *(HeapDelayedFreeBlock *volatile *)&g_HeapDelayedFreeBlocksHead.pNext = NULL;
}

void *HeapAllocSound(u_int bytes)
{
    volatile u16 *user = &g_HeapCurUser;
    u16 saved = *user;
    *user = 7;
    *(volatile u16 *)&g_HeapCurContentType = 47;
    void *block = HeapAlloc(bytes, 1);
    HeapPinBlock(block);
    *user = saved;
    return block;
}

void HeapCalloc(u_int count, u_int size)
{
    volatile u16 *user = &g_HeapCurUser;
    u16 saved = *user;
    *user = 7;
    *(volatile u16 *)&g_HeapCurContentType = 35;
    HeapAlloc(count * size, 0);
    *user = saved;
}

void HeapForceFree(void *block)
{
    HeapUnpinBlock(block);
    HeapFree(block);
}
