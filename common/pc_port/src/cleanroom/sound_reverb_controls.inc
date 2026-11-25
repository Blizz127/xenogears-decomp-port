void SoundInitiateReverbWorkAreaTransfer(s32 address, s32 size)
{
    s32 cachedBuffer = *(volatile s32 *)&g_SoundUploadDestBuffer;
    *(volatile s32 *)&g_SoundUploadSourceAddr = address;
    *(volatile s32 *)&g_SoundUploadBytesRemaining = size;

    if (cachedBuffer == 0) {
        void *buffer = SoundHeapAllocate(2112u);
        SoundPsxAddress bufferAddress = SOUND_PTR_TO_PSX(buffer);
        *(volatile s32 *)&g_SoundUploadDestBuffer = (s32)bufferAddress;
        if (bufferAddress == 0) {
            SoundHandleError(30);
        }
    }

    *(volatile short *)&g_SoundControlFlags |= 0x20;
    SoundExecuteReverbWorkAreaTransfer();
}

void SoundSetReverbModeWithAllocation(s32 mode, s32 depth, s32 delay, s32 feedback)
{
    if (mode == -2) {
        return;
    }
    if (mode == 0) {
        depth = delay = feedback = 0;
    } else if (mode == -1) {
        mode = *(volatile u8 *)&g_SoundReverbType;
    }

    long currentMode = 0;
    SpuGetReverbModeType(&currentMode);
    int needsAllocation = mode == 0 || (u32)currentMode != (u32)mode;
    s32 size = 0;
    s32 target = 0;
    if (needsAllocation) {
        s32 oldHandle = *(volatile s32 *)&g_SoundReverbMemoryHandle;
        if (oldHandle != -1) {
            SoundSpuMemoryFreeBlock(oldHandle);
        }
        if ((u32)mode < SPU_REV_MODE_MAX) {
            size = *(volatile s32 *)&g_ReverbWorkAreaSizes[mode];
        } else {
            /* An extended selector addresses the installed disc's RAM view. */
            u32 tableWord = 0x800508e8u + 4u * (u32)mode;
            __builtin_memcpy(&size, PSX_ADDR(tableWord), sizeof size);
        }
        u32 targetBits = 0x80000u - (u32)size;
        __builtin_memcpy(&target, &targetBits, sizeof target);
        u32 handle = SoundSpuMemoryAllocateBlockAtAddress(size, target, 5);
        *(volatile u32 *)&g_SoundReverbMemoryHandle = handle;
        if (handle == 0) {
            SoundHandleError(32);
            mode = depth = delay = feedback = 0;
        }
    }

    *(volatile u8 *)&g_SoundReverbType = (u8)mode;
    *(volatile u16 *)&g_SoundVolumeController.currentReverbDepth = (u16)depth;
    *(volatile u8 *)&g_SoundReverbDelay = (u8)delay;
    *(volatile u8 *)&g_SoundReverbFeedback = (u8)feedback;
    SoundApplyVolumeSettings();
    if (needsAllocation) {
        SpuSetReverbModeDepth(0, 0);
        SpuSetReverbModeType(mode);
        SoundInitiateReverbWorkAreaTransfer(target, size);
    } else {
        s16 left = *(volatile s16 *)&g_SoundReverbDepth.left;
        s16 right = *(volatile s16 *)&g_SoundReverbDepth.right;
        SpuSetReverbModeDepth(left, right);
        SpuSetReverbModeDelayTime(delay);
        SpuSetReverbModeFeedback(feedback);
    }
}
