void HeapUnpinBlock(HeapBlock *payload)
{
    uintptr_t address = (uintptr_t)payload;
    if (address != 0u && address < UINT64_C(0x800000000000)) {
        volatile HeapHeaderWord *metadata = heapMetadataWord(payload);
        *metadata &= ~(UINT32_C(1) << 25);
    }
}
