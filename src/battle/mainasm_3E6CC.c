/* battle mainasm_3E6CC: the retail-asm run at file offset 0x3E6CC, in retail order.
 * Functions still listed as INCLUDE_ASM are not byte-matching C yet. The whole
 * file is retail-build only; any port body for these functions lives in the
 * port's own TU. */
#include "common.h"

#ifndef XENO_PC_PORT
void func_800AE1BC(u8* a0, u8* a1, s32 a2) {
    u8* p;

    if (*(u16*)(a1 + 0x12) != 0) {
        *(s16*)(a0 + 0x98) = 0;
        if (a2 != 0) {
            *(u16*)(a0 + 0x9A) = *(u16*)(a1 + 0x2);
        } else {
            *(s16*)(a0 + 0x9A) = -1;
        }
        *(s16*)(a0 + 0x9C) = 0;
        *(u16*)(a0 + 0x9E) = *(u16*)(a1 + 0x12);
        p = a1 + *(s32*)(a1 + 0x14);
        *(u8**)(a0 + 0xA0) = p;
        *(u8**)(a0 + 0xA4) = p;
        return;
    }
    *(s16*)(a0 + 0x98) = -1;
}

extern u8* D_8005919C;
extern u8* D_800C4924;
s32 func_800AE220(u8* a0, s32 a1) {
    if (a1 == 0) {
        return *(u16*)(D_8005919C + 0x14) << 16;
    }
    if (a1 == 1) {
        return *(u16*)(*(u8**)(*(u8**)(a0 + 0xB0) + 8) + 0x14) << 16;
    }
    if (a1 == 2) {
        return *(u16*)(*(u8**)(*(u8**)(a0 + 0xB4) + 8) + 0x14) << 16;
    }
    if (a1 == 3) {
        return *(u16*)(D_800C4924 + 0x14) << 16;
    }
}

#endif
