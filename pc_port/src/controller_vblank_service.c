/* Native game-thread service for retail libetc's channel-4 callback.
 * PsyCross runs VSyncCallback on a separate SDL thread; game globals and
 * controller queues are not thread-safe, so no game callback is installed
 * there. Call this service from game-thread safe points instead. */
#include <stdint.h>
#include "system/controller_vblank.h"

extern int PsyX_Sys_GetVBlankCount(void);
/* XENO_DETERMINISTIC_VBLANK (psyq_compat.c): a jitter-free test clock the
 * service follows instead of the host counter once active.  Weak: absent in
 * unit tests that link this file alone. */
extern int PcPort_DeterministicVblank(uint32_t* now) __attribute__((weak));

static uint32_t serviced_count;
static int initialized;

static uint32_t vblank_now(void)
{
    uint32_t now = serviced_count;  /* in: the cursor the clock seeds from */
    uint32_t host;
    if (PcPort_DeterministicVblank && PcPort_DeterministicVblank(&now))
        return now;
    host = (uint32_t)PsyX_Sys_GetVBlankCount();
    /* Never step the cursor backwards: after a deterministic span the
     * virtual count may be ahead of the host's; wait for the host instead
     * of wrapping.  A no-op in normal runs (the host count only grows). */
    if (initialized && (int32_t)(host - serviced_count) < 0)
        return serviced_count;
    return host;
}
extern void PcPort_SoundLockstepVblank(void) __attribute__((weak));

/* Host speed-up (F6/F7/Backspace fast-forward, XENO_SPEED) and the retail
 * controller FIFO: every serviced vblank pushes one controller state, and
 * the game drains the 16-entry FIFO once per frame; on overflow it drops
 * that frame's whole input (ControllerResetState, retail behaviour for a
 * frame that ran too long).  At 5x the host vblank clock runs five times
 * faster, so a slow frame (the interpreted opening battle) spans far more
 * than 16 vblanks: the FIFO overflowed every frame and no press ever
 * reached the game (owner report 2026-10-05: soft-lock on a battle text box
 * at 5x).  While sped up, stop short of overflowing and drop the vblank
 * backlog instead: fast-forward then runs only as fast as the game consumes
 * frames, and input is never lost.  At 1x nothing changes.  Weak: unit
 * tests link this file without PsyCross or the controller module. */
extern int PsyX_GetSpeedMultiplier(void) __attribute__((weak));
extern unsigned int g_ControllerNumStates __attribute__((weak));
#define XENO_CONTROLLER_FIFO_STATES 16u

static int speedup_fifo_full(void)
{
    return PsyX_GetSpeedMultiplier && &g_ControllerNumStates != 0 &&
           PsyX_GetSpeedMultiplier() > 1 &&
           g_ControllerNumStates >= XENO_CONTROLLER_FIFO_STATES - 1u;
}
static void (*callback)(void);
static int servicing;
static int enabled = 1;
static int pending;
static uint32_t mask_epoch;
volatile int g_VsyncInterruptCount;

void PcPort_ResetVblankService(void)
{
    callback = 0;
    /* A reset takes the clock as it is: the never-backwards guard in
     * vblank_now() protects a running cursor, not a re-seed (it kept a stale
     * cursor when the host count was numerically lower, e.g. across the
     * 32-bit wrap). */
    initialized = 0;
    serviced_count = vblank_now();
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
    uint32_t now = vblank_now();
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
    now = vblank_now();
    epoch = mask_epoch;
    servicing = 1;
    while (pending || serviced_count != now) {
        if (speedup_fifo_full()) {
            serviced_count = now;   /* drop the sped-up backlog, keep the input */
            pending = 0;
            break;
        }
        if (pending)
            pending = 0;
        else
            ++serviced_count;
        g_VsyncInterruptCount = (int)((uint32_t)g_VsyncInterruptCount + 1u);
        if (callback)
            callback();
        /* XENO_SOUND_TICK_LOCKSTEP: the 4 RCnt2 sound ticks of this vblank,
         * on the game's clock (plat/xg_plat_psycross.c).  Weak: unit tests
         * that link this file alone have no sound tick. */
        if (PcPort_SoundLockstepVblank)
            PcPort_SoundLockstepVblank();
        /* Mask/reset operations may sample a newer clock. Do not continue
         * toward a stale target or overwrite their new cursor on return. */
        if (epoch != mask_epoch)
            break;
    }
    servicing = 0;
}
