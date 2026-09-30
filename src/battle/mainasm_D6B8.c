/* battle mainasm_D6B8: the retail-asm run at file offset 0xD6B8, in retail order.
 * Functions still listed as INCLUDE_ASM are not byte-matching C yet. The whole
 * file is retail-build only; any port body for these functions lives in the
 * port's own TU. */
#include "common.h"

#ifndef XENO_PC_PORT
#include "main/game.h"

extern u32 D_800D3410[];

void func_8007D1A8(u8 **pp, s32 idx) {
    u32 *t = (u32 *)((u8 *)D_800D3410 + ((idx & 0xFF) << 6));

    t[(*pp)[1]] = g_GameState.gold;
}

#endif
