/* SPDX-License-Identifier: MIT */
u32 SoundSpuMemoryAllocateBlock(s32 size, s32 unused)
{
    (void)unused;
    unsigned previous = 0;
    for (unsigned visited = 0; visited < MAX_SPU_MEMORY_BLOCKS; ++visited) {
        SoundSpuMemoryBlock *prior = &g_SoundSpuMemoryBlocks[previous];
        const uint32_t address = (uint32_t)prior->spuAddress + (uint32_t)prior->size;
        const int next = prior->nextBlockIndex;
        if (next < 0 || next >= MAX_SPU_MEMORY_BLOCKS)
            return 0;
        uint32_t stop = next == 0 ? 0x80000u : (uint32_t)g_SoundSpuMemoryBlocks[next].spuAddress;
        if ((int32_t)(stop - address) >= size) {
            unsigned index = 0;
            while (index < MAX_SPU_MEMORY_BLOCKS && g_SoundSpuMemoryBlocks[index].flags != 0)
                ++index;
            if (index == MAX_SPU_MEMORY_BLOCKS)
                index = 0;
            SoundSpuMemoryBlock *entry = &g_SoundSpuMemoryBlocks[index];
            entry->flags = SPU_MEMORY_IN_USE;
            entry->unk1 = 0;
            entry->spuAddress = (int32_t)address;
            entry->size = size;
            entry->nextBlockIndex = (short)next;
            prior->nextBlockIndex = (short)index;
            return address;
        }
        if (next == 0)
            break;
        previous = (unsigned)next;
    }
    return 0;
}
