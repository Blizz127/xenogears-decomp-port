#include "common.h"


#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/main13", func_80079948);
INCLUDE_ASM("asm/battle/nonmatchings/main13", func_800799C8);
INCLUDE_ASM("asm/battle/nonmatchings/main13", func_80079AB0);
INCLUDE_ASM("asm/battle/nonmatchings/main13", func_80079C24);
#endif


#ifndef XENO_PC_PORT
extern u8* D_800D2D28;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D3725[];
#endif
extern u32 func_80089C9C(u32 a, u32 b);


/* func_80079E18.s */
void func_80079E18(u8 index) {
    D_800D2D28[0xB4] = 1;
    *(u8*)(D_800D3725 + (((u32)index * 3) << 5)) = 1;
}
/* func_80079E4C.s */
void func_80079E4C(u8 index) {
    D_800D2D28[0xB4] = 0;
    *(u8*)(D_800D3725 + (((u32)index * 3) << 5)) = 0;
}
/* func_80079E7C.s */
u32 func_80079E7C(u16 v) {
    s32 i;

    for (i = 0; i < 0xB; i++) {
        if ((func_80089C9C(v & 0xFFFF, i & 0xFF) & 0xFFFF) != 0) {
            break;
        }
    }
    return i & 0xFF;
}
