/* battle mainasm_419DC: the retail run at file offset 0x419DC, split off the
 * front of main75: func_800B14CC is non-CDK code (it encodes its constants
 * as `ori`; 2.7.2 and 2.6.3 give the same code, built here under the
 * Default fallback), where main75's func_800B15D8 matches only under the
 * CDK cc1.  The whole file is retail-build only; the port body for
 * func_800B14CC lives in main75.c. */
#include "common.h"

#ifndef XENO_PC_PORT
extern u8* D_800D3368[];
extern u8 D_800C3D0C[];
extern void func_800AA934(u8* a0, u8* a1, u8* a2, u32 a3);
extern void func_800BE790(void);
extern void func_800A9FF0(u32 v);

/* func_800B14CC.s, byte-exact: for each of the three party slots other
 * than `self`, run func_800AA934(actor, actor, D_800C3D0C, 5) on its
 * D_800D3368 record; then wait (func_800BE790 per pass) until none of
 * those records has its +0x34 or +0x38 byte set, wait one more frame and
 * run func_800A9FF0 on each of those slots. */
void func_800B14CC(s32 self) {
    s32 i;
    s32 busy;

    for (i = 0; i < 3; i++) {
        if (i != self) {
            func_800AA934(D_800D3368[i], D_800D3368[i], D_800C3D0C, 5);
        }
    }
    do {
        busy = 0;
        for (i = 0; i < 3; i++) {
            if (i != self && D_800D3368[i] != NULL && (D_800D3368[i][0x34] != 0 || D_800D3368[i][0x38] != 0)) {
                busy = 1;
            }
        }
        func_800BE790();
    } while (busy != 0);
    func_800BE790();
    for (i = 0; i < 3; i++) {
        if (i != self) {
            func_800A9FF0(i);
        }
    }
}
#endif
