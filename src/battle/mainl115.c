#include "common.h"

/* func_800BC460 now lives in src/battle/mainc115.c (BattleCdk TU). */

#ifndef XENO_PC_PORT
extern u8 D_800C37C8;
#endif
extern void func_800BC2F0(s32 v);


/* func_800BCAA4.s */
void func_800BCAA4(void) {
    if (D_800C37C8 == 0) {
        func_800BC2F0(4);
    }
}
/* func_800BCAD0.s */
void func_800BCAD0(void) {
    if (D_800C37C8 == 0) {
        func_800BC2F0(1);
    }
}
