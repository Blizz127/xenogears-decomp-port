/* Retail world-map planar distance helper [0x80094154,0x800941C4). */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include "world_map_helper_94154.h"

static u32 h94154_lw(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 h94154_sra(u32 bits, unsigned shift)
{
    u32 shifted = bits >> shift;
    if ((bits & 0x80000000u) != 0u)
        shifted |= ~(UINT32_MAX >> shift);
    return shifted;
}

static u32 h94154_abs(u32 bits)
{
    return (bits & 0x80000000u) != 0u ? 0u - bits : bits;
}

s32 wm_80094154(u32 left, u32 right)
{
    u32 dx = h94154_abs(h94154_lw(left) - h94154_lw(right));
#if defined(W34N59_MUTANT_DISTANCE_USE_Y)
    u32 dz = h94154_abs(h94154_lw(left + 4u) - h94154_lw(right + 4u));
#else
    u32 dz = h94154_abs(h94154_lw(left + 8u) - h94154_lw(right + 8u));
#endif
    u32 scaled_x = h94154_sra(dx, 12u);
    u32 scaled_z = h94154_sra(dz, 12u);
    u32 sum = scaled_x * scaled_x + scaled_z * scaled_z;

    return (s32)SquareRoot0((int)sum);
}
