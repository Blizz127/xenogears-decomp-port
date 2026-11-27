#define xeno_cleanroom_cfc2 CFC2
#include "gte_readgeom_cleanroom.h"

/* floor(int32(raw) / 65536) without relying on right shift of a negative value */
static int32_t offset_value(uint32_t raw)
{
    uint32_t hi = raw >> 16;
    return hi >= 0x8000u ? (int32_t)hi - 65536 : (int32_t)hi;
}

void ReadGeomOffset(long *ofx, long *ofy)
{
    uint32_t raw_x = xeno_cleanroom_cfc2(24);
    uint32_t raw_y = xeno_cleanroom_cfc2(25);
    int32_t x = offset_value(raw_x);
    int32_t y = offset_value(raw_y);

    /* both reads precede both stores; ofx first, ofy second (second store wins on alias) */
    *ofx = (long)x;
    *ofy = (long)y;
}

#undef xeno_cleanroom_cfc2
