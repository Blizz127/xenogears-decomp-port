#include "common.h"


#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/mainl81", func_800B39C0);
#endif


#ifndef XENO_PC_PORT
extern u32 D_800C3558;
#endif


/* func_800B3B6C.s */
u32 func_800B3B6C(void) {
#ifdef XENO_PC_PORT
    /* Another TU declares D_800C3558 as a u8*, so the guest-RAM alias loads the
     * word and translates it: a bare `D_800C3558 != 0` is a pointer-nonnull test
     * that can never be false, and `D_800C3558 + 0x41` would be a host pointer.
     * Retail loads the word, branches on it, and dereferences 0x41 into the
     * guest address. */
    u32 p = *(u32 *)PSX_ADDR(0x800C3558);

    if (p != 0) {
        return *(u8 *)PSX_ADDR(p + 0x41);
    }
    return 1;
#else
    if (D_800C3558 != 0) {
        return *(u8*)(D_800C3558 + 0x41);
    }
    return 1;
#endif
}
