/* battle mainasm_B190: the retail-asm run at file offset 0xB190, in retail order.
 * Functions still listed as INCLUDE_ASM are not byte-matching C yet. The whole
 * file is retail-build only; any port body for these functions lives in the
 * port's own TU. */
#include "common.h"

#ifndef XENO_PC_PORT
extern u8 D_800D3420[];

/* func_8007AC80.s: multiply the row cell by the operand, saturating at 0xFFFF
 * (the mainc18 operator family's multiply; CDK compiler, BattleCdk).  The
 * clamped product goes back through c, the cell's value, before the store:
 * flow then sees c in the store block, so local-alloc leaves it to
 * global-alloc, the product gets no preference for c's $a0 and takes retail's
 * $v1.  Combine folds the copy into the store.  Storing v directly, or
 * chaining `row[p[1]] = c = v`, leaves the product in $a0. */
void func_8007AC80(u8** ppBoard, u8 index) {
    u32 off = (u32)index << 6;
    u8* base = D_800D3420;
    u8* p = *ppBoard;
    u16* row = (u16*)(base + off);
    s32 c = row[p[1]];
    s32 v = c * ((p[3] << 8) + p[2]);

    if (v > 0xFFFF) {
        v = 0xFFFF;
    }
    c = v;
    row[p[1]] = c;
}
#endif
