#define xeno_cleanroom_cfc2 CFC2
#include "gte_readgeom_cleanroom.h"

int32_t ReadGeomScreen(void)
{
    uint32_t w = xeno_cleanroom_cfc2(26);
    uint32_t lo = w & 0xFFFFu;

    /* sign-extend the low 16 bits; idempotent if the seam already extended them */
    return lo >= 0x8000u ? (int32_t)lo - 65536 : (int32_t)lo;
}

#undef xeno_cleanroom_cfc2
