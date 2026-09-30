/* battle mainasm_16244: the retail-asm run at file offset 0x16244, in retail order.
 * Functions still listed as INCLUDE_ASM are not byte-matching C yet. The whole
 * file is retail-build only; any port body for these functions lives in the
 * port's own TU. */
#include "common.h"

#ifndef XENO_PC_PORT
extern u32 func_80076A10(u32 sel, u8* p, s32 m, s32 n);
extern u8* D_800C3EA4;
extern u8* D_800C3EAC;
extern u8 D_800CCB34;
extern u8* D_800D2D28;

/* Lay out three sprites at x 0x2A/0x32/0x3A, y 0xD0 (the +0x2D4 digit, 0x19
 * and the +0x2D5 digit), counting them in +0x7B. */
void func_80085D34(void) {
    D_800D2D28[0x7B] = 0;
    D_800D2D28[0x7B] += func_80076A10(D_800C3EAC[0x2D4] + 0xF, D_800C3EA4 + 0x9C8 + D_800D2D28[0x7B] * 0x50, 0x2A, 0xD0);
    D_800D2D28[0x7B] += func_80076A10(0x19, D_800C3EA4 + 0x9C8 + D_800D2D28[0x7B] * 0x50, 0x32, 0xD0);
    D_800D2D28[0x7B] += func_80076A10(D_800C3EAC[0x2D5] + 0xF, D_800C3EA4 + 0x9C8 + D_800D2D28[0x7B] * 0x50, 0x3A, 0xD0);
    D_800D2D28[0xA4] = D_800CCB34;
}

#endif
