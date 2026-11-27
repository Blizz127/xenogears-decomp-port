void *LoadGameStateOverlay(unsigned int state)
{
    if (state >= NUM_GAME_STATE_OVERLAYS || g_GameStateOverlayArchiveOffsets[state] <= 0) {
        return NULL;
    }
    if (state == g_CurGameStateOverlayID) return g_CurGameStateOverlayBuffer;
    if (overlayLoadInProgress) return NULL;

    overlayLoadInProgress = 1;
    if (g_CurGameStateOverlayBuffer) {
        HeapUnpinBlock(g_CurGameStateOverlayBuffer);
        HeapFree(g_CurGameStateOverlayBuffer);
        g_CurGameStateOverlayBuffer = NULL;
    }
    g_CurGameStateOverlayID = UINT32_MAX;

    u_int owner = HeapGetCurrentUser();
    int section, entry;
    ArchiveGetArchiveOffsetIndices(&section, &entry);
    HeapSetCurrentUser(HEAP_USER_SUGI);
    ArchiveSetIndex(0, 1);
    u_int handler = HeapToggleErrorHandler(HEAP_ERROR_HANDLER_OFF);
    int logical = ArchiveDecodeSize(g_GameStateOverlayArchiveOffsets[state]);
    const PcPortOverlayLayout *layout = PcPort_GetOverlayLayout(state);
    if (layout && (g_ArchiveDebugTable || layout->file != g_GameStateOverlayArchiveOffsets[state] ||
                   layout->logicalSize != logical)) {
        layout = NULL;
    }
    int capacity = layout ? layout->sourceSize : (state == 4u ? 0 : logical);
    void *data = capacity > 0 ? HeapAlloc((u_int)capacity, 1) : NULL;
    if (!data) {
        fprintf(stderr, "[xeno-port] overlay %u archive %d unavailable (size=%d, no buffer)\n",
                state, g_GameStateOverlayArchiveOffsets[state], logical);
    } else {
        HeapPinBlock(data);
        unsigned size = (unsigned)logical;
        int result = layout ? PcPort_ArchiveReadGameOverlay(state, data, (unsigned)capacity, &size)
                            : ArchiveReadFileToBuffer(g_GameStateOverlayArchiveOffsets[state], data, 0, 0);
        if (result == 0) result = ArchiveCdDataSync(0);
        if (result == 0) {
            cachedSourceSize = size;
            cachedExpectedImageSize = layout && (state == 4u || size == (unsigned)layout->sourceSize)
                ? (size_t)layout->imageSize : 0;
            g_CurGameStateOverlayBuffer = data;
            g_CurGameStateOverlayID = state;
        } else {
            HeapUnpinBlock(data);
            HeapFree(data);
            fprintf(stderr, "[xeno-port] overlay %u archive %d read failed (status=%d)\n",
                    state, g_GameStateOverlayArchiveOffsets[state], result);
        }
    }
    HeapToggleErrorHandler(handler);
    ArchiveSetIndex(section, entry);
    HeapSetCurrentUser(owner);
    overlayLoadInProgress = 0;
    return g_CurGameStateOverlayBuffer;
}
