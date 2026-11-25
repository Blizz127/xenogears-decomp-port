void SoundApplyVolumeSettings(void)
{
    volatile SoundVolumeController *controller = &g_SoundVolumeController;
#ifdef _WIN32
    PsyX_Sys_SoundGateEnterCritical();
    SoundCancelMovieTerminalCdFade();
#endif
    SoundSetVolumeWithPhase(controller->currentMasterVolume,
                            &g_SoundVolumeController.commonAttr.mvol, 0);
    /* Sample both settings after the master callback. Both CD channels
     * receive the same sample before the reverb callback runs. */
    const s32 reverbDepth = controller->currentReverbDepth;
    const s32 cdVolume = controller->currentCdVolume;
    controller->commonAttr.cd.volume.right = cdVolume;
    controller->commonAttr.cd.volume.left = cdVolume;
    SoundSetVolumeWithPhase(reverbDepth, &g_SoundReverbDepth, 1);
    controller->commonAttr.mask |= 0xC3U;
#ifdef _WIN32
    PsyX_Sys_SoundGateExitCritical();
#endif
}
