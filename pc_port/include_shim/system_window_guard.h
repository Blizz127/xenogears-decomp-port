#ifndef XENO_SYSTEM_WINDOW_GUARD_H
#define XENO_SYSTEM_WINDOW_GUARD_H
#include <stddef.h>
#include <stdint.h>

/* Installed only around a synchronous typed guest-to-host window call.
 * Callbacks must return only for an accepted native span/pair. A rejection
 * leaves preceding game writes intact and unwinds to the bridge.
 * Addresses are native pointers, never guest tokens or scalar arguments. */
typedef struct PcPortSystemWindowGuard {
    void *opaque;
    void (*check)(void *, uintptr_t, size_t, unsigned, int, const char *);
    void (*packet)(void *, const void *, const void *);
    void (*reject)(void *, const char *);
} PcPortSystemWindowGuard;

const PcPortSystemWindowGuard *PcPort_SystemWindowGuardSwap(
    const PcPortSystemWindowGuard *);
#endif
