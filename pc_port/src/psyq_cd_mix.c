/*
 * PsyQ CD attenuator adapter for the native port.
 *
 * Retail CdMix (SLUS_006.64 0x8004138C-0x800413AC) submits the four-byte
 * CdlATV matrix to the CD controller and returns one.  PsyCross has no CdMix
 * owner, so forward the same matrix to its XA mixer instead of allowing the
 * generated no-op stub to discard it.
 */

#include "common.h"
#include "psyq/libcd.h"

extern void PsyX_SPUAL_SetCdAttenuation(
    unsigned char val0, unsigned char val1,
    unsigned char val2, unsigned char val3);

int CdMix(CdlATV *volume)
{
    PsyX_SPUAL_SetCdAttenuation(
        volume->val0, volume->val1, volume->val2, volume->val3);
    return 1;
}
