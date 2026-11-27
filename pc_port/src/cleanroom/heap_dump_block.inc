void HeapDebugDumpBlock(HeapBlock *header, void *payload, u_int size, int flags)
{
    const volatile HeapHeaderWord *metadata =
        (const volatile HeapHeaderWord *)((uintptr_t)header + sizeof(HeapHeaderWord));
    if (flags & HEAP_DEBUG_PRINT_MCB) {
        HeapPrintf(D_80059248, (u32)(uintptr_t)header & 0xffffffu);
    }
    if (flags & HEAP_DEBUG_PRINT_ADDRESS) {
        HeapPrintf(D_80059248, (u32)(uintptr_t)payload & 0xffffffu);
    }
    if (flags & HEAP_DEBUG_PRINT_SIZE) {
        HeapPrintf(D_80059250, size);
    }
    if (flags & HEAP_DEBUG_PRINT_USER) {
        u32 owner = heapWordOwner(*metadata);
        void *volatile *names = D_80050110;
        char *name = owner < 12u ? names[owner] : NULL;
        HeapPrintf(D_80059258, name ? name : "");
    }
    if (flags & HEAP_DEBUG_PRINT_GETADD) {
        HeapPrintf(D_80059248, (*metadata & 0x1fffffu) * 4u);
    }
    if (flags & HEAP_DEBUG_PRINT_FUNCTION) {
        u32 block = *metadata;
        if (heapWordOwner(block) != 0u) {
            u_char symbol[64];
            HeapGetSymbolNameFromAddress(0x80000000u + (block & 0x1fffffu) * 4u, symbol);
            HeapPrintf(D_8005925C, (char *)symbol);
            if ((flags & HEAP_DEBUG_PRINT_CONTENTS) && ((*metadata >> 26) & 31u)) {
                HeapPrintf(D_80059260);
            }
        }
    }
    if (flags & HEAP_DEBUG_PRINT_CONTENTS) {
        u32 block = *metadata;
        u32 content = block >> 26;
        u32 type = content & 31u;
        if (type != 0u) {
            char *label = NULL;
            if (content & HEAP_DEFAULT_CONTENT_TYPES) {
                void *volatile *labels = g_HeapContentTypeNames;
                if (type < 20u) {
                    label = labels[type];
                }
            } else {
                u32 owner = heapWordOwner(block);
                if (owner < 12u) {
                    __typeof__(g_HeapUserContentNames[0]) volatile *rows = g_HeapUserContentNames;
                    char *const volatile *row = rows[owner];
                    if (row) {
                        label = row[type];
                    }
                }
            }
            HeapPrintf(D_8005925C, label ? label : "");
        }
    }
    HeapPrintf(D_80059264);
}
