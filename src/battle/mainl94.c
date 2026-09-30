#include "common.h"


typedef struct { long vx, vy; long vz, pad; } VECTOR;
extern VECTOR* Square0(VECTOR* v0, VECTOR* v1);
extern long SquareRoot0(long a);
extern s32 ratan2(s32 y, s32 x);


/* func_800B6CEC.s */
void func_800B6CEC(u8* s) {
    VECTOR v;
    VECTOR r;
    s32 len;

    v.vx = *(s32*)(s + 0xC) >> 8;
    v.vy = *(s32*)(s + 0x10) >> 8;
    v.vz = *(s32*)(s + 0x14) >> 8;
    if (v.vz == 0) {
        v.vz = 4;
    }
    Square0(&v, &r);
    len = SquareRoot0(r.vx + r.vz);
    *(s16*)(*(u32*)(s + 0x20) + 2) = (s16)(-ratan2(v.vz, v.vx));
    *(s16*)(*(u32*)(s + 0x20) + 4) = (s16)ratan2(v.vy, len);
    *(s16*)(*(u32*)(s + 0x20) + 0) = 0;
    *(u32*)(s + 0x3C) |= 0x10000000;
}
/* func_800B6DC0.s */
void func_800B6DC0(u8* s) {
    u32 i;

    if (*(u32*)(s + 0x20) == 0) {
        return;
    }
    if (*(u32*)(*(u32*)(s + 0x20) + 0x34) == 0) {
        return;
    }
    for (i = 0; i != 8; i++) {
        *(u8*)((s32)(i << 3) + (s32)(*(u32*)(*(u32*)(s + 0x20) + 0x34)) + 0) = 0;
        *(u8*)((s32)(i << 3) + (s32)(*(u32*)(*(u32*)(s + 0x20) + 0x34)) + 1) = 0;
        *(u16*)((s32)(i << 3) + (s32)(*(u32*)(*(u32*)(s + 0x20) + 0x34)) + 2) = 0;
        *(u16*)((s32)(i << 3) + (s32)(*(u32*)(*(u32*)(s + 0x20) + 0x34)) + 4) = 0;
        *(u16*)((s32)(i << 3) + (s32)(*(u32*)(*(u32*)(s + 0x20) + 0x34)) + 6) = 0;
    }
    *(u8*)(*(u32*)(s + 0x20) + 0x3C) = 0;
    *(u8*)(*(u32*)(s + 0x20) + 0x3D) = 0;
}
