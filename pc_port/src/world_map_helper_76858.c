/* Retail world-map fixed-point vector blend [0x80076858,0x80076954). */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_76858.h"

static s16 h76858_lh(u32 address)
{
    s16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void h76858_sw(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u32 h76858_mul_low(s32 left, s32 right)
{
    return (u32)((int64_t)left * (int64_t)right);
}

static s32 h76858_sra(u32 bits, unsigned shift)
{
    u32 shifted = bits >> shift;
    s32 result;

    if ((bits & 0x80000000u) != 0u)
        shifted |= ~(UINT32_MAX >> shift);
    memcpy(&result, &shifted, sizeof(result));
    return result;
}

void wm_80076858(s32 parameter, u32 vector_a, u32 vector_b,
                 u32 vector_c, u32 output)
{
    s32 inverse = (s32)(4096u - (u32)parameter);
    s32 weight_a;
    s32 weight_b;
    s32 weight_c;
    u32 axis;

    weight_a = h76858_sra(h76858_mul_low(inverse, inverse) << 3, 12u);
#if defined(W34N59_MUTANT_DROP_MIDDLE_BIAS)
    weight_b = h76858_sra(h76858_mul_low(inverse, parameter), 8u);
#else
    weight_b = (s32)((u32)h76858_sra(
        h76858_mul_low(inverse, parameter), 8u) + 0x8000u);
#endif
    weight_c = h76858_sra(h76858_mul_low(parameter, parameter) << 3, 12u);

    for (axis = 0u; axis < 3u; axis++) {
        u32 offset = axis * 2u;
        u32 value = h76858_mul_low((s32)h76858_lh(vector_a + offset),
                                   weight_a);
        value += h76858_mul_low((s32)h76858_lh(vector_b + offset),
                                weight_b);
        value += h76858_mul_low((s32)h76858_lh(vector_c + offset),
                                weight_c);
        h76858_sw(output + axis * 4u, value);
    }
}
