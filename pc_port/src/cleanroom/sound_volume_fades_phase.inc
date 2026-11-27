static inline s16 SoundVolumeSignedHalf(u32 bits)
{
    return (s16)((s32)((bits & 0xffffu) ^ 0x8000u) - 0x8000);
}

static inline s32 SoundVolumeSignedWord(u32 bits)
{
    return (s32)((long long)(bits ^ 0x80000000u) - 0x80000000LL);
}

static inline void SoundScheduleVolumeFade(volatile AudioInterpolator *interpolator,
                                          s32 volume, s32 frames)
{
    u32 current = (u32)interpolator->currentValue;
    /* Bias the sign bit before shifting to obtain a signed arithmetic shift by eight. */
    s32 currentUnits = (s32)((current ^ 0x80000000u) >> 8) - 0x800000;
    u32 difference = ((u32)volume << 8) - (u32)currentUnits;

    if (difference != 0) {
        long long quotient = (long long)SoundVolumeSignedWord(difference) / frames;
        interpolator->counter = SoundVolumeSignedHalf((u32)frames);
        interpolator->stepIncrement = SoundVolumeSignedWord((u32)quotient << 8);
    }
}

void SoundSetVolumeWithPhase(s32 volume, SpuVolume *pVolume, s32 channelSelect)
{
    volatile SpuVolume *output = pVolume;
    u16 flags = (u16)*(volatile s16 *)&g_SoundControlFlags;
    s16 level = SoundVolumeSignedHalf((u32)volume);

    output->right = level;
    output->left = level;
    if ((flags & 0x600u) != 0) {
        u8 rightSelector = (flags & 0x200u) ? 0 : 1;
        s16 inverted = SoundVolumeSignedHalf(0u - (u32)volume);
        if ((u8)channelSelect == rightSelector) {
            output->right = inverted;
        } else {
            output->left = inverted;
        }
    }
}

void SoundSetMasterVolumeWithFade(s32 volume, s32 frames)
{
    volatile SoundVolumeController *controller = &g_SoundVolumeController;
    s16 level = SoundVolumeSignedHalf((u32)volume);

#ifdef _WIN32
    PsyX_Sys_SoundGateEnterCritical();
#endif
    controller->masterInterpolator.targetValue = level;
    if (frames != 0) {
        SoundScheduleVolumeFade(&controller->masterInterpolator, volume, frames);
    } else {
        controller->masterInterpolator.currentValue = SoundVolumeSignedWord((u32)volume << 16);
        controller->masterInterpolator.counter = 0;
        controller->currentMasterVolume = level;
        SoundSetVolumeWithPhase(level, &g_SoundVolumeController.commonAttr.mvol, 0);
        controller->commonAttr.mask |= 3u;
    }
#ifdef _WIN32
    PsyX_Sys_SoundGateExitCritical();
#endif
}

#ifdef _WIN32
static void SoundSetCdVolumeWithFadeLocked(s32 volume, s32 frames)
#else
void SoundSetCdVolumeWithFade(s32 volume, s32 frames)
#endif
{
    volatile SoundVolumeController *controller = &g_SoundVolumeController;
    s16 level = SoundVolumeSignedHalf((u32)volume);

    controller->cdInterpolator.targetValue = level;
    if (frames != 0) {
        SoundScheduleVolumeFade(&controller->cdInterpolator, volume, frames);
    } else {
        u32 mask = controller->commonAttr.mask;
        controller->cdInterpolator.currentValue = SoundVolumeSignedWord((u32)volume << 16);
        controller->cdInterpolator.counter = 0;
        controller->currentCdVolume = level;
        controller->commonAttr.cd.volume.right = level;
        controller->commonAttr.cd.volume.left = level;
        controller->commonAttr.mask = mask | 0xc0u;
    }
}
