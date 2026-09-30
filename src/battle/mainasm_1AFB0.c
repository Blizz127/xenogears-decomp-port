/* battle mainasm_1AFB0: the retail run at file offset 0x1AFB0, in retail order.
 * Functions still listed as INCLUDE_ASM are not byte-matching C yet. */
#include "common.h"

#ifndef XENO_BATTLE_OVERLAY_HOST_BODIES
extern u8 D_800C3CF4[];
#endif

/* Split `value` into nine decimal digits (most significant first) and blank
 * the leading zeros with 0xFF, keeping at least the last digit. */
void func_8008AAA0(u32 value) {
    s32 i;
    u32 div = 100000000;

    for (i = 0; i < 9; i++) {
        D_800C3CF4[i] = value / div;
        value %= div;
        div /= 10;
    }
    for (i = 1; i < 9; i++) {
        if (D_800C3CF4[i] != 0) {
            if (D_800C3CF4[i - 1] == 0) {
                D_800C3CF4[i - 1] = 0xFF;
            }
            break;
        }
        D_800C3CF4[i - 1] = 0xFF;
    }
}
