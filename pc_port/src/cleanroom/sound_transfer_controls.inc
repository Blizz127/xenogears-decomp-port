void SoundQueueSpuWriteCommand(u32 transferAddress, void *pData,
                               u_long dataSize, SoundCommandCallback_t pCallback)
{
    SoundQueueTransferCommand(transferAddress, pData, dataSize, pCallback, 1);
}

void SoundWdsSetTransferParamters(int transferAddress, int numBytesToTransfer)
{
    *(volatile int *)&g_SoundWdsCurSpuAddress = transferAddress;
    *(volatile int *)&g_SoundWdsRemainingBytes = numBytesToTransfer;
}

int SoundTransferQueueSync(void)
{
    u16 writeIndex = *(volatile u16 *)&g_SoundTransferQueueWriteIndex;
    u16 readIndex = *(volatile u16 *)&g_SoundTransferQueueReadIndex;
    if (writeIndex < readIndex) {
        writeIndex = (u16)(writeIndex + 8);
    }
    return (int)writeIndex - (int)readIndex >= 6;
}
