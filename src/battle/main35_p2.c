/* Retail TU part 2 of main35.c (see the BATTLE_TU_PART note there): the
 * still-assembly run below, then that file's part-2 C bodies. */
#define BATTLE_TU_PART 2
#include "common.h"
#ifndef XENO_PC_PORT
u32 func_80084854(u32 arg0, u32 arg1);
u16 func_80089C08(u8);
void func_80093B08(s32);
void func_800BC404(u32);
void func_800BCD98(u32);
extern s32 D_800C3E24;
extern struct BattleCommandContext * D_800C3EAC;
extern u8 D_800D2C34;
void func_8008189C(s32 arg0);
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/main35_p2", func_80081318);
INCLUDE_ASM("asm/battle/nonmatchings/main35_p2", func_80081504);
INCLUDE_ASM("asm/battle/nonmatchings/main35_p2", func_800816F8);

#ifndef XENO_PC_PORT
void func_8008189C(s32 arg0) {
    s32 var_s0;

    if (*(u8 *)((s8*)((*(void **)&D_800C3EAC)) + 0x2E9) == 0) {
        var_s0 = 0;
        func_800BC404(func_80089C08(*(u8 *)((s8*)((*(void **)&D_800C3EAC)) + 0x2E8)) & 0xFFFF);
        func_800BCD98(func_80089C08(*(u8 *)((s8*)((*(void **)&D_800C3EAC)) + 0x2E8)) & 0xFFFF);
        do {
            if ((func_80084854(*(u8 *)((s8*)((*(void **)&D_800C3EAC)) + 0x2E8), var_s0 & 0xFF) & 0xFF) != *(u8 *)((s8*)((*(void **)&D_800C3EAC)) + 0x2E8)) {
                *(s8 *)((s8*)((D_800C3E24 + var_s0)) + 0xE6) = 1;
            } else {
                *(s8 *)((s8*)((D_800C3E24 + var_s0)) + 0xE6) = 0;
            }
            var_s0 += 1;
        } while (var_s0 < 4);
        if (D_800D2C34 != 4) {
            func_80093B08(arg0 & 0xFF);
        }
    }
}
#endif

INCLUDE_ASM("asm/battle/nonmatchings/main35_p2", func_800819A4);
INCLUDE_ASM("asm/battle/nonmatchings/main35_p2", func_80081B58);
INCLUDE_ASM("asm/battle/nonmatchings/main35_p2", func_800820A4);
INCLUDE_ASM("asm/battle/nonmatchings/main35_p2", func_800822C4);
INCLUDE_ASM("asm/battle/nonmatchings/main35_p2", func_80082504);
INCLUDE_ASM("asm/battle/nonmatchings/main35_p2", func_800826CC);
INCLUDE_ASM("asm/battle/nonmatchings/main35_p2", func_80082820);
INCLUDE_ASM("asm/battle/nonmatchings/main35_p2", func_800829F4);
INCLUDE_ASM("asm/battle/nonmatchings/main35_p2", func_80082BB0);
INCLUDE_ASM("asm/battle/nonmatchings/main35_p2", func_80082D4C);
INCLUDE_ASM("asm/battle/nonmatchings/main35_p2", func_80082F7C);
INCLUDE_ASM("asm/battle/nonmatchings/main35_p2", func_800830A8);
INCLUDE_ASM("asm/battle/nonmatchings/main35_p2", func_80083340);
INCLUDE_ASM("asm/battle/nonmatchings/main35_p2", func_80083580);
INCLUDE_ASM("asm/battle/nonmatchings/main35_p2", func_80083748);
INCLUDE_ASM("asm/battle/nonmatchings/main35_p2", func_80083948);
#endif
#include "main35.c"
