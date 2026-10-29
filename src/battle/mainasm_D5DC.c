/* battle mainasm_D5DC: the retail-asm run at file offset 0xD5DC, in retail order.
 * Functions still listed as INCLUDE_ASM are not byte-matching C yet. The whole
 * file is retail-build only; any port body for these functions lives in the
 * port's own TU. */
#include "common.h"

#ifndef XENO_PC_PORT
extern u8 D_800D3430[];
extern u8 D_800D2CE0[];
extern u8 D_800D2CB0[];

/* func_8007D0CC.s, byte-exact: row[p[1]] = D_800D2CB0[i] for the first
 * i < 48 with D_800D2CE0[i] == p[2], after clearing row[p[1]].  row is the
 * offset with D_800D3430 added in place: as a two-set register sched1 does
 * not boost it, and `i = 0` lands in the p[1] load slot as in retail.
 * The do/while(0) around the cursor load has no semantic effect.  It is an
 * allocation-priority construct: its loop note raises the reference weight
 * of p (6 -> 7), so global-alloc assigns p before i ($a2/$a3, as retail).
 * Without it the same code has those two registers swapped (8/31).  Found
 * by decomp-permuter; revisit for a natural form. */
void func_8007D0CC(u8** pp, u8 idx) {
    u32 row;
    u8* p;
    s32 i;

    row = (u32)idx << 6;
    row += (u32)D_800D3430;
    *(u8*)(row + (*pp)[1]) = 0;
    for (i = 0; i < 0x30; i++) {
        do { p = *pp; } while (0);
        if (D_800D2CE0[i] == p[2]) {
            *(u8*)(row + p[1]) = D_800D2CB0[i];
            break;
        }
    }
}
#endif
