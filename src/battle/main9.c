#include "common.h"


#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/main9", func_80078998);
INCLUDE_ASM("asm/battle/nonmatchings/main9", func_80078B34);
#endif


#ifndef XENO_PC_PORT
extern u8* D_800C3EAC;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C402F[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2E60[];
#endif
#ifndef XENO_PC_PORT
extern u16 D_800D39E0;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2E62[];
#endif
extern void func_800785D4(u8 a, u8 b);


/* func_80078C9C.s */
void func_80078C9C(u8 a, u8 b) {
    u32 off = (u32)*(u8*)(D_800C3EAC + 0x2DA) * 72;

    *(u8*)(D_800C402F + off) = 0xFC;
    func_800785D4(a & 0xFF, b & 0xFF);
}
/* func_80078CEC.s */
void func_80078CEC(u8 a, u8 b) {
    u32 off = (u32)*(u8*)(D_800C3EAC + 0x2DA) * 72;

    *(u8*)(D_800C402F + off) = *(u8*)(D_800D2E60 + ((u32)b << 3));
    func_800785D4(a & 0xFF, b & 0xFF);
}
/* func_80078D48.s (the index arrives in a1: retail ignores a0) */
void func_80078D48(u32 unused, u8 index) {
    D_800D39E0 = *(u16*)(D_800D2E62 + ((u32)index << 3));
}
