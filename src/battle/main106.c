#include "common.h"


#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/main106", func_800B8DA4);
INCLUDE_ASM("asm/battle/nonmatchings/main106", func_800B8EBC);
#endif


extern void func_800245D8(u32 p, s32 v);
extern void func_80021BF8(u32 p, u32 v);


/* func_800B9020.s */
void func_800B9020(u8* p) {
    func_800245D8((u32)p, *(s8*)(p + 0xB0));
    func_80021BF8((u32)p, 0);
}
