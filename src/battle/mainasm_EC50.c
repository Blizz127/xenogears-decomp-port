/* battle mainasm_EC50: the retail-asm run at file offset 0xEC50, in retail order.
 * Functions still listed as INCLUDE_ASM are not byte-matching C yet. The whole
 * file is retail-build only; any port body for these functions lives in the
 * port's own TU. */
#include "common.h"

#ifndef XENO_PC_PORT
extern u32 D_800D3410[];

/* func_8007E740: row[(idx&0xFF)] table mul: T[p[1]] = T[p[1]] * p[2].
 * Sibling of func_8007E6A0/func_8007E6F0 in src/battle/mainc29.c (same
 * D_800D3410 row-table shape), but only pinning the mult result to $2
 * (v0) gives byte-exact reg alloc here; "base" is left as a plain local
 * (pinning it too, alongside the result, makes gcc hoist the row-table
 * lui/addiu ahead of the andi/sll and breaks the match). */
void func_8007E740(u8 **pp, s32 idx) {
    register u32 result asm("$2");
    u32 row;
    u32 *base;
    u8 *p;
    u32 *t;

    row = (idx & 0xFF) << 6;
    base = D_800D3410;
    p = *pp;
    t = (u32 *)((u8 *)base + row);

    result = p[2] * t[p[1]];
    t[p[1]] = result;
}

#endif
