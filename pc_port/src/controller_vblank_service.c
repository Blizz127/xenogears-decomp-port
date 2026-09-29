/* Native game-thread service for retail libetc's channel-4 callback.
 * PsyCross runs VSyncCallback on a separate SDL thread; game globals and
 * controller queues are not thread-safe, so no game callback is installed
 * there. Call this service from game-thread safe points instead. */
#include <stdint.h>
#include "system/controller_vblank.h"

extern int PsyX_Sys_GetVBlankCount(void);
static void (*callback)(void);
static uint32_t serviced_count;
static int servicing;
static int initialized;
static int enabled = 1;
static int pending;
static uint32_t mask_epoch;
volatile int g_VsyncInterruptCount;

void PcPort_ResetVblankService(void)
{
    callback = 0;
    serviced_count = (uint32_t)PsyX_Sys_GetVBlankCount();
    g_VsyncInterruptCount = 0;
    enabled = 1;
    pending = 0;
    initialized = 1;
    ++mask_epoch;
}

static void initialize(void)
{
    if (!initialized)
        PcPort_ResetVblankService();
}

/* I_STAT holds one pending bit, not one event for every masked vblank. */
static void latch_masked_ticks(void)
{
    uint32_t now = (uint32_t)PsyX_Sys_GetVBlankCount();
    pending |= now != serviced_count;
    serviced_count = now;
}

int PcPort_MaskVblank(void)
{
    int previous;
    initialize();
    previous = enabled;
    if (enabled) {
        latch_masked_ticks();
        enabled = 0;
        ++mask_epoch;
    }
    return previous;
}

void PcPort_UnmaskVblank(void)
{
    initialize();
    if (!enabled) {
        latch_masked_ticks();
        enabled = 1;
        ++mask_epoch;
    }
}

int PcPort_GetServicedVblankCount(void)
{
    return g_VsyncInterruptCount;
}

void func_8004B7D0(void (*next)(void))
{
    initialize();
    callback = next;
}

void PcPort_ServiceVblank(void)
{
    uint32_t now;
    uint32_t epoch;
    if (servicing)
        return;
    initialize();
    if (!enabled) {
        latch_masked_ticks();
        return;
    }
    now = (uint32_t)PsyX_Sys_GetVBlankCount();
    epoch = mask_epoch;
    servicing = 1;
    while (pending || serviced_count != now) {
        if (pending)
            pending = 0;
        else
            ++serviced_count;
        g_VsyncInterruptCount = (int)((uint32_t)g_VsyncInterruptCount + 1u);
        if (callback)
            callback();
        /* Mask/reset operations may sample a newer clock. Do not continue
         * toward a stale target or overwrite their new cursor on return. */
        if (epoch != mask_epoch)
            break;
    }
    servicing = 0;
}
