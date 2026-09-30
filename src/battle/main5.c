#include "common.h"


/* func_80076B00.s */
extern s32 D_800595A0;
void func_80076B00(u8* p) {
    func_80076AC8(p);
    p[4] = 0x80;
    p[5] = 0x80;
    p[6] = 0x80;
    if (D_800595A0 != 0)
        *(u16*)(p + 0x16) |= 0x40;
    else
        *(u16*)(p + 0x16) &= ~0x40;
}


extern void func_80076AC8(u8* p);


/* func_80076B68.s */
void func_80076B68(u8* p) {
    func_80076AC8(p);
    p[4] = 0x80;
    p[5] = 0x80;
    p[6] = 0x80;
    *(u16*)(p + 0x16) |= 0x20;
}
/* func_80076BAC.s */
void func_80076BAC(u8* p) {
    func_80076AC8(p);
    p[4] = 0x40;
    p[5] = 0x40;
    p[6] = 0x40;
    *(u16*)(p + 0x16) |= 0x20;
}
/* func_80076BF0.s */
void func_80076BF0(u8* p) {
    func_80076AC8(p);
    p[4] = 0x80;
    p[5] = 0x80;
    p[6] = 0x80;
    *(u16*)(p + 0x16) |= 0x40;
}
/* func_80076C34.s */
void func_80076C34(u8* p) {
    func_80076AC8(p);
    p[4] = 0x40;
    p[5] = 0x40;
    p[6] = 0x40;
    *(u16*)(p + 0x16) |= 0x40;
}
