#include "common.h"


#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/main50", func_8008FAD8);
INCLUDE_ASM("asm/battle/nonmatchings/main50", func_8008FC1C);
#endif


extern void func_8008FC1C(u32 a0, u32 a1, u32 a2, u32 a3, u32 a4);


/* func_8008FDE4.s */
void func_8008FDE4(void) {
    func_8008FC1C(0x20, 0x5C, 0xCC, 0x60, 0xE);
}
