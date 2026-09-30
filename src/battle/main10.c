#include "common.h"


#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/main10", func_80078D6C);
INCLUDE_ASM("asm/battle/nonmatchings/main10", func_80078E24);
#endif


#ifndef XENO_PC_PORT
extern u8 D_800D2E5D[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2E60[];
#endif
extern void func_80079ED8(u8 a0, u8 a1, u8 a2, s32 a3);


/* func_80079054.s */
void func_80079054(u8 a, u8 idx) {
    func_80079ED8(a & 0xFF, *(u8*)(D_800D2E5D + ((u32)idx << 3)),
                  *(u8*)(D_800D2E60 + ((u32)idx << 3)), 0);
}
