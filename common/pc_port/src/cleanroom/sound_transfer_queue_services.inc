extern void SoundHeapFree(void *);
extern void SoundQueueSpuWriteCommand(u32, void *, u_long, SoundCommandCallback_t);
extern int SoundTransferQueueSync(void);
extern void SoundHandleError(s32);
extern void PsyX_Sys_SoundGateEnterCritical(void);
extern void PsyX_Sys_SoundGateExitCritical(void);

void SoundProcessTransferCommand(void);

/* These globals can be changed by transfer callbacks. Keep the individual
 * accesses at the boundaries where the queue protocol observes them. */
#define SOUND_QUEUE_STATE(cell) (*(volatile __typeof__(cell) *)&(cell))

static u16 SoundNextTransferSlot(u16 index)
{
    u16 next = (u16)(index + 1);
    return next < SOUND_TRANSFER_QUEUE_SIZE ? next : 0;
}

void SoundExecuteReverbWorkAreaTransfer(void)
{
    s32 remaining = SOUND_QUEUE_STATE(g_SoundUploadBytesRemaining);
    if (remaining == 0) {
        s32 destination = SOUND_QUEUE_STATE(g_SoundUploadDestBuffer);
#ifdef _WIN32
        SoundHeapFree((void *)(intptr_t)destination);
#else
        SoundHeapFree(SOUND_PSX_TO_PTR(void, (u32)destination));
#endif
        short left = SOUND_QUEUE_STATE(g_SoundReverbDepth.left);
        short right = SOUND_QUEUE_STATE(g_SoundReverbDepth.right);
        SOUND_QUEUE_STATE(g_SoundUploadDestBuffer) = 0;
        SpuSetReverbModeDepth(left, right);
        SpuSetReverbModeDelayTime(SOUND_QUEUE_STATE(g_SoundReverbDelay));
        SpuSetReverbModeFeedback(SOUND_QUEUE_STATE(g_SoundReverbFeedback));
        SOUND_QUEUE_STATE(g_SoundControlFlags) &= ~0x20;
        return;
    }

    s32 chunk = remaining > 0x840 ? 0x800 : remaining;
    void *buffer = (void *)(intptr_t)SOUND_QUEUE_STATE(g_SoundUploadDestBuffer);
    u32 address = (u32)SOUND_QUEUE_STATE(g_SoundUploadSourceAddr);
    SOUND_QUEUE_STATE(g_SoundUploadBytesRemaining) = (s32)((u32)remaining - (u32)chunk);
    SOUND_QUEUE_STATE(g_SoundUploadSourceAddr) = (s32)(address + (u32)chunk);
    SoundQueueSpuWriteCommand(address, buffer, (u_long)chunk,
                              SoundExecuteReverbWorkAreaTransfer);
    if (!(SOUND_QUEUE_STATE(g_SoundControlFlags) & 0x10) && SOUND_QUEUE_STATE(g_SoundUploadDestBuffer) != 0)
        SoundQueueSpuWriteCommand(address, (void *)(intptr_t)SOUND_QUEUE_STATE(g_SoundUploadDestBuffer),
                                  (u_long)chunk, 0);
}

void SoundOnTransferCallback(void)
{
    u16 index = SOUND_QUEUE_STATE(g_SoundTransferQueueReadIndex);
    volatile SoundTransferCommand *record = &SOUND_QUEUE_STATE(g_SoundTransferQueue)[index];
    SoundCommandCallback_t notify = SoundTransferCallbackResolve(record->callbackToken);
    SOUND_QUEUE_STATE(g_SoundControlFlags) |= SOUND_CTL_FLAG_IRQ_HANDLER;
    if (notify)
        notify();

    short flags = SOUND_QUEUE_STATE(g_SoundControlFlags);
    u16 readIndex = SOUND_QUEUE_STATE(g_SoundTransferQueueReadIndex);
    u16 writeIndex = SOUND_QUEUE_STATE(g_SoundTransferQueueWriteIndex);
    SOUND_QUEUE_STATE(g_SoundControlFlags) = flags & ~0x10;
    if (readIndex != writeIndex)
        SoundProcessTransferCommand();
    SOUND_QUEUE_STATE(g_SoundControlFlags) &= ~SOUND_CTL_FLAG_IRQ_HANDLER;
}

void SoundQueueTransferCommand(u32 transferAddress, void *pData, u_long dataSize,
                               SoundCommandCallback_t callback, unsigned short commandType)
{
    int takeGate = !(SOUND_QUEUE_STATE(g_SoundControlFlags) & SOUND_CTL_FLAG_IRQ_HANDLER);
    if (takeGate) {
        while (SoundTransferQueueSync() != 0) {}
        EnterCriticalSection();
        PsyX_Sys_SoundGateEnterCritical();
    }

    u16 index = SoundNextTransferSlot(SOUND_QUEUE_STATE(g_SoundTransferQueueWriteIndex));
    SOUND_QUEUE_STATE(g_SoundTransferQueueWriteIndex) = index;
    volatile SoundTransferCommand *record = &SOUND_QUEUE_STATE(g_SoundTransferQueue)[index];
    record->commandType = commandType & 15;
    record->unk2 = 0;
    record->pSpuData = SOUND_PTR_TO_PSX(pData);
    record->pTransferAddress = transferAddress & 0x7fff8u;
    record->dataSize = (u32)dataSize;
    record->callbackToken = SoundTransferCallbackStore(index, callback);

    if (!(SOUND_QUEUE_STATE(g_SoundControlFlags) & 0x10))
        SoundProcessTransferCommand();
    if (takeGate) {
        PsyX_Sys_SoundGateExitCritical();
        ExitCriticalSection();
    }
}

void SoundProcessTransferCommand(void)
{
    u16 index = SoundNextTransferSlot(SOUND_QUEUE_STATE(g_SoundTransferQueueReadIndex));
    short flags = SOUND_QUEUE_STATE(g_SoundControlFlags);
    SOUND_QUEUE_STATE(g_SoundTransferQueueReadIndex) = index;
    SOUND_QUEUE_STATE(g_SoundControlFlags) = flags | 0x10;
    volatile SoundTransferCommand *record = &SOUND_QUEUE_STATE(g_SoundTransferQueue)[index];
    SpuTransferCallbackProc previous = SpuSetTransferCallback(SoundOnTransferCallback);
    SpuSetTransferMode(0);
    SpuSetTransferStartAddr(record->pTransferAddress);

    unsigned short kind = record->commandType;
    switch (kind) {
    case SOUND_SPU_COMMAND_WRITE:
    case SOUND_SPU_COMMAND_READ: {
        unsigned char *data = SOUND_PSX_TO_PTR(unsigned char, record->pSpuData);
        u32 size = record->dataSize;
        if (kind == SOUND_SPU_COMMAND_WRITE)
            SpuWrite(data, size);
        else
            SpuRead(data, size);
        break;
    }
    case 3:
    case 4:
        SOUND_QUEUE_STATE(D_80059548) = (s16)SpuReadDecodedData(SOUND_PSX_TO_PTR(SpuDecodedData, record->pSpuData),
                                              kind == 3 ? SPU_ALL : SPU_CDONLY);
        break;
    }
    if (previous != SoundOnTransferCallback)
        SoundHandleError(SOUND_ERR_UNEXPECTED_CALLBACK);
}

#undef SOUND_QUEUE_STATE
