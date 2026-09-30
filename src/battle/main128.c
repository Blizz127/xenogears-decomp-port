#include "common.h"


#ifndef XENO_PC_PORT
extern u32 D_800C3610;
#endif
extern u32 D_80059464;
#ifndef XENO_PC_PORT
extern u32 D_800D2D68;
#endif
extern void func_800BD2E4(void);
extern u32 func_800BF720(void);


/* func_800BF6CC.s: call the handler only while the D_800C3610 latch is set. */
void func_800BF6CC(void) {
    if (D_800C3610 != 0) {
        func_800BD2E4();
    }
}
/* func_800BF6F8.s: D_80059464 - func_800BF720(); retail loads the global after
 * the call, so the call result is materialised first. */
u32 func_800BF6F8(void) {
    u32 marker = func_800BF720();

    return D_80059464 - marker;
}
/* func_800BF720.s: boolean view of the D_800D2D68 word (sltu $v0, $zero, $v0). */
u32 func_800BF720(void) {
#ifdef XENO_PC_PORT
    /* mainc118.c/mainc120.c declare D_800D2D68 as an array, so the guest-RAM
     * alias is a pointer there and a bare `D_800D2D68 != 0` degenerates into a
     * pointer-nonnull test that is always true. Read element 0, which is the
     * word retail loads. */
    return D_800D2D68[0] != 0;
#else
    return D_800D2D68 != 0;
#endif
}
