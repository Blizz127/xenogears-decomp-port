static volatile HeapHeaderWord *heapAllocationLink(const void *payload)
{
    return (volatile HeapHeaderWord *)((uintptr_t)payload - sizeof(HeapBlock));
}

enum { HEAP_SOURCE_MASK = 0x001fffff, HEAP_OWNER_MASK = 0x01e00000, HEAP_PIN_MASK = 0x02000000 };
#define HEAP_CONTENT_MASK 0xfc000000u

static u32 heapReadAllocationContent(void)
{
    u32 tag = *(const volatile u16 *)&g_HeapCurContentType;
    *(volatile u16 *)&g_HeapCurContentType = HEAP_CONTENT_NONE;
    return tag << 26;
}

static void heapWriteAllocatedHeader(void *payload, u32 word, u32 content)
{
    u32 owner = (u32)(*(const volatile u16 *)&g_HeapCurUser & 15u) << 21;
    volatile HeapHeaderWord *header = heapMetadataWord(payload);
    word = (word & ~HEAP_OWNER_MASK) | owner;
    *header = word;
    word = (word & ~(HEAP_CONTENT_MASK | HEAP_PIN_MASK)) | content;
    *header = word;
    *header = (word & ~HEAP_SOURCE_MASK) | 0x001ffffeu;
}

static void *heapAllocateWhole(void *payload)
{
    u32 word = *heapMetadataWord(payload);
    u32 content = heapReadAllocationContent();
    heapWriteAllocatedHeader(payload, word, content);
    return payload;
}

static void *heapAllocateAtBack(void *start, u32 size)
{
    u32 content = *(const volatile u16 *)&g_HeapCurContentType;
    u32 end = *heapAllocationLink(start);
    *(volatile u16 *)&g_HeapCurContentType = HEAP_CONTENT_NONE;
    u32 address = end - size - (u32)sizeof(HeapBlock);
    void *result = (void *)(uintptr_t)address;
    u32 word = *heapMetadataWord(result);
    *heapAllocationLink(result) = end;
    heapWriteAllocatedHeader(result, word, content << 26);
    *heapAllocationLink(start) = address;
    return result;
}

static void *heapAllocateAtFront(void *start, u32 size)
{
    u32 address = (u32)(uintptr_t)start + size + (u32)sizeof(HeapBlock);
    void *rest = (void *)(uintptr_t)address;
    volatile HeapHeaderWord *newWord = heapMetadataWord(rest);
    volatile HeapHeaderWord *oldWord = heapMetadataWord(start);
    u32 copy = *newWord;
    u32 content = heapReadAllocationContent();
    u32 next = *heapAllocationLink(start);
    *heapAllocationLink(rest) = next;

    const u32 masks[] = { HEAP_OWNER_MASK, HEAP_CONTENT_MASK, HEAP_PIN_MASK, HEAP_SOURCE_MASK };
    for (unsigned i = 0; i < sizeof masks / sizeof masks[0]; ++i) {
        copy = (copy & ~masks[i]) | (*oldWord & masks[i]);
        *newWord = copy;
    }
    u32 word = *oldWord;
    *heapAllocationLink(start) = address;
    *oldWord = (word & ~HEAP_CONTENT_MASK) | content;
    word = *oldWord;
    u32 owner = (u32)(*(const volatile u16 *)&g_HeapCurUser & 15u) << 21;
    word = (word & ~(HEAP_OWNER_MASK | HEAP_PIN_MASK)) | owner;
    *oldWord = word;
    *oldWord = (word & ~HEAP_SOURCE_MASK) | 0x001ffffeu;
    return start;
}

void *HeapAlloc(u_int bytes, u_int flags)
{
    u32 consolidate = *(const volatile u32 *)&g_HeapNeedsConsolidation;
    *(volatile u32 *)&g_HeapLastAllocSrcAddr = 0u - 8u;
    if (consolidate) HeapConsolidate();
    *(volatile u32 *)&g_HeapLastAllocSize = bytes;
    void *current = *(void *volatile *)&g_Heap;
    u32 size = (bytes + 3u) & ~3u;
    void *choice = NULL;
    u32 best = 0x800000u;
    int takeWhole = 0;
    for (;;) {
        if (heapWordOwner(*heapMetadataWord(current)) == 0u) {
            u32 capacity = *heapAllocationLink(current) - (u32)(uintptr_t)current - (u32)sizeof(HeapBlock);
            u32 remaining = capacity - size;
            int whole = remaining == 0u || remaining == 4u;
            if (whole && flags != 1u) return heapAllocateWhole(current);
            if (whole || (int32_t)remaining > 4) {
                if (flags != 1u && flags != 2u) return heapAllocateAtFront(current, size);
                if (flags == 1u || capacity < best) {
                    choice = current;
                    takeWhole = whole;
                    best = capacity;
                }
            }
        } else if (heapWordOwner(*heapMetadataWord(current)) == 1u) {
            break;
        }
        current = (void *)(uintptr_t)*heapAllocationLink(current);
    }
    if (choice) {
        if (takeWhole) return heapAllocateWhole(choice);
        return flags == 1u ? heapAllocateAtBack(choice, size) : heapAllocateAtFront(choice, size);
    }
    if (*(const volatile u32 *)&g_HeapIsErrorHandlerOff == 0u) MainLoop(ERR_HEAP_OUT_OF_MEMORY);
    return NULL;
}
#undef HEAP_CONTENT_MASK
