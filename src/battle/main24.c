#include "common.h"


#ifndef XENO_PC_PORT
extern u8 D_800D3430[];
extern u8 func_8001BD40(u8 min, u8 max);

void func_8007BC84(u8 **pp, s32 idx) {
    u8 v = func_8001BD40(0, (*pp)[2]);
    u32 row = (u32)D_800D3430 + ((idx & 0xFF) << 6);

    *(u8 *)(row + (*pp)[1]) = v;
}

INCLUDE_ASM("asm/battle/nonmatchings/main24", func_8007BCE8);
INCLUDE_ASM("asm/battle/nonmatchings/main24", func_8007BD5C);
INCLUDE_ASM("asm/battle/nonmatchings/main24", func_8007BEA8);
INCLUDE_ASM("asm/battle/nonmatchings/main24", func_8007C040);
INCLUDE_ASM("asm/battle/nonmatchings/main24", func_8007C1A4);
INCLUDE_ASM("asm/battle/nonmatchings/main24", func_8007C33C);
#endif


#ifndef XENO_PC_PORT
extern u8 D_800D2E06[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2E0C[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800CCD34[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D3420[];
#endif
extern u32 func_8007A628(u32 a0, u32 a1);
extern u32 func_80089C08(u8 idx);


/* func_8007C4A0.s */
void func_8007C4A0(u8** ppBoard, u8 index) {
    u32 best = 0xFF;
    s32 bestIndex = 0;
    s32 i = 0;
    u8* s1 = D_800D2E06;

    for (; i < 3; i++, s1 += 2) {
        if ((func_8007A628((u32)i & 0xFF, (*ppBoard)[2]) & 0xFF) != 0) {
            s16 v = *(s16*)s1;

            if ((s32)best >= (s32)v) {
                best = *s1;
                bestIndex = i;
            }
        }
    }
    {
        u16 r = (u16)func_80089C08((u8)(bestIndex & 0xFF));
        u32 rowOff = ((u32)index & 0xFF) << 6;
        u8* p = *ppBoard;
        u16* row = (u16*)(D_800D3420 + rowOff);

        row[p[1]] = r;
    }
}
/* func_8007C580.s */
void func_8007C580(u8** ppBoard, u8 index) {
    u32 best = 0xFF;
    s32 bestIndex = 0;
    s32 i = 3;
    s32 skip = (index & 0xFF) + 3;
    u8* s1 = D_800D2E0C;

    for (; i < 0xB; i++, s1 += 2) {
        if ((func_8007A628((u32)i & 0xFF, (*ppBoard)[2]) & 0xFF) != 0) {
            s16 v = *(s16*)s1;

            if ((s32)best >= (s32)v && i != skip) {
                best = *s1;
                bestIndex = i;
            }
        }
    }
    {
        u16 r = (u16)func_80089C08((u8)(bestIndex & 0xFF));
        u32 rowOff = ((u32)index & 0xFF) << 6;
        u8* p = *ppBoard;
        u16* row = (u16*)(D_800D3420 + rowOff);

        row[p[1]] = r;
    }
}
/* func_8007C678.s */
void func_8007C678(u8** ppBoard, u8 index) {
    u32 best = 0xFFFF;
    s32 bestIndex = 0;
    s32 i;
    u32 off;

    for (i = 0, off = 0; i < 3; i++, off += 0x170) {
        if ((func_8007A628((u32)i & 0xFF, (*ppBoard)[2]) & 0xFF) != 0) {
            u32 v = *(u16*)(D_800CCD34 + off);

            if (best >= v) {
                best = v;
                bestIndex = i;
            }
        }
    }
    {
        u16 r = (u16)func_80089C08((u8)(bestIndex & 0xFF));
        u32 rowOff = ((u32)index & 0xFF) << 6;
        u8* p = *ppBoard;
        u16* row = (u16*)(D_800D3420 + rowOff);

        row[p[1]] = r;
    }
}
/* func_8007C75C.s */
void func_8007C75C(u8** ppBoard, u8 index) {
    u32 best = 0xFFFF;
    s32 bestIndex = 0;
    s32 i;
    u32 off;

    for (i = 3, off = 0x450; i < 0xB; i++, off += 0x170) {
        if ((func_8007A628((u32)i & 0xFF, (*ppBoard)[2]) & 0xFF) != 0) {
            u32 v = *(u16*)(D_800CCD34 + off);

            if (best >= v) {
                best = v;
                bestIndex = i;
            }
        }
    }
    {
        u16 r = (u16)func_80089C08((u8)(bestIndex & 0xFF));
        u32 rowOff = ((u32)index & 0xFF) << 6;
        u8* p = *ppBoard;
        u16* row = (u16*)(D_800D3420 + rowOff);

        row[p[1]] = r;
    }
}
