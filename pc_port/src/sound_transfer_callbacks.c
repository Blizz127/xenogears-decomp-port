#include "common.h"
#include "system/sound.h"

/*
 * The transfer queue has eight stable physical slots.  A nonzero token is the
 * slot index plus one; zero retains retail's null-callback meaning.  Function
 * pointers remain exclusively in this host-side table and are never narrowed.
 */
static SoundCommandCallback_t
    g_SoundTransferCallbackSidecar[SOUND_TRANSFER_QUEUE_SIZE];

SoundTransferCallbackToken SoundTransferCallbackStore(
    u16 queueIndex, SoundCommandCallback_t callback) {
    if (queueIndex >= SOUND_TRANSFER_QUEUE_SIZE) {
        return 0;
    }

    g_SoundTransferCallbackSidecar[queueIndex] = callback;
    if (callback == NULL) {
        return 0;
    }

    return (SoundTransferCallbackToken)queueIndex + 1;
}

SoundCommandCallback_t SoundTransferCallbackResolve(
    SoundTransferCallbackToken token) {
    if (token == 0 || token > SOUND_TRANSFER_QUEUE_SIZE) {
        return NULL;
    }

    return g_SoundTransferCallbackSidecar[token - 1];
}
