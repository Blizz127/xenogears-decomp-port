#include "common.h"


#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/main45", func_8008AC88);
INCLUDE_ASM("asm/battle/nonmatchings/main45", func_8008ADD0);
#endif


#ifndef XENO_PC_PORT
extern u8* D_800D2D28;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800C3EAC;
#endif
extern u32 func_80089C08(u32 v);
extern void func_800BC404(u32 v);
extern void func_800BCD98(u32 v);


/* func_8008B108.s */
void func_8008B108(u8 flag) {
    u8* p;
    u8* q;
    u8* r;

    p = D_800D2D28;
    p[0x9E] = 0;
    p[0x9D] = 0;
    p[0x9C] = 0;
    q = D_800D2D28;
    q[0xB3] = 0;
    q[0xB2] = 0;
    q[0xB1] = 0;
    q[0xB0] = 0;
    r = D_800D2D28;
    r[0xB7] = 0;
    if (flag == 0) {
        D_800D2D28[0xCB] = 1;
    }
}
/* func_8008B168.s */
void func_8008B168(u8 index) {
    u8* p;
    u8* q;
    u8* r;
    u32 v;
    u32 off;

    p = D_800D2D28;
    p[0x9E] = 1;
    p[0x9D] = 1;
    p[0x9C] = 1;
    q = D_800D2D28;
    q[0xB3] = 1;
    q[0xB2] = 1;
    q[0xB1] = 1;
    q[0xB0] = 1;
    r = D_800D2D28;
    r[0xB7] = 1;
    v = func_80089C08(index & 0xFF);
    off = (u32)index << 6;
    v |= func_80089C08(*(u8*)(D_800C3EAC + off + 0x3C));
    func_800BC404(v & 0xFFFF);
    func_800BCD98(func_80089C08(*(u8*)(D_800C3EAC + off + 0x3C)) & 0xFFFF);
}
