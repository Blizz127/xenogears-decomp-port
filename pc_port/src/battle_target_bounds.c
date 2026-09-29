#include "battle_target_bounds.h"
#include <stddef.h>

static int64_t signed_word(uint32_t value) {
    return value<0x80000000u?(int64_t)value:(int64_t)value-INT64_C(4294967296);
}
static uint32_t shift_right(uint32_t value,unsigned shift) {
    return (value>>shift)|((0u-(value>>31))<<(32-shift));
}
static int32_t signed_half(uint16_t value) {
    return value<0x8000u?(int32_t)value:(int32_t)value-65536;
}
uint32_t PcPortBattleAccumulateTargetRadius(uint32_t maximum,
    uint16_t screen_x,uint16_t screen_y) {
    int32_t x=signed_half((uint16_t)(((uint32_t)screen_x-160u)<<2));
    int32_t y=signed_half((uint16_t)(((uint32_t)screen_y-164u)<<2));
    uint32_t sum=(uint32_t)x*(uint32_t)x+(uint32_t)y*(uint32_t)y;
    return signed_word(maximum)<signed_word(sum)?sum:maximum;
}
int PcPortBattleComputeTargetBounds(uint32_t mask,
    const PcPortBattleTargetPoint points[11], PcPortBattleTargetBounds *result) {
    if (!points || !result) return -1;
    uint32_t sums[3]={0}, low[3], high[3];
    result->count=0;
    for (unsigned axis=0;axis<3;axis++) result->center[axis]=0;
    for (unsigned i=0;i<11;i++) {
        if (!(mask&(1u<<i)) || points[i].suppressed || !points[i].position) continue;
        result->count++;
        for (unsigned axis=0;axis<3;axis++)
            sums[axis]+=shift_right((uint32_t)points[i].position[axis],1);
    }
    if (!result->count) return 0;
    for (unsigned axis=0;axis<3;axis++)
        low[axis]=high[axis]=(uint32_t)(signed_word(sums[axis])/result->count)<<1;
    for (unsigned i=0;i<11;i++) {
        if (!(mask&(1u<<i)) || points[i].suppressed || !points[i].position) continue;
        for (unsigned axis=0;axis<3;axis++) {
            uint32_t value=(uint32_t)points[i].position[axis];
            if (signed_word(value)<signed_word(low[axis])) low[axis]=value;
            if (signed_word(value)>signed_word(high[axis])) high[axis]=value;
        }
    }
    for (unsigned axis=0;axis<3;axis++) {
        uint32_t sum=low[axis]+high[axis];
        sum+=sum>>31;
        result->center[axis]=(int32_t)signed_word(shift_right(sum,17));
    }
    return 0;
}
