/* battle mainasm_18B1C: the retail-asm run at file offset 0x18B1C, in retail order.
 * Functions still listed as INCLUDE_ASM are not byte-matching C yet. The whole
 * file is retail-build only; any port body for these functions lives in the
 * port's own TU. */
#include "common.h"
#ifndef XENO_PC_PORT
#include "system/memory.h"
#endif

#ifndef XENO_BATTLE_OVERLAY_HOST_BODIES
extern s8 D_800C207C;
extern s32 D_800C2080;
extern s32 D_800C2084;
extern s32 D_800C3A7C;
extern s32 D_800C3A80;
extern s32 D_800C3A84;
extern s32 D_800C3A88;
extern s32 D_800C3A8C;
extern s32 D_800C3A90;
extern s8 D_800C3A94;
extern s8 D_800C3A98;
extern s32 D_800C3A9C;
#endif
extern s32 func_8001BD40(s32, s32);

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/mainasm_18B1C", func_8008860C);
#endif

void func_8008887C(s32 arg0, s32 arg1, s32 arg2, s32 arg3) {
    s32 var_a0;
    s32 var_a1;

    D_800C3A7C = arg0;
    D_800C3A80 = arg1;
    D_800C3A84 = arg2;
    D_800C3A88 = arg3;
    if ((arg2 != arg0) && (arg3 != arg1)) {
        if (arg2 < arg0) {
            D_800C3A94 = 1;
            var_a0 = arg0 - arg2;
        } else {
            var_a0 = arg2 - arg0;
            D_800C3A94 = 0;
        }
        if (arg3 < arg1) {
            D_800C3A98 = 1;
            var_a1 = arg1 - arg3;
        } else {
            var_a1 = arg3 - arg1;
            D_800C3A98 = 0;
        }
        if (var_a0 >= var_a1) {
            D_800C3A8C = 0x100;
            D_800C3A90 = (s32) (var_a1 << 8) / var_a0;
        } else {
            D_800C3A90 = 0x100;
            D_800C3A8C = (s32) (var_a0 << 8) / var_a1;
        }
        D_800C2080 = 0;
        D_800C2084 = 0;
        D_800C3A9C = func_8001BD40(1, 8) & 0xFF;
        D_800C207C = 0;
    }
}

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/mainasm_18B1C", func_80088990);
#endif
