void SoundSetCdAttr(long reverb, long mix)
{
    volatile SpuCommonAttr *attr = &g_SoundVolumeController.commonAttr;
#ifdef _WIN32
    PsyX_Sys_SoundGateEnterCritical();
#endif
    u32 mask = attr->mask;

    attr->cd.reverb = (s32)(u32)reverb;
    attr->cd.mix = (s32)(u32)mix;
    attr->mask = mask | SPU_COMMON_CDREV | SPU_COMMON_CDMIX;
    SpuSetCommonAttr(&g_SoundVolumeController.commonAttr);
#ifdef _WIN32
    PsyX_Sys_SoundGateExitCritical();
#endif
}

extern void func_8003B930(AudioManager *manager);

void SoundInitializeAudioManager(AudioManager *manager)
{
    volatile AudioManager *state = manager;

    func_8003B930(manager);
    state->unk_0x32 = 1;
    state->unk_0x36 = 1;
    state->unk_0x3a = 0x30;
    state->unk_Interpolator_0x64.currentValue = 0x01000000;
    state->unk_Interpolator_0x70.currentValue = 0x7f000000;
    state->unk_0x58 = 0x00660000;
    state->unk_0x54 = 0x00006600;
    state->unk_0x1a = 0;
    state->unk_0x1b = 0;
    state->unk_0x30 = 0;
    state->unk_0x34 = 0;
    state->unk_0x38 = 4;
    state->unk_0x3c = 4;
    state->unk_0x3e = 4;
    state->unk_0x28 = 0;
    state->unk_0x24 = 0;
    state->unk_0x20 = 0;
    state->unk_0x48 = 0;
    state->unk_Interpolator_0x7c.currentValue = 0;
    state->unk_Interpolator_0x88.currentValue = 0;
    state->unk_Interpolator_0x64.counter = 0;
    state->unk_Interpolator_0x70.counter = 0;
    state->unk_Interpolator_0x7c.counter = 0;
    state->unk_Interpolator_0x88.counter = 0;
    state->unk_0x5c = 0;
    state->unk_0x60 = 0;
    state->unk_0x50 = 0x00010000;
}
