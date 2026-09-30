#include "common.h"


#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/main30", func_8007E7E4);
#endif


#ifndef XENO_PC_PORT
extern u8 D_800D3420[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D3430[];
#endif
#ifndef XENO_PC_PORT
extern u32 D_800D3410[];
#endif
#ifndef XENO_PC_PORT
extern u16 D_800CCD64[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D301C[];
#endif
#ifndef XENO_PC_PORT
extern u16 D_800D39DC;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C3EB7[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2DC0;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800D3364;
#endif
extern s32 func_80079E7C(u32 value);
extern void func_80078508(void* pBuffer);


/* func_8007E8AC.s */
void func_8007E8AC(u8** ppBoard) {
    u8* p = *ppBoard;
    u32 i2 = p[2];
    u32 i1 = p[1];
    u32 off = (i2 << 3) + (i1 << 6);
    u8* pBase = D_800D3364;

    *(u8*)(pBase + off + 0x140) = p[3];
}
/* func_8007E8E0.s */
void func_8007E8E0(u8** ppBoard, u8 index) {
    u8* p = *ppBoard;
    u32 i1 = p[1];
    u32 row = (u32)D_800D3420 + ((u32)index << 6);

    D_800D2DC0 = (u8)(func_80079E7C(*(u16*)((i1 << 1) + row)) + 1);
}
/* func_8007E934.s */
void func_8007E934(void) {
    u8 buffer[0x10];

    func_80078508(buffer);
}
/* func_8007E954.s */
u32 func_8007E954(u8** ppBoard, u8 index) {
    u8* p = *ppBoard;
    u8* pRow = D_800D3430 + ((u32)index << 6);

    return (pRow[p[1]] ^ p[2]) == 0;
}
/* func_8007E98C.s */
u32 func_8007E98C(u8** ppBoard, u8 index) {
    u8* p = *ppBoard;
    u32 i3 = p[3];
    u32 i1 = p[1];
    u32 i2 = p[2];
    u32 i3s = i3 << 8;
    u32 row = (u32)D_800D3420 + ((u32)index << 6);

    return (*(u16*)((i1 << 1) + row) ^ (i2 | i3s)) == 0;
}
/* func_8007E9D0.s */
u32 func_8007E9D0(u8** ppBoard, u8 index) {
    u8* p = *ppBoard;
    u8* pRow = D_800D3430 + ((u32)index << 6);

    return (u32)p[2] >= pRow[p[1]];
}
/* func_8007EA08.s */
u32 func_8007EA08(u8** ppBoard, u8 index) {
    u8* p = *ppBoard;
    u32 i3 = p[3];
    u32 i1 = p[1];
    u32 i2 = p[2];
    u32 i3s = i3 << 8;
    u32 row = (u32)D_800D3420 + ((u32)index << 6);
    u32 a = *(u16*)((i1 << 1) + row);

    return (i2 | i3s) >= a;
}
/* func_8007EA4C.s */
u32 func_8007EA4C(u8** ppBoard, u8 index) {
    u8* p = *ppBoard;
    u8* pRow = D_800D3430 + ((u32)index << 6);

    return pRow[p[1]] >= (u32)p[2];
}
/* func_8007EA84.s */
u32 func_8007EA84(u8** ppBoard, u8 index) {
    u8* p = *ppBoard;
    u32 i3 = p[3];
    u32 i1 = p[1];
    u32 i2 = p[2];
    u32 i3s = i3 << 8;
    u32 row = (u32)D_800D3420 + ((u32)index << 6);
    u32 a = *(u16*)((i1 << 1) + row);

    return a >= (i2 | i3s);
}
/* func_8007EAC8.s */
u32 func_8007EAC8(u8** ppBoard, u8 index) {
    u8* p = *ppBoard;
    u8* pRow = D_800D3430 + ((u32)index << 6);

    return (pRow[p[1]] ^ pRow[p[2]]) == 0;
}
/* func_8007EB08.s */
u32 func_8007EB08(u8** ppBoard, u8 index) {
    u8* p = *ppBoard;
    u32 i2 = p[2];
    u32 i1 = p[1];
    u32 row = (u32)D_800D3420 + ((u32)index << 6);
    u32 a = *(u16*)((i1 << 1) + row);
    u32 b = *(u16*)((i2 << 1) + row);

    return (a ^ b) == 0;
}
/* func_8007EB50.s */
u32 func_8007EB50(u8** ppBoard, u8 index) {
    u8* pRow = D_800D3430 + ((u32)index << 6);
    u8* p = *ppBoard;
    u32 a = pRow[p[1]];
    u32 b = pRow[p[2]];

    return b >= a;
}
/* func_8007EB90.s */
u32 func_8007EB90(u8** ppBoard, u8 index) {
    u8* p = *ppBoard;
    u32 i2 = p[2];
    u32 i1 = p[1];
    u32 row = (u32)D_800D3420 + ((u32)index << 6);
    u32 a = *(u16*)((i1 << 1) + row);
    u32 b = *(u16*)((i2 << 1) + row);

    return b >= a;
}
/* func_8007EBD8.s */
u32 func_8007EBD8(u8** ppBoard, u8 index) {
    u8* pRow = D_800D3430 + ((u32)index << 6);
    u8* p = *ppBoard;
    u32 a = pRow[p[1]];
    u32 b = p[2];

    return (a & b) != 0;
}
/* func_8007EC10.s */
u32 func_8007EC10(u8** ppBoard, u8 index) {
    u8* p = *ppBoard;
    u32 i3 = p[3];
    u32 i1 = p[1];
    u32 i2 = p[2];
    u32 i3s = i3 << 8;
    u32 row = (u32)D_800D3420 + ((u32)index << 6);
    u32 a = *(u16*)((i1 << 1) + row);

    return (a & (i2 + i3s)) != 0;
}
/* func_8007EC54.s */
u32 func_8007EC54(u8** ppBoard, u8 index) {
    u8* pRow = D_800D3430 + ((u32)index << 6);
    u8* p = *ppBoard;
    u32 a = pRow[p[1]];
    u32 b = pRow[p[2]];

    return (a & b) != 0;
}
/* func_8007EC94.s */
u32 func_8007EC94(u8** ppBoard, u8 index) {
    u8* p = *ppBoard;
    u32 i2 = p[2];
    u32 i1 = p[1];
    u32 row = (u32)D_800D3420 + ((u32)index << 6);
    u32 a = *(u16*)((i1 << 1) + row);
    u32 b = *(u16*)((i2 << 1) + row);

    return (a & b) != 0;
}
/* func_8007ECDC.s */
u32 func_8007ECDC(u8** ppBoard, u8 index) {
    u8* p = *ppBoard;
    u8* pRow = D_800D3430 + ((u32)index << 6);

    return (pRow[p[1]] ^ p[2]) != 0;
}
/* func_8007ED14.s */
u32 func_8007ED14(u8** ppBoard, u8 index) {
    u8* p = *ppBoard;
    u32 i3 = p[3];
    u32 i1 = p[1];
    u32 i2 = p[2];
    u32 i3s = i3 << 8;
    u32 row = (u32)D_800D3420 + ((u32)index << 6);
    u32 a = *(u16*)((i1 << 1) + row);

    return (a ^ (i2 | i3s)) != 0;
}
/* func_8007ED58.s */
u32 func_8007ED58(u8** ppBoard, u8 index) {
    u8* p = *ppBoard;
    u8* pRow = D_800D3430 + ((u32)index << 6);

    return (pRow[p[1]] ^ pRow[p[2]]) != 0;
}
/* func_8007ED98.s */
u32 func_8007ED98(u8** ppBoard, u8 index) {
    u8* p = *ppBoard;
    u32 i2 = p[2];
    u32 i1 = p[1];
    u32 row = (u32)D_800D3420 + ((u32)index << 6);
    u32 a = *(u16*)((i1 << 1) + row);
    u32 b = *(u16*)((i2 << 1) + row);

    return (a ^ b) != 0;
}
/* func_8007EDE0.s */
u32 func_8007EDE0(u8** ppBoard, u8 index) {
    u8* p = *ppBoard;
    u32 i2 = p[2];
    u32 i1 = p[1];
    u32 row = (u32)D_800D3410 + ((u32)index << 6);
    u32 a = *(u32*)((i1 << 2) + row);
    u32 b = *(u32*)((i2 << 2) + row);

    return (a ^ b) == 0;
}
/* func_8007EE28.s */
u32 func_8007EE28(u8** ppBoard, u8 index) {
    u8* p = *ppBoard;
    u32 i2 = p[2];
    u32 i1 = p[1];
    u32 row = (u32)D_800D3410 + ((u32)index << 6);
    u32 a = *(u32*)((i1 << 2) + row);
    u32 b = *(u32*)((i2 << 2) + row);

    return b >= a;
}
/* func_8007EE70.s */
u32 func_8007EE70(u8** ppBoard) {
    u8* p = *ppBoard;
    u8* pEntry = (u8*)D_800CCD64 + p[1] * 0x170;

    return (u32)*(u16*)pEntry >> 15;
}
/* func_8007EEA8.s */
u32 func_8007EEA8(u8** ppBoard) {
    u8* p = *ppBoard;

    return *(u8*)((u8*)D_800D301C + (p[1] << 2)) == 0;
}
/* func_8007EED0.s */
u32 func_8007EED0(void) {
    return (D_800D39DC & 3) == 0;
}
/* func_8007EEE8.s */
u32 func_8007EEE8(void) {
    u32 result = 1;
    u32 active = D_800D39DC >> 5;
    int i;

    for (i = 0; i < 8; i++) {
        u32 offset = 0x54 + i * 0x1C;
        if (active != 0 && (D_800C3EB7[offset] & 0x80) == 0) {
            result = 0;
            break;
        }
    }
    return result;
}
/* func_8007EF44.s */
u32 func_8007EF44(u8 index) {
    u32 offset = (index + 3) * 0x1C;

    return D_800C3EB7[offset] >> 7;
}
