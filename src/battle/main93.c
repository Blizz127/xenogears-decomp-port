#include "common.h"


extern s32 ratan2(s32 y, s32 x);


/* func_800B6C44.s */
void func_800B6C44(u8* s) {
    s16 v = (s16)ratan2(*(s32*)(s + 0x10) >> 8, *(s32*)(s + 0xC) >> 8);
    s16* p;

    p = (s16*)(*(u32*)(s + 0x20));
    *(s16*)((u8*)p + 4) = v;
    *(u32*)(s + 0x3C) |= 0x10000000;
}
/* func_800B6C98.s */
void func_800B6C98(u8* s) {
    s16 v = (s16)ratan2(*(s32*)(s + 0x14) >> 8, *(s32*)(s + 0xC) >> 8);
    s16* p;

    p = (s16*)(*(u32*)(s + 0x20));
    *(s16*)((u8*)p + 0) = v;
    *(u32*)(s + 0x3C) |= 0x10000000;
}
