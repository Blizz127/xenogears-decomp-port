#include <stdint.h>
#include "psyq_spu_noise_clock.h"

extern void *g_pSoundSpuRegisters;

long SpuSetNoiseClock(long noise_clock)
{
    if (noise_clock < 0)
        noise_clock = 0;
    if (noise_clock > 63)
        noise_clock = 63;

    /* ATTR is a halfword in the aligned SPU register bank. */
    volatile uint16_t *registers = g_pSoundSpuRegisters;
    volatile uint16_t *attr = &registers[0x1aa / sizeof(*registers)];
    uint16_t saved = *attr;
    *attr = (uint16_t)((saved & 0xc0ffu) | ((unsigned int)noise_clock << 8));
    return noise_clock;
}
