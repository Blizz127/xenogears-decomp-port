void SoundSetSpuIrqCallback(u32 token)
{
    SpuIRQCallbackProc volatile *const slot = &g_SoundSpuIrqCallbackFn;

    *slot = token ? (SpuIRQCallbackProc)(uintptr_t)token : NULL;
}

void SoundSpuIRQHandler(void)
{
    volatile u16 *const flags = (volatile u16 *)&g_SoundControlFlags;
    volatile u32 *const count = (volatile u32 *)&g_SoundSpuIRQCount;
    const u16 previous = *flags;
    SpuIRQCallbackProc callback = *(SpuIRQCallbackProc volatile *)&g_SoundSpuIrqCallbackFn;

    *flags = (u16)(previous | 4u);
    *count = *count + 1u;
    if (callback) {
        callback();
    }
    *flags = (u16)(*flags & 0xfffbu);
}

int SoundSpuMemoryFreeBlock(int key)
{
    volatile SoundSpuMemoryBlock *previous = NULL;
    volatile SoundSpuMemoryBlock *current = &g_SoundSpuMemoryBlocks[0];
    unsigned int inspected;

    for (inspected = 0; inspected < MAX_SPU_MEMORY_BLOCKS; ++inspected) {
        const s32 address = current->spuAddress;
        const s16 next = current->nextBlockIndex;

        if (address == key) {
            if (previous) {
                previous->nextBlockIndex = next;
            } else {
                *(volatile s16 *)PSX_ADDR(2) = next;
            }
            current->flags = 0;
            current->unk1 = 0;
            current->spuAddress = 0;
            current->nextBlockIndex = 0;
            return key;
        }
        if (next <= 0 || next >= MAX_SPU_MEMORY_BLOCKS) {
            return 0;
        }
        previous = current;
        current = &g_SoundSpuMemoryBlocks[next];
    }
    return 0;
}
