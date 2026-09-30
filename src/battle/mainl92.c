#include "common.h"


#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/mainl92", func_800B6A7C);
#endif
extern void func_801FC53C(u32 a0, s32 a1, s32 a2, s32 a3, s32 s0, s32 s1, s32 s2);
/* func_800B6B98.s: follow the record's signed 16-bit LE self-offset, then call
 * func_801FC53C(a0, r[0] << 4, r[1], (s8)r[2] << 3, r[3] << 3, (s8)r[4] << 3, r[5]). */
void func_800B6B98(u32 a0, u8* a1) {
    u8* r = a1 + (((s32)(s8)a1[1] << 8) | a1[0]);

    func_801FC53C(a0, r[0] << 4, r[1], (s32)(s8)r[2] << 3, r[3] << 3, (s32)(s8)r[4] << 3, r[5]);
}


typedef struct { s16 x; s16 y; s16 w; s16 h; } RECT;
extern int MoveImage(RECT* rect, int x, int y);
extern void func_800B73A0(void);


/* func_800B6BFC.s */
void func_800B6BFC(void) {
    RECT r;

    r.x = 0;
    r.y = 0;
    r.w = 0x140;
    r.h = 0xE0;
    MoveImage(&r, 0x2C0, 0x100);
    func_800B73A0();
}
