#ifndef PC_PORT_WIN32_CLOCK_H
#define PC_PORT_WIN32_CLOCK_H

#include <SDL_timer.h>
#include <stdint.h>

/* Convert the host performance counter without multiplying its uptime by
 * one million. The archive clock needs sub-millisecond resolution. */
static inline uint64_t PcPort_Win32MonotonicUsec(void)
{
    uint64_t frequency = SDL_GetPerformanceFrequency();
    uint64_t count = SDL_GetPerformanceCounter();
    if (frequency == 0)
        return 0;
    return (count / frequency) * 1000000u + ((count % frequency) * 1000000u) / frequency;
}

#endif
