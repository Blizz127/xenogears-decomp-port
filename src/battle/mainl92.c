#include "common.h"


#ifndef XENO_PC_PORT
extern s32 func_80022CAC(u8* actor, s32 v);
extern void func_801FC4C4(u32 a0, u32 a1, u8* a2, s32 a3, s32 a4, s32 a5, s32 a6, s32 a7);

/* func_800B6A7C.s, byte-exact: decode the parameter record at a signed
 * 16-bit offset in `args` (p) and hand three values to func_801FC4C4 with
 * the actor node's (+0x20) +0x34/+0x2C words and +0xC: with +0x3A set they
 * are func_80022CAC(actor, (s8)p[1] << 3 | p[0]) << 8 and
 * func_80022CAC(actor, p[2] / p[3]) << 16, otherwise the raw values << 5 /
 * << 13 / << 13; p[4] << 4 and p[5] << 2 follow on the stack.  The node's
 * +0x34/+0x2C/+0x30 words are cleared afterwards. */
void func_800B6A7C(u8* actor, u8* args) {
    u8* p = args + (((s8)args[1] << 8) | args[0]);
    s32 a;
    s32 b;
    s32 c;
    u8* node;

    if (*(u16*)(actor + 0x3A) != 0) {
        a = func_80022CAC(actor, ((s8)p[1] << 3) | p[0]) << 8;
        b = func_80022CAC(actor, p[2]) << 16;
        c = func_80022CAC(actor, p[3]) << 16;
    } else {
        a = (((s8)p[1] << 3) | p[0]) << 5;
        b = p[2] << 13;
        c = p[3] << 13;
    }
    node = *(u8**)(actor + 0x20);
    func_801FC4C4(*(u32*)(node + 0x34), *(u32*)(node + 0x2C), node + 0xC, a, b, c, p[4] << 4, p[5] << 2);
    *(u32*)(*(u8**)(actor + 0x20) + 0x34) = 0;
    *(u32*)(*(u8**)(actor + 0x20) + 0x2C) = 0;
    *(u32*)(*(u8**)(actor + 0x20) + 0x30) = 0;
}
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
