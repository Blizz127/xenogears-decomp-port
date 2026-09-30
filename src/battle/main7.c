#include "common.h"


#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/main7", func_80077698);
#endif


#ifndef XENO_PC_PORT
extern u8* D_800D2D28;
#endif
#ifndef XENO_PC_PORT
extern u8* D_800D2DC8;
#endif


/* func_80077980.s */
void func_80077980(void) {
    D_800D2D28[0xC6] = 0;
}
