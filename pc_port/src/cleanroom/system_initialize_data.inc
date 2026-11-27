void SystemInitializeData(void *file) {
    if (file == NULL) {
        HeapSetCurrentContentType(HEAP_CONTENT_NONE);
        return;
    }
    HeapPinBlock((HeapBlock *)file);
    *(void *volatile *)&g_SystemDataFile = file;
    void *volatile *entries = (void *volatile *)&g_SystemDataEntries;
    *entries = file;
    ResolveArchiveEntryPointers((u32 *)file);
    *entries = (void *)((uintptr_t)*entries + 4u);
}
