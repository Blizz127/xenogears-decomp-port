/* battle mainasm_BFF8: the retail-asm run at file offset 0xBFF8, in retail order.
 * Functions still listed as INCLUDE_ASM are not byte-matching C yet. The whole
 * file is retail-build only; any port body for these functions lives in the
 * port's own TU. */
#include "common.h"

extern u8 D_800CCE34[];

void func_8007BAE8(u8** a0, s32 a1) {
    u8* p;
    s32 hi;

    a1 = (a1 & 0xFF) + 3;
    p = *a0;
    hi = p[2] << 8;
    *(u32*)(D_800CCE34 + a1 * 0x170) = p[1] | hi;
}
