#include "common.h"


#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/main26", func_8007D1DC);
#endif


/* func_8007D30C.s */
#ifndef XENO_PC_PORT
extern u8 D_800D3410[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2C8B[];
#endif
void func_8007D30C(u8** ppBoard, u8 index) {
    u8* p = *ppBoard;
    u32 row = (u32)D_800D3410 + ((u32)index << 6);

    *(u32*)((p[1] << 2) + row) = D_800D2C8B[index];
}
