s32 SoundUnkDebug0(void *context)
{
    (void)context;
    return 0;
}

void SoundHeapFree(void *payload)
{
    SoundHeapBlockHeader *previous = g_SoundHeapHead;
    DisableEvent((long)g_unk_SoundEvent);

    if (previous && (void *)(previous + 1) == payload)
        previous = NULL;

    while (previous && previous->pNext) {
        SoundHeapBlockHeader *next = SOUND_PSX_TO_PTR(SoundHeapBlockHeader, previous->pNext);
        if ((void *)(next + 1) == payload) {
            previous->pNext = next->pNext;
            break;
        }
        previous = next;
    }

    EnableEvent((long)g_unk_SoundEvent);
}

void SoundHandleError(s32 errorId)
{
    if (g_SoundControlFlags & 0x88)
        return;

    g_SoundControlFlags |= 8;
    g_SoundSpuErrorId = (s16)errorId;
    SoundSpuMemoryFreeBlock(0x10000);
    SoundLoadWdsFile(&D_80050940, 0);
    SoundAddSedsEntry(&D_80050910);
    func_8003BDFC(0x10);

    u32 message = (u32)D_80050924[0] << 16;
    func_80039E60((s32)(message | 1u));
}

#ifdef _WIN32
static unsigned long long s_movieTerminalCdFade;
static void SoundSetCdVolumeWithFadeLocked(s32 volume, s32 frames);
extern void PsyX_SPUAL_CancelMovieTerminalFade(unsigned long long token);

static void SoundCancelMovieTerminalCdFade(void)
{
    if (s_movieTerminalCdFade) {
        PsyX_SPUAL_CancelMovieTerminalFade(s_movieTerminalCdFade);
        s_movieTerminalCdFade = 0;
    }
}

void SoundSetCdVolumeWithFade(s32 volume, s32 ticks)
{
    PsyX_Sys_SoundGateEnterCritical();
    SoundCancelMovieTerminalCdFade();
    SoundSetCdVolumeWithFadeLocked(volume, ticks);
    PsyX_Sys_SoundGateExitCritical();
}

void SoundSetMovieTerminalCdFade(unsigned long long receipt)
{
    PsyX_Sys_SoundGateEnterCritical();
    SoundCancelMovieTerminalCdFade();
    if (receipt) {
        const AudioInterpolator *fade = &g_SoundVolumeController.cdInterpolator;
        if (fade->counter == 0 && fade->currentValue == 0x7fff0000 &&
            !(g_SoundVolumeController.commonAttr.mask & (SPU_COMMON_CDVOLL | SPU_COMMON_CDVOLR))) {
            s_movieTerminalCdFade = receipt;
        } else {
            PsyX_SPUAL_CancelMovieTerminalFade(receipt);
        }
    }
    SoundSetCdVolumeWithFadeLocked(0, 40);
    PsyX_Sys_SoundGateExitCritical();
}
#endif
