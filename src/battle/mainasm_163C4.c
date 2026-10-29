/* battle mainasm_163C4: the retail-asm run at file offset 0x163C4, in retail order.
 * Functions still listed as INCLUDE_ASM are not byte-matching C yet. The whole
 * file is retail-build only; any port body for these functions lives in the
 * port's own TU. */
#include "common.h"

#ifndef XENO_PC_PORT
/* func_80085EB4.s, byte-exact: when D_800C3EAC +0x2D6 is set and kind
 * is 4, find the first of the 13 D_800C3160 rows whose 7 bytes match
 * D_800C3EAC +0x2CC..+0x2D2 (k = 13 when none does).  For a match at row
 * k, the fallthrough switch runs i++ 12 - k times, and the function
 * returns 1 when (u16)func_80089C6C(mask, byte 0xC - i, i.e. byte k, of
 * the D_800C31AC entry picked by the actor's D_800CCD3E byte) is nonzero
 * (mask = the g_GameState +0x16C0 halfword of 32-byte record
 * D_800D2D24[actor]).  Otherwise it returns 0.  jtbl_80070280 is this
 * TU's .rodata (config/battle.yaml).
 * Codegen:
 * - The inner loop body opens with a block-local (u8 v).  In the RTL
 *   dump the body then starts with a block-begin note and the loop keeps
 *   retail's layout: compare at the top, i < 7 at the bottom.  Without a
 *   declaration there, expand_end_loop rotates the loop (an entry jump
 *   into the middle; 368-376 vs 372).
 * - match = 0 is set in the break arm, so match shares v1 with the
 *   compared byte, as retail.
 * - The block-local c = ctx is retail's ctx copy in t1, and (c + i)
 *   puts ctx first in the address add.
 * - match = k before the switch: retail switches on a copy of k in v1.
 *   With switch (k), or with an uninitialised fresh n = k, CSE
 *   replaces the switch operand by k and the copy is gone (368 vs 372);
 *   an n initialised at its declaration (s32 n = 0;) also matches.
 * - Byte 0xC - i is read as [0xC - i] off the entry, which keeps
 *   retail's separate addiu 0xC. */
extern u8* D_800C3EAC;
extern u8* D_800C3160[];
extern u8* D_800C31AC[];
extern u8 D_800CCD3E[];
extern u8 D_800D2D24[];
extern u8 g_GameState[];
extern u32 func_80089C6C(u32 mask, u8 index);

s32 func_80085EB4(u8 kind, u8 actor) {
    u8* ctx = D_800C3EAC;
    s32 result = 0;

    if (ctx[0x2D6] != 0) {
        if (kind == 4) {
            s32 k = 0;
            u8* c = ctx;
            u8** tp = D_800C3160;
            s32 i = 0;
            s32 match;

            do {
                u8* p = *tp;

                do {
                    u8 v = (c + i)[0x2CC];

                    if (v != *p) {
                        match = 0;
                        break;
                    }
                    match = 1;
                    i++;
                    p++;
                } while (i < 7);
                i = 0;
                if (match) {
                    break;
                }
                k++;
                tp++;
            } while (k < 13);
            match = k;
            switch (match) {
            case 0:
                i++;
            case 1:
                i++;
            case 2:
                i++;
            case 3:
                i++;
            case 4:
                i++;
            case 5:
                i++;
            case 6:
                i++;
            case 7:
                i++;
            case 8:
                i++;
            case 9:
                i++;
            case 10:
                i++;
            case 11:
                i++;
            case 12:
                if ((u16)func_80089C6C(*(u16*)(g_GameState + 0x16C0 + D_800D2D24[actor] * 32),
                                       D_800C31AC[D_800CCD3E[actor * 368]][0xC - i])) {
                    result = 1;
                }
                break;
            }
        }
    }
    return result;
}
#endif
