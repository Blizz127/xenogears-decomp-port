/* battle mainasm_AD84: the retail-asm run at file offset 0xAD84, in retail order.
 * Functions still listed as INCLUDE_ASM are not byte-matching C yet. */
#include "common.h"

extern u8 D_800D3430[];

u8 func_8007A874(u8 **pp, u8 *dst, s32 row, s32 col) {
    u8 *p = *pp;
    u32 c = ((col & 0xFF) << 3) + p[1];
    u8 *src = D_800D3430 + ((row & 0xFF) << 6);
    u8 value = src[p[2]];

    dst[c] = value;
    return value;
}
