static volatile HeapHeaderWord *heapLinkForPayload(const void *payload)
{
    return (volatile HeapHeaderWord *)((uintptr_t)payload - sizeof(HeapBlock));
}

static void heapFreeList(int force, int matchOwner, unsigned target)
{
    void *current = *(void *volatile *)&g_Heap;
    while (heapWordOwner(*heapMetadataWord(current)) != 1u) {
        int selected = !matchOwner || heapWordOwner(*heapMetadataWord(current)) == target;
        void *next = (void *)(uintptr_t)*heapLinkForPayload(current);
        if (selected) {
            if (force) {
                HeapUnpinBlock(current);
            }
            HeapFree(current);
        }
        current = next;
    }
}

void HeapFreeAllBlocks(void)
{
    heapFreeList(0, 0, 0);
}

void HeapForceFreeAllBlocks(void)
{
    heapFreeList(1, 0, 0);
}

void HeapFreeBlocksWithFlag(u_char target)
{
    heapFreeList(0, 1, target);
}

void HeapConsolidate(void)
{
    void *current = *(void *volatile *)&g_Heap;
    while (heapWordOwner(*heapMetadataWord(current)) != 1u) {
        int freeBlock = heapWordOwner(*heapMetadataWord(current)) == 0u;
        volatile HeapHeaderWord *currentLink = heapLinkForPayload(current);
        void *next = (void *)(uintptr_t)*currentLink;
        if (freeBlock && heapWordOwner(*heapMetadataWord(next)) == 0u) {
            do {
                u32 following = *heapLinkForPayload(next);
                *currentLink = following;
                next = (void *)(uintptr_t)following;
            } while (heapWordOwner(*heapMetadataWord(next)) == 0u);
            next = (void *)(uintptr_t)*currentLink;
        }
        current = next;
    }
    *(volatile u32 *)&g_HeapNeedsConsolidation = 0;
}

void HeapTickDelayedFree(void)
{
    volatile HeapDelayedFreeBlock *previous = &g_HeapDelayedFreeBlocksHead;
    volatile HeapDelayedFreeBlock *current = previous->pNext;
    while (current) {
        u32 delay = current->delay;
        current->delay = delay - 1u;
        if (delay == 0u) {
            HeapFree(current->pMem);
            previous->pNext = current->pNext;
            HeapFree((void *)current);
            if (!previous->pNext) {
                return;
            }
        } else {
            previous = previous->pNext;
        }
        current = previous->pNext;
    }
}

void HeapFreeAllDelayedBlocks(void)
{
    volatile HeapDelayedFreeBlock *head = &g_HeapDelayedFreeBlocksHead;
    volatile HeapDelayedFreeBlock *current = head->pNext;
    while (current) {
        HeapFree(current->pMem);
        head->pNext = current->pNext;
        HeapFree((void *)current);
        current = head->pNext;
    }
}
