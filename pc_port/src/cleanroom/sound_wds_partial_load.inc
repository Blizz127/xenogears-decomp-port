/* SPDX-License-Identifier: MIT */
void *func_800380D0(void *input, s32 available, s32 placement)
{
    SoundWDSEntry *source = input;
    ++g_PcPortDiagPartialWdsLoads;
    if (!source)
        return NULL;
    if (SoundFindWdsEntry(source->id)) {
        SoundHandleError(22);
        return NULL;
    }

    const u32 address = SoundSpuMemoryAllocateWDS(source, placement);
    if (!address) {
        SoundHandleError(31);
        return NULL;
    }
    SoundWdsSetTransferParamters((s32)address, (s32)source->adpcmDataSize);
    const intptr_t offset = (s32)source->adpcmDataOffset;
    u8 *samples = (u8 *)((uintptr_t)source + (uintptr_t)offset);
    const s32 bytes = (s32)((u32)available - source->headerSizeMby);
    SoundTransferWdsPart(samples, bytes);

    SoundWDSEntry *record = func_80039024((s32)source->headerSizeMby);
    if (!record) {
        SoundSpuMemoryFreeBlock((s32)address);
        SoundHandleError(30);
        return NULL;
    }
    SoundHeapSetBlockMemory(record, source, (s32)source->headerSizeMby);

    DisableEvent(g_unk_SoundEvent);
    record->spuMemoryAddress = (s32)address;
    SoundWDSEntry *tail = g_SoundWdsLinkedList;
    if (tail) {
        while (tail->pNext)
            tail = SOUND_PSX_TO_PTR(SoundWDSEntry, tail->pNext);
        tail->pNext = SOUND_PTR_TO_PSX(record);
    } else {
        g_SoundWdsLinkedList = record;
    }
    EnableEvent(g_unk_SoundEvent);
    record->pNext = 0;
    return record;
}
