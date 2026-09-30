/* battle mainasm_3F910: the retail-asm run at file offset 0x3F910, in retail order.
 * Functions still listed as INCLUDE_ASM are not byte-matching C yet. The whole
 * file is retail-build only; any port body for these functions lives in the
 * port's own TU. */
#include "common.h"

#ifndef XENO_PC_PORT

extern u16 D_800C3E30;

/* Index of the lowest set bit among the low 13 bits of D_800C3E30 (13 if none). */
s32 func_800AF400(void) {
    s32 i;
    u16 flags = D_800C3E30;

    for (i = 0; i < 13; i++) {
        if ((flags >> i) & 1) {
            break;
        }
    }
    return i;
}

#endif
