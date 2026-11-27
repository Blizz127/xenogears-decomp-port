/* SPDX-License-Identifier: MIT */
static void PcPort_InitEnvelopeMethodTable(void);
void SoundClearVoiceDataPointers(void);
void SoundSetCdVolumeWithFade(s32 volume, s32 frames);

void SoundInitialize(s32 options)
{
    enum {
        active = 0x8000, startup_flags = 0xb801, setup_cd_mix = 0x4000,
        heap_bytes = 25344, queue_entries = 8, sample_reserve = 8192,
        sample_base = 65536, timer_period = 17640
    };

    if ((u16)g_SoundControlFlags & active) {
        SoundHandleError(40);
        return;
    }

    g_SoundControlFlags = (s16)((u16)options | startup_flags);
    PcPort_InitEnvelopeMethodTable();
    SpuInitMalloc(4, (char *)g_SoundSpuMemoryTableStart);
    SoundHeapInitialize(D_80065B0C, heap_bytes);
    SoundSpuMemoryInitialize();
    g_SoundTransferQueue = SoundHeapAllocate(queue_entries * sizeof(SoundTransferCommand));
    SoundClearVoiceDataPointers();

    D_800594E4 = 0x12345678;
    D_80059518 = NULL;
    D_800595D8 = 0;
    g_SoundAudioManagerListHead = NULL;
    g_SoundWdsLinkedList = NULL;
    g_SoundSedsLinkedList = NULL;
    g_SoundKeyOffFlags = 0;
    g_SoundKeyOnFlags = 0;
    g_unk_VoicesNeedingProcessing = 0;
    g_SoundVolumeController.commonAttr.mask = 12;
    g_SoundVolumeController.commonAttr.mvolmode.left = 0;
    g_SoundVolumeController.commonAttr.mvolmode.right = 0;

    EnterCriticalSection();
    g_unk_SoundEvent = (u32)OpenEvent(RCntCNT2, 2, RCntMdINTR, PcPort_SoundSequenceTimer());
    SetRCnt((int)RCntCNT2, timer_period, RCntMdINTR);
    StartRCnt((int)RCntCNT2);
    SpuSetTransferCallback(SoundOnTransferCallback);
    SpuSetIRQCallback(SoundSpuIRQHandler);
    SpuSetIRQ(0);
    D_80059504 = 0;
    g_SoundSpuIRQCount = 0;
    ExitCriticalSection();

    SoundSpuMemoryAllocateBlockAtAddress(sample_reserve, sample_base, 4);
    func_800386C4(1);
    SoundSetCdAttr(0, 1);
    SoundSetMasterVolumeWithFade(16383, 0);
    SoundSetCdVolumeWithFade(32767, 0);
    if ((u16)g_SoundControlFlags & setup_cd_mix)
        SoundSetupCdMix(128);

    D_800595D8 = SOUND_PTR_TO_PSX(func_8003B148(16));
    D_80059544 = 8;
    g_SoundReverbType = (u8)-1;
    g_SoundReverbMemoryHandle = -1;
    g_SoundUploadDestBuffer = 0;
    SoundSetReverbModeWithAllocation(4, 0, 0, 0);
    SpuSetReverb(1);
    g_SoundSpuErrorId = 0;
}
