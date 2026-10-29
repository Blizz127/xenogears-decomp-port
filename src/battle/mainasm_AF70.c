/* battle mainasm_AF70: the retail-asm run at file offset 0xAF70, in retail order.
 * Functions still listed as INCLUDE_ASM are not byte-matching C yet. The whole
 * file is retail-build only; any port body for these functions lives in the
 * port's own TU. */
#include "common.h"

#ifndef XENO_PC_PORT
extern u8 D_800D3430[];

/* Board op: multiply table cell p[1] of row `index` (D_800D3430, 64-byte
 * rows) by operand p[2], saturating at 0xFF; the threshold test truncates
 * the product to a signed halfword first.  The cell is the first multiply
 * operand and the stored value goes back through `prod` (found by a
 * permuter run; the operand swap alone is 9/22 off). */
void func_8007AA60(u8** ppBoard, u8 index) {
    u8* p = *ppBoard;
    u8* row = D_800D3430 + ((u32)index << 6);
    u8* q = row + p[1];
    s32 prod = *q * p[2];
    u8 value = prod;

    if ((s32)(s16)prod >= 0x100) {
        value = 0xFF;
    }
    *q = prod = value;
}
#endif
