void SoundReset(void)
{
    volatile short *const controlFlags = &g_SoundControlFlags;
    volatile short *const errorId = &g_SoundSpuErrorId;
    s32 channel;

    if (*controlFlags == 0) {
        SoundHandleError(41);
        return;
    }

    EnterCriticalSection();
    *controlFlags = 0;
    SpuSetIRQ(0);
    SpuSetTransferCallback(NULL);
    SpuSetIRQCallback(NULL);
    StopRCnt(RCntCNT2);
    CloseEvent(*(volatile u32 *)&g_unk_SoundEvent);
    ExitCriticalSection();

    for (channel = 0; channel < 24; ++channel) {
        SoundSetVoiceAdsrReleaseShiftAndMode(channel, 6, 3);
    }
    SoundSetVoiceKeyOff(0x00ffffffu);
    SpuSetReverbModeDepth(0, 0);
    SpuSetReverbModeType(0);
    *errorId = 0;
}
