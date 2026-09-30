#include "common.h"


#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/main49", func_8008CDE4);
INCLUDE_ASM("asm/battle/nonmatchings/main49", func_8008CFB8);
INCLUDE_ASM("asm/battle/nonmatchings/main49", func_8008D328);
INCLUDE_ASM("asm/battle/nonmatchings/main49", func_8008D598);
INCLUDE_ASM("asm/battle/nonmatchings/main49", func_8008DC34);
INCLUDE_ASM("asm/battle/nonmatchings/main49", func_8008DE04);
INCLUDE_ASM("asm/battle/nonmatchings/main49", func_8008E430);
INCLUDE_ASM("asm/battle/nonmatchings/main49", func_8008EA70);
INCLUDE_ASM("asm/battle/nonmatchings/main49", func_8008F0A8);
INCLUDE_ASM("asm/battle/nonmatchings/main49", func_8008F6E4);
INCLUDE_ASM("asm/battle/nonmatchings/main49", func_8008F8F4);
#endif


#ifndef XENO_PC_PORT
extern u8* D_800D2D28;
#endif
#ifndef XENO_PC_PORT
extern u32 D_800D2E38[];
#endif
#ifndef XENO_PC_PORT
extern u32 D_800D2D90[];
#endif
extern void func_800716D8(void);
extern void HeapFree(u32 p);


/* func_8008FA60.s */
void func_8008FA60(u8 index) {
    u32 i = (u32)index & 0xFF;
    u8* p = D_800D2D28;

    *(u8*)((int)(unsigned int)p + i + 0xB0) = 0;
    p = D_800D2D28;
    *(u8*)((int)(unsigned int)p + i + 0xB8) = 0;
    func_800716D8();
    HeapFree(D_800D2E38[i]);
    HeapFree(D_800D2D90[i]);
}
