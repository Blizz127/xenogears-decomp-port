void HeapDebugDump(u_int mode, u_int skipGroups, int requestedGroups, u_int flags)
{
    if (flags == 0u) {
        flags = HEAP_DEBUG_PRINT_TOTAL_FREE_SIZE | HEAP_DEBUG_PRINT_CONTENTS |
                HEAP_DEBUG_PRINT_SIZE | HEAP_DEBUG_PRINT_ADDRESS | HEAP_DEBUG_PRINT_NO;
    }
    if (mode != 0u) {
        HeapConsolidate();
    }
    if (!*(u8 *volatile *)&g_SymbolData) {
        flags &= ~HEAP_DEBUG_PRINT_FUNCTION;
    }
    char *const headings[] = {
        D_80059268, D_80059270, D_80059278, D_80059280,
        D_80059288, D_80059290, (char *)D_80018A58, (char *)D_80018A64
    };
    for (unsigned column = 0; column < sizeof(headings) / sizeof(headings[0]); ++column) {
        if (flags & (1u << column)) {
            HeapPrintf(headings[column]);
        }
    }
    HeapPrintf(D_80059264);

    void *current = *(void *volatile *)&g_Heap;
    void *start = current;
    u32 bytes = 0u;
    u32 index = 0u;
    u32 remaining = (u32)requestedGroups;
    while (heapWordOwner(*heapMetadataWord(current)) != HEAP_USER_END) {
        u32 next = heapNextPayloadWord(current);
        bytes += next - (u32)(uintptr_t)current - (u32)sizeof(HeapBlock);
        if (mode == 2u || mode == 3u) {
            if (mode == 3u) {
                next = heapNextPayloadWord(current);
            }
            void *neighbor = (void *)(uintptr_t)next;
            u32 block = *heapMetadataWord(current);
            u32 nextBlock = *heapMetadataWord(neighbor);
            const u32 groupBits = mode == 3u ? 0x1fffffu : ((15u << 21) | (63u << 26));
            if (((block ^ nextBlock) & groupBits) == 0u) {
                current = neighbor;
                ++index;
                continue;
            }
        }
        if (skipGroups) {
            --skipGroups;
        } else {
            if (flags & HEAP_DEBUG_PRINT_NO) {
                HeapPrintf(D_80059298, index);
            }
            HeapBlock *header = (HeapBlock *)((uintptr_t)current - sizeof(HeapBlock));
            HeapDebugDumpBlock(header, start, bytes, (int)flags);
            if (--remaining == 0u && requestedGroups != 0) {
                break;
            }
        }
        ++index;
        current = (void *)(uintptr_t)heapNextPayloadWord(current);
        start = current;
        bytes = 0u;
    }

    char *const footers[] = {D_800592A0, D_800592A8, D_800592A8, D_800592A8, D_800592B0};
    for (unsigned column = 0; column < sizeof(footers) / sizeof(footers[0]); ++column) {
        if (flags & (1u << column)) {
            HeapPrintf(footers[column]);
        }
    }
    if (flags & HEAP_DEBUG_PRINT_TOTAL_FREE_SIZE) {
        u32 freeBytes = HeapGetTotalFreeSize();
        HeapPrintf((char *)D_80018A70, freeBytes);
    }
    HeapPrintf(D_80059264);
}
