static void heapReportNullRelease(void)
{
    *(volatile u32 *)&g_HeapLastAllocSize = 0;
    *(volatile u32 *)&g_HeapLastAllocSrcAddr = 0u - 8u;
    MainLoop(0x83);
}

u_int HeapFree(void *payload)
{
    if (!payload) {
        if (*(const volatile u32 *)&g_HeapIsErrorHandlerOff == 0u) {
            heapReportNullRelease();
        }
        return 1u;
    }
    volatile HeapHeaderWord *metadata = heapMetadataWord(payload);
    if (*metadata & (1u << 25)) {
        return UINT32_MAX;
    }
    *metadata = 0x84000000u;
    *(volatile u32 *)&g_HeapNeedsConsolidation = 1u;
    return 0u;
}

void HeapDelayedFree(void *payload, u_int delay)
{
    if (!payload) {
        heapReportNullRelease();
        return;
    }
    if (delay == 0u) {
        HeapFree(payload);
        return;
    }

    HeapSetCurrentContentType(0x33);
    volatile HeapDelayedFreeBlock *node = HeapAlloc(sizeof(HeapDelayedFreeBlock), 1);
    if (node) {
        volatile HeapDelayedFreeBlock *head = &g_HeapDelayedFreeBlocksHead;
        HeapDelayedFreeBlock *previous = head->pNext;
        node->pMem = payload;
        node->delay = delay;
        node->pNext = previous;
        head->pNext = (HeapDelayedFreeBlock *)node;
    }
}
