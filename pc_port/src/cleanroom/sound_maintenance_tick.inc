/* SPDX-License-Identifier: MIT */
#include <stdatomic.h>

extern s32 D_80059540;
extern u16 D_8005955C;
extern s32 D_800595C4;
static atomic_uint_least64_t s_soundUnsupportedManagerTicks;

uint64_t PcPort_SoundUnsupportedManagerTicks(void)
{
    return atomic_load_explicit(&s_soundUnsupportedManagerTicks, memory_order_relaxed);
}

static void SoundApplyPendingMixerUpdates(void)
{
    SoundVolumeController *volume = &g_SoundVolumeController;
    if (volume->masterInterpolator.counter) {
        SoundTickInterpolator(&volume->masterInterpolator);
        volume->currentMasterVolume = (s16)((uint32_t)volume->masterInterpolator.currentValue >> 16);
        SoundSetVolumeWithPhase(volume->currentMasterVolume, &volume->commonAttr.mvol, 0);
        volume->commonAttr.mask |= 3u;
    }

    const int cdStepped = volume->cdInterpolator.counter != 0;
    if (cdStepped) {
        SoundTickInterpolator(&volume->cdInterpolator);
        const s16 level = (s16)((uint32_t)volume->cdInterpolator.currentValue >> 16);
        volume->currentCdVolume = level;
        volume->commonAttr.cd.volume.left = level;
        volume->commonAttr.cd.volume.right = level;
        volume->commonAttr.mask |= 0xc0u;
    }
    if (volume->commonAttr.mask) {
#ifdef _WIN32
        SoundSubmitMovieCommonAttr(&volume->commonAttr, cdStepped);
#else
        SpuSetCommonAttr(&volume->commonAttr);
#endif
        volume->commonAttr.mask = 0;
    }
}

/* Sequence requests are admitted separately. The hardware timer continues to
 * service the mixer, direct voice writes and IRQ even with playback unavailable. */
long PcPort_SoundMaintenanceTick(void)
{
    if (g_SoundControlFlags & 0x40u)
        return 0;

    const uint32_t begun = (uint32_t)GetRCnt((int)RCntCNT2);
    D_80059504 = (s32)((uint32_t)D_80059504 + 1u);
    if (!(D_80059504 & 1))
        SoundApplyPendingMixerUpdates();

    for (const AudioManager *m = g_SoundAudioManagerListHead; m;
         m = SOUND_PSX_TO_PTR(AudioManager, m->next)) {
        if ((uint16_t)m->unk_Flags & 0x8000u) {
            if (!atomic_fetch_add_explicit(&s_soundUnsupportedManagerTicks, 1, memory_order_relaxed))
                fputs("[xeno-port] sequence update unavailable for active sound manager\n", stderr);
            break;
        }
    }
    func_8003E900();
    func_8003EB5C();
    if (D_8005955C & 1u) {
        D_8005955C &= (u16)~1u;
        SpuSetIRQ(1);
    }

    const uint32_t ended = (uint32_t)GetRCnt((int)RCntCNT2);
    if (ended >= begun) {
        D_800595C4 = (s32)((uint32_t)D_800595C4 + (ended - begun));
        D_80059540 = (s32)((uint32_t)D_80059540 + 1u);
    }
    return 0;
}
