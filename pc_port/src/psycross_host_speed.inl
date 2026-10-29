/* Included by PsyX_main.cpp. Atomic reads are shared by the interrupt, CD and
 * sound workers; only host UI/startup changes the selected wall-clock rate. */
#include "host_speed.h"

static SDL_atomic_t s_xenoSpeed = { 1 };
static SDL_atomic_t s_xenoFastForwardHeld = { 0 };
static SDL_atomic_t s_xenoFastForwardSpeed = { 5 };
void PsyX_SPUAL_RefreshHostSpeed();

int PsyX_GetSpeedMultiplier(void)
{
    return SDL_AtomicGet(&s_xenoFastForwardHeld) ? SDL_AtomicGet(&s_xenoFastForwardSpeed)
                                                 : SDL_AtomicGet(&s_xenoSpeed);
}

/* Speed while the fast-forward key is held (default 5x); set from the
 * port config (game.fast_forward_speed) through xg_plat/timing.h. */
void PsyX_SetFastForwardSpeed(int multiplier)
{
    if (multiplier < 1) multiplier = 1;
    if (multiplier > 5) multiplier = 5;
    SDL_AtomicSet(&s_xenoFastForwardSpeed, multiplier);
}

/* Every speed change is logged with the thing that asked for it:
 *   [Psy-X] speed: fast-forward on (source: F6)
 *   [Psy-X] speed: 3x (source: F7)
 * `source` is one of F6, Backspace, L3, R3, dev-menu, F7, focus-lost, config,
 * console, env, api.  Nothing here is rate-limited (the input trace is, and
 * once hid the events that changed speed). */
void PsyX_SetSpeedMultiplierFrom(int multiplier, const char* source)
{
    if (multiplier < 1) multiplier = 1;
    if (multiplier > 5) multiplier = 5;
    if (SDL_AtomicSet(&s_xenoSpeed, multiplier) != multiplier) {
        PsyX_SPUAL_RefreshHostSpeed();
        eprintinfo("speed: %dx (source: %s)\n", multiplier, source ? source : "unknown");
    }
}

void PsyX_SetFastForwardHeldFrom(int held, const char* source)
{
    held = held != 0;
    if (SDL_AtomicSet(&s_xenoFastForwardHeld, held) != held) {
        PsyX_SPUAL_RefreshHostSpeed();
        eprintinfo("speed: fast-forward %s (source: %s)\n", held ? "on" : "off",
            source ? source : "unknown");
    }
}

void PsyX_SetSpeedMultiplier(int multiplier) { PsyX_SetSpeedMultiplierFrom(multiplier, "api"); }
void PsyX_SetFastForwardHeld(int held) { PsyX_SetFastForwardHeldFrom(held, "api"); }

static void PsyX_InitialiseSpeed()
{
    const char* speed = getenv("XENO_SPEED");
    int multiplier = 1;
    if (speed && speed[0] >= '1' && speed[0] <= '5' && speed[1] == '\0')
        multiplier = speed[0] - '0';
    else if (speed)
        eprintwarn("XENO_SPEED must be 1, 2, 3, 4 or 5; using 1x\n");
    PsyX_SetFastForwardHeldFrom(0, "init");
    PsyX_SetSpeedMultiplierFrom(multiplier, "env");
}
