u32 SoundSpuMemoryAllocateWDS(SoundWDSEntry *, int);
void SoundQueueSpuWriteCommand(u32, void *, u_long, SoundCommandCallback_t);
void *func_80039024(s32);
void SoundHeapSetBlockMemory(void *, void *, int);

static SoundWDSEntry *soundPublishBank(SoundWDSEntry *source, u32 address)
{
    SoundWDSEntry *record = func_80039024((s32)source->headerSizeMby);
    if (record == NULL) {
        SoundSpuMemoryFreeBlock((s32)address);
        SoundHandleError(30);
        return NULL;
    }

    SoundHeapSetBlockMemory(record, source, (s32)source->headerSizeMby);
    record->spuMemoryAddress = (s32)address;

    DisableEvent(g_unk_SoundEvent);
    SoundWDSEntry *tail = g_SoundWdsLinkedList;
    if (tail) {
        while (tail->pNext)
            tail = SOUND_PSX_TO_PTR(SoundWDSEntry, tail->pNext);
        tail->pNext = SOUND_PTR_TO_PSX(record);
    } else {
        g_SoundWdsLinkedList = record;
    }
    record->pNext = 0;
    EnableEvent(g_unk_SoundEvent);
    return record;
}

SoundWDSEntry *SoundLoadWdsFile(SoundWDSEntry *source, s32 mode)
{
    u32 address = SoundSpuMemoryAllocateWDS(source, mode);
    if (address == 0) {
        SoundHandleError(31);
        return NULL;
    }

    SoundQueueSpuWriteCommand(address, (u8 *)source + source->adpcmDataOffset,
                              source->adpcmDataSize, NULL);
    return soundPublishBank(source, address);
}

SoundWDSEntry *SoundLoadWdsFileHostStaged(SoundWDSEntry *source)
{
    u32 address = SoundSpuMemoryAllocateWDS(source, 0);
    if (address == 0) {
        SoundHandleError(31);
        return NULL;
    }

    SpuSetTransferStartAddr(address);
    SpuWrite((u8 *)source + source->adpcmDataOffset, source->adpcmDataSize);
    return soundPublishBank(source, address);
}
