void SoundSpuMemoryInitialize(void)
{
    volatile SoundSpuMemoryBlock *blocks = g_SoundSpuMemoryBlocks;
    int count = MAX_SPU_MEMORY_BLOCKS;

    while (count > 0) {
        --count;
        blocks[count].flags = 0;
    }

    blocks[0].flags = 0x81;
    blocks[0].unk1 = 5;
    blocks[0].spuAddress = 0;
    blocks[0].size = 0x1010;
    blocks[0].nextBlockIndex = 0;
}
