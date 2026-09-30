/* Retail TU part 2 of main73.c (see the BATTLE_TU_PART note there): the
 * still-assembly run below, then that file's part-2 C bodies. */
#define BATTLE_TU_PART 2
#include "common.h"
#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/main73_p2", func_800AF438);
INCLUDE_ASM("asm/battle/nonmatchings/main73_p2", func_800AF518);
INCLUDE_ASM("asm/battle/nonmatchings/main73_p2", func_800AF678);
INCLUDE_ASM("asm/battle/nonmatchings/main73_p2", func_800AFA98);
INCLUDE_ASM("asm/battle/nonmatchings/main73_p2", func_800AFB4C);
INCLUDE_ASM("asm/battle/nonmatchings/main73_p2", func_800AFC68);
INCLUDE_ASM("asm/battle/nonmatchings/main73_p2", func_800AFD98);
INCLUDE_ASM("asm/battle/nonmatchings/main73_p2", func_800AFF9C);
#endif
/* func_800B0060.s */
extern void func_8003852C(u32 p);
extern void HeapFree(u32 p);
void func_800B0060(u8* p) {
    if (*(u32*)(p + 0xC) != 0) {
        if (p[0x63] != 0) {
            func_8003852C(*(u32*)(*(u32*)(p + 0xB4) + 8));
            p[0x63] = 0;
        }
        HeapFree(*(u32*)(p + 0xC));
        *(u32*)(p + 0xC) = 0;
        *(u32*)(p + 0x18) = 0;
    }
}
#include "main73.c"
