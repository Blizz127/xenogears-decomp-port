#include "common.h"


#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/main96", func_800B7160);
#endif


extern void DrawSync(s32 mode);
extern void HeapFree(u32 p);


/* func_800B7330.s */
void func_800B7330(u8* p) {
    DrawSync(0);
    HeapFree((u32)p);
}
/* func_800B7364.s */
void func_800B7364(u32 p) {
    WorkListRemoveTask(p + 0x1C);
    TimerWorkListRemoveTask(p);
    func_80025180(p);
}
