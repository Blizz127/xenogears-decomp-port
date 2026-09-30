#include "common.h"


#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/main47", func_8008BD50);
INCLUDE_ASM("asm/battle/nonmatchings/main47", func_8008BED8);
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
extern void func_8008FA60(u32 v);
extern void func_800716D8(void);
extern void func_8007765C(void);
extern void func_80077980(void);


/* func_8008C360.s */
void func_8008C360(u8 a) {
    u8* p;
    u8* q;

    p = D_800D2D28;
    p[0x9E] = 0;
    p[0x9D] = 0;
    p[0x9C] = 0;
    q = D_800D2D28;
    q[0xB7] = 0;
    if (a != 0) {
        D_800D2D28[0xC6] = 0;
        func_8008FA60(0);
        func_8008FA60(1);
        func_800716D8();
        func_8007765C();
        func_80077980();
    } else {
        q = D_800D2D28;
        q[0xB1] = 0;
        q[0xB0] = 0;
    }
}
/* func_8008C3F0.s */
void func_8008C3F0(u8 index) {
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
    q[0xB7] = 3;
    r = D_800D2D28;
    r[0xB1] = 1;
    r[0xB0] = 1;
    v = func_80089C08(index & 0xFF);
    off = (u32)index << 6;
    v |= func_80089C08(*(u8*)(D_800C3EAC + off + 0x3C));
    func_800BC404(v & 0xFFFF);
    func_800BCD98(func_80089C08(*(u8*)(D_800C3EAC + off + 0x3C)) & 0xFFFF);
}
