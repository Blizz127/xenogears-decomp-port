/* Psy-Q compatibility: SpuSetNoiseClock.
 *
 * Documented behaviour (Psy-Q Library Reference, SpuSetNoiseClock: "sets the
 * noise clock, 0..0x3F, returns the value set"; psx-spx "SPU Control
 * Register (SPUCNT, 1F801DAAh)": bits 8-9 noise frequency step and bits
 * 10-13 noise frequency shift form one 6-bit noise clock field): clamp the
 * request to 0..0x3F, write it to SPUCNT bits 8-13 leaving the other bits
 * unchanged, and return the clamped value.  PsyCross has no equivalent.
 *
 * The port backs the SPU register page with native storage
 * (g_pSoundSpuRegisters); SPUCNT keeps its hardware offset 0x1AA from the
 * page base 1F801C00h.  The W34N121_MUTANT_* branches are fault injections
 * for pc_port/tests/run_w34n121_spu_noise_clock.sh.
 */
#include <string.h>

#include "common.h"
#include "psyq_spu_noise_clock.h"

extern void* g_pSoundSpuRegisters;

#if defined(W34N121_MUTANT_WRONG_REGISTER_OFFSET)
#define SPUCNT_OFFSET 0x1A8u
#else
#define SPUCNT_OFFSET 0x1AAu /* 1F801DAAh - 1F801C00h */
#endif
#define SPUCNT_NOISE_SHIFT 8
#define SPUCNT_NOISE_MASK  (0x3Fu << SPUCNT_NOISE_SHIFT)

long SpuSetNoiseClock(long noise_clock)
{
    u8* spucnt_addr = (u8*)g_pSoundSpuRegisters + SPUCNT_OFFSET;
    long clock = noise_clock;
    u16 spucnt;

#if !defined(W34N121_MUTANT_NO_LOW_CLAMP)
    if (clock < 0)
        clock = 0;
#endif
#if !defined(W34N121_MUTANT_NO_HIGH_CLAMP)
    if (clock > 0x3F)
        clock = 0x3F;
#endif

    memcpy(&spucnt, spucnt_addr, sizeof spucnt);
#if defined(W34N121_MUTANT_WRONG_PRESERVE_MASK)
    spucnt = (u16)((spucnt & 0x00FFu) | (((u16)clock & 0x3Fu) << SPUCNT_NOISE_SHIFT));
#elif defined(W34N121_MUTANT_WRONG_SHIFT)
    spucnt = (u16)((spucnt & (u16)~SPUCNT_NOISE_MASK) | (((u16)clock & 0x3Fu) << 7));
#else
    spucnt = (u16)((spucnt & (u16)~SPUCNT_NOISE_MASK) |
                   (((u16)clock & 0x3Fu) << SPUCNT_NOISE_SHIFT));
#endif
    memcpy(spucnt_addr, &spucnt, sizeof spucnt);

#if defined(W34N121_MUTANT_RETURN_INPUT)
    return noise_clock;
#else
    return clock;
#endif
}
