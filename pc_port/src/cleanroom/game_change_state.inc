#include <limits.h>

void ChangeGameState(unsigned int requested)
{
    volatile unsigned int *cachedID = &g_CurGameStateOverlayID;
    unsigned int previous = *cachedID;
    *(volatile unsigned int *)&g_CurGameState = requested;
    if (requested == previous) {
        return;
    }

    void *volatile *slot = &g_CurGameStateOverlayBuffer;
    void *cached = *slot;
    if (cached) {
        HeapUnpinBlock((HeapBlock *)cached);
        HeapFree(*slot);
        *slot = NULL;
    }
    *cachedID = UINT_MAX;
}
