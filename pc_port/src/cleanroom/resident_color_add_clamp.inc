#include <stdint.h>

int func_80021AD8(s32 first, s32 second)
{
    uint32_t sum = (uint32_t)first + (uint32_t)second;
    if (sum > INT32_MAX)
        return 0;
    return sum > 255u ? 255 : (int)sum;
}
