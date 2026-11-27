#ifdef _WIN32
extern void PsyX_SpuSetMovieTerminalCommonAttr(SpuCommonAttr *settings,
                                               unsigned long long token);

static void SoundSubmitMovieCommonAttr(SpuCommonAttr *settings, int cdStepped)
{
    unsigned long long token = cdStepped ? s_movieTerminalCdFade : 0;
    if (token)
        PsyX_SpuSetMovieTerminalCommonAttr(settings, token);
    else
        SpuSetCommonAttr(settings);
}
#endif
