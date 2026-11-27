s32 SoundSpuMemoryGetFreeBlock(void)
{
    const volatile SoundSpuMemoryBlock* blocks = g_SoundSpuMemoryBlocks;

    for (int slot = 0; slot < MAX_SPU_MEMORY_BLOCKS; ++slot) {
        if (blocks[slot].flags == SPU_MEMORY_FREE) {
            return slot;
        }
    }

    /* The sound manager also returns slot zero when every descriptor is busy. */
    return 0;
}
