int ArchiveCdDataSync(int mode) {
    int result;

    if (&g_PcPortArchiveDeferredFileSync && g_PcPortArchiveDeferredFileSync &&
        g_PcPortArchiveDeferredFileSync(mode, &result)) {
        return result;
    }

    if (mode == 0 && !g_PcPortArchiveFileRegistration &&
        !(g_PcPortArchiveQueueService && g_PcPortArchiveCancelFilteredQueue)) {
        for (;;) {
            PcPort_ArchivePollTransport();
            if (ArchiveDataSync() <= 0) {
                break;
            }
            if (PcPort_ArchiveWaitForQueueTick &&
                (g_PcPortArchiveQueueService ||
                 (g_PcPortArchiveCancelFilteredQueue && g_PcPortArchiveModeService))) {
                PcPort_ArchiveWaitForQueueTick();
            }
        }
    }
    return ArchiveDataSync();
}
