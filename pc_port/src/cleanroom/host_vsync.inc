/* Host display synchronization for the game-spelling Vsync API.
 * PsyCross owns the paced clock and swaps. Only the game thread delivers
 * controller/interrupt callbacks. A clock query must never flip a page. */
extern int PsyX_IsSceneOpen(void);
extern int PsyX_GpuTraceOn(void);
extern int PsyX_PresentDisplayFromVRAM(void);
extern int PsyX_PresentMoviePageFromVRAM(const DISPENV *page);
extern void PcPort_FieldCaptureOnVsync(void);
#pragma weak PcPort_FieldCaptureOnVsync
extern void PcPort_FieldPosDiag(void);
#pragma weak PcPort_FieldPosDiag
extern void PcPort_CullCamLogOnVsync(void);
extern void PcPort_WalkmeshDump(void) __attribute__((weak));
extern int PcPort_DiscPromptTakeRedraw(void) __attribute__((weak));

typedef int (*HostMoviePresenter)(const DISPENV *);

static void host_present_wait(int mode, const DISPENV *movie, HostMoviePresenter show_movie)
{
    const int rendered = PsyX_IsSceneOpen();
    if (PsyX_GpuTraceOn()) {
        extern int g_xeno_swap_index;
        printf("[gputrace] s=%d Vsync(%d) scene=%d\n", g_xeno_swap_index, mode, rendered);
    }
    DrawAllSplits();
    PsyX_EndScene();
    if (PcPort_FieldCaptureOnVsync)
        PcPort_FieldCaptureOnVsync();

    /* A rendered scene already swapped. A static screen can have changed
     * independently (a decoded movie page or the host disc insertion UI). */
    int repaint = mode == 0;
    if (mode == 3 && PcPort_DiscPromptTakeRedraw)
        repaint = PcPort_DiscPromptTakeRedraw();
    if (!rendered && repaint) {
        if (movie && show_movie)
            show_movie(movie);
        else
            PsyX_PresentDisplayFromVRAM();
    }
    if (PcPort_FieldPosDiag)
        PcPort_FieldPosDiag();
    if (xg_plat_mods_emit)
        xg_plat_mods_emit(XG_PLAT_EVENT_FRAME_TICK, 0, 0);
    if (PcPort_WalkmeshDump)
        PcPort_WalkmeshDump();
}

/* The opt-in harness clock has two domains. A field load keeps a live host
 * interrupt service while holding the game-time query at its previous value.
 * No real-time game path depends on the harness counters. */
static void host_advance_test_vblanks(unsigned ticks)
{
    s_detQuery += ticks;
    s_detVblankNow += ticks;
    if (s_detBattle)
        PcPort_DetBattleNoteAdvance();
}

static int host_sync_for_game(int mode, const DISPENV *movie, HostMoviePresenter show_movie)
{
    const int waits = mode == 0 || mode > 1;
    if (mode < 0 && PcPort_DetVblankActive()) {
        if (s_detPollPending && s_detPollLast == s_detVblankNow) {
            host_advance_test_vblanks(1);
            if (++s_detSpinVblanks == 1)
                fputs("[det-vblank] busy-wait Vsync(-1) poll advances the test clock by one vblank per spin (not a retail-timing claim)\n", stderr);
        }
        s_detPollLast = s_detVblankNow;
        s_detPollPending = 1;
    } else {
        s_detPollPending = 0;
    }
    if (waits) {
        host_present_wait(mode, movie, show_movie);
        if (PcPort_DetVblankActive())
            host_advance_test_vblanks(mode > 1 ? (unsigned)mode : (unsigned)PcPort_DetVblankK());
    }

    int sample = VSync(mode);
    XENO_SERVICE_VBLANK();

    if (mode < 0)
        return PcPort_GetServicedVblankCount ? PcPort_GetServicedVblankCount() : sample;
    if (mode == 1) {
        if (s_detVblankOn)
            return (int)s_detQuery;
        s_detLastQuery = sample;
        return sample;
    }

    PcPort_ForcedKernelSelect();
    PcPort_ForcedFieldMenu();
    PcPort_CullCamLogOnVsync();
    return s_detVblankOn ? (int)s_detQuery : sample;
}

int Vsync(int mode)
{
    return host_sync_for_game(mode, NULL, NULL);
}

void PcPort_VsyncMoviePage(const DISPENV *page)
{
    if (page && page->isrgb24) {
        DISPENV complete = *page;
        host_sync_for_game(0, &complete, PsyX_PresentMoviePageFromVRAM);
    } else {
        Vsync(0);
    }
}
