#include "common.h"


#ifndef XENO_PC_PORT
extern u8 D_800D3638[];
extern u8 D_800C355C[];
extern u8* D_800C3558__p[] asm("D_800C3558");
extern u8* D_800C3554[];
extern u8 D_800C3C00[];
extern u8 D_800C3C50[];
extern u8 D_800591AC[];
extern s32 D_80059464[];
extern void TimerWorkListAddTask(u32 list, void* task);
extern void WorkListAddTask(void* owner, void* task);
extern void TimerWorkListSetTaskCallback(void* task, void* callback);
extern void WorkListSetTaskCallback(void* task, void* callback);
extern void WorkListTaskSetOnFreeCallback(void* task, void* callback);
extern void func_800B36BC(u8* fx);
extern void func_800B3878(void);
extern void func_800B383C(u8* p);

/* func_800B39C0.s: unless D_800D3638 is set, start or retarget a colour
 * effect record: D_800C3C00 (kept in D_800C3554) while D_800C355C is
 * set, else D_800C3C50 (kept in D_800C3558).  A new record is added to the
 * timer and work lists with func_800B36BC / func_800B3878 / func_800B383C
 * as callbacks and its current colour (+0x45..+0x47) zeroed; an existing one
 * restarts from its +0x48..+0x4A colour.  Then store kind and the target
 * colour at +0x41..+0x44, 2 * ticks at +0x38 and +0x3C, and run
 * func_800B36BC once.  `kind` is an int: as a u8 it takes a different
 * callee-saved register than retail. */
void func_800B39C0(s32 ticks, s32 kind, u8 r, u8 g, u8 b) {
    u8* fx;
    u8 paused;

    if (D_800D3638[0] != 0) {
        return;
    }
    ticks *= 2;
    if (D_800C355C[0] != 0) {
        if (D_800C3558__p[0] == NULL) {
            fx = D_800C3C00;
            D_800C3554[0] = fx;
            goto init;
        }
        fx = D_800C3554[0];
        goto existing;
    }
    if (D_800C3558__p[0] != NULL) {
        fx = D_800C3558__p[0];
        goto existing;
    }
    fx = D_800C3C50;
    D_800C3558__p[0] = fx;
init:
    TimerWorkListAddTask(0, fx);
    WorkListAddTask(fx, fx + 0x1C);
    paused = D_800591AC[0];
    *(u32*)(fx + 0x14) &= 0x7FFFFFFF;
    if (paused != 0) {
        D_80059464[0] -= 1;
    }
    TimerWorkListSetTaskCallback(fx, func_800B36BC);
    WorkListSetTaskCallback(fx + 0x1C, func_800B3878);
    WorkListTaskSetOnFreeCallback(fx, func_800B383C);
    *(u8**)(fx + 4) = fx;
    *(u8**)(fx + 0x20) = fx;
    fx[0x40] = 0;
    fx[0x45] = 0;
    fx[0x46] = 0;
    fx[0x47] = 0;
    goto common;
existing:
    fx[0x45] = fx[0x48];
    fx[0x46] = fx[0x49];
    fx[0x47] = fx[0x4A];
common:
    fx[0x41] = kind;
    fx[0x42] = r;
    fx[0x43] = g;
    fx[0x44] = b;
    *(s32*)(fx + 0x38) = ticks;
    *(s32*)(fx + 0x3C) = ticks;
    func_800B36BC(fx);
}
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
