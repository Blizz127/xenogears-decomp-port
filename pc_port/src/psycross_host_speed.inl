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

void PsyX_SetSpeedMultiplier(int multiplier)
{
    if (multiplier < 1) multiplier = 1;
    if (multiplier > 5) multiplier = 5;
    if (SDL_AtomicSet(&s_xenoSpeed, multiplier) != multiplier) {
        PsyX_SPUAL_RefreshHostSpeed();
        eprintinfo("Game speed: %dx\n", PsyX_GetSpeedMultiplier());
    }
}

void PsyX_SetFastForwardHeld(int held)
{
    held = held != 0;
    if (SDL_AtomicSet(&s_xenoFastForwardHeld, held) != held) {
        PsyX_SPUAL_RefreshHostSpeed();
        eprintinfo("Game speed: %dx\n", PsyX_GetSpeedMultiplier());
    }
}

static void PsyX_InitialiseSpeed()
{
    const char* speed = getenv("XENO_SPEED");
    int multiplier = 1;
    if (speed && speed[0] >= '1' && speed[0] <= '5' && speed[1] == '\0')
        multiplier = speed[0] - '0';
    else if (speed)
        eprintwarn("XENO_SPEED must be 1, 2, 3, 4 or 5; using 1x\n");
    PsyX_SetFastForwardHeld(0);
    PsyX_SetSpeedMultiplier(multiplier);
}
