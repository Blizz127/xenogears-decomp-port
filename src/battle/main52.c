#include "common.h"


#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/main52", func_80097964);
#endif


#ifndef XENO_PC_PORT
extern u8* D_800C34B0;
#endif


/* func_80097D08.s */
void func_80097D08(void) {
    s16 i;

    for (i = 0xB; i != -1; i--) {
        *(u8*)(D_800C34B0 + i + 0x5FA0) = 0xFF;
        *(u32*)(((u32)i << 2) + (s32)(unsigned int)D_800C34B0 + 0x5F6C) = 0;
    }
}
