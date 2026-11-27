u32 SoundSpuMemoryAllocateBlockAtAddress(s32 size, s32 address, s32 option)
{
    (void)option;
    SoundSpuMemoryBlock *previous = g_SoundSpuMemoryBlocks;
    s32 end = (s32)((u32)address + (u32)size);
    for (;;) {
        s32 firstFree = (s32)((u32)previous->spuAddress + (u32)previous->size);
        if (address <= previous->spuAddress) {
            if (size > 0 || address < firstFree)
                return 0;
            previous = g_SoundSpuMemoryBlocks;
            break;
        }

        s16 next = previous->nextBlockIndex;
        s32 limit = next ? g_SoundSpuMemoryBlocks[next].spuAddress : 0x80000;
        if (!next || end <= limit) {
            s32 capacity = (s32)((u32)limit - (u32)firstFree);
            if (address < firstFree || size > capacity)
                return 0;
            break;
        }
        previous = &g_SoundSpuMemoryBlocks[next];
    }

    s32 slot = SoundSpuMemoryGetFreeBlock();
    SoundSpuMemoryBlock *block = &g_SoundSpuMemoryBlocks[slot];
    block->flags = SPU_MEMORY_IN_USE;
    block->unk1 = 0;
    block->spuAddress = address;
    block->size = size;
    block->nextBlockIndex = previous->nextBlockIndex;
    previous->nextBlockIndex = (s16)slot;
    return (u32)address;
}
